/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
#include "network/C4Network2Client.h"

#include "lib/C4Log.h"
#include "editor/C4Console.h"
#include "network/C4Network2.h"
#include "network/C4Network2IO.h"
#include "network/C4Network2Stats.h"
#include "gui/C4GameLobby.h" // fullscreen network lobby
#include "game/C4Game.h"
#include "player/C4PlayerList.h"

// *** C4Network2Client

C4Network2Client::C4Network2Client(C4Client *pClient)
		: pClient(pClient),
		iAddrCnt(0),
		eStatus(NCS_Ready),
		iLastActivity(0),
		pMsgConn(nullptr), pDataConn(nullptr),
		iNextConnAttempt(0),
		pNext(nullptr), pParent(nullptr), pstatPing(nullptr)
{
}

C4Network2Client::~C4Network2Client()
{
	ClearGraphs();
	if (pMsgConn) { pMsgConn->Close(); pMsgConn->DelRef(); } pMsgConn = nullptr;
	if (pDataConn) { pDataConn->Close(); pDataConn->DelRef(); } pDataConn = nullptr;
	if (pClient) pClient->UnlinkNetClient();
}

bool C4Network2Client::hasConn(C4Network2IOConnection *pConn)
{
	return pMsgConn == pConn || pDataConn == pConn;
}

void C4Network2Client::SetMsgConn(C4Network2IOConnection *pConn)
{
	// security
	if (pConn != pMsgConn)
	{
		if (pMsgConn) pMsgConn->DelRef();
		pMsgConn = pConn;
		pMsgConn->AddRef();
	}
	if (!pDataConn) SetDataConn(pConn);
}

void C4Network2Client::SetDataConn(C4Network2IOConnection *pConn)
{
	// security
	if (pConn != pDataConn)
	{
		if (pDataConn) pDataConn->DelRef();
		pDataConn = pConn;
		pDataConn->AddRef();
	}
	if (!pMsgConn) SetMsgConn(pConn);
}

void C4Network2Client::RemoveConn(C4Network2IOConnection *pConn)
{
	if (pConn == pMsgConn)
		{ pMsgConn->DelRef(); pMsgConn = nullptr; }
	if (pConn == pDataConn)
		{ pDataConn->DelRef(); pDataConn = nullptr; }
	if (pMsgConn && !pDataConn) SetDataConn(pMsgConn);
	if (!pMsgConn && pDataConn) SetMsgConn(pDataConn);
}


void C4Network2Client::CloseConns(const char *szMsg)
{
	C4PacketConnRe Pkt(false, false, szMsg);
	C4Network2IOConnection *pConn;
	while ((pConn = pMsgConn))
	{
		// send packet, close
		if (pConn->isOpen())
		{
			pConn->Send(MkC4NetIOPacket(PID_ConnRe, Pkt));
			pConn->Close();
		}
		// remove
		RemoveConn(pConn);
	}
}

bool C4Network2Client::SendMsg(C4NetIOPacket rPkt) const
{
	return getMsgConn() && getMsgConn()->Send(rPkt);
}

bool C4Network2Client::SendData(C4NetIOPacket rPkt) const
{
	return getDataConn() && getDataConn()->Send(rPkt);
}

bool C4Network2Client::DoConnectAttempt(C4Network2IO *pIO)
{
	// local?
	if (isLocal()) { iNextConnAttempt = 0; return true; }
	// msg and data connected? Nothing to do
	if (getMsgConn() != getDataConn()) { iNextConnAttempt = time(nullptr) + 10; return true; }
	// too early?
	if (iNextConnAttempt && iNextConnAttempt > time(nullptr)) return true;
	// find address to try
	int32_t iBestAddress = -1;
	for (int32_t i = 0; i < iAddrCnt; i++)
		// no connection for this protocol?
		if ((!pDataConn || Addr[i].getProtocol() != pDataConn->getProtocol()) &&
		    (!pMsgConn  || Addr[i].getProtocol() != pMsgConn->getProtocol()))
			// protocol available?
			if (pIO->getNetIO(Addr[i].getProtocol()))
				// new best address?
				if (iBestAddress < 0 || AddrAttempts[i] < AddrAttempts[iBestAddress])
					iBestAddress = i;
	// too many attempts or nothing found?
	if (iBestAddress < 0 || AddrAttempts[iBestAddress] > C4NetClientConnectAttempts)
		{ iNextConnAttempt = time(nullptr) + 10; return true; }
	// save attempt
	AddrAttempts[iBestAddress]++; iNextConnAttempt = time(nullptr) + C4NetClientConnectInterval;
	auto addr = Addr[iBestAddress].getAddr();
	std::set<int> interfaceIDs;
	if (addr.IsLocal())
	    interfaceIDs = Network.Clients.GetLocal()->getInterfaceIDs();
	else
	    interfaceIDs = {0};
	for (auto id : interfaceIDs)
	{
	    addr.SetScopeId(id);
	    // log
	    LogSilentF("Network: connecting client %s on %s...", getName(), addr.ToString().getData());
	    // connect
	    if (pIO->Connect(addr, Addr[iBestAddress].getProtocol(), pClient->getCore()))
		return true;
	}
	return false;
}

