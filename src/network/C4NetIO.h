/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
/* network i/o, featuring tcp, udp and multicast */

#ifndef C4NETIO_H
#define C4NETIO_H

#include "StdSync.h"
#include "StdBuf.h"
#include "StdCompiler.h"
#include "StdScheduler.h"

#ifdef _WIN32
#include <C4windowswrapper.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef WINSOCK_VERSION
#define WINSOCK_VERSION MAKEWORD(2,2)
#endif
// Events are Windows-specific
#define HAVE_WINSOCK
#else
#define SOCKET int
#define INVALID_SOCKET (-1)
#include <arpa/inet.h>
// for htons
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif

#ifndef HAVE_CONFIG_H
// #define C4NETIO_DEBUG
#endif


// net i/o base class
class C4NetIO : public StdSchedulerProc
{
public:
	C4NetIO();
	virtual ~C4NetIO();

	// *** constants / types
	static const int TO_INF; // = -1;
	static const uint16_t P_NONE; // = -1

	typedef sockaddr_in addr_t;

	// callback class
	class CBClass
	{
	public:
		virtual bool OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO) { return true; }
		virtual void OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) { }
		virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) = 0;
		virtual ~CBClass() { }
	};

	// used to explicitly callback to a specific class
	template <class T>
	class CBProxy : public CBClass
	{
		T *pTarget;
	public:
		CBProxy *operator () (T *pnTarget) { pTarget = pnTarget; return this; }
		virtual bool OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO)
		{ return pTarget->T::OnConn(AddrPeer, AddrConnect, pOwnAddr, pNetIO); }
		virtual void OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason)
		{ pTarget->T::OnDisconn(AddrPeer, pNetIO, szReason); }
		virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
		{ pTarget->T::OnPacket(rPacket, pNetIO); }
	};

#ifdef _MSC_VER
#define NETIO_CREATE_CALLBACK_PROXY(ForClass, ProxyName) \
    typedef class C4NetIO::CBProxy<ForClass> CBProxyT; \
    friend CBProxyT; \
    CBProxyT ProxyName;
#else
#define NETIO_CREATE_CALLBACK_PROXY(ForClass, ProxyName) \
    friend class C4NetIO::CBProxy<ForClass>; \
    C4NetIO::CBProxy<ForClass> ProxyName;
#endif

	// *** interface

	// * not multithreading safe
	virtual bool Init(uint16_t iPort = P_NONE) = 0;
	virtual bool InitBroadcast(addr_t *pBroadcastAddr) = 0;
	virtual bool Close() = 0;
	virtual bool CloseBroadcast() = 0;

	virtual bool Execute(int iTimeout = -1, pollfd * = 0) = 0; // (for StdSchedulerProc)
	virtual bool IsNotify() { return true; }

	// * multithreading safe
	virtual bool Connect(const addr_t &addr) = 0; // async!
	virtual bool Close(const addr_t &addr) = 0;

	virtual bool Send(const class C4NetIOPacket &rPacket) = 0;
	virtual bool SetBroadcast(const addr_t &addr, bool fSet = true) = 0;
	virtual bool Broadcast(const class C4NetIOPacket &rPacket) = 0;

	// statistics
	virtual bool GetStatistic(int *pBroadcastRate) = 0;
	virtual bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss) = 0;
	virtual void ClearStatistic() = 0;

	// *** errors
protected:
	StdCopyStrBuf Error;
	void SetError(const char *strnError, bool fSockErr = false);
public:
	virtual const char *GetError() const { return Error.getData(); }
	void ResetError() { Error.Clear(); }

	// *** callbacks
	virtual void SetCallback(CBClass *pnCallback) = 0;

};

// packet class
class C4NetIOPacket : public StdCopyBuf
{
public:

	C4NetIOPacket();

	// construct from memory (copies / references data)
	C4NetIOPacket(const void *pnData, size_t inSize, bool fCopy = false, const C4NetIO::addr_t &naddr = C4NetIO::addr_t());
	// construct from buffer (copies data)
	explicit C4NetIOPacket(const StdBuf &Buf, const C4NetIO::addr_t &naddr);
	// construct from status byte + buffer (copies data)
	C4NetIOPacket(uint8_t cStatusByte, const char *pnData, size_t inSize, const C4NetIO::addr_t &naddr = C4NetIO::addr_t());

