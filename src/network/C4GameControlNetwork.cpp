/*
 * OpenClonk, http://www.openclonk.org
 *
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
/* control managament: network part */

#include "C4Include.h"
#include "network/C4GameControlNetwork.h"

#include "game/C4Application.h"
#include "control/C4GameControl.h"
#include "game/C4Game.h"
#include "lib/C4Log.h"
#include "game/C4GraphicsSystem.h"

// *** C4GameControlNetwork

C4GameControlNetwork::C4GameControlNetwork(C4GameControl *pnParent)
		: fEnabled(false), fRunning(false), iClientID(C4ClientIDUnknown),
		fActivated(false), iTargetTick(-1),
		iControlPreSend(1), tWaitStart(C4TimeMilliseconds::PositiveInfinity), iAvgControlSendTime(0), iTargetFPS(38),
		iControlSent(0), iControlReady(0),
		pCtrlStack(nullptr),
		tNextControlRequest(0),
		pParent(pnParent)
{
	assert(pParent);
}

C4GameControlNetwork::~C4GameControlNetwork()
{
	Clear();
}

bool C4GameControlNetwork::Init(int32_t inClientID, bool fnHost, int32_t iStartTick, bool fnActivated, C4Network2 *pnNetwork) // by main thread
{
	if (IsEnabled()) Clear();
	// init
	iClientID = inClientID; fHost = fnHost;
	::Control.ControlTick = iStartTick;
	iControlSent = iControlReady = ::Control.getNextControlTick() - 1;
	fActivated = fnActivated;
	pNetwork = pnNetwork;
	// check
	CheckCompleteCtrl(false);
	// make sure no control has been lost
	pNetwork->Clients.BroadcastMsgToConnClients(MkC4NetIOPacket(PID_ControlReq, C4PacketControlReq(iControlReady + 1)));
	// ok
	fEnabled = true; fRunning = false;
	iTargetFPS = 38;
	tNextControlRequest = C4TimeMilliseconds::Now() + C4ControlRequestInterval;
	tWaitStart = C4TimeMilliseconds::PositiveInfinity;
	return true;
}

void C4GameControlNetwork::Clear() // by main thread
{
	fEnabled = false; fRunning = false;
	iAvgControlSendTime = 0;
	ClearCtrl(); ClearClients();
	// clear sync control
	SyncControl.Clear();
	while (pSyncCtrlQueue)
	{
		C4GameControlPacket *pPkt = pSyncCtrlQueue;
		pSyncCtrlQueue = pPkt->pNext;
		delete pPkt;
	}
}

void C4GameControlNetwork::Execute() // by main thread
{
	// Control ticks only
	if (Game.FrameCounter % ::Control.ControlRate)
		return;

	// Save time the control tick was reached
	tWaitStart = C4TimeMilliseconds::Now();

	// Execute any queued sync control
	ExecQueuedSyncCtrl();
}

bool C4GameControlNetwork::CtrlReady(int32_t iTick) // by main thread
{
	// check for complete control and pack it
	CheckCompleteCtrl(false);
	// control ready?
	return iControlReady >= iTick;
}

bool C4GameControlNetwork::GetControl(C4Control *pCtrl, int32_t iTick) // by main thread
{
	// lock
	CStdLock CtrlLock(&CtrlCSec);
	// look for control
	C4GameControlPacket *pPkt = getCtrl(C4ClientIDAll, iTick);
	if (!pPkt)
		return false;
	// set
	pCtrl->Clear();
	pCtrl->Append(pPkt->getControl());
	// calc performance
	CalcPerformance(iTick);
	tWaitStart = C4TimeMilliseconds::PositiveInfinity;
	// ok
	return true;
}

bool C4GameControlNetwork::ClientReady(int32_t iClientID, int32_t iTick) // by main thread
{
	if (eMode == CNM_Central && !fHost) return true;
	return !!getCtrl(iClientID, iTick);
}

