/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2008  Matthes Bender
 * Copyright (c) 2001-2008  Sven Eberhardt
 * Copyright (c) 2002-2008  Peter Wortmann
 * Copyright (c) 2004  Armin Burgmeier
 * Copyright (c) 2005-2009  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

/* Player data at runtime */

#include <C4Include.h>
#include <C4Player.h>

#include <C4Application.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4Command.h>
#include <C4League.h>
#include <C4Network2Stats.h>
#include <C4MessageInput.h>
#include <C4GamePadCon.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4FullScreen.h>
#include <C4GameOverDlg.h>
#include <C4ObjectMenu.h>
#include <C4MouseControl.h>
#include <C4GameMessage.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>
#include <C4Viewport.h>

C4Player::C4Player() : C4PlayerInfoCore()
{
	Default();
}

C4Player::~C4Player()
{
	Clear();
}

bool C4Player::ObjectInCrew(C4Object *tobj)
{
	C4Object *cobj; C4ObjectLink *clnk;
	if (!tobj) return false;
	for (clnk=Crew.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		if (cobj==tobj) return true;
	return false;
}

void C4Player::ClearPointers(C4Object *pObj, bool fDeath)
{
	// Crew
	while (Crew.Remove(pObj)) {}
	// Cursor
	if (Cursor==pObj)
	{
		// object is to be deleted; do NOT do script calls (like in Cursor->UnSelect(true))
		Cursor=NULL; AdjustCursorCommand(); // also selects and eventually does a script call!
	}
	// View-Cursor
	if (ViewCursor==pObj) ViewCursor = NULL;
	// View
	if (ViewTarget==pObj) ViewTarget=NULL;
	// FoW
	// (do not clear locals!)
	// no clear when death to do normal decay
	if (!fDeath)
		while (FoWViewObjs.Remove(pObj)) {}
	// Menu
	Menu.ClearPointers(pObj);
	// messageboard-queries
	RemoveMessageBoardQuery(pObj);
}

bool C4Player::ScenarioAndTeamInit(int32_t idTeam)
{
	C4PlayerInfo *pInfo = GetInfo();
	if (!pInfo) return false;
	C4Team *pTeam;
	if (idTeam == TEAMID_New)
	{
		// creation of a new team only if allowed by scenario
		if (!Game.Teams.IsAutoGenerateTeams())
			pTeam = NULL;
		else
		{
			if ((pTeam = Game.Teams.GetGenerateTeamByID(idTeam))) idTeam = pTeam->GetID();
		}
	}
	else
	{
		// uage of an existing team
		pTeam = Game.Teams.GetTeamByID(idTeam);
	}
	C4Team *pPrevTeam = Game.Teams.GetTeamByID(Team);
	// check if join to team is possible; e.g. not too many players
	if (pPrevTeam != pTeam && idTeam)
	{
		if (!Game.Teams.IsJoin2TeamAllowed(idTeam))
		{
			pTeam = NULL;
		}
	}
	if (!pTeam && idTeam)
	{
		OnTeamSelectionFailed();
		return false;
	}
	// team selection OK; execute it!
	if (pPrevTeam) pPrevTeam->RemovePlayerByID(pInfo->GetID());
	if (pTeam) pTeam->AddPlayer(*pInfo, true);
	if (!ScenarioInit()) return false;
	if (!FinalInit(false)) return false;
	return true;
}

void C4Player::Execute()
{
	if (!Status) return;

	// Open/refresh team menu if desired
	if (Status==PS_TeamSelection)
	{
		int32_t idSelectedTeam;
		if ((idSelectedTeam = Game.Teams.GetForcedTeamSelection(ID)))
		{
			// There's only one team left to join? Join there immediately.
			if (Menu.IsActive() && Menu.GetIdentification() == C4MN_TeamSelection) Menu.TryClose(false, false);
			if (LocalControl && !::Control.isReplay())
			{
				// team selection done through queue because TeamSelection-status may not be in sync (may be TeamSelectionPending!)
				DoTeamSelection(idSelectedTeam);
			}
		}
		else if (!Menu.IsActive()) ActivateMenuTeamSelection(false);
		else
		{
			// during team selection: Update view to selected team, if it has a position assigned
			C4MenuItem *pSelectedTeamItem;
			if ((pSelectedTeamItem = Menu.GetSelectedItem()))
			{
				int32_t idSelectedTeam = pSelectedTeamItem->GetValue();
				if (idSelectedTeam)
				{
					C4Team *pSelectedTeam;
					if ((pSelectedTeam = Game.Teams.GetTeamByID(idSelectedTeam)))
					{
						int32_t iPlrStartIndex = pSelectedTeam->GetPlrStartIndex();
						if (iPlrStartIndex && Inside<int32_t>(iPlrStartIndex, 1, C4S_MaxPlayer))
						{
							if (Game.C4S.PlrStart[iPlrStartIndex-1].Position[0] > -1)
							{
								// player has selected a team that has a valid start position assigned
								// set view to this position!
								ViewX = Game.C4S.PlrStart[iPlrStartIndex-1].Position[0] * ::Landscape.MapZoom;
								ViewY = Game.C4S.PlrStart[iPlrStartIndex-1].Position[1] * ::Landscape.MapZoom;
							}
						}
					}
				}
			}
		}
	}
	else if (Menu.IsActive() && Menu.GetIdentification() == C4MN_TeamSelection)
	{
		Menu.TryClose(false, false);
	}

	// Tick1
	UpdateCounts();
	UpdateView();
	ExecuteControl();
	Menu.Execute();
	if (Cursor)
		Cursor->AutoContextMenu(-1);

	// decay of dead viewtargets
	C4ObjectLink *pLnkNext = FoWViewObjs.First, *pLnk;
	while ((pLnk = pLnkNext))
	{
		pLnkNext = pLnk->Next;
		C4Object *pDeadClonk = pLnk->Obj;
		if (!pDeadClonk->GetAlive() && (pDeadClonk->Category & C4D_Living) && pDeadClonk->Status)
		{
			pDeadClonk->PlrViewRange -= 10;
			if (pDeadClonk->PlrViewRange <= 0)
				FoWViewObjs.Remove(pDeadClonk);
		}
	}

	// ::Game.iTick35
	if (!::Game.iTick35 && Status==PS_Normal)
	{
		ExecHomeBaseProduction();
		CheckElimination();
		if (pMsgBoardQuery && LocalControl) ExecMsgBoardQueries();
	}

	// Delays
	if (MessageStatus>0) MessageStatus--;
	if (RetireDelay>0) RetireDelay--;
	if (ViewWealth>0) ViewWealth--;
	if (ViewScore>0) ViewScore--;
	if (CursorFlash>0) CursorFlash--;
}

bool C4Player::Init(int32_t iNumber, int32_t iAtClient, const char *szAtClientName,
                    const char *szFilename, bool fScenarioInit, class C4PlayerInfo *pInfo)
{
	// safety
	if (!pInfo)
	{
		LogF("ERROR: Init player %s failed: No info!", szFilename);
		assert(false);
		return false;
	}
	// Status init
	Status=PS_Normal;
	if (szFilename)
		SCopy(Config.AtDataReadPath(szFilename),Filename);
	else
		*Filename='\0';
	Number = iNumber;
	ID = pInfo->GetID();
	Team = pInfo->GetTeam();
	NoEliminationCheck = pInfo->IsNoEliminationCheck();

	// At client
	AtClient=iAtClient; SCopy(szAtClientName,AtClientName,C4MaxTitle);

	if (Filename)
	{
		// Load core & crew info list
		// do not load portraits for remote players
		// this will prevent portraits from being shown for "remotely controlled"-Clonks of other players
		bool fLoadPortraits = (AtClient==C4ClientIDUnknown) || SEqualNoCase(AtClientName, Game.Clients.getLocalName());
		// fLoadPortraits = true
		if (!Load(Filename, !fScenarioInit, fLoadPortraits)) return false;
	}
	else
	{
		// no core file present: Keep defaults
		// This can happen for script players only
		assert(pInfo->GetType() == C4PT_Script);
	}

	// Take player name from player info; forcing overloads by the league or because of doubled player names
	Name.Copy(pInfo->GetName());

	// view pos init: Start at center pos
	ViewX = GBackWdt/2; ViewY = GBackHgt/2;

	// Scenario init
	if (fScenarioInit)
	{
		// mark player join in player info list
		// for non-scenarioinit, player should already be marked as joined
		pInfo->SetJoined(iNumber);

		// Number might have changed: Recheck list sorting before scenarioinit, which will do script calls
		::Players.RecheckPlayerSort(this);

		// check for a postponed scenario init, if no team is specified (post-lobby-join in network, or simply non-network)
		C4Team *pTeam = NULL;
		if (Team)
		{
			if (Game.Teams.IsAutoGenerateTeams())
				pTeam = Game.Teams.GetGenerateTeamByID(Team);
			else
				pTeam = Game.Teams.GetTeamByID(Team);
		}
		if (!pTeam && Game.Teams.IsRuntimeJoinTeamChoice())
		{
			if (pInfo->GetType() == C4PT_Script)
			{
				// script player without team: This can usually not happen, because RecheckPlayerInfoTeams should have been executed
				// just leave this player without the team
				assert(false);
			}
			else
			{
				// postponed init: Chose team first
				Status = PS_TeamSelection;
			}
		}

		// Init control method before scenario init, because script callbacks may need to know it!
		InitControl();

		// defaultdisabled controls
		Control.Init();

		// Special: Script players may skip scenario initialization altogether, and just desire a single callback to all objects
		// of a given ID
		if (!pInfo->IsScenarioInitDesired())
		{
			// only initialization that's done anyway is team hostility
			if (Team) SetTeamHostility();
			// callback definition passed?
			C4ID idCallback = pInfo->GetScriptPlayerExtraID();
			C4Def *pDefCallback;
			if (idCallback && (pDefCallback = C4Id2Def(idCallback)))
			{
				pDefCallback->Script.Call(PSF_InitializeScriptPlayer, 0,
				                          &C4AulParSet(C4VInt(Number), C4VInt(Team)));
			}
		}
		else
		{
			// player preinit: In case a team needs to be chosen first, no InitializePlayer-broadcast is done
			// this callback shall give scripters a chance to do stuff like starting an intro or enabling FoW, which might need to be done
			Game.Script.GRBroadcast(PSF_PreInitializePlayer, &C4AulParSet(C4VInt(Number)));
			// direct init
			if (Status != PS_TeamSelection) if (!ScenarioInit()) return false;
		}
	}

	// Load runtime data
	else
	{
		assert(pInfo->IsJoined());
		// (compile using DefaultRuntimeData) - also check if compilation returned sane results, i.e. ID assigned
		if (!LoadRuntimeData(Game.ScenarioFile) || !ID)
		{
			// for script players in non-savegames, this is OK - it means they get restored using default values
			// this happens when the users saves a scenario using the "Save scenario"-option while a script player
			// was joined
			if (!Game.C4S.Head.SaveGame && pInfo->GetType() == C4PT_Script)
			{
				Number = pInfo->GetInGameNumber();
				ColorDw = pInfo->GetColor();
				ID = pInfo->GetID();
				Team = pInfo->GetTeam();
			}
			else
				return false;
		}
		// Reset values default-overriden by old runtime data load (safety?)
		if (Number==C4P_Number_None) Number=iNumber;
		if (szFilename) SCopy(Config.AtUserDataPath(szFilename),Filename); else *Filename='\0';
		// NET2: Direct joins always send accurate client IDs and names in params
		// do not overwrite them with savegame data, because players might as well
		// change clients
		// (only call should be savegame recreation by C4PlayerInfoList::RecreatePlayers)
		AtClient = iAtClient;
		SCopy(szAtClientName,AtClientName,C4MaxTitle);
		// Number might have changed: Recheck list sorting
		::Players.RecheckPlayerSort(this);
		// Init control after loading runtime data, because control init will overwrite some of the values
		InitControl();
	}

	// store game joining time
	GameJoinTime = Game.Time;

	// Init FoW-viewobjects: NO_OWNER-FoW-repellers might need to be added
	for (C4ObjectLink *pLnk = ::Objects.First; pLnk; pLnk = pLnk->Next)
	{
		C4Object *pObj = pLnk->Obj;
		if (pObj->PlrViewRange && pObj->Owner == NO_OWNER)
			pObj->PlrFoWActualize();
	}

	// init graphs
	if (Game.pNetworkStatistics) CreateGraphs();

	return true;
}

bool C4Player::Save()
{
	C4Group hGroup;
	// Regular player saving need not be done for script players
	if (GetType() == C4PT_Script) return false;
	// Log
	LogF(LoadResStr("IDS_PRC_SAVEPLR"), Config.AtRelativePath(Filename));
	::GraphicsSystem.MessageBoard.EnsureLastMessage();
	// copy player to save somewhere else
	char szPath[_MAX_PATH + 1];
	SCopy(Config.AtTempPath(C4CFN_TempPlayer), szPath, _MAX_PATH);
	MakeTempFilename(szPath);
	// so full hard (flgr stupid) disks won't corrupt any player files...
	C4Group_CopyItem(Filename, szPath);
	// Open group
	if (!hGroup.Open(szPath,true))
		return false;
	// Save
	if (!Save(hGroup, false, !LocalControl))
		{ hGroup.Close(); return false; }
	// Close group
	if (!hGroup.Close()) return false;
	// resource
	C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(Filename),
	                          pDRes = NULL;
	bool fOfficial = pRes && ::Control.isCtrlHost();
	if (pRes) pDRes = pRes->Derive();
	// move back
	if (ItemExists(Filename)) EraseItem(Filename);
	if (!C4Group_MoveItem(szPath, Filename)) return false;
	// finish update
	if (pDRes && fOfficial) pDRes->FinishDerive();
	// Success
	return true;
}

bool C4Player::Save(C4Group &hGroup, bool fSavegame, bool fStoreTiny)
{
	// Save core
	if (!C4PlayerInfoCore::Save(hGroup))
		return false;
	// Save crew
	C4DefList *pDefs = &::Definitions;
	if (!CrewInfoList.Save(hGroup, fSavegame, fStoreTiny, pDefs))
		{ hGroup.Close(); return false; }
	// Sort
	hGroup.Sort(C4FLS_Player);
	return true;
}

void C4Player::PlaceReadyCrew(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase)
{
	int32_t cnt,crewnum,ctx,cty;
	C4Object *nobj;
	C4ObjectInfo *pInfo;
	C4Def *pDef;

	// Old specification
	if (Game.C4S.PlrStart[PlrStartIndex].ReadyCrew.IsClear())
	{
		// Target number of ready crew
		crewnum=Game.C4S.PlrStart[PlrStartIndex].Crew.Evaluate();
		// Place crew
		for (cnt=0; cnt<crewnum; cnt++)
		{
			// Set standard crew
			C4ID idStdCrew = Game.C4S.PlrStart[PlrStartIndex].NativeCrew;
			// Select member from home crew, add new if necessary
			while (!(pInfo=CrewInfoList.GetIdle(idStdCrew,::Definitions)))
				if (!CrewInfoList.New(idStdCrew,&::Definitions))
					break;
			// Crew placement location
			if (!pInfo || !(pDef=C4Id2Def(pInfo->id))) continue;
			ctx=tx1+Random(tx2-tx1); cty=ty;
			if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
				FindSolidGround(ctx,cty,pDef->Shape.Wdt*3);
			// Create object
			if ((nobj=Game.CreateInfoObject(pInfo,Number,ctx,cty)))
			{
				// Add object to crew
				Crew.Add(nobj, C4ObjectList::stMain);
				// add visibility range
				nobj->SetPlrViewRange(C4FOW_Def_View_RangeX);
				// If base is present, enter base
				if (FirstBase) { nobj->Enter(FirstBase); nobj->SetCommand(C4CMD_Exit); }
				// OnJoinCrew callback
#ifndef DEBUGREC_RECRUITMENT
				C4DebugRecOff DBGRECOFF;
#endif
				C4AulParSet parset(C4VInt(Number));
				nobj->Call(PSF_OnJoinCrew, &parset);
			}
		}
	}

	// New specification
	else
	{
		// Place crew
		int32_t iCount;
		C4ID id;
		for (cnt=0; (id=Game.C4S.PlrStart[PlrStartIndex].ReadyCrew.GetID(cnt,&iCount)); cnt++)
		{

			// Minimum one clonk if empty id
			iCount = Max<int32_t>(iCount,1);

			for (int32_t cnt2=0; cnt2<iCount; cnt2++)
			{
				// Select member from home crew, add new if necessary
				while (!(pInfo=CrewInfoList.GetIdle(id,::Definitions)))
					if (!CrewInfoList.New(id,&::Definitions))
						break;
				// Safety
				if (!pInfo || !(pDef=C4Id2Def(pInfo->id))) continue;
				// Crew placement location
				ctx=tx1+Random(tx2-tx1); cty=ty;
				if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
					FindSolidGround(ctx,cty,pDef->Shape.Wdt*3);
				// Create object
				if ((nobj=Game.CreateInfoObject(pInfo,Number,ctx,cty)))
				{
					// Add object to crew
					Crew.Add(nobj, C4ObjectList::stMain);
					// add visibility range
					nobj->SetPlrViewRange(C4FOW_Def_View_RangeX);
					// If base is present, enter base
					if (FirstBase) { nobj->Enter(FirstBase); nobj->SetCommand(C4CMD_Exit); }
					// OnJoinCrew callback
					{
#if !defined(DEBUGREC_RECRUITMENT)
						C4DebugRecOff DbgRecOff;
#endif
						C4AulParSet parset(C4VInt(Number));
						nobj->Call(PSF_OnJoinCrew, &parset);
					}
				}
			}

		}
	}

}

C4Object *CreateLine(C4ID linetype, int32_t owner, C4Object *fobj, C4Object *tobj);

bool CreatePowerConnection(C4Object *fbase, C4Object *tbase)
{
	if (CreateLine(C4ID::PowerLine,fbase->Owner,fbase,tbase)) return true;
	return false;
}

void C4Player::PlaceReadyBase(int32_t &tx, int32_t &ty, C4Object **pFirstBase)
{
	int32_t cnt,cnt2,ctx,cty;
	C4Def *def;
	C4ID cid;
	C4Object *cbase,*fpower=NULL;
	// Create ready base structures
	for (cnt=0; (cid=Game.C4S.PlrStart[PlrStartIndex].ReadyBase.GetID(cnt)); cnt++)
	{
		if ((def=C4Id2Def(cid)))
			for (cnt2=0; cnt2<Game.C4S.PlrStart[PlrStartIndex].ReadyBase.GetCount(cnt); cnt2++)
			{
				ctx=tx; cty=ty;
				if (Game.C4S.PlrStart[PlrStartIndex].EnforcePosition
				    || FindConSiteSpot(ctx,cty,def->Shape.Wdt,def->Shape.Hgt,def->Category,20))
					if ((cbase=Game.CreateObjectConstruction(C4Id2Def(cid),NULL,Number,ctx,cty,FullCon,true)))
					{
						// FirstBase
						if (!(*pFirstBase)) if ((cbase->Def->Entrance.Wdt>0) && (cbase->Def->Entrance.Hgt>0))
								{ *pFirstBase=cbase; tx=(*pFirstBase)->GetX(); ty=(*pFirstBase)->GetY(); }
						// First power plant
						if (cbase->Def->LineConnect & C4D_Power_Generator)
							if (!fpower) fpower=cbase;
					}
			}
	}

	// Power connections
	C4ObjectLink *clnk; C4Object *cobj;
	if (Game.Rules & C4RULE_StructuresNeedEnergy)
		if (fpower)
			for (clnk=::Objects.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
				if (cobj->Owner==Number)
					if (cobj->Def->LineConnect & C4D_Power_Consumer)
						CreatePowerConnection(fpower,cobj);
}

void C4Player::PlaceReadyVehic(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase)
{
	int32_t cnt,cnt2,ctx,cty;
	C4Def *def; C4ID cid; C4Object *cobj;
	for (cnt=0; (cid=Game.C4S.PlrStart[PlrStartIndex].ReadyVehic.GetID(cnt)); cnt++)
	{
		if ((def=C4Id2Def(cid)))
			for (cnt2=0; cnt2<Game.C4S.PlrStart[PlrStartIndex].ReadyVehic.GetCount(cnt); cnt2++)
			{
				ctx=tx1+Random(tx2-tx1); cty=ty;
				if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
					FindLevelGround(ctx,cty,def->Shape.Wdt,6);
				if ((cobj=Game.CreateObject(cid,NULL,Number,ctx,cty)))
				{
					if (FirstBase) // First base overrides target location
						{ cobj->Enter(FirstBase); cobj->SetCommand(C4CMD_Exit); }
				}
			}
	}
}

void C4Player::PlaceReadyMaterial(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase)
{
	int32_t cnt,cnt2,ctx,cty;
	C4Def *def; C4ID cid;

	// In base
	if (FirstBase)
	{
		FirstBase->CreateContentsByList(Game.C4S.PlrStart[PlrStartIndex].ReadyMaterial);
	}

	// Outside
	else
	{
		for (cnt=0; (cid=Game.C4S.PlrStart[PlrStartIndex].ReadyMaterial.GetID(cnt)); cnt++)
		{
			if ((def=C4Id2Def(cid)))
				for (cnt2=0; cnt2<Game.C4S.PlrStart[PlrStartIndex].ReadyMaterial.GetCount(cnt); cnt2++)
				{
					ctx=tx1+Random(tx2-tx1); cty=ty;
					if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
						FindSolidGround(ctx,cty,def->Shape.Wdt);
					Game.CreateObject(cid,NULL,Number,ctx,cty);
				}
		}
	}
}

DWORD RandomPlayerColor() // generate a random player color
{
	return 0xff<<24 | RGB(Min(SafeRandom(302), 256), Min(SafeRandom(302), 256), Min(SafeRandom(302), 256));
}

bool C4Player::ScenarioInit()
{
	int32_t ptx,pty;

	// player start index by team, if specified. Otherwise by player number
	PlrStartIndex = Number % C4S_MaxPlayer;
	C4Team *pTeam; int32_t i;
	if (Team && (pTeam = Game.Teams.GetTeamByID(Team))) if ((i=pTeam->GetPlrStartIndex())) PlrStartIndex=i-1;

	// Set color
	int32_t iColor=BoundBy<int32_t>(PrefColor,0,C4MaxColor-1);
	while (::Players.ColorTaken(iColor))
		{ ++iColor%=C4MaxColor; if (iColor==PrefColor) break; }
	Color=iColor;

	C4PlayerInfo *pInfo = GetInfo();
	if (!pInfo) { assert(false); LogF("Internal error: ScenarioInit for ghost player %s!", GetName()); return false; }

	// set color by player info class
	// re-setting, because runtime team choice may have altered color
	ColorDw = pInfo->GetColor();

	// any team selection is over now
	Status = PS_Normal;

	// Wealth, home base materials, abilities
	Wealth=Game.C4S.PlrStart[PlrStartIndex].Wealth.Evaluate();
	HomeBaseMaterial=Game.C4S.PlrStart[PlrStartIndex].HomeBaseMaterial;
	HomeBaseMaterial.ConsolidateValids(::Definitions);
	HomeBaseProduction=Game.C4S.PlrStart[PlrStartIndex].HomeBaseProduction;
	HomeBaseProduction.ConsolidateValids(::Definitions);
	Knowledge=Game.C4S.PlrStart[PlrStartIndex].BuildKnowledge;
	Knowledge.ConsolidateValids(::Definitions);
	Magic=Game.C4S.PlrStart[PlrStartIndex].Magic;
	Magic.ConsolidateValids(::Definitions);
	if (Magic.IsClear()) Magic.Load(::Definitions,C4D_Magic); // All magic default if empty
	Magic.SortByValue(::Definitions);

	// Starting position
	ptx = Game.C4S.PlrStart[PlrStartIndex].Position[0];
	pty = Game.C4S.PlrStart[PlrStartIndex].Position[1];

	// Zoomed position
	if (ptx>-1) ptx = BoundBy<int32_t>( ptx * Game.C4S.Landscape.MapZoom.Evaluate(), 0, GBackWdt-1 );
	if (pty>-1) pty = BoundBy<int32_t>( pty * Game.C4S.Landscape.MapZoom.Evaluate(), 0, GBackHgt-1 );

	// Standard position (PrefPosition)
	if (ptx<0)
		if (Game.StartupPlayerCount>=2)
		{
			int32_t iMaxPos=Game.StartupPlayerCount;
			// Map preferred position to available positions
			int32_t iStartPos=BoundBy(PrefPosition*iMaxPos/C4P_MaxPosition,0,iMaxPos-1);
			int32_t iPosition=iStartPos;
			// Distribute according to availability
			while (::Players.PositionTaken(iPosition))
				{ ++iPosition%=iMaxPos; if (iPosition==iStartPos) break; }
			Position=iPosition;
			// Set x position
			ptx=BoundBy(16+Position*(GBackWdt-32)/(iMaxPos-1),0,GBackWdt-16);
		}

	// All-random position
	if (ptx<0) ptx=16+Random(GBackWdt-32);
	if (pty<0) pty=16+Random(GBackHgt-32);

	// Place to solid ground
	if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
	{
		// Use nearest above-ground...
		FindSolidGround(ptx,pty,30);
		// Might have hit a small lake, or similar: Seach a real site spot from here
		FindConSiteSpot(ptx, pty, 30,50,C4D_Structure, 400);
	}

	// Place Readies
	C4Object *FirstBase = NULL;
	PlaceReadyBase(ptx,pty,&FirstBase);
	PlaceReadyMaterial(ptx-10,ptx+10,pty,FirstBase);
	PlaceReadyVehic(ptx-30,ptx+30,pty,FirstBase);
	PlaceReadyCrew(ptx-30,ptx+30,pty,FirstBase);

	// set initial hostility by team info
	if (Team) SetTeamHostility();

	if (fFogOfWar && !fFogOfWarInitialized)
	{
		fFogOfWarInitialized = true;
		// reset view objects
		::Objects.AssignPlrViewRange();
	}

	// Scenario script initialization
	Game.Script.GRBroadcast(PSF_InitializePlayer, &C4AulParSet(C4VInt(Number),
	                        C4VInt(ptx),
	                        C4VInt(pty),
	                        C4VObj(FirstBase),
	                        C4VInt(Team),
	                        C4VID(GetInfo()->GetScriptPlayerExtraID())));
	return true;
}

bool C4Player::FinalInit(bool fInitialScore)
{
	if (!Status) return true;

	// Init player's mouse control
	if (LocalControl)
		if (MouseControl)
			::MouseControl.Init(Number);

	// Set initial score
	if (fInitialScore)
	{
		InitialScore=CurrentScore;
	}

	// Cursor
	if (!Cursor) AdjustCursorCommand();

	// Update counts, pointers, views
	Execute();

	// Restore FoW after savegame
	if (fFogOfWar && !fFogOfWarInitialized)
	{
		fFogOfWarInitialized = true;
		// reset view objects
		::Objects.AssignPlrViewRange();
	}

	return true;
}

void C4Player::SetFoW(bool fEnable)
{
	// enable FoW
	if (fEnable && !fFogOfWarInitialized)
		::Objects.AssignPlrViewRange();
	// set flag
	fFogOfWar = fFogOfWarInitialized = fEnable;
}

bool C4Player::DoWealth(int32_t iChange)
{
	if (LocalControl)
	{
		if (iChange>0) StartSoundEffect("Cash");
		if (iChange<0) StartSoundEffect("UnCash");
	}
	SetWealth(Wealth+iChange);

	return true;
}

bool C4Player::SetWealth(int32_t iVal)
{
	if (iVal == Wealth) return true;

	Wealth=BoundBy<int32_t>(iVal,0,10000);
	ViewWealth = C4ViewDelay;

	Game.Script.GRBroadcast(PSF_OnWealthChanged,&C4AulParSet(C4VInt(Number)));

	return true;
}

void C4Player::SetViewMode(int32_t iMode, C4Object *pTarget)
{
	// safe back
	ViewMode=iMode; ViewTarget=pTarget;
}

void C4Player::ResetCursorView()
{
	// reset view to cursor if any cursor exists
	if (!ViewCursor && !Cursor) return;
	SetViewMode(C4PVM_Cursor);
}

void C4Player::Evaluate()
{
	// do not evaluate twice
	if (Evaluated) return;

	const int32_t SuccessBonus=100;

	// Set last round
	LastRound.Title = Game.ScenarioTitle;
	time(reinterpret_cast<time_t *>(&LastRound.Date));
	LastRound.Duration = Game.Time;
	LastRound.Won = !Eliminated;
	// Melee: personal value gain score ...check ::Objects(C4D_Goal)
	if (Game.C4S.Game.IsMelee()) LastRound.Score = Max<int32_t>(CurrentScore-InitialScore,0);
	// Cooperative: shared score
	else LastRound.Score = Max(::Players.AverageScoreGain(),0);
	LastRound.Level = 0; // unknown...
	LastRound.Bonus = SuccessBonus * LastRound.Won;
	LastRound.FinalScore = LastRound.Score + LastRound.Bonus;
	LastRound.TotalScore = TotalScore + LastRound.FinalScore;

	// Update player
	Rounds++;
	if (LastRound.Won) RoundsWon++; else RoundsLost++;
	TotalScore=LastRound.TotalScore;
	TotalPlayingTime+=Game.Time-GameJoinTime;

	// Crew
	CrewInfoList.Evaluate();

	// league
	if (Game.Parameters.isLeague())
		EvaluateLeague(false, Game.GameOver && !Eliminated);

	// Player is now evaluated
	Evaluated = true;

	// round results
	Game.RoundResults.EvaluatePlayer(this);
}

void C4Player::Surrender()
{
	if (Surrendered) return;
	Surrendered=true;
	Eliminated=true;
	RetireDelay=C4RetireDelay;
	StartSoundEffect("Eliminated");
	Log(FormatString(LoadResStr("IDS_PRC_PLRSURRENDERED"),GetName()).getData());
}

bool C4Player::SetHostility(int32_t iOpponent, int32_t hostile, bool fSilent)
{
	assert(hostile == 0 || hostile == 1);
	// Check opponent valid
	C4Player *opponent = ::Players.Get(iOpponent);
	if (!opponent || opponent == this)
		return false;
	// Set hostility
	if (hostile)
		Hostility.insert(opponent);
	else
		Hostility.erase(opponent);
	// no announce in first frame, or if specified
	if (!Game.FrameCounter || fSilent) return true;
	// Announce
	StartSoundEffect("Trumpet");
	Log(FormatString(LoadResStr(hostile ? "IDS_PLR_HOSTILITY" : "IDS_PLR_NOHOSTILITY"),
	                 GetName(),opponent->GetName()).getData());
	// Success
	return true;
}

bool C4Player::IsHostileTowards(const C4Player *plr) const
{
	assert(plr);
	if (!plr) return false;
	return Hostility.find(plr) != Hostility.end();
}

C4Object* C4Player::GetHiExpActiveCrew()
{
	C4ObjectLink *clnk;
	C4Object *cobj,*hiexp=NULL;
	int32_t iHighestExp=-2, iExp;
	for (clnk=Crew.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		if (!cobj->CrewDisabled)
		{
			if (cobj->Info) iExp = cobj->Info->Experience; else iExp=-1;
			if (!hiexp || (iExp>iHighestExp))
			{
				hiexp=cobj;
				iHighestExp=iExp;
			}
		}
	return hiexp;
}

C4Object* C4Player::GetHiRankActiveCrew()
{
	C4ObjectLink *clnk;
	C4Object *cobj,*hirank=NULL;
	int32_t iHighestRank=-2, iRank;
	for (clnk=Crew.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		if (!cobj->CrewDisabled)
		{
			if (cobj->Info) iRank = cobj->Info->Rank; else iRank=-1;
			if (!hirank || (iRank>iHighestRank))
			{
				hirank=cobj;
				iHighestRank=iRank;
			}
		}
	return hirank;
}

void C4Player::CheckCrewExPromotion()
{
	C4Object *hirank;
	if ((hirank=GetHiRankActiveCrew()))
		if (hirank->Info)
			if (hirank->Info->Rank<1) // No Fähnrich -> except. promo.
				if ((hirank=GetHiExpActiveCrew()))
					hirank->Promote(1,true,false);
}

void C4Player::SetTeamHostility()
{
	// team only
	if (!Team) return;
	// set hostilities
	for (C4Player *pPlr = ::Players.First; pPlr; pPlr = pPlr->Next)
		if (pPlr != this)
		{
			bool fHostile = (pPlr->Team != Team);
			SetHostility(pPlr->Number, fHostile, true);
			pPlr->SetHostility(Number, fHostile, true);
		}
}

bool C4Player::Message(const char *szMsg)
{
	if (!szMsg) return false;
	SCopy(szMsg,MessageBuf,256);
	MessageStatus=SLen(szMsg)*2;
	return true;
}

void C4Player::Clear()
{
	ClearGraphs();
	Crew.Clear();
	CrewInfoList.Clear();
	Menu.Clear();
	BigIcon.Clear();
	fFogOfWar=true;
	FoWViewObjs.Clear();
	fFogOfWarInitialized=false;
	while (pMsgBoardQuery)
	{
		C4MessageBoardQuery *pNext = pMsgBoardQuery->pNext;
		delete pMsgBoardQuery;
		pMsgBoardQuery = pNext;
	}
	if (pGamepad) delete pGamepad;
	pGamepad = NULL;
	Status = 0;
	ClearControl();
}

void C4Player::Default()
{
	Filename[0]=0;
	Number=C4P_Number_None;
	ID=0;
	Team = 0;
	DefaultRuntimeData();
	Menu.Default();
	Crew.Default();
	CrewInfoList.Default();
	LocalControl=false;
	BigIcon.Default();
	Next=NULL;
	fFogOfWar=true; fFogOfWarInitialized=false;
	FoWViewObjs.Default();
	LeagueEvaluated=false;
	GameJoinTime=0; // overwritten in Init
	pstatControls = pstatActions = NULL;
	ControlCount = ActionCount = 0;
	LastControlType = PCID_None;
	LastControlID = 0;
	pMsgBoardQuery = NULL;
	pGamepad = NULL;
	NoEliminationCheck = false;
	Evaluated = false;
	ZoomLimitMinWdt=ZoomLimitMinHgt=ZoomLimitMaxWdt=ZoomLimitMaxHgt=ZoomWdt=ZoomHgt=0;
	ViewLock = false;
}

bool C4Player::Load(const char *szFilename, bool fSavegame, bool fLoadPortraits)
{
	C4Group hGroup;
	// Open group
	if (!hGroup.Open(szFilename)) return false;
	// Load core
	if (!C4PlayerInfoCore::Load(hGroup))
		{ hGroup.Close(); return false; }
	// Load BigIcon
	if (hGroup.FindEntry(C4CFN_BigIcon)) BigIcon.Load(hGroup, C4CFN_BigIcon);
	// Load crew info list
	CrewInfoList.Load(hGroup, fLoadPortraits);
	// Close group
	hGroup.Close();
	// Success
	return true;
}

bool C4Player::Strip(const char *szFilename, bool fAggressive)
{
	// Opem group
	C4Group Grp;
	if (!Grp.Open(szFilename))
		return false;
	// Which type of stripping?
	if (!fAggressive)
	{
		// remove portrais
		Grp.Delete(C4CFN_Portraits, true);
		// remove bigicon, if the file size is too large
		size_t iBigIconSize=0;
		if (Grp.FindEntry(C4CFN_BigIcon, NULL, &iBigIconSize))
			if (iBigIconSize > C4NetResMaxBigicon*1024)
				Grp.Delete(C4CFN_BigIcon);
		Grp.Close();
	}
	else
	{
		// Load info core and crew info list
		C4PlayerInfoCore PlrInfoCore;
		C4ObjectInfoList CrewInfoList;
		if (!PlrInfoCore.Load(Grp) || !CrewInfoList.Load(Grp, false))
			return false;
		// Strip crew info list (remove object infos that are invalid for this scenario)
		CrewInfoList.Strip(::Definitions);
		// Create a new group that receives the bare essentials
		Grp.Close();
		if (!EraseItem(szFilename) ||
		    !Grp.Open(szFilename, true))
			return false;
		// Save info core & crew info list to newly-created file
		if (!PlrInfoCore.Save(Grp) || !CrewInfoList.Save(Grp, true, true, &::Definitions))
			return false;
		Grp.Close();
	}
	return true;
}

void C4Player::DrawHostility(C4Facet &cgo, int32_t iIndex)
{
	C4Player *pPlr;
	if ((pPlr=::Players.GetByIndex(iIndex)))
	{
		::GraphicsResource.fctCrewClr.DrawClr(cgo, true, pPlr->ColorDw);
		// Other player and hostile
		if (pPlr != this)
			if (Hostility.find(pPlr) != Hostility.end())
				::GraphicsResource.fctMenu.GetPhase(7).Draw(cgo);
	}
}

bool C4Player::MakeCrewMember(C4Object *pObj, bool fForceInfo, bool fDoCalls)
{
	C4ObjectInfo *cInf = NULL;
	if (!pObj || !pObj->Def->CrewMember || !pObj->Status) return false;

	// only if info is not yet assigned
	if (!pObj->Info && fForceInfo)
	{
		// Find crew info by name
		if (pObj->nInfo)
			cInf = CrewInfoList.GetIdle(pObj->nInfo.getData());

		// Find crew info by id
		if (!cInf)
			while (!( cInf = CrewInfoList.GetIdle(pObj->id,::Definitions) ))
				if (!CrewInfoList.New(pObj->id,&::Definitions))
					return false;

		// Set object info
		pObj->Info = cInf;
		pObj->SetName(cInf->Name);
	}

	// Add to crew
	if (!Crew.GetLink(pObj))
		Crew.Add(pObj, C4ObjectList::stMain);

	// add plr view
	if (!pObj->PlrViewRange) pObj->SetPlrViewRange(C4FOW_Def_View_RangeX); else pObj->PlrFoWActualize();

	// controlled by the player
	pObj->Controller = Number;

	// OnJoinCrew callback
	if (fDoCalls)
	{
		C4AulParSet parset(C4VInt(Number));
		pObj->Call(PSF_OnJoinCrew, &parset);
	}

	return true;
}

void C4Player::ExecuteControl()
{
	Control.Execute();
}

void C4Player::AdjustCursorCommand()
{
	// Reset view
	ResetCursorView();
	// Default cursor to hirank clonk
	if (!Cursor || Cursor->CrewDisabled)
	{
		C4Object *pHiRank = GetHiRankActiveCrew();
		if (!pHiRank)
			return;
		SetCursor(pHiRank,true);
		UpdateView();
	}
}

void C4Player::CursorRight()
{
	C4ObjectLink *cLnk;
	// Get next crew member
	if ((cLnk=Crew.GetLink(Cursor)))
		for (cLnk=cLnk->Next; cLnk; cLnk=cLnk->Next)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (!cLnk)
		for (cLnk=Crew.First; cLnk; cLnk=cLnk->Next)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (cLnk) SetCursor(cLnk->Obj, true);
	// Updates
	CursorFlash=30;
	UpdateView();
}

void C4Player::CursorLeft()
{
	C4ObjectLink *cLnk;
	// Get prev crew member
	if ((cLnk=Crew.GetLink(Cursor)))
		for (cLnk=cLnk->Prev; cLnk; cLnk=cLnk->Prev)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (!cLnk)
		for (cLnk=Crew.Last; cLnk; cLnk=cLnk->Prev)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (cLnk) SetCursor(cLnk->Obj, true);
	// Updates
	CursorFlash=30;
	UpdateView();
}

bool C4Player::ObjectCommand(int32_t iCommand, C4Object *pTarget, int32_t iX, int32_t iY, C4Object *pTarget2, C4Value iData, int32_t iMode)
{
	// Eliminated
	if (Eliminated) return false;
	// Hide startup
	if (ShowStartup) ShowStartup=false;
	// Always apply to cursor, even if it's not in the crew
	if (Cursor && Cursor->Status && Cursor != pTarget)
		ObjectCommand2Obj(Cursor, iCommand, pTarget, iX, iY, pTarget2, iData, iMode);

	// Success
	return true;
}

void C4Player::ObjectCommand2Obj(C4Object *cObj, int32_t iCommand, C4Object *pTarget, int32_t iX, int32_t iY, C4Object *pTarget2, C4Value iData, int32_t iMode)
{
	// forward to object
	if (iMode & C4P_Command_Append) cObj->AddCommand(iCommand,pTarget,iX,iY,0,pTarget2,true,iData,true,0,NULL,C4CMD_Mode_Base); // append: by Shift-click and for dragging of multiple objects (all independant; thus C4CMD_Mode_Base)
	else if (iMode & C4P_Command_Add) cObj->AddCommand(iCommand,pTarget,iX,iY,0,pTarget2,true,iData,false,0,NULL,C4CMD_Mode_Base); // append: by context menu and keyboard throw command (all independant; thus C4CMD_Mode_Base)
	else if (iMode & C4P_Command_Set) cObj->SetCommand(iCommand,pTarget,iX,iY,pTarget2,true,iData);
}

void C4Player::CompileFunc(StdCompiler *pComp, bool fExact)
{
	assert(ID);

	pComp->Value(mkNamingAdapt(Status,              "Status",               0));
	pComp->Value(mkNamingAdapt(AtClient,            "AtClient",             C4ClientIDUnknown));
	pComp->Value(mkNamingAdapt(toC4CStr(AtClientName),"AtClientName",        "Local"));
	pComp->Value(mkNamingAdapt(Number,              "Index",                C4P_Number_None));
	pComp->Value(mkNamingAdapt(ID,                  "ID",                   0));
	pComp->Value(mkNamingAdapt(Eliminated,          "Eliminated",           0));
	pComp->Value(mkNamingAdapt(Surrendered,         "Surrendered",          0));
	pComp->Value(mkNamingAdapt(Evaluated,            "Evaluated",            false));
	pComp->Value(mkNamingAdapt(Color,               "Color",                -1));
	pComp->Value(mkNamingAdapt(ColorDw,             "ColorDw",              0u));
	pComp->Value(mkNamingAdapt(Position,            "Position",             0));
	pComp->Value(mkNamingAdapt(ViewMode,            "ViewMode",             C4PVM_Cursor));
	pComp->Value(mkNamingAdapt(ViewX,               "ViewX",                0));
	pComp->Value(mkNamingAdapt(ViewY,               "ViewY",                0));
	pComp->Value(mkNamingAdapt(ViewLock,            "ViewLock",             false));
	pComp->Value(mkNamingAdapt(ZoomLimitMinWdt,     "ZoomLimitMinWdt",      0));
	pComp->Value(mkNamingAdapt(ZoomLimitMinHgt,     "ZoomLimitMinHgt",      0));
	pComp->Value(mkNamingAdapt(ZoomLimitMaxWdt,     "ZoomLimitMaxWdt",      0));
	pComp->Value(mkNamingAdapt(ZoomLimitMaxHgt,     "ZoomLimitMaxHgt",      0));
	pComp->Value(mkNamingAdapt(ZoomWdt,             "ZoomWdt",              0));
	pComp->Value(mkNamingAdapt(ZoomHgt,             "ZoomHgt",              0));
	pComp->Value(mkNamingAdapt(ViewWealth,          "ViewWealth",           0));
	pComp->Value(mkNamingAdapt(ViewScore,           "ViewScore",            0));
	pComp->Value(mkNamingAdapt(fFogOfWar,           "FogOfWar",             false));
	bool bForceFogOfWar = false;
	pComp->Value(mkNamingAdapt(bForceFogOfWar,      "ForceFogOfWar",        false));
	pComp->Value(mkNamingAdapt(ShowStartup,         "ShowStartup",          false));
	pComp->Value(mkNamingAdapt(Wealth,              "Wealth",               0));
	pComp->Value(mkNamingAdapt(CurrentScore,        "Score",                0));
	pComp->Value(mkNamingAdapt(InitialScore,        "InitialScore",         0));
	pComp->Value(mkNamingAdapt(ObjectsOwned,        "ObjectsOwned",         0));
	pComp->Value(mkNamingAdapt(Hostility,           "Hostile"               ));
	pComp->Value(mkNamingAdapt(ProductionDelay,     "ProductionDelay",      0));
	pComp->Value(mkNamingAdapt(ProductionUnit,      "ProductionUnit",       0));
	pComp->Value(mkNamingAdapt(CursorFlash,         "CursorFlash",          0));
	pComp->Value(mkNamingAdapt(Cursor,              "Cursor",               C4ObjectPtr::Null));
	pComp->Value(mkNamingAdapt(ViewCursor,          "ViewCursor",           C4ObjectPtr::Null));
	pComp->Value(mkNamingAdapt(MessageStatus,       "MessageStatus",        0));
	pComp->Value(mkNamingAdapt(toC4CStr(MessageBuf),"MessageBuf",           ""));
	pComp->Value(mkNamingAdapt(HomeBaseMaterial,    "HomeBaseMaterial"      ));
	pComp->Value(mkNamingAdapt(HomeBaseProduction,  "HomeBaseProduction"    ));
	pComp->Value(mkNamingAdapt(Knowledge,           "Knowledge"             ));
	pComp->Value(mkNamingAdapt(Magic,               "Magic"                 ));
	pComp->Value(mkNamingAdapt(Crew,                "Crew"                  ));
	pComp->Value(mkNamingAdapt(CrewInfoList.iNumCreated, "CrewCreated",     0));
	pComp->Value(mkNamingPtrAdapt( pMsgBoardQuery,  "MsgBoardQueries"        ));

	// Keys held down
	pComp->Value(Control);
}

bool C4Player::LoadRuntimeData(C4Group &hGroup)
{
	const char *pSource;
	// Use loaded game text component
	if (!(pSource = Game.GameText.GetData())) return false;
	// safety: Do nothing if playeer section is not even present (could kill initialized values)
	if (!SSearch(pSource, FormatString("[Player%i]", ID).getData())) return false;
	// Compile (Search player section - runtime data is stored by unique player ID)
	// Always compile exact. Exact data will not be present for savegame load, so it does not matter
	assert(ID);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
	      mkNamingAdapt(mkParAdapt(*this, true), FormatString("Player%i", ID).getData()),
	      StdStrBuf(pSource),
	      Game.GameText.GetFilePath()))
		return false;
	// Denumerate pointers
	DenumeratePointers();
	// Success
	return true;
}

void C4Player::ExecHomeBaseProduction()
{
	const int32_t MaxHomeBaseProduction = 25;
	ProductionDelay++;
	if (ProductionDelay>=60) // Minute Production Unit
	{
		ProductionDelay=0; ProductionUnit++;
		for (int32_t cnt=0; HomeBaseProduction.GetID(cnt); cnt++)
			if (HomeBaseProduction.GetCount(cnt)>0)
				if (ProductionUnit % BoundBy<int32_t>(11-HomeBaseProduction.GetCount(cnt),1,10) ==0)
					if (HomeBaseMaterial.GetIDCount(HomeBaseProduction.GetID(cnt))<MaxHomeBaseProduction)
						HomeBaseMaterial.IncreaseIDCount(HomeBaseProduction.GetID(cnt));
	}
}

void C4Player::UpdateCounts()
{
	C4Object *cobj; C4ObjectLink *clnk;
	CrewCnt = 0;
	for (clnk=Crew.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
	{
		++CrewCnt;
	}
}

void C4Player::CheckElimination()
{
	// Standard elimination: no crew
	if (CrewCnt<=0)
		// Already eliminated safety
		if (!Eliminated)
			// No automatic elimination desired?
			if (!NoEliminationCheck)
				// Do elimination!
				Eliminate();
}

void C4Player::UpdateView()
{
	// view target/cursor
	switch (ViewMode)
	{
	case C4PVM_Cursor:
	{
		C4Object *pViewObj;
		if (!(pViewObj=ViewCursor)) pViewObj=Cursor;
		if (pViewObj)
		{
			ViewX=pViewObj->GetX(); ViewY=pViewObj->GetY();
		}
		break;
	}
	case C4PVM_Target:
		if (ViewTarget)
		{
			ViewX=ViewTarget->GetX(); ViewY=ViewTarget->GetY();
		}
		break;
	case C4PVM_Scrolling:
		break;
	}
}

void C4Player::DefaultRuntimeData()
{
	Status=0;
	Eliminated=0;
	Surrendered=0;
	AtClient=C4ClientIDUnknown;
	SCopy("Local",AtClientName);
	Color=-1;
	ControlSet = NULL;
	ControlSetName.Clear();
	MouseControl=false;
	Position=-1;
	PlrStartIndex=0;
	RetireDelay=0;
	ViewMode=C4PVM_Cursor;
	ViewX=ViewY=0;
	ViewTarget=NULL;
	ShowStartup=true;
	CrewCnt=0;
	ViewWealth=ViewScore=0;
	Wealth=0;
	CurrentScore=InitialScore=0;
	ObjectsOwned=0;
	ProductionDelay=ProductionUnit=0;
	Cursor=ViewCursor=NULL;
	CursorFlash=30;
	MessageStatus=0;
	MessageBuf[0]=0;
	Hostility.clear();
	HomeBaseMaterial.Default();
	HomeBaseProduction.Default();
	Knowledge.Default();
	Magic.Default();
	FlashCom=0;
}

bool C4Player::ActivateMenuTeamSelection(bool fFromMain)
{
	// Menu symbol/init
	bool fSwitch = !(Status==PS_TeamSelection);
	Menu.InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Team),LoadResStr("IDS_MSG_SELTEAM"),Number, C4MN_Extra_None, 0, fSwitch ? C4MN_TeamSwitch : C4MN_TeamSelection);
	Menu.SetAlignment(fSwitch ? C4MN_Align_Left | C4MN_Align_Bottom : 0);
	Menu.Refill();
	// Go back to options menu on close
	if (fFromMain) Menu.SetCloseCommand("ActivateMenu:Main");
	return true;
}

void C4Player::DoTeamSelection(int32_t idTeam)
{
	// stop team selection. This might close the menu forever if the control gets lost
	// let's hope it doesn't!
	Status = PS_TeamSelectionPending;
	::Control.DoInput(CID_Script, new C4ControlScript(FormatString("InitScenarioPlayer(%d,%d)", (int)Number, (int)idTeam).getData()), CDT_Queue);
}

void C4Player::EnumeratePointers()
{
	// Cursor
	Cursor.EnumeratePointers();
	// ViewCursor
	ViewCursor.EnumeratePointers();
	// messageboard-queries
	for (C4MessageBoardQuery *pCheck = pMsgBoardQuery; pCheck; pCheck = pCheck->pNext)
		pCheck->CallbackObj.EnumeratePointers();
}

void C4Player::DenumeratePointers()
{
	// Crew
	Crew.DenumerateRead();
	// Cursor
	Cursor.DenumeratePointers();
	// ViewCursor
	ViewCursor.DenumeratePointers();
	// messageboard-queries
	for (C4MessageBoardQuery *pCheck = pMsgBoardQuery; pCheck; pCheck = pCheck->pNext)
		pCheck->CallbackObj.DenumeratePointers();
}

void C4Player::RemoveCrewObjects()
{
	C4Object *pCrew;

	// Remove all crew objects
	while ((pCrew = Crew.GetObject())) pCrew->AssignRemoval(true);
}

void C4Player::NotifyOwnedObjects()
{
	C4Object *cobj; C4ObjectLink *clnk;

	// notify objects in all object lists
	for (C4ObjectList *pList = &::Objects; pList; pList = ((pList == &::Objects) ? &::Objects.InactiveObjects : NULL))
		for (clnk = pList->First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
			if (cobj->Status)
				if (cobj->Owner == Number)
				{
					C4AulFunc *pFn = cobj->Def->Script.GetFuncRecursive(PSF_OnOwnerRemoved);
					// PSF_OnOwnerRemoved has an internal fallback function
					assert(pFn);
					if (pFn) pFn->Exec(cobj);
				}
}

bool C4Player::DoScore(int32_t iChange)
{
	CurrentScore = BoundBy<int32_t>( CurrentScore+iChange, -100000, 100000 );
	ViewScore = C4ViewDelay;
	return true;
}

void C4Player::SetCursor(C4Object *pObj, bool fSelectArrow)
{
	// check disabled
	if (pObj) if (pObj->CrewDisabled) return;
	bool fChanged = pObj != Cursor;
	C4Object *pPrev = Cursor;
	// Set cursor
	Cursor=pObj;
	// unselect previous
	if (pPrev && fChanged) pPrev->UnSelect();
	// Select object
	if (fChanged && Cursor) { Cursor->DoSelect(); }
	// View flash
	if (fSelectArrow) CursorFlash=30;
}

void C4Player::ScrollView(float iX, float iY, float ViewWdt, float ViewHgt)
{
	if (ViewLock) return;
	SetViewMode(C4PVM_Scrolling);
	float ViewportScrollBorder = Application.isEditor ? 0 : C4ViewportScrollBorder;
	ViewX = BoundBy<C4Real>( ViewX+ftofix(iX), ftofix(ViewWdt/2.0f-ViewportScrollBorder), ftofix(GBackWdt+ViewportScrollBorder-ViewWdt/2.0f) );
	ViewY = BoundBy<C4Real>( ViewY+ftofix(iY), ftofix(ViewHgt/2.0f-ViewportScrollBorder), ftofix(GBackHgt+ViewportScrollBorder-ViewHgt/2.0f) );
}

void C4Player::ClearControl()
{
	// Mark any control set as unused
	Control.Clear();
	// Reset control
	LocalControl = false;
	ControlSetName.Clear();
	ControlSet=NULL;
	if (pGamepad) { delete pGamepad; pGamepad=NULL; }
	MouseControl = false;
	// no controls issued yet
	ControlCount = ActionCount = 0;
	LastControlType = PCID_None;
	LastControlID = 0;
}

void C4Player::InitControl()
{
	// clear any previous
	ClearControl();
	// Check local control
	if (AtClient == ::Control.ClientID())
		if (!GetInfo() || GetInfo()->GetType() == C4PT_User)
			if (!::Control.isReplay())
				LocalControl=true;
	// needs to init control for local players only
	if (LocalControl)
	{
		// Preferred control
		ControlSetName = PrefControl;
		ControlSet = Game.PlayerControlAssignmentSets.GetSetByName(ControlSetName.getData());
		// control set unassigned/not known? fallback to some default then (=first defined control set)
		if (!ControlSet) ControlSet = Game.PlayerControlAssignmentSets.GetDefaultSet();
		// gamepad control safety (assuming the default control set is not using gamepad)
		if (ControlSet && ControlSet->HasGamepad() && !Config.General.GamepadEnabled)
		{
			ControlSet = Game.PlayerControlAssignmentSets.GetDefaultSet();
		}
		// Choose next while control taken
		// TODO
		// init gamepad
		if (ControlSet && ControlSet->HasGamepad())
		{
			pGamepad = new C4GamePadOpener(ControlSet->GetGamepadIndex());
		}
		// Mouse
		if (ControlSet && ControlSet->HasMouse() && PrefMouse)
			if (!::Players.MouseControlTaken())
				MouseControl=true;
		// Some controls such as gamepad control need special synced GUI elements
		// Do a script callback for selected control
		if (ControlSet)
		{
			::Control.DoInput(CID_Script, new C4ControlScript(FormatString("%s(%d,\"%s\",%d,%d,%d)", (const char *)PSF_InitializePlayerControl, (int)Number, ControlSet->GetName(), (int)ControlSet->HasKeyboard(), (int)ControlSet->HasMouse(), (int)ControlSet->HasGamepad()).getData()), CDT_Queue);
		}
		else
		{
			::Control.DoInput(CID_Script, new C4ControlScript(FormatString("%s(%d)", (const char *)PSF_InitializePlayerControl, (int)Number).getData()), CDT_Queue);
		}
	}
	// clear old control method and register new
	Control.RegisterKeyset(Number, ControlSet);
}

int igOffX, igOffY;

int VisibilityCheck(int iVis, int sx, int sy, int cx, int cy)
{
	sx -= igOffX; sy -= igOffY; cx -= igOffX; cy -= igOffY;
	int st = Max(1, Max(Abs(sx - cx), Abs(sy - cy)));
	for (int i = 0; i <= st; i++)
	{
		int x = (sx * (st - i) + cx * i) / st, y = (sy * (st - i) + cy * i) / st;
		if (GBackSolid(x, y))
		{
			if ((iVis -= 2) <= 0)
				return 0;
		}
	}
	return iVis;
}

void C4Player::FoW2Map(CClrModAddMap &rMap, int iOffX, int iOffY)
{
	// No fog of war
	if (!fFogOfWar) return;
	igOffX = iOffX; igOffY = iOffY;
	// Add view for all FoW-repellers - keep track of FoW-generators, which should be avaluated finally
	// so they override repellers
	bool fAnyGenerators = false;
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk=FoWViewObjs.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		if (!cobj->Contained || cobj->Contained->Def->ClosedContainer != 1)
		{
			if (cobj->PlrViewRange > 0)
				rMap.ReduceModulation(cobj->GetX() + iOffX, cobj->GetY() + iOffY, cobj->PlrViewRange, VisibilityCheck);
			else
				fAnyGenerators = true;
		}
	// Add view for target view object
	if (ViewMode==C4PVM_Target)
		if (ViewTarget)
			if (!ViewTarget->Contained || ViewTarget->Contained->Def->ClosedContainer != 1)
			{
				int iRange = ViewTarget->PlrViewRange;
				if (!iRange && Cursor) iRange = Cursor->PlrViewRange;
				if (!iRange) iRange = C4FOW_Def_View_RangeX;
				rMap.ReduceModulation(ViewTarget->GetX() + iOffX, ViewTarget->GetY() + iOffY, iRange, VisibilityCheck);
			}
	// apply generators
	// do this check, be cause in 99% of all normal scenarios, there will be no FoW-generators
	if (fAnyGenerators) FoWGenerators2Map(rMap, iOffX, iOffY);
}

void C4Player::FoWGenerators2Map(CClrModAddMap &rMap, int iOffX, int iOffY)
{
	// add fog to any generator pos (view range
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk=FoWViewObjs.First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		if (!cobj->Contained || cobj->Contained->Def->ClosedContainer != 1)
			if (cobj->PlrViewRange < 0)
				rMap.AddModulation(cobj->GetX() + iOffX, cobj->GetY() + iOffY,-cobj->PlrViewRange, ((uint32_t)cobj->ColorMod)>>24);
}

bool C4Player::FoWIsVisible(int32_t x, int32_t y)
{
	// check repellers and generators and ViewTarget
	bool fSeen=false;
	C4Object *cobj=NULL; C4ObjectLink *clnk;
	clnk=FoWViewObjs.First;
	int32_t iRange;
	for (;;)
	{
		if (clnk)
		{
			cobj=clnk->Obj;
			clnk=clnk->Next;
			iRange = cobj->PlrViewRange;
		}
		else if (ViewMode!=C4PVM_Target || !ViewTarget || ViewTarget == cobj)
			break;
		else
		{
			cobj = ViewTarget;
			iRange = cobj->PlrViewRange;
			if (!iRange && Cursor) iRange = Cursor->PlrViewRange;
			if (!iRange) iRange = C4FOW_Def_View_RangeX;
		}
		if (!cobj->Contained || cobj->Contained->Def->ClosedContainer != 1)
			if (Distance(cobj->GetX(), cobj->GetY(), x, y) < Abs(iRange))
			{
				if (iRange < 0)
				{
					if ((cobj->ColorMod & 0xff000000) != 0xff000000) // faded generators generate darkness only; no FoW blocking
						return false; // shadowed by FoW-generator
				}
				else
					fSeen = true; // made visible by FoW-repeller
			}
	}
	return fSeen;
}

void C4Player::CloseMenu()
{
	// cancel all player menus
	Menu.Close(false);
}

void C4Player::Eliminate()
{
	if (Eliminated) return;
	Eliminated=true;
	RetireDelay=C4RetireDelay;
	StartSoundEffect("Eliminated");
	Log(FormatString(LoadResStr("IDS_PRC_PLRELIMINATED"),GetName()).getData());

	// Early client deactivation check
	if (::Control.isCtrlHost() && AtClient > C4ClientIDHost)
	{
		// Check: Any player left at this client?
		C4Player *pPlr = NULL;
		for (int i = 0; (pPlr = ::Players.GetAtClient(AtClient, i)); i++)
			if (!pPlr->Eliminated)
				break;
		// If not, deactivate the client
		if (!pPlr)
			::Control.DoInput(CID_ClientUpdate,
			                  new C4ControlClientUpdate(AtClient, CUT_Activate, false),
			                  CDT_Sync);
	}
}

int32_t C4Player::ActiveCrewCount()
{
	// get number of objects in crew that is not disabled
	int32_t iNum=0;
	C4Object *cObj;
	for (C4ObjectLink *cLnk=Crew.First; cLnk; cLnk=cLnk->Next)
		if ((cObj=cLnk->Obj))
			if (!cObj->CrewDisabled)
				++iNum;
	// return it
	return iNum;
}

int32_t C4Player::GetSelectedCrewCount()
{
	if (Cursor && !Cursor->CrewDisabled)
		return 1;
	return 0;
}

void C4Player::EvaluateLeague(bool fDisconnected, bool fWon)
{
	// already evaluated?
	if (LeagueEvaluated) return; LeagueEvaluated=true;
	// set fate
	C4PlayerInfo *pInfo = GetInfo();
	if (pInfo)
	{
		if (fDisconnected)
			pInfo->SetDisconnected();
		if (fWon)
			pInfo->SetWinner();
	}
}

bool C4Player::LocalSync()
{
	// local sync not necessary for script players
	if (GetType() == C4PT_Script) return true;
	// evaluate total playing time
	TotalPlayingTime+=Game.Time-GameJoinTime;
	GameJoinTime = Game.Time;
	// evaluate total playing time of all the crew
	for (C4ObjectInfo *pInf = CrewInfoList.GetFirst(); pInf; pInf=pInf->Next)
		if (pInf->InAction)
		{
			pInf->TotalPlayingTime+=(Game.Time-pInf->InActionTime);
			pInf->InActionTime = Game.Time;
		}
	// save player
	if (!Save())
		return false;
	// done, success
	return true;
}

C4PlayerInfo *C4Player::GetInfo()
{
	return Game.PlayerInfos.GetPlayerInfoByID(ID);
}

bool C4Player::SetObjectCrewStatus(C4Object *pCrew, bool fNewStatus)
{
	// either add...
	if (fNewStatus)
	{
		// is in crew already?
		if (Crew.IsContained(pCrew)) return true;
		return MakeCrewMember(pCrew, false);
	}
	else
	{
		// already outside?
		if (!Crew.IsContained(pCrew)) return true;
		// ...or remove
		Crew.Remove(pCrew);
		C4AulParSet parset(C4VInt(Number));
		pCrew->Call(PSF_OnRemoveCrew, &parset);
		// remove info, if assigned to this player
		// theoretically, info objects could remain when the player is deleted
		// but then they would be reassigned to the player crew when loaded in a savegame
		//  by the crew-assignment code kept for backwards compatibility with pre-4.95.2-savegames
		if (pCrew->Info && CrewInfoList.IsElement(pCrew->Info))
		{
			pCrew->Info->Retire();
			pCrew->Info = NULL;
		}
	}
	// done, success
	return true;
}

void C4Player::CreateGraphs()
{
	// del prev
	ClearGraphs();
	// create graphs
	if (Game.pNetworkStatistics)
	{
		DWORD dwGraphClr = ColorDw;
		C4PlayerInfo *pInfo;
		if (ID && (pInfo = Game.PlayerInfos.GetPlayerInfoByID(ID)))
		{
			// set color by player info class
			dwGraphClr = pInfo->GetColor();
		}
		C4GUI::MakeColorReadableOnBlack(dwGraphClr); dwGraphClr &= 0xffffff;
		pstatControls = new C4TableGraph(C4TableGraph::DefaultBlockLength * 20, Game.pNetworkStatistics->ControlCounter);
		pstatControls->SetColorDw(dwGraphClr);
		pstatControls->SetTitle(GetName());
		pstatActions = new C4TableGraph(C4TableGraph::DefaultBlockLength * 20, Game.pNetworkStatistics->ControlCounter);
		pstatActions->SetColorDw(dwGraphClr);
		pstatActions->SetTitle(GetName());
		// register into
		Game.pNetworkStatistics->statControls.AddGraph(pstatControls);
		Game.pNetworkStatistics->statActions.AddGraph(pstatActions);
	}
}

void C4Player::ClearGraphs()
{
	// del all assigned graphs
	if (pstatControls)
	{
		if (Game.pNetworkStatistics) Game.pNetworkStatistics->statControls.RemoveGraph(pstatControls);
		delete pstatControls;
		pstatControls = NULL;
	}
	if (pstatActions)
	{
		if (Game.pNetworkStatistics) Game.pNetworkStatistics->statActions.RemoveGraph(pstatActions);
		delete pstatActions;
		pstatActions = NULL;
	}
}

void C4Player::CountControl(ControlType eType, int32_t iID, int32_t iCntAdd)
{
	// count it
	ControlCount += iCntAdd;
	// catch doubles
	if (eType == LastControlType && iID == LastControlID) return;
	// no double: count as action
	LastControlType = eType;
	LastControlID = iID;
	ActionCount += iCntAdd;
	// and give experience
	if (Cursor && Cursor->Info)
	{
		if (Cursor->Info)
		{
			Cursor->Info->ControlCount++; if ((Cursor->Info->ControlCount%5) == 0) Cursor->DoExperience(+1);
		}
	}
}

void C4Player::ExecMsgBoardQueries()
{
	// query now possible?
	if (!C4GUI::IsGUIValid()) return;
	// already active?
	if (::MessageInput.IsTypeIn()) return;
	// find an un-evaluated query
	C4MessageBoardQuery *pCheck = pMsgBoardQuery;
		while (pCheck) if (!pCheck->fAnswered) break; else pCheck = pCheck->pNext;
	if (!pCheck) return;
	// open it
	::MessageInput.StartTypeIn(true, pCheck->CallbackObj, pCheck->fIsUppercase, false, Number, pCheck->sInputQuery);
}

void C4Player::CallMessageBoard(C4Object *pForObj, const StdStrBuf &sQueryString, bool fIsUppercase)
{
	// remove any previous query for the same object
	RemoveMessageBoardQuery(pForObj);
	// sort new query to end of list
	C4MessageBoardQuery **ppTarget = &pMsgBoardQuery;
	while (*ppTarget) ppTarget = &((*ppTarget)->pNext);
	*ppTarget = new C4MessageBoardQuery(pForObj, sQueryString, fIsUppercase);
}

bool C4Player::RemoveMessageBoardQuery(C4Object *pForObj)
{
	// get matching query
	C4MessageBoardQuery **ppCheck = &pMsgBoardQuery, *pFound;
		while (*ppCheck) if ((*ppCheck)->CallbackObj == pForObj) break; else ppCheck = &((*ppCheck)->pNext);
	pFound = *ppCheck;
	if (!pFound) return false;
	// remove it
	*ppCheck = (*ppCheck)->pNext;
	delete pFound;
	return true;
}

bool C4Player::MarkMessageBoardQueryAnswered(C4Object *pForObj)
{
	// get matching query
	C4MessageBoardQuery *pCheck = pMsgBoardQuery;
		while (pCheck) if (pCheck->CallbackObj == pForObj && !pCheck->fAnswered) break; else pCheck = pCheck->pNext;
	if (!pCheck) return false;
	// mark it
	pCheck->fAnswered = true;
	return true;
}

bool C4Player::HasMessageBoardQuery()
{
	// return whether any object has a messageboard-query
	return !!pMsgBoardQuery;
}

void C4Player::OnTeamSelectionFailed()
{
	// looks like a selected team was not available: Go back to team selection if this is not a mislead call
	if (Status == PS_TeamSelectionPending)
		Status = PS_TeamSelection;
}

void C4Player::SetPlayerColor(uint32_t dwNewClr)
{
	// no change?
	if (dwNewClr == ColorDw) return;
	// reflect change in all active, player-owned objects
	// this can never catch everything (thinking of overlays, etc.); scenarios that allow team changes should take care of the rest
	uint32_t dwOldClr = ColorDw;
	ColorDw = dwNewClr;
	C4Object *pObj;
	for (C4ObjectLink *pLnk = ::Objects.First; pLnk; pLnk = pLnk->Next)
		if ((pObj = pLnk->Obj))
			if ((pObj->Status))
				if (pObj->Owner == Number)
				{
					if ((pObj->Color & 0xffffff) == (dwOldClr & 0xffffff))
						pObj->Color = (pObj->Color & 0xff000000u) | (dwNewClr & 0xffffff);
				}
}

C4PlayerType C4Player::GetType() const
{
	// type by info
	C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(ID);
	if (pInfo) return pInfo->GetType(); else { assert(false); return C4PT_User; }
}

bool C4Player::IsInvisible() const
{
	// invisible by info
	C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(ID);
	if (pInfo) return pInfo->IsInvisible(); else { assert(false); return false; }
}

void C4Player::ToggleMouseControl()
{
	// Activate mouse control if it's available
	if (!MouseControl && !::Players.MouseControlTaken())
	{
		::MouseControl.Init(Number);
		MouseControl=true;
	}
	// Deactivate mouse control
	else if (MouseControl)
	{
		::MouseControl.Clear();
		::MouseControl.Default();
		MouseControl = 0;
		// Scrolling isn't possible any more
		if (ViewMode == C4PVM_Scrolling)
			SetViewMode(C4PVM_Cursor);
	}
}

bool C4Player::ActivateMenuMain()
{
	// Not during game over dialog
	if (C4GameOverDlg::IsShown()) return false;
	// Open menu
	return !!Menu.ActivateMain(Number);
}

void C4Player::HostilitySet::CompileFunc(StdCompiler *pComp)
{
	int entries = size();
	if (pComp->isCompiler())
	{
		clear();
		pComp->Value(entries);
		while (entries--)
		{
			int number;
			pComp->Value(number);
			assert(::Players.Valid(number));
			C4Player *plr = ::Players.Get(number);
			if (plr)
				insert(plr);
		}
	}
	else
	{
		pComp->Value(entries);
		for (const_iterator it = begin(); it != end(); ++it)
		{
			int32_t num = (*it)->Number;
			pComp->Value(num); // Can't use (*it)->Number directly because StdCompiler is dumb about constness
		}
	}
}

void C4Player::SetZoomByViewRange(int32_t range_wdt, int32_t range_hgt, bool direct, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomWdt, range_wdt, no_increase, no_decrease);
	AdjustZoomParameter(&ZoomHgt, range_hgt, no_increase, no_decrease);
	ZoomToViewport(direct, no_decrease, no_increase); // inc/dec swapped for zoom, because it's inversely proportional to range
}

void C4Player::SetMinZoomByViewRange(int32_t range_wdt, int32_t range_hgt, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomLimitMinWdt, range_wdt, no_increase, no_decrease);
	AdjustZoomParameter(&ZoomLimitMinHgt, range_hgt, no_increase, no_decrease);
	ZoomLimitsToViewport();
}

