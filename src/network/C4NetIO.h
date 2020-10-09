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
/* network i/o, featuring tcp, udp and multicast */

#ifndef C4NETIO_H
#define C4NETIO_H

#include "lib/StdCompiler.h"
#include "platform/StdScheduler.h"
#include "platform/StdSync.h"

#ifdef _WIN32
#include "platform/C4windowswrapper.h"
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
	~C4NetIO() override;

	// *** constants / types
	static const int TO_INF; // = -1;

	struct HostAddress
	{
		enum AddressFamily
		{
			IPv6 = AF_INET6,
			IPv4 = AF_INET,
			UnknownFamily = 0
		};
		enum SpecialAddress
		{
			Loopback,	// IPv6 localhost (::1)
			Any,	// IPv6 any-address (::)
			AnyIPv4	// IPv4 any-address (0.0.0.0)
		};

		enum ToStringFlags
		{
			TSF_SkipZoneId = 1,
			TSF_SkipPort = 2
		};
		
		HostAddress() { Clear(); }
		HostAddress(const HostAddress &other) { SetHost(other); }
		HostAddress(SpecialAddress addr) { SetHost(addr); }
		explicit HostAddress(uint32_t addr) { SetHost(addr); }
		HostAddress(const StdStrBuf &addr) { SetHost(addr); }
		HostAddress(const sockaddr *addr) { SetHost(addr); }

		AddressFamily GetFamily() const;
		size_t GetAddrLen() const;
		
		void SetScopeId(int scopeId);
		int GetScopeId() const;

		void Clear();
		void SetHost(const sockaddr *addr);
		void SetHost(const HostAddress &host);
		void SetHost(SpecialAddress host);
		void SetHost(const StdStrBuf &host, AddressFamily family = UnknownFamily);
		void SetHost(uint32_t host);

		C4NetIO::HostAddress AsIPv6() const; // convert an IPv4 address to an IPv6-mapped IPv4 address
		C4NetIO::HostAddress AsIPv4() const; // try to convert an IPv6-mapped IPv4 address to an IPv4 address (returns unchanged address if not possible)

		// General categories
		bool IsNull() const;
		bool IsMulticast() const;
		bool IsLoopback() const;
		bool IsLocal() const; // IPv6 link-local address
		bool IsPrivate() const; // IPv6 ULA or IPv4 private address range
//		bool IsBroadcast() const;

		StdStrBuf ToString(int flags = 0) const;

		bool operator ==(const HostAddress &rhs) const;
		bool operator !=(const HostAddress &rhs) const { return !(*this == rhs); }

	protected:
		// data
		union
		{
			sockaddr gen;
			sockaddr_in v4;
			sockaddr_in6 v6;
		};
	};

	struct EndpointAddress : public HostAddress	// Host and port
	{
		static const uint16_t IPPORT_NONE = 0;

		EndpointAddress() { Clear(); }
		EndpointAddress(const EndpointAddress &other) : HostAddress() { SetAddress(other); }
		EndpointAddress& operator=(const EndpointAddress &other) { SetAddress(other); return *this; }
		EndpointAddress(const HostAddress &host, uint16_t port = IPPORT_NONE) : HostAddress(host) { SetPort(port); }
		EndpointAddress(HostAddress::SpecialAddress addr, uint16_t port = IPPORT_NONE) : HostAddress(addr) { SetPort(port); }
		explicit EndpointAddress(const StdStrBuf &addr) { SetAddress(addr); }

		StdStrBuf ToString(int flags = 0) const;

		void Clear();

		void SetAddress(const sockaddr *addr);
		void SetAddress(const EndpointAddress &other);
		void SetAddress(HostAddress::SpecialAddress addr, uint16_t port = IPPORT_NONE);
		void SetAddress(const HostAddress &host, uint16_t port = IPPORT_NONE);
		void SetAddress(const StdStrBuf &addr, AddressFamily family = UnknownFamily);

		HostAddress GetHost() const { return *this; }	// HostAddress copy ctor slices off port information
		EndpointAddress AsIPv6() const; // convert an IPv4 address to an IPv6-mapped IPv4 address
		EndpointAddress AsIPv4() const; // try to convert an IPv6-mapped IPv4 address to an IPv4 address (returns unchanged address if not possible)
		
		void SetPort(uint16_t port);
		void SetDefaultPort(uint16_t port); // set a port only if there is none
		uint16_t GetPort() const;

		bool IsNull() const;
		bool IsNullHost() const { return HostAddress::IsNull(); }

		// Pointer wrapper to be able to implicitly convert to sockaddr*
		class EndpointAddressPtr;
		const EndpointAddressPtr operator &() const;
		EndpointAddressPtr operator &();
		class EndpointAddressPtr
		{
			EndpointAddress * const p;
			friend EndpointAddressPtr EndpointAddress::operator &();
			friend const EndpointAddressPtr EndpointAddress::operator &() const;
			EndpointAddressPtr(EndpointAddress *p) : p(p) {}
		public:
			const EndpointAddress &operator *() const { return *p; }
			EndpointAddress &operator *() { return *p; }

			const EndpointAddress &operator ->() const { return *p; }
			EndpointAddress &operator ->() { return *p; }

			operator const EndpointAddress*() const { return p; }
			operator EndpointAddress*() { return p; }

			operator const sockaddr*() const { return &p->gen; }
			operator sockaddr*() { return &p->gen; }

			operator const sockaddr_in*() const { return &p->v4; }
			operator sockaddr_in*() { return &p->v4; }

			operator const sockaddr_in6*() const { return &p->v6; }
			operator sockaddr_in6*() { return &p->v6; }
		};

		bool operator ==(const EndpointAddress &rhs) const;
		bool operator !=(const EndpointAddress &rhs) const { return !(*this == rhs); }

		// conversions
		operator sockaddr() const { return gen; }
		operator sockaddr_in() const { assert(gen.sa_family == AF_INET); return v4; }
		operator sockaddr_in6() const { assert(gen.sa_family == AF_INET6); return v6; }

		// StdCompiler
		void CompileFunc(StdCompiler *comp);

	private:
		bool SetAddressByString(const StdStrBuf &address, short family);

		friend class EndpointAddressPtr;
	};
	typedef EndpointAddress addr_t;

	static std::vector<HostAddress> GetLocalAddresses();

	// callback class
	class CBClass
	{
	public:
		virtual bool OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO) { return true; }
		virtual void OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) { }
		virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) = 0;
		virtual ~CBClass() = default;
	};

	// used to explicitly callback to a specific class
	template <class T>
	class CBProxy : public CBClass
	{
		T *pTarget;
	public:
		CBProxy *operator () (T *pnTarget) { pTarget = pnTarget; return this; }
		bool OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO) override
		{ return pTarget->T::OnConn(AddrPeer, AddrConnect, pOwnAddr, pNetIO); }
		void OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) override
		{ pTarget->T::OnDisconn(AddrPeer, pNetIO, szReason); }
		void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) override
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
	virtual bool Init(uint16_t iPort = addr_t::IPPORT_NONE) = 0;
	virtual bool Close() = 0;

	bool Execute(int iTimeout = -1, pollfd * = nullptr) override = 0; // (for StdSchedulerProc)
	bool IsNotify() override { return true; }

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

