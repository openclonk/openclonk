/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Peter Wortmann
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Mortimer
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

#include <C4Include.h>
#include <C4Version.h>
#include <C4GameControl.h>
#include <C4Game.h>
#include <C4MessageInput.h>
#include <C4Log.h>
#include <C4Object.h>

#include "C4AulDebug.h"
#include "C4AulExec.h"

#ifndef NOAULDEBUG

// *** C4AulDebug

C4AulDebug::C4AulDebug()
		: fInit(false), fConnected(false)
{
	ZeroMem(&PeerAddr, sizeof PeerAddr);
}

C4AulDebug::~C4AulDebug()
{
	for (std::list<StdStrBuf*>::iterator it = StackTrace.begin(); it != StackTrace.end(); it++)
		{delete *it;}
}

void C4AulDebug::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Enlarge buffer
	int iSize = rPacket.getSize(),
	            iPos = rOutBuf.getSize();
	rOutBuf.Grow(iSize + 2);
	// Write packet
	rOutBuf.Write(rPacket, iPos);
	// Terminate
	uint8_t *pPos = getMBufPtr<uint8_t>(rOutBuf, iPos + iSize);
	*pPos = '\r'; *(pPos + 1) = '\n';
}

size_t C4AulDebug::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	// Find line separation
	const char *pSep = reinterpret_cast<const char *>(memchr(rInBuf.getData(), '\n', rInBuf.getSize()));
	if (!pSep)
		return 0;
	// Check if it's windows-style separation
	int iSize = pSep - getBufPtr<char>(rInBuf) + 1,
	            iLength = iSize - 1;
	if (iLength && *(pSep - 1) == '\r')
		iLength--;
	// Copy the line
	StdStrBuf Buf; Buf.Copy(getBufPtr<char>(rInBuf), iLength);
	// Password line?
	if (fConnected)
		ProcessLine(Buf);
	else if (!Password.getSize() || Password == Buf)
	{
		fConnected = true;
		SendLine("HLO", "This is " C4ENGINEINFOLONG ", " C4VERSION);
		Log("C4Aul debugger connected successfully!");
	}
	else
		C4NetIOTCP::Close(PeerAddr);
	// Consume line
	return iSize;
}

bool C4AulDebug::OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO)
{
	assert(pNetIO == this);
	// Already have a connection?
	if (fConnected) return false;
	// Check address
	if (AllowedAddr.sin_addr.s_addr)
		if (AllowedAddr.sin_addr.s_addr != AddrPeer.sin_addr.s_addr ||
		    (AllowedAddr.sin_port && AllowedAddr.sin_port != AddrPeer.sin_port))
		{
			LogF("C4AulDebug blocked connection from %s:%d", inet_ntoa(AddrPeer.sin_addr), htons(AddrPeer.sin_port));
			return false;
		}
	// Log
	LogF("C4AulDebug got connection from %s:%d", inet_ntoa(AddrPeer.sin_addr), htons(AddrPeer.sin_port));
	// Accept connection
	PeerAddr = AddrPeer;
	return true;
}

void C4AulDebug::OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason)
{
	LogF("C4AulDebug lost connection (%s)", szReason);
	fConnected = false;
	eState = DS_Go;
	ZeroMem(&PeerAddr, sizeof PeerAddr);
}

void C4AulDebug::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
	// Won't get called
}

bool C4AulDebug::SetAllowed(const char *szHost)
{
	// Clear
	ZeroMem(&AllowedAddr, sizeof(AllowedAddr));
	// No host?
	if (!szHost || !*szHost) return true;
	// Resolve the address
	return ResolveAddress(szHost, &AllowedAddr, 0);
}

bool C4AulDebug::Init(uint16_t iPort)
{
	if (fInit) Close();
	if (iPort == P_NONE) return false;

	// Register self as callback for network events
	C4NetIOTCP::SetCallback(this);

	// Start listening
	if (!C4NetIOTCP::Init(iPort))
		return false;

	// Okay
	fInit = true;
	eState = DS_Go;
	return true;
}

