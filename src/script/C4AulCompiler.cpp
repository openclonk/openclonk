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

#include "C4Include.h"
#include "script/C4AulCompiler.h"

#include <inttypes.h>

#include "script/C4Aul.h"
#include "script/C4AulParse.h"
#include "script/C4AulScriptFunc.h"
#include "script/C4ScriptHost.h"

#define C4AUL_Inherited     "inherited"
#define C4AUL_SafeInherited "_inherited"
#define C4AUL_DebugBreak    "__debugbreak"

#undef NDEBUG
#include <assert.h>

static std::string vstrprintf(const char *format, va_list args)
{
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, format, argcopy);
	if (size < 0)
		throw std::invalid_argument("invalid argument to strprintf");
	va_end(argcopy);
	std::string s;
	s.resize(size + 1);
	size = vsnprintf(&s[0], s.size(), format, args);
	assert(size >= 0);
	s.resize(size);
	return s;
}

static std::string strprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	std::string s = vstrprintf(format, args);
	va_end(args);
	return s;
}

static std::string FormatCodePosition(const C4ScriptHost *source_host, const char *pos, const C4ScriptHost *target_host = nullptr, const C4AulScriptFunc *func = nullptr)
{
	std::string s;
	if (func && func->GetFullName())
	{
		s += strprintf(" (in %s", func->GetFullName().getData());
		if (source_host && pos)
			s += ", ";
		else
			s += ")";
	}
	if (source_host && pos)
	{
		if (!func || !func->GetFullName())
			s += " (";

		int line = SGetLine(source_host->GetScript(), pos);
		int col = SLineGetCharacters(source_host->GetScript(), pos);

		s += strprintf("%s:%d:%d)",
			source_host->GetFilePath(),
			line, col
		);
	}
	if (target_host && source_host != target_host)
	{
		s += strprintf(" (as #appendto/#include to %s)", target_host->ScriptName.getData());
	}
	return s;
}

template<class... T>
static void Warn(const C4ScriptHost *target_host, const C4ScriptHost *host, const char *SPos, const C4AulScriptFunc *func, const char *msg, T &&...args)
{
	std::string message = sizeof...(T) > 0 ? strprintf(msg, std::forward<T>(args)...) : msg;
	message += FormatCodePosition(host, SPos, target_host, func);

	++::ScriptEngine.warnCnt;
	::ScriptEngine.GetErrorHandler()->OnWarning(message.c_str());
}

template<class... T>
static void Warn(const C4ScriptHost *target_host, const C4ScriptHost *host, const ::aul::ast::Node *n, const C4AulScriptFunc *func, const char *msg, T &&...args)
{
	return Warn(target_host, host, n->loc, func, msg, std::forward<T>(args)...);
}
template<class... T>
static void Warn(const C4ScriptHost *target_host, const C4ScriptHost *host, const std::nullptr_t &, const C4AulScriptFunc *func, const char *msg, T &&...args)
{
	return Warn(target_host, host, static_cast<const char*>(nullptr), func, msg, std::forward<T>(args)...);
}

template<class... T>
static C4AulParseError Error(const C4ScriptHost *target_host, const C4ScriptHost *host, const char *SPos, const C4AulScriptFunc *func, const char *msg, T &&...args)
{
	std::string message = sizeof...(T) > 0 ? strprintf(msg, std::forward<T>(args)...) : msg;

	message += FormatCodePosition(host, SPos, target_host, func);
	return C4AulParseError(static_cast<C4ScriptHost*>(nullptr), message.c_str());
}

template<class... T>
static C4AulParseError Error(const C4ScriptHost *target_host, const C4ScriptHost *host, const ::aul::ast::Node *n, const C4AulScriptFunc *func, const char *msg, T &&...args)
{
	return Error(target_host, host, n->loc, func, msg, std::forward<T>(args)...);
}
template<class... T>
static C4AulParseError Error(const C4ScriptHost *target_host, const C4ScriptHost *host, const std::nullptr_t &, const C4AulScriptFunc *func, const char *msg, T &&...args)
{
	return Error(target_host, host, static_cast<const char*>(nullptr), func, msg, std::forward<T>(args)...);
}

class C4AulCompiler::PreparseAstVisitor : public ::aul::DefaultRecursiveVisitor
{
	// target_host: The C4ScriptHost on which compilation is done
	C4ScriptHost *target_host = nullptr;
	// host: The C4ScriptHost where the script actually resides in
	C4ScriptHost *host = nullptr;
	// Fn: The C4AulScriptFunc that is currently getting parsed
	C4AulScriptFunc *Fn = nullptr;

public:
	PreparseAstVisitor(C4ScriptHost *host, C4ScriptHost *source_host, C4AulScriptFunc *func = nullptr) : target_host(host), host(source_host), Fn(func) {}
	explicit PreparseAstVisitor(C4AulScriptFunc *func) : Fn(func), target_host(func->pOrgScript), host(target_host) {}

	virtual ~PreparseAstVisitor() {}

	using DefaultRecursiveVisitor::visit;
	virtual void visit(const ::aul::ast::RangeLoop *n) override;
	virtual void visit(const ::aul::ast::VarDecl *n) override;
	virtual void visit(const ::aul::ast::FunctionDecl *n) override;
	virtual void visit(const ::aul::ast::CallExpr *n) override;
	virtual void visit(const ::aul::ast::ParExpr *n) override;
	virtual void visit(const ::aul::ast::AppendtoPragma *n) override;
	virtual void visit(const ::aul::ast::IncludePragma *n) override;
};

class C4AulCompiler::CodegenAstVisitor : public ::aul::DefaultRecursiveVisitor
{
	C4AulScriptFunc *Fn = nullptr;
	// target_host: The C4ScriptHost on which compilation is done
	C4ScriptHost *target_host = nullptr;
	// host: The C4ScriptHost where the script actually resides in
	C4ScriptHost *host = nullptr;

	int32_t stack_height = 0;
	bool at_jump_target = false;

	struct Loop
	{
		explicit Loop(int stack_height) : stack_height(stack_height) {}

		int stack_height = 0;
		std::vector<int> continues;
		std::vector<int> breaks;

		enum class Control
		{
			Continue,
			Break
		};
	};

	std::stack<Loop> active_loops;

	// The type of the variable on top of the value stack. C4V_Any if unknown.
	C4V_Type type_of_stack_top = C4V_Any;
	
	constexpr static bool IsJump(C4AulBCCType t)
	{
		return t == AB_JUMP || t == AB_JUMPAND || t == AB_JUMPOR || t == AB_JUMPNNIL || t == AB_CONDN || t == AB_COND;
	}

	int AddJumpTarget();
	void AddJumpTo(const char *loc, C4AulBCCType type, int target);
	void UpdateJump(int jump, int target);
	void PushLoop();
	void PopLoop(int continue_target);
	void AddLoopControl(const char *loc, Loop::Control c);

	int AddVarAccess(const char *TokenSPos, C4AulBCCType eType, intptr_t varnum);
	int AddBCC(const char *TokenSPos, C4AulBCCType eType, intptr_t X = 0);
	int AddBCC(const char *SPos, const C4AulBCC &bcc);

	template<class T>
	void MaybePopValueOf(const std::unique_ptr<T> &n)
	{
		if (!n) return;
		if (!n->has_value()) return;
		AddBCC(n->loc, AB_STACK, -1);
	}

	static int GetStackValue(C4AulBCCType eType, intptr_t X);
	void RemoveLastBCC();
	C4AulBCC MakeSetter(const char *SPos, bool fLeaveValue);

public:
	CodegenAstVisitor(C4ScriptHost *host, C4ScriptHost *source_host) : target_host(host), host(source_host) {}
	explicit CodegenAstVisitor(C4AulScriptFunc *func) : Fn(func), target_host(func->pOrgScript), host(target_host) {}

	virtual ~CodegenAstVisitor() {}