bool C4Network2Client::hasAddr(const C4Network2Address &addr) const
{
	// Note that the host only knows its own address as 0.0.0.0, so if the real address is being added, that can't be sorted out.
	for (int32_t i = 0; i < iAddrCnt; i++)
		if (Addr[i] == addr)
			return true;
	return false;
}

void C4Network2Client::ClearAddr()
{
	iAddrCnt = 0;
}

bool C4Network2Client::AddAddr(const C4Network2Address &addr, bool fAnnounce)
{
	// checks
	if (iAddrCnt + 1 >= C4ClientMaxAddr) return false;
	if (hasAddr(addr)) return true;
	// add
	Addr[iAddrCnt] = addr; AddrAttempts[iAddrCnt] = 0;
	iAddrCnt++;
	// attempt to use this one
	if (!iNextConnAttempt) iNextConnAttempt = time(nullptr);
	// announce
	if (fAnnounce)
		if (!pParent->BroadcastMsgToConnClients(MkC4NetIOPacket(PID_Addr, C4PacketAddr(getID(), addr))))
			return false;
	// done
	return true;
}

void C4Network2Client::AddLocalAddrs(int16_t iPortTCP, int16_t iPortUDP)
{
	C4NetIO::addr_t addr;

	for (auto& ha : C4NetIO::GetLocalAddresses())
	{
		addr.SetAddress(ha);
		if (iPortTCP)
		{
			addr.SetPort(iPortTCP);
			AddAddr(C4Network2Address(addr, P_TCP), false);
		}
		if (iPortUDP)
		{
			addr.SetPort(iPortUDP);
			AddAddr(C4Network2Address(addr, P_UDP), false);
		}
		if (addr.GetScopeId())
			InterfaceIDs.insert(addr.GetScopeId());
	}
}

void C4Network2Client::SendAddresses(C4Network2IOConnection *pConn)
{
	// send all addresses
	for (int32_t i = 0; i < iAddrCnt; i++)
	{
		if (Addr[i].getAddr().GetScopeId() && (!pConn || pConn->getPeerAddr().GetScopeId() != Addr[i].getAddr().GetScopeId()))
			continue;
		C4Network2Address addr(Addr[i]);
		addr.getAddr().SetScopeId(0);
		C4NetIOPacket Pkt = MkC4NetIOPacket(PID_Addr, C4PacketAddr(getID(), addr));
		if (pConn)
			pConn->Send(Pkt);
		else
			pParent->BroadcastMsgToConnClients(Pkt);
	}

}

void C4Network2Client::CreateGraphs()
{
	// del prev
	ClearGraphs();
	// get client color
	static const DWORD ClientDefColors[] = {0xff0000, 0x00ff00, 0xffff00, 0x7f7fff, 0xffffff, 0x00ffff, 0xff00ff, 0x7f7f7f, 0xff7f7f, 0x7fff7f, 0x0000ff};
	int32_t iClientColorNum = sizeof(ClientDefColors)/sizeof(DWORD);
	DWORD dwClientClr = ClientDefColors[std::max<int32_t>(getID(), 0) % iClientColorNum];
	// create graphs
	pstatPing = new C4TableGraph(C4TableGraph::DefaultBlockLength, Game.pNetworkStatistics ? Game.pNetworkStatistics->SecondCounter : 0);
	pstatPing->SetColorDw(dwClientClr);
	pstatPing->SetTitle(getName());
	// register into stat module
	if (Game.pNetworkStatistics) Game.pNetworkStatistics->statPings.AddGraph(pstatPing);
}

