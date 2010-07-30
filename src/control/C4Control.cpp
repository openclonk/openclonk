/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001-2002, 2004-2008  Sven Eberhardt
 * Copyright (c) 2004-2008  Peter Wortmann
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2006  Florian Groß
 * Copyright (c) 2006  Günther Brammer
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

/* Control packets contain all player input in the message queue */

#include <C4Include.h>
#include <C4Control.h>

#include <C4Object.h>
#include <C4GameSave.h>
#include <C4GameLobby.h>
#include <C4Random.h>
#include <C4Console.h>
#include <C4Log.h>
#include <C4GraphicsSystem.h>
#include <C4Player.h>
#include <C4RankSystem.h>
#include <C4PXS.h>
#include <C4MassMover.h>
#include <C4GameMessage.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>

#ifndef NOAULDEBUG
#include <C4AulDebug.h>
#endif

#include <C4AulExec.h>

// *** C4ControlPacket
C4ControlPacket::C4ControlPacket()
		: iByClient(::Control.ClientID())
{

}

C4ControlPacket::~C4ControlPacket()
{

}

bool C4ControlPacket::LocalControl() const
{
	return iByClient == ::Control.ClientID();
}

void C4ControlPacket::SetByClient(int32_t inByClient)
{
	iByClient = inByClient;
}

void C4ControlPacket::CompileFunc(StdCompiler *pComp)
{
	// Section must be set by caller
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iByClient), "ByClient", -1));
}

// *** C4Control

C4Control::C4Control()
{

}

C4Control::~C4Control()
{
	Clear();
}

void C4Control::Clear()
{
	Pkts.Clear();
}

bool C4Control::PreExecute() const
{
	bool fReady = true;
	for (C4IDPacket *pPkt = firstPkt(); pPkt; pPkt = nextPkt(pPkt))
	{
		// recheck packet type: Must be control
		if (pPkt->getPktType() & CID_First)
		{
			C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(pPkt->getPkt());
			if (pCtrlPkt)
				fReady &= pCtrlPkt->PreExecute();
		}
		else
		{
			LogF("C4Control::PreExecute: WARNING: Ignoring packet type %2x (not control.)", pPkt->getPktType());
		}
	}
	return fReady;
}

void C4Control::Execute() const
{
	for (C4IDPacket *pPkt = firstPkt(); pPkt; pPkt = nextPkt(pPkt))
	{
		// recheck packet type: Must be control
		if (pPkt->getPktType() & CID_First)
		{
			C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(pPkt->getPkt());
			if (pCtrlPkt)
				pCtrlPkt->Execute();
		}
		else
		{
			LogF("C4Control::Execute: WARNING: Ignoring packet type %2x (not control.)", pPkt->getPktType());
		}
	}
}

void C4Control::PreRec(C4Record *pRecord) const
{
	for (C4IDPacket *pPkt = firstPkt(); pPkt; pPkt = nextPkt(pPkt))
	{
		C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(pPkt->getPkt());
		if (pCtrlPkt)
			pCtrlPkt->PreRec(pRecord);
	}
}

void C4Control::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(Pkts);
}

// *** C4ControlSet

void C4ControlSet::Execute() const
{
	switch (eValType)
	{
	case C4CVT_None: break;

	case C4CVT_ControlRate: // adjust control rate
		// host only
		if (iByClient != C4ClientIDHost) break;
		// adjust control rate
		::Control.ControlRate += iData;
		::Control.ControlRate = BoundBy<int32_t>(::Control.ControlRate, 1, C4MaxControlRate);
		Game.Parameters.ControlRate = ::Control.ControlRate;
		// write back adjusted control rate to network settings
		if (::Control.isCtrlHost() && !::Control.isReplay() && ::Control.isNetwork())
			Config.Network.ControlRate = ::Control.ControlRate;
		// always show msg
		::GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_NET_CONTROLRATE"),::Control.ControlRate,Game.FrameCounter).getData());
		break;

	case C4CVT_AllowDebug: // allow debug mode?
	{
		bool fSet = !!iData;
		// disable debug
		if (!fSet && Game.DebugMode)
		{
			Game.DebugMode=false;
			::GraphicsSystem.DeactivateDebugOutput();
		}
		// save flag, log
		Game.Parameters.AllowDebug = fSet;
		Log(fSet ? "Debug ON" : "Debug OFF");
		break;
	}
	break;

	case C4CVT_MaxPlayer:
		// host only
		if (iByClient != C4ClientIDHost) break;
		// not in league
		if (Game.Parameters.isLeague())
		{
			Log("/set maxplayer disabled in league!");
			C4GUI::GUISound("Error");
			break;
		}
		// set it
		Game.Parameters.MaxPlayers = iData;
		LogF("MaxPlayer = %d", (int)Game.Parameters.MaxPlayers);
		break;

	case C4CVT_TeamDistribution:
		// host only
		if (iByClient != C4ClientIDHost) break;
		// set new value
		Game.Teams.SetTeamDistribution(static_cast<C4TeamList::TeamDist>(iData));
		break;

	case C4CVT_TeamColors:
		// host only
		if (!HostControl()) break;
		// set new value
		Game.Teams.SetTeamColors(!!iData);
		break;

	case C4CVT_FairCrew:
		// host only
		if (!HostControl()) break;
		// deny setting if it's fixed by scenario
		if (Game.Parameters.FairCrewForced)
		{
			if (::Control.isCtrlHost()) Log(LoadResStr("IDS_MSG_NOMODIFYFAIRCREW"));
			break;
		}
		// set new value
		if (iData < 0)
		{
			Game.Parameters.UseFairCrew = false;
			Game.Parameters.FairCrewStrength = 0;
		}
		else
		{
			Game.Parameters.UseFairCrew = true;
			Game.Parameters.FairCrewStrength = iData;
		}
		// runtime updates for runtime fairness adjustments
		if (Game.IsRunning)
		{
			// invalidate fair crew physicals
			const int iDefCount = ::Definitions.GetDefCount();
			for (int i = 0; i < iDefCount; i++)
				::Definitions.GetDef(i)->ClearFairCrewPhysicals();
			// show msg
			if (Game.Parameters.UseFairCrew)
			{
				int iRank = ::DefaultRanks.RankByExperience(Game.Parameters.FairCrewStrength);
				::GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_FAIRCREW_ACTIVATED"), ::DefaultRanks.GetRankName(iRank, true).getData()).getData());
			}
			else
				::GraphicsSystem.FlashMessage(LoadResStr("IDS_MSG_FAIRCREW_DEACTIVATED"));
		}
		// lobby updates
		if (::Network.isLobbyActive())
		{
			::Network.GetLobby()->UpdateFairCrew();
		}
		// this setting is part of the reference
		if (::Network.isEnabled() && ::Network.isHost())
			::Network.InvalidateReference();
		break;
	}
}