int32_t C4GameControlNetwork::ClientPerfStat(int32_t iClientID) // by main thread
{
	if (eMode == CNM_Central && !fHost) return true;
	// get client
	CStdLock ClientsLock(&ClientsCSec);
	C4GameControlClient *pClient = getClient(iClientID);
	// return performance
	return pClient ? pClient->getPerfStat() : 0;
}

int32_t C4GameControlNetwork::ClientNextControl(int32_t iClientID) // by main thread
{
	// get client
	CStdLock ClientsLock(&ClientsCSec);
	C4GameControlClient *pClient = getClient(iClientID);
	// return performance
	return pClient ? pClient->getNextControl() : 0;
}

bool C4GameControlNetwork::CtrlNeeded(int32_t iFrame) const // by main thread
{
	if (!IsEnabled() || !fActivated) return false;
	// check: should we send something at the moment?
	int32_t iSendFor = pParent->getCtrlTick(iFrame + iControlPreSend);
	// target tick set? do special check
	if (iTargetTick >= 0 && iControlSent >= iTargetTick) return false;
	// control sent for this ctrl tick?
	return iSendFor > iControlSent;
}

void C4GameControlNetwork::DoInput(const C4Control &Input) // by main thread
{
	if (!fEnabled) return;
	// pack
	C4GameControlPacket *pCtrl = new C4GameControlPacket();
	pCtrl->Set(iClientID, iControlSent+1, Input);
	// client in central or async mode: send to host (will resend it to the other clients)
	C4NetIOPacket CtrlPkt = MkC4NetIOPacket(PID_Control, *pCtrl);
	if (eMode != CNM_Decentral)
	{
		if (!fHost)
			if (!pNetwork->Clients.SendMsgToHost(CtrlPkt))
				Application.InteractiveThread.ThreadLog("Failed to send control to host!");
	}
	// decentral mode: always broadcast to everybody
	else
	{
		assert (eMode == CNM_Decentral);
		if (!pNetwork->Clients.BroadcastMsgToClients(CtrlPkt))
			Application.InteractiveThread.ThreadLog("Failed to broadcast control!");
	}
	// add to list
	AddCtrl(pCtrl);
	// ok, control is sent for this control tick
	iControlSent++;
	// ctrl complete?
	CheckCompleteCtrl(false);
}

void C4GameControlNetwork::DoInput(C4PacketType eCtrlType, C4ControlPacket *pCtrl, C4ControlDeliveryType eDelivery) // by main thread
{
	if (!fEnabled) return;

	// Create packet
	C4PacketControlPkt CtrlPkt(eDelivery, C4IDPacket(eCtrlType, pCtrl, false));

	switch (eDelivery)
	{

		// Sync control
	case CDT_Sync:
	{
		if (!fHost)
		{
			// Client: send to host
			if (!::Network.Clients.SendMsgToHost(MkC4NetIOPacket(PID_ControlPkt, CtrlPkt)))
				{ LogFatal("Network: could not send direct control packet!"); break; }
			delete pCtrl;
		}
		else
		{
			// Host: send to all clients
			::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_ControlPkt, CtrlPkt));
			// Execute at once, if possible
			if (::Network.isFrozen())
			{
				pParent->ExecControlPacket(eCtrlType, pCtrl);
				delete pCtrl;
				C4PacketExecSyncCtrl Pkt(pParent->ControlTick);
				::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_ExecSyncCtrl, Pkt));
			}
			else
			{
				// Safe back otherwise
				SyncControl.Add(eCtrlType, pCtrl);
				// And sync
				::Network.Sync();
			}
		}
	}
	break;

	// Direct/private control:
	case CDT_Direct:
	case CDT_Private:
	{
		// Send to all clients
		if (!::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_ControlPkt, CtrlPkt)))
			{ LogFatal("Network: could not send direct control packet!"); break; }
		// Exec at once
		pParent->ExecControlPacket(eCtrlType, pCtrl);
		delete pCtrl;
	}
	break;

	// Only some delivery types support single packets (queue control must be tick-stamped)
	default:
		assert(false);

	}

}

