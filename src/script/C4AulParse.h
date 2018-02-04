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
#include "script/C4AulAST.h"
#include "script/C4AulCompiler.h"
#include "script/C4AulScriptFunc.h"

#include <stack>

enum C4AulBCCType : int;
enum C4AulTokenType : int;

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
	C4AulParse(class C4ScriptHost *host);
	C4AulParse(C4AulScriptFunc * Fn, C4AulScriptContext* context, C4AulScriptEngine *Engine);
	~C4AulParse();
	std::unique_ptr<::aul::ast::FunctionDecl> Parse_DirectExec(const char *code, bool whole_function);
	std::unique_ptr<::aul::ast::Script> Parse_Script(C4ScriptHost *);

private:
	C4AulScriptFunc *Fn; C4ScriptHost * Host; C4ScriptHost * pOrgScript;
	C4AulScriptEngine *Engine;
	const char *SPos; // current position in the script
	const char *TokenSPos; // start of the current token in the script
	char Idtf[C4AUL_MAX_Identifier]; // current identifier
	C4AulTokenType TokenType; // current token type
	int32_t cInt; // current int constant
	C4String * cStr; // current string constant
	C4AulScriptContext* ContextToExecIn;
	void Parse_Function(bool parse_for_direct_exec);
	void Parse_WarningPragma();

protected:
	// All of the Parse_* functions need to be protected (not private!) so
	// we can make them public in a derived class for unit testing purposes
	std::unique_ptr<::aul::ast::FunctionDecl> Parse_ToplevelFunctionDecl();
	std::unique_ptr<::aul::ast::Stmt> Parse_Statement();
	std::unique_ptr<::aul::ast::Block> Parse_Block();
	std::unique_ptr<::aul::ast::ArrayLit> Parse_Array();
	std::unique_ptr<::aul::ast::ProplistLit> Parse_PropList();
	std::unique_ptr<::aul::ast::DoLoop> Parse_DoWhile();
	std::unique_ptr<::aul::ast::WhileLoop> Parse_While();
	std::unique_ptr<::aul::ast::If> Parse_If();
	std::unique_ptr<::aul::ast::ForLoop> Parse_For();
	std::unique_ptr<::aul::ast::RangeLoop> Parse_ForEach();
	std::unique_ptr<::aul::ast::Expr> Parse_Expression(int iParentPrio = -1);
	std::unique_ptr<::aul::ast::VarDecl> Parse_Var();
	void Shift();

private:
	void Parse_Function(::aul::ast::Function *func);
	void Parse_CallParams(::aul::ast::CallExpr *call);

	bool AdvanceSpaces(); // skip whitespaces; return whether script ended
	int GetOperator(const char* pScript);
	void ClearToken(); // clear any data held with the current token
	C4AulTokenType GetNextToken(); // get next token of SPos

	void Match(C4AulTokenType TokenType, const char * Expected = nullptr);
	void Check(C4AulTokenType TokenType, const char * Expected = nullptr);
	NORETURN void UnexpectedToken(const char * Expected);

	void Warn(C4AulWarningId warning, ...);
	bool IsWarningEnabled(const char *pos, C4AulWarningId warning) const;
	void Error(const char *pMsg, ...) GNUC_FORMAT_ATTRIBUTE_O;
	void AppendPosition(StdStrBuf & Buf);

	friend class C4AulParseError;
	std::stack<const char *> parse_pos_stack;
	void PushParsePos();
	void PopParsePos();
	void DiscardParsePos();
};

#endif
