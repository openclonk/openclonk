/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Handles engine execution in developer mode */

#include "C4Include.h"
#include "editor/C4Console.h"

#include "C4Version.h"
#include "control/C4GameControl.h"
#include "control/C4GameSave.h"
#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "gui/C4MessageInput.h"
#include "landscape/C4Landscape.h"
#include "object/C4Def.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "script/C4ScriptHost.h"

#include "platform/StdRegistry.h"

#define FILE_SELECT_FILTER_FOR_C4S "OpenClonk Scenario\0"         \
                                   "*.ocs *.ocf Scenario.txt\0" \
                                   "\0"

using namespace OpenFileFlags;

C4Console::C4Console(): C4ConsoleGUI()
{
	Active = false;
	Editing = true;
	FrameCounter=0;
	fGameOpen=false;

#ifdef USE_WIN32_WINDOWS
	hWindow=nullptr;
#endif
}

C4Console::~C4Console() = default;

C4Window * C4Console::Init(C4AbstractApp * pApp)
{
	if (!C4ConsoleGUI::CreateConsoleWindow(pApp))
		return nullptr;
	return this;
}

bool C4Console::In(const char *szText)
{
	if (!Active || !szText) return false;
	// begins with '/'? then it's a command
	if (*szText == '/')
	{
		::MessageInput.ProcessCommand(szText);
		// done
		return true;
	}
	// begins with '#'? then it's a message. Route via ProcessInput to allow #/sound
	// Also, in the lobby, everything written here is still a message
	bool is_chat_command = (*szText == '#');
	if (is_chat_command || (::Network.isEnabled() && !::Network.Status.isPastLobby()))
	{
		::MessageInput.ProcessInput(szText + is_chat_command);
		return true;
	}
	// editing enabled?
	if (!EditCursor.EditingOK()) return false;
	// pass through network queue
	::Control.DoInput(CID_Script, new C4ControlScript(szText, C4ControlScript::SCOPE_Console), CDT_Decide);
	return true;
}

// Someone defines Status as int....
#ifdef Status
#undef Status
#endif

void C4Console::DoPlay()
{
	Game.Unpause();
}

void C4Console::DoHalt()
{
	Game.Pause();
}

void C4Console::UpdateStatusBars()
{
	if (!Active) return;
	// Frame counter
	if (Game.FrameCounter!=FrameCounter)
	{
		FrameCounter=Game.FrameCounter;
		StdStrBuf str;
		str.Format("Frame: %i",FrameCounter);
		C4ConsoleGUI::DisplayInfoText(CONSOLE_FrameCounter, str);
	}
	// Time & FPS
	if ((Game.Time!=Time) || (Game.FPS!=FPS))
	{
		Time=Game.Time;
		FPS=Game.FPS;
		StdStrBuf str;
		str.Format("%02d:%02d:%02d (%i FPS)",Time/3600,(Time%3600)/60,Time%60,FPS);
		C4ConsoleGUI::DisplayInfoText(CONSOLE_TimeFPS, str);
	}
}