	using DefaultRecursiveVisitor::visit;
	virtual void visit(const ::aul::ast::Noop *) override;
	virtual void visit(const ::aul::ast::StringLit *n) override;
	virtual void visit(const ::aul::ast::IntLit *n) override;
	virtual void visit(const ::aul::ast::BoolLit *n) override;
	virtual void visit(const ::aul::ast::ArrayLit *n) override;
	virtual void visit(const ::aul::ast::ProplistLit *n) override;
	virtual void visit(const ::aul::ast::NilLit *n) override;
	virtual void visit(const ::aul::ast::ThisLit *n) override;
	virtual void visit(const ::aul::ast::VarExpr *n) override;
	virtual void visit(const ::aul::ast::UnOpExpr *n) override;
	virtual void visit(const ::aul::ast::BinOpExpr *n) override;
	virtual void visit(const ::aul::ast::AssignmentExpr *n) override;
	virtual void visit(const ::aul::ast::SubscriptExpr *n) override;
	virtual void visit(const ::aul::ast::SliceExpr *n) override;
	virtual void visit(const ::aul::ast::CallExpr *n) override;
	virtual void visit(const ::aul::ast::ParExpr *n) override;
	virtual void visit(const ::aul::ast::Block *n) override;
	virtual void visit(const ::aul::ast::Return *n) override;
	virtual void visit(const ::aul::ast::ForLoop *n) override;
	virtual void visit(const ::aul::ast::RangeLoop *n) override;
	virtual void visit(const ::aul::ast::DoLoop *n) override;
	virtual void visit(const ::aul::ast::WhileLoop *n) override;
	virtual void visit(const ::aul::ast::Break *n) override;
	virtual void visit(const ::aul::ast::Continue *n) override;
	virtual void visit(const ::aul::ast::If *n) override;
	virtual void visit(const ::aul::ast::VarDecl *n) override;
	virtual void visit(const ::aul::ast::FunctionDecl *n) override;
	virtual void visit(const ::aul::ast::FunctionExpr *n) override;

	template<class T>
	void EmitFunctionCode(const T *n)
	{
		// This dynamic_cast resolves the problem where we have a Function*
		// and want to emit code to it. All classes derived from Function
		// are also ultimately derived from Node, so this call is fine
		// without any additional checking.
		EmitFunctionCode(n, dynamic_cast<const ::aul::ast::Node*>(n));
	}

private:
	void EmitFunctionCode(const ::aul::ast::Function *f, const ::aul::ast::Node *n);
};

class C4AulCompiler::ConstexprEvaluator : public ::aul::AstVisitor
{
public:
	enum EvalFlag
	{
		// If this flag is set, ConstexprEvaluator will assume unset values
		// are nil. If it is not set, evaluation of unset values will throw
		// ExpressionNotConstant.
		IgnoreUnset = 1
	};
	typedef int EvalFlags;

	// Evaluates constant AST subtrees and returns the final C4Value.
	// Throws ExpressionNotConstant if evaluation fails.
	static C4Value eval(C4ScriptHost *host, const ::aul::ast::Expr *e, EvalFlags flags = 0);
	static C4Value eval_static(C4ScriptHost *host, C4PropListStatic *parent, const std::string &parent_key, const ::aul::ast::Expr *e, EvalFlags flags = 0);

private:
	C4ScriptHost *host = nullptr;
	C4Value v;
	bool ignore_unset_values = false;

	struct ProplistMagic
	{
		bool active = false;
		C4PropListStatic *parent = nullptr;
		std::string key;

		ProplistMagic() = default;
		ProplistMagic(bool active, C4PropListStatic *parent, const std::string &key) : active(active), parent(parent), key(key) {}
	} proplist_magic;

	explicit ConstexprEvaluator(C4ScriptHost *host) : host(host) {}

	NORETURN void nonconst(const ::aul::ast::Node *n) const
	{
		throw ExpressionNotConstant(host, n, nullptr, nullptr);
	}

	void AssertValueType(const C4Value &v, C4V_Type Type1, const char *opname, const ::aul::ast::Node *n)
	{
		// Typecheck parameter
		if (!v.CheckParConversion(Type1))
			throw Error(host, host, n, nullptr, "operator \"%s\": got %s, but expected %s", opname, v.GetTypeName(), GetC4VName(Type1));
	}
public:
	class ExpressionNotConstant : public C4AulParseError
	{
	public:
		ExpressionNotConstant(const C4ScriptHost *host, const ::aul::ast::Node *n, C4AulScriptFunc *Fn, const char *expr) :
			C4AulParseError(Error(host, host, n, Fn, "expression not constant: %s", expr)) {}
	};

	using AstVisitor::visit;
	virtual void visit(const ::aul::ast::StringLit *n) override;
	virtual void visit(const ::aul::ast::IntLit *n) override;
	virtual void visit(const ::aul::ast::BoolLit *n) override;
	virtual void visit(const ::aul::ast::ArrayLit *n) override;
	virtual void visit(const ::aul::ast::ProplistLit *n) override;
	virtual void visit(const ::aul::ast::NilLit *) override;
	virtual void visit(const ::aul::ast::ThisLit *n) override;
	virtual void visit(const ::aul::ast::VarExpr *n) override;
	virtual void visit(const ::aul::ast::UnOpExpr *n) override;
	virtual void visit(const ::aul::ast::BinOpExpr *n) override;
	virtual void visit(const ::aul::ast::AssignmentExpr *n) override;
	virtual void visit(const ::aul::ast::SubscriptExpr *n) override;
	virtual void visit(const ::aul::ast::SliceExpr *n) override;
	virtual void visit(const ::aul::ast::CallExpr *n) override;
	virtual void visit(const ::aul::ast::FunctionExpr *n) override;
};

class C4AulCompiler::ConstantResolver : public ::aul::DefaultRecursiveVisitor
{
	C4ScriptHost *host;
	explicit ConstantResolver(C4ScriptHost *host) : host(host) {}

public:
	static void resolve(C4ScriptHost *host, const ::aul::ast::Script *script)
	{
		// We resolve constants *twice*; this allows people to create circular
		// references in proplists or arrays.
		// Unfortunately it also results in unexpected behaviour in code like
		// this:
		//     static const c1 = c2, c2 = c3, c3 = 1;
		// which will set c1 to nil, and both c2 and c3 to 1.
		// While this is unlikely to happen often, we should fix that so it
		// resolves all three constants to 1.
		ConstantResolver r(host);
		r.visit(script);
	}
	virtual ~ConstantResolver() {}

	using DefaultRecursiveVisitor::visit;
	void visit(const ::aul::ast::VarDecl *n) override;
};

void C4AulCompiler::Preparse(C4ScriptHost *host, C4ScriptHost *source_host, const ::aul::ast::Script *script)
{
	PreparseAstVisitor v(host, source_host);
	v.visit(script);

	ConstantResolver::resolve(host, script);
}

void C4AulCompiler::Compile(C4ScriptHost *host, C4ScriptHost *source_host, const ::aul::ast::Script *script)
{
	ConstantResolver::resolve(host, script);

	CodegenAstVisitor v(host, source_host);
	v.visit(script);
}

void C4AulCompiler::Compile(C4AulScriptFunc *func, const ::aul::ast::Function *def)
{
	{
		// Don't visit the whole definition here; that would create a new function
		// and we don't want that.
		PreparseAstVisitor v(func);
		def->body->accept(&v);
	}
	{
		CodegenAstVisitor v(func);
		v.EmitFunctionCode(def);
	}
}

