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
#include "network/C4Network2Reference.h"

#include "C4Version.h"
#include "control/C4RoundResults.h"
#include "game/C4Application.h"

// *** C4Network2Reference

C4Network2Reference::C4Network2Reference()
		:  GameMode(), NetpuncherGameID(C4NetpuncherID())
{

}

C4Network2Reference::~C4Network2Reference() = default;

void C4Network2Reference::SetSourceAddress(const C4NetIO::EndpointAddress &ip)
{
	source = ip;
	if (iAddrCnt < C4ClientMaxAddr)
		Addrs[++iAddrCnt].SetAddr(ip);
}

void C4Network2Reference::InitLocal()
{
	// Copy all game parameters
	Parameters = ::Game.Parameters;

	// Discard player resources (we don't want these infos in the reference)
	// Add league performance (but only after game end)
	C4ClientPlayerInfos *pClientInfos; C4PlayerInfo *pPlayerInfo;
	int32_t i, j;
	for (i = 0; (pClientInfos = Parameters.PlayerInfos.GetIndexedInfo(i)); i++)
		for (j = 0; (pPlayerInfo = pClientInfos->GetPlayerInfo(j)); j++)
		{
			pPlayerInfo->DiscardResource();
			if(::Game.GameOver)
				pPlayerInfo->SetLeaguePerformance(::Game.RoundResults.GetLeaguePerformance(pPlayerInfo->GetID()));
		}

	// Special additional information in reference
	Icon = ::Game.C4S.Head.Icon;
	Title.CopyValidated(::Game.ScenarioTitle);
	GameMode = ::Game.C4S.Game.Mode;
	GameStatus = ::Network.Status;
	Time = ::Game.Time;
	Frame = ::Game.FrameCounter;
	StartTime = ::Game.StartTime;
	LeaguePerformance = ::Game.RoundResults.GetLeaguePerformance();
	Comment = Config.Network.Comment;
	JoinAllowed = ::Network.isJoinAllowed();
	ObservingAllowed = ::Network.isObservingAllowed();
	PasswordNeeded = ::Network.isPassworded();
	IsEditor = !!::Application.isEditor;
	NetpuncherGameID = ::Network.getNetpuncherGameID();
	NetpuncherAddr = ::Network.getNetpuncherAddr();
	Statistics = ::Game.RoundResults.GetStatistics();
	Game.Set();

	// Addresses
	C4Network2Client *pLocalNetClient = ::Game.Clients.getLocal()->getNetClient();
	iAddrCnt = pLocalNetClient->getAddrCnt();
	for (i = 0; i < iAddrCnt; i++)
		Addrs[i] = pLocalNetClient->getAddr(i);

}

void C4Network2Reference::SortNullIPsBack()
{
	// Sort all addresses with zero IP to back of list
	int iSortAddrCnt = iAddrCnt;
	for (int i = 0; i < iSortAddrCnt; i++)
		if (Addrs[i].isIPNull())
		{
			C4Network2Address Addr = Addrs[i];
			for (int j = i + 1; j < iAddrCnt; j++)
				Addrs[j - 1] = Addrs[j];
			Addrs[iAddrCnt - 1] = Addr;
			// Correct position
			i--; iSortAddrCnt--;
		}
}

void C4Network2Reference::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Icon,              "Icon",             0));
	pComp->Value(mkNamingAdapt(Title,             "Title",            "No title"));
	pComp->Value(mkNamingAdapt(mkParAdapt(GameMode, StdCompiler::RCT_IdtfAllowEmpty), "GameMode", ""));
	pComp->Value(mkParAdapt(GameStatus, true));
	pComp->Value(mkNamingAdapt(Time,              "Time",             0));
	pComp->Value(mkNamingAdapt(Frame,             "Frame",            0));
	pComp->Value(mkNamingAdapt(StartTime,         "StartTime",        0));
	pComp->Value(mkNamingAdapt(LeaguePerformance, "LeaguePerformance",0));
	pComp->Value(mkNamingAdapt(Comment,           "Comment",          ""));
	pComp->Value(mkNamingAdapt(JoinAllowed,       "JoinAllowed",      true));
	pComp->Value(mkNamingAdapt(ObservingAllowed,  "ObservingAllowed", true));
	pComp->Value(mkNamingAdapt(PasswordNeeded,    "PasswordNeeded",   false));
	pComp->Value(mkNamingAdapt(IsEditor,          "IsEditor",         false));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iAddrCnt), "AddressCount", 0));
	iAddrCnt = std::min<uint8_t>(C4ClientMaxAddr, iAddrCnt);
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Addrs, iAddrCnt, C4Network2Address()), "Address"));
	pComp->Value(mkNamingAdapt(Game.sEngineName,      "Game",             "None"));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Game.iVer,0),"Version"    ));
	pComp->Value(mkNamingAdapt(OfficialServer,  "OfficialServer", false));
	pComp->Value(mkNamingAdapt(NetpuncherGameID,  "NetpuncherID", C4NetpuncherID(), false, false));
	pComp->Value(mkNamingAdapt(NetpuncherAddr,  "NetpuncherAddr", "", false, false));
	pComp->Value(mkNamingAdapt(mkParAdapt(Statistics, StdCompiler::RCT_All), "Statistics", "", false, false));

	pComp->Value(Parameters);
}

int32_t C4Network2Reference::getSortOrder() const // Don't go over 100, because that's for the masterserver...
{
	C4GameVersion verThis;
	int iOrder = 0;
	// Official server
	if (isOfficialServer() && !Config.Network.UseAlternateServer) iOrder += 50;
	// Joinable
	if (isJoinAllowed() && (getGameVersion() == verThis)) iOrder += 25;
	// League game
	if (Parameters.isLeague()) iOrder += 5;
	// In lobby
	if (getGameStatus().isLobbyActive()) iOrder += 3;
	// No password needed
	if (!isPasswordNeeded()) iOrder += 1;
	// Done
	return iOrder;
}

