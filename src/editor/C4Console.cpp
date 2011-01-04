/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2004, 2008  Matthes Bender
 * Copyright (c) 2001-2002, 2004-2007  Sven Eberhardt
 * Copyright (c) 2004, 2007, 2009  Peter Wortmann
 * Copyright (c) 2005-2007, 2009-2010  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Mortimer
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
#include <C4PlayerList.h>
#include <C4GameControl.h>

#include <StdFile.h>
#include <StdRegistry.h>

#define FILE_SELECT_FILTER_FOR_C4S "Clonk 4 Scenario\0"         \
                                   "*.c4s;*.c4f;Scenario.txt\0" \
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
	if (Game.Script.Counter!=ScriptCounter)
	{
		ScriptCounter=Game.Script.Counter;
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

bool C4Console::SaveGame(bool fSaveGame)
{
	// Network hosts only
	if (::Network.isEnabled() && !::Network.isHost())
		{ Message(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return false; }


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
	bool fOkay=true;
	Console.SetCursor(C4ConsoleGUI::CURSOR_Wait);

	C4GameSave *pGameSave;
	if (fSaveGame)
		pGameSave = new C4GameSaveSavegame();
	else
		pGameSave = new C4GameSaveScenario(!Console.Active || ::Landscape.Mode==C4LSC_Exact, false);
	if (!pGameSave->Save(Game.ScenarioFile, false))
		{ Out("Game::Save failed"); fOkay=false; }
	delete pGameSave;

	// Close and reopen scenario file to fix file changes
	if (!Game.ScenarioFile.Close())
		{ Out("ScenarioFile::Close failed"); fOkay=false; }
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
		{ Out("ScenarioFile::Open failed"); fOkay=false; }

	Console.SetCursor(C4ConsoleGUI::CURSOR_Normal);

	// Initialize/script notification
	if (Game.fScriptCreatedObjects)
		if (!fSaveGame)
		{
			StdStrBuf str(LoadResStr("IDS_CNS_SCRIPTCREATEDOBJECTS"));
			str += LoadResStr("IDS_CNS_WARNDOUBLE");
			Message(str.getData());
			Game.fScriptCreatedObjects=false;
		}

	// Status report
	if (!fOkay) Message(LoadResStr("IDS_CNS_SAVERROR"));
	else Out(LoadResStr(fSaveGame ? "IDS_CNS_GAMESAVED" : "IDS_CNS_SCENARIOSAVED"));

	return fOkay;
}

bool C4Console::FileSave(bool fSaveGame)
{
	// Don't quicksave games over scenarios
	if (fSaveGame)
		if (!Game.C4S.Head.SaveGame)
		{
			Message(LoadResStr("IDS_CNS_NOGAMEOVERSCEN"));
			return false;
		}
	// Save game
	return SaveGame(fSaveGame);
}

bool C4Console::FileSaveAs(bool fSaveGame)
{
	// Do save-as dialog
	char filename[512+1];
	SCopy(Game.ScenarioFile.GetName(),filename);
	if (!FileSelect(filename,512,
	                "Clonk 4 Scenario\0*.c4s\0\0",
	                OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
	                true)) return false;
	DefaultExtension(filename,"c4s");
	bool fOkay=true;
	// Close current scenario file
	if (!Game.ScenarioFile.Close()) fOkay=false;
	// Copy current scenario file to target
	if (!C4Group_CopyItem(Game.ScenarioFilename,filename)) fOkay=false;
	// Open new scenario file
	SCopy(filename,Game.ScenarioFilename);

	SetCaption(GetFilename(Game.ScenarioFilename));
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename)) fOkay=false;
	// Failure message
	if (!fOkay)
	{
		Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"),Game.ScenarioFilename).getData()); return false;
	}
	// Save game
	return SaveGame(fSaveGame);
}

bool C4Console::Message(const char *szMessage, bool fQuery)
{
	if (!Active) return false;
	return C4ConsoleGUI::Message(szMessage, fQuery);
}

bool C4Console::FileOpen()
{
	// Get scenario file name
	char c4sfile[512+1]="";
	if (!FileSelect(c4sfile,512,
	                FILE_SELECT_FILTER_FOR_C4S,
	                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST))
		return false;
	Application.ClearCommandLine();
	Game.SetScenarioFilename(c4sfile);
	// Open game
	OpenGame();
	return true;
}

bool C4Console::FileOpenWPlrs()
{
	// Get scenario file name
	char c4sfile[512+1]="";
	if (!FileSelect(c4sfile,512,
	                FILE_SELECT_FILTER_FOR_C4S,
	                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST))
		return false;
	// Get player file name(s)
	char c4pfile[4096+1]="";
	if (!FileSelect(c4pfile,4096,
	                "Clonk 4 Player\0*.c4p\0\0",
	                OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	               )) return false;
	// Compose command line
	Application.ClearCommandLine();
	Game.SetScenarioFilename(c4sfile);
	if (DirectoryExists(c4pfile)) // Multiplayer
	{
		const char *cptr = c4pfile + SLen(c4pfile) + 1;
		while (*cptr)
		{
			char c4pfile2[512 + 1] = "";
			SAppend(c4pfile, c4pfile2, 512);
			SAppend(DirSep, c4pfile2, 512);
			SAppend(cptr, c4pfile2, 512);
			SAddModule(Game.PlayerFilenames, c4pfile2);
			cptr += SLen(cptr) + 1;
		}
	}
	else // Single player
	{
		SAddModule(Game.PlayerFilenames, c4pfile);
	}
	// Open game
	OpenGame();
	return true;
}

