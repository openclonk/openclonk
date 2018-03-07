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
#include "C4Include.h"
#include "network/C4Network2IO.h"

#include "control/C4GameControl.h"
#include "game/C4Application.h"
#include "network/C4Network2Discover.h"
#include "network/C4Network2Reference.h"
#include "network/C4Network2UPnP.h"

#ifndef HAVE_WINSOCK
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// internal structures
struct C4Network2IO::NetEvPacketData
{
	C4NetIOPacket Packet;
	C4Network2IOConnection *Conn;
};

// compile options
#define C4NET2IO_DUMP_LEVEL 1

// *** C4Network2IO

C4Network2IO::C4Network2IO()
		: tLastExecute(0), tLastPing(0), tLastStatistic(0) 
{
}

C4Network2IO::~C4Network2IO()
{
	Clear();
}

bool C4Network2IO::Init(int16_t iPortTCP, int16_t iPortUDP, int16_t iPortDiscover, int16_t iPortRefServer, bool fBroadcast, bool enable_upnp) // by main thread
{
	// Already initialized? Clear first
	if (pNetIO_TCP || pNetIO_UDP) Clear();

	// init members
	tLastPing = tLastStatistic = C4TimeMilliseconds::Now();
	iTCPIRate = iTCPORate = iTCPBCRate = 0;
	iUDPIRate = iUDPORate = iUDPBCRate = 0;

	// init event callback
	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.SetCallback(Ev_Net_Conn, this);
	Thread.SetCallback(Ev_Net_Disconn, this);
	Thread.SetCallback(Ev_Net_Packet, this);

	// initialize UPnP manager
	if (enable_upnp && (iPortTCP > 0 || iPortUDP > 0))
	{
		assert(!UPnPMgr);
		UPnPMgr = new C4Network2UPnP;
	}

	// initialize net i/o classes: TCP first
	if (iPortTCP > 0)
	{
		// create
		pNetIO_TCP = new C4NetIOTCP();
		// init
		if (!pNetIO_TCP->Init(iPortTCP))
		{
			LogF("Network: could not init TCP i/o (%s)", pNetIO_TCP->GetError() ? pNetIO_TCP->GetError() : "");
			delete pNetIO_TCP; pNetIO_TCP = nullptr;
		}
		else
			LogSilentF("Network: TCP initialized on port %d", iPortTCP);

		// add to thread, set callback
		if (pNetIO_TCP)
		{
			Thread.AddProc(pNetIO_TCP);
			pNetIO_TCP->SetCallback(this);
			if (UPnPMgr) UPnPMgr->AddMapping(P_TCP, iPortTCP, iPortTCP);
		}

	}
	// then UDP
	if (iPortUDP > 0)
	{
		// create
		pNetIO_UDP = new C4NetIOUDP();
		// init
		if (!pNetIO_UDP->Init(iPortUDP))
		{
			LogF("Network: could not init UDP i/o (%s)", pNetIO_UDP->GetError() ? pNetIO_UDP->GetError() : "");
			delete pNetIO_UDP; pNetIO_UDP = nullptr;
		}
		else
			LogSilentF("Network: UDP initialized on port %d", iPortUDP);

		// add to thread, set callback
		if (pNetIO_UDP)
		{
			Thread.AddProc(pNetIO_UDP);
			pNetIO_UDP->SetCallback(this);
			if (UPnPMgr) UPnPMgr->AddMapping(P_UDP, iPortUDP, iPortUDP);
		}
	}

	// no protocols?
	if (!pNetIO_TCP && !pNetIO_UDP)
	{
		LogFatal("Network: fatal - no protocols available!");
		Thread.ClearCallback(Ev_Net_Conn, this);
		Thread.ClearCallback(Ev_Net_Disconn, this);
		Thread.ClearCallback(Ev_Net_Packet, this);
		return false;
	}

	// discovery last
	if (iPortDiscover > 0)
	{
		// create
		pNetIODiscover = new C4Network2IODiscover(iPortRefServer);
		pNetIODiscover->SetDiscoverable(false);
		// init
		if (!pNetIODiscover->Init(iPortDiscover))
		{
			LogF("Network: could not init discovery (%s)", pNetIODiscover->GetError() ? pNetIODiscover->GetError() : "");
			delete pNetIODiscover; pNetIODiscover = nullptr;
		}
		else
			LogSilentF("Network: discovery initialized on port %d", iPortDiscover);
		// add to thread
		if (pNetIODiscover)
			Thread.AddProc(pNetIODiscover);
	}

	// plus reference server
	if (iPortRefServer > 0)
	{
		// create
		pRefServer = new C4Network2RefServer();
		// init
		if (!pRefServer->Init(iPortRefServer))
		{
			LogF("Network: could not init reference server (%s)", pNetIO_UDP->GetError() ? pNetIO_UDP->GetError() : "");
			delete pRefServer; pRefServer = nullptr;
		}
		else
			LogSilentF("Network: reference server initialized on port %d", iPortRefServer);
		// add to thread
		if (pRefServer)
			Thread.AddProc(pRefServer);
	}

	// own timer
	tLastExecute = C4TimeMilliseconds::Now();
	Thread.AddProc(this);

	// ok
	return true;
}

void C4Network2IO::Clear() // by main thread
{
	// process remaining events
	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.ProcessEvents();
	// clear event callbacks
	Thread.ClearCallback(Ev_Net_Conn, this);
	Thread.ClearCallback(Ev_Net_Disconn, this);
	Thread.ClearCallback(Ev_Net_Packet, this);
	// close all connections
	CStdLock ConnListLock(&ConnListCSec);
	for (C4Network2IOConnection *pConn = pConnList, *pNext; pConn; pConn = pNext)
	{
		pNext = pConn->pNext;
		// close
		pConn->Close();
		RemoveConnection(pConn);
	}
	// reset list
	pConnList = nullptr;
	ConnListLock.Clear();
	// close net i/o classes
	Thread.RemoveProc(this);
	if (pNetIODiscover) { Thread.RemoveProc(pNetIODiscover); delete pNetIODiscover; pNetIODiscover = nullptr; }
	if (pNetIO_TCP) { Thread.RemoveProc(pNetIO_TCP); delete pNetIO_TCP; pNetIO_TCP = nullptr; }
	if (pNetIO_UDP) { Thread.RemoveProc(pNetIO_UDP); delete pNetIO_UDP; pNetIO_UDP = nullptr; }
	if (pRefServer) { Thread.RemoveProc(pRefServer); delete pRefServer; pRefServer = nullptr; }
	if (UPnPMgr) { delete UPnPMgr; UPnPMgr = nullptr; }
	// remove auto-accepts
	ClearAutoAccept();
	// reset flags
	fAllowConnect = fExclusiveConn = false;
	// reset connection ID
	iNextConnID = 0;
}

void C4Network2IO::SetLocalCCore(const C4ClientCore &nCCore)
{
	CStdLock LCCoreLock(&LCCoreCSec);
	LCCore = nCCore;
}

