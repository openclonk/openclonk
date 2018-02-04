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
// parses scripts

#include "C4Include.h"
#include "script/C4AulParse.h"

#include "object/C4Def.h"
#include "script/C4AulDebug.h"
#include "script/C4AulExec.h"

#ifndef DEBUG_BYTECODE_DUMP
#define DEBUG_BYTECODE_DUMP 0
#endif
#include <iomanip>

#define C4AUL_Include       "#include"
#define C4AUL_Append        "#appendto"
#define C4AUL_Warning       "#warning"

#define C4Aul_Warning_enable "enable"
#define C4Aul_Warning_disable "disable"

#define C4AUL_Func          "func"

#define C4AUL_Private       "private"
#define C4AUL_Protected     "protected"
#define C4AUL_Public        "public"
#define C4AUL_Global        "global"
#define C4AUL_Const         "const"

#define C4AUL_If            "if"
#define C4AUL_Else          "else"
#define C4AUL_Do            "do"
#define C4AUL_While         "while"
#define C4AUL_For           "for"
#define C4AUL_In            "in"
#define C4AUL_Return        "return"
#define C4AUL_Var           "Var"
#define C4AUL_Par           "Par"
#define C4AUL_Break         "break"
#define C4AUL_Continue      "continue"
#define C4AUL_this          "this"

#define C4AUL_GlobalNamed   "static"
#define C4AUL_LocalNamed    "local"
#define C4AUL_VarNamed      "var"

#define C4AUL_TypeInt       "int"
#define C4AUL_TypeBool      "bool"
#define C4AUL_TypeC4ID      "id"
#define C4AUL_TypeDef       "def"
#define C4AUL_TypeEffect    "effect"
#define C4AUL_TypeC4Object  "object"
#define C4AUL_TypePropList  "proplist"
#define C4AUL_TypeString    "string"
#define C4AUL_TypeArray     "array"
#define C4AUL_TypeFunction  "func"

#define C4AUL_True          "true"
#define C4AUL_False         "false"
#define C4AUL_Nil           "nil"
#define C4AUL_New           "new"

// script token type
enum C4AulTokenType : int
{
	ATT_INVALID,// invalid token
	ATT_DIR,    // directive
	ATT_IDTF,   // identifier
	ATT_INT,    // integer constant
	ATT_STRING, // string constant
	ATT_DOT,    // "."
	ATT_COMMA,  // ","
	ATT_COLON,  // ":"
	ATT_SCOLON, // ";"
	ATT_BOPEN,  // "("
	ATT_BCLOSE, // ")"
	ATT_BOPEN2, // "["
	ATT_BCLOSE2,// "]"
	ATT_BLOPEN, // "{"
	ATT_BLCLOSE,// "}"
	ATT_CALL,   // "->"
	ATT_CALLFS, // "->~"
	ATT_LDOTS,  // '...'
	ATT_SET,    // '='
	ATT_OPERATOR,// operator
	ATT_EOF     // end of file
};

C4AulParse::C4AulParse(C4ScriptHost *a) :
	Fn(nullptr), Host(a), pOrgScript(a), Engine(a->Engine),
	SPos(a->Script.getData()), TokenSPos(SPos),
	TokenType(ATT_INVALID),
	ContextToExecIn(nullptr)
{ }

C4AulParse::C4AulParse(C4AulScriptFunc * Fn, C4AulScriptContext* context, C4AulScriptEngine *Engine) :
	Fn(Fn), Host(nullptr), pOrgScript(nullptr), Engine(Engine),
	SPos(Fn->Script), TokenSPos(SPos),
	TokenType(ATT_INVALID),
	ContextToExecIn(context)
{ }

C4AulParse::~C4AulParse()
{
	ClearToken();
}

void C4ScriptHost::Warn(const char *pMsg, ...)
{
	va_list args; va_start(args, pMsg);
	StdStrBuf Buf = FormatStringV(pMsg, args);
	Buf.AppendFormat(" (%s)", ScriptName.getData());
	Engine->GetErrorHandler()->OnWarning(Buf.getData());
	va_end(args);
}

void C4AulParse::Warn(C4AulWarningId warning, ...)
{
	if (!IsWarningEnabled(TokenSPos, warning))
		return;
	va_list args; va_start(args, warning);
	StdStrBuf Buf = FormatStringV(C4AulWarningMessages[static_cast<size_t>(warning)], args);
	AppendPosition(Buf);
	Buf.AppendFormat(" [%s]", C4AulWarningIDs[static_cast<size_t>(warning)]);
	Engine->GetErrorHandler()->OnWarning(Buf.getData());
	va_end(args);
}

bool C4AulParse::IsWarningEnabled(const char *pos, C4AulWarningId warning) const
{
	if (pOrgScript) return pOrgScript->IsWarningEnabled(pos, warning);
	// In DirectExec, the default warnings are always active.
	switch (warning)
	{
#define DIAG(id, text, enabled) case C4AulWarningId::id: return enabled;
#include "C4AulWarnings.h"
#undef DIAG
	default: return false;
	}
}

void C4AulParse::Error(const char *pMsg, ...)
{
	va_list args; va_start(args, pMsg);
	StdStrBuf Buf;
	Buf.FormatV(pMsg, args);

	throw C4AulParseError(this, Buf.getData());
}

void C4AulParse::AppendPosition(StdStrBuf & Buf)
{
	if (Fn && Fn->GetName())
	{
		// Show function name
		Buf.AppendFormat(" (in %s", Fn->GetName());

		// Exact position
		if (Fn->pOrgScript && TokenSPos)
			Buf.AppendFormat(", %s:%d:%d)",
			                      Fn->pOrgScript->ScriptName.getData(),
			                      SGetLine(Fn->pOrgScript->GetScript(), TokenSPos),
			                      SLineGetCharacters(Fn->pOrgScript->GetScript(), TokenSPos));
		else
			Buf.AppendChar(')');
	}
	else if (pOrgScript)
	{
		// Script name
		Buf.AppendFormat(" (%s:%d:%d)",
		                      pOrgScript->ScriptName.getData(),
		                      SGetLine(pOrgScript->GetScript(), TokenSPos),
		                      SLineGetCharacters(pOrgScript->GetScript(), TokenSPos));
	}
	// show a warning if the error is in a remote script
	if (pOrgScript != Host && Host)
		Buf.AppendFormat(" (as #appendto/#include to %s)", Host->ScriptName.getData());
}

C4AulParseError::C4AulParseError(C4AulParse * state, const char *pMsg)
{
	// compose error string
	sMessage.Copy(pMsg);
	state->AppendPosition(sMessage);
}

C4AulParseError::C4AulParseError(C4ScriptHost *pScript, const char *pMsg)
{
	// compose error string
	sMessage.Copy(pMsg);
	if (pScript)
	{
		// Script name
		sMessage.AppendFormat(" (%s)",
		                      pScript->ScriptName.getData());
	}
}

C4AulParseError::C4AulParseError(C4AulScriptFunc * Fn, const char *SPos, const char *pMsg)
{
	// compose error string
	sMessage.Copy(pMsg);
	if (!Fn) return;
	sMessage.Append(" (");
	// Show function name
	if (Fn->GetName())
		sMessage.AppendFormat("in %s", Fn->GetName());
	if (Fn->GetName() && Fn->pOrgScript && SPos)
		sMessage.Append(", ");
	// Exact position
	if (Fn->pOrgScript && SPos)
		sMessage.AppendFormat("%s:%d:%d)",
				      Fn->pOrgScript->ScriptName.getData(),
				      SGetLine(Fn->pOrgScript->GetScript(), SPos),
				      SLineGetCharacters(Fn->pOrgScript->GetScript(), SPos));
	else
		sMessage.AppendChar(')');
}