bool C4AulDebug::Close()
{
	if (!fInit) return true;
	fInit = fConnected = false;
	return C4NetIOTCP::Close();
}

bool C4AulDebug::Close(const addr_t &addr)
{
	if (!fInit) return true;
	bool success = C4NetIOTCP::Close(addr);
	if (success)
		fInit = fConnected = false;
	return success;
}

void C4AulDebug::OnLog(const char *szLine)
{
	if (!fConnected) return;
	SendLine("LOG", szLine);
}

void C4AulDebug::ProcessLine(const StdStrBuf &Line)
{
	// Get command
	StdStrBuf Cmd;
	Cmd.CopyUntil(Line.getData(), ' ');
	// Get data
	const char *szData = Line.getPtr(Cmd.getLength());
	if (*szData) szData++;
	// Identify command
	const char *szCmd = Cmd.getData();
	bool fOkay = true;
	const char *szAnswer = NULL;
	if (SEqualNoCase(szCmd, "HELP"))
	{
		fOkay = false; szAnswer = "Yeah, like I'm going to explain that /here/";
	}
	else if (SEqualNoCase(szCmd, "BYE") || SEqualNoCase(szCmd, "QUIT"))
		C4NetIOTCP::Close(PeerAddr);
	else if (SEqualNoCase(szCmd, "SAY"))
		::Control.DoInput(CID_Message, new C4ControlMessage(C4CMT_Normal, szData), CDT_Direct);
	else if (SEqualNoCase(szCmd, "CMD"))
		::MessageInput.ProcessCommand(szData);
	else if (SEqualNoCase(szCmd, "STP") || SEqualNoCase(szCmd, "S"))
		eState = DS_Step;
	else if (SEqualNoCase(szCmd, "GO") || SEqualNoCase(szCmd, "G"))
		eState = DS_Go;
	else if (SEqualNoCase(szCmd, "STO") || SEqualNoCase(szCmd, "O"))
		eState = DS_StepOver;
	else if (SEqualNoCase(szCmd, "STR") || SEqualNoCase(szCmd, "R"))
		eState = DS_StepOut;
	else if (SEqualNoCase(szCmd, "EXC") || SEqualNoCase(szCmd, "E"))
	{
		C4AulScriptContext* context = pExec->GetContext(pExec->GetContextDepth()-1);
		int32_t objectNum = context && context->Obj ? context->Obj->Number : C4ControlScript::SCOPE_Global;
		::Control.DoInput(CID_Script, new C4ControlScript(szData, objectNum, true, true), CDT_Decide);
	}
	else if (SEqualNoCase(szCmd, "PSE"))
		if (Game.IsPaused())
		{
			Game.Unpause();
			szAnswer = "Game unpaused.";
		}
		else
		{
			Game.Pause();
			szAnswer = "Game paused.";
		}
	else if (SEqualNoCase(szCmd, "LST"))
	{
		for (C4AulScript* script = ScriptEngine.Child0; script; script = script->Next)
		{
			SendLine(RelativePath(script->ScriptName));
		}
	}

	// toggle breakpoint
	else if (SEqualNoCase(szCmd, "TBR"))
	{
		// FIXME: this doesn't find functions which were included/appended
		StdStrBuf scriptPath;
		scriptPath.CopyUntil(szData, ':');
		const char* lineStart = szData+1+scriptPath.getLength();
		int line = strtol(szData+1+scriptPath.getLength(), const_cast<char**>(&lineStart), 10);

		C4AulScript* script;
		for (script = ScriptEngine.Child0; script; script = script->Next)
		{
			if (SEqualNoCase(RelativePath(script->ScriptName), scriptPath.getData()))
				break;
		}

		if (script)
		{
			C4AulBCC* foundDebugChunk = NULL;
			const char* scriptText = script->GetScript();
			for (C4AulBCC* chunk = &script->Code[0]; chunk; chunk++)
			{
				switch (chunk->bccType)
				{
				case AB_DEBUG:
					{
					int lineOfThisOne = SGetLine(scriptText, script->PosForCode[chunk - &script->Code[0]]);
					if (lineOfThisOne == line)
					{
						foundDebugChunk = chunk;
						goto Done;
					}
					/*else {
					  DebugLogF("Debug chunk at %d", lineOfThisOne);
					}*/
					}
					break;
				case AB_EOF:
					goto Done;
				default:
					break;
				}
			}
Done:
			if (foundDebugChunk)
			{
				foundDebugChunk->Par.i = !foundDebugChunk->Par.i; // activate breakpoint
			}
			else
			{
				szAnswer = "Can't set breakpoint (wrong line?)";
				fOkay = false;
			}
		}
		else
		{
			fOkay = false;
			szAnswer = "Can't find script";
		}


	}
	else if (SEqualNoCase(szCmd, "SST"))
	{
		std::list<StdStrBuf*>::iterator it = StackTrace.begin();
		for (it++; it != StackTrace.end(); it++)
		{
			SendLine("AT", (*it)->getData());
		}
		SendLine("EST");
	}
	else if (SEqualNoCase(szCmd, "VAR"))
	{
		
		C4Value *val = NULL;
		int varIndex;
		C4AulScriptContext* pCtx = pExec->GetContext(pExec->GetContextDepth() - 1);
		if (pCtx)
		{
			if ((varIndex = pCtx->Func->ParNamed.GetItemNr(szData)) != -1)
			{
				val = &pCtx->Pars[varIndex];
			}
			else if ((varIndex = pCtx->Func->VarNamed.GetItemNr(szData)) != -1)
			{
				val = &pCtx->Vars[varIndex];
			}
		}
		const char* typeName = val ? GetC4VName(val->GetType()) : "any";
		StdStrBuf output = FormatString("%s %s %s", szData, typeName, val ? val->GetDataString().getData() : "Unknown");
		SendLine("VAR", output.getData());
	}
	else
	{
		fOkay = false;
		szAnswer = "Can't do that";
	}
	// Send answer
	SendLine(fOkay ? "OK" : "ERR", szAnswer);
}

