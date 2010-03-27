/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2006  Peter Wortmann
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
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
#ifndef INC_C4Network2Client
#define INC_C4Network2Client

#include "C4NetIO.h"
#include "C4Network2IO.h"
#include "C4PacketBase.h"
#include "C4Client.h"

class C4Network2; class C4Network2IOConnection;

// maximum address count that is saved for one client
const int32_t C4ClientMaxAddr = 20;

// retry count and interval for connecting a client
const int32_t C4NetClientConnectAttempts = 3,
							C4NetClientConnectInterval = 6; // s

// network client status (host only)
enum C4Network2ClientStatus
{
	NCS_Joining,		// waiting for join data
	NCS_Chasing,		// client is behind (status not acknowledged, isn't waited for)
	NCS_NotReady,		// client is behind (status not acknowledged)
	NCS_Ready,			// client acknowledged network status
	NCS_Remove			// client is to be removed
};

class C4Network2Address
{
public:
	C4Network2Address()
		: eProtocol(P_NONE)
	{ ZeroMem(&addr, sizeof(addr)); }

	C4Network2Address(C4NetIO::addr_t addr, C4Network2IOProtocol eProtocol)
		: addr(addr), eProtocol(eProtocol)
	{ }

	C4Network2Address(const C4Network2Address &addr)
		: addr(addr.getAddr()), eProtocol(addr.getProtocol())
	{ }

	void operator = (const C4Network2Address &addr)
		{ SetAddr(addr.getAddr()); SetProtocol(addr.getProtocol()); }

	bool operator == (const C4Network2Address &addr) const;

protected:
	C4NetIO::addr_t addr;
	C4Network2IOProtocol eProtocol;

public:
	const C4NetIO::addr_t &getAddr() const { return addr; }
	in_addr								getIPAddr() const { return addr.sin_addr; }
	bool									isIPNull() const { return !addr.sin_addr.s_addr; }
	uint16_t							getPort() const { return htons(addr.sin_port); }
	C4Network2IOProtocol	getProtocol() const { return eProtocol; }

	StdStrBuf toString() const;

	void SetAddr(C4NetIO::addr_t naddr) { addr = naddr; }
	void SetIP(in_addr ip) { addr.sin_addr = ip; }
	void SetPort(uint16_t iPort) { addr.sin_port = htons(iPort); }
	void SetProtocol(C4Network2IOProtocol enProtocol) { eProtocol = enProtocol; }

	void CompileFunc(StdCompiler *pComp);
};

class C4Network2Client
{
	friend class C4Network2ClientList;
public:
	C4Network2Client(C4Client *pClient);
	~C4Network2Client();

protected:
	// client data
	C4Client *pClient;

	// addresses
	C4Network2Address Addr[C4ClientMaxAddr];
	int32_t AddrAttempts[C4ClientMaxAddr];
	int32_t iAddrCnt;

	// status
	C4Network2ClientStatus eStatus;

	// frame of last activity
	int32_t iLastActivity;

	// connections
	C4Network2IOConnection *pMsgConn, *pDataConn;
	time_t iNextConnAttempt;

	// part of client list
	C4Network2Client *pNext;
	class C4Network2ClientList *pParent;

	// statistics
	class C4TableGraph *pstatPing;

public:

	C4Client   *getClient()			const { return pClient; }
	const C4ClientCore &getCore() const { return getClient()->getCore(); }
	int32_t			getID()					const { return getCore().getID(); }
	bool				isLocal()				const { return pClient->isLocal(); }
	bool				isHost()				const { return getID() == C4ClientIDHost; }
	const char *getName()				const { return getCore().getName(); }
	bool				isActivated()		const { return getCore().isActivated(); }
	bool				isObserver()		const { return getCore().isObserver(); }

	int32_t			getAddrCnt()		const { return iAddrCnt; }
	const C4Network2Address &getAddr(int32_t i) const { return Addr[i]; }

