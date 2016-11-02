/*
 * OpenClonk, http://www.openclonk.org
 *
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

#ifndef C4AULDEBUG_H
#define C4AULDEBUG_H

#ifndef NOAULDEBUG

#include "network/C4NetIO.h"

// manages a debugging interface
class C4AulDebug : public C4NetIOTCP, private C4NetIO::CBClass
{
public:
	C4AulDebug();
	~C4AulDebug();
	static bool InitDebug(const char *szPassword, const char *szHost);
	bool Listen(uint16_t iPort, bool fWait);
	static inline C4AulDebug *GetDebugger() { return pDebug; }

private:
	bool fInit, fConnected;
	C4AulExec *pExec;
	static C4AulDebug *pDebug;
	C4NetIO::addr_t PeerAddr, AllowedAddr;
	StdCopyStrBuf Password;

	enum DebugState
	{
		DS_Go, // execute until next break point
		DS_Stop, // execution stopped
		DS_Step, // executing until next step point
		DS_StepOver, // executung until next step point on same level or above (or break point)
		DS_StepOut // executing until next step point above (or break point)
	};
	DebugState eState;
	int iStepCallDepth;

	// temporary stuff
	std::list<StdStrBuf*> StackTrace;

	void ObtainStackTrace(C4AulScriptContext* pCtx, C4AulBCC* pCPos);
	const char* RelativePath(StdStrBuf &path);

private:

	// Overridden
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf);
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr);

	// Callbacks
	bool OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO);
	void OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason);
	void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

public:
	bool isConnected() const { return fConnected; }

	void SetPassword(const char *szPassword) { Password = szPassword; }
	bool SetAllowed(const char *szHost);
	void SetEngine(class C4AulExec *pnExec) { pExec = pnExec; }

	bool Init(uint16_t iPort);
	virtual bool Close();
	virtual bool Close(const addr_t &addr);
	
	void ControlScriptEvaluated(const char* script, const char* result);

	void OnLog(const char *szLine);
	void DebugStep(C4AulBCC *pCPos, C4Value* stackTop);

private:
	struct ProcessLineResult
	{
		bool okay;
		std::string answer;
		ProcessLineResult(bool okay, const std::string answer)
			: okay(okay), answer(answer) {}
	};

	StdStrBuf FormatCodePos(C4AulScriptContext *pCtx, C4AulBCC *pCPos);

	ProcessLineResult ProcessLine(const StdStrBuf &Line);

	bool SendLine(const char *szType, const char *szData = nullptr);
};

#endif
#endif
