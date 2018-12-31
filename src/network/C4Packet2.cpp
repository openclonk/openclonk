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

#include "C4Version.h"
#include "control/C4RoundResults.h"
#include "gui/C4GameLobby.h"
#include "network/C4GameControlNetwork.h"
#include "network/C4Network2.h"
#include "network/C4Network2Res.h"

// *** constants

// workaround
template <class T> struct unpack_class
{
	static C4PacketBase *unpack(StdCompiler *pComp)
	{
		assert(pComp->isDeserializer());
		T *pPkt = new T();
		try
		{
			pComp->Value(*pPkt);
		}
		catch (...)
		{
			delete pPkt;
			throw;
		}
		return pPkt;
	}
};

#define PKT_UNPACK(T) unpack_class<T>::unpack


const C4PktHandlingData PktHandlingData[] =
{

	// C4Network2IO (network thread)
	{ PID_Conn,         PC_Network, "Connection Request",         true,   true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketConn)        },
	{ PID_ConnRe,       PC_Network, "Connection Request Reply",   true,   true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketConnRe)      },

	{ PID_Ping,         PC_Network, "Ping",                       true,   true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketPing)        },
	{ PID_Pong,         PC_Network, "Pong",                       true,   true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketPing)        },

	{ PID_FwdReq,       PC_Network, "Forward Request",            false,  true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketFwd)         },
	{ PID_Fwd,          PC_Network, "Forward",                    false,  true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketFwd)         },

	{ PID_PostMortem,   PC_Network, "Post Mortem",                false,  true,   PH_C4Network2IO,          PKT_UNPACK(C4PacketPostMortem)  },

	// C4Network2 (main thread)
	{ PID_Conn,         PC_Network, "Connection Request",         true,   false,  PH_C4Network2,            PKT_UNPACK(C4PacketConn)        },
	{ PID_ConnRe,       PC_Network, "Connection Request Reply",   true,   false,  PH_C4Network2,            PKT_UNPACK(C4PacketConnRe)      },

	{ PID_Status,       PC_Network, "Game Status",                true,   false,  PH_C4Network2,            PKT_UNPACK(C4Network2Status)    },
	{ PID_StatusAck,    PC_Network, "Game Status Acknowledgement",true,   false,  PH_C4Network2,            PKT_UNPACK(C4Network2Status)    },

	{ PID_ClientActReq, PC_Network, "Client Activation Request",  false,  false,  PH_C4Network2,            PKT_UNPACK(C4PacketActivateReq) },

	{ PID_JoinData,     PC_Network, "Join Data",                  false,  false,  PH_C4Network2,            PKT_UNPACK(C4PacketJoinData)    },

	// C4Network2PlayerList (main thread)
	{ PID_PlayerInfoUpdReq,PC_Network, "Player info update request",true, false,  PH_C4Network2Players,     PKT_UNPACK(C4PacketPlayerInfoUpdRequest)  },

	{ PID_LeagueRoundResults,PC_Network, "League round results",   true,  false,  PH_C4Network2Players,     PKT_UNPACK(C4PacketLeagueRoundResults)  },

	// C4GameLobby (main thread)
	{ PID_LobbyCountdown,PC_Network,"Lobby countdown",            false,  false,  PH_C4GUIMainDlg,          PKT_UNPACK(C4GameLobby::C4PacketCountdown)  },
	{ PID_SetScenarioParameter,PC_Network,"Scenario parameter",   false,  false,  PH_C4GUIMainDlg,          PKT_UNPACK(C4GameLobby::C4PacketSetScenarioParameter)  },

	// C4Network2ClientList (main thread)
	{ PID_Addr,         PC_Network, "Client Address",             false,  false,  PH_C4Network2ClientList,  PKT_UNPACK(C4PacketAddr)        },
	{ PID_TCPSimOpen,   PC_Network, "TCP simultaneous open req",  false,  false,  PH_C4Network2ClientList,  PKT_UNPACK(C4PacketTCPSimOpen)  },


	// C4Network2ResList (network thread)
	{ PID_NetResDis,    PC_Network, "Resource Discover",          true,   true,   PH_C4Network2ResList,     PKT_UNPACK(C4PacketResDiscover) },
	{ PID_NetResStat,   PC_Network, "Resource Status",            false,  true,   PH_C4Network2ResList,     PKT_UNPACK(C4PacketResStatus)   },
	{ PID_NetResDerive, PC_Network, "Resource Derive",            false,  true,   PH_C4Network2ResList,     PKT_UNPACK(C4Network2ResCore)   },
	{ PID_NetResReq,    PC_Network, "Resource Request",           false,  true,   PH_C4Network2ResList,     PKT_UNPACK(C4PacketResRequest)  },
	{ PID_NetResData,   PC_Network, "Resource Data",              false,  true,   PH_C4Network2ResList,     PKT_UNPACK(C4Network2ResChunk)  },

	// C4GameControlNetwork (network thread)
	{ PID_Control,      PC_Network, "Control",                    false,  true,   PH_C4GameControlNetwork,  PKT_UNPACK(C4GameControlPacket) },
	{ PID_ControlReq,   PC_Network, "Control Request",            false,  true,   PH_C4GameControlNetwork,  PKT_UNPACK(C4PacketControlReq)  },
	//                       main thread
	{ PID_ControlPkt,   PC_Network, "Control Paket",              false,  false,  PH_C4GameControlNetwork,  PKT_UNPACK(C4PacketControlPkt)  },
	{ PID_ExecSyncCtrl, PC_Network, "Execute Sync Control",       false,  false,  PH_C4GameControlNetwork,  PKT_UNPACK(C4PacketExecSyncCtrl)},

	// Control (Isn't send over network, handled only as part of a control list)
	{ CID_ClientJoin,   PC_Control, "Client Join",                false,  true,   0,                        PKT_UNPACK(C4ControlClientJoin) },
	{ CID_ClientUpdate, PC_Control, "Client Update",              false,  true,   0,                        PKT_UNPACK(C4ControlClientUpdate)},
	{ CID_ClientRemove, PC_Control, "Client Remove",              false,  true,   0,                        PKT_UNPACK(C4ControlClientRemove)},
	{ CID_Vote,         PC_Control, "Voting",                     false,  true,   0,                        PKT_UNPACK(C4ControlVote)       },
	{ CID_VoteEnd,      PC_Control, "Voting End",                 false,  true,   0,                        PKT_UNPACK(C4ControlVoteEnd)    },
	{ CID_SyncCheck,    PC_Control, "Sync Check",                 false,  true,   0,                        PKT_UNPACK(C4ControlSyncCheck)  },
	{ CID_Synchronize,  PC_Control, "Synchronize",                false,  true,   0,                        PKT_UNPACK(C4ControlSynchronize)},
	{ CID_Set,          PC_Control, "Set",                        false,  true,   0,                        PKT_UNPACK(C4ControlSet)        },
	{ CID_Script,       PC_Control, "Script",                     false,  true,   0,                        PKT_UNPACK(C4ControlScript)     },
	{ CID_MsgBoardReply,PC_Control, "Message Board Reply",        false,  true,   0,                        PKT_UNPACK(C4ControlMsgBoardReply)},
	{ CID_MsgBoardCmd  ,PC_Control, "Message Board Command",      false,  true,   0,                        PKT_UNPACK(C4ControlMsgBoardCmd)},
	{ CID_PlrInfo,      PC_Control, "Player Info",                false,  true,   0,                        PKT_UNPACK(C4ControlPlayerInfo) },
	{ CID_JoinPlr,      PC_Control, "Join Player",                false,  true,   0,                        PKT_UNPACK(C4ControlJoinPlayer) },
	{ CID_RemovePlr,    PC_Control, "Remove Player",              false,  true,   0,                        PKT_UNPACK(C4ControlRemovePlr)  },
	{ CID_PlrSelect,    PC_Control, "Player Select",              false,  true,   0,                        PKT_UNPACK(C4ControlPlayerSelect)},
	{ CID_PlrControl,   PC_Control, "Player Control",             false,  true,   0,                        PKT_UNPACK(C4ControlPlayerControl)},
	{ CID_PlrAction,    PC_Control, "Player Self-Mgmt Action",    false,  true,   0,                        PKT_UNPACK(C4ControlPlayerAction)},
	{ CID_PlrMouseMove, PC_Control, "Player Mouse Movement",      false,  true,   0,                        PKT_UNPACK(C4ControlPlayerMouse)},
	{ CID_Message,      PC_Control, "Message",                    false,  true,   0,                        PKT_UNPACK(C4ControlMessage)    },
	{ CID_MenuCommand,   PC_Control, "Menu Command",              false,  true,   0,                        PKT_UNPACK(C4ControlMenuCommand)},
	{ CID_EMMoveObj,    PC_Control, "EM Move Obj",                false,  true,   0,                        PKT_UNPACK(C4ControlEMMoveObject)},
	{ CID_EMDrawTool,   PC_Control, "EM Draw Tool",               false,  true,   0,                        PKT_UNPACK(C4ControlEMDrawTool) },
	{ CID_ReInitScenario,PC_Control, "Reinit Scenario",           false,  true,   0,                        PKT_UNPACK(C4ControlReInitScenario) },
	{ CID_EditGraph,    PC_Control, "Edit Graph",                 false,  true,   0,                        PKT_UNPACK(C4ControlEditGraph)  },

	{ CID_DebugRec,     PC_Control, "Debug Rec",                  false,  true,   0,                        PKT_UNPACK(C4ControlDebugRec)   },

	// EOL
	{ PID_None,         PC_Network, nullptr,                         false,  true,   0,                        nullptr                            }
};