#define ENSURE_COND(cond, failmsg) do { if (!(cond)) throw Error(target_host, host, n, Fn, failmsg); } while (0)

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::RangeLoop *n)
{
	const char *cname = n->var.c_str();
	if (n->scoped_var)
	{
		Fn->VarNamed.AddName(cname);
	}
	else
	{
		// Loop variable not explicitly declared here. Look it up in
		// the function and warn if it hasn't been declared at all.
		if (Fn->VarNamed.GetItemNr(cname) == -1)
		{
			Warn(target_host, host, n, Fn, "Implicit declaration of the loop variable in a for-in loop is deprecated: %s", cname);
			Fn->VarNamed.AddName(cname);
		}
	}
	DefaultRecursiveVisitor::visit(n);
}

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::VarDecl *n)
{
	if (n->constant && n->scope != ::aul::ast::VarDecl::Scope::Global)
	{
		Warn(target_host, host, n, Fn, "Non-global variables cannot be constant");
	}
	for (const auto &var : n->decls)
	{
		const char *cname = var.name.c_str();
		switch (n->scope)
		{
		case ::aul::ast::VarDecl::Scope::Func:
			{
				assert(Fn && "function-local var declaration outside of function");
				if (!Fn)
					throw Error(target_host, host, n, Fn, "internal error: function-local var declaration outside of function");

				if (target_host)
				{
					// if target_host is unset, we're parsing this func for direct execution,
					// in which case we don't want to warn about variable hiding.
					if (target_host->Engine->GlobalNamedNames.GetItemNr(cname) >= 0 || target_host->Engine->GlobalConstNames.GetItemNr(cname) >= 0)
						Warn(target_host, host, n, Fn, "function-local variable hides a global variable: %s", cname);
					C4String *s = ::Strings.FindString(cname);
					if (s && target_host->GetPropList()->HasProperty(s))
						Warn(target_host, host, n, Fn, "function-local variable hides an object-local variable: %s", cname);
				}
				Fn->VarNamed.AddName(cname);
				break;
			}
		case ::aul::ast::VarDecl::Scope::Object:
			{
				if (host->Engine->GlobalNamedNames.GetItemNr(cname) >= 0 || host->Engine->GlobalConstNames.GetItemNr(cname) >= 0)
					Warn(target_host, host, n, Fn, "object-local variable hides a global variable: %s", cname);
				C4String *s = ::Strings.RegString(cname);
				if (target_host->GetPropList()->HasProperty(s))
					Warn(target_host, host, n, Fn, "object-local variable declared multiple times: %s", cname);
				else
					target_host->GetPropList()->SetPropertyByS(s, C4VNull);
				break;
			}
		case ::aul::ast::VarDecl::Scope::Global:
			assert(!Fn && "global var declaration inside function");
			if (Fn)
				throw Error(target_host, host, n, Fn, "internal error: global var declaration inside function");

			if (host->Engine->GlobalNamedNames.GetItemNr(cname) >= 0 || host->Engine->GlobalConstNames.GetItemNr(cname) >= 0)
				Warn(target_host, host, n, Fn, "global variable declared multiple times: %s", cname);
			if (n->constant)
				host->Engine->GlobalConstNames.AddName(cname);
			else
				host->Engine->GlobalNamedNames.AddName(cname);
			break;
		}
	}

	if (n->scope == ::aul::ast::VarDecl::Scope::Func)
	{
		// only func-scoped variables can potentially have initializers we care
		// about in the pre-parsing stage: they may have calls that pass
		// unnamed parameters
		DefaultRecursiveVisitor::visit(n);
	}
}

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::FunctionDecl *n)
{
	// create script fn
	C4PropListStatic *Parent = n->is_global ? target_host->Engine->GetPropList() : target_host->GetPropList();
	const char *cname = n->name.c_str();

	assert(!Fn);

	// Look up the overloaded function before adding the overloading one
	C4AulFunc *parent_func = Parent->GetFunc(cname);

	Fn = new C4AulScriptFunc(Parent, target_host, cname, n->loc);
	host->ownedFunctions.push_back(C4VFunction(Fn));
	for (const auto &param : n->params)
	{
		Fn->AddPar(param.name.c_str(), param.type);
	}
	if (n->has_unnamed_params)
		Fn->ParCount = C4AUL_MAX_Par;

	// Add function to def/engine
	Fn->SetOverloaded(parent_func);
	Parent->SetPropertyByS(Fn->Name, C4VFunction(Fn));

	DefaultRecursiveVisitor::visit(n);

	Fn = nullptr;
}

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::CallExpr *n)
{
	if (n->append_unnamed_pars && Fn->ParCount != C4AUL_MAX_Par)
	{
		Fn->ParCount = C4AUL_MAX_Par;
	}
	DefaultRecursiveVisitor::visit(n);
}

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::ParExpr *n)
{
	if (Fn->ParCount != C4AUL_MAX_Par)
	{
		Warn(target_host, host, n, Fn, "using Par() inside a function forces it to take variable arguments");
		Fn->ParCount = C4AUL_MAX_Par;
	}
	DefaultRecursiveVisitor::visit(n);
}

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::AppendtoPragma *n)
{
	if (n->what.empty())
		host->Appends.emplace_back("*");
	else
		host->Appends.emplace_back(n->what.c_str());
}

void C4AulCompiler::PreparseAstVisitor::visit(const ::aul::ast::IncludePragma *n)
{
	host->Includes.emplace_back(n->what.c_str());
}

int C4AulCompiler::CodegenAstVisitor::GetStackValue(C4AulBCCType eType, intptr_t X)
{
	switch (eType)
	{
	case AB_INT:
	case AB_BOOL:
	case AB_STRING:
	case AB_CPROPLIST:
	case AB_CARRAY:
	case AB_CFUNCTION:
	case AB_NIL:
	case AB_LOCALN:
	case AB_GLOBALN:
	case AB_DUP:
	case AB_DUP_CONTEXT:
	case AB_THIS:
		return 1;

	case AB_Pow:
	case AB_Div:
	case AB_Mul:
	case AB_Mod:
	case AB_Sub:
	case AB_Sum:
	case AB_LeftShift:
	case AB_RightShift:
	case AB_LessThan:
	case AB_LessThanEqual:
	case AB_GreaterThan:
	case AB_GreaterThanEqual:
	case AB_Equal:
	case AB_NotEqual:
	case AB_BitAnd:
	case AB_BitXOr:
	case AB_BitOr:
	case AB_PROP_SET:
	case AB_ARRAYA:
	case AB_CONDN:
	case AB_COND:
	case AB_POP_TO:
	case AB_RETURN:
		// JUMPAND/JUMPOR/JUMPNNIL are special: They either jump over instructions adding one to the stack
		// or decrement the stack. Thus, for stack counting purposes, they decrement.
	case AB_JUMPAND:
	case AB_JUMPOR:
	case AB_JUMPNNIL:
		return -1;

	case AB_FUNC:
		return -reinterpret_cast<C4AulFunc *>(X)->GetParCount() + 1;

	case AB_CALL:
	case AB_CALLFS:
		return -C4AUL_MAX_Par;

	case AB_STACK_SET:
	case AB_LOCALN_SET:
	case AB_PROP:
	case AB_GLOBALN_SET:
	case AB_Inc:
	case AB_Dec:
	case AB_BitNot:
	case AB_Not:
	case AB_Neg:
	case AB_PAR:
	case AB_FOREACH_NEXT:
	case AB_ERR:
	case AB_EOFN:
	case AB_JUMP:
	case AB_DEBUG:
		return 0;

	case AB_STACK:
		return X;

	case AB_NEW_ARRAY:
		return -X + 1;

	case AB_NEW_PROPLIST:
		return -X * 2 + 1;

	case AB_ARRAYA_SET:
	case AB_ARRAY_SLICE:
		return -2;

	case AB_ARRAY_SLICE_SET:
		return -3;
	}
	assert(0 && "GetStackValue: unexpected bytecode not handled");
	return 0;
}

int C4AulCompiler::CodegenAstVisitor::AddVarAccess(const char *TokenSPos, C4AulBCCType eType, intptr_t varnum)
{
	return AddBCC(TokenSPos, eType, 1 + varnum - (stack_height + Fn->VarNamed.iSize));
}