bool C4AulDebug::SendLine(const char *szType, const char *szData)
{
	StdStrBuf Line = szData ? FormatString("%s %s", szType, szData) : StdStrBuf(szType);
	return Send(C4NetIOPacket(Line.getData(), Line.getSize(), false, PeerAddr));
}

void C4AulDebug::DebugStep(C4AulBCC *pCPos)
{
	// Get top context
	//C4AulScriptContext *pCtx = pExec->GetContext(pExec->GetContextDepth() - 1);

	// Already stopped? Ignore.
	// This means we are doing some calculation with suspended script engine.
	// We do /not/ want to have recursive suspensions...
	if (eState == DS_Stop)
		return;

	// Have break point?
	if (pCPos->Par.i)
		eState = DS_Step;

	StepPoint(pCPos);
}

void C4AulDebug::DebugStepIn(C4AulBCC *pCPos)
{

}

void C4AulDebug::DebugStepOut(C4AulBCC *pCPos, C4AulScriptContext *pRetCtx, C4Value *pRVal)
{

	// Ignore if already suspended, see above.
	if (eState == DS_Stop)
		return;

	// This counts as a regular step point
	StepPoint(pCPos, pRetCtx, pRVal);

}

void C4AulDebug::StepPoint(C4AulBCC *pCPos, C4AulScriptContext *pRetCtx, C4Value *pRVal)
{

	// Maybe got a command in the meantime?
	Execute(0);

	// Get current script context
	int iCallDepth = pExec->GetContextDepth();
	C4AulScriptContext *pCtx = pExec->GetContext(iCallDepth-1);

	// When we're stepping out of a script function, the context of the returning
	// function hasn't been removed yet.
	if (pCtx == pRetCtx)
	{
		iCallDepth--;
		// Check if we are returning to a script function
		if (pCtx->Return)
			pCtx--;
		else
			pCtx = NULL;
	}

	// Stepped out?
	if (pRVal && eState != DS_Go && iCallDepth <= iStepCallDepth)
	{
		StdStrBuf FuncDump = pRetCtx->ReturnDump();
		StdStrBuf ReturnDump = pRVal->GetDataString();
		SendLine("RET", FormatString("%s = %s", FuncDump.getData(), ReturnDump.getData()).getData());

		// Ignore as step point if we didn't "really" step out
		if (iCallDepth >= iStepCallDepth) return;
	}

	// Stop?
	switch (eState)
	{
		// Continue normally
	case DS_Go: return;

		// Always stop
	case DS_Stop: break;
	case DS_Step: break;

		// Only stop for same level or above
	case DS_StepOver:
		if (iCallDepth > iStepCallDepth)
			return;
		break;

		// Only stop above
	case DS_StepOut:
		if (iCallDepth >= iStepCallDepth)
			return;
		break;
	}

	// Let's stop here
	eState = DS_Stop;
	iStepCallDepth = iCallDepth;
	Game.HaltCount++;

	// No valid stop position? Just continue
	if (!pCtx)
	{
		Game.HaltCount--;
		eState = DS_Step;
		return;
	}

	// Signal
	if (pCPos && pCPos->bccType == AB_DEBUG && pCPos->Par.i)
		SendLine("STP", FormatString("Stopped on breakpoint %d", pCPos->Par.i).getData());
	else
		SendLine("STP", "Stepped");

	// Position
	ObtainStackTrace(pCtx, pCPos);
	SendLine("POS", StackTrace.front()->getData());

	// Suspend until we get some command
	while (eState == DS_Stop)
		if (!Application.ScheduleProcs())
		{
			Close();
			return;
		}

	// Do whatever we've been told.
	Game.HaltCount--;
}