	~C4NetIOPacket();

protected:

	// address
	C4NetIO::addr_t addr;

public:

	const C4NetIO::addr_t &getAddr() const { return addr; }

	uint8_t     getStatus()const { return getSize() ? *getBufPtr<char>(*this) : 0; }
	const char *getPData() const { return getSize() ? getBufPtr<char>(*this, 1) : NULL; }
	size_t      getPSize() const { return getSize() ? getSize() - 1 : 0; }
	StdBuf      getPBuf()  const { return getSize() ? getPart(1, getSize() - 1) : getRef(); }

	// Some overloads
	C4NetIOPacket getRef() const { return C4NetIOPacket(StdBuf::getRef(), addr); }
	C4NetIOPacket Duplicate() const  { return C4NetIOPacket(StdBuf::Duplicate(), addr); }
	// change addr
	void SetAddr(const C4NetIO::addr_t &naddr) { addr = naddr; }

	// delete contents
	void Clear();
	// Talk gcc into accepting references to temporaries
	ALLOW_TEMP_TO_REF(C4NetIOPacket)
};


// tcp network i/o
class C4NetIOTCP : public C4NetIO, protected CStdCSecExCallback
{
public:
	C4NetIOTCP();
	virtual ~C4NetIOTCP();

	// *** interface

	// * not multithreading safe
	virtual bool Init(uint16_t iPort = P_NONE);
	virtual bool InitBroadcast(addr_t *pBroadcastAddr);
	virtual bool Close();
	virtual bool CloseBroadcast();

	virtual bool Execute(int iMaxTime = TO_INF, pollfd * readyfds = 0);

	// * multithreading safe
	virtual bool Connect(const addr_t &addr);
	virtual bool Close(const addr_t &addr);

	virtual bool Send(const C4NetIOPacket &rPacket);
	virtual bool Broadcast(const C4NetIOPacket &rPacket);
	virtual bool SetBroadcast(const addr_t &addr, bool fSet = true);

	virtual void UnBlock();
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent();
#else
	virtual void GetFDs(std::vector<struct pollfd> & FDs);
#endif

	// statistics
	virtual bool GetStatistic(int *pBroadcastRate);
	virtual bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss);
	virtual void ClearStatistic();

protected:

	// * overridables (packet format)

	// Append packet data to output buffer
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);

	// Extract a packet from the start of the input buffer (if possible) and call OnPacket.
	// Should return the numer of bytes used.
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &Addr);

	// *** data

	// peer class
	class Peer
	{
	public:
		Peer(const C4NetIO::addr_t &naddr, SOCKET nsock, C4NetIOTCP *pnParent);
		~Peer();
	protected:
		// constants
		static const unsigned int iTCPHeaderSize; // = 28 + 24; // (bytes)
		static const unsigned int iMinIBufSize; // = 8192; // (bytes)
		// parent
		C4NetIOTCP *const pParent;
		// addr
		C4NetIO::addr_t addr;
		// socket connected
		SOCKET sock;
		// incoming & outgoing buffer
		StdBuf IBuf, OBuf;
		int iIBufUsage;
		// statistics
		int iIRate, iORate;
		// status (1 = open, 0 = closed)
		bool fOpen;
		// selected for broadcast?
		bool fDoBroadcast;
		// IO critical sections
		CStdCSec ICSec; CStdCSec OCSec;
	public:
		// data access
		const C4NetIO::addr_t &GetAddr() const { return addr; }
		SOCKET                 GetSocket() const { return sock; }
		int                    GetIRate() const { return iIRate; }
		int                    GetORate() const { return iORate; }
		// send a packet to this peer
		bool Send(const C4NetIOPacket &rPacket);
		// send as much data of the interal outgoing buffer as possible
		bool Send();
		// request buffer space for new input. Must call OnRecv or NoRecv afterwards!
		void *GetRecvBuf(int iSize);
		// called after the buffer returned by GetRecvBuf has been filled with fresh data
		void OnRecv(int iSize);
		// close socket
		void Close();
		// test: open?
		bool Open() const { return fOpen; }
		// selected for broadcast?
		bool doBroadcast() const { return fDoBroadcast; }
		// outgoing data waiting?
		bool hasWaitingData() const { return !OBuf.isNull(); }
		// select/unselect peer
		void SetBroadcast(bool fSet) { fDoBroadcast = fSet; }
		// statistics
		void ClearStatistics();
	public:
		// next peer
		Peer *Next;
	};
	friend class Peer;
	// peer list
	Peer *pPeerList;

	// small list for waited-for connections
	struct ConnectWait
	{
		SOCKET sock;
		addr_t addr;

		ConnectWait *Next;
	}
	*pConnectWaits;

	CStdCSecEx PeerListCSec;
	CStdCSec PeerListAddCSec;

	// initialized?
	bool fInit;

	// listen socket
	uint16_t iListenPort;
	SOCKET lsock;