int C4AulCompiler::CodegenAstVisitor::AddBCC(const char *TokenSPos, C4AulBCCType eType, intptr_t X)
{
	// Track stack size
	stack_height += GetStackValue(eType, X);

	// Use stack operation instead of 0-Any (enable optimization)
	if (eType == AB_NIL)
	{
		eType = AB_STACK;
		X = 1;
	}

	assert(eType != AB_STACK || X != 0);

	// Join checks only if it's not a jump target
	if (!at_jump_target && Fn->GetLastCode())
	{
		C4AulBCC *pCPos1 = Fn->GetLastCode();

		// Skip noop stack operation
		if (eType == AB_STACK && X == 0)
		{
			return Fn->GetCodePos() - 1;
		}

		// Join together stack operations
		if (eType == AB_STACK && pCPos1->bccType == AB_STACK &&
			(X <= 0 || pCPos1->Par.i >= 0))
		{
			pCPos1->Par.i += X;
			// Empty? Remove it. This relies on the parser not issuing
			// multiple negative stack operations consecutively, as
			// that could result in removing a jump target bytecode.
			if (!pCPos1->Par.i)
				Fn->RemoveLastBCC();
			return Fn->GetCodePos() - 1;
		}

		// Prune unneeded Incs / Decs
		if (eType == AB_STACK && X < 0 && (pCPos1->bccType == AB_Inc || pCPos1->bccType == AB_Dec))
		{
			if (!pCPos1->Par.X)
			{
				pCPos1->bccType = eType;
				pCPos1->Par.i = X;
				return Fn->GetCodePos() - 1;
			}
			else
			{
				// If it was a result modifier, we can safely remove it knowing that it was neither
				// the first chunk nor a jump target. We can therefore apply additional optimizations.
				Fn->RemoveLastBCC();
				pCPos1--;
			}
		}

		// Join STACK_SET + STACK -1 to POP_TO (equivalent)
		if (eType == AB_STACK && X == -1 && pCPos1->bccType == AB_STACK_SET)
		{
			pCPos1->bccType = AB_POP_TO;
			return Fn->GetCodePos() - 1;
		}

		// Join POP_TO + DUP to AB_STACK_SET if both target the same slot
		if (eType == AB_DUP && pCPos1->bccType == AB_POP_TO && X == pCPos1->Par.i + 1)
		{
			pCPos1->bccType = AB_STACK_SET;
			return Fn->GetCodePos() - 1;
		}

		// Reduce some constructs like SUM + INT 1 to INC or DEC
		if ((eType == AB_Sum || eType == AB_Sub) &&
			pCPos1->bccType == AB_INT &&
			(pCPos1->Par.i == 1 || pCPos1->Par.i == -1))
		{
			if ((pCPos1->Par.i > 0) == (eType == AB_Sum))
				pCPos1->bccType = AB_Inc;
			else
				pCPos1->bccType = AB_Dec;
			pCPos1->Par.i = X;
			return Fn->GetCodePos() - 1;
		}

		// Reduce Not + CONDN to COND, Not + COND to CONDN
		if ((eType == AB_CONDN || eType == AB_COND) && pCPos1->bccType == AB_Not)
		{
			pCPos1->bccType = eType == AB_CONDN ? AB_COND : AB_CONDN;
			pCPos1->Par.i = X + 1;
			return Fn->GetCodePos() - 1;
		}

		// Join AB_STRING + AB_ARRAYA to AB_PROP
		if (eType == AB_ARRAYA && pCPos1->bccType == AB_STRING)
		{
			pCPos1->bccType = AB_PROP;
			return Fn->GetCodePos() - 1;
		}

		// Join AB_INT + AB_Neg to AB_INT
		if (eType == AB_Neg && pCPos1->bccType == AB_INT)
		{
			pCPos1->Par.i *= -1;
			return Fn->GetCodePos() - 1;
		}
	}

	// Add
	Fn->AddBCC(eType, X, TokenSPos);

	// Reset jump flag
	at_jump_target = false;

	return Fn->GetCodePos() - 1;
}

void C4AulCompiler::CodegenAstVisitor::RemoveLastBCC()
{
	// Security: This is unsafe on anything that might get optimized away
	C4AulBCC *pBCC = Fn->GetLastCode();
	assert(pBCC->bccType != AB_STACK && pBCC->bccType != AB_STACK_SET && pBCC->bccType != AB_POP_TO);
	// Correct stack
	stack_height -= GetStackValue(pBCC->bccType, pBCC->Par.X);
	// Remove
	Fn->RemoveLastBCC();
}

int C4AulCompiler::CodegenAstVisitor::AddBCC(const char *SPos, const C4AulBCC &bcc)
{
	return AddBCC(SPos, bcc.bccType, bcc.Par.X);
}

C4AulBCC C4AulCompiler::CodegenAstVisitor::MakeSetter(const char *SPos, bool fLeaveValue)
{
	assert(Fn);
	C4AulBCC Value = *(Fn->GetLastCode()), Setter = Value;
	// Check type
	switch (Value.bccType)
	{
	case AB_ARRAYA: Setter.bccType = AB_ARRAYA_SET; break;
	case AB_ARRAY_SLICE: Setter.bccType = AB_ARRAY_SLICE_SET; break;
	case AB_DUP:
		Setter.bccType = AB_STACK_SET;
		// the setter additionally has the new value on the stack
		--Setter.Par.i;
		break;
	case AB_STACK_SET: Setter.bccType = AB_STACK_SET; break;
	case AB_LOCALN:
		Setter.bccType = AB_LOCALN_SET;
		break;
	case AB_PROP:
		Setter.bccType = AB_PROP_SET;
		break;
	case AB_GLOBALN: Setter.bccType = AB_GLOBALN_SET; break;
	default:
		throw Error(target_host, host, SPos, Fn, "assignment to a constant");
	}
	// If the new value is produced using the old one, the parameters to get the old one need to be duplicated.
	// Otherwise, the setter can just use the parameters originally meant for the getter.
	// All getters push one value, so the parameter count is one more than the values they pop from the stack.
	int iParCount = 1 - GetStackValue(Value.bccType, Value.Par.X);
	if (Value.bccType == AB_STACK_SET)
	{
		// STACK_SET has a side effect, so it can't be simply removed.
		// Discard the unused value the usual way instead.
		if (!fLeaveValue)
			AddBCC(SPos, AB_STACK, -1);
		// The original parameter isn't needed anymore, since in contrast to the other getters
		// it does not indicate a position.
		iParCount = 0;
	}
	else if (!fLeaveValue || iParCount)
	{
		RemoveLastBCC();
		at_jump_target = true; // In case the original BCC was a jump target
	}
	if (fLeaveValue && iParCount)
	{
		for (int i = 0; i < iParCount; i++)
			AddBCC(SPos, AB_DUP, 1 - iParCount);
		// Finally re-add original BCC
		AddBCC(SPos, Value.bccType, Value.Par.X);
	}
	// Done. The returned BCC should be added later once the value to be set was pushed on top.
	assert(iParCount == -GetStackValue(Setter.bccType, Setter.Par.X));
	return Setter;
}

int C4AulCompiler::CodegenAstVisitor::AddJumpTarget()
{
	assert(Fn && "Jump target outside of function");
	if (!Fn)
		throw C4AulParseError(host, "internal error: jump target outside of function");

	at_jump_target = true;
	return Fn->GetCodePos();
}

void C4AulCompiler::CodegenAstVisitor::UpdateJump(int jump, int target)
{
	C4AulBCC *code = Fn->GetCodeByPos(jump);
	assert(IsJump(code->bccType));
	code->Par.i = target - jump;
}

void C4AulCompiler::CodegenAstVisitor::AddJumpTo(const char *loc, C4AulBCCType type, int target)
{
	AddBCC(loc, type, target - Fn->GetCodePos());
}

void C4AulCompiler::CodegenAstVisitor::PushLoop()
{
	active_loops.emplace(stack_height);
}

void C4AulCompiler::CodegenAstVisitor::PopLoop(int continue_target)
{
	assert(!active_loops.empty());
	assert(stack_height == active_loops.top().stack_height);
	// Update all loop control jumps
	const auto &loop = active_loops.top();
	for (auto &c : loop.continues)
		UpdateJump(c, continue_target);
	int loop_exit = AddJumpTarget();
	for (auto &b : loop.breaks)
		UpdateJump(b, loop_exit);
	active_loops.pop();
}

