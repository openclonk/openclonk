/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// Engine internal C4Menus: Main menu, Options, Player join, Hostility, etc.

#include "C4Include.h"
#include "gui/C4MainMenu.h"

#include "control/C4GameControl.h"
#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4Viewport.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameOverDlg.h"
#include "network/C4Network2.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

// -----------------------------------------------------------
// C4MainMenu

C4MainMenu::C4MainMenu() : C4Menu() // will be re-adjusted later
{
	Clear();
}

void C4MainMenu::Default()
{
	C4Menu::Default();
	Player=NO_OWNER;
}

bool C4MainMenu::Init(C4FacetSurface &fctSymbol, const char *szEmpty, int32_t iPlayer, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	if (!DoInit(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	Player=iPlayer;
	return true;
}

bool C4MainMenu::InitRefSym(const C4Facet &fctSymbol, const char *szEmpty, int32_t iPlayer, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	if (!DoInitRefSym(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	Player=iPlayer;
	return true;
}

bool C4MainMenu::ActivateNewPlayer(int32_t iPlayer)
{
	// league or replay game
	if (Game.Parameters.isLeague() || Game.C4S.Head.Replay) return false;
	// Max player limit
	if (::Players.GetCount() >= Game.Parameters.MaxPlayers) return false;

	// Menu symbol/init
	if (GfxR->fctPlayerClr.Surface)
		GfxR->fctPlayerClr.Surface->SetClr(0xff);
	InitRefSym(GfxR->fctPlayerClr, LoadResStr("IDS_MENU_NOPLRFILES"), iPlayer);
	for (DirectoryIterator iter(Config.General.UserDataPath); *iter; ++iter)
		if (WildcardMatch("*.ocp", *iter))
		{
			char szFilename[_MAX_PATH_LEN], szCommand[_MAX_PATH_LEN+30];
			SCopy(*iter, szFilename, _MAX_PATH);
			if (DirectoryExists(szFilename)) continue;
			if (::Players.FileInUse(szFilename)) continue;
			// Open group
			C4Group hGroup;
			if (!hGroup.Open(szFilename)) continue;
			// Load player info
			C4PlayerInfoCore C4P;
			if (!C4P.Load(hGroup)) { hGroup.Close(); continue; }
			// Close group
			hGroup.Close();
			// Add player item
			sprintf(szCommand, "JoinPlayer:%s", szFilename);
			StdStrBuf sItemText;
			sItemText.Format(LoadResStr("IDS_MENU_NEWPLAYER"), C4P.PrefName);
			C4FacetSurface fctSymbol;
			// Add menu item
			Add(sItemText.getData(), fctSymbol, szCommand);
			// Reset symbol facet (menu holds on to the surface)
			fctSymbol.Default();
		}

	// Alignment
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");

	return true;
}

bool C4MainMenu::DoRefillInternal(bool &rfRefilled)
{
	// Variables
	C4FacetSurface fctSymbol;
	C4Player *pPlayer;
	C4IDList ListItems;
	C4Facet fctTarget;
	bool fWasEmpty = !GetItemCount();

	// Refill
	switch (Identification)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Hostility:
	{
		// Clear items
		ClearItems();
		// Refill player
		if (!(pPlayer = ::Players.Get(Player))) return false;
		// Refill items
		C4Player *pPlr; int32_t iIndex;
		for (iIndex=0; (pPlr = ::Players.GetByIndex(iIndex)); iIndex++)
			// Ignore player self and invisible
			if (pPlr != pPlayer) if (!pPlr->IsInvisible())
				{
					// Symbol
					fctSymbol.Create(C4SymbolSize,C4SymbolSize);
					pPlayer->DrawHostility(fctSymbol,iIndex);
					// Message
					StdStrBuf sMsg;
					bool isFriendly = pPlayer->Hostility.find(pPlr) == pPlayer->Hostility.end();
					if (isFriendly)
						sMsg.Format(LoadResStr("IDS_MENU_ATTACK"),pPlr->GetName());
					else
						sMsg.Format(LoadResStr("IDS_MENU_NOATTACK"),pPlr->GetName());
					// Command
					char szCommand[1000];
					sprintf(szCommand,"SetHostility:%i",pPlr->Number);
					// Info caption
					char szInfoCaption[C4MaxTitle+1],szFriendly[50],szNot[30]="";
					SCopy(LoadResStr(isFriendly ? "IDS_MENU_ATTACKHOSTILE" : "IDS_MENU_ATTACKFRIENDLY"),szFriendly);
					if (!isFriendly) SCopy(LoadResStr("IDS_MENU_ATTACKNOT"),szNot);
					sprintf(szInfoCaption,LoadResStr("IDS_MENU_ATTACKINFO"),pPlr->GetName(),szFriendly,szNot);
					if (iIndex==pPlayer->Number) SCopy(LoadResStr("IDS_MENU_ATTACKSELF"),szInfoCaption);
					// Add item
					Add(sMsg.getData(),fctSymbol,szCommand,C4MN_Item_NoCount,nullptr,szInfoCaption);
					fctSymbol.Default();
				}
		break;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_TeamSelection:
	case C4MN_TeamSwitch:
	{
		// Clear items
		ClearItems();
		// add all teams as menu items
		// 2do: Icon
		C4Team *pTeam; int32_t i=0; bool fAddNewTeam=Game.Teams.IsAutoGenerateTeams();
		for (;;)
		{
			pTeam = Game.Teams.GetTeamByIndex(i);
			if (pTeam)
			{
				// next regular team
				++i;
				// do not add a new team if an empty team exists
				if (!pTeam->GetPlayerCount()) fAddNewTeam = false;
			}
			else if (fAddNewTeam)
			{
				// join new team
				fAddNewTeam = false;
			}
			else
			{
				// all teams done
				break;
			}
			// create team symbol: Icon spec if specified; otherwise flag for empty and crew for nonempty team
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			const char *szIconSpec = pTeam ? pTeam->GetIconSpec() : nullptr;
			bool fHasIcon = false;
			if (szIconSpec && *szIconSpec)
			{
				fHasIcon = Game.DrawTextSpecImage(fctSymbol, szIconSpec, nullptr, pTeam->GetColor());
			}
			if (!fHasIcon)
			{
				if (pTeam && pTeam->GetPlayerCount())
					::GraphicsResource.fctCrewClr.DrawClr(fctSymbol, true, pTeam->GetColor());
				else
					C4GUI::Icon::GetIconFacet(C4GUI::Ico_Team).Draw(fctSymbol, true);
			}
			StdStrBuf sTeamName;
			if (pTeam)
			{
				sTeamName.Take(pTeam->GetNameWithParticipants());
			}
			else
				sTeamName.Ref(LoadResStr("IDS_PRC_NEWTEAM"));
			const char *szOperation = (Identification == C4MN_TeamSwitch) ? "TeamSwitch" : "TeamSel";
			Add(sTeamName.getData(), fctSymbol,FormatString("%s:%d", szOperation, pTeam ? pTeam->GetID() : TEAMID_New).getData(),
			    C4MN_Item_NoCount,nullptr,FormatString(LoadResStr("IDS_MSG_JOINTEAM"), sTeamName.getData()).getData(), C4ID(pTeam ? pTeam->GetID() : 0));
			fctSymbol.Default();
		}
		break;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Observer: // observer menu
	{
		// Clear items
		ClearItems();
		// Check validity
		C4Viewport *pVP = ::Viewports.GetViewport(NO_OWNER);
		if (!pVP) return false;
		int32_t iInitialSelection = 0;
		// Add free view
		AddRefSym(LoadResStr("IDS_MSG_FREEVIEW"), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Star), "Observe:Free", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MSG_FREELYSCROLLAROUNDTHEMAP"));
		// Add players
		C4Player *pPlr; int32_t iIndex;
		for (iIndex=0; (pPlr = ::Players.GetByIndex(iIndex)); iIndex++)
		{
			// Ignore invisible
			if (!pPlr->IsInvisible())
			{
				// Symbol
				fctSymbol.Create(C4SymbolSize,C4SymbolSize);
				::GraphicsResource.fctPlayerClr.DrawClr(fctSymbol, true, pPlr->ColorDw);
				// Message
				StdStrBuf sMsg;
				DWORD dwClr = pPlr->ColorDw;
				sMsg.Format("<c %x>%s</c>", (unsigned int)C4GUI::MakeColorReadableOnBlack(dwClr), pPlr->GetName());
				// Command
				StdStrBuf sCommand;
				sCommand.Format("Observe:%d", (int)pPlr->Number);
				// Info caption
				StdStrBuf sInfo;
				sInfo.Format(LoadResStr("IDS_TEXT_FOLLOWVIEWOFPLAYER"), pPlr->GetName());
				// Add item
				Add(sMsg.getData(),fctSymbol,sCommand.getData(),C4MN_Item_NoCount,nullptr,sInfo.getData());
				fctSymbol.Default();
				// check if this is the currently selected player
				if (pVP->GetPlayer() == pPlr->Number) iInitialSelection = GetItemCount()-1;
			}
			// Initial selection on followed player
			if (fWasEmpty) SetSelection(iInitialSelection, false, true);
		}
	}
	break;
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default:
		// No internal refill needed
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	// Successfull internal refill
	rfRefilled = true;
	return true;
}

void C4MainMenu::OnSelectionChanged(int32_t iNewSelection)
{
	// immediate player highlight in observer menu
	if (Identification == C4MN_Observer)
	{
		C4MenuItem *pItem = GetSelectedItem();
		if (pItem)
		{
			if (SEqual2(pItem->GetCommand(), "Observe:"))
				MenuCommand(pItem->GetCommand(), false);
		}
	}
}

void C4MainMenu::OnUserSelectItem(int32_t Player, int32_t iIndex)
{
	// direct selection for non-sync-menus
	SetSelection(iIndex, true, true);
}

void C4MainMenu::OnUserEnter(int32_t Player, int32_t iIndex, bool fRight)
{
	// direct menu control
	// but ensure selection is Okay before
	SetSelection(iIndex, true, false);
	Enter(fRight);
}

void C4MainMenu::OnUserClose()
{
	// just close
	TryClose(false, true);
}

bool C4MainMenu::ActivateGoals(int32_t iPlayer, bool fDoActivate)
{
	C4FacetSurface fctSymbol;
	C4Facet fctGF; // goal fulfilled facet

	if (fDoActivate)
	{
		// Menu symbol/init
		InitRefSym(GfxR->fctMenu.GetPhase(4),LoadResStr("IDS_MENU_CPGOALS"),iPlayer);
		SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
		SetPermanent(false);
		fctGF.Set(nullptr, C4SymbolSize-::GraphicsResource.fctCaptain.Wdt-2, 2, ::GraphicsResource.fctCaptain.Wdt, ::GraphicsResource.fctCaptain.Hgt);
	}
	// determine if the goals are fulfilled - do the calls even if the menu is not to be opened to ensure synchronization
	C4IDList GoalList, FulfilledGoalList;
	C4RoundResults::EvaluateGoals(GoalList, FulfilledGoalList, iPlayer);
	// Add Items
	if (fDoActivate)
	{
		int32_t iNumGoals = GoalList.GetNumberOfIDs(), cnt;
		C4ID idGoal; C4Def *pDef;
		for (int32_t i=0; i<iNumGoals; ++i)
			if ((idGoal = GoalList.GetID(i, &cnt)))
				if ((pDef = C4Id2Def(idGoal)))
				{
					fctSymbol.Create(C4SymbolSize,C4SymbolSize);
					// 2do: If an object instance is known, draw the object instead?
					// this would allow us to do dynamic pictures and overlays; e.g. draw the actual, required settlement score
					// for settlement score goals
					// Same for pDef->GetName(), pDef->GetDesc()
					pDef->Draw(fctSymbol);
					if (FulfilledGoalList.GetIDCount(idGoal))
					{
						fctGF.Surface=fctSymbol.Surface;
						::GraphicsResource.fctCaptain.Draw(fctGF);
					}
					StdStrBuf Command; Command.Format("Player:Goal:%s", idGoal.ToString());
					Add(pDef->GetName(),fctSymbol,Command.getData(),C4MN_Item_NoCount,nullptr,"");
				}
		// Go back to options menu on close
		SetCloseCommand("ActivateMenu:Main");
	}
	// Done
	return true;
}

bool C4MainMenu::ActivateRules(int32_t iPlayer)
{
	// Menu symbol/init
	char Command[256];
	C4FacetSurface fctSymbol;
	InitRefSym(GfxR->fctMenu.GetPhase(5),LoadResStr("IDS_MENU_CPRULES"),iPlayer);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(false);
	// Items
	int32_t cnt; C4ID idGoal; C4Def *pDef;
	for (cnt=0; (idGoal=::Objects.GetListID(C4D_Rule,cnt)); cnt++)
		if ((pDef=C4Id2Def(idGoal)))
		{
			fctSymbol.Create(C4SymbolSize,C4SymbolSize); pDef->Draw(fctSymbol);
			sprintf(Command, "Player:Rule:%s", idGoal.ToString());
			Add(pDef->GetName(),fctSymbol,Command,C4MN_Item_NoCount,nullptr,"");
		}
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");
	// Done
	return true;
}

bool LooksLikeInteger(const char *szInt)
{
	// safety
	if (!szInt) return false;
	// check sign
	if (*szInt == '+' || *szInt == '-') ++szInt;
	// check int32_t length
	if (!*szInt) return false;
	// must contain only digits now
	char c;
	while ((c = *(szInt++))) if (!Inside<char>(c, '0', '9')) return false;
	// it's an int32_t
	return true;
}

bool C4MainMenu::ActivateSavegame(int32_t iPlayer)
{
	// Check if saving is possible
	if (!Game.CanQuickSave()) return false;

	// Menu symbol/init
	char DirPath[_MAX_PATH_LEN];
	char ScenName[_MAX_PATH_LEN]; *ScenName=0;

	InitRefSym(GfxR->fctMenu.GetPhase(0), LoadResStr("IDS_MENU_CPSAVEGAME"), iPlayer);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);

	// target file name mask
	// get folder & filename to store in
	// some magic is needed to ensure savegames are stored properly into their folders
	SCopy(GetFilename(Game.ScenarioFilename), DirPath);
	if (DirPath[strlen(DirPath) - 1] == '\\') DirPath[strlen(DirPath) - 1] = 0;
	RemoveExtension(DirPath);
	if (LooksLikeInteger(DirPath))
	{
		// ScenTitle.ocf\%d.ocs-names (old-style savegames)
		// get owning folder
		if (Game.pParentGroup)
		{
			// owning folder determines filename
			SCopy(GetFilenameOnly(Game.pParentGroup->GetName()), ScenName);
		}
		else
		{
			// no owning folder known: too bad
			// make a vague guess based on the scenario title
			SCopy(GetFilenameOnly(Game.ScenarioFilename), ScenName);
		}
	}
	else
	{
		// DirPath is a valid filename for now...
		SCopy(DirPath, ScenName);
		// but remove trailing numbers to adjust new-style savegames
		char *pScenNameEnd = ScenName+SLen(ScenName);
		while (Inside<char>(*--pScenNameEnd, '0', '9'))
			if (pScenNameEnd == ScenName)
			{
				// should not happen: digit-only-filenames should have been caught earlier
				SCopy("dbg_error!", ScenName);
				pScenNameEnd = ScenName+SLen(ScenName)-1;
			}
		pScenNameEnd[1]=0;
	}

	// New Style 2007:
	// * scenarios are saved into ScenName.ocf/ScenName123.ocs to keep umlauts out of filenames
	// * language titles are stored in folders as title component
	StdStrBuf strFilename, strTitle;
	strFilename.Format("%s.ocf%c%s%%d.ocs", ScenName, DirectorySeparator, ScenName);
	strTitle = Game.ScenarioTitle;

	// Create menu items
	StdStrBuf strFilenameIndexed, strCommand, strCaption, strSavePath;
	for (int32_t i = 1; i <= 10; i++)
	{
		// Index filename
		strFilenameIndexed.Format(strFilename.getData(), i);
		// Compose commmand
		strCommand.Format("Save:Game:%s:%s", strFilenameIndexed.getData(), strTitle.getData()); // Notice: the language title might contain ':' and thus confuse the segment list - but C4Menu::MenuCommand will try to handle this...
		// Check free slot
		strSavePath.Format("%s%c%s", Config.AtUserDataPath(C4CFN_Savegames), DirectorySeparator, strFilenameIndexed.getData());
		bool fFree = !C4Group_IsGroup(strSavePath.getData());
		// Item caption
		strCaption = LoadResStr("IDS_MENU_CPSAVEGAME");
		// add menu item
		AddRefSym(strCaption.getData(), GfxR->fctMenu.GetPhase(i - 1, fFree ? 2 : 1), strCommand.getData(), C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MENU_CPSAVEGAMEINFO"));
	}

	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");

	return true;
}

bool C4MainMenu::ActivateHost(int32_t iPlayer)
{
	// Menu symbol/init
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), LoadResStr("IDS_MENU_DISCONNECTCLIENT"), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	// Clients
	for (C4Network2Client *pClient=::Network.Clients.GetNextClient(nullptr); pClient; pClient=::Network.Clients.GetNextClient(pClient))
	{
		bool fHost = (pClient->getID() == 0);
		StdStrBuf strText, strCommand;
		strText.Format("%s (%s)", pClient->getName(), pClient->getCore().getNick());
		strCommand.Format("Host:Kick:%d", pClient->getID());
		C4GUI::Icons iIcon = fHost ? C4GUI::Ico_Host : (pClient->isActivated() ? C4GUI::Ico_Client : C4GUI::Ico_ObserverClient);
		AddRefSym(strText.getData(), C4GUI::Icon::GetIconFacet(iIcon), strCommand.getData());
	}
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::ActivateClient(int32_t iPlayer)
{
	// Menu symbol/init
	C4FacetSurface fctSymbol;
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), LoadResStr("IDS_MENU_DISCONNECTFROMSERVER"), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,3,0);
	Add(LoadResStr("IDS_BTN_YES"), fctSymbol, "Part");
	fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,1,0);
	Add(LoadResStr("IDS_BTN_NO"), fctSymbol, "");
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::ActivateSurrender(int32_t iPlayer)
{
	C4FacetSurface fctSymbol;
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Surrender), LoadResStr("IDS_MENU_SURRENDER"), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,3,0);
	Add(LoadResStr("IDS_BTN_YES"), fctSymbol, "Surrender");
	fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,1,0);
	Add(LoadResStr("IDS_BTN_NO"), fctSymbol, "");
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::ActivateOptions(int32_t iPlayer, int32_t selection)
{
	// Menu symbol/init
	InitRefSym(GfxR->fctOptions.GetPhase(0), LoadResStr("IDS_MNU_OPTIONS"), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	// Sound
	AddRefSym(LoadResStr("IDS_DLG_SOUND"), GfxR->fctOptions.GetPhase(17 + Config.Sound.RXSound),"Options:Sound",C4MN_Item_NoCount);
	// Music
	AddRefSym(LoadResStr("IDS_MNU_MUSIC"), GfxR->fctOptions.GetPhase(1 + Config.Sound.RXMusic),"Options:Music",C4MN_Item_NoCount);
	// Music
	AddRefSym(LoadResStr("IDS_MENU_DISPLAY"), GfxR->fctMenu.GetPhase(8), "ActivateMenu:Display");
	// Restore selection
	SetSelection(selection, false, true);
	// Go back to main menu on close
	SetCloseCommand("ActivateMenu:Main");
	// Done
	return true;
}

bool C4MainMenu::ActivateDisplay(int32_t iPlayer, int32_t selection)
{
	// Menu symbol/init
	InitRefSym(GfxR->fctMenu.GetPhase(8), LoadResStr("IDS_MENU_DISPLAY"), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	// Crew player names
	AddRefSym(LoadResStr("IDS_MNU_PLAYERNAMES"), GfxR->fctOptions.GetPhase(7 + Config.Graphics.ShowCrewNames), "Display:PlayerNames", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MENU_PLAYERNAMES_DESC"));
	// Crew clonk names
	AddRefSym(LoadResStr("IDS_MNU_CLONKNAMES"), GfxR->fctOptions.GetPhase(9 + Config.Graphics.ShowCrewCNames), "Display:ClonkNames", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MENU_CLONKNAMES_DESC"));
	// Upper Board
	if (!Application.isEditor)
	{
		AddRefSym(LoadResStr("IDS_MNU_UPPERBOARD"),GfxR->fctOptions.GetPhase(3 + Config.Graphics.UpperBoard),"Display:UpperBoard",C4MN_Item_NoCount);
	}
	// FPS
	if (!Application.isEditor)
		AddRefSym(LoadResStr("IDS_MNU_FPS"), GfxR->fctOptions.GetPhase(5 + Config.General.FPS), "Display:FPS", C4MN_Item_NoCount);
	// Clock
	if (!Application.isEditor)
		AddRefSym(LoadResStr("IDS_MNU_CLOCK"), GfxR->fctOptions.GetPhase(15 + Config.Graphics.ShowClock), "Display:Clock", C4MN_Item_NoCount);
	// Restore selection
	SetSelection(selection, false, true);
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Options");
	// Done
	return true;
}

bool C4MainMenu::ActivateMain(int32_t iPlayer)
{
	// Determine player
	C4Player *pPlr = ::Players.Get(iPlayer);
	// Menu symbol/init
	C4FacetSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize, C4SymbolSize);
	GfxR->fctOKCancel.Draw(fctSymbol, true, 1, 1);
	Init(fctSymbol, LoadResStr(pPlr ? "IDS_MENU_CPMAIN" : "IDS_MENU_OBSERVER"), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	// Goals+Rules (player menu only)
	// Goal menu can't be shown because of script callbacks
	// Rule menu could be shown, but rule activation would issue script callbacks and trigger client activation
	// Showing rules but not showing goals would be strange anyway
	if (pPlr)
	{
		// Goals
		AddRefSym(LoadResStr("IDS_MENU_CPGOALS"),GfxR->fctMenu.GetPhase(4),"ActivateMenu:Goals",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_MENU_CPGOALSINFO"));
		// Rules
		AddRefSym(LoadResStr("IDS_MENU_CPRULES"),GfxR->fctMenu.GetPhase(5),"ActivateMenu:Rules",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_MENU_CPRULESINFO"));
	}
	// Observer menu in free viewport
	if (!pPlr)
	{
		AddRefSym(LoadResStr("IDS_TEXT_VIEW"),C4GUI::Icon::GetIconFacet(C4GUI::Ico_View),"ActivateMenu:Observer",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_TEXT_DETERMINEPLAYERVIEWTOFOLL"));
	}
	// Hostility (player menu only)
	if (pPlr && (::Players.GetCount() > 1))
	{
		GfxR->fctFlagClr.Surface->SetClr(0xff0000);
		AddRefSym(LoadResStr("IDS_MENU_CPATTACK"),GfxR->fctMenu.GetPhase(7),"ActivateMenu:Hostility",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_MENU_CPATTACKINFO"));
	}
	// Team change
	if (pPlr && Game.Teams.IsTeamSwitchAllowed())
	{
		C4Facet fctTeams; fctTeams = C4GUI::Icon::GetIconFacet(C4GUI::Ico_Team);
		AddRefSym(LoadResStr("IDS_MSG_SELTEAM"),fctTeams,"ActivateMenu:TeamSel",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_MSG_ALLOWSYOUTOJOINADIFFERENT"));
	}
	// Player join
	if ((::Players.GetCount() < Game.Parameters.MaxPlayers) && !Game.Parameters.isLeague())
	{
		AddRefSym(LoadResStr("IDS_MENU_CPNEWPLAYER"),GfxR->fctPlayerClr.GetPhase(),"ActivateMenu:NewPlayer",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_MENU_CPNEWPLAYERINFO"));
	}
	// Save game (player menu only - should we allow saving games with no players in it?)
	if (pPlr && (!::Network.isEnabled() || ::Network.isHost()))
	{
		AddRefSym(LoadResStr("IDS_MENU_CPSAVEGAME"),GfxR->fctMenu.GetPhase(0),"ActivateMenu:Save:Game",C4MN_Item_NoCount,nullptr,LoadResStr("IDS_MENU_CPSAVEGAMEINFO"));
	}
	// Options
	AddRefSym(LoadResStr("IDS_MNU_OPTIONS"), GfxR->fctOptions.GetPhase(0), "ActivateMenu:Options",C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MNU_OPTIONSINFO"));
	// Disconnect
	if (::Network.isEnabled())
	{
		// Host
		if (::Network.isHost() && Game.Clients.getClient(nullptr))
			AddRefSym(LoadResStr("IDS_MENU_DISCONNECT"), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), "ActivateMenu:Host", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_TEXT_KICKCERTAINCLIENTSFROMTHE"));
		// Client
		if (!::Network.isHost())
			AddRefSym(LoadResStr("IDS_MENU_DISCONNECT"), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), "ActivateMenu:Client", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_TEXT_DISCONNECTTHEGAMEFROMTHES"));
	}
	// Surrender (player menu only)
	if (pPlr)
		AddRefSym(LoadResStr("IDS_MENU_CPSURRENDER"), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Surrender), "ActivateMenu:Surrender", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MENU_CPSURRENDERINFO"));
	// Abort
	if (!Application.isEditor)
		AddRefSym(LoadResStr("IDS_MENU_ABORT"), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Exit), "Abort", C4MN_Item_NoCount, nullptr, LoadResStr("IDS_MENU_ABORT_DESC"));
	// No empty menus
	if (GetItemCount()==0) Close(false);
	// Done
	return true;
}

bool C4MainMenu::ActivateHostility(int32_t iPlayer)
{
	// Init menu
	C4FacetSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize, C4SymbolSize);
	GfxR->fctMenu.GetPhase(7).Draw(fctSymbol);
	Init(fctSymbol, LoadResStr("IDS_MENU_CPATTACK"), iPlayer, C4MN_Extra_None, 0, C4MN_Hostility);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	Refill();
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::MenuCommand(const char *szCommand, bool fIsCloseCommand)
{
	// Determine player
	C4Player *pPlr = ::Players.Get(Player);
	// Activate
	if (SEqual2(szCommand,"ActivateMenu:"))
	{
		if (C4GameOverDlg::IsShown()) return false; // no new menus during game over dlg
		if (SEqual(szCommand+13,"Main")) return ActivateMain(Player);
		if (SEqual(szCommand+13,"Hostility")) return ActivateHostility(Player);
		if (SEqual(szCommand+13,"NewPlayer")) return ActivateNewPlayer(Player);
		if (SEqual(szCommand+13,"Goals"))
		{
			::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::ActivateGoalMenu(::Players.Get(Player)), CDT_Queue);
			return true;
		}
		if (SEqual(szCommand+13,"Rules")) return ActivateRules(Player);
		if (SEqual(szCommand+13,"Host")) return ActivateHost(Player);
		if (SEqual(szCommand+13,"Client")) return ActivateClient(Player);
		if (SEqual(szCommand+13,"Options")) return ActivateOptions(Player);
		if (SEqual(szCommand+13,"Display")) return ActivateDisplay(Player);
		if (SEqual(szCommand+13,"Save:Game")) return ActivateSavegame(Player);
		if (SEqual(szCommand+13,"TeamSel")) return pPlr ? pPlr->ActivateMenuTeamSelection(true) : false;
		if (SEqual(szCommand+13,"Surrender")) return ActivateSurrender(Player);
		if (SEqual(szCommand+13,"Observer")) return ActivateObserver();
	}
	// JoinPlayer
	if (SEqual2(szCommand,"JoinPlayer:"))
	{
		// not in league or replay mode
		if (Game.Parameters.isLeague() || Game.C4S.Head.Replay) return false;
		// join player
		// 2do: not for observers and such?
		Players.JoinNew(szCommand+11);
		return true;
	}
	// SetHostility
	if (SEqual2(szCommand,"SetHostility:"))
	{
		// only if allowed
		if (!Game.Teams.IsHostilityChangeAllowed()) return false;
		int32_t iOpponent; sscanf(szCommand+13,"%i",&iOpponent);
		C4Player *pOpponent = ::Players.Get(iOpponent);
		if (!pOpponent || pOpponent->GetType() != C4PT_User) return false;
		::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::SetHostility(::Players.Get(Player), pOpponent, !::Players.HostilityDeclared(Player, pOpponent->Number)), CDT_Queue);
		return true;
	}
	// Abort
	if (SEqual2(szCommand,"Abort"))
	{
		FullScreen.ShowAbortDlg();
		return true;
	}
	// Surrender
	if (SEqual2(szCommand,"Surrender"))
	{
		::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::Surrender(::Players.Get(Player)), CDT_Queue);
		return true;
	}
	// Save game
	if (SEqual2(szCommand, "Save:Game:"))
	{
		char strFilename[_MAX_PATH_LEN]; SCopySegment(szCommand, 2, strFilename, ':', _MAX_PATH);
		char strTitle[_MAX_PATH_LEN]; SCopy(szCommand + SCharPos(':', szCommand, 2) + 1, strTitle, _MAX_PATH);
		Game.QuickSave(strFilename, strTitle);
		ActivateSavegame(Player);
		return true;
	}
	// Kick
	if (SEqual2(szCommand,"Host:Kick:"))
	{
		int iClientID = atoi(szCommand+10);
		if (iClientID && ::Network.isEnabled())
		{
			if (Game.Parameters.isLeague() && ::Players.GetAtClient(iClientID))
				::Network.Vote(VT_Kick, true, iClientID);
			else
			{
				C4Client *pClient = Game.Clients.getClientByID(iClientID);
				if (pClient) Game.Clients.CtrlRemove(pClient, LoadResStr("IDS_MSG_KICKBYMENU"));
				Close(true);
			}
		}
		return true;
	}
	// Part
	if (SEqual2(szCommand,"Part"))
	{
		if (::Network.isEnabled())
		{
			if (Game.Parameters.isLeague() && ::Players.GetLocalByIndex(0))
				::Network.Vote(VT_Kick, true, ::Control.ClientID());
			else
			{
				Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, LoadResStr("IDS_ERR_GAMELEFTVIAPLAYERMENU"));
				::Network.Clear();
			}
		}
		return true;
	}
	// Options
	if (SEqual2(szCommand,"Options:"))
	{
		// Music
		if (SEqual(szCommand + 8, "Music"))
		{
			Application.MusicSystem.ToggleOnOff();
		}
		// Sound
		if (SEqual(szCommand + 8, "Sound"))
		{
			if (Config.Sound.RXSound)
			{
				Application.SoundSystem.Clear();
				Config.Sound.RXSound = false;
			}
			else
			{
				Config.Sound.RXSound = true;
				if (!Application.SoundSystem.Init())
					{ Log(LoadResStr("IDS_PRC_NOSND")); }
			}
		}
		// Reopen with updated options
		ActivateOptions(Player, GetSelection());
		return true;
	}
	// Display
	if (SEqual2(szCommand,"Display:"))
	{
		// Upper board
		if (SEqual(szCommand + 8, "UpperBoard"))
		{
			Config.Graphics.UpperBoard = !Config.Graphics.UpperBoard;
			::Viewports.RecalculateViewports();
		}
		// FPS
		if (SEqual(szCommand + 8, "FPS")) Config.General.FPS = !Config.General.FPS;
		// Player names
		if (SEqual(szCommand + 8, "PlayerNames")) Config.Graphics.ShowCrewNames = !Config.Graphics.ShowCrewNames;
		// Clonk names
		if (SEqual(szCommand + 8, "ClonkNames")) Config.Graphics.ShowCrewCNames = !Config.Graphics.ShowCrewCNames;
		// Clock
		if (SEqual(szCommand + 8, "Clock")) Config.Graphics.ShowClock = !Config.Graphics.ShowClock;
		// Reopen with updated options
		ActivateDisplay(Player, GetSelection());
		return true;
	}
	// Goal info
	if (SEqual2(szCommand,"Player:Goal:") || SEqual2(szCommand,"Player:Rule:"))
	{
		if (!ValidPlr(Player)) return false; // observers may not look at goal/rule info, because it requires queue activation
		Close(true);
		C4Object *pObj; C4ID idItem(szCommand+12); C4Def * pDef = C4Id2Def(idItem);
		if (pDef && (pObj = ::Objects.Find(pDef)))
			::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::ActivateGoal(::Players.Get(Player), pObj), CDT_Queue);
		else
			return false;
		return true;
	}
	// Team selection
	if (SEqual2(szCommand, "TeamSel:"))
	{
		Close(true);
		int32_t idTeam = atoi(szCommand+8);

		// OK, join this team
		if (pPlr) pPlr->DoTeamSelection(idTeam);
		return true;
	}
	// Team switch
	if (SEqual2(szCommand, "TeamSwitch:"))
	{
		Close(true);
		int32_t idTeam = atoi(szCommand+11);

		// check if it's still allowed
		if (!Game.Teams.IsTeamSwitchAllowed()) return false;
		// OK, join this team
		::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::SetTeam(::Players.Get(Player), idTeam), CDT_Queue);
		return true;
	}
	// Observe
	if (SEqual2(szCommand, "Observe:"))
	{
		const char *szObserverTarget = szCommand+8;
		C4Viewport *pVP = ::Viewports.GetViewport(NO_OWNER);
		if (pVP) // viewport may have closed meanwhile
		{
			if (SEqual(szObserverTarget, "Free"))
			{
				// free view
				pVP->Init(NO_OWNER, true);
				return true;
			}
			else
			{
				// view following player
				int32_t iPlr = atoi(szObserverTarget);
				if (ValidPlr(iPlr))
				{
					pVP->Init(iPlr, true);
					return true;
				}
			}
		}
		return false;
	}
	// No valid command
	return false;
}

bool C4MainMenu::ActivateCommand(int32_t iPlayer, const char *szCommand)
{
	// init menu for player and activate with command
	Player = iPlayer;
	return MenuCommand(szCommand, false);
}

bool C4MainMenu::ActivateObserver()
{
	// Safety: Viewport lost?
	if (!::Viewports.GetViewport(NO_OWNER)) return false;
	// Menu symbol/init
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_View), LoadResStr("IDS_TEXT_VIEW"), NO_OWNER, C4MN_Extra_None, 0, C4MN_Observer, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	// Players added in Refill
	Refill();
	// Go back to main menu on close
	SetCloseCommand("ActivateMenu:Main");
	return true;
}