C4ControlDeliveryType C4GameControlNetwork::DecideControlDelivery() const
{
	// This doesn't make sense for clients
	if (!fHost)
		return CDT_Queue;
	// Decide the fastest control delivery type atm. Note this is a guess.
	// Control sent with the returned delivery type may in theory be delayed infinitely.
	if (::Network.isFrozen())
		return CDT_Sync;
	if (!Game.Clients.getLocal()->isActivated())
		return CDT_Sync;
	return CDT_Queue;
}

void C4GameControlNetwork::ExecSyncControl() // by main thread
{
	assert(fHost);

	// This is a callback from C4Network informing that a point where accumulated sync control
	// can be executed has been reached (it's "momentarily" safe to execute)

	// Nothing to do? Save some sweat.
	if (!SyncControl.firstPkt())
		return;

	// So let's spread the word, so clients will call ExecSyncControl, too.
	C4PacketExecSyncCtrl Pkt(pParent->ControlTick);
	::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_ExecSyncCtrl, Pkt));

	// Execute it
	ExecSyncControl(Pkt.getControlTick());

}

void C4GameControlNetwork::ExecSyncControl(int32_t iControlTick) // by main thread
{
	// Nothing to do?
	if (!SyncControl.firstPkt())
		return;

	// Copy control and clear
	C4Control Control = SyncControl;
	SyncControl.Clear();

	// Given control tick reached?
	if (pParent->ControlTick == iControlTick)
		pParent->ExecControl(Control);

	else if (pParent->ControlTick > iControlTick)
		// The host should make sure this doesn't happen.
		LogF("Network: Fatal: got sync control to execute for ctrl tick %d, but already in ctrl tick %d!", iControlTick, pParent->ControlTick);

	else
		// This sync control must be executed later, so safe it back
		AddSyncCtrlToQueue(Control, iControlTick);

}

void C4GameControlNetwork::AddClient(int32_t iClientID, const char *szName) // by main thread
{
	// security
	if (!fEnabled || getClient(iClientID)) return;
	// create new
	C4GameControlClient *pClient = new C4GameControlClient();
	pClient->Set(iClientID, szName);
	pClient->SetNextControl(::Control.ControlTick);
	// add client
	AddClient(pClient);
}

void C4GameControlNetwork::ClearClients()
{
	CStdLock ClientsLock(&ClientsCSec);
	while (pClients) { C4GameControlClient *pClient = pClients; RemoveClient(pClient); delete pClient; }
}

void C4GameControlNetwork::CopyClientList(const C4ClientList &rClients)
{
	CStdLock ClientLock(&ClientsCSec);
	// create local copy of activated client list
	ClearClients();
	C4Client *pClient = nullptr;
	while ((pClient = rClients.getClient(pClient)))
		if (pClient->isActivated())
			AddClient(pClient->getID(), pClient->getName());
}

void C4GameControlNetwork::SetRunning(bool fnRunning, int32_t inTargetTick) // by main thread
{
	assert(fEnabled);
	// check for redundant update, stop if running (safety)
	if (fRunning == fnRunning && iTargetTick == inTargetTick) return;
	fRunning = false;
	// set
	iTargetTick = inTargetTick;
	fRunning = fnRunning;
	// run?
	if (fRunning)
	{
		// refresh client list
		CopyClientList(Game.Clients);
		// check for complete ctrl
		CheckCompleteCtrl(false);
	}
}

void C4GameControlNetwork::SetActivated(bool fnActivated) // by main thread
{
	assert(fEnabled);
	// no change? ignore
	if (fActivated == fnActivated) return;
	// set
	fActivated = fnActivated;
	// Activated? Start to send control at next tick
	if (fActivated)
		iControlSent = ::Control.getNextControlTick() - 1;
}

