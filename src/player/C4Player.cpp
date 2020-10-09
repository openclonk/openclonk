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

/* Player data at runtime */

#include "C4Include.h"
#include "player/C4Player.h"

#include "control/C4GameControl.h"
#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameMessage.h"
#include "gui/C4GameOverDlg.h"
#include "gui/C4MessageInput.h"
#include "gui/C4MouseControl.h"
#include "landscape/C4Landscape.h"
#include "lib/C4Random.h"
#include "network/C4League.h"
#include "network/C4Network2Stats.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectMenu.h"
#include "platform/C4GamePadCon.h"
#include "player/C4PlayerList.h"

C4Player::C4Player() : C4PlayerInfoCore()
{
	Filename[0] = 0;
	Number = C4P_Number_None;
	ID = 0;
	Team = 0;
	DefaultRuntimeData();
	Menu.Default();
	Crew.Default();
	CrewInfoList.Default();
	LocalControl = false;
	BigIcon.Default();
	Next = nullptr;
	fFogOfWar = true;
	LeagueEvaluated = false;
	GameJoinTime = 0; // overwritten in Init
	pstatControls = pstatActions = nullptr;
	ControlCount = ActionCount = 0;
	LastControlType = PCID_None;
	LastControlID = 0;
	pMsgBoardQuery = nullptr;
	NoEliminationCheck = false;
	Evaluated = false;
	ZoomLimitMinWdt = ZoomLimitMinHgt = ZoomLimitMaxWdt = ZoomLimitMaxHgt = ZoomWdt = ZoomHgt = 0;
	ZoomLimitMinVal = ZoomLimitMaxVal = ZoomVal = Fix0;
	ViewLock = true;
	SoundModifier.Set0();
}

C4Player::~C4Player()
{
	ClearGraphs();
	Menu.Clear();
	SetSoundModifier(nullptr);
	while (pMsgBoardQuery)
	{
		C4MessageBoardQuery *pNext = pMsgBoardQuery->pNext;
		delete pMsgBoardQuery;
		pMsgBoardQuery = pNext;
	}
	ClearControl();
}

bool C4Player::ObjectInCrew(C4Object *tobj)
{
	if (!tobj) return false;
	for (C4Object *cobj : Crew)
		if (cobj==tobj) return true;
	return false;
}

void C4Player::ClearPointers(C4Object *pObj, bool fDeath)
{
	// Crew
	while (Crew.Remove(pObj)) {}
	// View-Cursor
	if (ViewCursor==pObj) ViewCursor = nullptr;
	// View
	if (ViewTarget==pObj) ViewTarget=nullptr;
	// Cursor
	if (Cursor == pObj)
	{
		// object is to be deleted; do NOT do script calls (like in Cursor->UnSelect(true))
		Cursor = nullptr; AdjustCursorCommand(); // also selects and eventually does a script call!
	}
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
			pTeam = nullptr;
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
		if (!Game.Teams.IsJoin2TeamAllowed(idTeam, pInfo->GetType()))
		{
			pTeam = nullptr;
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
	// perform any pending InitializePlayers() callback
	::Game.OnPlayerJoinFinished();
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
				int32_t idSelectedTeam = atoi(pSelectedTeamItem->GetCommand()+8);
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
								ViewX = Game.C4S.PlrStart[iPlrStartIndex-1].Position[0] * ::Landscape.GetMapZoom();
								ViewY = Game.C4S.PlrStart[iPlrStartIndex-1].Position[1] * ::Landscape.GetMapZoom();
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

	// Do we have a gamepad?
	if (pGamepad)
	{
		// Check whether it's still connected.
		if (!pGamepad->IsAttached())
		{
			// Allow the player to plug the gamepad back in. This allows
			// battery replacement or plugging the controller back
			// in after someone tripped over the wire.
			if (!FindGamepad())
			{
				LogF("%s: No gamepad available.", Name.getData());
				::Game.Pause();
			}
		}
	}
	// Should we have one? The player may have started the game
	// without turning their controller on, only noticing this
	// after the game started.
	else if (LocalControl && ControlSet && ControlSet->HasGamepad())
	{
		FindGamepad();
	}

	// Tick1
	UpdateView();
	ExecuteControl();
	Menu.Execute();

	// ::Game.iTick35
	if (!::Game.iTick35 && Status==PS_Normal)
	{
		ExecBaseProduction();
		CheckElimination();
		if (pMsgBoardQuery && LocalControl) ExecMsgBoardQueries();
	}

	// Delays
	if (MessageStatus>0) MessageStatus--;
	if (RetireDelay>0) RetireDelay--;
	if (CursorFlash>0) CursorFlash--;
}

bool C4Player::Init(int32_t iNumber, int32_t iAtClient, const char *szAtClientName,
                    const char *szFilename, bool fScenarioInit, class C4PlayerInfo *pInfo, C4ValueNumbers * numbers)
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
	Number = iNumber;
	ID = pInfo->GetID();
	Team = pInfo->GetTeam();
	NoEliminationCheck = pInfo->IsNoEliminationCheck();

	// At client
	AtClient=iAtClient; SCopy(szAtClientName,AtClientName,C4MaxTitle);

	if (szFilename)
	{
		// Load core & crew info list
		if (!Load(szFilename, !fScenarioInit)) return false;
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
	ViewX = ::Landscape.GetWidth()/2; ViewY = ::Landscape.GetHeight()/2;

	// Scenario init
	if (fScenarioInit)
	{
		// mark player join in player info list
		// for non-scenarioinit, player should already be marked as joined
		pInfo->SetJoined(iNumber);

		// Number might have changed: Recheck list sorting before scenarioinit, which will do script calls
		::Players.RecheckPlayerSort(this);

		// check for a postponed scenario init, if no team is specified (post-lobby-join in network, or simply non-network)
		C4Team *pTeam = nullptr;
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
		ClearControl();
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
				pDefCallback->Call(PSF_InitializeScriptPlayer, &C4AulParSet(Number, Team));
			}
		}
		else
		{
			// player preinit: In case a team needs to be chosen first, no InitializePlayer-broadcast is done
			// this callback shall give scripters a chance to do stuff like starting an intro or enabling FoW, which might need to be done
			::Game.GRBroadcast(PSF_PreInitializePlayer, &C4AulParSet(Number));
			// direct init
			if (Status != PS_TeamSelection) if (!ScenarioInit()) return false;
		}
	}

	// Load runtime data
	else
	{
		assert(pInfo->IsJoined());
		assert(numbers);
		// (compile using DefaultRuntimeData) - also check if compilation returned sane results, i.e. ID assigned
		if (!LoadRuntimeData(Game.ScenarioFile, numbers) || !ID)
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
	for (C4Object *pObj : Objects)
	{
		if ((pObj->lightRange || pObj->lightFadeoutRange) && pObj->Owner == NO_OWNER)
			pObj->UpdateLight();
	}

	// init graphs
	if (Game.pNetworkStatistics) CreateGraphs();

	// init sound mod
	SetSoundModifier(SoundModifier.getPropList());

	return true;
}

