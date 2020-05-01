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
#ifndef INC_C4PacketBase
#define INC_C4PacketBase

#include "network/C4NetIO.h"

// *** packet base class

class C4PacketBase
{
	friend class C4PacketList;
public:
	C4PacketBase();
	virtual ~C4PacketBase();

	// virtual functions to implement by derived classes
	virtual void CompileFunc(StdCompiler *pComp) = 0;

	// conversion (using above functions)
	C4NetIOPacket pack(const C4NetIO::addr_t &addr = C4NetIO::addr_t()) const;
	C4NetIOPacket pack(uint8_t cStatus, const C4NetIO::addr_t &addr = C4NetIO::addr_t()) const;
	void unpack(const C4NetIOPacket &Pkt, char *pStatus = nullptr);

};

inline C4NetIOPacket MkC4NetIOPacket(char cStatus, const class C4PacketBase &Pkt, const C4NetIO::addr_t &addr = C4NetIO::addr_t())
{
	return Pkt.pack(cStatus, addr);
}

// Filename Adaptor
// Converts the network filename separator to the native filename separator
struct C4NetFilenameAdapt
{
	StdStrBuf &FileName;
	explicit C4NetFilenameAdapt(StdStrBuf &FileName) : FileName(FileName) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
#ifdef _WIN32
		pComp->Value(FileName);
#else
		StdCopyStrBuf FileName2;
		if (pComp->isSerializer() && FileName)
		{
			FileName2.Copy(FileName);
			SReplaceChar(FileName2.getMData(),DirectorySeparator,'\\');
		}
		pComp->Value(FileName2);
		if (pComp->isDeserializer())
		{
			FileName.Take(FileName2);
			SReplaceChar(FileName.getMData(),'\\',DirectorySeparator);
		}
#endif
	}
	template <class T> bool operator == (const T &rVal) { return FileName == rVal; }
	template <class T> C4NetFilenameAdapt &operator = (const T &rVal) { FileName = rVal; return *this; }
};
inline C4NetFilenameAdapt mkNetFilenameAdapt(StdStrBuf &FileName) { return C4NetFilenameAdapt(FileName); }

// enumaration of all used packet types
enum C4PacketType
{
	PID_None          = 0xFF,

	// *** network

	// * base packets
	// ping
	PID_Ping          = 0x00,
	PID_Pong          = 0x01,

	// connecting
	PID_Conn          = 0x02,
	PID_ConnRe        = 0x03,

	// msg forwarding
	PID_FwdReq        = 0x04,
	PID_Fwd           = 0x05,

	// post mortem
	PID_PostMortem    = 0x06,

	// (packets before this ID won't be recovered post-mortem)
	PID_PacketLogStart = 0x04,

	// * game
	// game status
	PID_Status        = 0x10,
	PID_StatusAck     = 0x11,

	// client address propagation
	PID_Addr          = 0x12,

	// activation request
	PID_ClientActReq  = 0x13,

	// request to perform TCP simultaneous open
	PID_TCPSimOpen    = 0x14,

	// all data a client needs to get started
	PID_JoinData      = 0x15,

	// player info
	PID_PlayerInfoUpdReq = 0x16,

	// round results league info
	PID_LeagueRoundResults = 0x17,

	// * lobby
	PID_LobbyCountdown = 0x20,
	PID_SetScenarioParameter = 0x21, // scenario parameter update

	// * resources
	PID_NetResDis     = 0x30,
	PID_NetResStat    = 0x31,
	PID_NetResDerive  = 0x32,
	PID_NetResReq     = 0x33,
	PID_NetResData    = 0x34,

	// * control
	PID_Control       = 0x40,
	PID_ControlReq    = 0x41,
	PID_ControlPkt    = 0x42,
	PID_ExecSyncCtrl  = 0x43,

	// *** control
	CID_First         = 0x80,

	CID_ClientJoin    = CID_First | 0x00,
	CID_ClientUpdate  = CID_First | 0x01,
	CID_ClientRemove  = CID_First | 0x02,

	CID_Vote          = CID_First | 0x03,
	CID_VoteEnd       = CID_First | 0x04,

	CID_SyncCheck     = CID_First | 0x05,
	CID_Synchronize   = CID_First | 0x06,
	CID_Set           = CID_First | 0x07,
	CID_Script        = CID_First | 0x08,
	CID_MsgBoardReply = CID_First | 0x09,
	CID_MsgBoardCmd   = CID_First | 0x0A,

	CID_PlrInfo       = CID_First | 0x10,
	CID_JoinPlr       = CID_First | 0x11,
	CID_RemovePlr     = CID_First | 0x12,

