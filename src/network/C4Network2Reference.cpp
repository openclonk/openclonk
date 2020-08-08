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

#include <zlib.h>

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

// *** C4Network2HTTPClient

C4Network2HTTPClient::C4Network2HTTPClient()
{
	C4NetIOTCP::SetCallback(this);
}

C4Network2HTTPClient::~C4Network2HTTPClient() = default;

void C4Network2HTTPClient::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Just append the packet
	rOutBuf.Append(rPacket);
}

size_t C4Network2HTTPClient::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	// since new data arrived, increase timeout time
	ResetRequestTimeout();
	// Check for complete header
	if (!iDataOffset)
	{
		// Copy data into string buffer (terminate)
		StdStrBuf Data; Data.Copy(getBufPtr<char>(rInBuf), rInBuf.getSize());
		const char *pData = Data.getData();
		// Header complete?
		const char *pContent = SSearch(pData, "\r\n\r\n");
		if (!pContent)
			return 0;
		// Read the header
		if (!ReadHeader(Data))
		{
			fBusy = fSuccess = false;
			Close(addr);
			return rInBuf.getSize();
		}
	}
	iDownloadedSize = rInBuf.getSize() - iDataOffset;
	// Check if the packet is complete
	if (iTotalSize > iDownloadedSize)
	{
		return 0;
	}
	// Get data, uncompress it if needed
	StdBuf Data = rInBuf.getPart(iDataOffset, iTotalSize);
	if (fCompressed)
		if (!Decompress(&Data))
		{
			fBusy = fSuccess = false;
			Close(addr);
			return rInBuf.getSize();
		}
	// Save the result
	if (fBinary)
		ResultBin.Copy(Data);
	else
		ResultString.Copy(getBufPtr<char>(Data), Data.getSize());
	fBusy = false; fSuccess = true;
	// Callback
	OnPacket(C4NetIOPacket(Data, addr), this);
	// Done
	Close(addr);
	return rInBuf.getSize();
}

bool C4Network2HTTPClient::ReadHeader(StdStrBuf Data)
{
	const char *pData = Data.getData();
	const char *pContent = SSearch(pData, "\r\n\r\n");
	if (!pContent)
		return false;
	// Parse header line
	int iHTTPVer1, iHTTPVer2, iResponseCode, iStatusStringPtr;
	if (sscanf(pData, "HTTP/%d.%d %d %n", &iHTTPVer1, &iHTTPVer2, &iResponseCode, &iStatusStringPtr) != 3)
	{
		Error = "Invalid status line!";
		return false;
	}
	// Check HTTP version
	if (iHTTPVer1 != 1)
	{
		Error.Format("Unsupported HTTP version: %d.%d!", iHTTPVer1, iHTTPVer2);
		return false;
	}
	// Check code
	if (iResponseCode != 200)
	{
		// Get status string
		StdStrBuf StatusString; StatusString.CopyUntil(pData + iStatusStringPtr, '\r');
		// Create error message
		Error.Format("HTTP server responded %d: %s", iResponseCode, StatusString.getData());
		return false;
	}
	// Get content length
	const char *pContentLength = SSearch(pData, "\r\nContent-Length:");
	int iContentLength;
	if (!pContentLength || pContentLength > pContent ||
	    sscanf(pContentLength, "%d", &iContentLength) != 1)
	{
		Error.Format("Invalid server response: Content-Length is missing!");
		return false;
	}
	iTotalSize = iContentLength;
	iDataOffset = (pContent - pData);
	// Get content encoding
	const char *pContentEncoding = SSearch(pData, "\r\nContent-Encoding:");
	if (pContentEncoding)
	{
		while (*pContentEncoding == ' ') pContentEncoding++;
		StdStrBuf Encoding; Encoding.CopyUntil(pContentEncoding, '\r');
		if (Encoding == "gzip")
			fCompressed = true;
		else
			fCompressed = false;
	}
	else
		fCompressed = false;
	// Okay
	return true;
}

