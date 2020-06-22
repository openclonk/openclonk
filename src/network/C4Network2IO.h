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
#ifndef INC_C4Network2IO
#define INC_C4Network2IO

#include "netpuncher/C4PuncherPacket.h"
#include "network/C4Client.h"
#include "network/C4InteractiveThread.h"
#include "network/C4NetIO.h"

#include <atomic>

class C4Network2IOConnection;

// enums & constants
enum C4Network2IOProtocol
{
	P_UDP, P_TCP, P_NONE = -1
};

const int C4NetTimer                = 500,  // ms
          C4NetPingFreq             = 1000, // ms
          C4NetStatisticsFreq       = 1000, // ms
          C4NetAcceptTimeout        = 10,   // s
          C4NetPingTimeout          = 30000;// ms

// client count
const int C4NetMaxClients = 256;

class C4Network2IO
		: protected C4InteractiveThread::Callback,
		protected C4NetIO::CBClass,
		protected StdSchedulerProc
{
public:
	C4Network2IO();
	~C4Network2IO() override;

protected:

	// main traffic net i/o classes
	C4NetIO *pNetIO_TCP{nullptr}, *pNetIO_UDP{nullptr};

	// discovery net i/o
	class C4Network2IODiscover *pNetIODiscover{nullptr};

	// reference server
	class C4Network2RefServer *pRefServer{nullptr};

	// UPnP port mapping manager
	class C4Network2UPnP *UPnPMgr{nullptr};

	// local client core
	C4ClientCore LCCore;
	CStdCSec LCCoreCSec;

	// connection list
	C4Network2IOConnection *pConnList{nullptr};
	CStdCSec ConnListCSec, BroadcastCSec;

	// next connection ID to use
	uint32_t iNextConnID{0};

	// allow incoming connections?
	bool fAllowConnect{false};

	// connection acceptance
	struct AutoAccept
	{
		C4ClientCore CCore;
		AutoAccept *Next;
	}
	*pAutoAcceptList{nullptr};
	CStdCSec AutoAcceptCSec;

	// make sure only one connection is established?
	bool fExclusiveConn{false};

	// timer & ping
	C4TimeMilliseconds tLastExecute;
	C4TimeMilliseconds tLastPing;

	// statistics
	C4TimeMilliseconds tLastStatistic;
	int iTCPIRate{0}, iTCPORate{0}, iTCPBCRate{0},
	iUDPIRate{0}, iUDPORate{0}, iUDPBCRate{0};

	// punching
	C4NetIO::addr_t PuncherAddrIPv4, PuncherAddrIPv6;
	bool IsPuncherAddr(const C4NetIO::addr_t& addr) const;

public:

	bool hasTCP() const { return !! pNetIO_TCP; }
	bool hasUDP() const { return !! pNetIO_UDP; }

	// initialization
	bool Init(int16_t iPortTCP, int16_t iPortUDP, int16_t iPortDiscovery = -1, int16_t iPortRefServer = -1, bool fBroadcast = false, bool enable_upnp = true); // by main thread
	void Clear(); // by main thread
	void SetLocalCCore(const C4ClientCore &CCore); // by main thread

	// i/o types
	C4NetIO *MsgIO(); // by both
	C4NetIO *DataIO(); // by both

	// connections
	bool Connect(const C4NetIO::addr_t &addr, C4Network2IOProtocol eProt, const C4ClientCore &nCCore, const char *szPassword = nullptr); // by main thread
	bool ConnectWithSocket(const C4NetIO::addr_t &addr, C4Network2IOProtocol eProt, const C4ClientCore &nCCore, std::unique_ptr<C4NetIOTCP::Socket> socket, const char *szPassword = nullptr); // by main thread
	void SetAcceptMode(bool fAcceptAll); // by main thread
	void SetExclusiveConnMode(bool fExclusiveConn); // by main thread
	int getConnectionCount(); // by main thread

	void ClearAutoAccept(); // by main thread
	void AddAutoAccept(const C4ClientCore &CCore); // by main thread
	void RemoveAutoAccept(const C4ClientCore &CCore); // by main thread

	C4Network2IOConnection *GetMsgConnection(int iClientID); // by both (returns referenced connection!)
	C4Network2IOConnection *GetDataConnection(int iClientID); // by both (returns referenced connection!)

	// broadcasting
	void BeginBroadcast(bool fSelectAll = false); // by both
	void EndBroadcast(); // by both
	bool Broadcast(const C4NetIOPacket &rPkt); // by both

	// sending helpers
	bool SendMsgToClient(C4NetIOPacket &rPkt, int iClient); // by both
	bool BroadcastMsg(const C4NetIOPacket &rPkt); // by both

	// punch
	bool InitPuncher(C4NetIO::addr_t PuncherAddr); // by main thread
	void SendPuncherPacket(const C4NetpuncherPacket&, C4NetIO::HostAddress::AddressFamily family);
	void Punch(const C4NetIO::addr_t&); // sends a ping packet

	// stuff
	C4NetIO *getNetIO(C4Network2IOProtocol eProt); // by both
	const char *getNetIOName(C4NetIO *pNetIO);
	C4Network2IOProtocol getNetIOProt(C4NetIO *pNetIO);

	// statistics
	int getProtIRate(C4Network2IOProtocol eProt) const { return eProt == P_TCP ? iTCPIRate : iUDPIRate; }
	int getProtORate(C4Network2IOProtocol eProt) const { return eProt == P_TCP ? iTCPORate : iUDPORate; }
	int getProtBCRate(C4Network2IOProtocol eProt) const { return eProt == P_TCP ? iTCPBCRate : iUDPBCRate; }

	// reference
	void SetReference(class C4Network2Reference *pReference);
	bool IsReferenceNeeded();

protected:
	// *** callbacks
	// C4NetIO-Callbacks
	bool OnConn(const C4NetIO::addr_t &addr, const C4NetIO::addr_t &AddrConnect, const C4NetIO::addr_t *pOwnAddr, C4NetIO *pNetIO) override;
	void OnDisconn(const C4NetIO::addr_t &addr, C4NetIO *pNetIO, const char *szReason) override;
	void OnPacket(const C4NetIOPacket &rPacket, C4NetIO *pNetIO) override;
	// C4NetIOMan
	virtual void OnError(const char *strError, C4NetIO *pNetIO);
	// StdSchedulerProc
	bool Execute(int iTimeout, pollfd *) override;
	C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow) override;
	// Event callback by C4InteractiveThread
	void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) override; // by main thread

	// connections list
	void AddConnection(C4Network2IOConnection *pConn); // by both
	void RemoveConnection(C4Network2IOConnection *pConn); // by both
	C4Network2IOConnection *GetConnection(const C4NetIO::addr_t &addr, C4NetIO *pNetIO); // by both
	C4Network2IOConnection *GetConnectionByConnAddr(const C4NetIO::addr_t &addr, C4NetIO *pNetIO); // by both
	C4Network2IOConnection *GetConnectionByID(uint32_t iConnID); // by thread

	// network events (signals to main thread)
	struct NetEvPacketData;

	// connection acceptance
	bool doAutoAccept(const C4ClientCore &CCore, const C4Network2IOConnection &Conn);

	// general packet handling (= forward in most cases)
	bool HandlePacket(const C4NetIOPacket &rPacket, C4Network2IOConnection *pConn, bool fThread); // by both
	void CallHandlers(int iHandlers, const class C4IDPacket *pPacket, C4Network2IOConnection *pConn, bool fThread); // by both

	// packet handling (some are really handled here)
	void HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn);
	void HandleFwdReq(const class C4PacketFwd &rFwd, C4Network2IOConnection *pBy);
	void HandlePuncherPacket(const C4NetIOPacket &rPacket);

	// misc
	bool Ping();
	void CheckTimeout();
	void GenerateStatistics(int iInterval);
	void SendConnPackets();
};