protected:
//	virtual SOCKET CreateSocket() = 0;

	// Makes IPv4 connections from an IPv6 socket work.
	bool InitIPv6Socket(SOCKET socket);

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
	const char *getPData() const { return getSize() ? getBufPtr<char>(*this, 1) : nullptr; }
	size_t      getPSize() const { return getSize() ? getSize() - 1 : 0; }
	StdBuf      getPBuf()  const { return getSize() ? getPart(1, getSize() - 1) : getRef(); }

	// Some overloads
	C4NetIOPacket getRef() const { return C4NetIOPacket(StdBuf::getRef(), addr); }
	C4NetIOPacket Duplicate() const  { return C4NetIOPacket(StdBuf::Duplicate(), addr); }
	// change addr
	void SetAddr(const C4NetIO::addr_t &naddr) { addr = naddr; }

	// delete contents
	void Clear();
};


// tcp network i/o
class C4NetIOTCP : public C4NetIO, protected CStdCSecExCallback
{
public:
	C4NetIOTCP();
	~C4NetIOTCP() override;

	// Socket is an unconnected, but bound socket.
	class Socket
	{
		SOCKET sock;
		Socket(SOCKET s) : sock(s) { }
		friend class C4NetIOTCP;
	public:
		~Socket();
		// GetAddress returns the address the socket is bound to.
		C4NetIO::addr_t GetAddress();
	};

