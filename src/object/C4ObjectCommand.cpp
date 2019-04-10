/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Logic for C4Object: Commands */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4ObjectCom.h"


void C4Object::ClearCommands()
{
	C4Command *pNext;
	while (Command)
	{
		pNext=Command->Next;
		if (!Command->iExec)
			delete Command;
		else
			Command->iExec = 2;
		Command=pNext;
	}
}

void C4Object::ClearCommand(C4Command *pUntil)
{
	C4Command *pCom,*pNext;
	for (pCom=Command; pCom; pCom=pNext)
	{
		// Last one to clear
		if (pCom==pUntil) pNext=nullptr;
		// Next one to clear after this
		else pNext=pCom->Next;
		Command=pCom->Next;
		if (!pCom->iExec)
			delete pCom;
		else
			pCom->iExec = 2;
	}
}

bool C4Object::AddCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy,
                          int32_t iUpdateInterval, C4Object *pTarget2,
                          bool fInitEvaluation, C4Value iData, bool fAppend,
                          int32_t iRetries, C4String *szText, int32_t iBaseMode)
{
	// Command stack size safety
	const int32_t MaxCommandStack = 35;
	C4Command *pCom,*pLast; int32_t iCommands;
	for (pCom=Command,iCommands=0; pCom; pCom=pCom->Next,iCommands++) {}
	if (iCommands>=MaxCommandStack) return false;
	// Valid command safety
	if (!Inside(iCommand,C4CMD_First,C4CMD_Last)) return false;
	// Allocate and set new command
	if (!(pCom=new C4Command)) return false;
	pCom->Set(iCommand,this,pTarget,iTx,iTy,pTarget2,iData,
	          iUpdateInterval,!fInitEvaluation,iRetries,szText,iBaseMode);
	// Append to bottom of stack
	if (fAppend)
	{
		for (pLast=Command; pLast && pLast->Next; pLast=pLast->Next) {}
		if (pLast) pLast->Next=pCom;
		else Command=pCom;
	}
	// Add to top of command stack
	else
	{
		pCom->Next=Command;
		Command=pCom;
	}
	// Success
	return true;
}

void C4Object::SetCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy,
                          C4Object *pTarget2, bool fControl, C4Value iData,
                          int32_t iRetries, C4String *szText)
{
	// Clear stack
	ClearCommands();
	// Close menu
	if (fControl)
		if (!CloseMenu(false)) return;
	// Script overload
	if (fControl)
		if (!!Call(PSF_ControlCommand,&C4AulParSet(CommandName(iCommand),
		           pTarget,
		           iTx,
		           iTy,
		           pTarget2,
		           iData)))
			return;
	// Inside vehicle control overload
	if (Contained)
		if (Contained->Def->VehicleControl & C4D_VehicleControl_Inside)
		{
			Contained->Controller=Controller;
			if (!!Contained->Call(PSF_ControlCommand,&C4AulParSet(CommandName(iCommand),
			                      pTarget,
			                      iTx,
			                      iTy,
			                      pTarget2,
			                      iData,
			                      this)))
				return;
		}
	// Outside vehicle control overload
	if (GetProcedure()==DFA_PUSH)
		if (Action.Target)  if (Action.Target->Def->VehicleControl & C4D_VehicleControl_Outside)
			{
				Action.Target->Controller=Controller;
				if (!!Action.Target->Call(PSF_ControlCommand,&C4AulParSet(CommandName(iCommand),
				                          pTarget,
				                          iTx,
				                          iTy,
				                          pTarget2,
				                          iData)))
					return;
			}
	// Add new command
	AddCommand(iCommand,pTarget,iTx,iTy,0,pTarget2,true,iData,false,iRetries,szText,C4CMD_Mode_Base);
}

C4Command *C4Object::FindCommand(int32_t iCommandType) const
{
	// seek all commands
	for (C4Command *pCom = Command; pCom; pCom=pCom->Next)
		if (pCom->Command == iCommandType) return pCom;
	// nothing found
	return nullptr;
}

bool C4Object::ExecuteCommand()
{
	// Execute first command
	if (Command) Command->Execute();
	// Command finished: engine call
	if (Command && Command->Finished)
		Call(PSF_ControlCommandFinished,&C4AulParSet(CommandName(Command->Command), Command->Target, Command->Tx, Command->Ty, Command->Target2, Command->Data));
	// Clear finished commands
	while (Command && Command->Finished) ClearCommand(Command);
	// Done
	return true;
}

bool C4Object::PutAwayUnusedObject(C4Object *pToMakeRoomForObject) // Called by GetTryEnter
{
	// get unused object
	C4Object *pUnusedObject;
	C4AulFunc *pFnObj2Drop = GetFunc(PSF_GetObject2Drop);
	if (pFnObj2Drop)
		pUnusedObject = pFnObj2Drop->Exec(this, &C4AulParSet(pToMakeRoomForObject)).getObj();
	else
	{
		// is there any unused object to put away?
		if (!Contents.GetLastObject()) return false;
		// defaultly, it's the last object in the list
		// (contents list cannot have invalid status-objects)
		pUnusedObject = Contents.GetLastObject();
	}
	// no object to put away? fail
	if (!pUnusedObject) return false;
	// grabbing something?
	bool fPushing = (GetProcedure()==DFA_PUSH);
	if (fPushing)
		// try to put it in there
		if (ObjectComPut(this, Action.Target, pUnusedObject))
			return true;
	// in container? put in there
	if (Contained)
	{
		// try to put it in directly
		// note that this works too, if an object is grabbed inside the container
		if (ObjectComPut(this, Contained, pUnusedObject))
			return true;
		// now putting didn't work - drop it outside
		AddCommand(C4CMD_Drop, pUnusedObject);
		AddCommand(C4CMD_Exit);
		return true;
	}
	else
		// if uncontained, simply try to drop it
		// if this doesn't work, it won't ever
		return !!ObjectComDrop(this, pUnusedObject);
}