enum C4Network2IOConnStatus
{
	CS_Connect,           // waiting for connection
	CS_Connected,         // waiting for Conn
	CS_HalfAccepted,      // got Conn (peer identified, client class created if neccessary)
	CS_Accepted,          // got ConnRe (peer did accept)
	CS_Closed,
	CS_ConnectFail          // got closed before HalfAccepted was reached
};

class C4Network2IOConnection // shared
{
	friend class C4Network2IO;
public:
	C4Network2IOConnection();
	~C4Network2IOConnection();

protected:

	// connection data
	class C4NetIO *pNetClass{nullptr};
	C4Network2IOProtocol eProt;
	C4NetIO::addr_t PeerAddr, ConnectAddr;
	std::unique_ptr<C4NetIOTCP::Socket> TcpSimOpenSocket;

	// status data
	C4Network2IOConnStatus Status;
	uint32_t iID, iRemoteID;                // connection ID for this and the remote client
	bool fAutoAccept{false};                       // auto accepted by thread?
	bool fBroadcastTarget{false};                  // broadcast target?
	time_t iTimestamp{0};                      // timestamp of last status change
	int iPingTime{-1};                          // ping
	C4TimeMilliseconds tLastPing;          // if > iLastPong, it's the first ping that hasn't been answered yet, nullptr if no ping received yet
	C4TimeMilliseconds tLastPong;          // last pong received, nullptr if no pong received yet
	C4ClientCore CCore;                     // client core (>= CS_HalfAccepted)
	CStdCSec CCoreCSec;
	int iIRate, iORate;                     // input/output rates (by C4NetIO, in b/s)
	int iPacketLoss;                        // lost packets (in the last seconds)
	StdCopyStrBuf Password;                 // password to use for connect
	bool fConnSent{false};                         // initial connection packet send
	bool fPostMortemSent{false};                   // post mortem send