	// *** interface

	// * not multithreading safe
	bool Init(uint16_t iPort = addr_t::IPPORT_NONE) override;
	virtual bool InitBroadcast(addr_t *pBroadcastAddr);
	bool Close() override;
	virtual bool CloseBroadcast();

	bool Execute(int iMaxTime = TO_INF, pollfd * readyfds = nullptr) override;

	// * multithreading safe
	std::unique_ptr<Socket> Bind(const addr_t &addr);
	bool Connect(const addr_t &addr, std::unique_ptr<Socket> socket);
	bool Connect(const addr_t &addr) override;
	bool Close(const addr_t &addr) override;

	bool Send(const C4NetIOPacket &rPacket) override;
	bool Broadcast(const C4NetIOPacket &rPacket) override;
	bool SetBroadcast(const addr_t &addr, bool fSet = true) override;

	virtual void UnBlock();
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE GetEvent() override;
#else
	void GetFDs(std::vector<struct pollfd> & FDs) override;
#endif

	// statistics
	bool GetStatistic(int *pBroadcastRate) override;
	bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss) override;
	void ClearStatistic() override;

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
	Peer *pPeerList{nullptr};

	// small list for waited-for connections
	struct ConnectWait
	{
		SOCKET sock;
		addr_t addr;

		ConnectWait *Next;
	}
	*pConnectWaits{nullptr};

	CStdCSecEx PeerListCSec;
	CStdCSec PeerListAddCSec;

	// initialized?
	bool fInit{false};

	// listen socket
	uint16_t iListenPort;
	SOCKET lsock;

#ifdef STDSCHEDULER_USE_EVENTS
	// event indicating network activity
	HANDLE Event{nullptr};
#else
	// Pipe used for cancelling select
	int Pipe[2];
#endif

	// *** implementation

	bool Listen(uint16_t inListenPort);

	SOCKET CreateSocket(addr_t::AddressFamily family);
	bool Connect(const addr_t &addr, SOCKET nsock);

	Peer *Accept(SOCKET nsock = INVALID_SOCKET, const addr_t &ConnectAddr = addr_t());
	Peer *GetPeer(const addr_t &addr);
	void OnShareFree(CStdCSecEx *pCSec) override;

	void AddConnectWait(SOCKET sock, const addr_t &addr);
	ConnectWait *GetConnectWait(const addr_t &addr);
	void ClearConnectWaits();

	// *** callbacks
public:
	void SetCallback(CBClass *pnCallback) override { pCB = pnCallback; };
private:
	CBClass *pCB{nullptr};

};

// simple udp network i/o
// - No connections
// - Delivery not garantueed
// - Broadcast will multicast the packet to all clients with the same broadcast address.
class C4NetIOSimpleUDP : public C4NetIO
{
public:
	C4NetIOSimpleUDP();
	~C4NetIOSimpleUDP() override;

	bool Init(uint16_t iPort = addr_t::IPPORT_NONE) override;
	virtual bool InitBroadcast(addr_t *pBroadcastAddr);
	bool Close() override;
	virtual bool CloseBroadcast();

	bool Execute(int iMaxTime = TO_INF, pollfd * = nullptr) override;

	bool Send(const C4NetIOPacket &rPacket) override;
	bool Broadcast(const C4NetIOPacket &rPacket) override;

	virtual void UnBlock();
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE GetEvent() override;
#else
	void GetFDs(std::vector<struct pollfd> & FDs) override;
#endif

	// not implemented
	bool Connect(const addr_t &addr) override { assert(false); return false; }
	bool Close(const addr_t &addr) override { assert(false); return false; }

	bool SetBroadcast(const addr_t &addr, bool fSet = true) override { assert(false); return false; }

	bool GetStatistic(int *pBroadcastRate) override { assert(false); return false; }
	bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss) override
	{ assert(false); return false; }
	void ClearStatistic() override { assert(false); }

private:
	// status
	bool fInit{false};
	bool fMultiCast{false};
	uint16_t iPort;

	// the socket and the associated event
	SOCKET sock;
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE hEvent{nullptr};
#else
	int Pipe[2];