C4NetIO *C4Network2IO::MsgIO() // by both
{
	if (pNetIO_UDP) return pNetIO_UDP;
	if (pNetIO_TCP) return pNetIO_TCP;
	return nullptr;
}

C4NetIO *C4Network2IO::DataIO() // by both
{
	if (pNetIO_TCP) return pNetIO_TCP;
	if (pNetIO_UDP) return pNetIO_UDP;
	return nullptr;
}

bool C4Network2IO::Connect(const C4NetIO::addr_t &addr, C4Network2IOProtocol eProt, const C4ClientCore &nCCore, const char *szPassword) // by main thread
{
    return ConnectWithSocket(addr, eProt, nCCore, nullptr, szPassword);
}


bool C4Network2IO::ConnectWithSocket(const C4NetIO::addr_t &addr, C4Network2IOProtocol eProt, const C4ClientCore &nCCore, std::unique_ptr<C4NetIOTCP::Socket> socket, const char *szPassword) // by main thread
{
	// get network class
	C4NetIO *pNetIO = getNetIO(eProt);
	if (!pNetIO) return false;
	// already connected/connecting?
	if (GetConnectionByConnAddr(addr, pNetIO)) return true;
	// assign new connection ID, peer address isn't known yet
	uint32_t iConnID = iNextConnID++;
	C4NetIO::addr_t paddr;
	// create connection object and add to list
	C4Network2IOConnection *pConn = new C4Network2IOConnection();
	pConn->Set(pNetIO, eProt, paddr, addr, CS_Connect, szPassword, iConnID);
	pConn->SetCCore(nCCore);
	if (socket)
		pConn->SetSocket(std::move(socket));
	AddConnection(pConn);
	// connect
	if (!pConn->Connect())
	{
		// show error
		LogSilentF("Network: could not connect to %s using %s: %s", addr.ToString().getData(),
		     getNetIOName(pNetIO), pNetIO->GetError() ? pNetIO->GetError() : "");
		pNetIO->ResetError();
		// remove class
		RemoveConnection(pConn);
		return false;
	}
	// ok, wait for connection
	return true;
}

void C4Network2IO::SetAcceptMode(bool fnAllowConnect) // by main thread
{
	fAllowConnect = fnAllowConnect;
	// Allow connect? Allow discovery of this host
	if (fAllowConnect)
	{
		if (pNetIODiscover)
		{
			pNetIODiscover->SetDiscoverable(true);
			pNetIODiscover->Announce();
		}
	}
}

void C4Network2IO::SetExclusiveConnMode(bool fnExclusiveConn) // by main thread
{
	if (fExclusiveConn == fnExclusiveConn)
		return;
	// Set flag
	fExclusiveConn = fnExclusiveConn;
	// Allowed? Send all pending welcome packets
	if (!fExclusiveConn)
		SendConnPackets();
}

int C4Network2IO::getConnectionCount()  // by main thread
{
	int iCount = 0;
	CStdLock ConnListLock(&ConnListCSec);
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (!pConn->isClosed())
			iCount++;
	return iCount;
}

void C4Network2IO::ClearAutoAccept() // by main thread
{
	CStdLock AALock(&AutoAcceptCSec);
	// delete
	while (pAutoAcceptList)
	{
		// remove
		AutoAccept *pAcc = pAutoAcceptList;
		pAutoAcceptList = pAcc->Next;
		// delete
		delete pAcc;
	}
}

void C4Network2IO::AddAutoAccept(const C4ClientCore &CCore) // by main thread
{
	CStdLock AALock(&AutoAcceptCSec);
	// create
	AutoAccept *pAcc = new AutoAccept();
	pAcc->CCore = CCore;
	// add
	pAcc->Next = pAutoAcceptList;
	pAutoAcceptList = pAcc;
}

void C4Network2IO::RemoveAutoAccept(const C4ClientCore &CCore) // by main thread
{
	CStdLock AALock(&AutoAcceptCSec);
	// find & remove
	AutoAccept *pAcc = pAutoAcceptList, *pLast = nullptr;
	while (pAcc)
		if (pAcc->CCore.getDiffLevel(CCore) <= C4ClientCoreDL_IDMatch)
		{
			// unlink
			AutoAccept *pDelete = pAcc;
			pAcc = pAcc->Next;
			(pLast ? pLast->Next : pAutoAcceptList) = pAcc;
			// delete
			delete pDelete;
		}
		else
		{
			// next peer
			pLast = pAcc;
			pAcc = pAcc->Next;
		}
}

C4Network2IOConnection *C4Network2IO::GetMsgConnection(int iClientID) // by main thread
{
	CStdLock ConnListLock(&ConnListCSec);
	C4Network2IOConnection *pRes = nullptr;
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isAccepted())
			if (pConn->getClientID() == iClientID)
				if (pConn->getProtocol() == P_UDP || !pRes)
					pRes = pConn;
	// add reference
	if (pRes) pRes->AddRef();
	return pRes;
}

C4Network2IOConnection *C4Network2IO::GetDataConnection(int iClientID) // by main thread
{
	CStdLock ConnListLock(&ConnListCSec);
	C4Network2IOConnection *pRes = nullptr;
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isAccepted())
			if (pConn->getClientID() == iClientID)
				if (pConn->getProtocol() == P_TCP || !pRes)
					pRes = pConn;
	// add reference
	if (pRes) pRes->AddRef();
	return pRes;
}

void C4Network2IO::BeginBroadcast(bool fSelectAll)
{
	// lock
	BroadcastCSec.Enter();
	// reset all broadcast flags
	CStdLock ConnListLock(&ConnListCSec);
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isOpen())
			pConn->SetBroadcastTarget(fSelectAll);
}

void C4Network2IO::EndBroadcast()
{
	// unlock
	BroadcastCSec.Leave();
}

bool C4Network2IO::Broadcast(const C4NetIOPacket &rPkt)
{
	bool fSuccess = true;
	// There is no broadcasting atm, emulate it
	CStdLock ConnListLock(&ConnListCSec);
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isOpen() && pConn->isBroadcastTarget())
			fSuccess &= pConn->Send(rPkt);
	if(!fSuccess)
		Log("Network: Warning! Broadcast failed.");
	return fSuccess;
}

bool C4Network2IO::SendMsgToClient(C4NetIOPacket &rPkt, int iClient) // by both
{
	// find msg connection
	C4Network2IOConnection *pConn = GetMsgConnection(iClient);
	if (!pConn) return false;
	// send
	bool fSuccess = pConn->Send(rPkt);
	pConn->DelRef();
	return fSuccess;
}

