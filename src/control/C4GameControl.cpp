/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2008  Peter Wortmann
 * Copyright (c) 2005-2008  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
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
/* control management */

#include "C4Include.h"

#ifndef BIG_C4INCLUDE
#include <C4Application.h>
#include <C4Game.h>
#include <C4GameControl.h>
#include <C4GameOverDlg.h>
#include <C4Record.h>
#include <C4Log.h>
#include <C4Network2Stats.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4355)
#endif

// *** C4GameControl

C4GameControl::C4GameControl()
	: Network(this)
{
  Default();
}

C4GameControl::~C4GameControl()
{
	Clear();
}

bool C4GameControl::InitLocal(C4Client *pLocal)
{
	eMode = CM_Local; fPreInit = fInitComplete = true;
	fHost = true; iClientID = pLocal->getID();
	ControlRate = 1;
	// ok
	return true;
}

bool C4GameControl::InitNetwork(C4Client *pLocal)
{
	// network should already be initialized (by C4Network2)
	if(!Network.IsEnabled())
		return false;
	// set mode
	eMode = CM_Network; fPreInit = fInitComplete = true;
	fHost = pLocal->isHost(); iClientID = pLocal->getID();
	// control rate by parameters
	ControlRate = Game.Parameters.ControlRate;
	// ok
	return true;
}

bool C4GameControl::InitReplay(C4Group &rGroup)
{
	// open replay
	pPlayback = new C4Playback();
	if(!pPlayback->Open(rGroup))
	{
		LogFatal(LoadResStr("IDS_ERR_REPLAYREAD"));
		delete pPlayback; pPlayback = NULL;
		return false;
	}
	// set mode
	eMode = CM_Replay; fInitComplete = true;
	fHost = false; iClientID = C4ClientIDUnknown;
	// control rate by parameters
	ControlRate = Game.Parameters.ControlRate;
	// just in case
	StopRecord();
	// ok
	return true;
}

void C4GameControl::ChangeToLocal()
{
	// changes from any given mode to local
	// (emergency - think of network disconnect)

	// remove all non-local clients
	Game.Clients.RemoveRemote();
	// activate local client
	if(Game.Clients.getLocal())
		Game.Clients.getLocal()->SetActivated(true);

	// network: clear network
	if(eMode == CM_Network)
		{
		Network.Clear();
		if(::Network.isEnabled())
			::Network.Clear();
		}
	// replay: close playback
	else if(eMode == CM_Replay)
		{ delete pPlayback; pPlayback = NULL; }

	// we're now managing our own player info list; make sure counter works
	Game.PlayerInfos.FixIDCounter();

	// start the game, if we're not in the game over dialog
	// (otherwise, clients start game when host disconnected!)
	if (!C4GameOverDlg::IsShown()) Game.HaltCount = 0;

  // set status
	eMode = CM_Local; fHost = true;
	ControlRate = 1;
}

void C4GameControl::OnGameSynchronizing()
	{
	// start record if desired
	if (fRecordNeeded)
		{
		fRecordNeeded = false;
		StartRecord(false, false);
		}
	}

bool C4GameControl::StartRecord(bool fInitial, bool fStreaming)
{
  assert(fInitComplete);
	// already recording?
	if(pRecord) StopRecord();
	// start
	pRecord = new C4Record();
	if(!pRecord->Start(fInitial))
	{
		delete pRecord; pRecord = NULL;
		return false;
	}
	// streaming
	if(fStreaming)
	{
		if(!pRecord->StartStreaming(fInitial) ||
			 !::Network.StartStreaming(pRecord))
		{
			delete pRecord; pRecord = NULL;
			return false;
		}
	}
	// runtime records executed through queue: Must record initial control
	if (pExecutingControl)
		pRecord->Rec(*pExecutingControl, Game.FrameCounter);
	// ok
	return true;
}

void C4GameControl::StopRecord(StdStrBuf *pRecordName, BYTE *pRecordSHA1)
{
	if(pRecord)
	{
		::Network.FinishStreaming();
    pRecord->Stop(pRecordName, pRecordSHA1);
		// just delete
		delete pRecord; pRecord = NULL;
	}
}

void C4GameControl::RequestRuntimeRecord()
	{
	if (!IsRuntimeRecordPossible()) return; // cannot record
  fRecordNeeded = true;
	// request through a synchronize-call
	// currnetly do not request, but start record with next gamesync, so network runtime join can be debugged
#ifndef DEBUGREC
	::Control.DoInput(CID_Synchronize, new C4ControlSynchronize(false, true), CDT_Queue);
#endif
	}