	C4Network2ClientStatus getStatus() const { return eStatus; }
	bool				hasJoinData()		const { return getStatus() != NCS_Joining; }
	bool				isChasing()			const { return getStatus() == NCS_Chasing; }
	bool				isReady()				const { return getStatus() == NCS_Ready; }
	bool				isWaitedFor()		const { return getStatus() == NCS_NotReady || getStatus() == NCS_Ready; }
	bool				isRemoved()			const { return getStatus() == NCS_Remove; }

	bool				isConnected()		const { return !! pMsgConn; }
	time_t			getNextConnAttempt() const { return iNextConnAttempt; }
	int32_t     getLastActivity() const { return iLastActivity; }
	class C4TableGraph *getStatPing() const { return pstatPing; }

	C4Network2Client *getNext() const { return pNext; }

	void SetStatus(C4Network2ClientStatus enStatus) { eStatus = enStatus; }
	void SetLastActivity(int32_t iTick) { iLastActivity = iTick; }

	C4Network2IOConnection *getMsgConn() const { return pMsgConn; }
	C4Network2IOConnection *getDataConn() const { return pDataConn; }
	bool hasConn(C4Network2IOConnection *pConn);
	void SetMsgConn(C4Network2IOConnection *pConn);
	void SetDataConn(C4Network2IOConnection *pConn);
	void RemoveConn(C4Network2IOConnection *pConn);
	void CloseConns(const char *szMsg);

	bool SendMsg(C4NetIOPacket rPkt) const;
	bool SendData(C4NetIOPacket rPkt) const;

	bool DoConnectAttempt(class C4Network2IO *pIO);

	// addresses
	bool hasAddr(const C4Network2Address &addr) const;
	void ClearAddr();
	bool AddAddr(const C4Network2Address &addr, bool fAnnounce);
	void AddLocalAddrs(int16_t iPortTCP, int16_t iPortUDP);

	void SendAddresses(C4Network2IOConnection *pConn);

	// runtime statistics
	void CreateGraphs();
	void ClearGraphs();
};

class C4Network2ClientList
{
public:
	C4Network2ClientList(C4Network2IO *pIO);
	~C4Network2ClientList();

protected:
	C4Network2IO *pIO;
	C4Network2Client *pFirst;
	C4Network2Client *pLocal;
	C4ClientList *pClientList;
	bool fHost;

public:
	C4Network2Client *GetClientByID(int32_t iID) const;
	C4Network2Client *GetNextClientAfterID(int32_t iSmallerClientID) const;
	C4Network2Client *GetClient(const char *szName) const;
	C4Network2Client *GetClient(const C4ClientCore &CCore, int32_t iMaxDiffLevel = C4ClientCoreDL_IDMatch);
	C4Network2Client *GetClient(C4Network2IOConnection *pConn) const;
	C4Network2Client *GetLocal() const { return pLocal; }
	C4Network2Client *GetHost();
	C4Network2Client *GetNextClient(C4Network2Client *pClient);

	void Init(C4ClientList *pClientList, bool fHost);
	C4Network2Client *RegClient(C4Client *pClient);
	void DeleteClient(C4Network2Client *pClient);
	void Clear();

	// messages
	bool BroadcastMsgToConnClients(const C4NetIOPacket &rPkt);
	bool BroadcastMsgToClients(const C4NetIOPacket &rPkt);
	bool SendMsgToHost(C4NetIOPacket rPkt);
	bool SendMsgToClient(int32_t iClient, C4NetIOPacket RREF rPkt);

	// packet handling
	void HandlePacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn);

	// addresses
	void SendAddresses(C4Network2IOConnection *pConn);

	// connecting
	void DoConnectAttempts();

	// ready-ness
	void ResetReady();
	bool AllClientsReady() const;

	// activity
	void UpdateClientActivity();
};

// * Packets *

class C4PacketAddr : public C4PacketBase
{
public:
	C4PacketAddr() { }
	C4PacketAddr(int32_t iClientID, const C4Network2Address &addr)
		: iClientID(iClientID), addr(addr)
	{ }

protected:
	int32_t iClientID;
	C4Network2Address addr;

public:
	int32_t getClientID() const { return iClientID; }
	const C4Network2Address	&getAddr() const { return addr; }

	virtual void CompileFunc(StdCompiler *pComp);
};

#endif