bool C4Network2IO::BroadcastMsg(const C4NetIOPacket &rPkt) // by both
{
	// TODO: ugly algorithm. do better

	// begin broadcast
	BeginBroadcast(false);
	// select one connection per reachable client
	CStdLock ConnListLock(&ConnListCSec);
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isAccepted())
		{
			if (pConn->getProtocol() == P_UDP)
				pConn->SetBroadcastTarget(true);
			else if (pConn->getProtocol() == P_TCP)
			{
				C4Network2IOConnection *pConn2 = GetMsgConnection(pConn->getClientID());
				if (pConn == pConn2)
					pConn->SetBroadcastTarget(true);
				pConn2->DelRef();
			}
		}
	// send
	bool fSuccess = Broadcast(rPkt);
	// end broadcast
	EndBroadcast();
	// return
	return fSuccess;
}

bool C4Network2IO::InitPuncher(C4NetIO::addr_t nPuncherAddr)
{
	// UDP must be initialized
	if (!pNetIO_UDP)
		return false;
	// save address
	switch (nPuncherAddr.GetFamily())
	{
	case C4NetIO::HostAddress::IPv4:
	    PuncherAddrIPv4 = nPuncherAddr;
	    break;
	case C4NetIO::HostAddress::IPv6:
	    PuncherAddrIPv6 = nPuncherAddr;
	    break;
	case C4NetIO::HostAddress::UnknownFamily:
	    assert(!"Unexpected address family");
	}
	// let's punch
	return pNetIO_UDP->Connect(nPuncherAddr);
}

void C4Network2IO::Punch(const C4NetIO::addr_t &punchee_addr)
{
	if (!pNetIO_UDP)
		return;
	C4PacketPing PktPeng;
	dynamic_cast<C4NetIOUDP*>(pNetIO_UDP)->SendDirect(MkC4NetIOPacket(PID_Pong, PktPeng, punchee_addr));
}

void C4Network2IO::SendPuncherPacket(const C4NetpuncherPacket& p, C4NetIO::HostAddress::AddressFamily family)
{
	if (!pNetIO_UDP) return;
	if (family == C4NetIO::HostAddress::IPv4 && !PuncherAddrIPv4.IsNull())
		pNetIO_UDP->Send(p.PackTo(PuncherAddrIPv4));
	else if (family == C4NetIO::HostAddress::IPv6 && !PuncherAddrIPv6.IsNull())
		pNetIO_UDP->Send(p.PackTo(PuncherAddrIPv6));
}

bool C4Network2IO::IsPuncherAddr(const C4NetIO::addr_t& addr) const
{
	return (!PuncherAddrIPv4.IsNull() && PuncherAddrIPv4 == addr)
	    || (!PuncherAddrIPv6.IsNull() && PuncherAddrIPv6 == addr);
}

// C4NetIO interface
bool C4Network2IO::OnConn(const C4NetIO::addr_t &PeerAddr, const C4NetIO::addr_t &ConnectAddr, const C4NetIO::addr_t *pOwnAddr, C4NetIO *pNetIO)
{
	// puncher answer?
	if (pNetIO == pNetIO_UDP && IsPuncherAddr(ConnectAddr))
	{
		// got an address?
		if (pOwnAddr)
			::Network.OnPuncherConnect(*pOwnAddr);
		return true;
	}

#if(C4NET2IO_DUMP_LEVEL > 1)
	Application.InteractiveThread.ThreadLogS("OnConn: %s %s",
	           C4TimeMilliseconds::Now().AsString().getData(),
	           getNetIOName(pNetIO));
#endif
	// search connection
	C4Network2IOConnection *pConn = nullptr;
	if (!ConnectAddr.IsNull())
		pConn = GetConnectionByConnAddr(ConnectAddr, pNetIO);
	// not found?
	if (!pConn)
	{
		// allow connect?
		if (!fAllowConnect) return false;
		// create new connection object
		uint32_t iConnID = iNextConnID++;
		pConn = new C4Network2IOConnection();
		pConn->Set(pNetIO, getNetIOProt(pNetIO), PeerAddr, ConnectAddr, CS_Connected, nullptr, iConnID);
		// add to list
		AddConnection(pConn);
	}
	else
	{
		// already closed this connection (attempt)?
		if (pConn->isClosed())
			return false;
		if (!pConn->isOpen())
		{
			// change status
			pConn->SetStatus(CS_Connected);
			pConn->SetPeerAddr(PeerAddr);
		}
	}
	// send welcome packet, if appropriate
	SendConnPackets();
#if(C4NET2IO_DUMP_LEVEL > 0)
	// log
	Application.InteractiveThread.ThreadLogS("Network: got %s connection from %s", getNetIOName(pNetIO), PeerAddr.ToString().getData());
#endif
	// do event (disabled - unused)
	// pConn->AddRef(); PushNetEv(NE_Conn, pConn);
	// ok
	return true;
}

void C4Network2IO::OnDisconn(const C4NetIO::addr_t &addr, C4NetIO *pNetIO, const char *szReason)
{
	// punch?
	if (pNetIO == pNetIO_UDP && IsPuncherAddr(addr))
	{
		if (PuncherAddrIPv4 == addr)
			PuncherAddrIPv4.Clear();
		else
			PuncherAddrIPv6.Clear();
		return;
	}
#if(C4NET2IO_DUMP_LEVEL > 1)
	Application.InteractiveThread.ThreadLogS("OnDisconn: %s %s",
	           C4TimeMilliseconds::Now().AsString().getData(),
	           getNetIOName(pNetIO));
#endif
	// find connection
	C4Network2IOConnection *pConn = GetConnection(addr, pNetIO);
	if (!pConn) pConn = GetConnectionByConnAddr(addr, pNetIO);
	if (!pConn) return;
#if(C4NET2IO_DUMP_LEVEL > 0)
	// log
	Application.InteractiveThread.ThreadLogS("Network: %s connection to %s %s (%s)",
	    getNetIOName(pNetIO), addr.ToString().getData(), pConn->isConnecting() ? "failed" : "closed" , szReason);
#endif
	// already closed? ignore
	if (!pConn->isClosed())
		// not accepted yet? count as connection failure
		pConn->SetStatus(pConn->isHalfAccepted() ? CS_Closed : CS_ConnectFail);
	// keep connection for main thread message
	pConn->AddRef();
	// check for pending welcome packets
	SendConnPackets();
	// signal to main thread
	Application.InteractiveThread.PushEvent(Ev_Net_Disconn, pConn);
	// don't remove connection from list - wait for postmortem or timeout
}

void C4Network2IO::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
#if C4NET2IO_DUMP_LEVEL > 0
	auto tTime = C4TimeMilliseconds::Now();
#endif
#if(C4NET2IO_DUMP_LEVEL > 1)
	Application.InteractiveThread.ThreadLogS("OnPacket: %s status %02x %s",
	           C4TimeMilliseconds::Now().AsString().getData(),
	           rPacket.getStatus(), getNetIOName(pNetIO));