#ifdef STDSCHEDULER_USE_EVENTS
	// event indicating network activity
	HANDLE Event;
#else
	// Pipe used for cancelling select
	int Pipe[2];
#endif

	// *** implementation

	bool Listen(uint16_t inListenPort);

	Peer *Accept(SOCKET nsock = INVALID_SOCKET, const addr_t &ConnectAddr = addr_t());
	Peer *GetPeer(const addr_t &addr);
	void OnShareFree(CStdCSecEx *pCSec);

	void AddConnectWait(SOCKET sock, const addr_t &addr);
	ConnectWait *GetConnectWait(const addr_t &addr);
	void ClearConnectWaits();

	// *** callbacks
public:
	virtual void SetCallback(CBClass *pnCallback) { pCB = pnCallback; };
private:
	CBClass *pCB;

};

// simple udp network i/o
// - No connections
// - Delivery not garantueed
// - Broadcast will multicast the packet to all clients with the same broadcast address.
class C4NetIOSimpleUDP : public C4NetIO
{
public:
	C4NetIOSimpleUDP();
	virtual ~C4NetIOSimpleUDP();

	virtual bool Init(uint16_t iPort = P_NONE);
	virtual bool InitBroadcast(addr_t *pBroadcastAddr);
	virtual bool Close();
	virtual bool CloseBroadcast();

	virtual bool Execute(int iMaxTime = TO_INF, pollfd * = 0);

	virtual bool Send(const C4NetIOPacket &rPacket);
	virtual bool Broadcast(const C4NetIOPacket &rPacket);

	virtual void UnBlock();
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent();
#else
	virtual void GetFDs(std::vector<struct pollfd> & FDs);
#endif

	// not implemented
	virtual bool Connect(const addr_t &addr) { assert(false); return false; }
	virtual bool Close(const addr_t &addr) { assert(false); return false; }

	virtual bool SetBroadcast(const addr_t &addr, bool fSet = true) { assert(false); return false; }

	virtual bool GetStatistic(int *pBroadcastRate) { assert(false); return false; }
	virtual bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss)
	{ assert(false); return false; }
	virtual void ClearStatistic() { assert(false); }

private:
	// status
	bool fInit;
	bool fMultiCast;
	uint16_t iPort;

	// the socket and the associated event
	SOCKET sock;
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE hEvent;
#else
	int Pipe[2];
#endif

	// multicast
	addr_t MCAddr; ip_mreq MCGrpInfo;
	bool fMCLoopback;

	// multibind
	int fAllowReUse;

protected:

	// multicast address
	const addr_t &getMCAddr() const { return MCAddr; }

	// (try to) control loopback
	bool SetMCLoopback(int fLoopback);
	bool getMCLoopback() const { return fMCLoopback; }

	// enable multi-bind (call before Init!)
	void SetReUseAddress(bool fAllow);

private:

	// socket wait (check for readability)
	enum WaitResult { WR_Timeout, WR_Readable, WR_Cancelled, WR_Error = -1 };
	WaitResult WaitForSocket(int iTimeout);

	// *** callbacks
public:
	virtual void SetCallback(CBClass *pnCallback) { pCB = pnCallback; };
private:
	CBClass *pCB;

};

// udp network i/o
// - Connection are emulated
// - Delivery garantueed
// - Broadcast will automatically be activated on one side if it's active on the other side.
//   If the peer can't be reached through broadcasting, packets will be sent directly.
class C4NetIOUDP : public C4NetIOSimpleUDP, protected CStdCSecExCallback
{
public:
	C4NetIOUDP();
	virtual ~C4NetIOUDP();