#endif

	// multicast
	addr_t MCAddr; ipv6_mreq MCGrpInfo;
	bool fMCLoopback;

	// multibind
	int fAllowReUse{false};

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
	void SetCallback(CBClass *pnCallback) override { pCB = pnCallback; };
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
	~C4NetIOUDP() override;

	// *** interface

	bool Init(uint16_t iPort = addr_t::IPPORT_NONE) override;
	bool InitBroadcast(addr_t *pBroadcastAddr) override;
	bool Close() override;
	bool CloseBroadcast() override;

	bool Execute(int iMaxTime = TO_INF, pollfd * = nullptr) override;

	bool Connect(const addr_t &addr) override;
	bool Close(const addr_t &addr) override;

	bool Send(const C4NetIOPacket &rPacket) override;
	bool SendDirect(C4NetIOPacket &&rPacket); // (mt-safe)
	bool Broadcast(const C4NetIOPacket &rPacket) override;
	bool SetBroadcast(const addr_t &addr, bool fSet = true) override;

	C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow) override;

	bool GetStatistic(int *pBroadcastRate) override;
	bool GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss) override;
	void ClearStatistic() override;

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
	struct BinAddr;
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
		Packet(C4NetIOPacket &&rnData, nr_t inNr);
		~Packet();

	protected:
		// data
		nr_t iNr;
		C4NetIOPacket Data;
		bool *pFragmentGot{nullptr};

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
		Packet *pFront{nullptr}, *pBack{nullptr};
		// packet counts
		unsigned int iPacketCnt{0}, iMaxPacketCnt;
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
		bool DoCheck(int iAskCnt = 0, int iMCAskCnt = 0, unsigned int *pAskList = nullptr);

		// sending
		bool SendDirect(const Packet &rPacket, unsigned int iNr = ~0);
		bool SendDirect(C4NetIOPacket &&rPacket);

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
	bool fInit{false};
	bool fMultiCast{false};
	uint16_t iPort;

	// peer list
	Peer *pPeerList{nullptr};

	// currently initializing - do not process packets, save them back instead
	bool fSavePacket{false};
	C4NetIOPacket LastPacket;

	// multicast support data
	addr_t MCLoopbackAddr;
	bool fDelayedLoopbackTest{false};

	// check timing.
	C4TimeMilliseconds tNextCheck;

	// outgoing packet list (for multicast)
	PacketList OPackets;
	unsigned int iOPacketCounter{0};

	// statistics
	int iBroadcastRate{0};
	CStdCSec StatCSec;

	// callback proxy
	NETIO_CREATE_CALLBACK_PROXY(C4NetIOUDP, CBProxy)

	// * helpers

	// sending
	bool BroadcastDirect(const Packet &rPacket, unsigned int iNr = ~0u); // (mt-safe)

	// multicast related
	bool DoLoopbackTest();
	void ClearMCPackets();

	// peer list
	void AddPeer(Peer *pPeer);
	Peer *GetPeer(const addr_t &addr);
	Peer *ConnectPeer(const addr_t &PeerAddr, bool fFailCallback);
	void OnShareFree(CStdCSecEx *pCSec) override;

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
	void SetCallback(CBClass *pnCallback) override { pCB = pnCallback; };
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
	~C4NetIOMan() override;

	void Clear();

	void AddIO(C4NetIO *pNetIO, bool fSetCallback = true);
	void RemoveIO(C4NetIO *pNetIO);

protected:

	// net i/o list
	int iNetIOCnt{0}, iNetIOCapacity{0};
	C4NetIO **ppNetIO{nullptr};

	// overridables
	virtual void OnError(const char *strError, C4NetIO *pNetIO) { };

private:
	void OnError(StdSchedulerProc *pProc) override;

	void EnlargeIO(int iBy);
};

#ifdef HAVE_WINSOCK
bool AcquireWinSock();
void ReleaseWinSock();
#endif

// Class that keeps winsock loaded
class WinSockHolder
{
public:
	WinSockHolder()
	{
#ifdef HAVE_WINSOCK
		AcquireWinSock();
#endif
	}

	~WinSockHolder()
	{
#ifdef HAVE_WINSOCK
		ReleaseWinSock();
#endif
	}
};


#endif
