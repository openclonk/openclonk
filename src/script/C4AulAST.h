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

// C4Aul abstract syntax tree nodes

#ifndef INC_C4AulAST
#define INC_C4AulAST

#include <memory>

#include <map>
#include <vector>

#include "script/C4Value.h"

namespace aul { namespace ast {
	class Noop;
	class StringLit;
	class IntLit;
	class BoolLit;
	class ArrayLit;
	class ProplistLit;
	class NilLit;
	class ThisLit;
	class VarExpr;
	class UnOpExpr;
	class BinOpExpr;
	class AssignmentExpr;
	class SubscriptExpr;
	class SliceExpr;
	class CallExpr;
	class ParExpr;
	class FunctionExpr;
	class Block;
	class Return;
	class ForLoop;
	class RangeLoop;
	class DoLoop;
	class WhileLoop;
	class Break;
	class Continue;
	class If;
	class VarDecl;
	class FunctionDecl;
	class IncludePragma;
	class AppendtoPragma;
	class Script;
}}

namespace aul {
class AstVisitor
{
public:
	virtual ~AstVisitor() {}

	virtual void visit(const ::aul::ast::Noop *) {}
	virtual void visit(const ::aul::ast::StringLit *) {}
	virtual void visit(const ::aul::ast::IntLit *) {}
	virtual void visit(const ::aul::ast::BoolLit *) {}
	virtual void visit(const ::aul::ast::ArrayLit *) {}
	virtual void visit(const ::aul::ast::ProplistLit *) {}
	virtual void visit(const ::aul::ast::NilLit *) {}
	virtual void visit(const ::aul::ast::ThisLit *) {}
	virtual void visit(const ::aul::ast::VarExpr *n) {}
	virtual void visit(const ::aul::ast::UnOpExpr *) {}
	virtual void visit(const ::aul::ast::BinOpExpr *) {}
	virtual void visit(const ::aul::ast::AssignmentExpr *) {}
	virtual void visit(const ::aul::ast::SubscriptExpr *) {}
	virtual void visit(const ::aul::ast::SliceExpr *) {}
	virtual void visit(const ::aul::ast::CallExpr *) {}
	virtual void visit(const ::aul::ast::ParExpr *) {}
	virtual void visit(const ::aul::ast::Block *) {}
	virtual void visit(const ::aul::ast::Return *) {}
	virtual void visit(const ::aul::ast::ForLoop *) {}
	virtual void visit(const ::aul::ast::RangeLoop *) {}
	virtual void visit(const ::aul::ast::DoLoop *) {}
	virtual void visit(const ::aul::ast::WhileLoop *) {}
	virtual void visit(const ::aul::ast::Break *) {}
	virtual void visit(const ::aul::ast::Continue *) {}
	virtual void visit(const ::aul::ast::If *) {}
	virtual void visit(const ::aul::ast::VarDecl *) {}
	virtual void visit(const ::aul::ast::FunctionDecl *) {}
	virtual void visit(const ::aul::ast::FunctionExpr *) {}
	virtual void visit(const ::aul::ast::IncludePragma *) {}
	virtual void visit(const ::aul::ast::AppendtoPragma *) {}
	virtual void visit(const ::aul::ast::Script *) {}

	// This template will catch any type missing from the list above
	// to ensure that the nodes don't accidentally get visited via a
	// base class instead
	template<class T>
	void visit(const T *) = delete;
};
}

namespace aul { namespace ast {

#define AST_NODE(cls) \
	public: \
		virtual void accept(::aul::AstVisitor *v) const override { v->visit(this); } \
		template<class... T> static std::unique_ptr<cls> New(const char *loc, T &&...t) { auto n = std::make_unique<cls>(std::forward<T>(t)...); n->loc = loc; return n; } \
	private:

class Node
{
public:
	virtual ~Node() {}

	struct Location
	{
		std::string file;
		size_t line = 0;
		size_t column = 0;
	};

	const char *loc = nullptr;