	// *** interface

	virtual bool Init(uint16_t iPort = P_NONE);
	virtual bool InitBroadcast(addr_t *pBroadcastAddr);
	virtual bool Close();
	virtual bool CloseBroadcast();

	virtual bool Execute(int iMaxTime = TO_INF, pollfd * = 0);

	virtual bool Connect(const addr_t &addr);
	virtual bool Close(const addr_t &addr);

	virtual bool Send(const C4NetIOPacket &rPacket);
	virtual bool Broadcast(const C4NetIOPacket &rPacket);
	virtual bool SetBroadcast(const addr_t &addr, bool fSet = true);

	virtual C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow);

	virtual bool GetStatistic(int *pBroadcastRate);
	virtual bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss);
	virtual void ClearStatistic();

protected:

	// *** data

	// internal packet type ids
	enum IPTypeID
	{
		IPID_Ping = 0,
		IPID_Test = 1,
		IPID_Conn = 2,
		IPID_ConnOK = 3,
		IPID_AddAddr = 7,
		IPID_Data = 4,
		IPID_Check = 5,
		IPID_Close = 6
	};

	// packet structures
	struct PacketHdr; struct TestPacket; struct ConnPacket; struct ConnOKPacket; struct AddAddrPacket;
	struct DataPacketHdr; struct CheckPacketHdr; struct ClosePacket;

	// constants
	static const unsigned int iVersion; // = 2;

	static const unsigned int iStdTimeout, // = 1000, // (ms)
	iCheckInterval; // = 1000 // (ms)

	static const unsigned int iMaxOPacketBacklog; // = 100;

	static const unsigned int iUDPHeaderSize; // = 8 + 24; // (bytes)

	// packet class
	class PacketList;
	class Packet
	{
		friend class PacketList;
	public:

		// constants / structures
		static const size_t MaxSize; // = 1024;
		static const size_t MaxDataSize; // = MaxSize - sizeof(Header);

		// types used for packing
		typedef uint32_t nr_t;

		// construction / destruction
		Packet();
		Packet(C4NetIOPacket RREF rnData, nr_t inNr);
		~Packet();

	protected:
		// data
		nr_t iNr;
		C4NetIOPacket Data;
		bool *pFragmentGot;

	public:
		// data access
		C4NetIOPacket       &GetData()          { return Data; }
		const C4NetIOPacket &GetData()    const { return Data; }
		nr_t                 GetNr()      const { return iNr; }
		bool                 Empty()      const { return Data.isNull(); }
		bool                 Multicast()  const { return !!(Data.getStatus() & 0x80); }

		// fragmention
		nr_t                 FragmentCnt() const;
		C4NetIOPacket        GetFragment(nr_t iFNr, bool fBroadcastFlag = false) const;
		bool                 Complete() const;
		bool                 FragmentPresent(nr_t iFNr) const;
		bool                 AddFragment(const C4NetIOPacket &Packet, const C4NetIO::addr_t &addr);
	protected:
		::size_t             FragmentSize(nr_t iFNr) const;
		// list
		Packet *Next, *Prev;
	};
	friend class Packet;

	class PacketList
	{
	public:
		PacketList(unsigned int iMaxPacketCnt = ~0);
		~PacketList();

	protected:
		// packet list
		Packet *pFront, *pBack;
		// packet counts
		unsigned int iPacketCnt, iMaxPacketCnt;
		// critical section
		CStdCSecEx ListCSec;

	public:
		Packet *GetPacket(unsigned int iNr);
		Packet *GetPacketFrgm(unsigned int iNr);
		Packet *GetFirstPacketComplete();
		bool FragmentPresent(unsigned int iNr);

		bool AddPacket(Packet *pPacket);
		bool DeletePacket(Packet *pPacket);
		void ClearPackets(unsigned int iUntil);
		void Clear();
	};
	friend class PacketList;

	// peer class
	class Peer
	{
	public:

		// construction / destruction
		Peer(const C4NetIO::addr_t &naddr, C4NetIOUDP *pnParent);
		~Peer();

	protected:

		// constants
		static const unsigned int iConnectRetries; // = 5
		static const unsigned int iReCheckInterval; // = 1000 (ms)

		// parent class
		C4NetIOUDP *const pParent;

		// peer address
		C4NetIO::addr_t addr;
		// alternate peer address
		C4NetIO::addr_t addr2;
		// the address used by the peer
		addr_t PeerAddr;
		// connection status
		enum ConnStatus
		{
			CS_None, CS_Conn, CS_Works, CS_Closed
		}
		eStatus;
		// does multicast work?
		bool fMultiCast;
		// selected for broadcast?
		bool fDoBroadcast;
		// do callback on connection timeout?
		bool fConnFailCallback;

		// packet lists (outgoing, incoming, incoming multicast)
		PacketList OPackets;
		PacketList IPackets, IMCPackets;

		// packet counters
		unsigned int iOPacketCounter;
		unsigned int iIPacketCounter, iRIPacketCounter;
		unsigned int iIMCPacketCounter, iRIMCPacketCounter;

		unsigned int iMCAckPacketCounter;

		// output critical section
		CStdCSec OutCSec;

		// connection check time limit.
		C4TimeMilliseconds tNextReCheck;
		unsigned int iLastPacketAsked, iLastMCPacketAsked;

		// timeout time.
		C4TimeMilliseconds tTimeout;
		unsigned int iRetries;

		// statistics
		int iIRate, iORate, iLoss;
		CStdCSec StatCSec;

	public:
		// data access
		const C4NetIO::addr_t &GetAddr() const { return addr; }
		const C4NetIO::addr_t &GetAltAddr() const { return addr2; }

		// initiate connection
		bool Connect(bool fFailCallback);

		// send something to this computer
		bool Send(const C4NetIOPacket &rPacket);
		// check for lost packets
		bool Check(bool fForceCheck = true);

		// called if something from this peer was received
		void OnRecv(const C4NetIOPacket &Packet);

		// close connection
		void Close(const char *szReason);
		// open?
		bool Open() const { return eStatus == CS_Works; }
		// closed?
		bool Closed() const { return eStatus == CS_Closed; }
		// multicast support?
		bool MultiCast() const { return fMultiCast; }

		// acknowledgment check
		unsigned int GetMCAckPacketCounter() const { return iMCAckPacketCounter; }

		// timeout checking
		C4TimeMilliseconds GetTimeout() { return tTimeout; }
		void CheckTimeout();

		// selected for broadcast?
		bool doBroadcast() const { return fDoBroadcast; }
		// select/unselect peer
		void SetBroadcast(bool fSet) { fDoBroadcast = fSet; }

		// alternate address
		void SetAltAddr(const C4NetIO::addr_t &naddr2) { addr2 = naddr2; }

		// statistics
		int GetIRate() const { return iIRate; }
		int GetORate() const { return iORate; }
		int GetLoss() const { return iLoss; }
		void ClearStatistics();

	protected:

		// helpers
		bool DoConn(bool fMC);
		bool DoCheck(int iAskCnt = 0, int iMCAskCnt = 0, unsigned int *pAskList = NULL);

		// sending
		bool SendDirect(const Packet &rPacket, unsigned int iNr = ~0);
		bool SendDirect(C4NetIOPacket RREF rPacket);

		// events
		void OnConn();
		void OnClose(const char *szReason);

		// incoming packet list
		void CheckCompleteIPackets();

		// timeout
		void SetTimeout(int iLength = iStdTimeout, int iRetryCnt = 0);
		void OnTimeout();

	public:
		// next peer
		Peer *Next;
	};
	friend class Peer;

	// critical sections
	CStdCSecEx PeerListCSec;
	CStdCSec PeerListAddCSec;
	CStdCSec OutCSec;

	// status
	bool fInit;
	bool fMultiCast;
	uint16_t iPort;

	// peer list
	Peer *pPeerList;

	// currently initializing - do not process packets, save them back instead
	bool fSavePacket;
	C4NetIOPacket LastPacket;

	// multicast support data
	addr_t MCLoopbackAddr;
	bool fDelayedLoopbackTest;

	// check timing.
	C4TimeMilliseconds tNextCheck;

	// outgoing packet list (for multicast)
	PacketList OPackets;
	unsigned int iOPacketCounter;

	// statistics
	int iBroadcastRate;
	CStdCSec StatCSec;

	// callback proxy
	NETIO_CREATE_CALLBACK_PROXY(C4NetIOUDP, CBProxy)

	// * helpers

	// sending
	bool BroadcastDirect(const Packet &rPacket, unsigned int iNr = ~0u); // (mt-safe)
	bool SendDirect(C4NetIOPacket RREF rPacket); // (mt-safe)

	// multicast related
	bool DoLoopbackTest();
	void ClearMCPackets();

	// peer list
	void AddPeer(Peer *pPeer);
	Peer *GetPeer(const addr_t &addr);
	Peer *ConnectPeer(const addr_t &PeerAddr, bool fFailCallback);
	void OnShareFree(CStdCSecEx *pCSec);

	// connection check
	void DoCheck();

	// critical section: only one execute at a time
	CStdCSec ExecuteCSec;

	// debug
