
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

	private:
		bool fInit, fConnected;
		class C4AulExec *pExec;
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