StdStrBuf C4Network2Reference::getGameGoalString() const
{
	if (GameMode.getLength() > 0)
	{
		// Prefer to derive string from game mode
		return FormatString("%s: %s", LoadResStr("IDS_MENU_CPGOALS"), GameMode.getData());
	}
	else
	{
		// If not defined, fall back to goal string
		return Parameters.GetGameGoalString();
	}
}


// *** C4Network2RefServer

C4Network2RefServer::C4Network2RefServer() = default;

C4Network2RefServer::~C4Network2RefServer()
{
	Clear();
}

void C4Network2RefServer::Clear()
{
	C4NetIOTCP::Close();
	delete pReference; pReference = nullptr;
}

void C4Network2RefServer::SetReference(C4Network2Reference *pNewReference)
{
	CStdLock RefLock(&RefCSec);
	delete pReference; pReference = pNewReference;
}

void C4Network2RefServer::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Just append the packet
	rOutBuf.Append(rPacket);
}

size_t C4Network2RefServer::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	const char *pData = getBufPtr<char>(rInBuf);
	// Check for complete header
	const char *pHeaderEnd = strstr(pData, "\r\n\r\n");
	if (!pHeaderEnd)
		return 0;
	// Check method (only GET is allowed for now)
	if (!SEqual2(pData, "GET "))
	{
		RespondNotImplemented(addr, "Method not implemented");
		return rInBuf.getSize();
	}
	// Check target
	// TODO
	RespondReference(addr);
	return rInBuf.getSize();
}

void C4Network2RefServer::RespondNotImplemented(const C4NetIO::addr_t &addr, const char *szMessage)
{
	// Send the message
	StdStrBuf Data = FormatString("HTTP/1.0 501 %s\r\n\r\n", szMessage);
	Send(C4NetIOPacket(Data.getData(), Data.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

void C4Network2RefServer::RespondReference(const C4NetIO::addr_t &addr)
{
	CStdLock RefLock(&RefCSec);
	// Pack
	StdStrBuf PacketData = DecompileToBuf<StdCompilerINIWrite>(mkNamingPtrAdapt(pReference, "Reference"));
	// Create header
	StdStrBuf Header = FormatString(
	                     "HTTP/1.1 200 Found\r\n"
	                     "Content-Length: %lu\r\n"
	                     "Content-Type: text/plain; charset=UTF-8\r\n"
	                     "Server: " C4ENGINENAME "/" C4VERSION "\r\n"
	                     "\r\n",
	                     static_cast<unsigned long>(PacketData.getLength()));
	// Send back
	Send(C4NetIOPacket(Header, Header.getLength(), false, addr));
	Send(C4NetIOPacket(PacketData, PacketData.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

// *** C4Network2UpdateClient

bool C4Network2UpdateClient::QueryUpdateURL()
{
	// Perform an Query query
	return Query(nullptr, false);
}

bool C4Network2UpdateClient::GetUpdateURL(StdStrBuf *pUpdateURL)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	// Parse response
	try
	{
		CompileFromBuf<StdCompilerINIRead>(mkNamingAdapt(
		                                     mkNamingAdapt(mkParAdapt(*pUpdateURL, StdCompiler::RCT_All), "UpdateURL", ""),
		                                     C4ENGINENAME), ResultString);
	}
	catch (StdCompiler::Exception *pExc)
	{
		SetError(pExc->Msg.getData());
		return false;
	}
	// done; version OK!
	return true;
}

bool C4Network2UpdateClient::GetVersion(StdStrBuf *pVersion)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	// Parse response
	try
	{
		CompileFromBuf<StdCompilerINIRead>(mkNamingAdapt(
		                                     mkNamingAdapt(mkParAdapt(*pVersion, StdCompiler::RCT_All), "Version", ""),
		                                     C4ENGINENAME), ResultString);
	}
	catch (StdCompiler::Exception *pExc)
	{
		SetError(pExc->Msg.getData());
		return false;
	}
	// done; version OK!
	return true;
}

// *** C4Network2RefClient

bool C4Network2RefClient::QueryReferences()
{
	// Perform an Query query
	return Query(nullptr, false);
}

bool C4Network2RefClient::GetReferences(C4Network2Reference **&rpReferences, int32_t &rRefCount)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	// local update test
	try
	{
		// Create compiler
		StdCompilerINIRead Comp;
		Comp.setInput(ResultString);
		Comp.Begin();
		// Read reference count
		Comp.Value(mkNamingCountAdapt(rRefCount, "Reference"));
		// Create reference array and initialize
		rpReferences = new C4Network2Reference *[rRefCount];
		for (int i = 0; i < rRefCount; i++)
			rpReferences[i] = nullptr;
		// Get references
		Comp.Value(mkNamingAdapt(mkArrayAdaptMap(rpReferences, rRefCount, mkPtrAdaptNoNull<C4Network2Reference>), "Reference"));
		mkPtrAdaptNoNull<C4Network2Reference>(*rpReferences);
		// Done
		Comp.End();
	}
	catch (StdCompiler::Exception *pExc)
	{
		SetError(pExc->Msg.getData());
		return false;
	}
	// Set source ip
	for (int i = 0; i < rRefCount; i++)
		rpReferences[i]->SetSourceAddress(getServerAddress());
	// Done
	ResetError();
	return true;
}