bool C4AulParse::AdvanceSpaces()
{
	if (!SPos)
		return false;
	while(*SPos)
	{
		if (*SPos == '/')
		{
			// // comment
			if (SPos[1] == '/')
			{
				SPos += 2;
				while (*SPos && *SPos != 13 && *SPos != 10)
					++SPos;
			}
			// /* comment */
			else if (SPos[1] == '*')
			{
				SPos += 2;
				while (*SPos && (*SPos != '*' || SPos[1] != '/'))
					++SPos;
				SPos += 2;
			}
			else
				return true;
		}
		// Skip any "zero width no-break spaces" (also known as Byte Order Marks)
		else if (*SPos == '\xEF' && SPos[1] == '\xBB' && SPos[2] == '\xBF')
			SPos += 3;
		else if ((unsigned)*SPos > 32)
			return true;
		else
			++SPos;
	}
	// end of script reached
	return false;
}

//=========================== C4Script Operator Map ===================================
const C4ScriptOpDef C4ScriptOpMap[] =
{
	// priority                      postfix
	// |  identifier                 |  changer
	// |  |     Bytecode             |  |  no second id
	// |  |     |                    |  |  |  RetType   ParType1    ParType2
	// prefix
	{ 15, "++", AB_Inc,              false, true, false, C4V_Int,  C4V_Int,    C4V_Any},
	{ 15, "--", AB_Dec,              false, true, false, C4V_Int,  C4V_Int,    C4V_Any},
	{ 15, "~",  AB_BitNot,           false, false, false, C4V_Int,  C4V_Int,    C4V_Any},
	{ 15, "!",  AB_Not,              false, false, false, C4V_Bool, C4V_Bool,   C4V_Any},
	{ 15, "+",  AB_ERR,              false, false, false, C4V_Int,  C4V_Int,    C4V_Any},
	{ 15, "-",  AB_Neg,              false, false, false, C4V_Int,  C4V_Int,    C4V_Any},
	
	// postfix (whithout second statement)
	{ 16, "++", AB_Inc,              true, true, true, C4V_Int,  C4V_Int,    C4V_Any},
	{ 16, "--", AB_Dec,              true, true, true, C4V_Int,  C4V_Int,    C4V_Any},
	
	// postfix
	{ 14, "**", AB_Pow,              true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 13, "/",  AB_Div,              true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 13, "*",  AB_Mul,              true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 13, "%",  AB_Mod,              true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 12, "-",  AB_Sub,              true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 12, "+",  AB_Sum,              true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 11, "<<", AB_LeftShift,        true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 11, ">>", AB_RightShift,       true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 10, "<",  AB_LessThan,         true, false, false, C4V_Bool, C4V_Int,    C4V_Int},
	{ 10, "<=", AB_LessThanEqual,    true, false, false, C4V_Bool, C4V_Int,    C4V_Int},
	{ 10, ">",  AB_GreaterThan,      true, false, false, C4V_Bool, C4V_Int,    C4V_Int},
	{ 10, ">=", AB_GreaterThanEqual, true, false, false, C4V_Bool, C4V_Int,    C4V_Int},
	{ 9, "==",  AB_Equal,            true, false, false, C4V_Bool, C4V_Any,    C4V_Any},
	{ 9, "!=",  AB_NotEqual,         true, false, false, C4V_Bool, C4V_Any,    C4V_Any},
	{ 8, "&",   AB_BitAnd,           true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 6, "^",   AB_BitXOr,           true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 6, "|",   AB_BitOr,            true, false, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 5, "&&",  AB_JUMPAND,          true, false, false, C4V_Bool, C4V_Bool,   C4V_Bool},
	{ 4, "||",  AB_JUMPOR,           true, false, false, C4V_Bool, C4V_Bool,   C4V_Bool},
	{ 3, "??",  AB_JUMPNNIL,         true, false, false, C4V_Any,  C4V_Any,    C4V_Any},
	
	// changers
	{ 2, "*=",  AB_Mul,              true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "/=",  AB_Div,              true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "%=",  AB_Mod,              true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "+=",  AB_Sum,              true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "-=",  AB_Sub,              true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "&=",  AB_BitAnd,           true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "|=",  AB_BitOr,            true, true, false, C4V_Int,  C4V_Int,    C4V_Int},
	{ 2, "^=",  AB_BitXOr,           true, true, false, C4V_Int,  C4V_Int,    C4V_Int},

	{ 0, nullptr,  AB_ERR,              false, false, false, C4V_Nil,  C4V_Nil,    C4V_Nil}
};

int C4AulParse::GetOperator(const char* pScript)
{
	// return value:
	// >= 0: operator found. could be found in C4ScriptOfDef
	// -1:   isn't an operator

	unsigned int i;

	if (!*pScript) return 0;
	// operators are not alphabetical
	if ((*pScript >= 'a' && *pScript <= 'z') ||
	    (*pScript >= 'A' && *pScript <= 'Z'))
	{
		return -1;
	}

	// find the longest operator
	int len = 0; int maxfound = -1;
	for (i=0; C4ScriptOpMap[i].Identifier; i++)
	{
		if (SEqual2(pScript, C4ScriptOpMap[i].Identifier))
		{
			int oplen = SLen(C4ScriptOpMap[i].Identifier);
			if (oplen > len)
			{
				len = oplen;
				maxfound = i;
			}
		}
	}
	return maxfound;
}

void C4AulParse::ClearToken()
{
	// if last token was a string, make sure its ref is deleted
	if (TokenType == ATT_STRING && cStr)
	{
		cStr->DecRef();
		TokenType = ATT_INVALID;
	}
}