	// packet backlog
	uint32_t iOutPacketCounter{0}, iInPacketCounter{0};
	struct PacketLogEntry
	{
		uint32_t Number;
		C4NetIOPacket Pkt;
		PacketLogEntry *Next;
	};
	PacketLogEntry *pPacketLog{nullptr};
	CStdCSec PacketLogCSec;

	// list (C4Network2IO)
	C4Network2IOConnection *pNext{nullptr};

	// reference counter
	std::atomic_long iRefCnt;

public:
	C4NetIO  *getNetClass()   const { return pNetClass; }
	C4Network2IOProtocol   getProtocol()    const { return eProt; }
	const C4NetIO::addr_t &getPeerAddr()    const { return PeerAddr.GetPort() ? PeerAddr : ConnectAddr; }
	const C4NetIO::addr_t &getConnectAddr() const { return ConnectAddr; }
	uint32_t  getID()         const { return iID; }
	uint32_t  getRemoteID()   const { return iRemoteID; }
	time_t    getTimestamp()  const { return iTimestamp; }
	const C4ClientCore &getCCore()  const { return CCore; }
	CStdCSec &getCCoreCSec()        { return CCoreCSec; }
	int       getClientID()   const { return CCore.getID(); }
	bool      isHost()        const { return CCore.isHost(); }
	int       getPingTime()   const { return iPingTime; }
	int       getLag()        const;
	int       getIRate()      const { return iIRate; }
	int       getORate()      const { return iORate; }
	int       getPacketLoss() const { return iPacketLoss; }
	const char *getPassword() const { return Password.getData(); }
	bool      isConnSent()    const { return fConnSent; }

	uint32_t  getInPacketCounter()  const { return iInPacketCounter; }
	uint32_t  getOutPacketCounter() const { return iOutPacketCounter; }

	bool      isConnecting()  const { return Status == CS_Connect; }
	bool      isOpen()        const { return Status != CS_Connect && Status != CS_Closed && Status != CS_ConnectFail; }
	bool      isHalfAccepted()const { return Status == CS_HalfAccepted || Status == CS_Accepted; }
	bool      isAccepted()    const { return Status == CS_Accepted; }
	bool      isClosed()      const { return Status == CS_Closed || Status == CS_ConnectFail; }
	bool      isAutoAccepted()const { return fAutoAccept; }
	bool      isBroadcastTarget() const { return fBroadcastTarget; }
	bool      isFailed()      const { return Status == CS_ConnectFail; }

protected:
	// called by C4Network2IO only
	void Set(C4NetIO *pnNetClass, C4Network2IOProtocol eProt, const C4NetIO::addr_t &nPeerAddr, const C4NetIO::addr_t &nConnectAddr, C4Network2IOConnStatus nStatus, const char *szPassword, uint32_t iID);
	void SetSocket(std::unique_ptr<C4NetIOTCP::Socket> socket);
	void SetRemoteID(uint32_t iRemoteID);
	void SetPeerAddr(const C4NetIO::addr_t &nPeerAddr);
	void OnPing();
	void SetPingTime(int iPingTime);
	void SetStatus(C4Network2IOConnStatus nStatus);
	void SetAutoAccepted();
	void OnPacketReceived(uint8_t iPacketType);
	void ClearPacketLog(uint32_t iStartNumber = ~0);

public:
	// status changing
	void SetHalfAccepted()  { SetStatus(CS_HalfAccepted); }
	void SetAccepted()      { SetStatus(CS_Accepted); }
	void SetCCore(const C4ClientCore &nCCore);
	void ResetAutoAccepted() { fAutoAccept = false; }
	void SetConnSent()      { fConnSent = true; }