#endif
	if (pNetIO == pNetIO_UDP && IsPuncherAddr(rPacket.getAddr()))
	{
		HandlePuncherPacket(rPacket);
		return;
	}
	if (!rPacket.getSize()) return;
	// find connection
	C4Network2IOConnection *pConn = GetConnection(rPacket.getAddr(), pNetIO);
	if (!pConn)
	{
	    Application.InteractiveThread.ThreadLog("Network: could not find connection for %s packet (status %02x) from %s!", getNetIOName(pNetIO), rPacket.getStatus(), rPacket.getAddr().ToString().getData());
	    return;
	}
#if(C4NET2IO_DUMP_LEVEL > 2)
	uint32_t iFindConnectionBlocked = C4TimeMilliseconds::Now() - tTime;
	if (iFindConnectionBlocked > 100)
		Application.InteractiveThread.ThreadLogS("OnPacket: ... blocked %d ms for finding the connection!", iFindConnectionBlocked);
#endif
	// notify
	pConn->OnPacketReceived(rPacket.getStatus());
	// handle packet
	HandlePacket(rPacket, pConn, true);
	// log time
#if(C4NET2IO_DUMP_LEVEL > 1)
	uint32_t iHandlingBlocked = C4TimeMilliseconds::Now() - tTime;
	if (iHandlingBlocked > 100)
		Application.InteractiveThread.ThreadLogS("OnPacket: ... blocked %d ms for handling!", iHandlingBlocked);
#endif
}

void C4Network2IO::OnError(const char *strError, C4NetIO *pNetIO)
{
	// let's log it
	Application.InteractiveThread.ThreadLog("Network: %s error: %s", getNetIOName(pNetIO), strError);
}

bool C4Network2IO::Execute(int iTimeout, pollfd *)
{
	tLastExecute = C4TimeMilliseconds::Now();

	// check for timeout
	CheckTimeout();

	// ping all open connections
	if (!Inside(tLastPing, tLastExecute - C4NetPingFreq, tLastExecute))
	{
		Ping();
		tLastPing = tLastExecute;
	}

	// do statistics
	if (!Inside(tLastStatistic, tLastExecute - C4NetStatisticsFreq, tLastExecute))
	{
		GenerateStatistics(tLastExecute - tLastStatistic);
		tLastStatistic = tLastExecute;
	}

	// resources
	::Network.ResList.OnTimer();

	// ok
	return true;
}

C4TimeMilliseconds C4Network2IO::GetNextTick(C4TimeMilliseconds tNow)
{
	return tLastExecute + C4NetTimer;
}

void C4Network2IO::OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) // by main thread
{
	switch (eEvent)
	{
	case Ev_Net_Conn: // got a connection
	{
		C4Network2IOConnection *pConn = reinterpret_cast<C4Network2IOConnection *>(pEventData);
		// do callback
		::Network.OnConn(pConn);
		// remove reference
		pConn->DelRef();
	}
	break;

	case Ev_Net_Disconn: // connection closed
	{
		C4Network2IOConnection *pConn = reinterpret_cast<C4Network2IOConnection *>(pEventData);
		assert(pConn->isClosed());
		// do callback
		::Network.OnDisconn(pConn);
		// remove reference
		pConn->DelRef();
	}
	break;

	case Ev_Net_Packet: // got packet
	{
		NetEvPacketData *pData = reinterpret_cast<NetEvPacketData *>(pEventData);
		// handle
		HandlePacket(pData->Packet, pData->Conn, false);
		// clear up
		pData->Conn->DelRef();
		delete pData;
	}
	break;

	default:
		// TODO
		break;
	}
}

C4NetIO *C4Network2IO::getNetIO(C4Network2IOProtocol eProt) // by both
{
	switch (eProt)
	{
	case P_UDP: return pNetIO_UDP;
	case P_TCP: return pNetIO_TCP;
	default: return nullptr;
	}
}

const char *C4Network2IO::getNetIOName(C4NetIO *pNetIO)
{
	if (!pNetIO) return "nullptr";
	if (pNetIO == pNetIO_TCP) return "TCP";
	if (pNetIO == pNetIO_UDP) return "UDP";
	return "UNKNOWN";
}

C4Network2IOProtocol C4Network2IO::getNetIOProt(C4NetIO *pNetIO)
{
	if (!pNetIO) return P_NONE;
	if (pNetIO == pNetIO_TCP) return P_TCP;
	if (pNetIO == pNetIO_UDP) return P_UDP;
	return P_NONE;
}

void C4Network2IO::AddConnection(C4Network2IOConnection *pConn) // by both
{
	CStdLock ConnListLock(&ConnListCSec);
	// add reference
	pConn->AddRef();
	// add to list
	pConn->pNext = pConnList; pConnList = pConn;
}

void C4Network2IO::RemoveConnection(C4Network2IOConnection *pConn) // by both
{
	CStdLock ConnListLock(&ConnListCSec);
	// search & remove
	if (pConnList == pConn)
		pConnList = pConn->pNext;
	else
	{
		C4Network2IOConnection *pAct;
		for (pAct = pConnList; pAct; pAct = pAct->pNext)
			if (pAct->pNext == pConn)
				break;
		if (pAct)
			pAct->pNext = pConn->pNext;
		else
			return;
	}
	// remove reference
	pConn->pNext = nullptr; pConn->DelRef();
}

C4Network2IOConnection *C4Network2IO::GetConnection(const C4NetIO::addr_t &addr, C4NetIO *pNetIO) // by both
{
	CStdLock ConnListLock(&ConnListCSec);
	// search
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->getNetClass() == pNetIO && pConn->getPeerAddr() == addr)
			return pConn;
	return nullptr;
}

C4Network2IOConnection *C4Network2IO::GetConnectionByConnAddr(const C4NetIO::addr_t &addr, C4NetIO *pNetIO) // by both
{
	CStdLock ConnListLock(&ConnListCSec);
	// search
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->getNetClass() == pNetIO && pConn->getConnectAddr() == addr)
			return pConn;
	return nullptr;
}

C4Network2IOConnection *C4Network2IO::GetConnectionByID(uint32_t iConnID) // by thread
{
	CStdLock ConnListLock(&ConnListCSec);
	// search
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->getID() == iConnID)
			return pConn;
	return nullptr;
}

void C4Network2IO::SetReference(C4Network2Reference *pReference)
{
	if (pRefServer)
		pRefServer->SetReference(pReference);
	else
		delete pReference;
}

bool C4Network2IO::IsReferenceNeeded()
{
	return !!pRefServer;
}

bool C4Network2IO::doAutoAccept(const C4ClientCore &CCore, const C4Network2IOConnection &Conn)
{
	CStdLock AALock(&AutoAcceptCSec);
	// check if connection with the given client should be allowed
	for (AutoAccept *pAcc = pAutoAcceptList; pAcc; pAcc = pAcc->Next)
		// core match?
		if (CCore.getDiffLevel(pAcc->CCore) <= C4ClientCoreDL_IDMatch)
		{
			// check: already got another connection for this client? Peer IP must match, then.
			for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
				if (pConn->isAccepted() &&
				    pConn->getCCore().getDiffLevel(CCore) <= C4ClientCoreDL_IDMatch &&
					pConn->getPeerAddr().GetHost() != Conn.getPeerAddr().GetHost())
					return false;
			// not found or IP matches? Let pass
			return true;
		}
	return false;
}