void C4ControlSet::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdapt(eValType), "Type", C4CVT_None));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iData), "Data", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlScript

void C4ControlScript::Execute() const
{
	const char *szScript = Script.getData();
	// user script: from host only
	if (!fInternal && (iByClient != C4ClientIDHost) && !Console.Active) return;
	// execute
	C4Object *pObj = NULL;
	C4AulScript *pScript;
	if (iTargetObj == SCOPE_Console)
		pScript = &Game.Script;
	else if (iTargetObj == SCOPE_Global)
		pScript = &::ScriptEngine;
	else if ((pObj = ::Objects.SafeObjectPointer(iTargetObj)))
		pScript = &(pObj->Def->Script);
	else
		// default: Fallback to global context
		pScript = &::ScriptEngine;
	C4Value rVal(pScript->DirectExec(pObj, szScript, "console script", false, C4AulScript::MAXSTRICT, fUseVarsFromCallerContext ? AulExec.GetContext(AulExec.GetContextDepth()-1) : NULL));
#ifndef NOAULDEBUG
	C4AulDebug* pDebug;
	if ( (pDebug = ::ScriptEngine.GetDebugger()) )
	{
		pDebug->ControlScriptEvaluated(szScript, rVal.GetDataString().getData());
	}
#endif
	// show messages
	if (!fInternal)
	{
		// print script
		if (pObj)
			LogF("-> %s::%s", pObj->Def->GetName(), szScript);
		else
			LogF("-> %s", szScript);
		// print result
		if (!LocalControl())
		{
			C4Network2Client *pClient = NULL;
			if (::Network.isEnabled())
				pClient = ::Network.Clients.GetClientByID(iByClient);
			if (pClient)
				LogF(" = %s (by %s)", rVal.GetDataString().getData(), pClient->getName());
			else
				LogF(" = %s (by client %d)", rVal.GetDataString().getData(), iByClient);
		}
		else
			LogF(" = %s", rVal.GetDataString().getData());
	}
}

void C4ControlScript::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iTargetObj, "TargetObj", -1));
	pComp->Value(mkNamingAdapt(fInternal, "Internal", false));
	pComp->Value(mkNamingAdapt(fUseVarsFromCallerContext, "UseVarsFromCallerContext", false));
	pComp->Value(mkNamingAdapt(Script, "Script", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlPlayerSelect

C4ControlPlayerSelect::C4ControlPlayerSelect(int32_t iPlr, const C4ObjectList &Objs, bool fIsAlt)
		: iPlr(iPlr), fIsAlt(fIsAlt), iObjCnt(Objs.ObjectCount())
{
	pObjNrs = new int32_t[iObjCnt];
	int32_t i = 0;
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		pObjNrs[i++] = pLnk->Obj->Number;
	assert(i == iObjCnt);
}

void C4ControlPlayerSelect::Execute() const
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return;

	// Check object list
	C4Object *pObj;
	int32_t iControlChecksum = 0;
	for (int32_t i = 0; i < iObjCnt; i++)
		if ((pObj = ::Objects.SafeObjectPointer(pObjNrs[i])))
		{
			iControlChecksum += pObj->Number * (iControlChecksum+4787821);
			// user defined object selection: callback to object
			if (pObj->Category & C4D_MouseSelect)
			{
				if (fIsAlt)
					pObj->Call(PSF_MouseSelectionAlt, &C4AulParSet(C4VInt(iPlr)));
				else
					pObj->Call(PSF_MouseSelection, &C4AulParSet(C4VInt(iPlr)));
			}
		}
	// count
	pPlr->CountControl(C4Player::PCID_Command, iControlChecksum);
}

void C4ControlPlayerSelect::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iPlr, "Player", -1));
	pComp->Value(mkNamingAdapt(fIsAlt, "IsAlt", false));
	pComp->Value(mkNamingAdapt(iObjCnt, "ObjCnt", 0));
	// Compile array
	if (pComp->isCompiler())
		{ delete[] pObjNrs; pObjNrs = new int32_t [iObjCnt]; }
	pComp->Value(mkNamingAdapt(mkArrayAdapt(pObjNrs, iObjCnt), "Objs", 0));

	C4ControlPacket::CompileFunc(pComp);
}


// *** C4ControlPlayerControl

void C4ControlPlayerControl::Execute() const
{
	C4PlayerControl *pTargetCtrl = NULL;
	if (iPlr == -1)
	{
		// neutral control packet: Execute in global control
	}
	else
	{
		// player-based control: Execute on control owned by player
		C4Player *pPlr=::Players.Get(iPlr);
		if (pPlr)
		{
			pTargetCtrl = &(pPlr->Control);
		}
	}
	if (pTargetCtrl) pTargetCtrl->ExecuteControlPacket(this);
}

void C4ControlPlayerControl::ControlItem::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(iControl);
	pComp->Separator();
	pComp->Value(iTriggerMode);
}

void C4ControlPlayerControl::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlr), "Player", -1));
	pComp->Value(mkNamingAdapt(fRelease, "Release", false));
	pComp->Value(mkNamingAdapt(ExtraData, "ExtraData", C4KeyEventData()));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(ControlItems), "Controls", ControlItemVec()));
	C4ControlPacket::CompileFunc(pComp);
}


// *** C4ControlPlayerCommand