void C4AulCompiler::CodegenAstVisitor::AddLoopControl(const char *loc, Loop::Control c)
{
	assert(!active_loops.empty());
	if (active_loops.empty())
		throw C4AulParseError(host, "internal error: loop control code emitted outside of loop");
	// Clear stack
	assert(active_loops.top().stack_height == stack_height);
	if (active_loops.top().stack_height - stack_height > 0)
		AddBCC(loc, AB_STACK, active_loops.top().stack_height - stack_height);
	int jump = AddBCC(loc, AB_JUMP, 0);
	switch (c)
	{
	case Loop::Control::Continue:
		active_loops.top().continues.push_back(jump);
		break;
	case Loop::Control::Break:
		active_loops.top().breaks.push_back(jump);
		break;
	}
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::Noop *) {}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::StringLit *n)
{
	AddBCC(n->loc, AB_STRING, (intptr_t)::Strings.RegString(n->value.c_str()));
	type_of_stack_top = C4V_String;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::IntLit *n)
{
	AddBCC(n->loc, AB_INT, n->value);
	type_of_stack_top = C4V_Int;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::BoolLit *n)
{
	AddBCC(n->loc, AB_BOOL, n->value);
	type_of_stack_top = C4V_Bool;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::ArrayLit *n)
{
	for (const auto &e : n->values)
	{
		e->accept(this);
	}
	AddBCC(n->loc, AB_NEW_ARRAY, n->values.size());
	type_of_stack_top = C4V_Array;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::ProplistLit *n)
{
	for (const auto &e : n->values)
	{
		AddBCC(n->loc, AB_STRING, (intptr_t)::Strings.RegString(e.first.c_str()));
		e.second->accept(this);
	}
	AddBCC(n->loc, AB_NEW_PROPLIST, n->values.size());
	type_of_stack_top = C4V_PropList;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::NilLit *n)
{
	AddBCC(n->loc, AB_NIL);
	type_of_stack_top = C4V_Nil;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::ThisLit *n)
{
	AddBCC(n->loc, AB_THIS);
	type_of_stack_top = C4V_PropList;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::VarExpr *n)
{
	assert(Fn);
	C4Value dummy;
	const char *cname = n->identifier.c_str();
	C4String *interned = ::Strings.FindString(cname);

	// Reset known type of top of value stack so we don't keep the old one around
	type_of_stack_top = C4V_Any;

	// Lookup order: Parameters > var > local > global > global const
	// Why parameters are considered before function-scoped variables
	// you ask? I've no idea, but that's how it was before I started
	// changing things.
	if (Fn->ParNamed.GetItemNr(cname) != -1)
	{
		int pos = Fn->ParNamed.GetItemNr(cname);
		AddVarAccess(n->loc, AB_DUP, -Fn->GetParCount() + pos);
		type_of_stack_top = Fn->GetParType()[pos];
	}
	else if (Fn->VarNamed.GetItemNr(cname) != -1)
	{
		AddVarAccess(n->loc, AB_DUP, Fn->VarNamed.GetItemNr(cname));
	}
	// Can't use Fn->Parent->HasProperty here because that only returns true
	// for immediate properties, while we also want to interrogate prototypes
	else if (Fn->Parent && interned && Fn->Parent->GetPropertyByS(interned, &dummy))
	{
		AddBCC(n->loc, AB_LOCALN, (intptr_t)interned);
	}
	else if (ScriptEngine.GlobalNamedNames.GetItemNr(cname) != -1)
	{
		AddBCC(n->loc, AB_GLOBALN, ScriptEngine.GlobalNamedNames.GetItemNr(cname));
	}
	else if (ScriptEngine.GlobalConstNames.GetItemNr(cname) != -1)
	{
		C4Value v;
		ENSURE_COND(ScriptEngine.GetGlobalConstant(cname, &v), "internal error: global constant not retrievable");
		switch (v.GetType())
		{
		case C4V_Nil:
			AddBCC(n->loc, AB_NIL);
			break;
		case C4V_Int:
			AddBCC(n->loc, AB_INT, v._getInt());
			break;
		case C4V_Bool:
			AddBCC(n->loc, AB_BOOL, v._getBool());
			break;
		case C4V_PropList:
			AddBCC(n->loc, AB_CPROPLIST, reinterpret_cast<intptr_t>(v._getPropList()));
			break;
		case C4V_String:
			AddBCC(n->loc, AB_STRING, reinterpret_cast<intptr_t>(v._getStr()));
			break;
		case C4V_Array:
			AddBCC(n->loc, AB_CARRAY, reinterpret_cast<intptr_t>(v._getArray()));
			break;
		case C4V_Function:
			AddBCC(n->loc, AB_CFUNCTION, reinterpret_cast<intptr_t>(v._getFunction()));
		default:
			throw Error(target_host, host, n, Fn, "internal error: global constant of unexpected type: %s (of type %s)", cname, v.GetTypeName());
		}
		type_of_stack_top = v.GetType();
	}
	else
	{
		throw Error(target_host, host, n, Fn, "symbol not found in any symbol table: %s", cname);
	}
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::UnOpExpr *n)
{
	n->operand->accept(this);
	const auto &op = C4ScriptOpMap[n->op];
	if (op.Changer)
	{
		C4AulBCC setter = MakeSetter(n->loc, true);
		AddBCC(n->loc, op.Code, 0);
		AddBCC(n->loc, setter);
		// On postfix inc/dec, regenerate the previous value
		if (op.Postfix && (op.Code == AB_Inc || op.Code == AB_Dec))
		{
			AddBCC(n->loc, op.Code == AB_Inc ? AB_Dec : AB_Inc, 1);
		}
	}
	else
	{
		AddBCC(n->loc, op.Code);
	}
	type_of_stack_top = op.RetType;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::BinOpExpr *n)
{
	n->lhs->accept(this);

	const auto &op = C4ScriptOpMap[n->op];
	if (op.Code == AB_JUMPAND || op.Code == AB_JUMPOR || op.Code == AB_JUMPNNIL)
	{
		// Short-circuiting operators. These are slightly more complex
		// because we don't want to evaluate their rhs operand when the
		// lhs one already decided the result
		int jump = AddBCC(n->loc, op.Code);
		n->rhs->accept(this);
		UpdateJump(jump, AddJumpTarget());
	}
	else if (op.Changer)
	{
		C4AulBCC setter = MakeSetter(n->loc, true);
		n->rhs->accept(this);
		AddBCC(n->loc, op.Code);
		AddBCC(n->loc, setter);
	}
	else
	{
		n->rhs->accept(this);
		AddBCC(n->loc, op.Code, 0);
	}

	type_of_stack_top = op.RetType;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::AssignmentExpr *n)
{
	n->lhs->accept(this);
	C4AulBCC setter = MakeSetter(n->loc, false);
	n->rhs->accept(this);
	AddBCC(n->loc, setter);

	// Assignment does not change the type of the variable
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::SubscriptExpr *n)
{
	n->object->accept(this);
	n->index->accept(this);
	AddBCC(n->loc, AB_ARRAYA);

	// FIXME: Check if the subscripted object is a literal and if so, retrieve type
	type_of_stack_top = C4V_Any;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::SliceExpr *n)
{
	n->object->accept(this);
	n->start->accept(this);
	n->end->accept(this);
	AddBCC(n->loc, AB_ARRAY_SLICE);

	type_of_stack_top = C4V_Array;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::CallExpr *n)
{
	const char *cname = n->callee.c_str();

	if (n->callee == C4AUL_DebugBreak)
	{
		if (n->context)
			throw Error(target_host, host, n, Fn, "\"%s\" can't be called in a different context", cname);
		if (!n->args.empty())
			throw Error(target_host, host, n, Fn, "\"%s\" must not have any arguments", cname);

		AddBCC(n->loc, AB_DEBUG);
		// Add a pseudo-nil to keep the stack balanced
		AddBCC(n->loc, AB_NIL);
		type_of_stack_top = C4V_Nil;
		return;
	}

	const auto pre_call_stack = stack_height;

	if (n->context)
		n->context->accept(this);

	std::vector<C4V_Type> known_par_types;
	known_par_types.reserve(n->args.size());

	for (const auto &arg : n->args)
	{
		arg->accept(this);
		known_par_types.push_back(type_of_stack_top);
	}

	C4AulFunc *callee = nullptr;

	// Special handling for the overload chain
	if (n->callee == C4AUL_Inherited || n->callee == C4AUL_SafeInherited)
	{
		if (n->context)
		{
			throw Error(target_host, host, n, Fn, "\"%s\" can't be called in a different context", cname);
		}
		callee = Fn->OwnerOverloaded;
		if (!callee)
		{
			if (n->callee == C4AUL_SafeInherited)
			{
				// pop all args off the stack
				if (!n->args.empty())
					AddBCC(n->loc, AB_STACK, -(intptr_t)n->args.size());
				// and "return" nil
				AddBCC(n->loc, AB_NIL);
				type_of_stack_top = C4V_Nil;
				return;
			}
			else
			{
				throw Error(target_host, host, n, Fn, "inherited function not found (use " C4AUL_SafeInherited " to disable this message)");
			}
		}
	}

	int fn_argc = C4AUL_MAX_Par;
	if (!n->context)
	{
		// if this is a function without explicit context, we resolve it
		if (!callee)
			callee = Fn->Parent->GetFunc(cname);
		if (!callee && target_host)
			callee = target_host->Engine->GetFunc(cname);

		if (callee)
			fn_argc = callee->GetParCount();
		else
			throw Error(target_host, host, n, Fn, "called function not found: %s", cname);
	}

	if (n->args.size() > fn_argc)
	{
		// Pop off any args that are over the limit
		Warn(target_host, host, n->args[fn_argc].get(), Fn,
			"call to %s passes %d parameters, of which only %d are used", cname, n->args.size(), fn_argc);
		AddBCC(n->loc, AB_STACK, fn_argc - n->args.size());
	}
	else if (n->args.size() < fn_argc)
	{
		if (n->append_unnamed_pars)
		{
			assert(Fn->GetParCount() == C4AUL_MAX_Par);
			int missing_par_count = fn_argc - n->args.size();
			int available_par_count = Fn->GetParCount() - Fn->ParNamed.iSize;
			for (int i = 0; i < std::min(missing_par_count, available_par_count); ++i)
			{
				AddVarAccess(n->loc, AB_DUP, -Fn->GetParCount() + Fn->ParNamed.iSize + i);
			}
			// Fill up remaining, unsettable parameters with nil
			if (available_par_count < missing_par_count)
				AddBCC(n->loc, AB_STACK, missing_par_count - available_par_count);
		}
		else if (fn_argc > n->args.size())
		{
			// Add nil for each missing parameter
			AddBCC(n->loc, AB_STACK, fn_argc - n->args.size());
		}
	}

	// Check passed parameters for this call (as far as possible)
	std::vector<C4V_Type> expected_par_types;
	if (n->context)
	{
		AddBCC(n->loc, n->safe_call ? AB_CALLFS : AB_CALL, (intptr_t)::Strings.RegString(cname));
		// Since we don't know the context in which this call will happen at
		// runtime, we'll check whether all available functions with the same
		// name agree on their parameters.
		const C4AulFunc *candidate = target_host ? target_host->Engine->GetFirstFunc(cname) : nullptr;
		if (candidate)
		{
			expected_par_types.assign(candidate->GetParType(), candidate->GetParType() + candidate->GetParCount());
			while ((candidate = target_host->Engine->GetNextSNFunc(candidate)) != nullptr)
			{
				if (candidate->GetParCount() > expected_par_types.size())
				{
					expected_par_types.resize(candidate->GetParCount(), C4V_Any);
				}
				for (size_t i = 0; i < std::min<size_t>(known_par_types.size(), candidate->GetParCount()); ++i)
				{
					C4V_Type a = known_par_types[i];
					C4V_Type b = candidate->GetParType()[i];
					// If we can convert one of the types into the other
					// without a warning, use the wider one
					bool implicit_a_to_b = !C4Value::WarnAboutConversion(a, b);
					bool implicit_b_to_a = !C4Value::WarnAboutConversion(b, a);
					if (implicit_a_to_b && !implicit_b_to_a)
						expected_par_types[i] = b;
					else if (implicit_b_to_a && !implicit_a_to_b)
						expected_par_types[i] = a;
					// but if we can convert neither of the types into the
					// other, give up and assume the user will do the right
					// thing
					else if (!implicit_a_to_b && !implicit_b_to_a)
						expected_par_types[i] = C4V_Any;
				}
			}
		}
		type_of_stack_top = C4V_Any;
	}
	else
	{
		assert(callee);
		AddBCC(n->loc, AB_FUNC, (intptr_t)callee);
		expected_par_types.assign(callee->GetParType(), callee->GetParType() + callee->GetParCount());
		type_of_stack_top = callee->GetRetType();
	}

	// Check parameters
	for (size_t i = 0; i < std::min(known_par_types.size(), expected_par_types.size()); ++i)
	{
		C4V_Type from = known_par_types[i];
		C4V_Type to = expected_par_types[i];
		if (C4Value::WarnAboutConversion(from, to))
		{
			Warn(target_host, host, n->args[i].get(), Fn, "parameter %d of %s is %s (%s expected)", i, cname, GetC4VName(from), GetC4VName(to));
		}
	}

	// We leave one value (the return value) on the stack
	assert(pre_call_stack + 1 == stack_height);
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::ParExpr *n)
{
	n->arg->accept(this);
	AddBCC(n->loc, AB_PAR);
	type_of_stack_top = C4V_Any;
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::Block *n)
{
	for (const auto &s : n->children)
	{
		const auto pre_statement_stack = stack_height;
		s->accept(this);
		// If the statement has left a stack value, pop it off
		MaybePopValueOf(s);
		assert(pre_statement_stack == stack_height);
	}
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::Return *n)
{
	n->value->accept(this);
	AddBCC(n->loc, AB_RETURN);
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::ForLoop *n)
{
#if 0
	// Bytecode arranged like this:
	//        initializer
	//  cond: condition
	//        CONDN exit
	//  body: body
	//  incr: incrementor
	//        JUMP cond
	//  exit:
	//
	// continue jumps to incr
	// break jumps to exit

	if (n->init)
	{
		n->init->accept(this);
		MaybePopValueOf(n->init);
	}
	int cond = -1, condition_jump = -1;
	PushLoop();
	if (n->cond)
	{
		cond = AddJumpTarget();
		n->cond->accept(this);
		active_loops.top().breaks.push_back(AddBCC(n->cond->loc, AB_CONDN));
	}

	int body = AddJumpTarget();
	if (!n->cond)
		cond = body;
	n->body->accept(this);
	MaybePopValueOf(n->body);

	int incr = -1;
	if (n->incr)
	{
		incr = AddJumpTarget();
		n->incr->accept(this);
		MaybePopValueOf(n->incr);
	}
	else
	{
		// If no incrementor exists, just jump straight to the condition
		incr = cond;
	}
	// start the next iteration of the loop
	AddJumpTo(AB_JUMP, cond);
	PopLoop(incr);
#else
	// Bytecode arranged like this:
	//        initializer
	//  cond: condition
	//        CONDN exit
	//        JUMP body
	//  incr: incrementor
	//        JUMP cond
	//  body: body
	//        JUMP incr
	//  exit:
	//
	// continue jumps to incr
	// break jumps to exit

	if (n->init)
	{
		n->init->accept(this);
		MaybePopValueOf(n->init);
	}
	PushLoop();
	int cond = AddJumpTarget();
	if (n->cond)
	{
		n->cond->accept(this);
		active_loops.top().breaks.push_back(AddBCC(n->cond->loc, AB_CONDN));
	}
	int incr = cond;
	if (n->incr)
	{
		int cond_jump = AddBCC(n->loc, AB_JUMP);
		incr = AddJumpTarget();
		n->incr->accept(this);
		MaybePopValueOf(n->incr);
		AddJumpTo(n->loc, AB_JUMP, cond);
		UpdateJump(cond_jump, AddJumpTarget());
	}
	n->body->accept(this);
	MaybePopValueOf(n->body);
	AddJumpTo(n->loc, AB_JUMP, incr);
	PopLoop(incr);
#endif
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::RangeLoop *n)
{
	// Bytecode arranged like this:
	//        condition (aka iterated array)
	//        INT 0 (the loop index variable)
	//  cond: FOREACH_NEXT
	//        JUMP exit
	//  body: body
	//        JUMP cond
	//  exit: STACK -2 (to clean the iteration variables)
	//
	// continue jumps to cond
	// break jumps to exit

	const char *cname = n->var.c_str();
	int var_id = Fn->VarNamed.GetItemNr(cname);
	assert(var_id != -1 && "CodegenAstVisitor: unable to find variable in foreach");
	if (var_id == -1)
		throw Error(target_host, host, n, Fn, "internal error: unable to find variable in foreach: %s", cname);
	// Emit code for array
	n->cond->accept(this);
	// Emit code for iteration
	AddBCC(n->loc, AB_INT, 0);
	int cond = AddJumpTarget();
	PushLoop();
	AddVarAccess(n->loc, AB_FOREACH_NEXT, var_id);
	AddLoopControl(n->loc, Loop::Control::Break); // Will be skipped by AB_FOREACH_NEXT as long as more entries exist
										  // Emit body
	n->body->accept(this);
	MaybePopValueOf(n->body);
	// continue starts the next iteration of the loop
	AddLoopControl(n->loc, Loop::Control::Continue);
	PopLoop(cond);
	// Pop off iterator and array
	AddBCC(n->loc, AB_STACK, -2);
}