bool C4Network2IO::HandlePacket(const C4NetIOPacket &rPacket, C4Network2IOConnection *pConn, bool fThread)
{
	// security: add connection reference
	if (!pConn) return false; pConn->AddRef();
	
	// accept only PID_Conn and PID_Ping on non-accepted connections
	if(!pConn->isHalfAccepted())
		if(rPacket.getStatus() != PID_Conn && rPacket.getStatus() != PID_Ping && rPacket.getStatus() != PID_ConnRe)
			return false;

	// unpack packet (yet another no-idea-why-it's-needed-cast)
	C4IDPacket Pkt; C4PacketBase &PktB = Pkt;
	try
	{
		PktB.unpack(rPacket);
	}
	catch (StdCompiler::Exception *pExc)
	{
		Application.InteractiveThread.ThreadLog("Network: error: Failed to unpack packet id %02x: %s", rPacket.getStatus(), pExc->Msg.getData());
		delete pExc;
#ifndef _DEBUG
		pConn->Close();
#endif
		return false;
	}

	// dump packet (network thread only)
#if(C4NET2IO_DUMP_LEVEL > 0)
	if (Config.Network.PacketLogging && fThread && Pkt.getPktType() != PID_Ping && Pkt.getPktType() != PID_Pong && Pkt.getPktType() != PID_NetResData)
	{
		// StdStrBuf PacketDump = DecompileToBuf<StdCompilerINIWrite>(mkNamingAdaptrPacket);
		StdStrBuf PacketHeader = FormatString("HandlePacket: %s by %s (%lu bytes, counter %d)",
		                                      C4TimeMilliseconds::Now().AsString().getData(),
		                                      pConn->getPeerAddr().ToString().getData(),
		                                      static_cast<unsigned long>(rPacket.getSize()), pConn->getInPacketCounter());
		StdStrBuf Dump = DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(Pkt, PacketHeader.getData()));
		// Put it directly. The standard functions behind StdBuf.Format seem to choke when you pass them too much data.
		Application.InteractiveThread.PushEvent(Ev_LogSilent, Dump.GrabPointer());
	}
#endif

	// search packet handling data
	bool fSendToMainThread = false, fHandled = false;
	for (const C4PktHandlingData *pHData = PktHandlingData; pHData->ID != PID_None; pHData++)
		if (pHData->ID == rPacket.getStatus())
		{
			// correct thread?
			if (!pHData->ProcByThread == !fThread)
			{
				// connection accepted?
				if (pHData->AcceptedOnly || pConn->isAccepted() || pConn->isClosed())
				{
					fHandled = true;
#if(C4NET2IO_DUMP_LEVEL > 2)
					C4TimeMilliseconds tStart = C4TimeMilliseconds::Now();
#endif

					// call handler(s)
					CallHandlers(pHData->HandlerID, &Pkt, pConn, fThread);

#if(C4NET2IO_DUMP_LEVEL > 2)
					uint32_t iBlockedTime = C4TimeMilliseconds::Now() - tStart;
					if (fThread && iBlockedTime > 100)
					{
						Application.InteractiveThread.ThreadLogS("HandlePacket: ... blocked for %u ms!", iBlockedTime);
					}
#endif

				}
			}
			// transfer to main thread?
			else if (!pHData->ProcByThread && fThread)
			{
				fHandled = true;
				fSendToMainThread = true;
			}
		}

	// send to main thread?
	if (fSendToMainThread)
	{
		// create data
		NetEvPacketData *pEvData = new NetEvPacketData;
		pEvData->Packet.Take(rPacket.Duplicate());
		pEvData->Conn = pConn; pConn->AddRef();
		// trigger event
		if (!Application.InteractiveThread.PushEvent(Ev_Net_Packet, pEvData))
			Application.InteractiveThread.ThreadLogS("...push event ");
	}

	// unhandled?
	if (!fHandled && !pConn->isClosed())
		Application.InteractiveThread.ThreadLog("Network: Unhandled packet (status %02x)", rPacket.getStatus());

	// remove connection reference
	pConn->DelRef();
	return fHandled;
}

void C4Network2IO::CallHandlers(int iHandlerID, const C4IDPacket *pPkt, C4Network2IOConnection *pConn, bool fThread)
{
	// emulate old callbacks
	char cStatus = pPkt->getPktType();
	const C4PacketBase *pPacket = pPkt->getPkt();
	// this class (network thread)
	if (iHandlerID & PH_C4Network2IO)
	{
		assert(fThread);
		HandlePacket(cStatus, pPacket, pConn);
	}
	// main network class (main thread)
	if (iHandlerID & PH_C4Network2)
	{
		assert(!fThread);
		::Network.HandlePacket(cStatus, pPacket, pConn);
	}
	// fullscreen lobby
	if (iHandlerID & PH_C4GUIMainDlg)
	{
		assert(!fThread);
		::Network.HandleLobbyPacket(cStatus, pPacket, pConn);
	}
	// client list class (main thread)
	if (iHandlerID & PH_C4Network2ClientList)
	{
		assert(!fThread);
		::Network.Clients.HandlePacket(cStatus, pPacket, pConn);
	}
	// player list class (main thread)
	if (iHandlerID & PH_C4Network2Players)
	{
		assert(!fThread);
		::Network.Players.HandlePacket(cStatus, pPacket, pConn);
	}
	// resource list class (network thread)
	if (iHandlerID & PH_C4Network2ResList)
	{
		assert(fThread);
		::Network.ResList.HandlePacket(cStatus, pPacket, pConn);
	}
	// network control (mixed)
	if (iHandlerID & PH_C4GameControlNetwork)
	{
		::Control.Network.HandlePacket(cStatus, pPacket, pConn);
	}
}