void C4AulDebug::ControlScriptEvaluated(const char* script, const char* result)
{
	SendLine("EVR", FormatString("%s=%s", script, result).getData());
}

const char* C4AulDebug::RelativePath(StdStrBuf &path)
{
	const char* p = path.getData();
	const char* result = Config.AtRelativePath(p);
	if (p != result)
		return result;
	// try path relative to scenario container
	StdStrBuf scenarioContainerPath;
	GetParentPath(::Game.ScenarioFile.GetName(), &scenarioContainerPath);
	return GetRelativePathS(p, scenarioContainerPath.getData());
}

void C4AulDebug::ObtainStackTrace(C4AulScriptContext* pCtx, C4AulBCC* pCPos)
{
	for (std::list<StdStrBuf*>::iterator it = StackTrace.begin(); it != StackTrace.end(); it++)
		{delete *it;}
	StackTrace.clear();
	for (int ctxNum = pExec->GetContextDepth()-1; ctxNum >= 0; ctxNum--)
	{
		C4AulScriptContext* c = pExec->GetContext(ctxNum);
		C4AulBCC* _cpos = c == pCtx ? pCPos : c->CPos;
		if (_cpos)
		{
			StdStrBuf* format = new StdStrBuf(FormatCodePos(c, _cpos));
			StackTrace.push_back(format);
		}
	}
}

StdStrBuf C4AulDebug::FormatCodePos(C4AulScriptContext *pCtx, C4AulBCC *pCPos)
{
	// Get position in script
	int iLine = pCtx->Func->GetLineOfCode(pCPos);
	// Format
	return FormatString("%s:%d", RelativePath(pCtx->Func->pOrgScript->ScriptName), iLine);
}

#endif
