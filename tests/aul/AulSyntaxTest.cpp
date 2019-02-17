/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2016, The OpenClonk Team and contributors
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

#include "ErrorHandler.h"
#include <gtest/gtest.h>

#include "AulSyntaxTestDetail.h"

#include "script/C4AulAST.h"
#include "script/C4AulParse.h"
#include "script/C4ScriptHost.h"

#include <type_traits>
#include <typeinfo>

class DummyScriptHost : public C4ScriptHost
{
public:
	explicit DummyScriptHost(const char *code)
	{
		Script.Copy(code);
	}
};

class TestableAulParse : public C4AulParse
{
public:
	template<class... T>
	TestableAulParse(T &&...t) : C4AulParse(std::forward<T>(t)...)
	{
		Shift();
	}

	// Make all of the Parse_* function public so we can test them
	using C4AulParse::Parse_ToplevelFunctionDecl;
	using C4AulParse::Parse_Statement;
	using C4AulParse::Parse_Block;
	using C4AulParse::Parse_Array;
	using C4AulParse::Parse_PropList;
	using C4AulParse::Parse_DoWhile;
	using C4AulParse::Parse_While;
	using C4AulParse::Parse_If;
	using C4AulParse::Parse_For;
	using C4AulParse::Parse_ForEach;
	using C4AulParse::Parse_Expression;
	using C4AulParse::Parse_Var;
};

class AulSyntaxTest
{};

static std::unique_ptr<::aul::ast::Stmt> ParseStatement(const char *code)
{
	DummyScriptHost host{ code };
	TestableAulParse parser{ &host };
	return parser.Parse_Statement();
}
static std::unique_ptr<::aul::ast::Expr> ParseExpression(const char *code)
{
	DummyScriptHost host{ code };
	TestableAulParse parser{ &host };
	return parser.Parse_Expression();
}
static std::unique_ptr<::aul::ast::Script> ParseScript(const char *code)
{
	DummyScriptHost host{ code };
	TestableAulParse parser{ &host };
	return parser.Parse_Script(&host);
}

TEST(AulSyntaxTest, ParseSyntaxErrors)
{
	EXPECT_THROW(ParseStatement("1"), C4AulParseError);
	EXPECT_THROW(ParseStatement("func Main() {}"), C4AulParseError);
}

TEST(AulSyntaxTest, PragmaWarningWithInsufficientData)
{
	EXPECT_THROW(ParseScript("#warning"), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning\n"), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning "), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning \n"), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning e"), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning enabl"), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning d"), C4AulParseError);
	EXPECT_THROW(ParseScript("#warning disabl"), C4AulParseError);
}