void C4Network2IO::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn)
{
	// security
	if (!pConn) return;

#define GETPKT(type, name) \
    assert(pPacket); const type &name = \
     static_cast<const type &>(*pPacket);

	switch (cStatus)
	{

	case PID_Conn: // connection request
	{
		if (!pConn->isOpen()) break;
		// get packet
		GETPKT(C4PacketConn, rPkt)
		// set connection ID
		pConn->SetRemoteID(rPkt.getConnID());
		// check auto-accept
		if (doAutoAccept(rPkt.getCCore(), *pConn))
		{
			// send answer back
			C4PacketConnRe pcr(true, false, "auto accept");
			if (!pConn->Send(MkC4NetIOPacket(PID_ConnRe, pcr)))
				pConn->Close();
			// accept
			pConn->SetStatus(CS_HalfAccepted);
			pConn->SetCCore(rPkt.getCCore());
			pConn->SetAutoAccepted();
		}
		// note that this packet will get processed by C4Network2, too (main thread)
	}
	break;

	case PID_ConnRe: // connection request reply
	{
		if (!pConn->isOpen()) break;
		// conn not sent? That's fishy.
		// FIXME: Note this happens if the peer has exclusive connection mode on.
		if (!pConn->isConnSent())
		{
			pConn->Close();
			break;
		}
		// get packet
		GETPKT(C4PacketConnRe, rPkt)
		// auto accept connection
		if (rPkt.isOK())
		{
			if (pConn->isHalfAccepted() && pConn->isAutoAccepted())
				pConn->SetAccepted();
		}
	}
	break;

	case PID_Ping:
	{
		if (!pConn->isOpen()) break;
		GETPKT(C4PacketPing, rPkt)
		// pong
		C4PacketPing PktPong = rPkt;
		pConn->Send(MkC4NetIOPacket(PID_Pong, PktPong));
		// remove received packets from log
		pConn->ClearPacketLog(rPkt.getPacketCounter());
	}
	break;

	case PID_Pong:
	{
		if (!pConn->isOpen()) break;
		GETPKT(C4PacketPing, rPkt);
		// save
		pConn->SetPingTime(rPkt.getTravelTime());
	}
	break;

	case PID_FwdReq:
	{
		GETPKT(C4PacketFwd, rPkt);
		HandleFwdReq(rPkt, pConn);
	}
	break;

	case PID_Fwd:
	{
		GETPKT(C4PacketFwd, rPkt);
		// only received accidently?
		if (!rPkt.DoFwdTo(LCCore.getID())) break;
		// handle
		C4NetIOPacket Packet(rPkt.getData(), pConn->getPeerAddr());
		HandlePacket(Packet, pConn, true);
	}
	break;

	case PID_PostMortem:
	{
		GETPKT(C4PacketPostMortem, rPkt);
		// Get connection
		C4Network2IOConnection *pConn = GetConnectionByID(rPkt.getConnID());
		if (!pConn) return;
		// Handle all packets
		uint32_t iCounter;
		for (iCounter = pConn->getInPacketCounter(); ; iCounter++)
		{
			// Get packet
			const C4NetIOPacket *pPkt = rPkt.getPacket(iCounter);
			if (!pPkt) break;
			// Handle it
			HandlePacket(*pPkt, pConn, true);
		}
		// Log
		if (iCounter > pConn->getInPacketCounter())
			Application.InteractiveThread.ThreadLogS("Network: Recovered %d packets", iCounter - pConn->getInPacketCounter());
		// Remove the connection from our list
		if (!pConn->isClosed())
			pConn->Close();
		RemoveConnection(pConn);
	}
	break;

	}

#undef GETPKT
}

void C4Network2IO::HandleFwdReq(const C4PacketFwd &rFwd, C4Network2IOConnection *pBy)
{
	CStdLock ConnListLock(&ConnListCSec);
	// init packet
	C4PacketFwd nFwd;
	nFwd.SetListType(false);
	// find all clients the message should be forwarded to
	int iClientID; C4Network2IOConnection *pConn;
	for (pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isAccepted())
			if ((iClientID = pConn->getClientID()) >= 0)
				if (iClientID != pBy->getClientID())
					if (rFwd.DoFwdTo(iClientID) && !nFwd.DoFwdTo(iClientID))
						nFwd.AddClient(iClientID);
	// check count (hardcoded: broadcast for > 2 clients)
	if (nFwd.getClientCnt() <= 2)
	{
		C4NetIOPacket Pkt(rFwd.getData(), C4NetIO::addr_t());
		for (int i = 0; i < nFwd.getClientCnt(); i++)
			if ((pConn = GetMsgConnection(nFwd.getClient(i))))
			{
				pConn->Send(Pkt);
				pConn->DelRef();
			}
	}
	else
	{
		// Temporarily unlock connection list for getting broadcast lock
		// (might lead to deathlocks otherwise, as the lock is often taken
		//  in the opposite order)
		ConnListLock.Clear();

		BeginBroadcast();
		nFwd.SetData(rFwd.getData());
		// add all clients
		CStdLock ConnListLock(&ConnListCSec);
		for (int i = 0; i < nFwd.getClientCnt(); i++)
			if ((pConn = GetMsgConnection(nFwd.getClient(i))))
			{
				pConn->SetBroadcastTarget(true);
				pConn->DelRef();
			}
		// broadcast
		Broadcast(MkC4NetIOPacket(PID_Fwd, nFwd));
		EndBroadcast();
	}
	// doing a callback here; don't lock!
	ConnListLock.Clear();
	// forward to self?
	if (rFwd.DoFwdTo(LCCore.getID()))
	{
		C4NetIOPacket Packet(rFwd.getData(), pBy->getPeerAddr());
		HandlePacket(Packet, pBy, true);
	}
}

void C4Network2IO::HandlePuncherPacket(const C4NetIOPacket& rPacket)
{
	auto pkt = C4NetpuncherPacket::Construct(rPacket);
	if (pkt && ::Network.HandlePuncherPacket(move(pkt), rPacket.getAddr().GetFamily()));
	else
	{
		assert(pNetIO_UDP);
		pNetIO_UDP->Close(rPacket.getAddr());
	}
}

bool C4Network2IO::Ping()
{
	bool fSuccess = true;
	// ping all connections
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isOpen())
		{
			C4PacketPing Ping(pConn->getInPacketCounter(), pConn->getOutPacketCounter());
			fSuccess &= pConn->Send(MkC4NetIOPacket(PID_Ping, Ping));
			pConn->OnPing();
		}
	return fSuccess;
}

void C4Network2IO::CheckTimeout()
{
	// acquire lock
	CStdLock ConnListLock(&ConnListCSec);
	// check all connections for timeout (use deletion-safe iteration method just in case)
	for (C4Network2IOConnection *pConn = pConnList, *pNext; pConn; pConn = pNext)
	{
		pNext = pConn->pNext;
		// status timeout
		if (!pConn->isClosed() && !pConn->isAccepted())
			if (difftime(time(nullptr), pConn->getTimestamp()) > C4NetAcceptTimeout)
			{
				Application.InteractiveThread.ThreadLogS("Network: connection accept timeout to %s", pConn->getPeerAddr().ToString().getData());
				pConn->Close();
			}
		// ping timeout
		if (pConn->isAccepted())
			if ((pConn->getLag() != -1 ? pConn->getLag() : 1000 * difftime(time(nullptr), pConn->getTimestamp()))
			    > C4NetPingTimeout)
			{
				Application.InteractiveThread.ThreadLogS("%d %d %d", (int)pConn->getLag(), (int)time(nullptr), (int)pConn->getTimestamp());
				Application.InteractiveThread.ThreadLogS("Network: ping timeout to %s", pConn->getPeerAddr().ToString().getData());
				pConn->Close();
			}
		// delayed connection removal
		if (pConn->isClosed())
			if (difftime(time(nullptr), pConn->getTimestamp()) > C4NetAcceptTimeout)
				RemoveConnection(pConn);
	}
}

