/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2004, 2008  Matthes Bender
 * Copyright (c) 2001-2002, 2004-2007  Sven Eberhardt
 * Copyright (c) 2004, 2007, 2009  Peter Wortmann
 * Copyright (c) 2005-2007, 2009-2011  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Martin Plicht
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Handles engine execution in developer mode */

#include <C4Include.h>
#include <C4Console.h>

#include <C4Application.h>
#include <C4Def.h>
#include <C4GameSave.h>
#include <C4Game.h>
#include <C4MessageInput.h>
#include <C4UserMessages.h>
#include <C4Version.h>
#include <C4Language.h>
#include <C4Player.h>
#include <C4Landscape.h>
#include <C4GraphicsSystem.h>
#include <C4Viewport.h>
#include <C4ScriptHost.h>
#include <C4PlayerList.h>
#include <C4GameControl.h>

#include <StdFile.h>
#include <StdRegistry.h>

#define FILE_SELECT_FILTER_FOR_C4S "Clonk 4 Scenario\0"         \
                                   "*.ocs;*.ocf;Scenario.txt\0" \
                                   "\0"

using namespace OpenFileFlags;

C4Console::C4Console(): C4ConsoleGUI()
{
	Active = false;
	Editing = true;
	ScriptCounter=0;
	FrameCounter=0;
	fGameOpen=false;

#ifdef _WIN32
	hWindow=NULL;
#endif
}

C4Console::~C4Console()
{
}

#if defined(USE_X11) && !defined(WITH_DEVELOPER_MODE)
void C4Console::HandleMessage (XEvent & e)
{
	// Parent handling
	C4ConsoleBase::HandleMessage(e);

	switch (e.type)
	{
	case FocusIn:
		Application.Active = true;
		break;
	case FocusOut:
		Application.Active = false;
		break;
	}
}
#endif // USE_X11