	CID_PlrSelect     = CID_First | 0x20,
	CID_PlrControl    = CID_First | 0x21,

	CID_Message       = CID_First | 0x23,
	CID_PlrAction     = CID_First | 0x24,
	CID_PlrMouseMove  = CID_First | 0x25,

	CID_EMMoveObj     = CID_First | 0x30,
	CID_EMDrawTool    = CID_First | 0x31,
	CID_ReInitScenario= CID_First | 0x32,
	CID_EditGraph     = CID_First | 0x33,

	CID_DebugRec      = CID_First | 0x40,
	CID_MenuCommand   = CID_First | 0x41,

	// Note: There are some more packet types in src/netpuncher/C4PuncherPacket.h
	// They have been picked to be distinct from these for safety, not for necessary.
};

// packet classes
enum C4PacketClass
{
	PC_Network,                           // network packet - internal stuff
	PC_Control                            // control packet - game data (saved in records)
};

// enumeration of packet handlers
enum C4PacketHandlerID
{
	PH_C4Network2IO           = 1 << 0,   // network i/o class
	PH_C4Network2             = 1 << 1,   // main network class
	PH_C4GUIMainDlg           = 1 << 2,   // network lobby class
	PH_C4Network2ClientList   = 1 << 3,   // client list class
	PH_C4Network2Players      = 1 << 4,   // player list class
	PH_C4Network2ResList      = 1 << 5,   // resource list class
	PH_C4GameControlNetwork   = 1 << 6,   // network control class
};


// packet handling data
struct C4PktHandlingData
{
	C4PacketType ID;
	C4PacketClass Class;
	const char  *Name;

	bool AcceptedOnly;
	bool ProcByThread;

	int32_t HandlerID; // (C4PacketHandlerID)

	class C4PacketBase *(*FnUnpack)(StdCompiler *pComp);
};
extern const C4PktHandlingData PktHandlingData[];

const char *PacketNameByID(C4PacketType eID);


// *** general packet types

// raw packet: contains std buffer
class C4PktBuf : public C4PacketBase
{
protected:
	StdCopyBuf Data;
public:
	C4PktBuf();
	C4PktBuf(const C4PktBuf &rCopy) { *this = rCopy; }
	C4PktBuf(const StdBuf &rCpyData) { *this = rCpyData; }
	C4PktBuf &operator =(const C4PktBuf &rCopy);
	C4PktBuf &operator =(const StdBuf &rCopy);

	size_t getSize() const { return Data.getSize(); }
	const void *getData() const { return Data.getData(); }

	void CompileFunc(StdCompiler *pComp) override;
};

// "identified" packet: packet with packet type id
class C4IDPacket : public C4PacketBase
{
	friend class C4PacketList;
public:
	C4IDPacket();
	C4IDPacket(C4PacketType eID, C4PacketBase *pPkt, bool fTakePkt = true);
	C4IDPacket(const C4IDPacket &Packet2);
	~C4IDPacket() override;

protected:
	C4PacketType eID{PID_None};
	C4PacketBase *pPkt{nullptr};
	bool fOwnPkt{true};

	// used by C4PacketList
	C4IDPacket *pNext{nullptr};

public:
	C4PacketType  getPktType() const { return eID; }
	C4PacketBase *getPkt()     const { return pPkt; }
	const char   *getPktName() const;

	void Clear();
	void Default();
	void Set(C4PacketType eType, C4PacketBase *pPkt);

	void CompileFunc(StdCompiler *pComp) override;
};

// list of identified packets
class C4PacketList : public C4PacketBase
{
public:
	C4PacketList();
	C4PacketList(const C4PacketList &List2);
	~C4PacketList() override;

protected:
	C4IDPacket *pFirst{nullptr}, *pLast{nullptr};

public:
	C4IDPacket *firstPkt() const { return pFirst; }
	C4IDPacket *nextPkt(C4IDPacket *pPkt) const { return pPkt->pNext; }

	int32_t getPktCnt() const;

	void Add(C4IDPacket *pPkt);
	void AddHead(C4IDPacket *pPkt);
	void Add(C4PacketType eType, C4PacketBase *pPkt);
	void AddHead(C4PacketType eType, C4PacketBase *pPkt);

	void Take(C4PacketList &List);
	void Append(const C4PacketList &List);

	void Clear();
	void Remove(C4IDPacket *pPkt);
	void Delete(C4IDPacket *pPkt);

	void CompileFunc(StdCompiler *pComp) override;
};
#endif // INC_C4PacketBase