void C4Network2IO::GenerateStatistics(int iInterval)
{
	int iTCPIRateSum = 0, iTCPORateSum = 0, iUDPIRateSum = 0, iUDPORateSum = 0;

	// acquire lock, get connection statistics
	CStdLock ConnListLock(&ConnListCSec);
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isOpen())
		{
			bool fTCP = pConn->getNetClass() == pNetIO_TCP;
			pConn->DoStatistics(iInterval, fTCP ? &iTCPIRateSum : &iUDPIRateSum,
			                    fTCP ? &iTCPORateSum : &iUDPORateSum);
		}
	ConnListLock.Clear();

	// get broadcast statistics
	int inTCPBCRate = 0, inUDPBCRate = 0;
	if (pNetIO_TCP) pNetIO_TCP->GetStatistic(&inTCPBCRate);
	if (pNetIO_UDP) pNetIO_UDP->GetStatistic(&inUDPBCRate);

	// normalize everything
	iTCPIRateSum = iTCPIRateSum * 1000 / iInterval;
	iTCPORateSum = iTCPORateSum * 1000 / iInterval;
	iUDPIRateSum = iUDPIRateSum * 1000 / iInterval;
	iUDPORateSum = iUDPORateSum * 1000 / iInterval;
	inTCPBCRate = inTCPBCRate * 1000 / iInterval;
	inUDPBCRate = inUDPBCRate * 1000 / iInterval;

	// clear
	if (pNetIO_TCP) pNetIO_TCP->ClearStatistic();
	if (pNetIO_UDP) pNetIO_UDP->ClearStatistic();

	// save back
	iTCPIRate = iTCPIRateSum; iTCPORate = iTCPORateSum; iTCPBCRate = inTCPBCRate;
	iUDPIRate = iUDPIRateSum; iUDPORate = iUDPORateSum; iUDPBCRate = inUDPBCRate;
}

void C4Network2IO::SendConnPackets()
{
	CStdLock ConnListLock(&ConnListCSec);

	// exlusive conn?
	if (fExclusiveConn)
		// find a live connection
		for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
			if (pConn->isAccepted() || (!pConn->isClosed() && pConn->isConnSent()))
				// do not sent additional conn packets - no other connection should succeed
				return;

	// sent pending welcome packet(s)
	for (C4Network2IOConnection *pConn = pConnList; pConn; pConn = pConn->pNext)
		if (pConn->isOpen() && !pConn->isConnSent())
		{
			// make packet
			CStdLock LCCoreLock(&LCCoreCSec);
			C4NetIOPacket Pkt = MkC4NetIOPacket(PID_Conn, C4PacketConn(LCCore, pConn->getID(), pConn->getPassword()));
			LCCoreLock.Clear();
			// send
			if (!pConn->Send(Pkt))
				pConn->Close();
			else
			{
				// set flag
				pConn->SetConnSent();
				// only one conn packet at a time
				if (fExclusiveConn)
					return;
			}
		}

}

// *** C4Network2IOConnection

C4Network2IOConnection::C4Network2IOConnection()
		: iID(~0), iRemoteID(~0),
		
		tLastPing(C4TimeMilliseconds::NegativeInfinity),
		tLastPong(C4TimeMilliseconds::NegativeInfinity),
		
		iRefCnt(0)
{
}

C4Network2IOConnection::~C4Network2IOConnection()
{
	assert(!iRefCnt);
	// connection needs to be closed?
	if (pNetClass && !isClosed()) Close();
	// clear the packet log
	ClearPacketLog();
}

int C4Network2IOConnection::getLag() const
{
	if (iPingTime != -1)
	{
		// Last ping not answered yet?
		if(tLastPing > tLastPong)
		{
			int iPingLag = C4TimeMilliseconds::Now() - tLastPing;
			// Use it for lag measurement once it's larger then the last ping time
			// (the ping time won't be better than this anyway once the pong's here)
			return std::max(iPingLag, iPingTime);
		}
	}
	// Last ping result
	return iPingTime;
}

void C4Network2IOConnection::Set(C4NetIO *pnNetClass, C4Network2IOProtocol enProt, const C4NetIO::addr_t &nPeerAddr, const C4NetIO::addr_t &nConnectAddr, C4Network2IOConnStatus nStatus, const char *szPassword, uint32_t inID)
{
	// save data
	pNetClass = pnNetClass; eProt = enProt;
	PeerAddr = nPeerAddr; ConnectAddr = nConnectAddr;
	Status = nStatus;
	Password = szPassword;
	iID = inID;
	// initialize
	fBroadcastTarget = false;
	iTimestamp = time(nullptr); iPingTime = -1;
}

void C4Network2IOConnection::SetSocket(std::unique_ptr<C4NetIOTCP::Socket> socket)
{
	TcpSimOpenSocket = std::move(socket);
}

void C4Network2IOConnection::SetRemoteID(uint32_t inRemoteID)
{
	iRemoteID = inRemoteID;
}

void C4Network2IOConnection::SetPeerAddr(const C4NetIO::addr_t &nPeerAddr)
{
	// just do it
	PeerAddr = nPeerAddr;
}

void C4Network2IOConnection::OnPing()
{
	// Still no pong for the last ping?
	if (tLastPong < tLastPing)
		return;

	// Save time
	tLastPing = C4TimeMilliseconds::Now();
}

void C4Network2IOConnection::SetPingTime(int inPingTime)
{
	// save it
	iPingTime = inPingTime;
	// pong received - save timestamp
	tLastPong = C4TimeMilliseconds::Now();
}

void C4Network2IOConnection::SetStatus(C4Network2IOConnStatus nStatus)
{
	if (nStatus != Status)
	{
		// Connection can't return from these
		assert(!isClosed());
		// set status
		Status = nStatus;
		// reset timestamp for connect/accept/close
		if (Status == CS_Connect || Status == CS_Connected || Status == CS_Accepted || Status == CS_Closed)
			iTimestamp = time(nullptr);
	}
}

void C4Network2IOConnection::SetAutoAccepted()
{
	fAutoAccept = true;
}

void C4Network2IOConnection::OnPacketReceived(uint8_t iPacketType)
{
	// Just count them
	if (iPacketType >= PID_PacketLogStart)
		iInPacketCounter++;
}

void C4Network2IOConnection::ClearPacketLog(uint32_t iUntilID)
{
	// Search position of first packet to delete
	PacketLogEntry *pPos, *pPrev = nullptr;
	for (pPos = pPacketLog; pPos; pPrev = pPos, pPos = pPos->Next)
		if (pPos->Number < iUntilID)
			break;
	if (pPos)
	{
		// Remove packets from list
		(pPrev ? pPrev->Next : pPacketLog) = nullptr;
		// Delete everything
		while (pPos)
		{
			PacketLogEntry *pDelete = pPos;
			pPos = pPos->Next;
			delete pDelete;
		}
	}
}

