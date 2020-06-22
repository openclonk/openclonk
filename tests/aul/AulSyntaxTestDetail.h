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

// A lot of ugly helper code to
//   a) check whether two ASTs are the same
//   b) format an AST into human-readable format in case they aren't

#ifndef INC_AulSyntaxTestDetail
#define INC_AulSyntaxTestDetail

#include <ostream>
#include <assert.h>
#include "script/C4AulAST.h"
#include "script/C4AulParse.h"

class AstFormattingVisitor : public ::aul::AstVisitor
{
	std::ostream &target;
public:
	AstFormattingVisitor(std::ostream &target) : target(target)
	{}

	virtual void visit(const ::aul::ast::Noop *) override
	{
		target << "no-op";
	}
	virtual void visit(const ::aul::ast::StringLit *n) override
	{
		target << "\"" << n->value << "\"";
	}
	virtual void visit(const ::aul::ast::IntLit *n) override
	{
		target << n->value;
	}
	virtual void visit(const ::aul::ast::BoolLit *n) override
	{
		target << (n->value ? "true" : "false");
	}
	virtual void visit(const ::aul::ast::ArrayLit *n) override
	{
		target << "(array";
		for (auto &v : n->values)
		{
			target << " ";
			v->accept(this);
		}
		target << ")";
	}
	virtual void visit(const ::aul::ast::ProplistLit *n) override
	{
		target << "(proplist";
		for (auto &v : n->values)
		{
			target << " (\"" << v.first << "\" ";
			v.second->accept(this);
			target << ")";
		}
		target << ")";
	}
	virtual void visit(const ::aul::ast::NilLit *) override
	{
		target << "nil";
	}
	virtual void visit(const ::aul::ast::ThisLit *) override
	{
		target << "this";
	}
	virtual void visit(const ::aul::ast::VarExpr *n) override
	{
		target << "(var-expr " << n->identifier << ")";
	}
	virtual void visit(const ::aul::ast::UnOpExpr *n) override
	{
		target << "(" << C4ScriptOpMap[n->op].Identifier << " ";
		if (C4ScriptOpMap[n->op].Postfix)
			target << "postfix ";
		n->operand->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::BinOpExpr *n) override
	{
		target << "(" << C4ScriptOpMap[n->op].Identifier << " ";
		n->lhs->accept(this);
		target << " ";
		n->rhs->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::AssignmentExpr *n) override
	{
		target << "(= ";
		n->lhs->accept(this);
		target << " ";
		n->rhs->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::SubscriptExpr *n) override
	{
		target << "(subscript ";
		n->object->accept(this);
		target << " ";
		n->index->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::SliceExpr *n) override
	{
		target << "(slice ";
		n->object->accept(this);
		target << " ";
		n->start->accept(this);
		target << " ";
		n->end->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::CallExpr *n) override
	{
		target << "(";
		if (n->safe_call)
			target << "safe-";
		target << "call";
		if (n->context)
		{
			target << "-with-context";
			n->context->accept(this);
		}
		target << " (args";
		for (auto &v : n->args)
		{
			target << " ";
			v->accept(this);
		}
		target << ")";
		if (n->append_unnamed_pars)
			target << " append-unnamed";
		target << ")";
	}
	virtual void visit(const ::aul::ast::ParExpr *n) override
	{
		target << "(par ";
		n->arg->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::Block *n) override
	{
		target << "(block";
		for (auto &v : n->children)
		{
			target << " ";
			v->accept(this);
		}
		target << ")";
	}
	virtual void visit(const ::aul::ast::Return *n) override
	{
		if (n->value)
		{
			target << "(return ";
			n->value->accept(this);
			target << ")";
		}
		else
		{
			target << "(return)";
		}
	}
	virtual void visit(const ::aul::ast::ForLoop *n) override
	{
		target << "(for";
		if (n->init)
		{
			target << " (init ";
			n->init->accept(this);
			target << ")";
		}
		if (n->cond)
		{
			target << " (cond ";
			n->cond->accept(this);
			target << ")";
		}
		if (n->incr)
		{
			target << " (incr ";
			n->incr->accept(this);
			target << ")";
		}
		target << " ";
		n->body->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::RangeLoop *n) override
	{
		target << "(for-in";
		if (n->scoped_var)
			target << "-with-scope";
		target << " \"" << n->var << "\" ";
		n->cond->accept(this);
		target << " ";
		n->body->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::DoLoop *n) override
	{
		target << "(do ";
		n->cond->accept(this);
		target << " ";
		n->body->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::WhileLoop *n) override
	{
		target << "(while ";
		n->cond->accept(this);
		target << " ";
		n->body->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::Break *n) override
	{
		target << "break";
	}
	virtual void visit(const ::aul::ast::Continue *n) override
	{
		target << "continue";
	}
	virtual void visit(const ::aul::ast::If *n) override
	{
		target << "(if ";
		n->cond->accept(this);
		target << " ";
		n->iftrue->accept(this);
		if (n->iffalse)
		{
			target << " ";
			n->iffalse->accept(this);
		}
		target << ")";
	}
	virtual void visit(const ::aul::ast::VarDecl *n) override
	{
		target << "(var-decl ";
		if (n->constant)
			target << "const ";
		switch (n->scope)
		{
		case ::aul::ast::VarDecl::Scope::Func:
			target << "func-scope"; break;
		case ::aul::ast::VarDecl::Scope::Object:
			target << "obj-scope"; break;
		case ::aul::ast::VarDecl::Scope::Global:
			target << "global-scope"; break;
		}
		for (auto &d : n->decls)
		{
			target << " (" << d.name;
			if (d.init)
			{
				target << " ";
				d.init->accept(this);
			}
			target << ")";
		}
		target << ")";
	}
	virtual void visit(const ::aul::ast::FunctionDecl *n) override
	{
		target << "(func-decl " << n->name << " (";
		for (auto &p : n->params)
		{
			target << "(" << GetC4VName(p.type) << " " << p.name << ")";
		}
		if (n->has_unnamed_params)
			target << " variable-args";
		n->body->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::FunctionExpr *n) override
	{
		target << "(func-expr " << " (";
		for (auto &p : n->params)
		{
			target << "(" << GetC4VName(p.type) << " " << p.name << ")";
		}
		if (n->has_unnamed_params)
			target << " variable-args";
		n->body->accept(this);
		target << ")";
	}
	virtual void visit(const ::aul::ast::IncludePragma *n) override
	{
		target << "(include-pragma \"" << n->what << "\")";
	}
	virtual void visit(const ::aul::ast::AppendtoPragma *n) override
	{
		target << "(appendto-pragma \"" << n->what << "\")";
	}
	virtual void visit(const ::aul::ast::Script *n) override
	{
		target << "(script";
		for (auto &d : n->declarations)
		{
			target << " ";
			d->accept(this);
		}
		target << ")";
	}
};

