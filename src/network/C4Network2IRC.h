/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
#ifndef C4NETWORK2IRC_H
#define C4NETWORK2IRC_H

#include "network/C4NetIO.h"
#include <time.h>

enum C4Network2IRCMessageType
{
	MSG_Server,
	MSG_Status,
	MSG_Message,
	MSG_Notice,
	MSG_Action
};

const int C4NetIRCMaxLogLength = 300000; // Maximum total length of log - if log gets longer, it will be cleared even if messages have never been read
const int C4NetIRCMaxReadLogLength = 1000; // Maximum log messages kept even after they've been read

class C4Network2IRCMessage
{
	friend class C4Network2IRCClient;
public:
	C4Network2IRCMessage(C4Network2IRCMessageType enType, const char *szSource, const char *szTarget, const char *szData)
			: iTimestamp(time(nullptr)), eType(enType), Source(szSource), Target(szTarget), Data(szData), Next(0)
	{ }

private:
	time_t iTimestamp;
	C4Network2IRCMessageType eType;
	StdCopyStrBuf Source, Target, Data;

	// used by C4Network2IRCClient
	C4Network2IRCMessage *Next;

public:
	time_t getTimestamp() const { return iTimestamp; }
	C4Network2IRCMessageType getType() const { return eType; }
	const char *getSource() const { return Source.getData(); }
	const char *getTarget() const { return Target.getData(); }
	const char *getData() const { return Data.getData(); }
	bool isChannel() const { return Target.getLength() && (*Target.getData() == '#' || *Target.getData() == '+'); }

	C4Network2IRCMessage *getNext() const { return Next; }
};

class C4Network2IRCUser
{
	friend class C4Network2IRCChannel;
public:
	C4Network2IRCUser(const char *szName);

private:
	StdCopyStrBuf Prefix;
	StdCopyStrBuf Name;

	// used by C4Network2IRCChannel
	C4Network2IRCUser *Next;

public:
	const char *getPrefix() const { return Prefix.getData(); }
	const char *getName() const { return Name.getData(); }
	C4Network2IRCUser *getNext() const { return Next; }

private:
	// called by C4Network2IRCChannel
	void SetPrefix(const char *szPrefix) { Prefix = szPrefix; }

};

class C4Network2IRCChannel
{
	friend class C4Network2IRCClient;
public:
	C4Network2IRCChannel(const char *szName);
	~C4Network2IRCChannel();

private:
	StdCopyStrBuf Name;
	StdCopyStrBuf Topic;
	C4Network2IRCUser *pUsers;

	bool fReceivingUsers;

	// used by C4Network2IRCClient
	C4Network2IRCChannel *Next;

public:
	const char *getName() const { return Name.getData(); }
	const char *getTopic() const { return Topic.getData(); }
	C4Network2IRCUser *getUsers() const { return pUsers; }
	C4Network2IRCUser *getUser(const char *szName) const;
	bool isUsersLocked() const { return fReceivingUsers; }

private:
	// called by C4Network2IRCClient
	void OnUsers(const char *szUsers, const char *szPrefixes);
	void OnUsersEnd();
	void OnJoin(const char *szUser);
	void OnPart(const char *szUser, const char *szComment);
	void OnKick(const char *szUser, const char *szComment);
	void OnTopic(const char *szTopic);

	C4Network2IRCUser *AddUser(const char *szName);
	void DeleteUser(C4Network2IRCUser *pUser);
	void ClearUsers();
};

class C4Network2IRCClient : public C4NetIOTCP, private C4NetIO::CBClass
{
public:
	C4Network2IRCClient();
	~C4Network2IRCClient();

private:

	// Connection information
	C4NetIO::addr_t ServerAddr, PeerAddr;
	bool fConnecting, fConnected;

	// Status information
	StdCopyStrBuf Nick, RealName, Password;
	StdCopyStrBuf AutoJoin;
	C4Network2IRCChannel *pChannels;

	// User mode/prefix map
	StdCopyStrBuf Prefixes;

	// Message log
	C4Network2IRCMessage *pLog, *pLogLastRead, *pLogEnd;
	int32_t iLogLength, iUnreadLogLength;

	// Event queue for notify
	class C4InteractiveThread *pNotify;

	// Critical section for data
	CStdCSec CSec;

private:

	// Overridden
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr);

	// Callbacks
	bool OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO);
	void OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason);
	void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

public:

	CStdCSec *getCSec() { return &CSec; }

	// only call these while holding a lock on the above critical section!
	C4Network2IRCChannel *getFirstChannel() const;
	C4Network2IRCChannel *getNextChannel(C4Network2IRCChannel *pPrevChan) const;
	C4Network2IRCChannel *getChannel(const char *szName) const;
	C4Network2IRCMessage *getMessageLog() const { return pLog; }
	C4Network2IRCMessage *getUnreadMessageLog() const { return pLogLastRead ? pLogLastRead->getNext() : pLog; }
	void ClearMessageLog();
	void MarkMessageLogRead();
	const char *getUserName() const { return Nick.getData(); }

	using C4NetIOTCP::Connect;
	// Simple network communication
	bool Connect(const char *szServer, const char *szNick, const char *szRealName, const char *szPassword = nullptr, const char *szChannel = nullptr);
	using C4NetIOTCP::Close;
	bool Close();
	using C4NetIOTCP::Send;
	bool Send(const char *szCommand, const char *szParameters = nullptr);

	// Notfiy interface
	void SetNotify(class C4InteractiveThread *pnNotify) { pNotify = pnNotify; }

	// Special IRC commands
	bool Quit(const char *szReason);
	bool Join(const char *szChannel);
	bool Part(const char *szChannel);
	bool Message(const char *szTarget, const char *szText);
	bool Notice(const char *szTarget, const char *szText);
	bool Action(const char *szTarget, const char *szText);
	bool ChangeNick(const char *szNewNick);
	bool RegisterNick(const char *szPassword, const char *szMail);

	// Status
	bool IsActive() const { return fConnecting || fConnected; }
	bool IsConnected() const { return fConnected; }

private:
	void OnCommand(const char *szSender, const char *szCommand, const char *szParameters);
	void OnNumericCommand(const char *szSender, int iCommand, const char *szParameters);
	void OnConnected();
	void OnMessage(bool fNotice, const char *szSource, const char *szTarget, const char *szText);

	void PopMessage();
	void PushMessage(C4Network2IRCMessageType eType, const char *szSource, const char *szTarget, const char *szText);

	C4Network2IRCChannel *AddChannel(const char *szName);
	void DeleteChannel(C4Network2IRCChannel *pChannel);
};

#endif // C4NETWORK2IRC_H