bool C4Player::Save()
{
	C4Group hGroup;
	// Regular player saving need not be done for script players
	if (GetType() == C4PT_Script) return false;
	// Log
	LogF(LoadResStr("IDS_PRC_SAVEPLR"), Config.AtRelativePath(Filename));
	::GraphicsSystem.MessageBoard->EnsureLastMessage();
	// copy player to save somewhere else
	char szPath[_MAX_PATH_LEN];
	SCopy(Config.AtTempPath(C4CFN_TempPlayer), szPath, _MAX_PATH);
	MakeTempFilename(szPath);
	// For local players, we save over the old player file, as there might
	// be all kinds of non-essential stuff in it. For non-local players, we
	// just re-create it every time (it's temporary anyway).
	if (LocalControl)
	{
		// But make sure to copy it first so full hard (flgr stupid) disks
		// won't corrupt any player files...
		C4Group_CopyItem(Filename, szPath);
	}
	else
	{
		// For non-local players, we can actually use the loaded definition
		// list to strip out all non-existant definitions. This is only valid
		// because we know the file to be temporary.
		CrewInfoList.Strip(::Definitions);
	}
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
	                          pDRes = nullptr;
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
	int32_t cnt,ctx,cty;
	C4Object *nobj;
	C4ObjectInfo *pInfo;
	C4Def *pDef;

	// Place crew
	int32_t iCount;
	C4ID id;
	for (cnt=0; (id=Game.C4S.PlrStart[PlrStartIndex].ReadyCrew.GetID(cnt,&iCount)); cnt++)
	{
		// Minimum one clonk if empty id
		iCount = std::max<int32_t>(iCount,1);

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
				Crew.Add(nobj, C4ObjectList::stNone);
				// add visibility range
				nobj->SetLightRange(C4FOW_DefLightRangeX, C4FOW_DefLightFadeoutRangeX);
				// If base is present, enter base
				if (FirstBase) { nobj->Enter(FirstBase); nobj->SetCommand(C4CMD_Exit); }
				// OnJoinCrew callback
				{
					C4DebugRecOff DbgRecOff{ !DEBUGREC_RECRUITMENT };
					C4AulParSet parset(Number);
					nobj->Call(PSF_OnJoinCrew, &parset);
				}
			}
		}
	}

}