bool C4Network2HTTPClient::Decompress(StdBuf *pData)
{
	size_t iSize = pData->getSize();
	// Create buffer
	uint32_t iOutSize = *getBufPtr<uint32_t>(*pData, pData->getSize() - sizeof(uint32_t));
	iOutSize = std::min<uint32_t>(iOutSize, iSize * 1000);
	StdBuf Out; Out.New(iOutSize);
	// Prepare stream
	z_stream zstrm;
	ZeroMem(&zstrm, sizeof(zstrm));
	zstrm.next_in = const_cast<Byte *>(getBufPtr<Byte>(*pData));
	zstrm.avail_in = pData->getSize();
	zstrm.next_out = getMBufPtr<Byte>(Out);
	zstrm.avail_out = Out.getSize();
	// Inflate...
	if (inflateInit2(&zstrm, 15 + 16) != Z_OK)
	{
		Error.Format("Could not decompress data!");
		return false;
	}
	// Inflate!
	if (inflate(&zstrm, Z_FINISH) != Z_STREAM_END)
	{
		inflateEnd(&zstrm);
		Error.Format("Could not decompress data!");
		return false;
	}
	// Return the buffer
	Out.SetSize(zstrm.total_out);
	pData->Take(std::move(Out));
	// Okay
	inflateEnd(&zstrm);
	return true;
}

bool C4Network2HTTPClient::OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const C4NetIO::addr_t *pOwnAddr, C4NetIO *pNetIO)
{
	// Make sure we're actually waiting for this connection
	if (fConnected || (AddrConnect != ServerAddr && AddrConnect != ServerAddrFallback))
		return false;
	// Save pack peer address
	PeerAddr = AddrPeer;
	// Send the request
	if (!Send(C4NetIOPacket(Request, AddrPeer)))
	{
		Error.Format("Unable to send HTTP request: %s", Error.getData());
	}
	Request.Clear();
	fConnected = true;
	return true;
}

void C4Network2HTTPClient::OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason)
{
	// Got no complete packet? Failure...
	if (!fSuccess && Error.isNull())
	{
		fBusy = false;
		Error.Format("Unexpected disconnect: %s", szReason);
	}
	fConnected = false;
	// Notify
	if (pNotify)
		pNotify->PushEvent(Ev_HTTP_Response, this);
}

void C4Network2HTTPClient::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
	// Everything worthwhile was already done in UnpackPacket. Only do notify callback
	if (pNotify)
		pNotify->PushEvent(Ev_HTTP_Response, this);
}

bool C4Network2HTTPClient::Execute(int iMaxTime)
{
	// Check timeout
	if (fBusy)
	{
		if (C4TimeMilliseconds::Now() > HappyEyeballsTimeout)
		{
			HappyEyeballsTimeout = C4TimeMilliseconds::PositiveInfinity;
			Application.InteractiveThread.ThreadLogS("HTTP: Starting fallback connection to %s (%s)", Server.getData(), ServerAddrFallback.ToString().getData());
			Connect(ServerAddrFallback);
		}
		if (time(nullptr) > iRequestTimeout)
		{
			Cancel("Request timeout");
			return true;
		}
	}
	// Execute normally
	return C4NetIOTCP::Execute(iMaxTime);
}

C4TimeMilliseconds C4Network2HTTPClient::GetNextTick(C4TimeMilliseconds tNow)
{
	C4TimeMilliseconds tNetIOTCPTick = C4NetIOTCP::GetNextTick(tNow);
	if (!fBusy)
		return tNetIOTCPTick;

	C4TimeMilliseconds tHTTPClientTick = tNow + 1000 * std::max<time_t>(iRequestTimeout - time(nullptr), 0);

	C4TimeMilliseconds HappyEyeballsTick = tNow + std::max(HappyEyeballsTimeout - C4TimeMilliseconds::Now(), 0);

	return std::min({tNetIOTCPTick, tHTTPClientTick, HappyEyeballsTick});
}