void C4GameControlNetwork::SetCtrlMode(C4GameControlNetworkMode enMode) // by main thread
{
	assert(fEnabled);
	// no change?
	if (eMode == enMode) return;
	// set mode
	eMode = enMode;
	// changed to decentral? rebroadcast all own control
	if (enMode == CNM_Decentral)
	{
		CStdLock CtrlLock(&CtrlCSec); C4GameControlPacket *pPkt;
		for (int32_t iCtrlTick = ::Control.ControlTick; (pPkt = getCtrl(iClientID, iCtrlTick)); iCtrlTick++)
			::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_Control, *pPkt));
	}
	else if (enMode == CNM_Central && fHost)
	{
		CStdLock CtrlLock(&CtrlCSec); C4GameControlPacket *pPkt;
		for (int32_t iCtrlTick = ::Control.ControlTick; (pPkt = getCtrl(C4ClientIDAll, iCtrlTick)); iCtrlTick++)
			::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_Control, *pPkt));
	}
}

void C4GameControlNetwork::CalcPerformance(int32_t iCtrlTick)
{
	CStdLock ControlLock(&CtrlCSec);
	CStdLock ClientLock(&ClientsCSec);
	// should only be called if ready
	assert(CtrlReady(iCtrlTick));
	// calc perfomance for all clients
	int32_t iClientsPing=0; int32_t iPingClientCount=0; int32_t iNumTunnels=0; int32_t iHostPing=0;
	for (C4GameControlClient *pClient = pClients; pClient; pClient = pClient->pNext)
	{
		// Some rudimentary PreSend-calculation
		// get associated connection - nullptr for self
		C4Network2Client *pNetClt = ::Network.Clients.GetClientByID(pClient->getClientID());
		if (pNetClt && !pNetClt->isLocal())
		{
			C4Network2IOConnection *pConn = pNetClt->getMsgConn();
			if (!pConn)
				// remember tunnel
				++iNumTunnels;
			else
				// store ping
				if (pClient->getClientID() == C4ClientIDHost)
					iHostPing = pConn->getPingTime();
				else
				{
					iClientsPing += pConn->getPingTime();
					++iPingClientCount;
				}
		}
		// Performance statistics
		// find control (may not be found, if we only got the complete ctrl)
		C4GameControlPacket *pCtrl = getCtrl(pClient->getClientID(), iCtrlTick);
		if (!pCtrl || tWaitStart.IsInfinite()) continue;
		// calc stats
		pClient->AddPerf(pCtrl->getTime() - tWaitStart);
	}
	// Now do PreSend-calcs based on ping times
	int32_t iControlSendTime;
	if (eMode == CNM_Decentral)
	{
		// average ping time
		iControlSendTime = (iClientsPing + iHostPing * (iNumTunnels + 1)) / (iPingClientCount + iNumTunnels + 1);
		// decentral mode: Only half the ping is used if there are no tunnels
		if (!iNumTunnels) iControlSendTime /= 2;
	}
	else
	{
		// central mode: Control must go to host and back
		iControlSendTime = iHostPing;
	}
	// calc some average
	if (iControlSendTime)
	{
		iAvgControlSendTime = (iAvgControlSendTime * 149 + iControlSendTime * 1000) / 150;
		// now calculate the all-time optimum PreSend there is
		int32_t iBestPreSend = Clamp((iTargetFPS * iAvgControlSendTime) / 1000000 + 1, 1, 15);
		// fixed PreSend?
		if (iTargetFPS <= 0) iBestPreSend = -iTargetFPS;
		// Ha! Set it!
		if (getControlPreSend() != iBestPreSend)
		{
			setControlPreSend(iBestPreSend);
			::GraphicsSystem.FlashMessage(FormatString("PreSend: %d  - TargetFPS: %d", iBestPreSend, iTargetFPS).getData());
		}
	}
}