void C4Player::PlaceReadyBase(int32_t &tx, int32_t &ty, C4Object **pFirstBase)
{
	int32_t cnt,cnt2,ctx,cty;
	C4Def *def;
	C4ID cid;
	C4Object *cbase;
	// Create ready base structures
	for (cnt=0; (cid=Game.C4S.PlrStart[PlrStartIndex].ReadyBase.GetID(cnt)); cnt++)
	{
		if ((def=C4Id2Def(cid)))
			for (cnt2=0; cnt2<Game.C4S.PlrStart[PlrStartIndex].ReadyBase.GetCount(cnt); cnt2++)
			{
				ctx=tx; cty=ty;
				if (Game.C4S.PlrStart[PlrStartIndex].EnforcePosition
				    || FindConSiteSpot(ctx,cty,def->Shape.Wdt,def->Shape.Hgt,20))
					if ((cbase=Game.CreateObjectConstruction(C4Id2Def(cid),nullptr,Number,ctx,cty,FullCon,true)))
					{
						// FirstBase
						if (!(*pFirstBase)) if ((cbase->Def->Entrance.Wdt>0) && (cbase->Def->Entrance.Hgt>0))
								{ *pFirstBase=cbase; tx=(*pFirstBase)->GetX(); ty=(*pFirstBase)->GetY(); }
					}
			}
	}
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
				if ((cobj=Game.CreateObject(cid,nullptr,Number,ctx,cty)))
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
					Game.CreateObject(cid,nullptr,Number,ctx,cty);
				}
		}
	}
}

bool C4Player::ScenarioInit()
{
	int32_t ptx,pty;

	// player start index by team, if specified. Otherwise by player number
	PlrStartIndex = Number % C4S_MaxPlayer;
	C4Team *pTeam; int32_t i;
	if (Team && (pTeam = Game.Teams.GetTeamByID(Team))) if ((i=pTeam->GetPlrStartIndex())) PlrStartIndex=i-1;

	C4PlayerInfo *pInfo = GetInfo();
	if (!pInfo) { assert(false); LogF("Internal error: ScenarioInit for ghost player %s!", GetName()); return false; }

	// set color by player info class
	// re-setting, because runtime team choice may have altered color
	ColorDw = pInfo->GetColor();

	// any team selection is over now
	Status = PS_Normal;

	// Wealth, home base materials, abilities
	Wealth=Game.C4S.PlrStart[PlrStartIndex].Wealth.Evaluate();
	BaseMaterial=Game.C4S.PlrStart[PlrStartIndex].BaseMaterial;
	BaseMaterial.ConsolidateValids(::Definitions);
	BaseProduction=Game.C4S.PlrStart[PlrStartIndex].BaseProduction;
	BaseProduction.ConsolidateValids(::Definitions);
	Knowledge=Game.C4S.PlrStart[PlrStartIndex].BuildKnowledge;
	Knowledge.ConsolidateValids(::Definitions);

	// Starting position
	ptx = Game.C4S.PlrStart[PlrStartIndex].Position[0];
	pty = Game.C4S.PlrStart[PlrStartIndex].Position[1];

	// Zoomed position
	if (ptx>-1) ptx = Clamp<int32_t>( ptx * Game.C4S.Landscape.MapZoom.Evaluate(), 0, ::Landscape.GetWidth()-1 );
	if (pty>-1) pty = Clamp<int32_t>( pty * Game.C4S.Landscape.MapZoom.Evaluate(), 0, ::Landscape.GetHeight()-1 );

	// Standard position (PrefPosition)
	if (ptx<0)
		if (Game.StartupPlayerCount>=2)
		{
			int32_t iMaxPos=Game.StartupPlayerCount;
			// Try to initialize PrefPosition using teams. This should put players of a team next to each other.
			int PrefPosition = 0;
			C4PlayerInfo *plr;
			for (int i = 0; (plr = Game.PlayerInfos.GetPlayerInfoByIndex(i)) != nullptr; i++)
			{
				if (plr->GetTeam() < Team)
					PrefPosition++;
			}
			// Map preferred position to available positions
			int32_t iStartPos=Clamp(PrefPosition*iMaxPos/C4P_MaxPosition,0,iMaxPos-1);
			int32_t iPosition=iStartPos;
			// Distribute according to availability
			while (::Players.PositionTaken(iPosition))
			{
				++iPosition;
				iPosition %= iMaxPos;
				if (iPosition == iStartPos)
					break;
			}
			Position=iPosition;
			// Set x position
			ptx=Clamp(16+Position*(::Landscape.GetWidth()-32)/(iMaxPos-1),0,::Landscape.GetWidth()-16);
		}

	// All-random position
	if (ptx<0) ptx=16+Random(::Landscape.GetWidth()-32);
	if (pty<0) pty=16+Random(::Landscape.GetHeight()-32);

	// Place to solid ground
	if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
	{
		// Use nearest above-ground...
		FindSolidGround(ptx,pty,30);
		// Might have hit a small lake, or similar: Seach a real site spot from here
		FindConSiteSpot(ptx, pty, 30, 50, 400);
	}

	// Place Readies
	C4Object *FirstBase = nullptr;
	PlaceReadyBase(ptx,pty,&FirstBase);
	PlaceReadyMaterial(ptx-10,ptx+10,pty,FirstBase);
	PlaceReadyVehic(ptx-30,ptx+30,pty,FirstBase);
	PlaceReadyCrew(ptx-30,ptx+30,pty,FirstBase);

	// set initial hostility by team info
	if (Team) SetTeamHostility();

	// Scenario script initialization
	::Game.GRBroadcast(PSF_InitializePlayer, &C4AulParSet(Number,
	                        ptx,
	                        pty,
	                        FirstBase,
	                        Team,
	                        C4Id2Def(GetInfo()->GetScriptPlayerExtraID())));
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

	return true;
}

