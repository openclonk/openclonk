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

#include "script/C4Value.h"

enum C4AulBCCType : int;

class C4AulCompiler
{
public:
	C4AulScriptFunc *Fn;
	bool at_jump_target = false;
	int stack_height = 0;

	int AddBCC(const char * SPos, C4AulBCCType eType, intptr_t X = 0);
	void ErrorOut(const char * SPos, class C4AulError & e);
	void RemoveLastBCC();
	C4V_Type GetLastRetType(C4AulScriptEngine * Engine, C4V_Type to); // for warning purposes

	int AddVarAccess(const char * TokenSPos, C4AulBCCType eType, intptr_t varnum);
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
	Loop *active_loops = NULL;

	void PushLoop();
	void PopLoop(int ContinueJump);
	void AddLoopControl(const char * SPos, bool fBreak);
	~C4AulCompiler()
	{
		while (active_loops) PopLoop(0);
	}
};

#endif