CStdWindow * C4Console::Init(CStdApp * pApp)
{
	return C4ConsoleGUI::CreateConsoleWindow(pApp);
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
	// begins with '#'? then it's a message. Route cia ProcessInput to allow #/sound
	if (*szText == '#')
	{
		::MessageInput.ProcessInput(szText + 1);
		return true;
	}
	// editing enabled?
	if (!EditCursor.EditingOK()) return false;
	// pass through network queue
	::Control.DoInput(CID_Script, new C4ControlScript(szText, C4ControlScript::SCOPE_Console, false), CDT_Decide);
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
	// Script counter
	if (::GameScript.Counter!=ScriptCounter)
	{
		ScriptCounter=::GameScript.Counter;
		StdStrBuf str;
		str.Format("Script: %i",ScriptCounter);
		C4ConsoleGUI::DisplayInfoText(CONSOLE_ScriptCounter, str);
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

bool C4Console::SaveScenario(const char * path)
{
	// Network hosts only
	if (::Network.isEnabled() && !::Network.isHost())
		{ Message(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return false; }

	// Open new scenario file
	if (path)
	{
		SCopy(path, Game.ScenarioFilename);
		SetCaption(GetFilename(Game.ScenarioFilename));
		if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
		{
			Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"),Game.ScenarioFilename).getData());
			return false;
		}
	}

	// Can't save to child groups
	if (Game.ScenarioFile.GetMother())
	{
		StdStrBuf str;
		str.Format(LoadResStr("IDS_CNS_NOCHILDSAVE"),
		           GetFilename(Game.ScenarioFile.GetName()));
		Message(str.getData());
		return false;
	}

	// Save game to open scenario file
	SetCursor(C4ConsoleGUI::CURSOR_Wait);

	bool fOkay=true;
	C4GameSave *pGameSave = new C4GameSaveScenario(!Console.Active || ::Landscape.Mode==C4LSC_Exact, false);
	if (!pGameSave->Save(Game.ScenarioFile, false))
		{ Out("Game::Save failed"); fOkay=false; }
	delete pGameSave;

	// Close and reopen scenario file to fix file changes
	if (!Game.ScenarioFile.Close())
		{ Out("ScenarioFile::Close failed"); fOkay=false; }
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
		{ Out("ScenarioFile::Open failed"); fOkay=false; }

	SetCursor(C4ConsoleGUI::CURSOR_Normal);

	// Initialize/script notification
	if (Game.fScriptCreatedObjects)
	{
		StdStrBuf str(LoadResStr("IDS_CNS_SCRIPTCREATEDOBJECTS"));
		str += LoadResStr("IDS_CNS_WARNDOUBLE");
		Message(str.getData());
		Game.fScriptCreatedObjects=false;
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
	return SaveScenario(NULL);
}

bool C4Console::FileSaveAs(bool fSaveGame)
{
	// Do save-as dialog
	StdStrBuf filename;
	filename.Copy(Game.ScenarioFile.GetName());
	if (!FileSelect(&filename,
	                "Clonk 4 Scenario\0*.ocs\0\0",
	                OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
	                true)) return false;
	DefaultExtension(&filename,"ocs");
	// Close current scenario file
	Game.ScenarioFile.Close();
	if (fSaveGame)
		// Save game
		return SaveGame(filename.getData());
	else
		return SaveScenario(filename.getData());
}

bool C4Console::Message(const char *szMessage, bool fQuery)
{
	if (!Active) return false;
	return C4ConsoleGUI::Message(szMessage, fQuery);
}

bool C4Console::FileOpen()
{
	// Get scenario file name
	StdStrBuf c4sfile ("");
	if (!FileSelect(&c4sfile,
	                FILE_SELECT_FILTER_FOR_C4S,
	                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST))
		return false;
	Application.ClearCommandLine();
	// Open game
	Application.OpenGame(c4sfile.getData());
	return true;
}

bool C4Console::FileOpenWPlrs()
{
	// Get scenario file name
	StdStrBuf c4sfile ("");
	if (!FileSelect(&c4sfile,
	                FILE_SELECT_FILTER_FOR_C4S,
	                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST))
		return false;
	// Get player file name(s)
	StdStrBuf c4pfile ("");
	if (!FileSelect(&c4pfile,
	                "Clonk 4 Player\0*.ocp\0\0",
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
}

void C4Console::Default()
{
	EditCursor.Default();
	PropertyDlgObject = 0;
	ToolsDlg.Default();
}

void C4Console::Clear()
{
	C4ConsoleBase::Clear();
	EditCursor.Clear();
	ToolsDlg.Clear();
	PropertyDlgClose();
	ClearViewportMenu();
	ClearPlayerMenu();
	ClearNetMenu();
	if (pSurface) delete pSurface;
	pSurface = 0;
#ifndef _WIN32
	Application.Quit();
#endif
}

void C4Console::Close()
{
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
	ClearInput();
	// add global and standard functions
	std::list <char*> functions = ::ScriptEngine.GetFunctionNames(&::GameScript);
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
	StdStrBuf c4pfile ("");
	if (!FileSelect(&c4pfile,
	                "Clonk 4 Player\0*.ocp\0\0",
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
	AddNetMenuItemForPlayer(IDM_NET_CLIENT1+Game.Clients.getLocalID(), str);
	// Clients
	for (C4Network2Client *pClient=::Network.Clients.GetNextClient(NULL); pClient; pClient=::Network.Clients.GetNextClient(pClient))
	{
		str.Format(LoadResStr(pClient->isActivated() ? "IDS_MNU_NETCLIENT" : "IDS_MNU_NETCLIENTDE"),
		           pClient->getName(), pClient->getID());
		AddNetMenuItemForPlayer(IDM_NET_CLIENT1+pClient->getID(), str);
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
	SetCaption(GetFilename(szFilename));
	C4ConsoleGUI::SetCaptionToFileName(szFilename);
}

void C4Console::Execute()
{
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
}

void C4Console::CloseGame()
{
	if (!Active || !fGameOpen) return;
	fGameOpen=false;
	EnableControls(fGameOpen);
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));
}

bool C4Console::TogglePause()
{
	return Game.TogglePause();
}

#if !(defined(_WIN32) || defined(USE_COCOA) || defined(WITH_DEVELOPER_MODE))
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
void C4ConsoleGUI::AddNetMenuItemForPlayer(int, StdStrBuf&) {}
void C4ConsoleGUI::AddNetMenu() {}
void C4ConsoleGUI::ToolsDlgClose() {}
void C4ConsoleGUI::ClearInput() {}
bool C4ConsoleGUI::ClearLog() {return 0;}
void C4ConsoleGUI::ClearNetMenu() {}
void C4ConsoleGUI::ClearPlayerMenu() {}
void C4ConsoleGUI::ClearViewportMenu() {}
CStdWindow * C4ConsoleGUI::CreateConsoleWindow(CStdApp*) {return 0;}
void C4ConsoleGUI::DisplayInfoText(C4ConsoleGUI::InfoTextType, StdStrBuf&) {}
void C4ConsoleGUI::DoEnableControls(bool) {}
bool C4ConsoleGUI::DoUpdateHaltCtrls(bool) {return 0;}
bool C4ConsoleGUI::FileSelect(StdStrBuf *, char const*, unsigned int, bool) {return 0;}
bool C4ConsoleGUI::Message(char const*, bool) {return 0;}
void C4ConsoleGUI::Out(char const*) {}
bool C4ConsoleGUI::PropertyDlgOpen() {return 0;}
void C4ConsoleGUI::PropertyDlgClose() {}
void C4ConsoleGUI::PropertyDlgUpdate(C4ObjectList &rSelection) {}
void C4ConsoleGUI::RecordingEnabled() {}
void C4ConsoleGUI::SetCaptionToFileName(char const*) {}
void C4ConsoleGUI::SetCursor(C4ConsoleGUI::Cursor) {}
void C4ConsoleGUI::SetInputFunctions(std::list<char*, std::allocator<char*> >&) {}
void C4ConsoleGUI::ShowAboutWithCopyright(StdStrBuf&) {}
void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(C4ToolsDlg*) {}
bool C4ConsoleGUI::ToolsDlgOpen(C4ToolsDlg*) {return 0;}
void C4ConsoleGUI::ToolsDlgSelectTexture(C4ToolsDlg*, char const*) {}
void C4ConsoleGUI::ToolsDlgSetMaterial(C4ToolsDlg*, char const*) {}
void C4ConsoleGUI::ToolsDlgSetTexture(C4ToolsDlg*, char const*) {}
bool C4ConsoleGUI::UpdateModeCtrls(int) {return 0;}
void C4ToolsDlg::EnableControls() {}
void C4ToolsDlg::InitGradeCtrl() {}
void C4ToolsDlg::NeedPreviewUpdate() {}
bool C4ToolsDlg::PopMaterial() {return 0;}
bool C4ToolsDlg::PopTextures() {return 0;}
void C4ToolsDlg::UpdateIFTControls() {}
void C4ToolsDlg::UpdateLandscapeModeCtrls() {}
void C4ToolsDlg::UpdateTextures() {}
void C4ToolsDlg::UpdateToolCtrls() {}
bool C4Viewport::ScrollBarsByViewPosition() {return 0;}
bool C4Viewport::TogglePlayerLock() {return 0;}
void CStdWindow::RequestUpdate() {}
bool OpenURL(char const*) {return 0;}
#include "C4ConsoleGUICommon.h"
#endif