void C4GameControlNetwork::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn)
{
	// security
	if (!pConn)
		return;

#define GETPKT(type, name) \
    assert(pPacket); const type &name = \
      static_cast<const type &>(*pPacket);

	switch (cStatus)
	{

	case PID_Control: // control
	{
		GETPKT(C4GameControlPacket, rPkt)
		HandleControl(pConn->getClientID(), rPkt);
	}
	break;

	case PID_ControlReq: // control request
	{
		if (!IsEnabled()) break;
		if (pConn->isClosed() || !pConn->isAccepted()) break;
		GETPKT(C4PacketControlReq, rPkt)
		HandleControlReq(rPkt, pConn);
	}
	break;

	case PID_ControlPkt: // single control packet (main thread!)
	{
		GETPKT(C4PacketControlPkt, rPkt)
		// security
		if (!fEnabled) break;
		if (rPkt.getCtrl().getPktType() < CID_First) break;
		// create copy (HandleControlPkt will store or delete)
		C4IDPacket Copy(rPkt.getCtrl());
		// some sanity checks
		C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(Copy.getPkt());
		if (!pConn->isHost() && pConn->getClientID() != pCtrlPkt->getByClient())
			break;
		// handle
		HandleControlPkt(Copy.getPktType(), pCtrlPkt, rPkt.getDelivery());
		Copy.Default();
	}
	break;

	case PID_ExecSyncCtrl:
	{
		GETPKT(C4PacketExecSyncCtrl, rPkt)
		// security
		if (!fEnabled) break;
		// handle
		ExecSyncControl(rPkt.getControlTick());
	}
	break;

	}

#undef GETPKT
}

void C4GameControlNetwork::OnResComplete(C4Network2Res *pRes)
{
	// player?
	if (pRes->getType() == NRT_Player)
		// check for ctrl ready
		CheckCompleteCtrl(true);
}

void C4GameControlNetwork::HandleControl(int32_t iByClientID, const C4GameControlPacket &rPkt)
{
	// already got that control? just ignore
	if (getCtrl(rPkt.getClientID(), rPkt.getCtrlTick())) return;
	// create copy, add to list
	C4GameControlPacket *pCopy = new C4GameControlPacket(rPkt);
	AddCtrl(pCopy);
	// check: control complete?
	if (IsEnabled())
		CheckCompleteCtrl(true);
	// note that C4GameControlNetwork will save incoming control even before
	// Init() was called.
}

void C4GameControlNetwork::HandleControlReq(const C4PacketControlReq &rPkt, C4Network2IOConnection *pConn)
{
	CStdLock CtrlLock(&CtrlCSec);
	for (int iTick = rPkt.getCtrlTick(); ; iTick++)
	{
		// search complete control
		C4GameControlPacket *pCtrl = getCtrl(C4ClientIDAll, iTick);
		if (pCtrl)
		{
			// send
			pConn->Send(MkC4NetIOPacket(PID_Control, *pCtrl));
			continue;
		}
		// send everything we have for this tick (this is an emergency case, so efficiency
		// isn't that important for now).
		bool fFound = false;
		for (pCtrl = pCtrlStack; pCtrl; pCtrl = pCtrl->pNext)
			if (pCtrl->getCtrlTick() == iTick)
			{
				pConn->Send(MkC4NetIOPacket(PID_Control, *pCtrl));
				fFound = true;
			}
		// nothing found for this tick?
		if (!fFound) break;
	}
}

void C4GameControlNetwork::HandleControlPkt(C4PacketType eCtrlType, C4ControlPacket *pCtrl, C4ControlDeliveryType eType) // main thread
{

	// direct control? execute at once
	if (eType == CDT_Direct || eType == CDT_Private)
	{
		pParent->ExecControlPacket(eCtrlType, pCtrl);
		delete pCtrl;
		return;
	}

	// sync ctrl from client? resend
	if (fHost && eType == CDT_Sync)
	{
		DoInput(eCtrlType, pCtrl, eType);
		return;
	}

	// Execute queued control first
	ExecQueuedSyncCtrl();

	// Execute at once, if possible
	if (::Network.isFrozen())
	{
		pParent->ExecControlPacket(eCtrlType, pCtrl);
		delete pCtrl;
	}
	else
	{
		// Safe back otherwise
		SyncControl.Add(eCtrlType, pCtrl);
	}

}