void C4AulCompiler::CodegenAstVisitor::EmitFunctionCode(const ::aul::ast::Function *f, const ::aul::ast::Node *n)
{
	assert(Fn != nullptr);

	Fn->ClearCode();

	// Reserve var stack space
	if (Fn->VarNamed.iSize > 0)
		AddBCC(n->loc, AB_STACK, Fn->VarNamed.iSize);
	stack_height = 0;

	try
	{
		f->body->accept(this);
	}
	catch (C4AulParseError &e)
	{
		AddBCC(nullptr, AB_ERR, (intptr_t)::Strings.RegString(e.what()));
		throw;
	}

	if (f->body->children.empty() || !dynamic_cast<::aul::ast::Return*>(f->body->children.rbegin()->get()))
	{
		// If the last statement isn't a return, add one to the byte
		// code. We're not doing CFA because the worst thing that might
		// happen is we insert two instructions that never get executed.
		AddBCC(n->loc, AB_NIL);
		AddBCC(n->loc, AB_RETURN);
	}
	Fn->DumpByteCode();
	// This instruction should never be reached but we'll add it just in
	// case.
	AddBCC(n->loc, AB_EOFN);
	assert(stack_height == 0);
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::DoLoop *n)
{
	int body = AddJumpTarget();
	PushLoop();
	try
	{
		n->body->accept(this);
		MaybePopValueOf(n->body);
		int cond = AddJumpTarget();
		n->cond->accept(this);
		AddJumpTo(n->loc, AB_COND, body);
		PopLoop(cond);
	}
	catch (...)
	{
		PopLoop(AddJumpTarget());
		throw;
	}
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::WhileLoop *n)
{
	int cond = AddJumpTarget();
	PushLoop();
	try
	{
		n->cond->accept(this);
		active_loops.top().breaks.push_back(AddBCC(n->cond->loc, AB_CONDN));
		n->body->accept(this);
		MaybePopValueOf(n->body);
		// continue starts the next iteration of the loop
		AddLoopControl(n->loc, Loop::Control::Continue);
		PopLoop(cond);
	}
	catch (...)
	{
		PopLoop(AddJumpTarget());
		throw;
	}
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::Break *n)
{
	ENSURE_COND(!active_loops.empty(), "'break' outside loop");
	AddLoopControl(n->loc, Loop::Control::Break);
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::Continue *n)
{
	ENSURE_COND(!active_loops.empty(), "'continue' outside loop");
	AddLoopControl(n->loc, Loop::Control::Continue);
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::If *n)
{
	n->cond->accept(this);
	int jump = AddBCC(n->loc, AB_CONDN);
	n->iftrue->accept(this);
	MaybePopValueOf(n->iftrue);
	if (n->iffalse)
	{
		int jumpout = AddBCC(n->loc, AB_JUMP);
		UpdateJump(jump, AddJumpTarget());
		jump = jumpout;
		n->iffalse->accept(this);
		MaybePopValueOf(n->iffalse);
	}
	UpdateJump(jump, AddJumpTarget());
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::VarDecl *n)
{
	for (const auto &dec : n->decls)
	{
		const char *cname = dec.name.c_str();
		switch (n->scope)
		{
		case ::aul::ast::VarDecl::Scope::Func:
			if (dec.init)
			{
				// Emit code for the initializer
				dec.init->accept(this);
				int var_idx = Fn->VarNamed.GetItemNr(cname);
				assert(var_idx >= 0 && "CodegenAstVisitor: var not found in variable table");
				if (var_idx < 0)
				{
					AddBCC(n->loc, AB_STACK, -1);
					throw Error(target_host, host, n, Fn, "internal error: var not found in variable table: %s", cname);
				}
				AddVarAccess(n->loc, AB_POP_TO, var_idx);
			}
			break;
		case ::aul::ast::VarDecl::Scope::Object:
		case ::aul::ast::VarDecl::Scope::Global:
			// Object-local and global constants are handled by ConstantResolver.
			break;
		}
	}
}