bool C4Console::SaveGame(const char * path)
{
	// Network hosts only
	if (::Network.isEnabled() && !::Network.isHost())
		{ Message(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return false; }

	// Save game to open scenario file
	bool fOkay=true;
	SetCursor(C4ConsoleGUI::CURSOR_Wait);

	C4GameSave *pGameSave = new C4GameSaveSavegame();
	if (!pGameSave->Save(path))
		{ Out("Save failed"); fOkay=false; }
	delete pGameSave;

	SetCursor(C4ConsoleGUI::CURSOR_Normal);

	// Status report
	if (!fOkay) Message(LoadResStr("IDS_CNS_SAVERROR"));
	else Out(LoadResStr("IDS_CNS_GAMESAVED"));

	return fOkay;
}

bool C4Console::SaveScenario(const char * path, bool export_packed)
{
	C4Group *save_target_group = &Game.ScenarioFile;
	C4Group export_group;
	if (export_packed)
	{
		// Export to packed file: Delete existing
		if (FileExists(path))
		{
			if (ItemIdentical(Game.ScenarioFilename, path) || !EraseItem(path))
			{
				Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"), path).getData());
				return false;
			}
		}
		// Write into new, packed copy
		if (!C4Group_PackDirectoryTo(Game.ScenarioFilename, path) || !export_group.Open(path))
		{
			Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"), path).getData());
			return false;
		}
		save_target_group = &export_group;
	}
	else if (path)
	{
		// When trying to save into a subfolder of the existing scenario, the copy function
		// below will try to recursively copy everything until it blows the stack. There is
		// really no good reason to do this, so we just disallow it here.
		if (SEqual2(path, Game.ScenarioFilename))
		{
			Message(LoadResStr("IDS_CNS_RECURSIVESAVEASERROR"));
			return false;
		}
		// Open new scenario file
		// Close current scenario file to allow re-opening at new path
		Game.ScenarioFile.Close();
		// Copy current scenario file to target
		if (!C4Group_CopyItem(Game.ScenarioFilename,path))
		{
			Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"),path).getData());
			return false;
		}
		// Re-open at new path (unless exporting, in which case the export is just a copy)
		SCopy(path, Game.ScenarioFilename, _MAX_PATH);
		SetCaptionToFilename(Game.ScenarioFilename);
		if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
		{
			Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"), Game.ScenarioFilename).getData());
			return false;
		}
	}
	else
	{
		// Do not save to temp network file
		if (Game.TempScenarioFile && ItemIdentical(Game.TempScenarioFile.getData(), Game.ScenarioFilename))
		{
			Message(LoadResStr("IDS_CNS_NONETREFSAVE"));
			return false;
		}
	}

	// Can't save to child groups
	if (save_target_group->GetMother() && save_target_group->GetMother()->IsPacked())
	{
		StdStrBuf str;
		str.Format(LoadResStr("IDS_CNS_NOCHILDSAVE"),
		           GetFilename(save_target_group->GetName()));
		Message(str.getData());
		return false;
	}

	// Save game to open scenario file
	SetCursor(C4ConsoleGUI::CURSOR_Wait);

	bool fOkay=true;
	C4GameSave *pGameSave = new C4GameSaveScenario(!Console.Active || ::Landscape.GetMode() == LandscapeMode::Exact, false);
	if (!pGameSave->Save(*save_target_group, false))
		{ Out("Game::Save failed"); fOkay=false; }
	delete pGameSave;

	// Close and reopen scenario file to fix file changes
	if (!export_packed)
	{
		if (!Game.ScenarioFile.Close())
		{
			Out("ScenarioFile::Close failed"); fOkay = false;
		}
		if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
		{
			Out("ScenarioFile::Open failed"); fOkay = false;
		}
	}

	SetCursor(C4ConsoleGUI::CURSOR_Normal);

	// Initialize/script notification
	if (Game.fScriptCreatedObjects)
	{
		StdStrBuf str(LoadResStr("IDS_CNS_SCRIPTCREATEDOBJECTS"));
		str += LoadResStr("IDS_CNS_WARNDOUBLE");
		Message(str.getData());
		Game.fScriptCreatedObjects = false;
	}

	// Status report
	if (!fOkay) Message(LoadResStr("IDS_CNS_SAVERROR"));
	else Out(LoadResStr("IDS_CNS_SCENARIOSAVED"));

	return fOkay;
}

bool C4Console::FileSave()
{
	// Save game
	// FIXME: What about editing a savegame inplace? (Game.C4S.Head.SaveGame)
	return SaveScenario(nullptr);
}

bool C4Console::FileSaveAs(bool fSaveGame, bool export_packed)
{
	// Do save-as dialog
	StdCopyStrBuf filename("");
	filename.Copy(Game.ScenarioFile.GetName());
	if (export_packed)
	{
		RemoveExtension(&filename);
		filename.Append("_packed.ocs");
	}
	if (!FileSelect(&filename,
	                "OpenClonk Scenario\0*.ocs\0\0",
	                OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
	                true)) return false;
	DefaultExtension(&filename,"ocs");
	if (!export_packed) ::Config.Developer.AddRecentlyEditedScenario(filename.getData());
	if (fSaveGame)
		// Save game
		return SaveGame(filename.getData());
	else
		return SaveScenario(filename.getData(), export_packed);
}

bool C4Console::Message(const char *szMessage, bool fQuery)
{
	if (!Active) return false;
	return C4ConsoleGUI::Message(szMessage, fQuery);
}

bool C4Console::FileNew()
{
	StdCopyStrBuf filename;
#ifdef WITH_QT_EDITOR
	bool host_in_network = false;
	if (!C4ConsoleGUI::CreateNewScenario(&filename, &host_in_network)) return false;
	Application.ClearCommandLine();
	::Config.Developer.AddRecentlyEditedScenario(filename.getData());
	if (host_in_network)
	{
		Game.NetworkActive = true;
		Game.fLobby = true;
	}
	Application.OpenGame(filename.getData());
	return true;
#endif
	// Not implemented
	return false;

}

bool C4Console::FileOpen(const char *filename, bool host_in_network)
{
	// Get scenario file name
	StdCopyStrBuf c4sfile("");
	if (!filename)
	{
		if (!FileSelect(&c4sfile,
			FILE_SELECT_FILTER_FOR_C4S,
			OFN_HIDEREADONLY | OFN_FILEMUSTEXIST))
			return false;
		filename = c4sfile.getData();
	}
	Application.ClearCommandLine();
	::Config.Developer.AddRecentlyEditedScenario(filename);
	if (host_in_network)
	{
		Game.NetworkActive = true;
		Game.fLobby = true;
	}
	// Open game
	Application.OpenGame(filename);
	return true;
}

bool C4Console::FileOpenWPlrs()
{
	// Get scenario file name
	StdCopyStrBuf c4sfile("");
	if (!FileSelect(&c4sfile,
	                FILE_SELECT_FILTER_FOR_C4S,
	                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST))
		return false;
	// Get player file name(s)
	StdCopyStrBuf c4pfile("");
	if (!FileSelect(&c4pfile,
	                "OpenClonk Player\0*.ocp\0\0",
	                OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	               )) return false;
	// Compose command line
	Application.ClearCommandLine();
	if (DirectoryExists(c4pfile.getData())) // Multiplayer
	{
		const char *cptr = c4pfile.getData() + SLen(c4pfile.getData()) + 1;
		while (*cptr)
		{
			char c4pfile2[512 + 1] = "";
			SAppend(c4pfile.getData(), c4pfile2, 512);
			SAppend(DirSep, c4pfile2, 512);
			SAppend(cptr, c4pfile2, 512);
			SAddModule(Game.PlayerFilenames, c4pfile2);
			cptr += SLen(cptr) + 1;
		}
	}
	else // Single player
	{
		SAddModule(Game.PlayerFilenames, c4pfile.getData());
	}
	::Config.Developer.AddRecentlyEditedScenario(c4sfile.getData());
	// Open game
	Application.OpenGame(c4sfile.getData());
	return true;
}

bool C4Console::FileClose()
{
	if (!fGameOpen) return false;
	Application.QuitGame();
	return true;
}

bool C4Console::FileSelect(StdStrBuf * sFilename, const char * szFilter, DWORD dwFlags, bool fSave)
{
	return C4ConsoleGUI::FileSelect(sFilename, szFilter, dwFlags, fSave);
}

bool C4Console::FileRecord()
{
	// only in running mode
	if (!Game.IsRunning || !::Control.IsRuntimeRecordPossible()) return false;
	// start record!
	::Control.RequestRuntimeRecord();
	// disable menuitem
	RecordingEnabled();
	return true;
}

void C4Console::ClearPointers(C4Object *pObj)
{
	EditCursor.ClearPointers(pObj);
	C4ConsoleGUI::ClearPointers(pObj);
}

void C4Console::Default()
{
	EditCursor.Default();
	PropertyDlgObject = nullptr;
	ToolsDlg.Default();
}

void C4Console::Clear()
{
	if (pSurface) delete pSurface;
	pSurface = nullptr;

	C4Window::Clear();
	C4ConsoleGUI::DeleteConsoleWindow();

	EditCursor.Clear();
	ToolsDlg.Clear();
	PropertyDlgClose();
	ClearViewportMenu();
	ClearPlayerMenu();
	ClearNetMenu();
#ifndef _WIN32
	Application.Quit();
#endif
}

void C4Console::Close()
{
	C4ConsoleGUI::CloseConsoleWindow();
	Application.Quit();
}

bool C4Console::FileQuit()
{
	Close();
	return true;
}

void C4Console::HelpAbout()
{
	StdStrBuf strCopyright;
	strCopyright.Format("Copyright (c) %s %s", C4COPYRIGHT_YEAR, C4COPYRIGHT_COMPANY);
	ShowAboutWithCopyright(strCopyright);
}

void C4Console::ViewportNew()
{
	if (!fGameOpen) return;
	::Viewports.CreateViewport(NO_OWNER);
}

bool C4Console::UpdateViewportMenu()
{
	if (!Active) return false;
	ClearViewportMenu();
	for (C4Player *pPlr=::Players.First; pPlr; pPlr=pPlr->Next)
	{
		StdStrBuf sText;
		sText.Format(LoadResStr("IDS_CNS_NEWPLRVIEWPORT"),pPlr->GetName());
		C4ConsoleGUI::AddMenuItemForPlayer(pPlr, sText);
	}
	return true;
}

void C4Console::ClearViewportMenu()
{
	if (!Active) return;
	C4ConsoleGUI::ClearViewportMenu();
}

void C4Console::UpdateInputCtrl()
{
	// add global and standard functions
	std::list <const char*> functions = ::Console.GetScriptSuggestions(::GameScript.ScenPropList._getPropList(), C4Console::MRU_Scenario);
	SetInputFunctions(functions);
}

bool C4Console::UpdatePlayerMenu()
{
	if (!Active) return false;
	ClearPlayerMenu();
	for (C4Player *pPlr=::Players.First; pPlr; pPlr=pPlr->Next)
	{
		StdStrBuf sText;
		if (::Network.isEnabled())
			sText.Format(LoadResStr("IDS_CNS_PLRQUITNET"),pPlr->GetName(),pPlr->AtClientName);
		else
			sText.Format(LoadResStr("IDS_CNS_PLRQUIT"),pPlr->GetName());
		AddKickPlayerMenuItem(pPlr, sText, (!::Network.isEnabled() || ::Network.isHost()) && Editing);
	}
	return true;
}

void C4Console::UpdateMenus()
{
	if (!Active) return;
	EnableControls(fGameOpen);
	UpdatePlayerMenu();
	UpdateViewportMenu();
	UpdateNetMenu();
}

void C4Console::PlayerJoin()
{
	// Get player file name(s)
	StdCopyStrBuf c4pfile("");
	if (!FileSelect(&c4pfile,
	                "OpenClonk Player\0*.ocp\0\0",
	                OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	               )) return;

	// Multiple players
	if (DirectoryExists(c4pfile.getData()))
	{
		const char *cptr = c4pfile.getData() + SLen(c4pfile.getData()) + 1;
		while (*cptr)
		{
			StdStrBuf f;
			f.Copy(c4pfile.getData());
			f.AppendBackslash(); f.Append(cptr);
			cptr += SLen(cptr)+1;
			::Players.JoinNew(f.getData());
		}
	}
	// Single player
	else
	{
		::Players.JoinNew(c4pfile.getData());
	}
}

void C4Console::UpdateNetMenu()
{
	// Active & network hosting check
	if (!Active) return;
	if (!::Network.isHost() || !::Network.isEnabled()) return;
	// Clear old
	ClearNetMenu();
	// Insert menu
	C4ConsoleGUI::AddNetMenu();

	// Host
	StdStrBuf str;
	str.Format(LoadResStr("IDS_MNU_NETHOST"),Game.Clients.getLocalName(),Game.Clients.getLocalID());
	AddNetMenuItemForPlayer(Game.Clients.getLocalID(), str.getData(), C4ConsoleGUI::CO_None);
	// Clients
	for (C4Network2Client *pClient=::Network.Clients.GetNextClient(nullptr); pClient; pClient=::Network.Clients.GetNextClient(pClient))
	{
		if (pClient->isHost()) continue;
		str.Format(LoadResStr(pClient->isActivated() ? "IDS_MNU_NETCLIENT_DEACTIVATE" : "IDS_MNU_NETCLIENT_ACTIVATE"),
		           pClient->getName(), pClient->getID());
		AddNetMenuItemForPlayer(pClient->getID(), str.getData(), pClient->isActivated() ? C4ConsoleGUI::CO_Deactivate : C4ConsoleGUI::CO_Activate);
		str.Format(LoadResStr("IDS_NET_KICKCLIENTEX"), pClient->getName(), pClient->getID());
		AddNetMenuItemForPlayer(pClient->getID(), str.getData(), C4ConsoleGUI::CO_Kick);
	}
	return;
}

void C4Console::ClearNetMenu()
{
	if (!Active) return;
	C4ConsoleGUI::ClearNetMenu();
}

void C4Console::SetCaptionToFilename(const char* szFilename)
{
	SetTitle(GetFilename(szFilename));
	C4ConsoleGUI::SetCaptionToFileName(szFilename);
}

void C4Console::Execute()
{
	C4ConsoleGUI::Execute();
	EditCursor.Execute();
	ObjectListDlg.Execute();
	UpdateStatusBars();
	::GraphicsSystem.Execute();
}

void C4Console::InitGame()
{
	if (!Active) return;
	// Default game dependent members
	Default();
	SetCaptionToFilename(Game.ScenarioFilename);
	// Init game dependent members
	EditCursor.Init();

	// Console updates
	fGameOpen=true;
	UpdateInputCtrl();
	EnableControls(fGameOpen);
	UpdatePlayerMenu();
	UpdateViewportMenu();
	// Initial neutral viewport unless started with players
	if (!Game.PlayerInfos.GetStartupCount()) ::Viewports.CreateViewport(NO_OWNER);
}

void C4Console::CloseGame()
{
	if (!Active || !fGameOpen) return;
	fGameOpen=false;
	EnableControls(fGameOpen);
	SetTitle(LoadResStr("IDS_CNS_CONSOLE"));
}

bool C4Console::TogglePause()
{
	return Game.TogglePause();
}

std::list<const char *> C4Console::GetScriptSuggestions(C4PropList *target, RecentScriptInputLists section) const
{
	// Functions for this object
	std::list<const char *> functions = ::ScriptEngine.GetFunctionNames(target);
	// Prepend most recently used script calls in reverse order
	const std::list<StdCopyStrBuf> &mru = recent_script_input[section];
	if (!mru.empty())
	{
		functions.insert(functions.begin(), nullptr);
		// add pointers into string buffer list
		// do not iterate with for (auto i : mru) because this would copy the buffer and add stack pointers
		for (const auto & i : mru)
			functions.insert(functions.begin(), i.getData());
	}
	return functions;
}

void C4Console::RegisterRecentInput(const char *input, RecentScriptInputLists section)
{
	std::list<StdCopyStrBuf> &mru = recent_script_input[section];
	// remove previous copy (i.e.: Same input just gets pushed to top)
	mru.remove(StdCopyStrBuf(input));
	// register to list
	mru.emplace_back(input);
	// limit history length
	if (static_cast<int32_t>(mru.size()) > ::Config.Developer.MaxScriptMRU)
		mru.erase(mru.begin());
}

#if !(defined(USE_WIN32_WINDOWS) || defined(WITH_QT_EDITOR))
class C4ConsoleGUI::State: public C4ConsoleGUI::InternalState<class C4ConsoleGUI>
{
	public: State(C4ConsoleGUI *console): Super(console) {}
};
class C4ToolsDlg::State: public C4ConsoleGUI::InternalState<class C4ToolsDlg>
{
	public: State(C4ToolsDlg* dlg): Super(dlg) {}
	void Clear() {}
	void Default() {}
};
void C4ConsoleGUI::AddKickPlayerMenuItem(C4Player*, StdStrBuf&, bool) {}
void C4ConsoleGUI::AddMenuItemForPlayer(C4Player*, StdStrBuf&) {}
void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t, const char *, C4ConsoleGUI::ClientOperation) {}
void C4ConsoleGUI::AddNetMenu() {}
void C4ConsoleGUI::ToolsDlgClose() {}
bool C4ConsoleGUI::ClearLog() {return false;}
void C4ConsoleGUI::ClearNetMenu() {}
void C4ConsoleGUI::ClearPlayerMenu() {}
void C4ConsoleGUI::ClearViewportMenu() {}
bool C4ConsoleGUI::CreateConsoleWindow(C4AbstractApp * pApp)
{
	C4Rect r(0, 0, 400, 350);
	if (!C4Window::Init(C4Window::W_Console, pApp, LoadResStr("IDS_CNS_CONSOLE"), &r))
		return false;
	return true;
}
void C4ConsoleGUI::DeleteConsoleWindow() {}
void C4ConsoleGUI::DisplayInfoText(C4ConsoleGUI::InfoTextType, StdStrBuf&) {}
void C4ConsoleGUI::DoEnableControls(bool) {}
bool C4ConsoleGUI::DoUpdateHaltCtrls(bool) {return false;}
bool C4ConsoleGUI::FileSelect(StdStrBuf *, char const*, DWORD, bool) {return false;}
bool C4ConsoleGUI::Message(char const*, bool) {return false;}
void C4ConsoleGUI::Out(char const*) {}
bool C4ConsoleGUI::PropertyDlgOpen() {return false;}
void C4ConsoleGUI::PropertyDlgClose() {}
void C4ConsoleGUI::PropertyDlgUpdate(class C4EditCursorSelection &, bool) {}
void C4ConsoleGUI::RecordingEnabled() {}
void C4ConsoleGUI::SetCaptionToFileName(char const*) {}
void C4ConsoleGUI::SetCursor(C4ConsoleGUI::Cursor) {}
void C4ConsoleGUI::SetInputFunctions(std::list<const char*>&) {}
void C4ConsoleGUI::ShowAboutWithCopyright(StdStrBuf&) {}
void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(C4ToolsDlg*) {}
bool C4ConsoleGUI::ToolsDlgOpen(C4ToolsDlg*) {return false;}
void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg*, char const*) {}
void C4ConsoleGUI::ToolsDlgSelectTexture(C4ToolsDlg*, char const*) {}
void C4ConsoleGUI::ToolsDlgSelectBackMaterial(C4ToolsDlg*, char const*) {}
void C4ConsoleGUI::ToolsDlgSelectBackTexture(C4ToolsDlg*, char const*) {}
bool C4ConsoleGUI::UpdateModeCtrls(int) {return false;}
void C4ToolsDlg::EnableControls() {}
void C4ToolsDlg::InitGradeCtrl() {}
void C4ToolsDlg::NeedPreviewUpdate() {}
bool C4ToolsDlg::PopMaterial() {return false;}
bool C4ToolsDlg::PopTextures() {return false;}
void C4ToolsDlg::UpdateIFTControls() {}
void C4ToolsDlg::UpdateLandscapeModeCtrls() {}
void C4ToolsDlg::UpdateTextures() {}
void C4ToolsDlg::UpdateToolCtrls() {}
#if !defined(USE_COCOA)
bool C4Viewport::ScrollBarsByViewPosition() {return false;}
bool C4Viewport::TogglePlayerLock() {return false;}
#endif
#include "editor/C4ConsoleGUICommon.h"
#endif