C4ControlPlayerCommand::C4ControlPlayerCommand(int32_t iPlr, int32_t iCmd, int32_t iX, int32_t iY,
    C4Object *pTarget, C4Object *pTarget2, int32_t iData, int32_t iAddMode)
		: iPlr(iPlr), iCmd(iCmd), iX(iX), iY(iY),
		iTarget(::Objects.ObjectNumber(pTarget)), iTarget2(::Objects.ObjectNumber(pTarget2)),
		iData(iData), iAddMode(iAddMode)
{

}

void C4ControlPlayerCommand::Execute() const
{
	C4Player *pPlr=::Players.Get(iPlr);
	if (pPlr)
	{
		pPlr->CountControl(C4Player::PCID_Command, iCmd+iX+iY+iTarget+iTarget2);
		pPlr->ObjectCommand(iCmd,
		                    ::Objects.ObjectPointer(iTarget),
		                    iX,iY,
		                    ::Objects.ObjectPointer(iTarget2),
		                    C4Value(iData),
		                    iAddMode);
	}
}

void C4ControlPlayerCommand::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlr), "Player", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iCmd), "Cmd", 0));
	pComp->Value(mkNamingAdapt(iX, "X", 0));
	pComp->Value(mkNamingAdapt(iY, "Y", 0));
	pComp->Value(mkNamingAdapt(iTarget, "Target", 0));
	pComp->Value(mkNamingAdapt(iTarget2, "Target2", 0));
	pComp->Value(mkNamingAdapt(iData, "Data", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iAddMode), "AddMode", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlSyncCheck

C4ControlSyncCheck::C4ControlSyncCheck()
{
}

void C4ControlSyncCheck::Set()
{
	extern int32_t FRndPtr3;
	Frame = Game.FrameCounter;
	ControlTick = ::Control.ControlTick;
	Random3 = FRndPtr3;
	RandomCount = ::RandomCount;
	AllCrewPosX = GetAllCrewPosX();
	PXSCount = ::PXS.Count;
	MassMoverIndex = ::MassMover.CreatePtr;
	ObjectCount = ::Objects.ObjectCount();
	ObjectEnumerationIndex = Game.ObjectEnumerationIndex;
	SectShapeSum = ::Objects.Sectors.getShapeSum();
}

int32_t C4ControlSyncCheck::GetAllCrewPosX()
{
	int32_t cpx=0;
	for (C4Player *pPlr=::Players.First; pPlr; pPlr=pPlr->Next)
		for (C4ObjectLink *clnk=pPlr->Crew.First; clnk; clnk=clnk->Next)
			cpx += fixtoi(clnk->Obj->fix_x, 100);
	return cpx;
}

void C4ControlSyncCheck::Execute() const
{
	// control host?
	if (::Control.isCtrlHost()) return;

	// get the saved sync check data
	C4ControlSyncCheck* pSyncCheck = ::Control.GetSyncCheck(Frame), &SyncCheck = *pSyncCheck;
	if (!pSyncCheck)
	{
		::Control.SyncChecks.Add(CID_SyncCheck, new C4ControlSyncCheck(*this));
		return;
	}

	// Not equal
	if ( Frame                  != pSyncCheck->Frame
	     ||(ControlTick            != pSyncCheck->ControlTick   && !::Control.isReplay())
	     || Random3                != pSyncCheck->Random3
	     || RandomCount            != pSyncCheck->RandomCount
	     || AllCrewPosX            != pSyncCheck->AllCrewPosX
	     || PXSCount               != pSyncCheck->PXSCount
	     || MassMoverIndex         != pSyncCheck->MassMoverIndex
	     || ObjectCount            != pSyncCheck->ObjectCount
	     || ObjectEnumerationIndex != pSyncCheck->ObjectEnumerationIndex
	     || SectShapeSum           != pSyncCheck->SectShapeSum)
	{
		const char *szThis = "Client", *szOther = ::Control.isReplay() ? "Rec ":"Host";
		if (iByClient != ::Control.ClientID())
			{ const char *szTemp = szThis; szThis = szOther; szOther = szTemp; }
		// Message
		LogFatal("Network: Synchronization loss!");
		LogFatal(FormatString("Network: %s Frm %i Ctrl %i Rnc %i Rn3 %i Cpx %i PXS %i MMi %i Obc %i Oei %i Sct %i", szThis, Frame,ControlTick,RandomCount,Random3,AllCrewPosX,PXSCount,MassMoverIndex,ObjectCount,ObjectEnumerationIndex, SectShapeSum).getData());
		LogFatal(FormatString("Network: %s Frm %i Ctrl %i Rnc %i Rn3 %i Cpx %i PXS %i MMi %i Obc %i Oei %i Sct %i", szOther, SyncCheck.Frame,SyncCheck.ControlTick,SyncCheck.RandomCount,SyncCheck.Random3,SyncCheck.AllCrewPosX,SyncCheck.PXSCount,SyncCheck.MassMoverIndex,SyncCheck.ObjectCount,SyncCheck.ObjectEnumerationIndex, SyncCheck.SectShapeSum).getData());
		StartSoundEffect("SyncError");
#ifdef _DEBUG
		// Debug safe
		C4GameSaveNetwork SaveGame(false);
		SaveGame.Save(Config.AtExePath("Desync.c4s"));
#endif
		// league: Notify regular client disconnect within the game
		::Network.LeagueNotifyDisconnect(C4ClientIDHost, C4LDR_Desync);
		// Deactivate / end
		if (::Control.isReplay())
			Game.DoGameOver();
		else if (::Control.isNetwork())
		{
			Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, "Network: Synchronization loss!");
			::Network.Clear();
		}
	}

}