C4AulTokenType C4AulParse::GetNextToken()
{
	// clear mem of prev token
	ClearToken();
	// move to start of token
	if (!AdvanceSpaces()) return ATT_EOF;
	// store offset
	TokenSPos = SPos;

	// get char
	char C = *(SPos++);
	// Mostly sorted by frequency, except that tokens that have
	// other tokens as prefixes need to be checked for first.
	if (Inside(C, 'a', 'z') || Inside(C, 'A', 'Z') || C == '_' || C == '#')
	{
		// identifier or directive
		bool dir = C == '#';
		int Len = 1;
		C = *SPos;
		while (Inside(C, '0', '9') || Inside(C, 'a', 'z') || Inside(C, 'A', 'Z') || C == '_')
		{
			++Len;
			C = *(++SPos);
		}

		// Special case for #warning because we don't want to give it to the parser
		if (dir && SEqual2(TokenSPos, C4AUL_Warning))
		{
			// Look for end of line or end of file
			while (*SPos != '\n' && *SPos != '\0') ++SPos;
			Parse_WarningPragma();
			// And actually return the next token.
			return GetNextToken();
		}

		Len = std::min(Len, C4AUL_MAX_Identifier);
		SCopy(TokenSPos, Idtf, Len);
		return dir ? ATT_DIR : ATT_IDTF;
	}
	else if (C == '(') return ATT_BOPEN;  // "("
	else if (C == ')') return ATT_BCLOSE; // ")"
	else if (C == ',') return ATT_COMMA;  // ","
	else if (C == ';') return ATT_SCOLON; // ";"
	else if (Inside(C, '0', '9'))
	{
		// integer
		if (C == '0' && *SPos == 'x')
		{
			// hexadecimal
			cInt = StrToI32(SPos + 1, 16, &SPos);
			return ATT_INT;
		}
		else
		{
			// decimal
			cInt = StrToI32(TokenSPos, 10, &SPos);
			return ATT_INT;
		}
	}
	else if (C == '-' && *SPos == '>' && *(SPos + 1) == '~')
		{ SPos+=2; return ATT_CALLFS;}// "->~"
	else if (C == '-' && *SPos == '>')
		{ ++SPos;  return ATT_CALL; } // "->"
	else if ((cInt = GetOperator(SPos - 1)) != -1)
	{
		SPos += SLen(C4ScriptOpMap[cInt].Identifier) - 1;
		return ATT_OPERATOR;
	}
	else if (C == '=') return ATT_SET;    // "="
	else if (C == '{') return ATT_BLOPEN; // "{"
	else if (C == '}') return ATT_BLCLOSE;// "}"
	else if (C == '"')
	{
		// string
		std::string strbuf;
		strbuf.reserve(512); // assume most strings to be smaller than this
		// string end
		while (*SPos != '"')
		{
			C = *SPos;
			++SPos;
			if (C == '\\') // escape
				switch (*SPos)
				{
				case '"':  ++SPos; strbuf.push_back('"');  break;
				case '\\': ++SPos; strbuf.push_back('\\'); break;
				case 'n':  ++SPos; strbuf.push_back('\n'); break;
				case 't':  ++SPos; strbuf.push_back('\t'); break;
				case 'x':
				{
					++SPos;
					// hexadecimal escape: \xAD.
					// First char must be a hexdigit
					if (!std::isxdigit(*SPos))
					{
						Warn(C4AulWarningId::invalid_hex_escape);
						strbuf.push_back('\\'); strbuf.push_back('x');
					}
					else
					{
						char ch = 0;
						while (std::isxdigit(*SPos))
						{
							ch *= 16;
							if (*SPos >= '0' && *SPos <= '9')
								ch += *SPos - '0';
							else if (*SPos >= 'a' && *SPos <= 'f')
								ch += *SPos - 'a' + 10;
							else if (*SPos >= 'A' && *SPos <= 'F')
								ch += *SPos - 'A' + 10;
							++SPos;
						};
						strbuf.push_back(ch);
					}
					break;
				}
				case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
				{
					// Octal escape: \142
					char ch = 0;
					while (SPos[0] >= '0' && SPos[0] <= '7')
					{
						ch *= 8;
						ch += *SPos++ -'0';
					}
					strbuf.push_back(ch);
					break;
				}
				default:
				{
					// just insert "\"
					strbuf.push_back('\\');
					// show warning
					Warn(C4AulWarningId::invalid_escape_sequence, *(SPos + 1));
				}
				}
			else if (C == 0 || C == 10 || C == 13) // line break / feed
				throw C4AulParseError(this, "string not closed");
			else
				// copy character
				strbuf.push_back(C);
		}
		++SPos;
		cStr = Strings.RegString(StdStrBuf(strbuf.data(),strbuf.size()));
		// hold onto string, ClearToken will deref it
		cStr->IncRef();
		return ATT_STRING;
	}
	else if (C == '[') return ATT_BOPEN2; // "["
	else if (C == ']') return ATT_BCLOSE2;// "]"
	else if (C == '.' && *SPos == '.' && *(SPos + 1) == '.')
		{ SPos+=2; return ATT_LDOTS; } // "..."
	else if (C == '.') return ATT_DOT;    // "."
	else if (C == ':') return ATT_COLON;  // ":"
	else
	{
		// show appropriate error message
		if (C >= '!' && C <= '~')
			throw C4AulParseError(this, FormatString("unexpected character '%c' found", C).getData());
		else
			throw C4AulParseError(this, FormatString(R"(unexpected character \x%x found)", (int)(unsigned char) C).getData());
	}
}

static const char * GetTTName(C4AulBCCType e)
{
	switch (e)
	{
	case AB_ARRAYA: return "ARRAYA";  // array access
	case AB_ARRAYA_SET: return "ARRAYA_SET";  // setter
	case AB_PROP: return "PROP";
	case AB_PROP_SET: return "PROP_SET";
	case AB_ARRAY_SLICE: return "ARRAY_SLICE";
	case AB_ARRAY_SLICE_SET: return "ARRAY_SLICE_SET";
	case AB_STACK_SET: return "STACK_SET";
	case AB_LOCALN: return "LOCALN";  // a named local
	case AB_LOCALN_SET: return "LOCALN_SET";
	case AB_GLOBALN: return "GLOBALN";  // a named global
	case AB_GLOBALN_SET: return "GLOBALN_SET";
	case AB_PAR: return "PAR";      // Par statement
	case AB_THIS: return "THIS";
	case AB_FUNC: return "FUNC";    // function

// prefix
	case AB_Inc: return "Inc";  // ++
	case AB_Dec: return "Dec";  // --
	case AB_BitNot: return "BitNot";  // ~
	case AB_Not: return "Not";  // !
	case AB_Neg: return "Neg";  // -

// postfix
	case AB_Pow: return "Pow";  // **
	case AB_Div: return "Div";  // /
	case AB_Mul: return "Mul";  // *
	case AB_Mod: return "Mod";  // %
	case AB_Sub: return "Sub";  // -
	case AB_Sum: return "Sum";  // +
	case AB_LeftShift: return "LeftShift";  // <<
	case AB_RightShift: return "RightShift";  // >>
	case AB_LessThan: return "LessThan";  // <
	case AB_LessThanEqual: return "LessThanEqual";  // <=
	case AB_GreaterThan: return "GreaterThan";  // >
	case AB_GreaterThanEqual: return "GreaterThanEqual";  // >=
	case AB_Equal: return "Equal";  // ==
	case AB_NotEqual: return "NotEqual";  // !=
	case AB_BitAnd: return "BitAnd";  // &
	case AB_BitXOr: return "BitXOr";  // ^
	case AB_BitOr: return "BitOr";  // |

	case AB_CALL: return "CALL";    // direct object call
	case AB_CALLFS: return "CALLFS";  // failsafe direct call
	case AB_STACK: return "STACK";    // push nulls / pop
	case AB_INT: return "INT";      // constant: int
	case AB_BOOL: return "BOOL";    // constant: bool
	case AB_STRING: return "STRING";  // constant: string
	case AB_CPROPLIST: return "CPROPLIST"; // constant: proplist
	case AB_CARRAY: return "CARRAY";  // constant: array
	case AB_CFUNCTION: return "CFUNCTION";  // constant: function
	case AB_NIL: return "NIL";    // constant: nil
	case AB_NEW_ARRAY: return "NEW_ARRAY";    // semi-constant: array
	case AB_DUP: return "DUP";    // duplicate value from stack
	case AB_DUP_CONTEXT: return "AB_DUP_CONTEXT"; // duplicate value from stack of parent function
	case AB_NEW_PROPLIST: return "NEW_PROPLIST";    // create a new proplist
	case AB_POP_TO: return "POP_TO";    // initialization of named var
	case AB_JUMP: return "JUMP";    // jump
	case AB_JUMPAND: return "JUMPAND";
	case AB_JUMPOR: return "JUMPOR";
	case AB_JUMPNNIL: return "JUMPNNIL"; // nil-coalescing operator ("??")
	case AB_CONDN: return "CONDN";    // conditional jump (negated, pops stack)
	case AB_COND: return "COND";    // conditional jump (pops stack)
	case AB_FOREACH_NEXT: return "FOREACH_NEXT"; // foreach: next element
	case AB_RETURN: return "RETURN";  // return statement
	case AB_ERR: return "ERR";      // parse error at this position
	case AB_DEBUG: return "DEBUG";      // debug break
	case AB_EOFN: return "EOFN";    // end of function
	}
	assert(false); return "UNKNOWN";
}

