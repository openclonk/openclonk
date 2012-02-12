/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Peter Wortmann
 * Copyright (c) 2010  Martin Plicht
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef C4AULDEBUG_H
#define C4AULDEBUG_H

#ifndef NOAULDEBUG

#include "C4Aul.h"
#include "C4NetIO.h"

// manages a debugging interface
class C4AulDebug : public C4NetIOTCP, private C4NetIO::CBClass
{
public:
	C4AulDebug();
	~C4AulDebug();
	static bool InitDebug(uint16_t iPort, const char *szPassword, const char *szHost, bool fWait);
	static inline C4AulDebug *GetDebugger() { return pDebug; }

private:
	bool fInit, fConnected;
	class C4AulExec *pExec;
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
	void DebugStep(C4AulBCC *pCPos);
	void DebugStepIn(C4AulBCC *pCPos);
	void DebugStepOut(C4AulBCC *pCPos, C4AulScriptContext *pRetCtx, C4Value *pRVal);

private:
	void StepPoint(C4AulBCC *pCPos, C4AulScriptContext *pRetCtx = NULL, C4Value *pRVal = NULL);

	StdStrBuf FormatCodePos(C4AulScriptContext *pCtx, C4AulBCC *pCPos);

	void ProcessLine(const StdStrBuf &Line);

	bool SendLine(const char *szType, const char *szData = NULL);
};

#endif
#endif