void C4ControlSyncCheck::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(Frame), "Frame", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(ControlTick), "ControlTick", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(Random3), "Random3", 0));
	pComp->Value(mkNamingAdapt(RandomCount, "RandomCount", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(AllCrewPosX), "AllCrewPosX", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(PXSCount), "PXSCount", 0));
	pComp->Value(mkNamingAdapt(MassMoverIndex, "MassMoverIndex", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(ObjectCount), "ObjectCount", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(ObjectEnumerationIndex), "ObjectEnumerationIndex", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(SectShapeSum), "SectShapeSum", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlSynchronize

void C4ControlSynchronize::Execute() const
{
	Game.Synchronize(fSavePlrFiles);
	if (fSyncClearance) Game.SyncClearance();
}

void C4ControlSynchronize::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(fSavePlrFiles, "SavePlrs", false));
	pComp->Value(mkNamingAdapt(fSyncClearance, "SyncClear", false));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlClientJoin

void C4ControlClientJoin::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	// add client
	C4Client *pClient = Game.Clients.Add(Core);
	if (!pClient) return;
	// log
	LogF(LoadResStr("IDS_NET_CLIENT_JOIN"), Core.getName());
	// lobby callback
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	if (pLobby) pLobby->OnClientJoin(pClient);
	// console callback
	if (Console.Active) Console.UpdateMenus();
}

void C4ControlClientJoin::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Core, "ClientCore"));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4Control

void C4ControlClientUpdate::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	// find client
	C4Client *pClient = Game.Clients.getClientByID(iID);
	if (!pClient) return;
	StdCopyStrBuf strClient(LoadResStr(pClient->isLocal() ? "IDS_NET_LOCAL_CLIENT" : "IDS_NET_CLIENT"));
	// do whatever specified
	switch (eType)
	{
	case CUT_None: break;
	case CUT_Activate:
		// nothing to do?
		if (pClient->isActivated() == !!iData) break;
		// log
		LogF(LoadResStr(iData ? "IDS_NET_CLIENT_ACTIVATED" : "IDS_NET_CLIENT_DEACTIVATED"), strClient.getData(), pClient->getName());
		// activate/deactivate
		pClient->SetActivated(!!iData);
		// local?
		if (pClient->isLocal())
			::Control.SetActivated(!!iData);
		break;
	case CUT_SetObserver:
		// nothing to do?
		if (pClient->isObserver()) break;
		// log
		LogF(LoadResStr("IDS_NET_CLIENT_OBSERVE"), strClient.getData(), pClient->getName());
		// set observer (will deactivate)
		pClient->SetObserver();
		// local?
		if (pClient->isLocal())
			::Control.SetActivated(false);
		// remove all players ("soft kick")
		::Players.RemoveAtClient(iID, true);
		break;
	}
}

void C4ControlClientUpdate::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eType), "Type", CUT_None));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iID), "ClientID", C4ClientIDUnknown));
	if (eType == CUT_Activate)
		pComp->Value(mkNamingAdapt(mkIntPackAdapt(iData), "Data", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlClientRemove

void C4ControlClientRemove::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	if (iID == C4ClientIDHost) return;
	// find client
	C4Client *pClient = Game.Clients.getClientByID(iID);
	if (!pClient)
	{
		// TODO: in replays, client list is not yet synchronized
		// remove players anyway
		if (::Control.isReplay()) ::Players.RemoveAtClient(iID, true);
		return;
	}
	StdCopyStrBuf strClient(LoadResStr(pClient->isLocal() ? "IDS_NET_LOCAL_CLIENT" : "IDS_NET_CLIENT"));
	// local?
	if (pClient->isLocal())
	{
		StdStrBuf sMsg;
		sMsg.Format(LoadResStr("IDS_NET_CLIENT_REMOVED"), strClient.getData(), pClient->getName(), strReason.getData());
		Log(sMsg.getData());
		Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, sMsg.getData());
		::Control.ChangeToLocal();
		return;
	}
	// remove client
	if (!Game.Clients.Remove(pClient)) return;
	// log
	LogF(LoadResStr("IDS_NET_CLIENT_REMOVED"), strClient.getData(), pClient->getName(), strReason.getData());
	// remove all players
	::Players.RemoveAtClient(iID, true);
	// remove all resources
	if (::Network.isEnabled())
		::Network.ResList.RemoveAtClient(iID);
	// lobby callback
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	if (pLobby && ::pGUI) pLobby->OnClientPart(pClient);
	// player list callback
	::Network.Players.OnClientPart(pClient);
	// console callback
	if (Console.Active) Console.UpdateMenus();

	// delete
	delete pClient;
}

void C4ControlClientRemove::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iID), "ClientID", CUT_None));
	pComp->Value(mkNamingAdapt(strReason, "Reason", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlJoinPlayer

C4ControlJoinPlayer::C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo, const C4Network2ResCore &ResCore)
		: Filename(szFilename, true), iAtClient(iAtClient),
		idInfo(iIDInfo), fByRes(true), ResCore(ResCore)
{
}

C4ControlJoinPlayer::C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo)
		: Filename(szFilename, true), iAtClient(iAtClient),
		idInfo(iIDInfo), fByRes(false)
{
	// load from file if filename is given - which may not be the case for script players
	if (szFilename)
	{
		StdStrBuf filename_buf;
		const char *filename = Config.AtDataReadPath(szFilename);
		bool file_is_temp = false;
		if (DirectoryExists(filename))
		{
			// the player file is unpacked - temp pack and read
			filename_buf.Copy(Config.AtTempPath(GetFilenameOnly(filename)));
			MakeTempFilename(&filename_buf);
			if (C4Group_PackDirectoryTo(filename, filename_buf.getData()))
			{
				filename = filename_buf.getData();
				file_is_temp = true;
			}
			else
			{
				// pack failed
				LogF("[!]Error packing player file %s to %s for join: Pack failed.", filename, filename_buf.getData());
				assert(false);
			}
		}
		bool fSuccess = PlrData.LoadFromFile(filename);
		if (!fSuccess)
		{
			assert(false);
			LogF("[!]Error loading player file from %s.", filename);
		}
		if (file_is_temp) EraseFile(filename);
	}
}