void C4Network2Client::ClearGraphs()
{
	// del all assigned graphs
	if (pstatPing)
	{
		if (Game.pNetworkStatistics) Game.pNetworkStatistics->statPings.RemoveGraph(pstatPing);
		delete pstatPing;
		pstatPing = nullptr;
	}
}

// *** C4Network2ClientList

C4Network2ClientList::C4Network2ClientList(C4Network2IO *pIO)
		: pIO(pIO), pFirst(nullptr), pLocal(nullptr)
{

}

C4Network2ClientList::~C4Network2ClientList()
{
	Clear();
}

C4Network2Client *C4Network2ClientList::GetClientByID(int32_t iID) const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->getID() == iID)
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetNextClientAfterID(int32_t iSmallerClientID) const
{
	// return client with smallest ID > iSmallerClientID
	C4Network2Client *pBest = nullptr;
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->getID() > iSmallerClientID)
			if (!pBest || pBest->getID() > pClient->getID())
				pBest = pClient;
	return pBest;
}

C4Network2Client *C4Network2ClientList::GetClient(const char *szName) const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (SEqual(pClient->getName(), szName))
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetClient(C4Network2IOConnection *pConn) const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->hasConn(pConn))
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetClient(const C4ClientCore &CCore, int32_t iMaxDiffLevel)
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->getCore().getDiffLevel(CCore) <= iMaxDiffLevel)
			return pClient;
	return nullptr;
}

unsigned int C4Network2ClientList::Count()
{
	unsigned int ret(0);
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext) ret++;
	return ret;
}

C4Network2Client *C4Network2ClientList::GetHost()
{
	return GetClientByID(C4ClientIDHost);
}

C4Network2Client *C4Network2ClientList::GetNextClient(C4Network2Client *pClient)
{
	return pClient ? pClient->pNext : pFirst;
}

void C4Network2ClientList::Init(C4ClientList *pnClientList, bool fnHost)
{
	// save flag
	fHost = fnHost;
	// initialize
	pClientList = pnClientList;
	pClientList->InitNetwork(this);
}

C4Network2Client *C4Network2ClientList::RegClient(C4Client *pClient)
{
	// security
	if (pClient->getNetClient())
		return pClient->getNetClient();
	// find insert position
	C4Network2Client *pPos = pFirst, *pLast = nullptr;
	for (; pPos; pLast = pPos, pPos = pPos->getNext())
		if (pPos->getID() > pClient->getID())
			break;
	assert(!pLast || pLast->getID() != pClient->getID());
	// create new client
	C4Network2Client *pNetClient = new C4Network2Client(pClient);
	// add to list
	pNetClient->pNext = pPos;
	(pLast ? pLast->pNext : pFirst) = pNetClient;
	pNetClient->pParent = this;
	// local?
	if (pClient->isLocal())
		pLocal = pNetClient;
	else
		// set auto-accept
		pIO->AddAutoAccept(pClient->getCore());
	// add
	return pNetClient;
}

void C4Network2ClientList::DeleteClient(C4Network2Client *pClient)
{
	// close connections
	pClient->CloseConns("removing client");
	// remove from list
	if (pClient == pFirst)
		pFirst = pClient->getNext();
	else
	{
		C4Network2Client *pPrev;
		for (pPrev = pFirst; pPrev && pPrev->getNext(); pPrev = pPrev->getNext())
			if (pPrev->getNext() == pClient)
				break;
		if (pPrev && pPrev->getNext() == pClient)
			pPrev->pNext = pClient->getNext();
	}
	// remove auto-accept
	pIO->RemoveAutoAccept(pClient->getCore());
	// delete
	delete pClient;
}

void C4Network2ClientList::Clear()
{
	// remove link to main client list
	if (pClientList)
	{
		C4ClientList *poClientList = pClientList;
		pClientList = nullptr;
		poClientList->ClearNetwork();
	}
	// delete clients
	while (pFirst)
	{
		DeleteClient(pFirst);
	}
	pLocal = nullptr;
}