bool C4Console::FileClose()
{
	return CloseGame();
}

bool C4Console::FileSelect(char *sFilename, int iSize, const char * szFilter, DWORD dwFlags, bool fSave)
{
	return C4ConsoleGUI::FileSelect(sFilename, iSize, szFilter, dwFlags, fSave);
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
	PropertyDlg.ClearPointers(pObj);
}

void C4Console::Default()
{
	EditCursor.Default();
	PropertyDlg.Default();
	ToolsDlg.Default();
}

void C4Console::Clear()
{
	C4ConsoleBase::Clear();
	EditCursor.Clear();
	PropertyDlg.Clear();
	ToolsDlg.Clear();
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

void C4Console::EditTitle()
{
	if (::Network.isEnabled()) return;
	Game.Title.Open();
}

void C4Console::EditScript()
{
	if (::Network.isEnabled()) return;
	Game.Script.Open();
	::ScriptEngine.ReLink(&::Definitions);
}

void C4Console::EditInfo()
{
	if (::Network.isEnabled()) return;
	Game.Info.Open();
}

void C4Console::EditObjects()
{
	ObjectListDlg.Open();
}

void C4Console::UpdateInputCtrl()
{
	int cnt;
	C4AulScriptFunc *pRef;

	ClearInput();
	// add global and standard functions
	std::vector<char*> functions;
	for (C4AulFunc *pFn = ::ScriptEngine.GetFirstFunc(); pFn; pFn = ::ScriptEngine.GetNextFunc(pFn))
	{
		if (pFn->GetPublic())
		{
			functions.push_back(pFn->Name);
		}
	}
	// Add scenario script functions
	if (pRef=Game.Script.GetSFunc(0))
		functions.push_back((char*)C4ConsoleGUI::LIST_DIVIDER);
	for (cnt=0; (pRef=Game.Script.GetSFunc(cnt)); cnt++)
	{
		functions.push_back(pRef->Name);
	}
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
	char c4pfile[4096+1]="";
	if (!FileSelect(c4pfile,4096,
	                "Clonk 4 Player\0*.c4p\0\0",
	                OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	               )) return;

	// Compose player file list
	char c4plist[6000]="";
	// Multiple players
	if (DirectoryExists(c4pfile))
	{
		const char *cptr = c4pfile+SLen(c4pfile)+1;
		while (*cptr)
		{
			SNewSegment(c4plist);
			SAppend(c4pfile,c4plist); SAppend(DirSep ,c4plist); SAppend(cptr,c4plist);
			cptr += SLen(cptr)+1;
		}
	}
	// Single player
	else
	{
		SAppend(c4pfile,c4plist);
	}

	// Join players (via network/ctrl queue)
	char szPlayerFilename[_MAX_PATH+1];
	for (int iPar=0; SCopySegment(c4plist,iPar,szPlayerFilename,';',_MAX_PATH); iPar++)
		if (szPlayerFilename[0])
		{
			if (::Network.isEnabled())
				::Network.Players.JoinLocalPlayer(szPlayerFilename, true);
			else
				::Players.CtrlJoinLocalNoNetwork(szPlayerFilename, Game.Clients.getLocalID(), Game.Clients.getLocalName());
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
	PropertyDlg.Execute();
	ObjectListDlg.Execute();
	UpdateStatusBars();
	::GraphicsSystem.Execute();
}

bool C4Console::OpenGame()
{
	bool fGameWasOpen = fGameOpen;
	// Close any old game
	CloseGame();

	// Default game dependent members
	Default();
	SetCaption(GetFilename(Game.ScenarioFile.GetName()));
	// Init game dependent members
	if (!EditCursor.Init()) return false;
	// Default game - only if open before, because we do not want to default out the GUI
	if (fGameWasOpen) Game.Default();

	// PreInit is required because GUI has been deleted
	if (!Game.PreInit() ) { Game.Clear(); return false; }

	// Init game
	if (!Game.Init())
	{
		Game.Clear();
		return false;
	}

	// Console updates
	fGameOpen=true;
	UpdateInputCtrl();
	EnableControls(fGameOpen);
	UpdatePlayerMenu();
	UpdateViewportMenu();
	SetCaptionToFilename(Game.ScenarioFilename);

	return true;
}

bool C4Console::CloseGame()
{
	if (!fGameOpen) return false;
	Game.Clear();
	Game.GameOver=false; // No leftover values when exiting on closed game
	Game.GameOverDlgShown=false;
	fGameOpen=false;
	EnableControls(fGameOpen);
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));
	return true;
}

bool C4Console::TogglePause()
{
	return Game.TogglePause();
}