C4GameControlClient *C4GameControlNetwork::getClient(int32_t iID) // by both
{
	CStdLock ClientsLock(&ClientsCSec);
	for (C4GameControlClient *pClient = pClients; pClient; pClient = pClient->pNext)
		if (pClient->getClientID() == iID)
			return pClient;
	return nullptr;
}

void C4GameControlNetwork::AddClient(C4GameControlClient *pClient) // by main thread
{
	if (!pClient) return;
	// lock
	CStdLock ClientsLock(&ClientsCSec);
	// add (ordered)
	C4GameControlClient *pPrev = nullptr, *pPos = pClients;
	for (; pPos; pPrev = pPos, pPos = pPos->pNext)
		if (pPos->getClientID() > pClient->getClientID())
			break;
	// insert
	(pPrev ? pPrev->pNext : pClients) = pClient;
	pClient->pNext = pPos;
}

void C4GameControlNetwork::RemoveClient(C4GameControlClient *pClient) // by main thread
{
	// obtain lock
	CStdLock ClientsLock(&ClientsCSec);
	// first client?
	if (pClient == pClients)
		pClients = pClient->pNext;
	else
	{
		C4GameControlClient *pPrev;
		for (pPrev = pClients; pPrev && pPrev->pNext; pPrev = pPrev->pNext)
			if (pPrev->pNext == pClient)
				break;
		if (pPrev && pPrev->pNext == pClient)
			pPrev->pNext = pClient->pNext;
	}
}

C4GameControlPacket *C4GameControlNetwork::getCtrl(int32_t iClientID, int32_t iCtrlTick) // by both
{
	// lock
	CStdLock CtrlLock(&CtrlCSec);
	// search
	for (C4GameControlPacket *pCtrl = pCtrlStack; pCtrl; pCtrl = pCtrl->pNext)
		if (pCtrl->getClientID() == iClientID && pCtrl->getCtrlTick() == iCtrlTick)
			return pCtrl;
	return nullptr;
}

void C4GameControlNetwork::AddCtrl(C4GameControlPacket *pCtrl) // by both
{
	// lock
	CStdLock CtrlLock(&CtrlCSec);
	// add to list
	pCtrl->pNext = pCtrlStack;
	pCtrlStack = pCtrl;
}

void C4GameControlNetwork::ClearCtrl(int32_t iBeforeTick) // by main thread
{
	// lock
	CStdLock CtrlLock(&CtrlCSec);
	// clear all old control
	C4GameControlPacket *pCtrl = pCtrlStack, *pLast = nullptr;
	while (pCtrl)
	{
		// old?
		if (iBeforeTick == -1 || pCtrl->getCtrlTick() < iBeforeTick)
		{
			// unlink
			C4GameControlPacket *pDelete = pCtrl;
			pCtrl = pCtrl->pNext;
			(pLast ? pLast->pNext : pCtrlStack) = pCtrl;
			// delete
			delete pDelete;
		}
		else
		{
			pLast = pCtrl;
			pCtrl = pCtrl->pNext;
		}
	}
}

