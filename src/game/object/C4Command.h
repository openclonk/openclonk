/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2008-2011  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* The command stack controls an object's complex and independent behavior */

#ifndef INC_C4Command
#define INC_C4Command

#include "C4ObjectPtr.h"
#include "C4Value.h"

enum C4CMD
{
	C4CMD_None,
	C4CMD_Follow,
	C4CMD_MoveTo,
	C4CMD_Enter,
	C4CMD_Exit,
	C4CMD_Grab,
	C4CMD_Throw,
	C4CMD_UnGrab,
	C4CMD_Jump,
	C4CMD_Wait,
	C4CMD_Get,
	C4CMD_Put,
	C4CMD_Drop,
	C4CMD_Dig,
	C4CMD_Activate,
	C4CMD_PushTo,
	C4CMD_Transfer,
	C4CMD_Attack,
	C4CMD_Buy,
	C4CMD_Sell,
	C4CMD_Acquire,
	C4CMD_Retry,
	C4CMD_Home,
	C4CMD_Call,
	C4CMD_Take,
	C4CMD_Take2,
};

const int32_t C4CMD_First     = C4CMD_Follow,
              C4CMD_Last      = C4CMD_Take2; // carlo

const int32_t C4CMD_Mode_SilentSub  = 0, // subcommand; failure will cause base to fail (no message in case of failure)
              C4CMD_Mode_Base       = 1, // regular base command
              C4CMD_Mode_SilentBase = 2, // silent base command (no message in case of failure)
              C4CMD_Mode_Sub    = 3; // subcommand; failure will cause base to fail

// MoveTo and Enter command options: Include push target
const int32_t C4CMD_MoveTo_NoPosAdjust = 1,
    C4CMD_MoveTo_PushTarget  = 2;

const int32_t C4CMD_Enter_PushTarget   = 2;

const char *CommandName(int32_t iCommand);
const char* CommandNameID(int32_t iCommand);
int32_t CommandByName(const char *szCommand);

class C4Command
{
public:
	C4Command();
	~C4Command();
public:
	C4Object *cObj;
	int32_t Command;
	C4Value Tx;
	int32_t Ty;
	C4ObjectPtr Target,Target2;
	C4Value Data;
	int32_t UpdateInterval;
	int32_t Evaluated,PathChecked,Finished;
	int32_t Failures,Retries,Permit;
	C4String *Text;
	C4Command *Next;
	int32_t iExec; // 0 = not executing, 1 = executing, 2 = executing, command should delete himself on finish
	int32_t BaseMode; // 0: subcommand/unmarked base (if failing, base will fail, too); 1: base command; 2: silent base command
public:
	void Set(int32_t iCommand, C4Object *pObj, C4Object *pTarget, C4Value iTx, int32_t iTy, C4Object *pTarget2, C4Value iData, int32_t iUpdateInterval, bool fEvaluated, int32_t iRetries, C4String *szText, int32_t iBaseMode);
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void Default();
	void Denumerate(C4ValueNumbers *);
	void CompileFunc(StdCompiler *pComp, C4ValueNumbers *);
protected:
	void Call();
	void Home();
	void Retry();
	void Fail(const char *szFailMessage=0);
	void Acquire();
	void Sell();
	void Buy();
	void Attack();
	void Transfer();
	void Finish(bool fSuccess=false, const char *szFailMessage=0);
	void Follow();
	void MoveTo();
	void Enter();
	void Exit();
	void Grab();
	void UnGrab();
	void Throw();
	void Jump();
	void Wait();
	void Take();
	void Take2();
	bool GetTryEnter(); // at object pos during get-command: Try entering it
	void Get();
	void Put();
	void Drop();
	void Dig();
	void Activate();
	void PushTo();
	int32_t CallFailed();
	bool JumpControl();
	bool FlightControl();
	bool InitEvaluation();
	int32_t GetExpGain(); // get control counts gained by this command; 1EXP=5 ControlCounts
};

#endif