using namespace ::aul::ast;
TEST(AulSyntaxTest, ParseLiterals)
{
	// Basic literals
	EXPECT_THAT(ParseExpression("1"), MatchesAst(IntLit(1)));
	EXPECT_THAT(ParseExpression("2147483647"), MatchesAst(IntLit(2147483647)));
	// While a leading + seems like it should be an operator expression,
	// we don't ever emit a no-op + operator from the parser
	EXPECT_THAT(ParseExpression("+4"), MatchesAst(IntLit(4)));
	EXPECT_THAT(ParseExpression("0xFFFFFFFF"), MatchesAst(IntLit(0xFFFFFFFF)));
	EXPECT_THAT(ParseExpression("this"), MatchesAst(ThisLit()));
	EXPECT_THAT(ParseExpression("nil"), MatchesAst(NilLit()));
	EXPECT_THAT(ParseExpression("false"), MatchesAst(BoolLit(false)));
	EXPECT_THAT(ParseExpression("true"), MatchesAst(BoolLit(true)));

	// String literals
	EXPECT_THAT(ParseExpression("\"\""), MatchesAst(StringLit("")));
	EXPECT_THAT(ParseExpression("\"[]\""), MatchesAst(StringLit("[]")));
	// can't use a raw string for this because MSVC chokes on it w/ C2017: illegal escape sequence
	//EXPECT_THAT(ParseExpression(R"("\"")"), MatchesAst(StringLit("\"")));
	EXPECT_THAT(ParseExpression("\"\\\"\""), MatchesAst(StringLit("\""))); 
	EXPECT_THAT(ParseExpression("\"\\xaF\\x41\""), MatchesAst(StringLit("\xaf\x41")));
	EXPECT_THAT(ParseExpression("\"\\142\""), MatchesAst(StringLit("\142")));
	EXPECT_THAT(ParseExpression("\"\\\\\\t\\n\""), MatchesAst(StringLit("\\\t\n")));

	// Compound literals
	{
		auto ast = ArrayLit();
		EXPECT_THAT(ParseExpression("[]"), MatchesAst(ast));
		ast.values.push_back(std::make_unique<IntLit>(1));
		EXPECT_THAT(ParseExpression("[1]"), MatchesAst(ast));
		ast.values.push_back(std::make_unique<IntLit>(2));
		ast.values.push_back(std::make_unique<IntLit>(3));
		EXPECT_THAT(ParseExpression("[1, 2, 3]"), MatchesAst(ast));
	}
	{
		auto ast = ArrayLit();
		ast.values.push_back(std::make_unique<StringLit>("Hello"));
		EXPECT_THAT(ParseExpression("[\"Hello\"]"), MatchesAst(ast));
		ast.values.push_back(std::make_unique<IntLit>(2));
		ast.values.push_back(std::make_unique<ArrayLit>());
		EXPECT_THAT(ParseExpression("[\"Hello\", 2, []]"), MatchesAst(ast));
	}
	{
		auto ast = ProplistLit();
		ast.values.emplace_back("foo", std::make_unique<StringLit>("bar"));
		EXPECT_THAT(ParseExpression(R"({foo: "bar"})"), MatchesAst(ast));
		ast.values.emplace_back("Prototype", std::make_unique<ProplistLit>());
		EXPECT_THAT(ParseExpression(R"({foo: "bar", Prototype: {}})"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseUnOps)
{
	// Basic unary operator expressions
	{
		auto ast = UnOpExpr(0,
			std::make_unique<VarExpr>("foo")
		);
		EXPECT_THAT(ParseExpression("++foo"), MatchesAst(ast));
	}
	{
		auto ast = UnOpExpr(7,
			std::make_unique<VarExpr>("foo")
		);
		EXPECT_THAT(ParseExpression("foo--"), MatchesAst(ast));
	}
	{
		auto ast = UnOpExpr(5,
			std::make_unique<IntLit>(0)
		);
		EXPECT_THAT(ParseExpression("-0"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseBinOps)
{
	// Basic binary operator expressions
	{
		auto ast = BinOpExpr(13,
			std::make_unique<IntLit>(1),
			std::make_unique<IntLit>(1)
		);
		EXPECT_THAT(ParseExpression("1 + 1"), MatchesAst(ast));
	}
	{
		// This expression doesn't make any sense semantically, but
		// syntactically it's correct
		auto ast = BinOpExpr(31,
			std::make_unique<IntLit>(1),
			std::make_unique<IntLit>(1)
		);
		EXPECT_THAT(ParseExpression("1 += 1"), MatchesAst(ast));
	}

	// Assignment operator
	{
		auto ast = AssignmentExpr(
			std::make_unique<VarExpr>("foo"),
			std::make_unique<VarExpr>("bar")
		);
		EXPECT_THAT(ParseExpression("foo = bar"), MatchesAst(ast));
	}
	{
		auto ast = AssignmentExpr(
			std::make_unique<VarExpr>("foo"),
			std::make_unique<BoolLit>(false)
		);
		EXPECT_THAT(ParseExpression("foo = false"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseOpPriority)
{
	{
		// Ensure ambiguities are resolved correctly
		auto ast = BinOpExpr(13,
			std::make_unique<UnOpExpr>(6,
				std::make_unique<VarExpr>("a")
			),
			std::make_unique<VarExpr>("b")
		);
		EXPECT_THAT(ParseExpression("a+++b"), MatchesAst(ast));
	}
	{
		auto ast = BinOpExpr(13,
			std::make_unique<VarExpr>("a"),
			std::make_unique<UnOpExpr>(0,
				std::make_unique<VarExpr>("b")
			)
		);
		EXPECT_THAT(ParseExpression("a+ ++b"), MatchesAst(ast));
	}
	{
		// This looks strange but prefix + is never emitted.
		// We should consider whether this should be allowed, however
		auto ast = BinOpExpr(13,
			std::make_unique<VarExpr>("a"),
			std::make_unique<UnOpExpr>(5,
				std::make_unique<VarExpr>("b")
			)
		);
		EXPECT_THAT(ParseExpression("a+-+b"), MatchesAst(ast));
	}
	{
		// * has higher priority than +
		auto ast = BinOpExpr(13,
			std::make_unique<VarExpr>("a"),
			std::make_unique<BinOpExpr>(10,
				std::make_unique<VarExpr>("b"),
				std::make_unique<VarExpr>("c")
			)
		);
		EXPECT_THAT(ParseExpression("a + b * c"), MatchesAst(ast));
	}
	{
		// Parentheses override operator priorities
		auto ast = BinOpExpr(10,
			std::make_unique<BinOpExpr>(13,
				std::make_unique<VarExpr>("a"),
				std::make_unique<VarExpr>("b")
			),
			std::make_unique<VarExpr>("c")
		);
		EXPECT_THAT(ParseExpression("(a + b) * c"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseSubscripts)
{
	{
		// Standard integer literal subscript
		auto ast = SubscriptExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<IntLit>(0)
		);
		EXPECT_THAT(ParseExpression("a[0]"), MatchesAst(ast));
	}
	{
		// Standard string literal subscript
		auto ast = SubscriptExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<StringLit>("b")
		);
		EXPECT_THAT(ParseExpression("a[\"b\"]"), MatchesAst(ast));
		// also accept syntactic sugar with .
		EXPECT_THAT(ParseExpression("a.b"), MatchesAst(ast));
	}
	{
		// Expression-based subscript
		auto ast = SubscriptExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<BinOpExpr>(
				13,
				std::make_unique<IntLit>(1),
				std::make_unique<VarExpr>("b")
				)
		);
		EXPECT_THAT(ParseExpression("a[1 + b]"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseSlices)
{
	{
		// Slice with no bounds
		auto ast = SliceExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<IntLit>(0),
			std::make_unique<IntLit>(std::numeric_limits<int32_t>::max())
		);
		EXPECT_THAT(ParseExpression("a[:]"), MatchesAst(ast));
	}

	{
		// Slice with lower bound
		auto ast = SliceExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<IntLit>(7),
			std::make_unique<IntLit>(std::numeric_limits<int32_t>::max())
		);
		EXPECT_THAT(ParseExpression("a[7:]"), MatchesAst(ast));
	}
	{
		// Slice with upper bound
		auto ast = SliceExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<IntLit>(0),
			std::make_unique<IntLit>(42)
		);
		EXPECT_THAT(ParseExpression("a[:42]"), MatchesAst(ast));
	}
	{
		// Slice with both bounds
		auto ast = SliceExpr(
			std::make_unique<VarExpr>("a"),
			std::make_unique<IntLit>(7),
			std::make_unique<IntLit>(42)
		);
		EXPECT_THAT(ParseExpression("a[7:42]"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseCalls)
{
	{
		// Standard call without context or args
		auto ast = CallExpr();
		ast.callee = "a";
		EXPECT_THAT(ParseExpression("a()"), MatchesAst(ast));
		// Standard call with context but no args
		ast.context = std::make_unique<VarExpr>("b");
		EXPECT_THAT(ParseExpression("b->a()"), MatchesAst(ast));
		// Fail-safe call with context but no args
		ast.safe_call = true;
		EXPECT_THAT(ParseExpression("b->~a()"), MatchesAst(ast));
		// Standard call with context and args
		ast.safe_call = false;
		ast.args.push_back(std::make_unique<BoolLit>(false));
		EXPECT_THAT(ParseExpression("b->a(false)"), MatchesAst(ast));
		// Standard call with context and args, passing unnamed parameters
		ast.append_unnamed_pars = true;
		EXPECT_THAT(ParseExpression("b->a(false, ...)"), MatchesAst(ast));
	}
	{
		// Nested call, outer fail-safe with context, inner standard without context
		auto inner = std::make_unique<CallExpr>();
		inner->callee = "a";
		inner->args.push_back(std::make_unique<IntLit>(42));
		auto ast = CallExpr();
		ast.callee = "c";
		ast.context = std::make_unique<VarExpr>("b");
		ast.args.push_back(std::move(inner));
		ast.append_unnamed_pars = true;
		EXPECT_THAT(ParseExpression("b->c(a(42), ...)"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseBlocks)
{
	{
		// Empty block
		auto ast = Block();
		EXPECT_THAT(ParseStatement("{}"), MatchesAst(ast));
	}
	{
		// Single-statement block
		auto stmt = std::make_unique<AssignmentExpr>(
			std::make_unique<VarExpr>("i"),
			std::make_unique<IntLit>(0)
			);
		auto ast = Block();
		ast.children.push_back(std::move(stmt));
		EXPECT_THAT(ParseStatement("{ i = 0; }"), MatchesAst(ast));
	}
	{
		// Nested block
		auto inner = std::make_unique<Block>();
		inner->children.push_back(
			std::make_unique<AssignmentExpr>(
				std::make_unique<VarExpr>("i"),
				std::make_unique<IntLit>(0)
			));
		auto ast = Block();
		ast.children.push_back(std::move(inner));
		ast.children.push_back(
			std::make_unique<AssignmentExpr>(
				std::make_unique<VarExpr>("i"),
				std::make_unique<IntLit>(1)
				));
		EXPECT_THAT(ParseStatement("{ { i = 0; } i = 1; }"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseForLoops)
{
	{
		// No initializer, condition, nor incrementor
		auto ast = ForLoop();
		ast.body = std::make_unique<Noop>();
		EXPECT_THAT(ParseStatement("for (;;);"), MatchesAst(ast));
	}
	{
		// Initializer without variable declaration
		auto ast = ForLoop();
		ast.body = std::make_unique<Noop>();
		ast.init = std::make_unique<AssignmentExpr>(
			std::make_unique<VarExpr>("i"),
			std::make_unique<IntLit>(0)
		);
		EXPECT_THAT(ParseStatement("for (i = 0;;);"), MatchesAst(ast));
	}
	{
		// Initializer with variable declaration
		auto decl = std::make_unique<VarDecl>();
		decl->decls.push_back(VarDecl::Var{ "i", std::make_unique<IntLit>(0) });
		auto ast = ForLoop();
		ast.body = std::make_unique<Noop>();
		ast.init = std::move(decl);
		EXPECT_THAT(ParseStatement("for (var i = 0;;);"), MatchesAst(ast));
	}
	{
		// Full for loop
		auto init = std::make_unique<VarDecl>();
		init->decls.push_back(VarDecl::Var{ "i", std::make_unique<IntLit>(0) });
		auto cond = std::make_unique<BinOpExpr>(
			16,
			std::make_unique<VarExpr>("i"),
			std::make_unique<IntLit>(10)
			);
		auto incr = std::make_unique<UnOpExpr>(
			0,
			std::make_unique<VarExpr>("i")
			);
		auto body = std::make_unique<CallExpr>();
		body->callee = "Log";
		body->args.push_back(std::make_unique<StringLit>("%d"));
		body->args.push_back(std::make_unique<VarExpr>("i"));
		auto ast = ForLoop();
		ast.init = std::move(init);
		ast.cond = std::move(cond);
		ast.incr = std::move(incr);
		ast.body = std::move(body);
		EXPECT_THAT(ParseStatement(R"(for (var i = 0; i < 10; ++i) Log("%d", i);)"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseForEachLoops)
{
	{
		// for-each without explicit variable declaration
		auto ast = RangeLoop();
		ast.var = "i";
		ast.body = std::make_unique<Noop>();
		ast.cond = std::make_unique<ArrayLit>();
		EXPECT_THAT(ParseStatement("for (i in []);"), MatchesAst(ast));
		// and with explicit variable declaration
		ast.scoped_var = true;
		EXPECT_THAT(ParseStatement("for (var i in []);"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseDoLoops)
{
	{
		// empty do loop with trivial condition
		auto ast = DoLoop();
		ast.body = std::make_unique<Noop>();
		ast.cond = std::make_unique<BoolLit>(false);
		EXPECT_THAT(ParseStatement("do ; while(false);"), MatchesAst(ast));
	}
	{
		// nested do loops with trivial condition
		auto inner = std::make_unique<DoLoop>();
		inner->body = std::make_unique<Noop>();
		inner->cond = std::make_unique<BoolLit>(false);
		auto ast = DoLoop();
		ast.body = std::move(inner);
		ast.cond = std::make_unique<BoolLit>(false);
		EXPECT_THAT(ParseStatement("do do ; while (false); while (false);"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseWhileLoops)
{
	{
		// empty while loop with trivial condition
		auto ast = WhileLoop();
		ast.cond = std::make_unique<BoolLit>(true);
		ast.body = std::make_unique<Noop>();
		EXPECT_THAT(ParseStatement("while(true);"), MatchesAst(ast));
	}
	{
		// nested while loop with trivial condition
		auto inner = std::make_unique<WhileLoop>();
		inner->cond = std::make_unique<BoolLit>(false);
		inner->body = std::make_unique<Noop>();
		auto ast = WhileLoop();
		ast.cond = std::make_unique<BoolLit>(true);
		ast.body = std::move(inner);
		EXPECT_THAT(ParseStatement("while(true) while(false);"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseIfs)
{
	{
		// trivial condition, no else branch
		auto ast = If();
		ast.cond = std::make_unique<BoolLit>(false);
		ast.iftrue = std::make_unique<Noop>();
		EXPECT_THAT(ParseStatement("if(false);"), MatchesAst(ast));
		// trivial condition, with else branch
		ast.iffalse = std::make_unique<Noop>();
		EXPECT_THAT(ParseStatement("if(false); else;"), MatchesAst(ast));
	}
	{
		// trivial condition, nested ifs. else binds to the inner if.
		auto inner = std::make_unique<If>();
		inner->cond = std::make_unique<BoolLit>(false);
		inner->iftrue = std::make_unique<AssignmentExpr>(
			std::make_unique<VarExpr>("i"),
			std::make_unique<IntLit>(0)
			);
		inner->iffalse = std::make_unique<Noop>();
		auto ast = If();
		ast.cond = std::make_unique<NilLit>();
		ast.iftrue = std::move(inner);
		EXPECT_THAT(ParseStatement("if(nil) if(false) i = 0; else;"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseFunctionDecls)
{
	{
		// local function, no parameters
		auto func = std::make_unique<FunctionDecl>("f");
		func->body = std::make_unique<Block>();
		auto ast = Script();
		ast.declarations.push_back(std::move(func));
		EXPECT_THAT(ParseScript("func f() {}"), MatchesAst(ast));
	}
	{
		// global function, unnamed parameters only
		auto func = std::make_unique<FunctionDecl>("f");
		func->body = std::make_unique<Block>();
		func->has_unnamed_params = true;
		func->is_global = true;
		auto ast = Script();
		ast.declarations.push_back(std::move(func));
		EXPECT_THAT(ParseScript("global func f(...) {}"), MatchesAst(ast));
	}
	{
		// local function, named parameters only
		auto func = std::make_unique<FunctionDecl>("f");
		func->body = std::make_unique<Block>();
		func->params.push_back(Function::Parameter{ "a", C4V_Array });
		func->params.push_back(Function::Parameter{ "b", C4V_Int });
		func->params.push_back(Function::Parameter{ "c", C4V_Any });
		auto ast = Script();
		ast.declarations.push_back(std::move(func));
		EXPECT_THAT(ParseScript("func f(array a, int b, c) {}"), MatchesAst(ast));
	}
	{
		// local function, named and unnamed parameters
		auto func = std::make_unique<FunctionDecl>("f");
		func->body = std::make_unique<Block>();
		func->params.push_back(Function::Parameter{ "a", C4V_Array });
		func->has_unnamed_params = true;
		auto ast = Script();
		ast.declarations.push_back(std::move(func));
		EXPECT_THAT(ParseScript("func f(array a, ...) {}"), MatchesAst(ast));
	}
}

TEST(AulSyntaxTest, ParseVarDecls)
{
	{
		// function-scoped variables, no initializer
		auto ast = VarDecl();
		ast.scope = VarDecl::Scope::Func;
		ast.decls.push_back(VarDecl::Var{ "a", nullptr });
		EXPECT_THAT(ParseStatement("var a;"), MatchesAst(ast));
		ast.decls.push_back(VarDecl::Var{ "b", nullptr });
		EXPECT_THAT(ParseStatement("var a, b;"), MatchesAst(ast));
	}
	{
		// function-scoped variables, partially initialized
		auto ast = VarDecl();
		ast.scope = VarDecl::Scope::Func;
		ast.decls.push_back(VarDecl::Var{ "a", std::make_unique<IntLit>(1) });
		EXPECT_THAT(ParseStatement("var a = 1;"), MatchesAst(ast));
		ast.decls.push_back(VarDecl::Var{ "b", nullptr });
		EXPECT_THAT(ParseStatement("var a = 1, b;"), MatchesAst(ast));
	}
	{
		// object-scoped variables, partially initialized
		auto ast = VarDecl();
		ast.scope = VarDecl::Scope::Object;
		ast.decls.push_back(VarDecl::Var{ "a", std::make_unique<IntLit>(1) });
		EXPECT_THAT(ParseStatement("local a = 1;"), MatchesAst(ast));
		ast.decls.push_back(VarDecl::Var{ "b", nullptr });
		EXPECT_THAT(ParseStatement("local a = 1, b;"), MatchesAst(ast));
	}
	{
		// global variables, partially initialized
		auto var = std::make_unique<VarDecl>();
		var->scope = VarDecl::Scope::Global;
		var->decls.push_back(VarDecl::Var{ "a", std::make_unique<IntLit>(1) });
		var->decls.push_back(VarDecl::Var{ "b", nullptr });
		auto ast = Script();
		ast.declarations.push_back(std::move(var));
		EXPECT_THAT(ParseScript("static a = 1, b;"), MatchesAst(ast));
	}
	{
		// global constant, initialized
		auto var = std::make_unique<VarDecl>();
		var->scope = VarDecl::Scope::Global;
		var->constant = true;
		var->decls.push_back(VarDecl::Var{ "a", std::make_unique<IntLit>(1) });
		auto call = std::make_unique<CallExpr>();
		call->callee = "f";
		var->decls.push_back(VarDecl::Var{ "b", std::move(call) });
		auto ast = Script();
		ast.declarations.push_back(std::move(var));
		EXPECT_THAT(ParseScript("static const a = 1, b = f();"), MatchesAst(ast));
	}
}