void C4AulScriptFunc::DumpByteCode()
{
	if (DEBUG_BYTECODE_DUMP)
	{
		fprintf(stderr, "%s:\n", GetName());
		std::map<C4AulBCC *, int> labels;
		int labeln = 0;
		for (auto & bcc: Code)
		{
			switch (bcc.bccType)
			{
			case AB_JUMP: case AB_JUMPAND: case AB_JUMPOR: case AB_JUMPNNIL: case AB_CONDN: case AB_COND:
				labels[&bcc + bcc.Par.i] = ++labeln; break;
			default: break;
			}
		}
		for (auto & bcc: Code)
		{
			C4AulBCCType eType = bcc.bccType;
			if (labels.find(&bcc) != labels.end())
				fprintf(stderr, "%d:\n", labels[&bcc]);
			fprintf(stderr, "\t%d\t%-20s", GetLineOfCode(&bcc), GetTTName(eType));
			switch (eType)
			{
			case AB_FUNC:
				fprintf(stderr, "\t%s\n", bcc.Par.f->GetFullName().getData()); break;
			case AB_ERR:
				if (bcc.Par.s)
			case AB_CALL: case AB_CALLFS: case AB_LOCALN: case AB_LOCALN_SET: case AB_PROP: case AB_PROP_SET:
				fprintf(stderr, "\t%s\n", bcc.Par.s->GetCStr()); break;
			case AB_STRING:
			{
				const StdStrBuf &s = bcc.Par.s->GetData();
				std::string es;
				std::for_each(s.getData(), s.getData() + s.getLength(), [&es](char c) {
					if (std::isgraph((unsigned char)c))
					{
						es += c;
					}
					else
					{
						switch (c)
						{
						case '\'': es.append(R"(\')"); break;
						case '\"': es.append(R"(\")"); break;
						case '\\': es.append(R"(\\)"); break;
						case '\a': es.append(R"(\a)"); break;
						case '\b': es.append(R"(\b)"); break;
						case '\f': es.append(R"(\f)"); break;
						case '\n': es.append(R"(\n)"); break;
						case '\r': es.append(R"(\r)"); break;
						case '\t': es.append(R"(\t)"); break;
						case '\v': es.append(R"(\v)"); break;
						default:
						{
							std::stringstream hex;
							hex << R"(\x)" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>((unsigned char)c);
							es.append(hex.str());
							break;
						}
						}
					}
				});
				fprintf(stderr, "\t\"%s\"\n", es.c_str()); break;
			}
			case AB_DEBUG: case AB_NIL: case AB_RETURN:
			case AB_PAR: case AB_THIS:
			case AB_ARRAYA: case AB_ARRAYA_SET: case AB_ARRAY_SLICE: case AB_ARRAY_SLICE_SET:
			case AB_EOFN:
				assert(!bcc.Par.X); fprintf(stderr, "\n"); break;
			case AB_CARRAY:
				fprintf(stderr, "\t%s\n", C4VArray(bcc.Par.a).GetDataString().getData()); break;
			case AB_CPROPLIST:
				fprintf(stderr, "\t%s\n", C4VPropList(bcc.Par.p).GetDataString().getData()); break;
			case AB_JUMP: case AB_JUMPAND: case AB_JUMPOR: case AB_JUMPNNIL: case AB_CONDN: case AB_COND:
				fprintf(stderr, "\t% -d\n", labels[&bcc + bcc.Par.i]); break;
			default:
				fprintf(stderr, "\t% -d\n", bcc.Par.i); break;
			}
		}
	}
}

bool C4ScriptHost::Preparse()
{
	// handle easiest case first
	if (State < ASS_NONE) return false;

	// clear stuff
	Includes.clear(); Appends.clear();

	GetPropList()->C4PropList::Clear();
	GetPropList()->SetProperty(P_Prototype, C4VPropList(Engine->GetPropList()));
	LocalValues.Clear();

	// Add any engine functions specific to this script
	AddEngineFunctions();

	// Insert default warnings
	assert(enabledWarnings.empty());
	auto &warnings = enabledWarnings[Script.getData()];
#define DIAG(id, text, enabled) warnings.set(static_cast<size_t>(C4AulWarningId::id), enabled);
#include "C4AulWarnings.h"
#undef DIAG

	C4AulParse parser(this);
	ast = parser.Parse_Script(this);

	C4AulCompiler::Preparse(this, this, ast.get());

	// #include will have to be resolved now...
	IncludesResolved = false;

	// Parse will write the properties back after the ones from included scripts
	GetPropList()->Properties.Swap(&LocalValues);

	// return success
	this->State = ASS_PREPARSED;
	return true;
}

static const char * GetTokenName(C4AulTokenType TokenType)
{
	switch (TokenType)
	{
	case ATT_INVALID: return "invalid token";
	case ATT_DIR: return "directive";
	case ATT_IDTF: return "identifier";
	case ATT_INT: return "integer constant";
	case ATT_STRING: return "string constant";
	case ATT_DOT: return "'.'";
	case ATT_COMMA: return "','";
	case ATT_COLON: return "':'";
	case ATT_SCOLON: return "';'";
	case ATT_BOPEN: return "'('";
	case ATT_BCLOSE: return "')'";
	case ATT_BOPEN2: return "'['";
	case ATT_BCLOSE2: return "']'";
	case ATT_BLOPEN: return "'{'";
	case ATT_BLCLOSE: return "'}'";
	case ATT_CALL: return "'->'";
	case ATT_CALLFS: return "'->~'";
	case ATT_LDOTS: return "'...'";
	case ATT_SET: return "'='";
	case ATT_OPERATOR: return "operator";
	case ATT_EOF: return "end of file";
	default: return "unrecognized token";
	}
}

void C4AulParse::Shift()
{
	TokenType = GetNextToken();
}
void C4AulParse::Check(C4AulTokenType RefTokenType, const char * Expected)
{
	if (TokenType != RefTokenType)
		UnexpectedToken(Expected ? Expected : GetTokenName(RefTokenType));
}
void C4AulParse::Match(C4AulTokenType RefTokenType, const char * Expected)
{
	Check(RefTokenType, Expected);
	Shift();
}
void C4AulParse::UnexpectedToken(const char * Expected)
{
	throw C4AulParseError(this, FormatString("%s expected, but found %s", Expected, GetTokenName(TokenType)).getData());
}

void C4AulParse::Parse_WarningPragma()
{
	assert(SEqual2(TokenSPos, C4AUL_Warning));
	assert(std::isspace(TokenSPos[sizeof(C4AUL_Warning) - 1]));


	// Read parameters in to string buffer. The sizeof() includes the terminating \0, but
	// that's okay because we need to skip (at least) one whitespace character anyway.
	std::string line(TokenSPos + sizeof(C4AUL_Warning), SPos);
	auto end = line.end();
	auto cursor = std::find_if_not(begin(line), end, IsWhiteSpace);

	if (cursor == end)
		throw C4AulParseError(this, "'" C4Aul_Warning_enable "' or '" C4Aul_Warning_disable "' expected, but found end of line");

	// Split directive on whitespace
	auto start = cursor;
	cursor = std::find_if(start, end, IsWhiteSpace);
	bool enable_warning = false;
	if (std::equal(start, cursor, C4Aul_Warning_enable))
	{
		enable_warning = true;
	}
	else if (std::equal(start, cursor, C4Aul_Warning_disable))
	{
		enable_warning = false;
	}
	else
	{
		throw C4AulParseError(this, FormatString("'" C4Aul_Warning_enable "' or '" C4Aul_Warning_disable "' expected, but found '%s'", std::string(start, cursor).c_str()).getData());
	}

	cursor = std::find_if_not(cursor, end, IsWhiteSpace);
	if (cursor == end)
	{
		// enable or disable all warnings
#define DIAG(id, text, enabled) pOrgScript->EnableWarning(TokenSPos, C4AulWarningId::id, enable_warning);
#include "C4AulWarnings.h"
#undef DIAG
		return;
	}

	// enable or disable specific warnings
	static const std::map<std::string, C4AulWarningId> warnings{
#define DIAG(id, text, enabled) std::make_pair(#id, C4AulWarningId::id),
#include "C4AulWarnings.h"
#undef DIAG
	};
	while (cursor != end)
	{
		start = std::find_if_not(cursor, end, IsWhiteSpace);
		cursor = std::find_if(start, end, IsWhiteSpace);
		auto entry = warnings.find(std::string(start, cursor));
		if (entry != warnings.end())
		{
			pOrgScript->EnableWarning(TokenSPos, entry->second, enable_warning);
		}
	}
}