const char *PacketNameByID(C4PacketType eID)
{
	for (const C4PktHandlingData *pPData = PktHandlingData; pPData->ID != PID_None; pPData++)
		if (pPData->ID == eID)
			return pPData->Name;
	return "?!?";
}

// *** C4PacketBase

C4PacketBase::C4PacketBase() = default;

C4PacketBase::~C4PacketBase() = default;

C4NetIOPacket C4PacketBase::pack(const C4NetIO::addr_t &addr) const
{
	return C4NetIOPacket(DecompileToBuf<StdCompilerBinWrite>(*this), addr);
}

C4NetIOPacket C4PacketBase::pack(uint8_t cStatus, const C4NetIO::addr_t &addr) const
{
	return C4NetIOPacket(DecompileToBuf<StdCompilerBinWrite>(mkInsertAdapt(mkDecompileAdapt(*this), cStatus)), addr);
}

void C4PacketBase::unpack(const C4NetIOPacket &Pkt, char *pStatus)
{
	if (pStatus) *pStatus = Pkt.getStatus();
	CompileFromBuf<StdCompilerBinRead>(*this, pStatus ? Pkt.getPBuf() : Pkt.getRef());
}


// *** C4PktBuf

C4PktBuf::C4PktBuf() = default;

C4PktBuf::C4PktBuf(const C4PktBuf &rCopy) : C4PacketBase(rCopy)
{
	Data.Copy(rCopy.Data);
}