	// connection operations
	bool Connect();
	void Close();
	bool Send(const C4NetIOPacket &rPkt);
	void SetBroadcastTarget(bool fSet); // (only call after C4Network2IO::BeginBroadcast!)

	// statistics
	void DoStatistics(int iInterval, int *pIRateSum, int *pORateSum);

	// reference counting
	void AddRef(); void DelRef();

	// post mortem
	bool CreatePostMortem(class C4PacketPostMortem *pPkt);

};

// * Packets *

class C4PacketPing : public C4PacketBase
{
public:
	C4PacketPing(uint32_t iPacketCounter = 0, uint32_t iRemotePacketCounter = 0);

protected:
	C4TimeMilliseconds tTime;
	uint32_t iPacketCounter;

public:
	uint32_t getTravelTime() const;
	uint32_t getPacketCounter() const { return iPacketCounter; }

	void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketConn : public C4PacketBase
{
public:
	C4PacketConn();
	C4PacketConn(const class C4ClientCore &nCCore, uint32_t iConnID, const char *szPassword = nullptr);

protected:
	int32_t iVer;
	uint32_t iConnID;
	C4ClientCore CCore;
	StdCopyStrBuf Password;

public:
	int32_t getVer() const { return iVer; }
	uint32_t getConnID() const { return iConnID; }
	const C4ClientCore &getCCore() const { return CCore; }
	const char *getPassword() const { return Password.getData(); }

	void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketConnRe : public C4PacketBase
{
public:
	C4PacketConnRe();
	C4PacketConnRe(bool fOK, bool fWrongPassword, const char *szMsg = nullptr);

protected:
	bool fOK, fWrongPassword;
	StdStrBuf szMsg;

public:
	bool isOK() const { return fOK; }
	bool isPasswordWrong() const { return fWrongPassword; }
	const char *getMsg() const { return szMsg.getData(); }

	void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketFwd : public C4PacketBase
{
public:
	C4PacketFwd();
	C4PacketFwd(const StdBuf &Pkt);

protected:
	bool fNegativeList{false};
	int32_t iClients[C4NetMaxClients];
	int32_t iClientCnt{0};
	StdCopyBuf Data;

public:
	const StdCopyBuf &getData() const { return Data; }
	bool isNegativeList() const { return fNegativeList; }
	int32_t getClient(int32_t i)  const { return iClients[i]; }
	int32_t getClientCnt()    const { return iClientCnt; }

	bool DoFwdTo(int32_t iClient) const;

	void SetData(const StdBuf &Pkt);
	void SetListType(bool fnNegativeList);
	void AddClient(int32_t iClient);

	void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketPostMortem : public C4PacketBase
{
public:
	C4PacketPostMortem();
	~C4PacketPostMortem() override;

private:
	uint32_t iConnID;
	uint32_t iPacketCounter; // last packet counter of dead connection
	uint32_t iPacketCount{0};
	struct PacketLink
	{
		C4NetIOPacket Pkt;
		PacketLink *Next;
	};
	PacketLink *pPackets{nullptr};

public:
	uint32_t getConnID() const { return iConnID; }
	uint32_t getPacketCount() const { return iPacketCount; }
	void SetConnID(uint32_t inConnID) { iConnID = inConnID; }

	const C4NetIOPacket *getPacket(uint32_t iNumber) const;
	void SetPacketCounter(uint32_t iPacketCounter);
	void Add(const C4NetIOPacket &rPkt);

	void CompileFunc(StdCompiler *pComp) override;
};

#endif