bool C4Network2IOConnection::CreatePostMortem(C4PacketPostMortem *pPkt)
{
	// Security
	if (!pPkt) return false;
	CStdLock PacketLogLock(&PacketLogCSec);
	// Nothing to do?
	if (!pPacketLog) return false;
	// Already created?
	if (fPostMortemSent) return false;
	// Set connection ID and packet counter
	pPkt->SetConnID(iRemoteID);
	pPkt->SetPacketCounter(iOutPacketCounter);
	// Add packets
	for (PacketLogEntry *pEntry = pPacketLog; pEntry; pEntry = pEntry->Next)
		pPkt->Add(pEntry->Pkt);
	// Okay
	fPostMortemSent = true;
	return true;
}

void C4Network2IOConnection::SetCCore(const C4ClientCore &nCCore)
{
	CStdLock CCoreLock(&CCoreCSec);
	CCore = nCCore;
}

bool C4Network2IOConnection::Connect()
{
	if (!pNetClass) return false;
	if (TcpSimOpenSocket)
	{
		auto pNetTCP = dynamic_cast<C4NetIOTCP*>(pNetClass);
		return pNetTCP->Connect(ConnectAddr, std::move(TcpSimOpenSocket));
	}
	// try connect
	return pNetClass->Connect(ConnectAddr);
}

void C4Network2IOConnection::Close()
{
	if (!pNetClass || isClosed()) return;
	// set status
	SetStatus(CS_Closed);
	// close
	pNetClass->Close(PeerAddr);
}

bool C4Network2IOConnection::Send(const C4NetIOPacket &rPkt)
{
	// some packets shouldn't go into the log
	if (rPkt.getStatus() < PID_PacketLogStart)
	{
		assert(isOpen());
		C4NetIOPacket Copy(rPkt);
		Copy.SetAddr(PeerAddr);
		return pNetClass->Send(Copy);
	}
	CStdLock PacketLogLock(&PacketLogCSec);
	// create log entry
	PacketLogEntry *pLogEntry = new PacketLogEntry();
	pLogEntry->Number = iOutPacketCounter++;
	pLogEntry->Pkt = rPkt;
	pLogEntry->Next = pPacketLog;
	pPacketLog = pLogEntry;
	// set address
	pLogEntry->Pkt.SetAddr(PeerAddr);
	// closed? No sweat, post mortem will reroute it later.
	if (!isOpen())
	{
		// post mortem already sent? This shouldn't happen
		if (fPostMortemSent) { assert(false); return false; }
		// okay then
		return true;
	}
	// send
	bool fSuccess = pNetClass->Send(pLogEntry->Pkt);
	if (fSuccess)
		assert(!fPostMortemSent);
	else {
		// Not being able to send a packet is actually a big deal,
		// as this means that we will have hole in the packet
		// order. Better close the connection - post mortem should
		// ideally sort everything out from here.
		LogF("Network: Fatal: Send failed (%s)", pNetClass->GetError());
		pNetClass->ResetError();
		Close();
	}
	return fSuccess;
}

void C4Network2IOConnection::SetBroadcastTarget(bool fSet)
{
	// Note that each thread will have to make sure that this flag won't be
	// changed until Broadcast() is called. See C4Network2IO::BroadcastCSec.
	pNetClass->SetBroadcast(PeerAddr, fSet);
	fBroadcastTarget = fSet;
}

void C4Network2IOConnection::DoStatistics(int iInterval, int *pIRateSum, int *pORateSum)
{
	// get C4NetIO statistics
	int inIRate, inORate, inLoss;
	if (!isOpen() || !pNetClass->GetConnStatistic(PeerAddr, &inIRate, &inORate, &inLoss))
	{
		iIRate = iORate = iPacketLoss = 0;
		return;
	}
	// normalize
	inIRate = inIRate * 1000 / iInterval;
	inORate = inORate * 1000 / iInterval;
	// set
	iIRate = inIRate; iORate = inORate; iPacketLoss = inLoss;
	// sum up
	if (pIRateSum) *pIRateSum += iIRate;
	if (pORateSum) *pORateSum += iORate;
}

void C4Network2IOConnection::AddRef()
{
	++iRefCnt;
}

void C4Network2IOConnection::DelRef()
{
	if (--iRefCnt == 0)
		delete this;
}


// *** C4PacketPostMortem

C4PacketPostMortem::C4PacketPostMortem()
		: iConnID(~0),
		iPacketCounter(~0)
{

}

C4PacketPostMortem::~C4PacketPostMortem()
{
	while (pPackets)
	{
		PacketLink *pDelete = pPackets;
		pPackets = pPackets->Next;
		delete pDelete;
	}
	iPacketCount = 0;
}

const C4NetIOPacket *C4PacketPostMortem::getPacket(uint32_t iNumber) const
{
	// Security
	if (!Inside(iNumber, iPacketCounter - iPacketCount, iPacketCounter - 1))
		return nullptr;
	// Calculate position in list
	iNumber = iNumber + iPacketCount - iPacketCounter;
	// Search for the packet with the given number
	PacketLink *pLink = pPackets;
	for (; pLink && iNumber; iNumber--)
		pLink = pLink->Next;
	// Not found?
	return pLink ? &pLink->Pkt : nullptr;
}

void C4PacketPostMortem::SetPacketCounter(uint32_t inPacketCounter)
{
	iPacketCounter = inPacketCounter;
}

void C4PacketPostMortem::Add(const C4NetIOPacket &rPkt)
{
	// Add to head of list (reverse order)
	PacketLink *pLink = new PacketLink();
	pLink->Pkt = rPkt;
	pLink->Next = pPackets;
	pPackets = pLink;
	iPacketCount++;
}

void C4PacketPostMortem::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();

	// Connection ID, packet number and packet count
	pComp->Value(mkNamingAdapt(iConnID, "ConnID"));
	pComp->Value(mkNamingAdapt(iPacketCounter, "PacketCounter"));
	pComp->Value(mkNamingAdapt(iPacketCount, "PacketCount"));

	// Packets
	if (deserializing)
	{
		// Read packets
		for (uint32_t i = 0; i < iPacketCount; i++)
		{
			// Create list entry
			PacketLink *pLink = new PacketLink();
			pLink->Next = pPackets;
			pPackets = pLink;
			// Compile data
			pComp->Value(mkNamingAdapt(pLink->Pkt, "PacketData"));
		}
		// Reverse order
		PacketLink *pPackets2 = pPackets;
		pPackets = nullptr;
		while (pPackets2)
		{
			// Get link
			PacketLink *pLink = pPackets2;
			pPackets2 = pLink->Next;
			// Readd to list
			pLink->Next = pPackets;
			pPackets = pLink;
		}
	}
	else
	{
		// Write packets
		for (PacketLink *pLink = pPackets; pLink; pLink = pLink->Next)
			pComp->Value(mkNamingAdapt(pLink->Pkt, "PacketData"));
	}
}