void C4Player::SetFoW(bool fEnable)
{
	// set flag
	fFogOfWar = fEnable;
}

bool C4Player::DoWealth(int32_t iChange)
{
	if (LocalControl)
	{
		if (iChange>0) StartSoundEffect("UI::Cash");
		if (iChange<0) StartSoundEffect("UI::UnCash");
	}
	SetWealth(Wealth+iChange);

	return true;
}

bool C4Player::SetWealth(int32_t iVal)
{
	if (iVal == Wealth) return true;

	Wealth=Clamp<int32_t>(iVal,0,1000000000);

	::Game.GRBroadcast(PSF_OnWealthChanged,&C4AulParSet(Number));

	return true;
}

bool C4Player::SetKnowledge(C4ID id, bool fRemove)
{
	if (fRemove)
	{
		long iIndex = Knowledge.GetIndex(id);
		if (iIndex<0) return false;
		return Knowledge.DeleteItem(iIndex);
	}
	else
	{
		if (!C4Id2Def(id)) return false;
		return Knowledge.SetIDCount(id, 1, true);
	}
}

void C4Player::SetViewMode(int32_t iMode, C4Object *pTarget, bool immediate_position)
{
	// safe back
	ViewMode=iMode; ViewTarget=pTarget;
	// immediate position set desired?
	if (immediate_position)
	{
		UpdateView();
		C4Viewport *vp = ::Viewports.GetViewport(this->Number);
		if (vp) vp->AdjustPosition(true);
	}
}

void C4Player::ResetCursorView(bool immediate_position)
{
	// reset view to cursor if any cursor exists
	if (!ViewCursor && !Cursor) return;
	SetViewMode(C4PVM_Cursor, nullptr, immediate_position);
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
	if (Game.C4S.Game.IsMelee()) LastRound.Score = std::max<int32_t>(CurrentScore-InitialScore,0);
	// Cooperative: shared score
	else LastRound.Score = std::max(::Players.AverageScoreGain(),0);
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
	StartSoundEffect("UI::Eliminated");
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
	StartSoundEffect("UI::Trumpet");
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
	C4Object *hiexp=nullptr;
	int32_t iHighestExp=-2, iExp;
	for (C4Object *cobj : Crew)
	{
		if (!cobj->CrewDisabled)
		{
			if (cobj->Info) iExp = cobj->Info->Experience; else iExp=-1;
			if (!hiexp || (iExp>iHighestExp))
			{
				hiexp=cobj;
				iHighestExp=iExp;
			}
		}
	}
	return hiexp;
}