bool C4GameControl::IsRuntimeRecordPossible() const
	{
	// already requested?
	if (fRecordNeeded) return false;
	// not from replay
	if (isReplay()) return false;
	// not if recording already
	if (isRecord()) return false;
	// Record OK
	return true;
	}

bool C4GameControl::RecAddFile(const char *szLocalFilename, const char *szAddAs)
	{
	if (!isRecord() || !pRecord) return false;
	return pRecord->AddFile(szLocalFilename, szAddAs);
	}

void C4GameControl::Clear()
{
	StopRecord();
	ChangeToLocal();
  Default();
}

void C4GameControl::Default()
{
	Input.Clear();
  Network.Clear();
	eMode = CM_None;
	fHost = fPreInit = fInitComplete = false;
	iClientID = C4ClientIDUnknown;
	pRecord = NULL;
	pPlayback = NULL;
  SyncChecks.Clear();
	ControlRate = BoundBy<int>(Config.Network.ControlRate, 1, C4MaxControlRate);
  ControlTick = 0;
  SyncRate = C4SyncCheckRate;
  DoSync = false;
	fRecordNeeded = false;
	pExecutingControl = NULL;
}

bool C4GameControl::Prepare()
{
  assert(fInitComplete);

	// Prepare control, return true if everything is ready for GameGo.

	switch(eMode)
	{
	// full steam ahead
	case CM_Local: case CM_Replay:
		return true;

	case CM_Network:

		Network.Execute();

    // deactivated and got control: request activate
    if(Input.firstPkt() && !Game.Clients.getLocal()->isActivated())
      ::Network.RequestActivate();

		// needs input?
		while(Network.CtrlNeeded(Game.FrameCounter))
		{
			Network.DoInput(Input);
			Input.Clear();
		}

		// control tick?
		if(Game.FrameCounter % ControlRate)
			return true;

		// check GameGo
		return Network.CtrlReady(ControlTick);
	}

	return false;
}

void C4GameControl::Execute()
{
	// Execute all available control

  assert(fInitComplete);

	// control tick? replay must always be executed.
	if(!isReplay() && Game.FrameCounter % ControlRate)
		return;

	// Get control
	C4Control Control;
	if(eMode == CM_Local)
	{
		// control = input
		Control.Take(Input);
	}
	if(eMode == CM_Network)
	{
		// control = network input
		if(!Network.GetControl(&Control, ControlTick))
		{
			LogFatal("Network: could not retrieve control from C4GameControlNetwork!");
			return;
		}
	}
	if(eMode == CM_Replay)
	{
		if(!pPlayback) { ChangeToLocal(); return; }
		// control = replay data
		pPlayback->ExecuteControl(&Control, Game.FrameCounter);
	}

	// Record: Save ctrl
	if(pRecord)
		pRecord->Rec(Control, Game.FrameCounter);

	// debug: recheck PreExecute
	assert(Control.PreExecute());

	// execute
	pExecutingControl = &Control;
	Control.Execute();
	Control.Clear();
	pExecutingControl = NULL;

	// statistics record
	if (Game.pNetworkStatistics) Game.pNetworkStatistics->ExecuteControlFrame();

}

void C4GameControl::Ticks()
{
  assert(fInitComplete);

	if(!(Game.FrameCounter % ControlRate))
		ControlTick++;
	if(!(Game.FrameCounter % SyncRate))
		DoSync = true;

	// calc next tick without waiting for timer? (catchup cases)
	if(eMode == CM_Network)
		if(Network.CtrlOverflow(ControlTick))
		{
			Game.GameGo = true;
			if(Network.GetBehind(ControlTick) >= 25)
				if(Game.FrameCounter % ((Network.GetBehind(ControlTick) + 15) / 20))
					Game.DoSkipFrame = true;
		}
}

bool C4GameControl::CtrlTickReached(int32_t iTick)
{
  // 1. control tick reached?
  if(ControlTick < iTick) return false;
	// 2. control tick?
	if(Game.FrameCounter % ControlRate) return false;
  // ok then
  return true;
}

int32_t C4GameControl::getCtrlTick(int32_t iFrame) const
{
	// returns control tick by frame. Note this is a guess, as the control rate
	// can always change, so don't rely on the return value too much.

	return iFrame / ControlRate + ControlTick - Game.FrameCounter / ControlRate;
}

int32_t C4GameControl::getNextControlTick() const
{
	return ControlTick + (Game.FrameCounter % ControlRate ? 1 : 0);
}

void C4GameControl::AdjustControlRate(int32_t iBy)
{
	// control host only
	if(isCtrlHost())
		::Control.DoInput(CID_Set, new C4ControlSet(C4CVT_ControlRate, iBy), CDT_Decide);
}