void C4AulCompiler::CodegenAstVisitor::visit(const ::aul::ast::FunctionDecl *n)
{
	C4PropListStatic *Parent = n->is_global ? target_host->Engine->GetPropList() : target_host->GetPropList();

	C4String *name = ::Strings.FindString(n->name.c_str());
	C4AulFunc *f = Parent->GetFunc(name);
	while (f)
	{
		if (f->SFunc() && f->SFunc()->pOrgScript == host && f->Parent == Parent)
		{
			if (Fn)
				Warn(target_host, host, n, Fn, "function declared multiple times");
			Fn = f->SFunc();
		}
		f = f->SFunc() ? f->SFunc()->OwnerOverloaded : 0;
	}

	if (!Fn && Parent->HasProperty(name))
	{
		throw Error(target_host, host, n, Fn, "declaration of '%s': cannot override local variable via 'func %s'", n->name.c_str(), n->name.c_str());
	}

	assert(Fn && "CodegenAstVisitor: unable to find function definition");
	if (!Fn)
		throw Error(target_host, host, n, Fn, "internal error: unable to find function definition for %s", n->name.c_str());

	// If this isn't a global function, but there is a global one with
	// the same name, and this function isn't overloading a different
	// one, add the global function to the overload chain
	if (!n->is_global && !Fn->OwnerOverloaded)
	{
		C4AulFunc *global_parent = target_host->Engine->GetFunc(Fn->GetName());
		if (global_parent)
			Fn->SetOverloaded(global_parent);
	}

	EmitFunctionCode(n);

	Fn = nullptr;
}

void C4AulCompiler::CodegenAstVisitor::visit(const::aul::ast::FunctionExpr * n)
{
	throw Error(target_host, host, n, Fn, "can't define a function in a function-scoped proplist");
}

#undef ENSURE_COND
#define ENSURE_COND(cond, failmsg) do { if (!(cond)) throw Error(host, host, n, nullptr, failmsg); } while (0)

// Evaluates constant AST subtrees and returns the final C4Value.
// Throws ExpressionNotConstant if evaluation fails.

C4Value C4AulCompiler::ConstexprEvaluator::eval(C4ScriptHost *host, const ::aul::ast::Expr *e, EvalFlags flags)
{
	ConstexprEvaluator ce(host);
	ce.ignore_unset_values = (flags & IgnoreUnset) == IgnoreUnset;
	e->accept(&ce);
	return ce.v;
}