C4Object* C4Player::GetHiRankActiveCrew()
{
	C4Object *hirank=nullptr;
	int32_t iHighestRank=-2, iRank;
	for (C4Object *cobj : Crew)
	{
		if (!cobj->CrewDisabled)
		{
			if (cobj->Info) iRank = cobj->Info->Rank; else iRank=-1;
			if (!hirank || (iRank>iHighestRank))
			{
				hirank=cobj;
				iHighestRank=iRank;
			}
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

bool C4Player::Load(const char *szFilename, bool fSavegame)
{
	C4Group hGroup;
	// Open group
	if (!Reloc.Open(hGroup, szFilename)) return false;
	// Remember filename
	SCopy(hGroup.GetFullName().getData(), Filename, _MAX_PATH);
	// Load core
	if (!C4PlayerInfoCore::Load(hGroup))
		{ hGroup.Close(); return false; }
	// Load BigIcon
	if (hGroup.FindEntry(C4CFN_BigIcon)) BigIcon.Load(hGroup, C4CFN_BigIcon, C4FCT_Full, C4FCT_Full, false, 0);
	// Load crew info list
	CrewInfoList.Load(hGroup);
	// Close group
	hGroup.Close();
	// Success
	return true;
}

bool C4Player::Strip(const char *szFilename, bool fAggressive)
{
	// Open group
	C4Group Grp;
	if (!Grp.Open(szFilename))
		return false;
	// Which type of stripping?
	if (!fAggressive)
	{
		// remove bigicon, if the file size is too large
		size_t iBigIconSize=0;
		if (Grp.FindEntry(C4CFN_BigIcon, nullptr, &iBigIconSize))
			if (iBigIconSize > C4NetResMaxBigicon*1024)
				Grp.Delete(C4CFN_BigIcon);
		Grp.Close();
	}
	else
	{
		// Load info core and crew info list
		C4PlayerInfoCore PlrInfoCore;
		C4ObjectInfoList CrewInfoList;
		if (!PlrInfoCore.Load(Grp) || !CrewInfoList.Load(Grp))
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
	C4ObjectInfo *cInf = nullptr;
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
		Crew.Add(pObj, C4ObjectList::stNone);

	// add light
	if (!pObj->lightRange)
		pObj->SetLightRange(C4FOW_DefLightRangeX, C4FOW_DefLightFadeoutRangeX);
	else
		pObj->UpdateLight();

	// controlled by the player
	pObj->Controller = Number;

	// OnJoinCrew callback
	if (fDoCalls)
	{
		C4AulParSet parset(Number);
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

void C4Player::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
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
	pComp->Value(mkNamingAdapt(ColorDw,             "ColorDw",              0u));
	pComp->Value(mkNamingAdapt(Position,            "Position",             0));
	pComp->Value(mkNamingAdapt(ViewMode,            "ViewMode",             C4PVM_Cursor));
	pComp->Value(mkNamingAdapt(ViewX,               "ViewX",                0));
	pComp->Value(mkNamingAdapt(ViewY,               "ViewY",                0));
	pComp->Value(mkNamingAdapt(ViewLock,            "ViewLock",             true));
	pComp->Value(mkNamingAdapt(ZoomLimitMinWdt,     "ZoomLimitMinWdt",      0));
	pComp->Value(mkNamingAdapt(ZoomLimitMinHgt,     "ZoomLimitMinHgt",      0));
	pComp->Value(mkNamingAdapt(ZoomLimitMaxWdt,     "ZoomLimitMaxWdt",      0));
	pComp->Value(mkNamingAdapt(ZoomLimitMaxHgt,     "ZoomLimitMaxHgt",      0));
	pComp->Value(mkNamingAdapt(ZoomWdt,             "ZoomWdt",              0));
	pComp->Value(mkNamingAdapt(ZoomHgt,             "ZoomHgt",              0));
	pComp->Value(mkNamingAdapt(ZoomLimitMinVal,     "ZoomLimitMinVal",      Fix0));
	pComp->Value(mkNamingAdapt(ZoomLimitMaxVal,     "ZoomLimitMaxVal",      Fix0));
	pComp->Value(mkNamingAdapt(ZoomVal,             "ZoomVal",              Fix0));
	pComp->Value(mkNamingAdapt(fFogOfWar,           "FogOfWar",             false));
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
	pComp->Value(mkNamingAdapt(BaseMaterial,        "BaseMaterial"          ));
	pComp->Value(mkNamingAdapt(BaseProduction,      "BaseProduction"        ));
	pComp->Value(mkNamingAdapt(Knowledge,           "Knowledge"             ));
	pComp->Value(mkNamingAdapt(mkParAdapt(Crew, numbers), "Crew"            ));
	pComp->Value(mkNamingAdapt(CrewInfoList.iNumCreated, "CrewCreated",     0));
	pComp->Value(mkNamingPtrAdapt( pMsgBoardQuery,  "MsgBoardQueries"        ));
	pComp->Value(mkNamingAdapt(mkParAdapt(SoundModifier, numbers), "SoundModifier", C4Value()));
	
	if (pComp->isDeserializer())
	{
		SoundModifier.Denumerate(numbers);
	}

	// Keys held down
	pComp->Value(Control);
}

bool C4Player::LoadRuntimeData(C4Group &hGroup, C4ValueNumbers * numbers)
{
	const char *pSource;
	// Use loaded game text component
	if (!(pSource = Game.GameText.GetData())) return false;
	// safety: Do nothing if player section is not even present (could kill initialized values)
	if (!SSearch(pSource, FormatString("[Player%i]", ID).getData())) return false;
	// Compile (Search player section - runtime data is stored by unique player ID)
	// Always compile exact. Exact data will not be present for savegame load, so it does not matter
	assert(ID);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
	      mkNamingAdapt(mkParAdapt(*this, numbers), FormatString("Player%i", ID).getData()),
	      StdStrBuf(pSource),
	      Game.GameText.GetFilePath()))
		return false;
	// Denumerate pointers
	DenumeratePointers();
	// Success
	return true;
}

void C4Player::ExecBaseProduction()
{
	const int32_t MaxBaseProduction = 25;
	ProductionDelay++;
	if (ProductionDelay>=60) // Minute Production Unit
	{
		ProductionDelay=0; ProductionUnit++;
		for (int32_t cnt=0; BaseProduction.GetID(cnt); cnt++)
			if (BaseProduction.GetCount(cnt)>0)
				if (ProductionUnit % Clamp<int32_t>(11-BaseProduction.GetCount(cnt),1,10) ==0)
					if (BaseMaterial.GetIDCount(BaseProduction.GetID(cnt)) < MaxBaseProduction)
						BaseMaterial.IncreaseIDCount(BaseProduction.GetID(cnt));
	}
}

void C4Player::CheckElimination()
{
	// Standard elimination: no crew
	if (!Crew.GetFirstObject())
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
	ControlSet = nullptr;
	ControlSetName.Clear();
	MouseControl=false;
	Position=-1;
	PlrStartIndex=0;
	RetireDelay=0;
	ViewMode=C4PVM_Cursor;
	ViewX=ViewY=0;
	ViewTarget=nullptr;
	ShowStartup=true;
	Wealth=0;
	CurrentScore=InitialScore=0;
	ObjectsOwned=0;
	ProductionDelay=ProductionUnit=0;
	Cursor=ViewCursor=nullptr;
	CursorFlash=30;
	MessageStatus=0;
	MessageBuf[0]=0;
	Hostility.clear();
	BaseMaterial.Default();
	BaseProduction.Default();
	Knowledge.Default();
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
	::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::InitScenarioPlayer(this, idTeam), CDT_Queue);
}

void C4Player::DenumeratePointers()
{
	// Crew
	Crew.DenumeratePointers();
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

int32_t C4Player::FindNewOwner() const
{
	int32_t iNewOwner = NO_OWNER;
	C4Team *pTeam;
	if (Team) if ((pTeam = Game.Teams.GetTeamByID(Team)))
	{
		for (int32_t i=0; i<pTeam->GetPlayerCount(); ++i)
		{
			int32_t iPlrID = pTeam->GetIndexedPlayer(i);
			if (iPlrID && iPlrID != ID)
			{
				C4PlayerInfo *pPlrInfo = Game.PlayerInfos.GetPlayerInfoByID(iPlrID);
				if (pPlrInfo) if (pPlrInfo->IsJoined())
				{
					// this looks like a good new owner
					iNewOwner = pPlrInfo->GetInGameNumber();
					break;
				}
			}
		}
	}
	// if noone from the same team was found, try to find another non-hostile player
	// (necessary for cooperative rounds without teams)
	if (iNewOwner == NO_OWNER)
		for (C4Player *pOtherPlr = ::Players.First; pOtherPlr; pOtherPlr = pOtherPlr->Next)
			if (pOtherPlr != this) if (!pOtherPlr->Eliminated)
					if (!::Players.Hostile(pOtherPlr->Number, Number))
						iNewOwner = pOtherPlr->Number;

	return iNewOwner;
}

void C4Player::NotifyOwnedObjects()
{
	int32_t iNewOwner = FindNewOwner();
	// notify objects in all object lists
	for (C4ObjectList *pList = &::Objects; pList; pList = ((pList == &::Objects) ? &::Objects.InactiveObjects : nullptr))
	{
		for (C4Object *cobj : *pList)
		{
			if (cobj->Status && cobj->Owner == Number)
			{
				C4AulFunc *pFn = cobj->GetFunc(PSF_OnOwnerRemoved);
				if (pFn)
				{
					C4AulParSet pars(iNewOwner);
					pFn->Exec(cobj, &pars);
				}
				else
				{
					// crew members: Those are removed later (AFTER the player has been removed, for backwards compatiblity with relaunch scripting)
					if (Crew.IsContained(cobj))
						continue;
					// Regular objects: Try to find a new, suitable owner from the same team
					// Ignore StaticBack, because this would not be compatible with many internal objects such as team account
					if ((cobj->Category & C4D_StaticBack) == 0)
						cobj->SetOwner(iNewOwner);
				}
			}
		}
	}
}

bool C4Player::DoScore(int32_t iChange)
{
	CurrentScore = Clamp<int32_t>( CurrentScore+iChange, -100000, 100000 );
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
	ViewX = Clamp<C4Real>( ViewX+ftofix(iX), ftofix(ViewWdt/2.0f-ViewportScrollBorder), ftofix(::Landscape.GetWidth()+ViewportScrollBorder-ViewWdt/2.0f) );
	ViewY = Clamp<C4Real>( ViewY+ftofix(iY), ftofix(ViewHgt/2.0f-ViewportScrollBorder), ftofix(::Landscape.GetHeight()+ViewportScrollBorder-ViewHgt/2.0f) );
}

void C4Player::ClearControl()
{
	// Mark any control set as unused
	Control.Clear();
	// Reset control
	LocalControl = false;
	ControlSetName.Clear();
	ControlSet=nullptr;
	MouseControl = false;
	if (pGamepad)
	{
		pGamepad->SetPlayer(NO_OWNER);
		pGamepad.reset();
	}
	// no controls issued yet
	ControlCount = ActionCount = 0;
	LastControlType = PCID_None;
	LastControlID = 0;
}

void C4Player::InitControl()
{
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
		ControlSet = Game.PlayerControlUserAssignmentSets.GetSetByName(ControlSetName.getData());
		// control set unassigned/not known? fallback to some default then (=first defined control set)
		if (!ControlSet) ControlSet = Game.PlayerControlUserAssignmentSets.GetDefaultSet();
		// gamepad control safety (assuming the default control set is not using gamepad)
		if (ControlSet && ControlSet->HasGamepad() && !Config.General.GamepadEnabled)
		{
			ControlSet = Game.PlayerControlUserAssignmentSets.GetDefaultSet();
		}
		// Choose next while control taken
		// TODO
		// init gamepad
		if (ControlSet && ControlSet->HasGamepad())
		{
			if (!FindGamepad())
			{
				LogF("No gamepad available for %s, please plug one in!", Name.getData());
				::Game.Pause();
			}
		}
		// Mouse
		if (ControlSet && ControlSet->HasMouse() && PrefMouse)
			if (!::Players.MouseControlTaken())
				MouseControl=true;
		// Some controls such as gamepad control need special synced GUI elements
		// Do a script callback for selected control
		::Control.DoInput(CID_PlrAction, C4ControlPlayerAction::InitPlayerControl(this, ControlSet), CDT_Queue);
	}
	// clear old control method and register new
	Control.RegisterKeyset(Number, ControlSet);
}

bool C4Player::FindGamepad()
{
	auto newPad = Application.pGamePadControl->GetAvailableGamePad();
	if (!newPad) return false;
	newPad->SetPlayer(ID);
	// Release the old gamepad.
	if (pGamepad) pGamepad->SetPlayer(NO_OWNER);
	pGamepad = newPad;
	LogF("%s: Using gamepad #%d.", Name.getData(), pGamepad->GetID());
	return true;
}

int igOffX, igOffY;

int VisibilityCheck(int iVis, int sx, int sy, int cx, int cy)
{
	sx -= igOffX; sy -= igOffY; cx -= igOffX; cy -= igOffY;
	int st = std::max(1, std::max(Abs(sx - cx), Abs(sy - cy)));
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
	StartSoundEffect("UI::Eliminated");
	Log(FormatString(LoadResStr("IDS_PRC_PLRELIMINATED"),GetName()).getData());

	// Early client deactivation check
	if (::Control.isCtrlHost() && AtClient > C4ClientIDHost && !::Application.isEditor)
	{
		// Check: Any player left at this client?
		C4Player *pPlr = nullptr;
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
	for (C4Object *cObj : Crew)
		if (cObj)
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
	if (LeagueEvaluated) return;
	LeagueEvaluated=true;
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
		C4AulParSet parset(Number);
		pCrew->Call(PSF_OnRemoveCrew, &parset);
		// remove info, if assigned to this player
		// theoretically, info objects could remain when the player is deleted
		// but then they would be reassigned to the player crew when loaded in a savegame
		//  by the crew-assignment code kept for backwards compatibility with pre-4.95.2-savegames
		if (pCrew->Info && CrewInfoList.IsElement(pCrew->Info))
		{
			pCrew->Info->Retire();
			pCrew->Info = nullptr;
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
		pstatControls = nullptr;
	}
	if (pstatActions)
	{
		if (Game.pNetworkStatistics) Game.pNetworkStatistics->statActions.RemoveGraph(pstatActions);
		delete pstatActions;
		pstatActions = nullptr;
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
	for (C4Object *pObj : Objects)
		if (pObj && pObj->Status && pObj->Owner == Number)
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
	if (pComp->isDeserializer())
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
		for (auto it : *this)
		{
			int32_t num = it->Number;
			pComp->Value(num); // Can't use (*it)->Number directly because StdCompiler is dumb about constness
		}
	}
}

void C4Player::SetZoomByViewRange(int32_t range_wdt, int32_t range_hgt, bool direct, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomWdt, range_wdt, no_increase, no_decrease);
	AdjustZoomParameter(&ZoomHgt, range_hgt, no_increase, no_decrease);
	ZoomToViewports(direct, no_decrease, no_increase); // inc/dec swapped for zoom, because it's inversely proportional to range
}

void C4Player::SetMinZoomByViewRange(int32_t range_wdt, int32_t range_hgt, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomLimitMinWdt, range_wdt, no_increase, no_decrease);
	AdjustZoomParameter(&ZoomLimitMinHgt, range_hgt, no_increase, no_decrease);
	ZoomLimitsToViewports();
}

void C4Player::SetMaxZoomByViewRange(int32_t range_wdt, int32_t range_hgt, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomLimitMaxWdt, range_wdt, no_increase, no_decrease);
	AdjustZoomParameter(&ZoomLimitMaxHgt, range_hgt, no_increase, no_decrease);
	ZoomLimitsToViewports();
}