#ifdef C4NETIO_DEBUG
	int hDebugLog;
	void OpenDebugLog();
	void CloseDebugLog();
	void DebugLogPkt(bool fOut, const C4NetIOPacket &Pkt);
#endif

	// *** callbacks
public:
	virtual void SetCallback(CBClass *pnCallback) { pCB = pnCallback; };
private:
	CBClass *pCB;

	// callback interface for C4NetIO
	virtual bool OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO);
	virtual void OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason);
	virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

	void OnAddAddress(const addr_t &FromAddr, const AddAddrPacket &Packet);
};

// net i/o management (e.g. thread support)
class C4NetIOMan : public C4NetIO::CBClass, public StdSchedulerThread
{
public:
	C4NetIOMan();
	virtual ~C4NetIOMan();

	void Clear();

	void AddIO(C4NetIO *pNetIO, bool fSetCallback = true);
	void RemoveIO(C4NetIO *pNetIO);

protected:

	// net i/o list
	int iNetIOCnt, iNetIOCapacity;
	C4NetIO **ppNetIO;

	// overridables
	virtual void OnError(const char *strError, C4NetIO *pNetIO) { };

private:
	virtual void OnError(StdSchedulerProc *pProc);

	void EnlargeIO(int iBy);
};

// helpers
inline bool AddrEqual(const C4NetIO::addr_t addr1, const C4NetIO::addr_t addr2)
{
	return addr1.sin_addr.s_addr == addr2.sin_addr.s_addr &&
	       addr1.sin_family      == addr2.sin_family      &&
	       addr1.sin_port        == addr2.sin_port;
}
inline bool operator == (const C4NetIO::addr_t addr1, const C4NetIO::addr_t addr2) { return AddrEqual(addr1, addr2); }
inline bool operator != (const C4NetIO::addr_t addr1, const C4NetIO::addr_t addr2) { return !AddrEqual(addr1, addr2); }

