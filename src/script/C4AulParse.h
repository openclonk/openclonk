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

#ifndef INC_C4AulParse
#define INC_C4AulParse

#include "script/C4Aul.h"
#include "script/C4AulScriptFunc.h"

enum C4AulBCCType : int;
enum C4AulTokenType : int;

class C4CodeGen
{
public:
	C4AulScriptFunc *Fn;
	bool fJump = false;
	int iStack = 0;

	int GetStackValue(C4AulBCCType eType, intptr_t X = 0);
	int AddBCC(const char * SPos, C4AulBCCType eType, intptr_t X = 0);
	void ErrorOut(const char * SPos, C4AulError & e);
	void RemoveLastBCC();
	C4V_Type GetLastRetType(C4AulScriptEngine * Engine, C4V_Type to); // for warning purposes

	C4AulBCC MakeSetter(const char * TokenSPos, bool fLeaveValue = false); // Prepares to generate a setter for the last value that was generated

	int JumpHere(); // Get position for a later jump to next instruction added
	void SetJumpHere(int iJumpOp); // Use the next inserted instruction as jump target for the given jump operation
	void SetJump(int iJumpOp, int iWhere);
	void AddJump(const char * SPos, C4AulBCCType eType, int iWhere);

	// Keep track of loops and break/continue usages
	struct Loop
	{
		struct Control
		{
			bool Break;
			int Pos;
			Control *Next;
		};
		Control *Controls;
		int StackSize;
		Loop *Next;
	};
	Loop *pLoopStack = NULL;

	void PushLoop();
	void PopLoop(int ContinueJump);
	void AddLoopControl(const char * SPos, bool fBreak);
	~C4CodeGen()
	{ while (pLoopStack) PopLoop(0); }
};

struct C4ScriptOpDef
{
	unsigned short Priority;
	const char* Identifier;
	C4AulBCCType Code;
	bool Postfix;
	bool Changer; // changes first operand to result, rewrite to "a = a (op) b"
	bool NoSecondStatement; // no second statement expected (++/-- postfix)
	C4V_Type RetType; // type returned. ignored by C4V
	C4V_Type Type1;
	C4V_Type Type2;
};

extern const C4ScriptOpDef C4ScriptOpMap[];

class C4AulParse
{
public:
	enum Type { PARSER, PREPARSER };
	C4AulParse(C4ScriptHost * a, enum Type Type);
	C4AulParse(C4AulScriptFunc * Fn, C4AulScriptContext* context, C4AulScriptEngine *Engine);
	~C4AulParse();
	void Parse_DirectExec();
	void Parse_Script(C4ScriptHost *);

private:
	C4AulScriptFunc *Fn; C4ScriptHost * Host; C4ScriptHost * pOrgScript;
	C4AulScriptEngine *Engine;
	const char *SPos; // current position in the script
	const char *TokenSPos; // start of the current token in the script
	char Idtf[C4AUL_MAX_Identifier]; // current identifier
	C4AulTokenType TokenType; // current token type
	int32_t cInt; // current int constant
	C4String * cStr; // current string constant
	enum Type Type; // emitting bytecode?
	C4AulScriptContext* ContextToExecIn;
	void Parse_Function();
	void Parse_FuncBody();
	void Parse_Statement();
	void Parse_Block();
	int Parse_Params(int iMaxCnt, const char * sWarn, C4AulFunc * pFunc = 0);
	void Parse_Array();
	void Parse_PropList();
	void Parse_DoWhile();
	void Parse_While();
	void Parse_If();
	void Parse_For();
	void Parse_ForEach();
	void Parse_Expression(int iParentPrio = -1);
	void Parse_Var();
	void Parse_Local();
	void Parse_Static();
	void Parse_Const();
	C4Value Parse_ConstExpression(C4PropListStatic * parent, C4String * Name);
	C4Value Parse_ConstPropList(C4PropListStatic * parent, C4String * Name);
	void Store_Const(C4PropListStatic * parent, C4String * Name, const C4Value & v);

	bool AdvanceSpaces(); // skip whitespaces; return whether script ended
	int GetOperator(const char* pScript);
	void ClearToken(); // clear any data held with the current token
	C4AulTokenType GetNextToken(); // get next token of SPos

	void Shift();
	void Match(C4AulTokenType TokenType, const char * Expected = NULL);
	void Check(C4AulTokenType TokenType, const char * Expected = NULL);
	NORETURN void UnexpectedToken(const char * Expected);
	static const char * GetTokenName(C4AulTokenType TokenType);

	void Warn(const char *pMsg, ...) GNUC_FORMAT_ATTRIBUTE_O;
	void Error(const char *pMsg, ...) GNUC_FORMAT_ATTRIBUTE_O;
	void AppendPosition(StdStrBuf & Buf);

	int AddVarAccess(C4AulBCCType eType, intptr_t varnum);
	void DebugChunk();
	C4CodeGen codegen;
	int AddBCC(C4AulBCCType eType, intptr_t X = 0)
	{ if (Type == PARSER) return codegen.AddBCC(TokenSPos, eType, X); else return -1; }
	C4V_Type GetLastRetType(C4V_Type to)
	{ return codegen.GetLastRetType(Engine, to); }
	C4AulBCC MakeSetter(bool fLeaveValue = false)
	{ return Type == PARSER ? codegen.MakeSetter(TokenSPos, fLeaveValue) : C4AulBCC(AB_ERR, 0); }
	void SetJumpHere(int iJumpOp)
	{ if (Type == PARSER) codegen.SetJumpHere(iJumpOp); }
	void AddJump(C4AulBCCType eType, int iWhere)
	{ if (Type == PARSER) codegen.AddJump(TokenSPos, eType, iWhere); }
	void PushLoop()
	{ if (Type == PARSER) codegen.PushLoop(); }
	void PopLoop(int Jump)
	{ if (Type == PARSER) codegen.PopLoop(Jump); }

	friend class C4AulParseError;
};

#endif