void C4Player::SetZoom(C4Real zoom, bool direct, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomVal, zoom, no_increase, no_decrease);
	ZoomToViewports(direct, no_increase, no_decrease);
}

void C4Player::SetMinZoom(C4Real zoom, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomLimitMinVal, zoom, no_increase, no_decrease);
	ZoomLimitsToViewports();
}

void C4Player::SetMaxZoom(C4Real zoom, bool no_increase, bool no_decrease)
{
	AdjustZoomParameter(&ZoomLimitMaxVal, zoom, no_increase, no_decrease);
	ZoomLimitsToViewports();
}

void C4Player::ZoomToViewports(bool direct, bool no_increase, bool no_decrease)
{
	C4Viewport *vp = nullptr;
	while((vp = ::Viewports.GetViewport(Number, vp)) != nullptr)
		ZoomToViewport(vp, direct, no_increase, no_decrease);
}

void C4Player::ZoomToViewport(C4Viewport* vp, bool direct, bool no_increase, bool no_decrease)
{
	float new_zoom = ZoomVal ? fixtof(ZoomVal) : vp->GetZoomByViewRange((ZoomWdt || ZoomHgt) ? ZoomWdt : C4VP_DefViewRangeX,ZoomHgt);
	float old_zoom = vp->GetZoomTarget();
	if (new_zoom > old_zoom && no_increase) return;
	if (new_zoom < old_zoom && no_decrease) return;
	vp->SetZoom(new_zoom, direct);
}