// These templates use the above formatter to write an AST as a human-readable
// expression to the output if a test fails, instead of the default which is a
// hex memory dump
template<class T>
std::enable_if_t<std::is_base_of<::aul::ast::Node, T>::value, std::ostream &>
operator<<(::std::ostream &os, const T &node)
{
	AstFormattingVisitor v(os);
	node.accept(&v);
	return os;
}
template<class T>
std::enable_if_t<std::is_base_of<::aul::ast::Node, T>::value, std::ostream &>
operator<<(::std::ostream &os, const std::unique_ptr<T> &node)
{
	return os << *node;
}
template<class T>
std::enable_if_t<std::is_base_of<::aul::ast::Node, T>::value, std::ostream &>
operator<<(::std::ostream &os, const std::reference_wrapper<T> &node)
{
	return os << node.get();
}

static bool MatchesAstImpl(const ::aul::ast::Node *a_, const ::aul::ast::Node *b_);
template<class T, class U>
static bool MatchesAstImpl(const std::unique_ptr<T> &a_, const std::unique_ptr<U> &b_)
{
	return MatchesAstImpl(a_.get(), b_.get());
}
template<class T>
static bool MatchesAstImpl(const std::unique_ptr<T> &a_, const ::aul::ast::Node *b_)
{
	return MatchesAstImpl(a_.get(), b_);
}
template<class U>
static bool MatchesAstImpl(const ::aul::ast::Node *a_, const std::unique_ptr<U> &b_)
{
	return MatchesAstImpl(a_, b_.get());
}
static bool MatchesAstImpl(const ::aul::ast::Node *a_, const ::aul::ast::Node *b_)
{
	// It would be real nice if C++ had proper multimethods, but alas it
	// does not.
	// Since this method is only used in testing, the overhead of all
	// the dynamic_cast'ing we're doing here should be fine.

	// If a and b are both nullptr, they match.
	if (a_ == nullptr && b_ == nullptr) return true;
	// If one of a and b is a nullptr, but not the other, they don't match.
	if ((a_ == nullptr) != (b_ == nullptr)) return false;

	// If a and b are not of the same (dynamic) type, they don't match.
	if (typeid(*a_) != typeid(*b_)) return false;

	// Ok this is ugly as sin but I don't think we can do it any cleaner
	// without adding specialized acceptors to the AST nodes.
	// We're dynamic_cast'ing both nodes to the expected type, test the
	// result to make sure the cast succeeded, then run the body, then
	// set a and b to nullptr to break out of the loop.
	// The body gets a and b cast to the expected type instead of the base
	// Node*, so we can check all members without additional, explicit
	// casting.
#define WHEN(type) for (const type *a = dynamic_cast<const type*>(a_), *b = dynamic_cast<const type*>(b_); a && b; a = b = nullptr)

	// The base (non-composite) literals all just compare values, but since
	// they're different types we can't just use one common case.
	WHEN(::aul::ast::StringLit)
	{
		return a->value == b->value;
	}
	WHEN(::aul::ast::IntLit)
	{
		return a->value == b->value;
	}
	WHEN(::aul::ast::BoolLit)
	{
		return a->value == b->value;
	}

	// nil and this don't have any values to compare, so just checking type
	// is sufficient
	WHEN(::aul::ast::NilLit)
	{
		return true;
	}
	WHEN(::aul::ast::ThisLit)
	{
		return true;
	}

	// No-ops don't have anything to compare either
	WHEN(::aul::ast::Noop)
	{
		return true;
	}

	// Array literals need to compare all entries recursively
	WHEN(::aul::ast::ArrayLit)
	{
		return std::equal(a->values.begin(), a->values.end(), b->values.begin(), b->values.end(), [](const auto &a0, const auto &b0)
		{
			return MatchesAstImpl(a0, b0);
		});
	}

	// Proplist literals need to compare all entries by key and value
	WHEN(::aul::ast::ProplistLit)
	{
		return std::equal(a->values.begin(), a->values.end(), b->values.begin(), b->values.end(), [](const auto &a0, const auto &b0)
		{
			if (a0.first != b0.first)
				return false;
			return MatchesAstImpl(a0.second, b0.second);
		});
	}

	// Operators need to have matching opcodes and LHS/RHS
	WHEN(::aul::ast::UnOpExpr)
	{
		return a->op == b->op
			&& MatchesAstImpl(a->operand, b->operand);
	}
	WHEN(::aul::ast::BinOpExpr)
	{
		return a->op == b->op
			&& MatchesAstImpl(a->lhs, b->lhs)
			&& MatchesAstImpl(a->rhs, b->rhs);
	}
	WHEN(::aul::ast::AssignmentExpr)
	{
		return MatchesAstImpl(a->lhs, b->lhs)
			&& MatchesAstImpl(a->rhs, b->rhs);
	}

	// Variable expressions just need to reference the same identifier
	WHEN(::aul::ast::VarExpr)
	{
		return a->identifier == b->identifier;
	}

	// Subscript expressions need to have the same object and index
	WHEN(::aul::ast::SubscriptExpr)
	{
		return MatchesAstImpl(a->index, b->index)
			&& MatchesAstImpl(a->object, b->object);
	}

	// Slice expressions need to have the same base object and start/end indices
	WHEN(::aul::ast::SliceExpr)
	{
		return MatchesAstImpl(a->object, b->object)
			&& MatchesAstImpl(a->start, b->start)
			&& MatchesAstImpl(a->end, b->end);
	}

	// Call expressions need to match safety, context, identifier and args
	// (including unnamed arg passthrough)
	WHEN(::aul::ast::CallExpr)
	{
		if (!(a->safe_call == b->safe_call
			&& a->append_unnamed_pars == b->append_unnamed_pars
			&& MatchesAstImpl(a->context, b->context)
			&& a->callee == b->callee))
			return false;


		return std::equal(a->args.begin(), a->args.end(), b->args.begin(), b->args.end(), [](const auto &a0, const auto &b0)
		{
			return MatchesAstImpl(a0, b0);
		});
	}

	// Par() expressions need the same index
	WHEN(::aul::ast::ParExpr)
	{
		return MatchesAstImpl(a->arg, b->arg);
	}

	// Blocks need to have the same children
	WHEN(::aul::ast::Block)
	{
		return std::equal(a->children.begin(), a->children.end(), b->children.begin(), b->children.end(), [](const auto &a0, const auto &b0)
		{
			return MatchesAstImpl(a0, b0);
		});
	}

	// Return statements need to have the same parameters
	WHEN(::aul::ast::Return)
	{
		return MatchesAstImpl(a->value, b->value);
	}

	// for loops need to have the same initializer, condition, incrementor,
	// and body
	WHEN(::aul::ast::ForLoop)
	{
		return MatchesAstImpl(a->init, b->init)
			&& MatchesAstImpl(a->cond, b->cond)
			&& MatchesAstImpl(a->incr, b->incr)
			&& MatchesAstImpl(a->body, b->body);
	}

	// range loops need to have the same scoping, loop variable, target object,
	// and body
	WHEN(::aul::ast::RangeLoop)
	{
		return a->scoped_var == b->scoped_var
			&& a->var == b->var
			&& MatchesAstImpl(a->cond, b->cond)
			&& MatchesAstImpl(a->body, b->body);
	}

	// do and while loops need to have the same condition and body
	WHEN(::aul::ast::Loop)
	{
		assert(typeid(*a) == typeid(::aul::ast::DoLoop) || typeid(*a) == typeid(::aul::ast::WhileLoop));
		return MatchesAstImpl(a->cond, b->cond)
			&& MatchesAstImpl(a->body, b->body);
	}

	// break and continue just need to have the same type
	WHEN(::aul::ast::LoopControl)
	{
		return true;
	}

	// if-else needs to have the same condition, then-branch and else-branch
	WHEN(::aul::ast::If)
	{
		return MatchesAstImpl(a->cond, b->cond)
			&& MatchesAstImpl(a->iftrue, b->iftrue)
			&& MatchesAstImpl(a->iffalse, b->iffalse);
	}

	// variable declarations need to have the same scope, constancy, and
	// for each declaration have the same identifier and initializer
	WHEN(::aul::ast::VarDecl)
	{
		if (a->scope != b->scope || a->constant != b->constant)
			return false;

		return std::equal(a->decls.begin(), a->decls.end(), b->decls.begin(), b->decls.end(), [](const auto &a0, const auto &b0)
		{
			return a0.name == b0.name
				&& MatchesAstImpl(a0.init, b0.init);
		});
	}

	// function declarations need to have the same name, and scope,
	// plus the common function parts
	WHEN(::aul::ast::FunctionDecl)
	{
		if (a->name != b->name)
			return false;
		if (a->is_global != b->is_global)
			return false;
		// but keep checking
	}
	// all functions (declarations and expressions) need to have the same
	// parameter list and body
	WHEN(::aul::ast::Function)
	{
		if (a->has_unnamed_params != b->has_unnamed_params)
			return false;
		return std::equal(a->params.begin(), a->params.end(), b->params.begin(), b->params.end(), [](const auto &a0, const auto &b0)
			{
				return a0.name == b0.name
					&& a0.type == b0.type;
			})
			&& MatchesAstImpl(a->body, b->body);
	}
	// include and appendto pragmas need to include/appendto the same identifier
	WHEN(::aul::ast::IncludePragma)
	{
		return a->what == b->what;
	}
	WHEN(::aul::ast::AppendtoPragma)
	{
		return a->what == b->what;
	}
	// scripts need to have the same list of declarations
	WHEN(::aul::ast::Script)
	{
		return std::equal(a->declarations.begin(), a->declarations.end(), b->declarations.begin(), b->declarations.end(), [](const auto &a0, const auto &b0)
		{
			return MatchesAstImpl(a0, b0);
		});
	}

	assert(!"AST matching fell through to the default case");
	return false;
#undef WHEN
}

// helper templates to turn a pointer, unique_ptr or reference to T
// into an unconditional reference to T. We're using this so we only
// have to handle references in MatchesAst instead of requiring
// several overloads.
template<class T>
static const T &deref(const std::unique_ptr<T> &p)
{
	return *p;
}
template<class T>
static const T &deref(const std::reference_wrapper<T> &p)
{
	return p;
}
template<class T>
static const T &deref(const T *p)
{
	return *p;
}
template<class T>
static const T &deref(const T &p)
{
	return p;
}

// The actual matcher. Just delegates to the recursive function above.
MATCHER_P(MatchesAstP, ast, "")
{
	return MatchesAstImpl(&deref(arg), &deref(ast));
}
// And a convenience wrapper that stores the AST we're matching against
// in a std::reference_wrapper because we can't copy ASTs, but GTest
// requires that. Don't keep the matcher around longer than the AST or
// things will go sour.
template<class T>
auto MatchesAst(const T &t)
{
	return MatchesAstP(std::cref(t));
}

#endif