void C4ControlJoinPlayer::Execute() const
{
	const char *szFilename = Filename.getData();

	// get client
	C4Client *pClient = Game.Clients.getClientByID(iAtClient);
	if (!pClient) return;

	// get info
	C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(idInfo);
	if (!pInfo)
	{
		LogF("ERROR: Ghost player join: No info for %d", idInfo);
		assert(false);
		return;
	}

	else if (LocalControl())
	{
		// Local player: Just join from local file
		Game.JoinPlayer(szFilename, iAtClient, pClient->getName(), pInfo);
	}
	else if (!fByRes)
	{
		if (PlrData.getSize())
		{
			// create temp file
			StdStrBuf PlayerFilename; PlayerFilename.Format("%s-%s",pClient->getName(),GetFilename(szFilename));
			PlayerFilename = Config.AtTempPath(PlayerFilename.getData());
			// copy to it
			if (PlrData.SaveToFile(PlayerFilename.getData()))
			{
				Game.JoinPlayer(PlayerFilename.getData(), iAtClient, pClient->getName(), pInfo);
				EraseFile(PlayerFilename.getData());
			}
		}
		else if (pInfo->GetType() == C4PT_Script)
		{
			// script players may join without data
			Game.JoinPlayer(NULL, iAtClient, pClient->getName(), pInfo);
		}
		else
		{
			// no player data for user player present: Must not happen
			LogF("ERROR: Ghost player join: No player data for %s", (const char*)pInfo->GetName());
			assert(false);
			return;
		}
	}
	else if (::Control.isNetwork())
	{
		// Find ressource
		C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(ResCore.getID());
		if (pRes && pRes->isComplete())
			Game.JoinPlayer(pRes->getFile(), iAtClient, pClient->getName(), pInfo);
	}
	else if (::Control.isReplay())
	{
		// Expect player in scenario file
		StdStrBuf PlayerFilename; PlayerFilename.Format("%s" DirSep "%d-%s", Game.ScenarioFilename, ResCore.getID(), GetFilename(ResCore.getFileName()));
		Game.JoinPlayer(PlayerFilename.getData(), iAtClient, pClient ? pClient->getName() : "Unknown", pInfo);
	}
	else
		// Shouldn't happen
		assert(false);
}

void C4ControlJoinPlayer::Strip()
{
	// By resource? Can't touch player file, then.
	if (fByRes) return;
	// create temp file
	StdStrBuf PlayerFilename; PlayerFilename = GetFilename(Filename.getData());
	PlayerFilename = Config.AtTempPath(PlayerFilename.getData());
	// Copy to it
	if (PlrData.SaveToFile(PlayerFilename.getData()))
	{
		// open as group
		C4Group Grp;
		if (!Grp.Open(PlayerFilename.getData()))
			{ EraseFile(PlayerFilename.getData()); return; }
		// remove portrais
		Grp.Delete(C4CFN_Portraits, true);
		// remove bigicon, if the file size is too large
		size_t iBigIconSize=0;
		if (Grp.FindEntry(C4CFN_BigIcon, NULL, &iBigIconSize))
			if (iBigIconSize > C4NetResMaxBigicon*1024)
				Grp.Delete(C4CFN_BigIcon);
		Grp.Close();
		// Set new data
		StdBuf NewPlrData;
		if (!NewPlrData.LoadFromFile(PlayerFilename.getData()))
			{ EraseFile(PlayerFilename.getData()); return; }
		PlrData = NewPlrData;
		// Done
		EraseFile(PlayerFilename.getData());
	}
}

bool C4ControlJoinPlayer::PreExecute() const
{
	// all data included in control packet?
	if (!fByRes) return true;
	// client lost?
	if (!Game.Clients.getClientByID(iAtClient)) return true;
	// network only
	if (!::Control.isNetwork()) return true;
	// search ressource
	C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(ResCore.getID());
	// doesn't exist? start loading
	if (!pRes) { pRes = ::Network.ResList.AddByCore(ResCore, true); }
	if (!pRes) return true;
	// is loading or removed?
	return !pRes->isLoading();
}

void C4ControlJoinPlayer::PreRec(C4Record *pRecord)
{
	if (!pRecord) return;
	if (fByRes)
	{
		// get local file by id
		C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(ResCore.getID());
		if (!pRes || pRes->isRemoved()) return;
		// create a copy of the resource
		StdStrBuf szTemp; szTemp.Copy(pRes->getFile());
		MakeTempFilename(&szTemp);
		if (C4Group_CopyItem(pRes->getFile(), szTemp.getData()))
		{
			// add to record
			StdStrBuf szTarget = FormatString("%d-%s", ResCore.getID(), GetFilename(ResCore.getFileName()));
			pRecord->AddFile(szTemp.getData(), szTarget.getData(), true);
		}
	}
	else
	{
		// player data raw within control: Will be used directly in record
	}
}

void C4ControlJoinPlayer::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkNetFilenameAdapt(Filename), "Filename", ""));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iAtClient), "AtClient", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(idInfo), "InfoID", -1));
	pComp->Value(mkNamingAdapt(fByRes, "ByRes", false));
	if (fByRes)
		pComp->Value(mkNamingAdapt(ResCore, "ResCore"));
	else
		pComp->Value(mkNamingAdapt(PlrData, "PlrData"));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlEMMoveObject

C4ControlEMMoveObject::C4ControlEMMoveObject(C4ControlEMObjectAction eAction, int32_t tx, int32_t ty, C4Object *pTargetObj,
    int32_t iObjectNum, int32_t *pObjects, const char *szScript)
		: eAction(eAction), tx(tx), ty(ty), iTargetObj(::Objects.ObjectNumber(pTargetObj)),
		iObjectNum(iObjectNum), pObjects(pObjects), Script(szScript, true)
{

}

C4ControlEMMoveObject::~C4ControlEMMoveObject()
{
	delete [] pObjects; pObjects = NULL;
}

