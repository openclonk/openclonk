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

#ifndef INC_C4AulCompiler
#define INC_C4AulCompiler

#include "script/C4AulAST.h"

class C4AulCompiler
{
public:
	static void Compile(C4AulScriptFunc *out, const ::aul::ast::Function *f);

	static void Preparse(C4ScriptHost *out, C4ScriptHost *source, const ::aul::ast::Script *s);
	static void Compile(C4ScriptHost *out, C4ScriptHost *source, const ::aul::ast::Script *s);

private:
	class ConstexprEvaluator;
	class ConstantResolver;
	class PreparseAstVisitor;
	class CodegenAstVisitor;
};

#endif