void C4GameControl::SetActivated(bool fnActivated)
{
	fActivated = fnActivated;
	if(eMode == CM_Network)
		Network.SetActivated(fnActivated);
}

void C4GameControl::DoInput(C4PacketType eCtrlType, C4ControlPacket *pPkt, C4ControlDeliveryType eDelivery)
{
	assert(fPreInit);

  // check if the control can be executed
	if(eDelivery == CDT_Direct || eDelivery == CDT_Private)
		assert(!pPkt->Sync());
	if(!fInitComplete)
		assert(pPkt->Lobby());

	// decide control type
	if(eDelivery == CDT_Decide)
		eDelivery = DecideControlDelivery();

  // queue?
  if(eDelivery == CDT_Queue)
	{
		// add, will be executed/sent later
		Input.Add(eCtrlType, pPkt);
		return;
	}

	// Network?
	if(isNetwork())
	{
		Network.DoInput(eCtrlType, pPkt, eDelivery);
	}
	else
	{
		// Local mode: execute at once
		ExecControlPacket(eCtrlType, pPkt);
		delete pPkt;
	}

}

void C4GameControl::DbgRec(C4RecordChunkType eType, const uint8_t *pData, size_t iSize)
{
#ifdef DEBUGREC
  if (DoNoDebugRec>0) return;
  // record data
  if(pRecord)
    pRecord->Rec(Game.FrameCounter, C4PktDebugRec(eType, StdBuf(pData, iSize)), eType);
	// check against playback
	if (pPlayback)
		pPlayback->Check(eType, pData, iSize);
#endif // DEBUGREC
}

C4ControlDeliveryType C4GameControl::DecideControlDelivery()
{
	// network
	if(eMode == CM_Network)
		return Network.DecideControlDelivery();
	// use direct
	return CDT_Direct;
}

void C4GameControl::DoSyncCheck()
{
  // only once
  if(!DoSync) return;
  DoSync = false;
	// create sync check
	C4ControlSyncCheck *pSyncCheck = new C4ControlSyncCheck();
	pSyncCheck->Set();
	// host?
	if(fHost)
		// add sync check to control queue or send it directly if the queue isn't active
		DoInput(CID_SyncCheck, pSyncCheck, fActivated ? CDT_Queue : CDT_Direct);
	else
		{
		// already have sync check?
		C4ControlSyncCheck* pSyncCheck2 = GetSyncCheck(Game.FrameCounter);
		if(!pSyncCheck2)
			// add to sync check array
			SyncChecks.Add(CID_SyncCheck, pSyncCheck);
		else
			{
			// check
			pSyncCheck->Execute();
			delete pSyncCheck;
			}
		}
	// remove old
	RemoveOldSyncChecks();
}

void C4GameControl::ExecControl(const C4Control &rCtrl)
{
  // nothing to do?
  if(!rCtrl.firstPkt()) return;
  // execute it
  if(!rCtrl.PreExecute()) Log("Control: PreExecute failed for sync control!");
  rCtrl.Execute();
  // record
  if(pRecord)
    pRecord->Rec(rCtrl, Game.FrameCounter);
}

void C4GameControl::ExecControlPacket(C4PacketType eCtrlType, C4ControlPacket *pPkt)
{
	// execute it
	if(!pPkt->PreExecute()) Log("Control: PreExecute failed for direct control!");
	pPkt->Execute();
	// record it
	if(pRecord)
		pRecord->Rec(eCtrlType, pPkt, Game.FrameCounter);
}

C4ControlSyncCheck *C4GameControl::GetSyncCheck(int32_t iTick)
{
	for(C4IDPacket *pPkt = SyncChecks.firstPkt(); pPkt; pPkt = SyncChecks.nextPkt(pPkt))
	{
		// should be a sync check
		if(pPkt->getPktType() != CID_SyncCheck) continue;
		// get sync check
		C4ControlSyncCheck *pCheck = static_cast<C4ControlSyncCheck *>(pPkt->getPkt());
		// packet that's searched for?
		if(pCheck->getFrame() == iTick)
			return pCheck;
	}
	return NULL;
}

void C4GameControl::RemoveOldSyncChecks()
{
	C4IDPacket *pNext;
	for(C4IDPacket *pPkt = SyncChecks.firstPkt(); pPkt; pPkt = pNext)
	{
		pNext = SyncChecks.nextPkt(pPkt);
		// should be a sync check
		if(pPkt->getPktType() != CID_SyncCheck) continue;
		// remove?
		C4ControlSyncCheck *pCheck = static_cast<C4ControlSyncCheck *>(pPkt->getPkt());
		if(pCheck->getFrame() < Game.FrameCounter - C4SyncCheckMaxKeep)
			SyncChecks.Delete(pPkt);
	}
}

C4GameControl Control;