	virtual void accept(::aul::AstVisitor *) const = 0;
};

class Stmt : public Node
{
public:
	// Does executing this statement generate a return value?
	virtual bool has_value() const { return false; }
};

typedef std::unique_ptr<Stmt> StmtPtr;

class Noop : public Stmt
{
	AST_NODE(Noop);
};

class Expr : public Stmt
{
public:
	virtual bool has_value() const override { return true; }
};
typedef std::unique_ptr<Expr> ExprPtr;

class Literal : public Expr
{};

class StringLit : public Literal
{
	AST_NODE(StringLit);
public:
	explicit StringLit(const std::string &value) : value(value) {}
	std::string value;
};

class IntLit : public Literal
{
	AST_NODE(IntLit);
public:
	explicit IntLit(int32_t value) : value(value) {}
	uint32_t value;
};

class BoolLit : public Literal
{
	AST_NODE(BoolLit);
public:
	explicit BoolLit(bool value) : value(value) {}
	bool value;
};

class ArrayLit : public Literal
{
	AST_NODE(ArrayLit);
public:
	std::vector<ExprPtr> values;
};

class ProplistLit : public Literal
{
	AST_NODE(ProplistLit);
public:
	std::vector<std::pair<std::string, ExprPtr>> values;
};

class NilLit : public Literal
{
	AST_NODE(NilLit);
};

class ThisLit : public Literal
{
	AST_NODE(ThisLit);
};

class VarExpr : public Expr
{
	AST_NODE(VarExpr);
public:
	explicit VarExpr(const std::string &identifier) : identifier(identifier) {}
	std::string identifier;
};

class UnOpExpr : public Expr
{
	AST_NODE(UnOpExpr);
public:
	UnOpExpr(int op, ExprPtr &&operand) : op(op), operand(std::move(operand)) {}
	ExprPtr operand;
	int op; // TODO: Make this a proper operator type
};

class BinOpExpr : public Expr
{
	AST_NODE(BinOpExpr);
public:
	BinOpExpr(int op, ExprPtr &&lhs, ExprPtr &&rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
	ExprPtr lhs, rhs;
	int op; // TODO: Make this a proper operator type
};

class AssignmentExpr : public Expr
{
	AST_NODE(AssignmentExpr);
public:
	AssignmentExpr(ExprPtr &&lhs, ExprPtr &&rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
	ExprPtr lhs, rhs;
};

class SubscriptExpr : public Expr
{
	AST_NODE(SubscriptExpr);
public:
	SubscriptExpr(ExprPtr &&object, ExprPtr &&index) : object(std::move(object)), index(std::move(index)) {}
	ExprPtr object, index;
};

class SliceExpr : public Expr
{
	AST_NODE(SliceExpr);
public:
	SliceExpr(ExprPtr &&object, ExprPtr &&start, ExprPtr &&end) : object(std::move(object)), start(std::move(start)), end(std::move(end)) {}
	ExprPtr object, start, end;
};

class CallExpr : public Expr
{
	AST_NODE(CallExpr);
public:
	bool safe_call = false; // Will this call fail gracefully when the function doesn't exist?
	bool append_unnamed_pars = false; // Will this call append all unnamed parameters of the current function?
	ExprPtr context;
	std::vector<ExprPtr> args;
	std::string callee;
};

class ParExpr : public Expr
{
	AST_NODE(ParExpr);
public:
	explicit ParExpr(ExprPtr &&arg) : arg(std::move(arg)) {}
	ExprPtr arg;
};

class Block : public Stmt
{
	AST_NODE(Block);
public:
	std::vector<StmtPtr> children;
};

class ControlFlow : public Stmt
{};

class Return : public ControlFlow
{
	AST_NODE(Return);
public:
	explicit Return(ExprPtr &&value) : value(std::move(value)) {}
	ExprPtr value;
};

class Loop : public ControlFlow
{
public:
	ExprPtr cond;
	StmtPtr body;
};
typedef std::unique_ptr<Loop> LoopPtr;

class ForLoop : public Loop
{
	AST_NODE(ForLoop);
public:
	StmtPtr init;
	ExprPtr incr;
};

class RangeLoop : public Loop
{
	AST_NODE(RangeLoop);
public:
	std::string var;
	bool scoped_var = false;
};

class DoLoop : public Loop
{
	AST_NODE(DoLoop);
};

class WhileLoop : public Loop
{
	AST_NODE(WhileLoop);
};

class LoopControl : public ControlFlow
{};

class Break : public LoopControl
{
	AST_NODE(Break);
};

class Continue : public LoopControl
{
	AST_NODE(Continue);
};

class If : public ControlFlow
{
	AST_NODE(If);
public:
	ExprPtr cond;
	StmtPtr iftrue, iffalse;
};

class Decl : public Stmt
{};
typedef std::unique_ptr<Decl> DeclPtr;

class VarDecl : public Decl
{
	AST_NODE(VarDecl);
public:
	enum class Scope
	{
		Func,
		Object,
		Global
	};

	Scope scope;
	bool constant;