void C4ControlEMMoveObject::Execute() const
{
	bool fLocalCall = LocalControl();
	switch (eAction)
	{
	case EMMO_Move:
	{
		if (!pObjects) break;
		// move all given objects
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i]))) if (pObj->Status)
				{
					pObj->ForcePosition(pObj->GetX()+tx,pObj->GetY()+ty);
					pObj->xdir=pObj->ydir=0;
					pObj->Mobile = false;
				}
	}
	break;
	case EMMO_Enter:
	{
		if (!pObjects) break;
		// enter all given objects into target
		C4Object *pObj, *pTarget = ::Objects.SafeObjectPointer(iTargetObj);
		if (pTarget)
			for (int i=0; i<iObjectNum; ++i)
				if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
					pObj->Enter(pTarget);
	}
	break;
	case EMMO_Duplicate:
	{
		if (!pObjects) break;
		// local call? adjust selection then
		if (fLocalCall) Console.EditCursor.GetSelection().Clear();
		// perform duplication
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
			{
				pObj = Game.CreateObject(pObj->GetPrototype(), pObj, pObj->Owner, pObj->GetX(), pObj->GetY());
				if (pObj && fLocalCall) Console.EditCursor.GetSelection().Add(pObj, C4ObjectList::stNone);
			}
		// update status
		if (fLocalCall)
		{
			Console.EditCursor.SetHold(true);
			Console.PropertyDlg.Update(Console.EditCursor.GetSelection());
		}
	}
	break;
	case EMMO_Script:
	{
		if (!pObjects) return;
		// execute script ...
		C4ControlScript ScriptCtrl(Script.getData(), C4ControlScript::SCOPE_Global, false);
		ScriptCtrl.SetByClient(iByClient);
		// ... for each object in selection
		for (int i=0; i<iObjectNum; ++i)
		{
			ScriptCtrl.SetTargetObj(pObjects[i]);
			ScriptCtrl.Execute();
		}
		break;
	}
	case EMMO_Remove:
	{
		if (!pObjects) return;
		// remove all objects
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
				pObj->AssignRemoval();
	}
	break; // Here was fallthrough. Seemed wrong. ck.
	case EMMO_Exit:
	{
		if (!pObjects) return;
		// exit all objects
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
				pObj->Exit(pObj->GetX(), pObj->GetY(), pObj->r);
	}
	break; // Same. ck.
	}
	// update property dlg & status bar
	if (fLocalCall)
		Console.EditCursor.OnSelectionChanged();
}