bool C4Network2ClientList::BroadcastMsgToConnClients(const C4NetIOPacket &rPkt)
{
	// Send a msg to all clients that are currently directly reachable.

	// lock
	pIO->BeginBroadcast(false);
	// select connections for broadcast
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (pClient->isConnected())
			pClient->getMsgConn()->SetBroadcastTarget(true);
	// broadcast
	bool fSuccess = pIO->Broadcast(rPkt);
	// unlock
	pIO->EndBroadcast();
	// finished
	return fSuccess;
}

bool C4Network2ClientList::BroadcastMsgToClients(const C4NetIOPacket &rPkt)
{
	// Send a msg to all clients, including clients that are not connected to
	// this computer (will get forwarded by host).

	C4PacketFwd Fwd; Fwd.SetListType(true);
	// lock
	pIO->BeginBroadcast(false);
	// select connections for broadcast
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (!pClient->isHost())
			if (pClient->isConnected())
			{
				pClient->getMsgConn()->SetBroadcastTarget(true);
				Fwd.AddClient(pClient->getID());
			}
	// broadcast
	bool fSuccess = pIO->Broadcast(rPkt);
	// unlock
	pIO->EndBroadcast();
	// clients: send forward request to host
	if (!fHost)
	{
		Fwd.SetData(rPkt);
		fSuccess &= SendMsgToHost(MkC4NetIOPacket(PID_FwdReq, Fwd));
	}
	return fSuccess;
}

bool C4Network2ClientList::SendMsgToHost(C4NetIOPacket rPkt)
{
	// find host
	C4Network2Client *pHost = GetHost();
	if (!pHost) return false;
	// send message
	if (!pHost->getMsgConn()) return false;
	return pHost->SendMsg(rPkt);
}

bool C4Network2ClientList::SendMsgToClient(int32_t iClient, C4NetIOPacket &&rPkt)
{
	// find client
	C4Network2Client *pClient = GetClientByID(iClient);
	if (!pClient) return false;
	// connected? send directly
	if (pClient->isConnected())
		return pClient->SendMsg(rPkt);
	// forward
	C4PacketFwd Fwd; Fwd.SetListType(false);
	Fwd.AddClient(iClient);
	Fwd.SetData(rPkt);
	return SendMsgToHost(MkC4NetIOPacket(PID_FwdReq, Fwd));
}

void C4Network2ClientList::HandlePacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn)
{
	// find associated client
	C4Network2Client *pClient = GetClient(pConn);
	if (!pClient) return;

#define GETPKT(type, name) \
    assert(pBasePkt); const type &name = \
     static_cast<const type &>(*pBasePkt);

	switch (cStatus)
	{

	case PID_Addr: // address propagation
	{
		GETPKT(C4PacketAddr, rPkt)
		// find client
		pClient = GetClientByID(rPkt.getClientID());
		if (pClient)
		{
			C4Network2Address addr = rPkt.getAddr();
			// IP zero? Set to IP from where the packet came
			if (addr.isIPNull())
			{
				addr.SetIP(pConn->getPeerAddr());
			}
			// add (no announce)
			if (pClient->AddAddr(addr, true))
				// new address? Try to connect
				pClient->DoConnectAttempt(pIO);
		}
	}
	break;

	}

#undef GETPKT
}

void C4Network2ClientList::SendAddresses(C4Network2IOConnection *pConn)
{
	// send all client addresses known
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		pClient->SendAddresses(pConn);
}

void C4Network2ClientList::DoConnectAttempts()
{
	// check interval
	time_t t; time(&t);
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (!pClient->isLocal() && !pClient->isRemoved() && pClient->getNextConnAttempt() && pClient->getNextConnAttempt() <= t)
			// attempt connect
			pClient->DoConnectAttempt(pIO);
}

void C4Network2ClientList::ResetReady()
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (pClient->isWaitedFor())
			pClient->SetStatus(NCS_NotReady);
}

bool C4Network2ClientList::AllClientsReady() const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (!pClient->isLocal() && pClient->isWaitedFor() && !pClient->isReady())
			return false;
	return true;
}

void C4Network2ClientList::UpdateClientActivity()
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (pClient->isActivated())
			if (::Players.GetAtClient(pClient->getID()))
				pClient->SetLastActivity(Game.FrameCounter);
}

// *** C4PacketAddr

void C4PacketAddr::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iClientID), "ClientID", C4ClientIDUnknown));
	pComp->Value(mkNamingAdapt(addr, "Addr"));
}