void C4AulScriptFunc::ParseDirectExecFunc(C4AulScriptEngine *Engine, C4AulScriptContext* context)
{
	ClearCode();
	// parse
	C4AulParse state(this, context, Engine);
	auto func = state.Parse_DirectExec(Script, true);
	C4AulCompiler::Compile(this, func.get());
}

void C4AulScriptFunc::ParseDirectExecStatement(C4AulScriptEngine *Engine, C4AulScriptContext* context)
{
	ClearCode();
	// parse
	C4AulParse state(this, context, Engine);
	auto func = state.Parse_DirectExec(Script, false);
	C4AulCompiler::Compile(this, func.get());
}

std::unique_ptr<::aul::ast::FunctionDecl> C4AulParse::Parse_DirectExec(const char *code, bool whole_function)
{
	// get first token
	Shift();
	// Synthesize a wrapping function which we can call
	std::unique_ptr<::aul::ast::FunctionDecl> func;
	if (whole_function)
	{
		func = Parse_ToplevelFunctionDecl();
	}
	else
	{
		auto expr = Parse_Expression();
		func = std::make_unique<::aul::ast::FunctionDecl>("$internal$eval");
		func->body = std::make_unique<::aul::ast::Block>();
		func->body->children.push_back(std::make_unique<::aul::ast::Return>(std::move(expr)));
	}
	Match(ATT_EOF);
	return func;
}

std::unique_ptr<::aul::ast::Script> C4AulParse::Parse_Script(C4ScriptHost * scripthost)
{
	pOrgScript = scripthost;
	SPos = pOrgScript->Script.getData();
	const char * SPos0 = SPos;
	bool first_error = true;
	auto script = ::aul::ast::Script::New(SPos0);
	while (true) try
	{
		// Go to the next token if the current token could not be processed or no token has yet been parsed
		if (SPos == SPos0)
		{
			Shift();
		}
		SPos0 = SPos;
		switch (TokenType)
		{
		case ATT_DIR:
			// check for include statement
			if (SEqual(Idtf, C4AUL_Include))
			{
				Shift();
				// get id of script to include
				Check(ATT_IDTF, "script name");
				script->declarations.push_back(::aul::ast::IncludePragma::New(TokenSPos, Idtf));
				Shift();
			}
			else if (SEqual(Idtf, C4AUL_Append))
			{
				if (pOrgScript->GetPropList()->GetDef())
					throw C4AulParseError(this, "#appendto in a Definition");
				Shift();
				// get id of script to include/append
				switch (TokenType)
				{
				case ATT_IDTF:
					script->declarations.push_back(::aul::ast::AppendtoPragma::New(TokenSPos, Idtf));
					break;
				case ATT_OPERATOR:
					if (SEqual(C4ScriptOpMap[cInt].Identifier, "*"))
					{
						script->declarations.push_back(::aul::ast::AppendtoPragma::New(TokenSPos));
						break;
					}
					//fallthrough
				default:
					// -> ID expected
					UnexpectedToken("identifier or '*'");
				}
				Shift();
			}
			else
				// -> unknown directive
				Error("unknown directive: %s", Idtf);
			break;
		case ATT_IDTF:
			// need a keyword here to avoid parsing random function contents
			// after a syntax error in a function
			// check for object-local variable definition (local)
			if (SEqual(Idtf, C4AUL_LocalNamed) || SEqual(Idtf, C4AUL_GlobalNamed))
			{
				script->declarations.push_back(Parse_Var());
				Match(ATT_SCOLON);
			}
			// check for variable definition (static)
			else
				script->declarations.push_back(Parse_ToplevelFunctionDecl());
			break;
		case ATT_EOF:
			return script;
		default:
			UnexpectedToken("declaration");
		}
		first_error = true;
	}
	catch (C4AulError &err)
	{
		if (first_error)
		{
			++Engine->errCnt;
			::ScriptEngine.ErrorHandler->OnError(err.what());
		}
		first_error = false;
	}
}

std::unique_ptr<::aul::ast::FunctionDecl> C4AulParse::Parse_ToplevelFunctionDecl()
{
	const char *NodeStart = TokenSPos;
	bool is_global = SEqual(Idtf, C4AUL_Global);
	// skip access modifier
	if (SEqual(Idtf, C4AUL_Private) ||
		SEqual(Idtf, C4AUL_Protected) ||
		SEqual(Idtf, C4AUL_Public) ||
		SEqual(Idtf, C4AUL_Global))
	{
		Shift();
	}

	// check for func declaration
	if (!SEqual(Idtf, C4AUL_Func))
		Error("Declaration expected, but found identifier: %s", Idtf);
	Shift();
	// get next token, must be func name
	Check(ATT_IDTF, "function name");

	auto func = ::aul::ast::FunctionDecl::New(NodeStart, Idtf);
	func->is_global = is_global;
	Shift();
	Parse_Function(func.get());
	return func;
}

void C4AulParse::Parse_Function(::aul::ast::Function *func)
{
	Match(ATT_BOPEN);
	// get pars
	while (TokenType != ATT_BCLOSE)
	{
		// too many parameters?
		if (func->params.size() >= C4AUL_MAX_Par)
			throw C4AulParseError(this, "'func' parameter list: too many parameters (max 10)");
		if (TokenType == ATT_LDOTS)
		{
			func->has_unnamed_params = true;
			Shift();
			// don't allow any more parameters after ellipsis
			break;
		}
		// must be a name or type now
		Check(ATT_IDTF, "parameter, '...', or ')'");
		// type identifier?
		C4V_Type t = C4V_Any;
		if (SEqual(Idtf, C4AUL_TypeInt)) { t = C4V_Int; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeBool)) { t = C4V_Bool; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeC4ID)) { t = C4V_Def; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeDef)) { t = C4V_Def; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeEffect)) { t = C4V_Effect; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeC4Object)) { t = C4V_Object; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypePropList)) { t = C4V_PropList; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeString)) { t = C4V_String; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeArray)) { t = C4V_Array; Shift(); }
		else if (SEqual(Idtf, C4AUL_TypeFunction)) { t = C4V_Function; Shift(); }
		// a parameter name which matched a type name?
		std::string par_name;
		if (TokenType == ATT_BCLOSE || TokenType == ATT_COMMA)
		{
			par_name = Idtf;
			Warn(C4AulWarningId::type_name_used_as_par_name, Idtf);
		}
		else
		{
			Check(ATT_IDTF, "parameter name");
			par_name = Idtf;
			Shift();
		}
		func->params.emplace_back(par_name, t);
		// end of params?
		if (TokenType == ATT_BCLOSE)
		{
			break;
		}
		// must be a comma now
		Match(ATT_COMMA, "',' or ')'");
	}
	Match(ATT_BCLOSE);
	func->body = Parse_Block();
}

std::unique_ptr<::aul::ast::Block> C4AulParse::Parse_Block()
{
	auto block = ::aul::ast::Block::New(TokenSPos);
	Match(ATT_BLOPEN);
	while (TokenType != ATT_BLCLOSE)
	{
		block->children.push_back(Parse_Statement());
	}
	Shift();
	return block;
}