void C4ControlEMMoveObject::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eAction), "Action"));
	pComp->Value(mkNamingAdapt(tx, "tx", 0));
	pComp->Value(mkNamingAdapt(ty, "ty", 0));
	pComp->Value(mkNamingAdapt(iTargetObj, "TargetObj", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iObjectNum), "ObjectNum", 0));
	if (pComp->isCompiler()) { delete [] pObjects; pObjects = new int32_t [iObjectNum]; }
	pComp->Value(mkNamingAdapt(mkArrayAdapt(pObjects, iObjectNum), "Objs", -1));
	if (eAction == EMMO_Script)
		pComp->Value(mkNamingAdapt(Script, "Script", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlEMDrawTool

C4ControlEMDrawTool::C4ControlEMDrawTool(C4ControlEMDrawAction eAction, int32_t iMode,
    int32_t iX, int32_t iY, int32_t iX2, int32_t iY2, int32_t iGrade,
    bool fIFT, const char *szMaterial, const char *szTexture)
		: eAction(eAction), iMode(iMode), iX(iX), iY(iY), iX2(iX2), iY2(iY2), iGrade(iGrade),
		fIFT(fIFT), Material(szMaterial, true), Texture(szTexture, true)
{

}

void C4ControlEMDrawTool::Execute() const
{
	// set new mode
	if (eAction == EMDT_SetMode)
	{
		Console.ToolsDlg.SetLandscapeMode(iMode, true);
		return;
	}
	// check current mode
	assert(::Landscape.Mode == iMode);
	if (::Landscape.Mode != iMode) return;
	// assert validity of parameters
	if (!Material.getSize()) return;
	const char *szMaterial = Material.getData(),
	                         *szTexture = Texture.getData();
	// perform action
	switch (eAction)
	{
	case EMDT_Brush: // brush tool
		if (!Texture.getSize()) break;
		::Landscape.DrawBrush(iX, iY, iGrade, szMaterial, szTexture, fIFT);
		break;
	case EMDT_Line: // line tool
		if (!Texture.getSize()) break;
		::Landscape.DrawLine(iX,iY,iX2,iY2, iGrade, szMaterial, szTexture, fIFT);
		break;
	case EMDT_Rect: // rect tool
		if (!Texture.getSize()) break;
		::Landscape.DrawBox(iX,iY,iX2,iY2, iGrade, szMaterial, szTexture, fIFT);
		break;
	case EMDT_Fill: // fill tool
	{
		int iMat = ::MaterialMap.Get(szMaterial);
		if (!MatValid(iMat)) return;
		for (int cnt=0; cnt<iGrade; cnt++)
			::Landscape.InsertMaterial(iMat,iX+Random(iGrade)-iGrade/2,iY+Random(iGrade)-iGrade/2);
	}
	break;
	default:
		break;
	}
}


void C4ControlEMDrawTool::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eAction), "Action"));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iMode), "Mode", 0));
	pComp->Value(mkNamingAdapt(iX, "X", 0));
	pComp->Value(mkNamingAdapt(iY, "Y", 0));
	pComp->Value(mkNamingAdapt(iX2, "X2", 0));
	pComp->Value(mkNamingAdapt(iY2, "Y2", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iGrade),  "Grade", 0));
	pComp->Value(mkNamingAdapt(fIFT, "IFT", false));
	pComp->Value(mkNamingAdapt(Material, "Material", ""));
	pComp->Value(mkNamingAdapt(Texture, "Texture", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlMessage

void C4ControlMessage::Execute() const
{
	const char *szMessage = Message.getData();
	// get player
	C4Player *pPlr = (iPlayer < 0 ? NULL : ::Players.Get(iPlayer));
	// security
	if (pPlr && pPlr->AtClient != iByClient) return;
	// do not record message as control, because it is not synced!
	//if (pPlr) pPlr->CountControl(C4Player::PCID_Message, Message.GetHash());
	// get lobby to forward to
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	StdStrBuf str;
	switch (eType)
	{
	case C4CMT_Normal:
	case C4CMT_Me:
		// log it
		if (pPlr)
		{
			if (pPlr->AtClient != iByClient) break;
			str.Format((eType == C4CMT_Normal ? "<c %x><<i></i>%s> %s</c>" : "<c %x> * %s %s</c>"),
			           pPlr->ColorDw, pPlr->GetName(), szMessage);
		}
		else
		{
			C4Client *pClient = Game.Clients.getClientByID(iByClient);
			str.Format((eType == C4CMT_Normal ? "<%s> %s" : " * %s %s"),
			           pClient ? pClient->getNick() : "???", szMessage);
		}
		// 2 lobby
		if (pLobby)
			pLobby->OnMessage(Game.Clients.getClientByID(iByClient), str.getData());
		// or 2 log
		else
			Log(str.getData());
		break;

	case C4CMT_Say:
		// show as game message above player cursor
		if (pPlr && pPlr->Cursor)
		{
			if (Game.C4S.Head.Film == C4SFilm_Cinematic)
			{
				StdStrBuf sMessage; sMessage.Format("<%s> %s", pPlr->Cursor->GetName(), szMessage);
				uint32_t dwClr = pPlr->Cursor->Color;
				if (!dwClr) dwClr = 0xff;
				GameMsgObjectDw(sMessage.getData(), pPlr->Cursor, dwClr|0xff000000);
			}
			else
				GameMsgObjectDw(szMessage, pPlr->Cursor, pPlr->ColorDw|0xff000000);
		}
		break;

	case C4CMT_Team:
	{
		// show only if sending player is allied with a local one
		if (pPlr)
		{
			// for running game mode, check actual hostility
			C4Player *pLocalPlr;
			for (int cnt = 0; (pLocalPlr = ::Players.GetLocalByIndex(cnt)); cnt++)
				if (!Hostile(pLocalPlr->Number, iPlayer))
					break;
			if (pLocalPlr) Log(FormatString("<c %x>{%s} %s</c>", pPlr->ColorDw, pPlr->GetName(), szMessage).getData());
		}
		else if (pLobby)
		{
			// in lobby mode, no player has joined yet - check teams of unjoined players
			if (!Game.PlayerInfos.HasSameTeamPlayers(iByClient, Game.Clients.getLocalID())) break;
			// OK - permit message
			C4Client *pClient = Game.Clients.getClientByID(iByClient);
			pLobby->OnMessage(Game.Clients.getClientByID(iByClient),
			                  FormatString("{%s} %s", pClient ? pClient->getNick() : "???", szMessage).getData());
		}
	}
	break;

	case C4CMT_Private:
	{
		if (!pPlr) break;
		// show only if the target player is local
		C4Player *pLocalPlr;
		for (int cnt = 0; (pLocalPlr = ::Players.GetLocalByIndex(cnt)); cnt++)
			if (pLocalPlr->ID == iToPlayer)
				break;
		if (pLocalPlr)
		{
			Log(FormatString("<c %x>[%s] %s</c>", pPlr->ColorDw, pPlr->GetName(), szMessage).getData());
		}
	}
	break;

	case C4CMT_Sound:
		// tehehe, sound!
		if (StartSoundEffect(szMessage, false, 100, NULL))
		{
			if (pLobby) pLobby->OnClientSound(Game.Clients.getClientByID(iByClient));
		}
		break;

	case C4CMT_Alert:
		// notify inactive users
		Application.NotifyUserIfInactive();
		break;

	case C4CMT_System:
		// sender must be host
		if (!HostControl()) break;
		// show
		LogF("Network: %s", szMessage);
		break;

	}
}

void C4ControlMessage::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eType), "Type", C4CMT_Normal));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlayer), "Player", -1));
	if (eType == C4CMT_Private)
		pComp->Value(mkNamingAdapt(mkIntPackAdapt(iToPlayer), "ToPlayer", -1));
	pComp->Value(mkNamingAdapt(Message, "Message", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlPlayerInfo

void C4ControlPlayerInfo::Execute() const
{
	// join to player info list
	// replay and local control: direct join
	if (::Control.isReplay() || !::Control.isNetwork())
	{
		// add info directly
		Game.PlayerInfos.AddInfo(new C4ClientPlayerInfos(PlrInfo));
		// make sure team list reflects teams set in player infos
		Game.Teams.RecheckPlayers();
		// replay: actual player join packet will follow
		// offline game: Issue the join
		if (::Control.isLocal())
			Game.PlayerInfos.LocalJoinUnjoinedPlayersInQueue();
	}
	else
		// network:
		::Network.Players.HandlePlayerInfo(PlrInfo);
}

void C4ControlPlayerInfo::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(PlrInfo);
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlRemovePlr

void C4ControlRemovePlr::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	// remove
	::Players.Remove(iPlr, fDisconnected, false);
}

void C4ControlRemovePlr::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlr), "Plr", -1));
	pComp->Value(mkNamingAdapt(fDisconnected, "Disconnected", false));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlDebugRec

void C4ControlDebugRec::Execute() const
{

}

void C4ControlDebugRec::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(Data);
}

// *** C4ControlVote

StdStrBuf C4ControlVote::getDesc() const
{
	// Describe action
	StdStrBuf Action;
	switch (eType)
	{
	case VT_Cancel:
		Action = LoadResStr("IDS_VOTE_CANCELTHEROUND"); break;
	case VT_Kick:
		if (iData == iByClient)
			Action = LoadResStr("IDS_VOTE_LEAVETHEGAME");
		else
		{
			C4Client *pTargetClient = Game.Clients.getClientByID(iData);
			Action.Format(LoadResStr("IDS_VOTE_KICKCLIENT"), pTargetClient ? pTargetClient->getName() : "???");
		}
		break;
	case VT_Pause:
		if (iData)
			Action = LoadResStr("IDS_TEXT_PAUSETHEGAME");
		else
			Action = LoadResStr("IDS_TEXT_UNPAUSETHEGAME");
		break;
	default:
		Action = "perform some mysterious action"; break;
	}
	return Action;
}

StdStrBuf C4ControlVote::getDescWarning() const
{
	StdStrBuf Warning;
	switch (eType)
	{
	case VT_Cancel:
		Warning = LoadResStr("IDS_TEXT_WARNINGIFTHEGAMEISCANCELL"); break;
	case VT_Kick:
		Warning = LoadResStr("IDS_TEXT_WARNINGNOLEAGUEPOINTSWILL"); break;
	default:
		Warning = ""; break;
	}
	return Warning;
}