C4PktBuf::C4PktBuf(const StdBuf &rCpyData)
{
	Data.Copy(rCpyData);
}

void C4PktBuf::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Data, "Data"));
}

// *** C4IDPacket

C4IDPacket::C4IDPacket() = default;

C4IDPacket::C4IDPacket(C4PacketType eID, C4PacketBase *pPkt, bool fTakePkt)
		: eID(eID), pPkt(pPkt), fOwnPkt(fTakePkt), pNext(nullptr)
{

}

C4IDPacket::C4IDPacket(const C4IDPacket &Packet2)
		: C4PacketBase(Packet2)
{
	// kinda hacky (note this might throw an uncaught exception)
	C4PacketBase::unpack(Packet2.C4PacketBase::pack());
}

C4IDPacket::~C4IDPacket()
{
	Clear();
}

const char *C4IDPacket::getPktName() const
{
	// Use map
	for (const C4PktHandlingData *pPData = PktHandlingData; pPData->ID != PID_None; pPData++)
		if (pPData->ID == eID && pPData->Name)
			return pPData->Name;
	return "Unknown Packet Type";
}

void C4IDPacket::Default()
{
	eID = PID_None; pPkt = nullptr;
}

void C4IDPacket::Clear()
{
	if (fOwnPkt) delete pPkt;
	pPkt = nullptr;
	eID = PID_None;
}

void C4IDPacket::CompileFunc(StdCompiler *pComp)
{
	// Packet ID
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eID), "ID", PID_None));
	// Compiling or Decompiling?
	if (pComp->isDeserializer())
	{
		if (!pComp->Name(getPktName()))
			{ pComp->excCorrupt("C4IDPacket: Data value needed! Packet data missing!"); return; }
		// Delete old packet
		if (fOwnPkt) delete pPkt;
		pPkt = nullptr;
		if (eID == PID_None) return;
		// Search unpacking function
		for (const C4PktHandlingData *pPData = PktHandlingData; pPData->ID != PID_None; pPData++)
			if (pPData->ID == eID && pPData->FnUnpack)
			{
				pPkt = pPData->FnUnpack(pComp);
				break;
			}
		if (!pPkt)
			pComp->excCorrupt("C4IDPacket: could not unpack packet id %02x!", eID);
		pComp->NameEnd();
	}
	else if (eID != PID_None)
		// Just write
		pComp->Value(mkNamingAdapt(*pPkt, getPktName()));
}

// *** C4PacketList

C4PacketList::C4PacketList() = default;

