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
#include "network/C4HTTP.h"

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


// Loads current update url string (mini-HTTP-client)
class C4Network2UpdateClient : public C4HTTPClient
{
public:
	C4Network2UpdateClient() : C4HTTPClient() {}

	bool QueryUpdateURL();
	bool GetUpdateURL(StdStrBuf *pUpdateURL);
	bool GetVersion(StdStrBuf *pVersion);
};

// Loads references (mini-HTTP-client)
class C4Network2RefClient : public C4HTTPClient
{
public:
	C4Network2RefClient() : C4HTTPClient() {}

	bool QueryReferences();
	bool GetReferences(C4Network2Reference **&rpReferences, int32_t &rRefCount);
};

#endif // C4NETWORK2REFERENCE_H