std::unique_ptr<::aul::ast::Stmt> C4AulParse::Parse_Statement()
{
	const char *NodeStart = TokenSPos;
	std::unique_ptr<::aul::ast::Stmt> stmt;
	switch (TokenType)
	{
		// do we have a block start?
	case ATT_BLOPEN:
		return Parse_Block();
	case ATT_BOPEN:
	case ATT_BOPEN2:
	case ATT_SET:
	case ATT_OPERATOR:
	case ATT_INT:
	case ATT_STRING:
	{
		stmt = Parse_Expression();
		Match(ATT_SCOLON);
		return stmt;
	}
	// additional function separator
	case ATT_SCOLON:
		Shift();
		return ::aul::ast::Noop::New(NodeStart);
	case ATT_IDTF:
		// check for variable definition
		if (SEqual(Idtf, C4AUL_VarNamed) || SEqual(Idtf, C4AUL_LocalNamed) || SEqual(Idtf, C4AUL_GlobalNamed))
			stmt = Parse_Var();
		// check new-form func begin
		else if (SEqual(Idtf, C4AUL_Func) ||
		         SEqual(Idtf, C4AUL_Private) ||
		         SEqual(Idtf, C4AUL_Protected) ||
		         SEqual(Idtf, C4AUL_Public) ||
		         SEqual(Idtf, C4AUL_Global))
		{
			throw C4AulParseError(this, "unexpected end of function");
		}
		// get function by identifier: first check special functions
		else if (SEqual(Idtf, C4AUL_If)) // if
		{
			return Parse_If();
		}
		else if (SEqual(Idtf, C4AUL_Else)) // else
		{
			throw C4AulParseError(this, "misplaced 'else'");
		}
		else if (SEqual(Idtf, C4AUL_Do)) // while
		{
			stmt = Parse_DoWhile();
		}
		else if (SEqual(Idtf, C4AUL_While)) // while
		{
			return Parse_While();
		}
		else if (SEqual(Idtf, C4AUL_For)) // for
		{
			PushParsePos();
			Shift();
			// Look if it's the for([var] foo in array)-form
			// must be followed by a bracket
			Match(ATT_BOPEN);
			// optional var
			if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
				Shift();
			// variable and "in"
			if (TokenType == ATT_IDTF
			    && GetNextToken() == ATT_IDTF
			    && SEqual(Idtf, C4AUL_In))
			{
				// reparse the stuff in the brackets like normal statements
				PopParsePos();
				return Parse_ForEach();
			}
			else
			{
				// reparse the stuff in the brackets like normal statements
				PopParsePos();
				return Parse_For();
			}
		}
		else if (SEqual(Idtf, C4AUL_Return)) // return
		{
			Shift();
			if (TokenType == ATT_SCOLON)
			{
				// allow return; without return value (implies nil)
				stmt = ::aul::ast::Return::New(NodeStart, ::aul::ast::NilLit::New(NodeStart));
			}
			else
			{
				// return retval;
				stmt = ::aul::ast::Return::New(NodeStart, Parse_Expression());
			}
		}
		else if (SEqual(Idtf, C4AUL_Break)) // break
		{
			Shift();
			stmt = ::aul::ast::Break::New(NodeStart);
		}
		else if (SEqual(Idtf, C4AUL_Continue)) // continue
		{
			Shift();
			stmt = ::aul::ast::Continue::New(NodeStart);
		}
		else
		{
			stmt = Parse_Expression();
		}
		Match(ATT_SCOLON);
		assert(stmt);
		return stmt;
	default:
		UnexpectedToken("statement");
	}
}

void C4AulParse::Parse_CallParams(::aul::ast::CallExpr *call)
{
	assert(call != nullptr);
	assert(call->args.empty());

	// so it's a regular function; force "("
	Match(ATT_BOPEN);
	while(TokenType != ATT_BCLOSE) switch(TokenType)
	{
	case ATT_COMMA:
		// got no parameter before a ","
		Warn(C4AulWarningId::empty_parameter_in_call, (unsigned)call->args.size(), call->callee.c_str());
		call->args.push_back(::aul::ast::NilLit::New(TokenSPos));
		Shift();
		break;
	case ATT_LDOTS:
		// functions using ... always take as many parameters as possible
		Shift();
		call->append_unnamed_pars = true;
		// Do not allow more parameters even if there is space left
		Check(ATT_BCLOSE);
		break;
	default:
		// get a parameter
		call->args.push_back(Parse_Expression());
		// end of parameter list?
		if (TokenType != ATT_BCLOSE)
			Match(ATT_COMMA, "',' or ')'");
		break;
	}
	Match(ATT_BCLOSE);
}

std::unique_ptr<::aul::ast::ArrayLit> C4AulParse::Parse_Array()
{
	auto arr = ::aul::ast::ArrayLit::New(TokenSPos);
	// force "["
	Match(ATT_BOPEN2);
	// Create an array
	while (TokenType != ATT_BCLOSE2)
	{
		// got no parameter before a ","? then push nil
		if (TokenType == ATT_COMMA)
		{
			Warn(C4AulWarningId::empty_parameter_in_array, (unsigned)arr->values.size());
			arr->values.emplace_back(::aul::ast::NilLit::New(TokenSPos));
		}
		else
			arr->values.emplace_back(Parse_Expression());
		if (TokenType == ATT_BCLOSE2)
			break;
		Match(ATT_COMMA, "',' or ']'");
		// [] -> size 0, [*,] -> size 2, [*,*,] -> size 3
		if (TokenType == ATT_BCLOSE2)
		{
			Warn(C4AulWarningId::empty_parameter_in_array, (unsigned)arr->values.size());
			arr->values.emplace_back(::aul::ast::NilLit::New(TokenSPos));
		}
	}
	Shift();
	return arr;
}

std::unique_ptr<::aul::ast::ProplistLit> C4AulParse::Parse_PropList()
{
	auto proplist = ::aul::ast::ProplistLit::New(TokenSPos);
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_New))
	{
		Shift();
		proplist->values.emplace_back(Strings.P[P_Prototype].GetCStr(), Parse_Expression());
	}
	Match(ATT_BLOPEN);
	while (TokenType != ATT_BLCLOSE)
	{
		std::string key;
		if (TokenType == ATT_IDTF)
		{
			key = Idtf;
			Shift();
		}
		else if (TokenType == ATT_STRING)
		{
			key = cStr->GetCStr();
			Shift();
		}
		else UnexpectedToken("string or identifier");
		if (TokenType != ATT_COLON && TokenType != ATT_SET)
			UnexpectedToken("':' or '='");
		Shift();
		proplist->values.emplace_back(key, Parse_Expression());
		if (TokenType == ATT_COMMA)
			Shift();
		else if (TokenType != ATT_BLCLOSE)
			UnexpectedToken("'}' or ','");
	}
	Shift();
	return proplist;
}

std::unique_ptr<::aul::ast::DoLoop> C4AulParse::Parse_DoWhile()
{
	auto loop = ::aul::ast::DoLoop::New(TokenSPos);
	Shift();
	loop->body = Parse_Statement();
	// Execute condition
	if (TokenType != ATT_IDTF || !SEqual(Idtf, C4AUL_While))
		UnexpectedToken("'while'");
	Shift();
	Match(ATT_BOPEN);
	loop->cond = Parse_Expression();
	Match(ATT_BCLOSE);
	return loop;
}

std::unique_ptr<::aul::ast::WhileLoop> C4AulParse::Parse_While()
{
	auto loop = ::aul::ast::WhileLoop::New(TokenSPos);
	Shift();
	// Execute condition
	Match(ATT_BOPEN);
	loop->cond = Parse_Expression();
	Match(ATT_BCLOSE);
	// Execute body
	loop->body = Parse_Statement();
	return loop;
}

std::unique_ptr<::aul::ast::If> C4AulParse::Parse_If()
{
	auto stmt = ::aul::ast::If::New(TokenSPos);
	Shift();
	Match(ATT_BOPEN);
	stmt->cond = Parse_Expression();
	Match(ATT_BCLOSE);
	// parse controlled statement
	stmt->iftrue = Parse_Statement();
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Else))
	{
		Shift();
		// expect a command now
		stmt->iffalse = Parse_Statement();
	}
	return stmt;
}