C4PacketList::C4PacketList(const C4PacketList &List2)
		: C4PacketBase(List2),
		pFirst(nullptr), pLast(nullptr)
{
	Append(List2);
}

C4PacketList::~C4PacketList()
{
	Clear();
}

int32_t C4PacketList::getPktCnt() const
{
	int32_t iCnt = 0;
	for (C4IDPacket *pPkt = pFirst; pPkt; pPkt = pPkt->pNext)
		iCnt++;
	return iCnt;
}

void C4PacketList::Add(C4IDPacket *pPkt)
{
	assert(!pPkt->pNext);
	(pLast ? pLast->pNext : pFirst) = pPkt;
	pLast = pPkt;
}

void C4PacketList::AddHead(C4IDPacket *pPkt)
{
	assert(!pPkt->pNext);
	pPkt->pNext = pFirst;
	pFirst = pPkt;
	if (!pLast) pLast = pPkt;
}

void C4PacketList::Add(C4PacketType eType, C4PacketBase *pPkt)
{
	Add(new C4IDPacket(eType, pPkt));
}

void C4PacketList::AddHead(C4PacketType eType, C4PacketBase *pPkt)
{
	AddHead(new C4IDPacket(eType, pPkt));
}

void C4PacketList::Take(C4PacketList &List)
{
	pFirst = List.pFirst;
	pLast = List.pLast;
	List.pFirst = List.pLast = nullptr;
}

void C4PacketList::Append(const C4PacketList &List)
{
	for (C4IDPacket *pPkt = List.firstPkt(); pPkt; pPkt = List.nextPkt(pPkt))
		Add(new C4IDPacket(*pPkt));
}

void C4PacketList::Clear()
{
	while (pFirst)
		Delete(pFirst);
}

void C4PacketList::Remove(C4IDPacket *pPkt)
{
	if (pPkt == pFirst)
	{
		pFirst = pPkt->pNext;
		if (pPkt == pLast)
			pLast = nullptr;
	}
	else
	{
		C4IDPacket *pPrev;
		for (pPrev = pFirst; pPrev && pPrev->pNext != pPkt; pPrev = pPrev->pNext) {}
		if (pPrev)
		{
			pPrev->pNext = pPkt->pNext;
			if (pPkt == pLast)
				pLast = pPrev;
		}
	}
}

void C4PacketList::Delete(C4IDPacket *pPkt)
{
	Remove(pPkt);
	delete pPkt;
}

void C4PacketList::CompileFunc(StdCompiler *pComp)
{
	// unpack packets
	if (pComp->isDeserializer())
	{
		// Read until no further sections available
		while (pComp->Name("IDPacket"))
		{
			// Read the packet
			C4IDPacket *pPkt = new C4IDPacket();
			try
			{
				pComp->Value(*pPkt);
				pComp->NameEnd();
			}
			catch (...)
			{
				delete pPkt;
				throw;
			}
			// Terminator?
			if (!pPkt->getPkt()) { delete pPkt; break; }
			// Add to list
			Add(pPkt);
		}
		pComp->NameEnd();
	}
	else
	{
		// Write all packets
		for (C4IDPacket *pPkt = pFirst; pPkt; pPkt = pPkt->pNext)
			pComp->Value(mkNamingAdapt(*pPkt, "IDPacket"));
		// Terminate, if no naming is available
		if (!pComp->hasNaming())
		{
			C4IDPacket Pkt;
			pComp->Value(mkNamingAdapt(Pkt, "IDPacket"));
		}
	}
}

// *** C4PacketConn

C4PacketConn::C4PacketConn()
		: iVer(C4XVER1*100 + C4XVER2)
{
}

C4PacketConn::C4PacketConn(const C4ClientCore &nCCore, uint32_t inConnID, const char *szPassword)
		: iVer(C4XVER1*100 + C4XVER2),
		iConnID(inConnID),
		CCore(nCCore),
		Password(szPassword)
{
}

void C4PacketConn::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(CCore, "CCore"));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iVer), "Version", -1));
	pComp->Value(mkNamingAdapt(Password, "Password", ""));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iConnID), "ConnID", ~0u));
}

// *** C4PacketConnRe

C4PacketConnRe::C4PacketConnRe() = default;

C4PacketConnRe::C4PacketConnRe(bool fnOK, bool fWrongPassword, const char *sznMsg)
		: fOK(fnOK),
		fWrongPassword(fWrongPassword),
		szMsg(sznMsg, true)
{
}

void C4PacketConnRe::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(fOK, "OK", true));
	pComp->Value(mkNamingAdapt(szMsg, "Message", ""));
	pComp->Value(mkNamingAdapt(fWrongPassword, "WrongPassword", false));
}