// there seems to be no standard way to get these numbers, so let's do it the dirty way...
inline uint8_t &in_addr_b(in_addr &addr, int i)
{
	assert(0 <= i && i < 4);
	return *(reinterpret_cast<uint8_t *>(&addr.s_addr) + i);
}

inline void CompileFunc(in_addr &ip, StdCompiler *pComp)
{
	pComp->Value(in_addr_b(ip, 0)); pComp->Separator(StdCompiler::SEP_PART);
	pComp->Value(in_addr_b(ip, 1)); pComp->Separator(StdCompiler::SEP_PART);
	pComp->Value(in_addr_b(ip, 2)); pComp->Separator(StdCompiler::SEP_PART);
	pComp->Value(in_addr_b(ip, 3));
}

inline void CompileFunc(C4NetIO::addr_t &addr, StdCompiler *pComp)
{
	pComp->Value(addr.sin_addr); pComp->Separator(StdCompiler::SEP_PART2);
	uint16_t iPort = htons(addr.sin_port);
	pComp->Value(iPort);
	addr.sin_port = htons(iPort);
	if (pComp->isCompiler())
	{
		addr.sin_family = AF_INET;
		ZeroMem(addr.sin_zero, sizeof(addr.sin_zero));
	}
}

#ifdef HAVE_WINSOCK
bool AcquireWinSock();
void ReleaseWinSock();
#endif
bool ResolveAddress(const char *szAddress, C4NetIO::addr_t *paddr, uint16_t iPort);

#endif