void C4Player::ZoomLimitsToViewports()
{
	C4Viewport *vp = nullptr;
	while((vp = ::Viewports.GetViewport(Number, vp)) != nullptr)
		ZoomLimitsToViewport(vp);
}

void C4Player::ZoomLimitsToViewport(C4Viewport* vp)
{
	float zoom_max = ZoomLimitMaxVal ? fixtof(ZoomLimitMaxVal) : vp->GetZoomByViewRange((ZoomLimitMinWdt || ZoomLimitMinHgt) ? ZoomLimitMinWdt : C4VP_DefMinViewRangeX,ZoomLimitMinHgt);
	float zoom_min = ZoomLimitMinVal ? fixtof(ZoomLimitMinVal) : vp->GetZoomByViewRange((ZoomLimitMaxWdt || ZoomLimitMaxHgt) ? ZoomLimitMaxWdt : C4VP_DefMaxViewRangeX,ZoomLimitMaxHgt);
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

bool C4Player::AdjustZoomParameter(C4Real *zoom_par, C4Real new_val, bool no_increase, bool no_decrease)
{
	// helper function: Adjust *zoom_par to new_val if increase/decrease not forbidden
	if (new_val < *zoom_par)
	{
		if (!no_decrease) *zoom_par = new_val;
		return !no_decrease;
	}
	else if(new_val > *zoom_par)
	{
		if (!no_increase) *zoom_par = new_val;
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

bool C4Player::GainScenarioAchievement(const char *achievement_id, int32_t value, const char *scen_name_override)
{
	// Determine full ID of achievement
	if (!scen_name_override)
	{
		if (::Game.C4S.Head.Origin.getLength())
			scen_name_override = ::Game.C4S.Head.Origin.getData();
		else
			scen_name_override = ::Game.ScenarioFilename;
	}
	StdStrBuf sAchvID = C4ScenarioParameters::AddFilename2ID(scen_name_override, achievement_id);
	// Gain achievement iff it's an improvement
	Achievements.SetValue(sAchvID.getData(), value, true);
	return true;
}

void C4Player::SetSoundModifier(C4PropList *new_modifier)
{
	// set modifier to be applied to all new sounds being played in a player's viewport
	// update prop list parameter
	C4SoundModifier *mod;
	if (new_modifier)
	{
		SoundModifier.SetPropList(new_modifier);
		mod = ::Application.SoundSystem.Modifiers.Get(new_modifier, true);
	}
	else
	{
		SoundModifier.Set0();
		mod = nullptr;
	}
	// update in sound system
	::Application.SoundSystem.Modifiers.SetGlobalModifier(mod, Number);
}