void C4GameControlNetwork::CheckCompleteCtrl(bool fSetEvent) // by both
{
	// only when running (client list may be invalid)
	if (!fRunning || !fEnabled) return;

	CStdLock CtrlLock(&CtrlCSec);
	CStdLock ClientLock(&ClientsCSec);

	for (;;)
	{
		// control available?
		C4GameControlPacket *pComplete = getCtrl(C4ClientIDAll, iControlReady + 1);
		// get stop tick
		int32_t iStopTick = iTargetTick;
		if (pSyncCtrlQueue)
			if (iStopTick < 0 || iStopTick > pSyncCtrlQueue->getCtrlTick())
				iStopTick = pSyncCtrlQueue->getCtrlTick();
		// pack control?
		if (!pComplete)
		{
			// own control not ready?
			if (fActivated && iControlSent <= iControlReady) break;
			// no clients? no need to pack more than one tick into the future
			if (!pClients && ::Control.ControlTick <= iControlReady) break;
			// stop packing?
			if (iStopTick >= 0 && iControlReady + 1 >= iStopTick) break;
			// central mode and not host?
			if (eMode != CNM_Decentral && !fHost) break;
			// (try to) pack
			if (!(pComplete = PackCompleteCtrl(iControlReady + 1)))
				break;
		}
		// preexecute to check if it's ready for execute
		if (!pComplete->getControl().PreExecute())
			break;
		// ok, control for this tick is ready
		iControlReady++;
		// tell the main thread to move on
		if (fSetEvent && Game.GameGo && iControlReady >= ::Control.ControlTick)
			Application.NextTick();
	}
	// clear old ctrl
	if (::Control.ControlTick >= C4ControlBacklog)
		ClearCtrl(::Control.ControlTick - C4ControlBacklog);
	// target ctrl tick to reach?
	if (iControlReady < iTargetTick &&
	    (!fActivated || iControlSent > iControlReady) &&
	    C4TimeMilliseconds::Now() >= tNextControlRequest)
	{
		Application.InteractiveThread.ThreadLogS("Network: Recovering: Requesting control for tick %d...", iControlReady + 1);
		// make request
		C4NetIOPacket Pkt = MkC4NetIOPacket(PID_ControlReq, C4PacketControlReq(iControlReady + 1));
		// send control requests
		if (eMode == CNM_Central)
			::Network.Clients.SendMsgToHost(Pkt);
		else
			::Network.Clients.BroadcastMsgToConnClients(Pkt);
		// set time for next request
		tNextControlRequest = C4TimeMilliseconds::Now() + C4ControlRequestInterval;
	}
}

C4GameControlPacket *C4GameControlNetwork::PackCompleteCtrl(int32_t iTick)
{
	CStdLock CtrlLock(&CtrlCSec);
	CStdLock ClientLock(&ClientsCSec);

	// check if ctrl by all clients is ready
	C4GameControlClient *pClient;
	for (pClient = pClients; pClient; pClient = pClient->pNext)
		if (!getCtrl(pClient->getClientID(), iTick))
			break;
	if (pClient)
	{
		// async mode: wait n extra frames for slow clients
		const int iMaxWait = (Config.Network.AsyncMaxWait * 1000) / iTargetFPS;
		if (eMode != CNM_Async || C4TimeMilliseconds::Now() <= tWaitStart + iMaxWait)
			return nullptr;
	}

	// create packet
	C4GameControlPacket *pComplete = new C4GameControlPacket();
	pComplete->Set(C4ClientIDAll, iTick);

	// pack everything in ID order (client list is ordered this way)
	C4GameControlPacket *pCtrl;
	for (pClient = pClients; pClient; pClient = pClient->pNext)
		while (pClient->getNextControl() <= iTick)
		{
			// get control
			int32_t iNextControl = pClient->getNextControl();
			pCtrl = getCtrl(pClient->getClientID(), iNextControl);
			if (!pCtrl) break;
			pClient->SetNextControl(iNextControl + 1);
			assert(pCtrl);
			// add
			pComplete->Add(*pCtrl);
		}

	// add to list
	AddCtrl(pComplete);

	// host: send to clients (central and async mode)
	if (eMode != CNM_Decentral)
		::Network.Clients.BroadcastMsgToConnClients(MkC4NetIOPacket(PID_Control, *pComplete));

	// advance control request time
	tNextControlRequest = std::max(tNextControlRequest, C4TimeMilliseconds::Now() + C4ControlRequestInterval);

	// return
	return pComplete;
}

void C4GameControlNetwork::AddSyncCtrlToQueue(const C4Control &Ctrl, int32_t iTick)  // by main thread
{
	// search place in queue. It's vitally important that new packets are placed
	// behind packets for the same tick, so they will be executed in the right order.
	C4GameControlPacket *pAfter = nullptr, *pBefore = pSyncCtrlQueue;
	while (pBefore && pBefore->getCtrlTick() <= iTick)
		{ pAfter = pBefore; pBefore = pBefore->pNext; }
	// create
	C4GameControlPacket *pnPkt = new C4GameControlPacket();
	pnPkt->Set(C4ClientIDUnknown, iTick, Ctrl);
	// insert
	(pAfter ? pAfter->pNext : pSyncCtrlQueue) = pnPkt;
	pnPkt->pNext = pBefore;
}