bool C4Network2HTTPClient::Query(const StdBuf &Data, bool fBinary)
{
	if (Server.isNull()) return false;
	// Cancel previous request
	if (fBusy)
		Cancel("Cancelled");
	// No result known yet
	ResultString.Clear();
	// store mode
	this->fBinary = fBinary;
	// Create request
	StdStrBuf Header;
	if (Data.getSize())
		Header.Format(
		  "POST %s HTTP/1.0\r\n"
		  "Host: %s\r\n"
		  "Connection: Close\r\n"
		  "%s"
		  "Content-Length: %lu\r\n"
		  "Content-Type: text/plain; charset=utf-8\r\n"
		  "Accept-Charset: utf-8\r\n"
		  "Accept-Encoding: gzip\r\n"
		  "Accept-Language: %s\r\n"
		  "User-Agent: " C4ENGINENAME "/" C4VERSION "\r\n"
		  "\r\n",
		  RequestPath.getData(),
		  Server.getData(),
		  headerAcceptedResponseType.c_str(),
		  static_cast<unsigned long>(Data.getSize()),
		  Config.General.LanguageEx);
	else
		Header.Format(
		  "GET %s HTTP/1.0\r\n"
		  "Host: %s\r\n"
		  "Connection: Close\r\n"
		  "%s"
		  "Accept-Charset: utf-8\r\n"
		  "Accept-Encoding: gzip\r\n"
		  "Accept-Language: %s\r\n"
		  "User-Agent: " C4ENGINENAME "/" C4VERSION "\r\n"
		  "\r\n",
		  RequestPath.getData(),
		  Server.getData(),
		  headerAcceptedResponseType.c_str(),
		  Config.General.LanguageEx);
	// Compose query
	Request.Take(Header.GrabPointer(), Header.getLength());
	Request.Append(Data);
	// Start connecting
	if (!Connect(ServerAddr))
		return false;
	// Also try the fallback address after some time (if there is one)
	if (!ServerAddrFallback.IsNull())
		HappyEyeballsTimeout = C4TimeMilliseconds::Now() + C4Network2HTTPHappyEyeballsTimeout;
	else
		HappyEyeballsTimeout = C4TimeMilliseconds::PositiveInfinity;
	// Okay, request will be performed when connection is complete
	fBusy = true;
	iDataOffset = 0;
	ResetRequestTimeout();
	ResetError();
	return true;
}

void C4Network2HTTPClient::ResetRequestTimeout()
{
	// timeout C4Network2HTTPQueryTimeout seconds from this point
	iRequestTimeout = time(nullptr) + C4Network2HTTPQueryTimeout;
}

void C4Network2HTTPClient::Cancel(const char *szReason)
{
	// Close connection - and connection attempt
	Close(ServerAddr); Close(ServerAddrFallback); Close(PeerAddr);
	// Reset flags
	fBusy = fSuccess = fConnected = fBinary = false;
	iDownloadedSize = iTotalSize = iDataOffset = 0;
	Error = szReason;
}

void C4Network2HTTPClient::Clear()
{
	fBusy = fSuccess = fConnected = fBinary = false;
	iDownloadedSize = iTotalSize = iDataOffset = 0;
	ResultBin.Clear();
	ResultString.Clear();
	Error.Clear();
}

bool C4Network2HTTPClient::SetServer(const char *szServerAddress)
{
	// Split address
	const char *pRequestPath;
	if ((pRequestPath = strchr(szServerAddress, '/')))
	{
		Server.CopyUntil(szServerAddress, '/');
		RequestPath = pRequestPath;
	}
	else
	{
		Server = szServerAddress;
		RequestPath = "/";
	}
	// Resolve address
	ServerAddr.SetAddress(Server);
	if (ServerAddr.IsNull())
	{
		SetError(FormatString("Could not resolve server address %s!", Server.getData()).getData());
		return false;
	}
	ServerAddr.SetDefaultPort(GetDefaultPort());
	if (ServerAddr.GetFamily() == C4NetIO::HostAddress::IPv6)
	{
		// Try to find a fallback IPv4 address for Happy Eyeballs.
		ServerAddrFallback.SetAddress(Server, C4NetIO::HostAddress::IPv4);
		ServerAddrFallback.SetDefaultPort(GetDefaultPort());
	}
	else
		ServerAddrFallback.Clear();
	// Remove port
	const char *firstColon = strchr(Server.getData(), ':');
	const char *lastColon = strrchr(Server.getData(), ':');
	if (firstColon)
		// hostname/IPv4 address    or IPv6 address with port (e.g. [::1]:1234)
		if (firstColon == lastColon || (Server[0] == '[' && *(lastColon - 1) == ']'))
			Server.SetLength(lastColon - Server.getData());
		
	// Done
	ResetError();
	return true;
}

void C4Network2HTTPClient::SetExpectedResponseType(C4Network2HTTPClient::ResponseType type)
{
	switch (type)
	{
	case C4Network2HTTPClient::ResponseType::XML:
		headerAcceptedResponseType = "Accept:application/xml\r\n";
		break;
	case C4Network2HTTPClient::ResponseType::NoPreference: // fallthrough
	default:
		headerAcceptedResponseType = "";
		break;
	};
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

