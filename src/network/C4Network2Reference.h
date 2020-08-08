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
#ifndef C4NETWORK2REFERENCE_H
#define C4NETWORK2REFERENCE_H

#include "C4Version.h"
#include "control/C4GameParameters.h"
#include "game/C4GameVersion.h"
#include "lib/C4InputValidation.h"
#include "network/C4Network2.h"
#include "network/C4Network2Client.h"

const int C4Network2HTTPQueryTimeout = 10; // (s)
const uint32_t C4Network2HTTPHappyEyeballsTimeout = 300; // (ms)

// Session data
class C4Network2Reference
{
public:
	C4Network2Reference();
	~C4Network2Reference();

	// Game parameters
	C4GameParameters Parameters;

private:
	// General information
	int32_t Icon{0};
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameExNoEmpty> Title;
	StdCopyStrBuf GameMode; // Game mode (e.g. "Melee") to decide the correct league for evaluation
	C4Network2Status GameStatus;
	int32_t Time{0};
	int32_t Frame{0};
	int32_t StartTime{0};
	int32_t LeaguePerformance{0}; // custom settlement league performance if scenario doesn't use elapsed frames
	ValidatedStdCopyStrBuf<C4InVal::VAL_Comment> Comment;
	bool JoinAllowed{true};
	bool ObservingAllowed{true};
	bool PasswordNeeded{false};
	bool OfficialServer{false};
	bool IsEditor{false};
	C4NetpuncherID NetpuncherGameID;
	StdCopyStrBuf NetpuncherAddr;
	StdCopyStrBuf Statistics;

	// Engine information
	C4GameVersion Game;

	// Network addresses
	uint8_t iAddrCnt{0};
	C4Network2Address Addrs[C4ClientMaxAddr];
	C4NetIO::EndpointAddress source;

public:
	const C4Network2Address &getAddr(int i) const { return Addrs[i]; }
	C4Network2Address &getAddr(int i) { return Addrs[i]; }
	int getAddrCnt() const { return iAddrCnt; }
	const char *getTitle() const { return Title.getData(); }
	int32_t getIcon() const { return Icon; }
	C4Network2Status getGameStatus() const { return GameStatus; }
	const char *getComment() const { return Comment.getData(); }
	const C4GameVersion &getGameVersion() const { return Game; }
	bool isPasswordNeeded() const { return PasswordNeeded; }
	bool isJoinAllowed() const { return JoinAllowed; }
	bool isOfficialServer() const { return OfficialServer; }
	int32_t getSortOrder() const;
	int32_t getTime() const { return Time; }
	int32_t getStartTime() const { return StartTime; }
	StdStrBuf getGameGoalString() const;
	bool isEditor() const { return IsEditor; }
	C4NetpuncherID getNetpuncherGameID() const { return NetpuncherGameID; }
	StdStrBuf getNetpuncherAddr() const { return NetpuncherAddr; }

	void SetSourceAddress(const C4NetIO::EndpointAddress &ip);
	const C4NetIO::EndpointAddress &GetSourceAddress() const { return source; }

	void InitLocal();

	void SortNullIPsBack();

	void CompileFunc(StdCompiler *pComp);
};

// Serves references (mini-HTTP-server)
class C4Network2RefServer : public C4NetIOTCP
{
public:
	C4Network2RefServer();
	~C4Network2RefServer() override;

private:
	CStdCSec RefCSec;
	C4Network2Reference *pReference{nullptr};

public:
	void Clear();
	void SetReference(C4Network2Reference *pReference);

protected:
	// Overridden
	void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf) override;
	size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr) override;

private:
	// Responses
	void RespondNotImplemented(const C4NetIO::addr_t &addr, const char *szMessage);
	void RespondReference(const C4NetIO::addr_t &addr);

};

// mini HTTP client
class C4Network2HTTPClient : public C4NetIOTCP, private C4NetIO::CBClass
{
public:
	C4Network2HTTPClient();
	~C4Network2HTTPClient() override;

private:

	// Address information
	C4NetIO::addr_t ServerAddr, ServerAddrFallback, PeerAddr;
	StdCopyStrBuf Server, RequestPath;
	std::string headerAcceptedResponseType = "";

	bool fBinary{false};
	bool fBusy{false}, fSuccess{false}, fConnected{false};
	size_t iDataOffset{0};
	StdCopyBuf Request;
	time_t iRequestTimeout;
	C4TimeMilliseconds HappyEyeballsTimeout;

	// Response header data
	size_t iDownloadedSize{0}, iTotalSize{0};
	bool fCompressed;

	// Event queue to use for notify when something happens
	class C4InteractiveThread *pNotify{nullptr};

protected:
	StdCopyBuf ResultBin; // set if fBinary
	StdCopyStrBuf ResultString; // set if !fBinary

protected:

	// Overridden
	void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf) override;
	size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr) override;

	// Callbacks
	bool OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO) override;
	void OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) override;
	void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) override;

	void ResetRequestTimeout();
	virtual int32_t GetDefaultPort() { return 80; }

public:
	bool Query(const StdBuf &Data, bool fBinary);
	bool Query(const char *szData, bool fBinary) { return Query(StdBuf(szData, SLen(szData)), fBinary); }

	bool isBusy() const { return fBusy; }
	bool isSuccess() const { return fSuccess; }
	bool isConnected() const { return fConnected; }
	size_t getTotalSize() const { return iTotalSize; }
	size_t getDownloadedSize() const { return iDownloadedSize; }
	const StdBuf &getResultBin() const { assert(fBinary); return ResultBin; }
	const char *getResultString() const { assert(!fBinary); return ResultString.getData(); }
	const char *getServerName() const { return Server.getData(); }
	const char *getRequest() const { return RequestPath.getData(); }
	const C4NetIO::addr_t &getServerAddress() const { return ServerAddr; }

	void Cancel(const char *szReason);
	void Clear();

	bool SetServer(const char *szServerAddress);
	enum ResponseType { NoPreference, XML };
	void SetExpectedResponseType(ResponseType type);
	void SetNotify(class C4InteractiveThread *pnNotify) { pNotify = pnNotify; }

	// Overridden
	bool Execute(int iMaxTime, pollfd * readyfds) override { return Execute(iMaxTime); }
	virtual bool Execute(int iMaxTime = TO_INF);
	C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow) override;

private:
	bool ReadHeader(StdStrBuf Data);
	bool Decompress(StdBuf *pData);

};

// Loads current update url string (mini-HTTP-client)
class C4Network2UpdateClient : public C4Network2HTTPClient
{
protected:
	int32_t GetDefaultPort() override { return C4NetStdPortHTTP; }
public:
	C4Network2UpdateClient() : C4Network2HTTPClient() {}

	bool QueryUpdateURL();
	bool GetUpdateURL(StdStrBuf *pUpdateURL);
	bool GetVersion(StdStrBuf *pVersion);
};

// Loads references (mini-HTTP-client)
class C4Network2RefClient : public C4Network2HTTPClient
{
protected:
	int32_t GetDefaultPort() override { return C4NetStdPortRefServer; }
public:
	C4Network2RefClient() : C4Network2HTTPClient() {}

	bool QueryReferences();
	bool GetReferences(C4Network2Reference **&rpReferences, int32_t &rRefCount);
};

#endif // C4NETWORK2REFERENCE_H