void C4GameControlNetwork::ExecQueuedSyncCtrl()  // by main thread
{
	// security
	while (pSyncCtrlQueue && pSyncCtrlQueue->getCtrlTick() < pParent->ControlTick)
	{
		LogF("Network: Fatal: got sync control to execute for ctrl tick %d, but already in ctrl tick %d!", pSyncCtrlQueue->getCtrlTick(), pParent->ControlTick);
		// remove it
		C4GameControlPacket *pPkt = pSyncCtrlQueue;
		pSyncCtrlQueue = pPkt->pNext;
		delete pPkt;
	}
	// nothing to do?
	if (!pSyncCtrlQueue || pSyncCtrlQueue->getCtrlTick() > pParent->ControlTick)
		return;
	// this should hold by now
	assert(pSyncCtrlQueue && pSyncCtrlQueue->getCtrlTick() == pParent->ControlTick);
	do
	{
		// execute it
		pParent->ExecControl(pSyncCtrlQueue->getControl());
		// remove the packet
		C4GameControlPacket *pPkt = pSyncCtrlQueue;
		pSyncCtrlQueue = pPkt->pNext;
		delete pPkt;
	}
	while (pSyncCtrlQueue && pSyncCtrlQueue->getCtrlTick() == pParent->ControlTick);
	// refresh copy of client list
	CopyClientList(Game.Clients);
}

// *** C4GameControlPacket

C4GameControlPacket::C4GameControlPacket()
		: iClientID(C4ClientIDUnknown),
		iCtrlTick(-1),
		tTime(C4TimeMilliseconds::Now()),
		pNext(nullptr)
{

}

C4GameControlPacket::C4GameControlPacket(const C4GameControlPacket &Pkt2)
		: C4PacketBase(Pkt2), iClientID(Pkt2.getClientID()),
		iCtrlTick(Pkt2.getCtrlTick()),
		tTime(C4TimeMilliseconds::Now()),
		pNext(nullptr)
{
	Ctrl.Copy(Pkt2.getControl());
}

C4GameControlPacket &C4GameControlPacket::operator = (const C4GameControlPacket &Pkt2)
{
	Set(Pkt2.getClientID(), Pkt2.getCtrlTick(), Pkt2.getControl());
	return *this;
}

void C4GameControlPacket::Set(int32_t inClientID, int32_t inCtrlTick)
{
	iClientID = inClientID;
	iCtrlTick = inCtrlTick;
}

void C4GameControlPacket::Set(int32_t inClientID, int32_t inCtrlTick, const C4Control &nCtrl)
{
	iClientID = inClientID;
	iCtrlTick = inCtrlTick;
	Ctrl.Copy(nCtrl);
}

void C4GameControlPacket::Add(const C4GameControlPacket &Ctrl2)
{
	Ctrl.Append(Ctrl2.getControl());
}

void C4GameControlPacket::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iClientID), "ClientID", C4ClientIDUnknown));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iCtrlTick), "CtrlTick", -1));
	pComp->Value(mkNamingAdapt(Ctrl, "Ctrl"));
}

// *** C4GameControlClient

C4GameControlClient::C4GameControlClient()
		: iClientID(C4ClientIDUnknown), iNextControl(0), iPerformance(0)
{
	szName[0] = '\0';
}

int32_t C4GameControlClient::getPerfStat() const
{
	return iPerformance / 100;
}

void C4GameControlClient::Set(int32_t inClientID, const char *sznName)
{
	iClientID = inClientID;
	SCopy(sznName, szName, sizeof(szName)-1);
}

void C4GameControlClient::AddPerf(int32_t iTime)
{
	iPerformance += (iTime * 100 - iPerformance) / 100;
}