	struct Var
	{
		std::string name;
		ExprPtr init;
	};
	std::vector<Var> decls;
};

class Function
{
public:
	struct Parameter
	{
		std::string name;
		C4V_Type type;
		explicit Parameter(const std::string &name, C4V_Type type = C4V_Any) : name(name), type(type) {}
	};
	std::vector<Parameter> params;
	bool has_unnamed_params = false;
	std::unique_ptr<Block> body;

	virtual ~Function() = default;
	virtual void accept(::aul::AstVisitor *v) const = 0;
};

class FunctionDecl : public Decl, public Function
{
	AST_NODE(FunctionDecl);
public:
	explicit FunctionDecl(const std::string &name) : name(name) {}
	std::string name;
	bool is_global = false;
};

class FunctionExpr : public Expr, public Function
{
	// This node is used for constant proplists
	AST_NODE(FunctionExpr);
public:
};

class Pragma : public Decl
{};

class IncludePragma : public Pragma
{
	AST_NODE(IncludePragma);
public:
	explicit IncludePragma(const std::string &what) : what(what) {}
	std::string what;
};

class AppendtoPragma : public Pragma
{
	AST_NODE(AppendtoPragma);
public:
	AppendtoPragma() = default;
	explicit AppendtoPragma(const std::string &what) : what(what) {}
	std::string what;
};

class Script : public Node
{
	AST_NODE(Script);
public:
	std::vector<DeclPtr> declarations;
};

#undef AST_NODE

}}

namespace aul {
// A recursive visitor that visits the children of all nodes. Override the visit() functions you're interested in in child classes.
class DefaultRecursiveVisitor : public AstVisitor
{
public:
	virtual ~DefaultRecursiveVisitor() {}

	using AstVisitor::visit;

	virtual void visit(const ::aul::ast::ArrayLit *n) override
	{
		for (const auto &c : n->values)
			c->accept(this);
	}
	virtual void visit(const ::aul::ast::ProplistLit *n) override
	{
		for (const auto &c : n->values)
			c.second->accept(this);
	}
	virtual void visit(const ::aul::ast::UnOpExpr *n) override
	{
		n->operand->accept(this);
	}
	virtual void visit(const ::aul::ast::BinOpExpr *n) override
	{
		n->lhs->accept(this);
		n->rhs->accept(this);
	}
	virtual void visit(const ::aul::ast::AssignmentExpr *n) override
	{
		n->lhs->accept(this);
		n->rhs->accept(this);
	}
	virtual void visit(const ::aul::ast::SubscriptExpr *n) override
	{
		n->object->accept(this);
		n->index->accept(this);
	}
	virtual void visit(const ::aul::ast::SliceExpr *n) override
	{
		n->object->accept(this);
		n->start->accept(this);
		n->end->accept(this);
	}
	virtual void visit(const ::aul::ast::CallExpr *n) override
	{
		if (n->context)
			n->context->accept(this);
		for (const auto &a : n->args)
			a->accept(this);
	}
	virtual void visit(const ::aul::ast::ParExpr *n) override
	{
		n->arg->accept(this);
	}
	virtual void visit(const ::aul::ast::Block *n) override
	{
		for (const auto &s : n->children)
			s->accept(this);
	}
	virtual void visit(const ::aul::ast::Return *n) override
	{
		n->value->accept(this);
	}
	virtual void visit(const ::aul::ast::ForLoop *n) override
	{
		if (n->init)
			n->init->accept(this);
		if (n->cond)
			n->cond->accept(this);
		if (n->incr)
			n->incr->accept(this);
		n->body->accept(this);
	}
	virtual void visit(const ::aul::ast::RangeLoop *n) override
	{
		n->cond->accept(this);
		n->body->accept(this);
	}
	virtual void visit(const ::aul::ast::DoLoop *n) override
	{
		n->body->accept(this);
		n->cond->accept(this);
	}
	virtual void visit(const ::aul::ast::WhileLoop *n) override
	{
		n->cond->accept(this);
		n->body->accept(this);
	}
	virtual void visit(const ::aul::ast::If *n) override
	{
		n->cond->accept(this);
		n->iftrue->accept(this);
		if (n->iffalse)
			n->iffalse->accept(this);
	}
	virtual void visit(const ::aul::ast::VarDecl *n) override
	{
		for (const auto &d : n->decls)
			if (d.init)
				d.init->accept(this);
	}
	virtual void visit(const ::aul::ast::FunctionDecl *n) override
	{
		n->body->accept(this);
	}
	virtual void visit(const ::aul::ast::FunctionExpr *n) override
	{
		n->body->accept(this);
	}
	virtual void visit(const ::aul::ast::Script *n) override
	{
		for (const auto &d : n->declarations)
			d->accept(this);
	}
};
}

#endif