std::unique_ptr<::aul::ast::ForLoop> C4AulParse::Parse_For()
{
	auto loop = ::aul::ast::ForLoop::New(TokenSPos);
	Match(ATT_IDTF); Match(ATT_BOPEN);
	// Initialization
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
	{
		loop->init = Parse_Var();
	}
	else if (TokenType != ATT_SCOLON)
	{
		loop->init = Parse_Expression();
	}
	// Consume first semicolon
	Match(ATT_SCOLON);
	// Condition
	if (TokenType != ATT_SCOLON)
	{
		loop->cond = Parse_Expression();
	}
	// Consume second semicolon
	Match(ATT_SCOLON);
	// Incrementor
	if (TokenType != ATT_BCLOSE)
	{
		loop->incr = Parse_Expression();
	}
	// Consume closing bracket
	Match(ATT_BCLOSE);
	loop->body = Parse_Statement();
	return loop;
}

std::unique_ptr<::aul::ast::RangeLoop> C4AulParse::Parse_ForEach()
{
	auto loop = ::aul::ast::RangeLoop::New(TokenSPos);
	Match(ATT_IDTF); Match(ATT_BOPEN);
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
	{
		loop->scoped_var = true;
		Shift();
	}
	// get variable name
	Check(ATT_IDTF, "variable name");
	loop->var = Idtf;
	Shift();
	if (TokenType != ATT_IDTF || !SEqual(Idtf, C4AUL_In))
		UnexpectedToken("'in'");
	Shift();
	// get expression for array
	loop->cond = Parse_Expression();
	Match(ATT_BCLOSE);
	// loop body
	loop->body = Parse_Statement();
	return loop;
}

static bool GetPropertyByS(const C4PropList * p, const char * s, C4Value & v)
{
	C4String * k = Strings.FindString(s);
	if (!k) return false;
	return p->GetPropertyByS(k, &v);
}

std::unique_ptr<::aul::ast::Expr> C4AulParse::Parse_Expression(int iParentPrio)
{
	const char *NodeStart = TokenSPos;
	std::unique_ptr<::aul::ast::Expr> expr;
	const C4ScriptOpDef * op;
	C4AulFunc *FoundFn = nullptr;
	C4Value val;
	switch (TokenType)
	{
	case ATT_IDTF:
		// XXX: Resolving literals here means that you can't create a variable
		// with the names "true", "false", "nil" or "this" anymore. I don't
		// consider this too much of a problem, because there is never a reason
		// to do this and it makes my job a lot easier
		if (SEqual(Idtf, C4AUL_True))
		{
			Shift();
			expr = ::aul::ast::BoolLit::New(NodeStart, true);
		}
		else if (SEqual(Idtf, C4AUL_False))
		{
			Shift();
			expr = ::aul::ast::BoolLit::New(NodeStart, false);
		}
		else if (SEqual(Idtf, C4AUL_Nil))
		{
			Shift();
			expr = ::aul::ast::NilLit::New(NodeStart);
		}
		else if (SEqual(Idtf, C4AUL_this))
		{
			Shift();
			if (TokenType == ATT_BOPEN)
			{
				Shift();
				Match(ATT_BCLOSE);
				// TODO: maybe warn about "this" with parentheses?
			}
			expr = ::aul::ast::ThisLit::New(NodeStart);
		}
		// XXX: Other things that people aren't allowed to do anymore: name their variables or functions any of:
		// "if", "else", "for", "while", "do", "return", or "Par".
		// We could allow variables with these names and disambiguate based on the syntax, but no.
		else if (SEqual(Idtf, C4AUL_If) || SEqual(Idtf, C4AUL_Else) || SEqual(Idtf, C4AUL_For) || SEqual(Idtf, C4AUL_While) || SEqual(Idtf, C4AUL_Do) || SEqual(Idtf, C4AUL_Return))
		{
			Error("reserved identifier not allowed in expressions: %s", Idtf);
		}
		else if (SEqual(Idtf, C4AUL_Par))
		{
			Shift();
			// "Par" is special in that it isn't a function and thus doesn't accept an arbitrary number of parameters
			Match(ATT_BOPEN);
			expr = ::aul::ast::ParExpr::New(NodeStart, Parse_Expression());
			Match(ATT_BCLOSE);
		}
		else if (SEqual(Idtf, C4AUL_New))
		{
			// Because people might call a variables or functions "new", we need to look ahead and guess whether it's a proplist constructor.
			PushParsePos();
			Shift();
			if (TokenType == ATT_IDTF)
			{
				// this must be a proplist because two identifiers can't immediately follow each other
				PopParsePos();
				expr = Parse_PropList();
			}
			else
			{
				// Some non-identifier means this is either a variable, a function, or a syntax error. Which one exactly is something we'll figure out later.
				PopParsePos();
			}
		}
		else if (SEqual(Idtf, C4AUL_Func))
		{
			PushParsePos();
			Shift();
			if (TokenType == ATT_BOPEN)
			{
				auto func = ::aul::ast::FunctionExpr::New(NodeStart);
				Parse_Function(func.get());
				expr = std::move(func);
				DiscardParsePos();
			}
			else
			{
				PopParsePos();
			}
		}
		if (!expr)
		{
			// If we end up here, it must be a proper identifier (or a reserved word that's used as an identifier).
			// Time to look ahead and see whether it's a function call.
			std::string identifier = Idtf;
			Shift();
			if (TokenType == ATT_BOPEN)
			{
				// Well, it looks like one, at least
				auto func = ::aul::ast::CallExpr::New(NodeStart);
				func->callee = identifier;
				Parse_CallParams(func.get());
				expr = std::move(func);
			}
			else
			{
				// It's most certainly not a function call.
				expr = ::aul::ast::VarExpr::New(NodeStart, identifier);
			}
		}
		break;
	case ATT_INT: // constant in cInt
		expr = ::aul::ast::IntLit::New(NodeStart, cInt);
		Shift();
		break;
	case ATT_STRING: // reference in cStr
		expr = ::aul::ast::StringLit::New(NodeStart, cStr->GetCStr());
		Shift();
		break;
	case ATT_OPERATOR:
		// -> must be a prefix operator
		op = &C4ScriptOpMap[cInt];
		// postfix?
		if (op->Postfix)
			// oops. that's wrong
			throw C4AulParseError(this, "postfix operator without first expression");
		Shift();
		// generate code for the following expression
		expr = Parse_Expression(op->Priority);
		if (SEqual(op->Identifier, "+"))
		{
			// This is a no-op.
		}
		else
		{
			expr = ::aul::ast::UnOpExpr::New(NodeStart, op - C4ScriptOpMap, std::move(expr));
		}
		break;
	case ATT_BOPEN:
		Shift();
		expr = Parse_Expression();
		Match(ATT_BCLOSE);
		break;
	case ATT_BOPEN2:
		expr = Parse_Array();
		break;
	case ATT_BLOPEN:
		expr = Parse_PropList();
		break;
	default:
		UnexpectedToken("expression");
	}

	assert(expr);

	while (true)
	{
		NodeStart = TokenSPos;
		switch (TokenType)
		{
		case ATT_SET:
			{
				// back out of any kind of parent operator
				// (except other setters, as those are right-associative)
				if (iParentPrio > 1)
					return expr;
				Shift();
				expr = ::aul::ast::AssignmentExpr::New(NodeStart, std::move(expr), Parse_Expression(1));
				break;
			}
		case ATT_OPERATOR:
			{
				// expect postfix operator
				const C4ScriptOpDef * op = &C4ScriptOpMap[cInt];
				if (!op->Postfix)
				{
					// does an operator with the same name exist?
					// when it's a postfix-operator, it can be used instead.
					const C4ScriptOpDef * postfixop;
					for (postfixop = op + 1; postfixop->Identifier; ++postfixop)
						if (SEqual(op->Identifier, postfixop->Identifier))
							if (postfixop->Postfix)
								break;
					// not found?
					if (!postfixop->Identifier)
					{
						Error("unexpected prefix operator: %s", op->Identifier);
					}
					// otherwise use the new-found correct postfix operator
					op = postfixop;
				}

				if (iParentPrio + !op->Changer > op->Priority)
					return expr;

				Shift();
				if (op->NoSecondStatement)
				{
					// Postfix unary op
					expr = ::aul::ast::UnOpExpr::New(NodeStart, op - C4ScriptOpMap, std::move(expr));
				}
				else
				{
					expr = ::aul::ast::BinOpExpr::New(NodeStart, op - C4ScriptOpMap, std::move(expr), Parse_Expression(op->Priority));
				}
				break;
			}
		case ATT_BOPEN2:
			{
				// parse either [index], or [start:end] in which case either index is optional
				Shift();
				::aul::ast::ExprPtr start;
				if (TokenType == ATT_COLON)
					start = ::aul::ast::IntLit::New(TokenSPos, 0); // slice with first index missing -> implicit start index zero
				else
					start = Parse_Expression();

				if (TokenType == ATT_BCLOSE2)
				{
					expr = ::aul::ast::SubscriptExpr::New(NodeStart, std::move(expr), std::move(start));
				}
				else if (TokenType == ATT_COLON)
				{
					Shift();
					::aul::ast::ExprPtr end;
					if (TokenType == ATT_BCLOSE2)
					{
						end = ::aul::ast::IntLit::New(TokenSPos, std::numeric_limits<int32_t>::max());
					}
					else
					{
						end = Parse_Expression();
					}
					expr = ::aul::ast::SliceExpr::New(NodeStart, std::move(expr), std::move(start), std::move(end));
				}
				else
				{
					UnexpectedToken("']' or ':'");
				}
				Match(ATT_BCLOSE2);
				break;
			}
		case ATT_DOT:
			Shift();
			Check(ATT_IDTF, "property name");
			expr = ::aul::ast::SubscriptExpr::New(NodeStart, std::move(expr), ::aul::ast::StringLit::New(TokenSPos, Idtf));
			Shift();
			break;
		case ATT_CALL: case ATT_CALLFS:
			{
				auto call = ::aul::ast::CallExpr::New(NodeStart);
				call->context = std::move(expr);
				call->safe_call = TokenType == ATT_CALLFS;
				Shift();
				Check(ATT_IDTF, "function name after '->'");
				call->callee = Idtf;
				Shift();
				Parse_CallParams(call.get());
				expr = std::move(call);
				break;
			}
		default:
			return expr;
		}
	}
}