C4Value C4AulCompiler::ConstexprEvaluator::eval_static(C4ScriptHost *host, C4PropListStatic *parent, const std::string &parent_key, const ::aul::ast::Expr *e, EvalFlags flags)
{
	ConstexprEvaluator ce(host);
	ce.proplist_magic = ConstexprEvaluator::ProplistMagic{ true, parent, parent_key };
	ce.ignore_unset_values = (flags & IgnoreUnset) == IgnoreUnset;
	e->accept(&ce);
	return ce.v;
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::StringLit *n) { v = C4VString(n->value.c_str()); }

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::IntLit *n) { v = C4VInt(n->value); }

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::BoolLit *n) { v = C4VBool(n->value); }

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::ArrayLit *n)
{
	auto a = std::make_unique<C4ValueArray>(n->values.size());
	for (size_t i = 0; i < n->values.size(); ++i)
	{
		n->values[i]->accept(this);
		a->SetItem(i, v);
	}
	v = C4VArray(a.release());
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::ProplistLit *n)
{
	std::unique_ptr<C4PropList> new_proplist;
	C4PropList *p = nullptr;

	bool first_pass = true;

	if (proplist_magic.active)
	{
		// Check if there's already a proplist available
		C4String *key = ::Strings.RegString(proplist_magic.key.c_str());
		C4Value old;
		if (proplist_magic.parent)
		{
			proplist_magic.parent->GetPropertyByS(key, &old);
		}
		else
		{
			// If proplist_magic.parent is NULL, we're handling a global constant.
			host->Engine->GetGlobalConstant(key->GetCStr(), &old);
		}
		if (old.getPropList())
		{
			p = old.getPropList();
			first_pass = false;
		}
		else
		{
			p = C4PropList::NewStatic(NULL, proplist_magic.parent, key);
			new_proplist.reset(p);
		}
	}
	else
	{
		p = C4PropList::New();
		new_proplist.reset(p);
	}

	// Since the values may be functions that refer to other values in the
	// proplist, pre-populate the new proplist with dummy values until the
	// real ones are set
	if (first_pass)
	{
		for (const auto &kv : n->values)
		{
			p->SetPropertyByS(::Strings.RegString(kv.first.c_str()), C4VNull);
		}
	}

	auto saved_magic = std::move(proplist_magic);
	for (const auto &kv : n->values)
	{
		proplist_magic = ProplistMagic { saved_magic.active, p->IsStatic(), kv.first };
		kv.second->accept(this);
		p->SetPropertyByS(::Strings.RegString(kv.first.c_str()), v);
		proplist_magic = std::move(saved_magic);
	}
	v = C4VPropList(p);
	new_proplist.release();
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::NilLit *) { v = C4VNull; }

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::ThisLit *n) { nonconst(n); }

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::VarExpr *n)
{
	const char *cname = n->identifier.c_str();
	C4String *interned = ::Strings.FindString(cname);
	if (interned && host->GetPropList()->GetPropertyByS(interned, &v))
		return;
	if (host->Engine->GetGlobalConstant(cname, &v))
		return;

	if (ignore_unset_values)
	{
		v = C4VNull;
		return;
	}

	nonconst(n);
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::UnOpExpr *n)
{
	n->operand->accept(this);
	assert(n->op > 0);
	const auto &op = C4ScriptOpMap[n->op];
	if (op.Changer)
		nonconst(n);
	AssertValueType(v, op.Type1, op.Identifier, n);
	switch (op.Code)
	{
	case AB_BitNot:
		v.SetInt(~v._getInt());
		break;
	case AB_Not:
		v.SetBool(!v.getBool());
		break;
	case AB_Neg:
		v.SetInt(-v._getInt());
		break;
	default:
		assert(!"ConstexprEvaluator: Unexpected unary operator");
		throw Error(host, host, n, nullptr, "internal error: unary operator not found in operator table");
	}
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::BinOpExpr *n)
{
	assert(n->op > 0);
	const auto &op = C4ScriptOpMap[n->op];
	if (op.Changer)
		nonconst(n);

	n->lhs->accept(this);
	C4Value lhs = v;
	// Evaluate the short-circuiting operators here
	if ((op.Code == AB_JUMPAND && !lhs) || (op.Code == AB_JUMPOR && lhs) || (op.Code == AB_JUMPNNIL && lhs.GetType() != C4V_Nil))
	{
		v = lhs;
		return;
	}
	n->rhs->accept(this);
	C4Value &rhs = v;

	AssertValueType(lhs, op.Type1, op.Identifier, n);
	AssertValueType(rhs, op.Type2, op.Identifier, n);

	switch (op.Code)
	{
	case AB_Pow:
		v.SetInt(Pow(lhs._getInt(), rhs._getInt()));
		break;
	case AB_Div:
		ENSURE_COND(rhs._getInt() != 0, "division by zero");
		ENSURE_COND(lhs._getInt() != INT32_MIN || rhs._getInt() != -1, "division overflow");
		v.SetInt(lhs._getInt() / rhs._getInt());
		break;
	case AB_Mul:
		v.SetInt(lhs._getInt() * rhs._getInt());
		break;
	case AB_Mod:
		ENSURE_COND(rhs._getInt() != 0, "division by zero");
		ENSURE_COND(lhs._getInt() != INT32_MIN || rhs._getInt() != -1, "division overflow");
		v.SetInt(lhs._getInt() / rhs._getInt());
		break;
#define INT_BINOP(code, op) case code: v.SetInt(lhs._getInt() op rhs._getInt()); break
		INT_BINOP(AB_Sum, +);
		INT_BINOP(AB_Sub, -);
		INT_BINOP(AB_LeftShift, << );
		INT_BINOP(AB_RightShift, >> );
		INT_BINOP(AB_BitAnd, &);
		INT_BINOP(AB_BitXOr, ^);
		INT_BINOP(AB_BitOr, | );
#undef INT_BINOP
#define BOOL_BINOP(code, op) case code: v.SetBool(lhs._getInt() op rhs._getInt()); break
		BOOL_BINOP(AB_LessThan, <);
		BOOL_BINOP(AB_LessThanEqual, <= );
		BOOL_BINOP(AB_GreaterThan, >);
		BOOL_BINOP(AB_GreaterThanEqual, >= );
#undef BOOL_BINOP
	case AB_Equal:
		v.SetBool(lhs.IsIdenticalTo(rhs));
		break;
	case AB_NotEqual:
		v.SetBool(!lhs.IsIdenticalTo(rhs));
		break;
	case AB_JUMPAND:
	case AB_JUMPOR:
	case AB_JUMPNNIL:
		// If we hit this, then the short-circuit above failed
		v = rhs;
		break;
	default:
		assert(!"ConstexprEvaluator: Unexpected binary operator");
		throw Error(host, host, n, nullptr, "internal error: binary operator not found in operator table");
		break;
	}
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::AssignmentExpr *n)
{
	nonconst(n);
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::SubscriptExpr *n)
{
	n->object->accept(this);
	C4Value obj = v;
	n->index->accept(this);
	C4Value &index = v;

	if (obj.CheckConversion(C4V_Array))
	{
		ENSURE_COND(index.CheckConversion(C4V_Int), FormatString("array access: index of type %s, but expected int", index.GetTypeName()).getData());
		v = obj.getArray()->GetItem(index.getInt());
	}
	else if (obj.CheckConversion(C4V_PropList))
	{
		ENSURE_COND(index.CheckConversion(C4V_String), FormatString("proplist access: index of type %s, but expected string", index.GetTypeName()).getData());
		if (!obj.getPropList()->GetPropertyByS(index.getStr(), &v))
			v.Set0();
	}
	else
	{
		ENSURE_COND(false, FormatString("can't access %s as array or proplist", obj.GetTypeName()).getData());
	}
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::SliceExpr *n)
{
	n->object->accept(this);
	C4Value obj = v;
	n->start->accept(this);
	C4Value start = v;
	n->end->accept(this);
	C4Value &end = v;

	ENSURE_COND(obj.CheckConversion(C4V_Array), FormatString("array slice: can't access %s as an array", obj.GetTypeName()).getData());
	ENSURE_COND(start.CheckConversion(C4V_Int), FormatString("array slice: start index of type %s, int expected", start.GetTypeName()).getData());
	ENSURE_COND(end.CheckConversion(C4V_Int), FormatString("array slice: end index of type %s, int expected", end.GetTypeName()).getData());

	v.SetArray(obj.getArray()->GetSlice(start.getInt(), end.getInt()));
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::CallExpr *n)
{
	// TODO: allow side-effect-free calls here
	nonconst(n);
}

void C4AulCompiler::ConstexprEvaluator::visit(const ::aul::ast::FunctionExpr *n)
{
	// Function expressions can only occur inside static proplists.
	ENSURE_COND(proplist_magic.active, "internal error: function expression outside of static proplist");

	C4AulScriptFunc *sfunc = nullptr;
	bool first_pass = true;

	if (auto func = proplist_magic.parent->GetFunc(proplist_magic.key.c_str()))
	{
		sfunc = func->SFunc();
		first_pass = false;
	}
	else
	{
		sfunc = new C4AulScriptFunc(proplist_magic.parent, host, proplist_magic.key.c_str(), n->loc);
	}

	ENSURE_COND(sfunc != nullptr, "internal error: function expression target resolved to non-function value");

	if (first_pass)
	{
		for (const auto &param : n->params)
		{
			sfunc->AddPar(param.name.c_str());
		}
		if (n->has_unnamed_params)
			sfunc->ParCount = C4AUL_MAX_Par;

		PreparseAstVisitor preparser(host, host, sfunc);
		preparser.visit(n->body.get());
	}
	else
	{
		CodegenAstVisitor cg(sfunc);
		cg.EmitFunctionCode(n);
	}

	v.SetFunction(sfunc);
}

void C4AulCompiler::ConstantResolver::visit(const ::aul::ast::VarDecl *n)
{
	for (const auto &dec : n->decls)
	{
		const char *cname = dec.name.c_str();
		switch (n->scope)
		{
		case ::aul::ast::VarDecl::Scope::Func:
			// Function-scoped declarations and their initializers are handled by CodegenAstVisitor.
			break;
		case ::aul::ast::VarDecl::Scope::Object:
			if (dec.init)
			{
				assert(host->GetPropList()->IsStatic());
				C4Value v = ConstexprEvaluator::eval_static(host, host->GetPropList()->IsStatic(), dec.name, dec.init.get(), ConstexprEvaluator::IgnoreUnset);
				host->GetPropList()->SetPropertyByS(::Strings.RegString(cname), v);
			}
			else
			{
				host->GetPropList()->SetPropertyByS(::Strings.RegString(cname), C4VNull);
			}
			break;
		case ::aul::ast::VarDecl::Scope::Global:
			if ((dec.init != nullptr) != n->constant)
				throw Error(host, host, n->loc, nullptr, "global variable must be either constant or uninitialized: %s", cname);
			else if (dec.init)
			{
				assert(n->constant && "CodegenAstVisitor: initialized global variable isn't const");
				C4Value *v = host->Engine->GlobalConsts.GetItem(cname);
				assert(v && "CodegenAstVisitor: global constant not found in variable table");
				if (!v)
					throw Error(host, host, n->loc, nullptr, "internal error: global constant not found in variable table: %s", cname);
				*v = ConstexprEvaluator::eval_static(host, nullptr, dec.name, dec.init.get(), ConstexprEvaluator::IgnoreUnset);
			}
			break;
		}
	}
}