// *** C4PacketFwd

C4PacketFwd::C4PacketFwd() = default;

C4PacketFwd::C4PacketFwd(const StdBuf &Pkt)
		: Data(Pkt)
{
}

bool C4PacketFwd::DoFwdTo(int32_t iClient) const
{
	for (int32_t i = 0; i < iClientCnt; i++)
		if (iClients[i] == iClient)
			return !fNegativeList;
	return fNegativeList;
}

void C4PacketFwd::SetData(const StdBuf &Pkt)
{
	Data = Pkt;
}

void C4PacketFwd::SetListType(bool fnNegativeList)
{
	fNegativeList = fnNegativeList;
}

void C4PacketFwd::AddClient(int32_t iClient)
{
	if (iClientCnt + 1 > C4NetMaxClients) return;
	// add
	iClients[iClientCnt++] = iClient;
}

void C4PacketFwd::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(fNegativeList, "Negative", false));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iClientCnt), "ClientCnt", 0));
	pComp->Value(mkNamingAdapt(mkArrayAdaptMap(iClients, iClientCnt, mkIntPackAdapt<int32_t>), "Clients", -1));
	pComp->Value(mkNamingAdapt(Data, "Data"));
}

// *** C4PacketJoinData

void C4PacketJoinData::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iClientID), "ClientID", C4ClientIDUnknown));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iStartCtrlTick), "CtrlTick", -1));
	pComp->Value(mkNamingAdapt(mkParAdapt(GameStatus, true), "GameStatus"));
	pComp->Value(mkNamingAdapt(Dynamic, "Dynamic"));
	pComp->Value(mkNamingAdapt(ScenarioParameterDefs, "ParameterDefs"));
	pComp->Value(Parameters);
}

// *** C4PacketPing

C4PacketPing::C4PacketPing(uint32_t iPacketCounter, uint32_t iRemotePacketCounter)
		: tTime(C4TimeMilliseconds::Now()),
		iPacketCounter(iPacketCounter)
{
}

uint32_t C4PacketPing::getTravelTime() const
{
	return C4TimeMilliseconds::Now() - tTime;
}

void C4PacketPing::CompileFunc(StdCompiler *pComp)
{
	uint32_t time = tTime.AsInt();
	pComp->Value(mkNamingAdapt(time, "Time", 0U));
	tTime = C4TimeMilliseconds(time);

	pComp->Value(mkNamingAdapt(iPacketCounter, "PacketCounter", 0U));
}

// *** C4PacketResStatus

C4PacketResStatus::C4PacketResStatus() = default;

C4PacketResStatus::C4PacketResStatus(int32_t iResID, const C4Network2ResChunkData &nChunks)
		: iResID(iResID), Chunks(nChunks)
{

}

void C4PacketResStatus::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iResID, "ResID"));
	pComp->Value(mkNamingAdapt(Chunks, "Chunks"));
}

// *** C4PacketResDiscover

C4PacketResDiscover::C4PacketResDiscover() = default;

bool C4PacketResDiscover::isIDPresent(int32_t iID) const
{
	for (int32_t i = 0; i < iDisIDCnt; i++)
		if (iDisIDs[i] == iID)
			return true;
	return false;
}

bool C4PacketResDiscover::AddDisID(int32_t iID)
{
	if (iDisIDCnt + 1 >= int32_t(sizeof(iDisIDs) / sizeof(*iDisIDs))) return false;
	// add
	iDisIDs[iDisIDCnt++] = iID;
	return true;
}

void C4PacketResDiscover::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iDisIDCnt), "DiscoverCnt", 0));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(iDisIDs, iDisIDCnt), "Discovers", -1));
}

// *** C4PacketResRequest

C4PacketResRequest::C4PacketResRequest(int32_t inID, int32_t inChunk)
		: iReqID(inID), iReqChunk(inChunk)
{

}

void C4PacketResRequest::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iReqID, "ResID", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iReqChunk), "Chunk", -1));
}

// *** C4PacketControlReq

C4PacketControlReq::C4PacketControlReq(int32_t inCtrlTick)
		: iCtrlTick(inCtrlTick)
{

}

void C4PacketControlReq::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iCtrlTick), "CtrlTick", -1));
}

// *** C4PacketActivateReq

void C4PacketActivateReq::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iTick), "Tick", -1));
}


// *** C4PacketControlPkt

void C4PacketControlPkt::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eDelivery), "Delivery", CDT_Queue));
	pComp->Value(Ctrl);
}