std::unique_ptr<::aul::ast::VarDecl> C4AulParse::Parse_Var()
{
	auto decl = ::aul::ast::VarDecl::New(TokenSPos);
	if (SEqual(Idtf, C4AUL_VarNamed))
	{
		decl->scope = ::aul::ast::VarDecl::Scope::Func;
	}
	else if (SEqual(Idtf, C4AUL_LocalNamed))
	{
		decl->scope = ::aul::ast::VarDecl::Scope::Object;
	}
	else if (SEqual(Idtf, C4AUL_GlobalNamed))
	{
		decl->scope = ::aul::ast::VarDecl::Scope::Global;
	}
	else
	{
		assert(0 && "C4AulParse::Parse_Var called with invalid parse state (current token should be scope of variable)");
		// Uh this shouldn't happen, ever
		Error("internal error: C4AulParse::Parse_Var called with invalid parse state (current token should be scope of variable, but is '%s')", Idtf);
	}
	Shift();
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Const))
	{
		decl->constant = true;
		Shift();
	}
	while (true)
	{
		// get desired variable name
		Check(ATT_IDTF, "variable name");
		std::string identifier = Idtf;
		Shift();
		::aul::ast::ExprPtr init;
		if (TokenType == ATT_SET)
		{
			Shift();
			init = Parse_Expression();
		}
		decl->decls.push_back({ identifier, std::move(init) });
		if (TokenType == ATT_SCOLON)
			return decl;
		Match(ATT_COMMA, "',' or ';'");
	}
}

void C4ScriptHost::CopyPropList(C4Set<C4Property> & from, C4PropListStatic * to)
{
	// append all funcs and local variable initializations
	const C4Property * prop = from.First();
	while (prop)
	{
		switch(prop->Value.GetType())
		{
		case C4V_Function:
			{
				C4AulScriptFunc * sf = prop->Value.getFunction()->SFunc();
				if (sf)
				{
					C4AulScriptFunc *sfc;
					if (sf->pOrgScript != this)
						sfc = new C4AulScriptFunc(to, *sf);
					else
						sfc = sf;
					sfc->SetOverloaded(to->GetFunc(sf->Name));
					to->SetPropertyByS(prop->Key, C4VFunction(sfc));
				}
				else
				{
					// engine function
					to->SetPropertyByS(prop->Key, prop->Value);
				}
			}
			break;
		case C4V_PropList:
			{
				C4PropListStatic * p = prop->Value._getPropList()->IsStatic();
				assert(p);
				if (prop->Key != &::Strings.P[P_Prototype])
					if (!p || p->GetParent() != to)
					{
						p = C4PropList::NewStatic(nullptr, to, prop->Key);
						CopyPropList(prop->Value._getPropList()->Properties, p);
					}
				to->SetPropertyByS(prop->Key, C4VPropList(p));
			}
			break;
		case C4V_Array: // FIXME: copy the array if necessary
		default:
			to->SetPropertyByS(prop->Key, prop->Value);
		}
		prop = from.Next(prop);
	}
}

bool C4ScriptHost::Parse()
{
	// check state
	if (State != ASS_LINKED) return false;

	if (!Appends.empty())
	{
		// #appendto scripts are not allowed to contain global functions or belong to definitions
		// so their contents are not reachable
		return true;
	}

	C4PropListStatic * p = GetPropList();

	for (auto & SourceScript : SourceScripts)
	{
		CopyPropList(SourceScript->LocalValues, p);
		if (SourceScript == this)
			continue;
		// definition appends
		if (GetPropList() && GetPropList()->GetDef() && SourceScript->GetPropList() && SourceScript->GetPropList()->GetDef())
			GetPropList()->GetDef()->IncludeDefinition(SourceScript->GetPropList()->GetDef());
	}

	// generate bytecode
	for (auto &s : SourceScripts)
		C4AulCompiler::Compile(this, s, s->ast.get());

	// save line count
	Engine->lineCnt += SGetLine(Script.getData(), Script.getPtr(Script.getLength()));

	// finished
	State = ASS_PARSED;

	return true;
}

void C4ScriptHost::EnableWarning(const char *pos, C4AulWarningId warning, bool enable)
{
	auto entry = enabledWarnings.emplace(pos, decltype(enabledWarnings)::mapped_type{});
	if (entry.second)
	{
		// If there was no earlier entry for this position, copy the previous
		// warning state
		assert(entry.first != enabledWarnings.begin());
		auto previous = entry.first;
		--previous;
		entry.first->second = previous->second;
	}
	entry.first->second.set(static_cast<size_t>(warning), enable);
}

bool C4ScriptHost::IsWarningEnabled(const char *pos, C4AulWarningId warning) const
{
	assert(!enabledWarnings.empty());
	if (enabledWarnings.empty())
		return false;
	
	// find nearest set of warnings at or before the current position
	auto entry = enabledWarnings.upper_bound(pos);
	assert(entry != enabledWarnings.begin());
	if (entry != enabledWarnings.begin())
	{
		--entry;
	}

	return entry->second.test(static_cast<size_t>(warning));
}

void C4AulParse::PushParsePos()
{
	parse_pos_stack.push(TokenSPos);
}

void C4AulParse::PopParsePos()
{
	assert(!parse_pos_stack.empty());
	SPos = parse_pos_stack.top();
	DiscardParsePos();
	Shift();
}

void C4AulParse::DiscardParsePos()
{
	assert(!parse_pos_stack.empty());
	parse_pos_stack.pop();
}