void C4ControlVote::Execute() const
{
	// Log
	C4Client *pClient = Game.Clients.getClientByID(iByClient);
	if (fApprove)
		LogF(LoadResStr("IDS_VOTE_WANTSTO"), pClient->getName(), getDesc().getData());
	else
		LogF(LoadResStr("IDS_VOTE_DOESNOTWANTTO"), pClient->getName(), getDesc().getData());
	// Save vote back
	if (::Network.isEnabled())
		::Network.AddVote(*this);
	// Vote done?
	if (::Control.isCtrlHost())
	{
		// Count votes
		int32_t iPositive = 0, iNegative = 0, iVotes = 0;
		// If there are no teams, count as if all were in the same team
		// (which happens to be equivalent to "everyone is in his own team" here)
		for (int32_t i = 0; i < Max<int32_t>(Game.Teams.GetTeamCount(), 1); i++)
		{
			C4Team *pTeam = Game.Teams.GetTeamByIndex(i);
			// Votes for this team
			int32_t iPositiveTeam = 0, iNegativeTeam = 0, iVotesTeam = 0;
			// Check each player
			for (int32_t j = 0; j < (pTeam ? pTeam->GetPlayerCount() : Game.PlayerInfos.GetPlayerCount()); j++)
			{
				int32_t iClientID = C4ClientIDUnknown;
				C4PlayerInfo *pNfo;
				if (!pTeam)
				{
					pNfo = Game.PlayerInfos.GetPlayerInfoByIndex(j);
					if (!pNfo) continue; // shouldn't happen
					iClientID = Game.PlayerInfos.GetClientInfoByPlayerID(pNfo->GetID())->GetClientID();
				}
				else
				{
					pNfo = Game.PlayerInfos.GetPlayerInfoByID(pTeam->GetIndexedPlayer(j), &iClientID);
					if (!pNfo) continue; // shouldn't happen
				}
				if (iClientID < 0) continue;
				// Client disconnected?
				if (!Game.Clients.getClientByID(iClientID)) continue;
				// Player eliminated or never joined?
				if (!pNfo->IsJoined()) continue;
				// Okay, this player can vote
				iVotesTeam++;
				// Search vote of this client on the subject
				C4IDPacket *pPkt; C4ControlVote *pVote;
				if ((pPkt = ::Network.GetVote(iClientID, eType, iData)))
					if ((pVote = static_cast<C4ControlVote *>(pPkt->getPkt())))
					{
						if (pVote->isApprove())
							iPositiveTeam++;
						else
							iNegativeTeam++;
					}
			}
			// Any votes available?
			if (iVotesTeam)
			{
				iVotes++;
				// Approval by team? More then 50% needed
				if (iPositiveTeam * 2 > iVotesTeam)
					iPositive++;
				// Disapproval by team? More then 50% needed
				else if (iNegativeTeam * 2 >= iVotesTeam)
					iNegative++;
			}
		}
		// Approval? More then 50% needed
		if (iPositive * 2 > iVotes)
			::Control.DoInput(CID_VoteEnd,
			                  new C4ControlVoteEnd(eType, true, iData),
			                  CDT_Sync);
		// Disapproval?
		else if (iNegative * 2 >= iVotes)
			::Control.DoInput(CID_VoteEnd,
			                  new C4ControlVoteEnd(eType, false, iData),
			                  CDT_Sync);
	}
}

void C4ControlVote::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eType), "Type", VT_None));
	pComp->Value(mkNamingAdapt(fApprove, "Approve", true));
	pComp->Value(mkNamingAdapt(iData, "Data", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlVoteEnd

void C4ControlVoteEnd::Execute() const
{
	// End the voting process
	if (!HostControl()) return;
	if (::Network.isEnabled())
		::Network.EndVote(getType(), isApprove(), getData());
	// Log
	StdStrBuf sMsg;
	if (isApprove())
		sMsg.Format(LoadResStr("IDS_TEXT_ITWASDECIDEDTO"), getDesc().getData());
	else
		sMsg.Format(LoadResStr("IDS_TEXT_ITWASDECIDEDNOTTO"), getDesc().getData());
	Log(sMsg.getData());
	// Approved?
	if (!isApprove()) return;
	// Do it
	C4ClientPlayerInfos *pInfos; C4PlayerInfo *pInfo;
	int iClient, iInfo;
	switch (getType())
	{
	case VT_Cancel:
		// Flag players
		if (!Game.GameOver)
			for (iClient = 0; (pInfos = Game.PlayerInfos.GetIndexedInfo(iClient)); iClient++)
				for (iInfo = 0; (pInfo = pInfos->GetPlayerInfo(iInfo)); iInfo++)
					if (!pInfo->IsRemoved())
						pInfo->SetVotedOut();
		// Abort the game
		Game.Abort(true);
		break;
	case VT_Kick:
		// Flag players
		pInfos = Game.PlayerInfos.GetInfoByClientID(getData());
		if (!Game.GameOver)
			if (pInfos)
				for (iInfo = 0; (pInfo = pInfos->GetPlayerInfo(iInfo)); iInfo++)
					if (!pInfo->IsRemoved())
						pInfo->SetVotedOut();
		// Remove the client
		if (::Control.isCtrlHost())
		{
			C4Client *pClient = Game.Clients.getClientByID(getData());
			if (pClient)
				Game.Clients.CtrlRemove(pClient, LoadResStr("IDS_VOTE_VOTEDOUT"));
		}
		// It is ourselves that have been voted out?
		if (getData() == Game.Clients.getLocalID())
		{
			// otherwise, we have been kicked by the host.
			// Do a regular disconnect and display reason in game over dialog, so the client knows what has happened!
			Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, FormatString(LoadResStr("IDS_ERR_YOUHAVEBEENREMOVEDBYVOTIN"), sMsg.getData()).getData());
			::Network.Clear();
			// Game over immediately, so poor player won't continue game alone
			Game.DoGameOver();
		}
		break;
	default:
		// TODO
		break;
	}
}

void C4ControlVoteEnd::CompileFunc(StdCompiler *pComp)
{
	C4ControlVote::CompileFunc(pComp);
}