void C4Player::SetMaxZoomByViewRange(int32_t range_wdt, int32_t range_hgt, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomLimitMaxWdt, range_wdt, no_increase, no_decrease);
	AdjustZoomParameter(&ZoomLimitMaxHgt, range_hgt, no_increase, no_decrease);
	ZoomLimitsToViewport();
}

void C4Player::ZoomToViewport(bool direct, bool no_increase, bool no_decrease)
{
	C4Viewport *vp = ::GraphicsSystem.GetViewport(Number);
	if (!vp) return;
	float new_zoom = vp->GetZoomByViewRange((ZoomWdt || ZoomHgt) ? ZoomWdt : C4VP_DefViewRangeX,ZoomHgt);
	float old_zoom = vp->GetZoomTarget();
	if (new_zoom > old_zoom && no_increase) return;
	if (new_zoom < old_zoom && no_decrease) return;
	vp->SetZoom(new_zoom, direct);
}

void C4Player::ZoomLimitsToViewport()
{
	C4Viewport *vp = ::GraphicsSystem.GetViewport(Number);
	if (!vp) return;
	float zoom_max = vp->GetZoomByViewRange((ZoomLimitMinWdt || ZoomLimitMinHgt) ? ZoomLimitMinWdt : C4VP_DefMinViewRangeX,ZoomLimitMinHgt);
	float zoom_min = vp->GetZoomByViewRange((ZoomLimitMaxWdt || ZoomLimitMaxHgt) ? ZoomLimitMaxWdt : C4VP_DefMaxViewRangeX,ZoomLimitMaxHgt);
	vp->SetZoomLimits(zoom_min, zoom_max);
}

bool C4Player::AdjustZoomParameter(int32_t *range_par, int32_t new_val, bool no_increase, bool no_decrease)
{
	// helper function: Adjust *range_par to new_val if increase/decrease not forbidden
	if (new_val < *range_par)
	{
		if (!no_decrease) *range_par = new_val;
		return !no_decrease;
	}
	else if(new_val > *range_par)
	{
		if (!no_increase) *range_par = new_val;
		return !no_increase;
	}
	return true;
}

void C4Player::SetViewLocked(bool to_val)
{
	if ((ViewLock = to_val))
	{
		// view was locked - cancel any scrolling
		if (ViewMode == C4PVM_Scrolling) SetViewMode(C4PVM_Cursor);
	}
}
