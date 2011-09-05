/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2001, 2006  Peter Wortmann
 * Copyright (c) 2001-2002, 2004-2007, 2010  Sven Eberhardt
 * Copyright (c) 2004, 2006-2007, 2009-2011  Günther Brammer
 * Copyright (c) 2005, 2009-2010  Armin Burgmeier
 * Copyright (c) 2009-2010  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>

#include <C4Aul.h>
#include <C4AulDefFunc.h>
#include <C4Command.h>
#include <C4DefList.h>
#include <C4GameMessage.h>
#include <C4GraphicsResource.h>
#include <C4Material.h>
#include <C4MeshAnimation.h>
#include <C4ObjectCom.h>
#include <C4ObjectInfo.h>
#include <C4ObjectMenu.h>
#include <C4Player.h>
#include <C4PlayerList.h>
#include <C4Random.h>
#include <C4RankSystem.h>
#include <C4Teams.h>

static bool FnChangeDef(C4AulObjectContext *cthr, C4ID to_id)
{
	return !!cthr->Obj->ChangeDef(to_id);
}

static C4Void FnSetSolidMask(C4AulObjectContext *cthr, long iX, long iY, long iWdt, long iHgt, long iTX, long iTY)
{
	cthr->Obj->SetSolidMask(iX,iY,iWdt,iHgt,iTX,iTY);
	return C4Void();
}

static C4Void FnDeathAnnounce(C4AulObjectContext *cthr)
{
	const long MaxDeathMsg=7;
	if (Game.C4S.Head.Film)
		return C4Void();
	// Check if crew member has an own death message
	if (cthr->Obj->Info && *(cthr->Obj->Info->DeathMessage))
	{
		GameMsgObject(cthr->Obj->Info->DeathMessage, cthr->Obj);
	}
	else
	{
		char idDeathMsg[128+1]; sprintf(idDeathMsg, "IDS_OBJ_DEATH%d", 1 + SafeRandom(MaxDeathMsg));
		GameMsgObject(FormatString(LoadResStr(idDeathMsg), cthr->Obj->GetName()).getData(), cthr->Obj);
	}
	return C4Void();
}

static bool FnGrabContents(C4AulObjectContext *cthr, C4Object *from)
{
	if (!from) return false;
	if (cthr->Obj == from) return false;
	cthr->Obj->GrabContents(from);
	return true;
}

static bool FnPunch(C4AulObjectContext *cthr, C4Object *target, long punch)
{
	if (!target) return false;
	return !!ObjectComPunch(cthr->Obj,target,punch);
}

static bool FnKill(C4AulContext *cthr, C4Object *pObj, bool fForced)
{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	if (!pObj->GetAlive()) return false;
	// Trace kills by player-owned objects
	// Do not trace for NO_OWNER, because that would include e.g. the Suicide-rule
	if (cthr->Obj && ValidPlr(cthr->Obj->Controller)) pObj->UpdatLastEnergyLossCause(cthr->Obj->Controller);
	// Do the kill
	pObj->AssignDeath(!!fForced);
	return true;
}

static C4Void FnFling(C4AulObjectContext *cthr, long iXDir, long iYDir, long iPrec, bool fAddSpeed)
{
	if (!iPrec) iPrec=1;
	cthr->Obj->Fling(itofix(iXDir, iPrec),itofix(iYDir, iPrec),fAddSpeed);
	// unstick from ground, because Fling command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cthr->Obj->Action.t_attach=0;
	return C4Void();
}

static bool FnJump(C4AulObjectContext *cthr)
{
	return !!ObjectComJump(cthr->Obj);
}

static bool FnEnter(C4AulObjectContext *cthr, C4Object *pTarget)
{
	return !!cthr->Obj->Enter(pTarget,true,true,NULL);
}

static bool FnExit(C4AulObjectContext *cthr, long tx, long ty, long tr, long txdir, long tydir, long trdir)
{
	tx+=cthr->Obj->GetX();
	ty+=cthr->Obj->GetY();
	ObjectComCancelAttach(cthr->Obj);
	return !!cthr->Obj->Exit(tx,
	                         ty+cthr->Obj->Shape.y,
	                         tr,
	                         itofix(txdir),itofix(tydir),
	                         itofix(trdir) / 10);
}

static bool FnCollect(C4AulObjectContext *cthr, C4Object *pItem, bool ignoreOCF)
{
	// local call / safety
	if (!pItem) return false;
	// check OCF of collector (MaxCarry)
	if ((cthr->Obj->OCF & OCF_Collection) || ignoreOCF)
		// collect
		return !!cthr->Obj->Collect(pItem);
	// failure
	return false;
}

static C4Void FnRemoveObject(C4AulObjectContext *cthr, bool fEjectContents)
{
	cthr->Obj->AssignRemoval(fEjectContents);
	return C4Void();
}

static C4Void FnSetPosition(C4AulObjectContext *cthr, long iX, long iY, bool fCheckBounds, long iPrec)
{
	if (!iPrec) iPrec = 1;
	C4Real i_x = itofix(iX, iPrec), i_y = itofix(iY, iPrec);
	if (fCheckBounds)
	{
		// BoundsCheck takes ref to C4Real and not to long
		cthr->Obj->BoundsCheck(i_x, i_y);
	}
	cthr->Obj->ForcePosition(i_x, i_y);
	// update liquid
	cthr->Obj->UpdateInLiquid();
	return C4Void();
}

static C4Void FnDoCon(C4AulObjectContext *cthr, long iChange, long iPrec)
{
	if (!iPrec) iPrec = 100;
	cthr->Obj->DoCon(FullCon*iChange/iPrec);
	return C4Void();
}

static long FnGetCon(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 100;
	return iPrec*cthr->Obj->GetCon()/FullCon;
}

static C4String *FnGetName(C4AulContext *cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetName");
		else
			return String(cthr->Def->GetName());
	else
		return String(cthr->Obj->GetName());
}

static bool FnSetName(C4AulContext *cthr, C4String *pNewName, bool fSetInInfo, bool fMakeValidIfExists)
{
	if (!cthr->Obj)
	{
		if (!cthr->Def)
			throw new NeedNonGlobalContext("SetName");
		else if (fSetInInfo)
			return false;
		// Definition name
		cthr->Def->SetName(FnStringPar(pNewName));
		return true;
	}
	else
	{
		// Object name
		if (fSetInInfo)
		{
			// setting name in info
			C4ObjectInfo *pInfo = cthr->Obj->Info;
			if (!pInfo) return false;
			const char *szName = pNewName->GetCStr();
			// empty names are bad; e.g., could cause problems in savegames
			if (!szName || !*szName) return false;
			// name must not be too long
			if (std::strlen(szName) > C4MaxName) return false;
			// any change at all?
			if (SEqual(szName, pInfo->Name)) return true;
			// make sure names in info list aren't duplicated
			// querying owner info list here isn't 100% accurate, as infos might have been stolen by other players
			// however, there is no good way to track the original list ATM
			C4ObjectInfoList *pInfoList = NULL;
			C4Player *pOwner = ::Players.Get(cthr->Obj->Owner);
			if (pOwner) pInfoList = &pOwner->CrewInfoList;
			char NameBuf[C4MaxName+1];
			if (pInfoList) if (pInfoList->NameExists(szName))
				{
					if (!fMakeValidIfExists) return false;
					SCopy(szName, NameBuf, C4MaxName);
					pInfoList->MakeValidName(NameBuf);
					szName = NameBuf;
				}
			SCopy(szName, pInfo->Name, C4MaxName);
			cthr->Obj->SetName(); // make sure object uses info name
			cthr->Obj->Call(PSF_NameChange,&C4AulParSet(C4VBool(true)));
		}
		else
		{
			if (!pNewName) cthr->Obj->SetName();
			else cthr->Obj->SetName(pNewName->GetCStr());
			cthr->Obj->Call(PSF_NameChange,&C4AulParSet(C4VBool(false)));
		}
	}
	return true;
}

static C4Value FnSetCrewExtraData(C4AulContext *cthr, C4Value *strDataName_C4V, C4Value *Data)
{
	if (!cthr->Obj)
		throw NeedObjectContext("SetCrewExtraData");

	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid crew with info? (for great nullpointer prevention)
	if (!cthr->Obj->Info) return C4Value();
	// do not allow data type C4V_Array or C4V_C4Object
	if (Data->GetType() != C4V_Nil &&
	    Data->GetType() != C4V_Int &&
	    Data->GetType() != C4V_Bool &&
	    Data->GetType() != C4V_String) return C4VNull;
	// get pointer on info...
	C4ObjectInfo *pInfo = cthr->Obj->Info;
	// no name list created yet?
	if (!pInfo->ExtraData.pNames)
		// create name list
		pInfo->ExtraData.CreateTempNameList();
	// data name already exists?
	long ival;
	if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) != -1)
		pInfo->ExtraData[ival] = *Data;
	else
	{
		// add name
		pInfo->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
		pInfo->ExtraData[ival] = *Data;
	}
	// ok, return the value that has been set
	return *Data;
}

static C4Value FnGetCrewExtraData(C4AulContext *cthr, C4Value *strDataName_C4V)
{
	if (!cthr->Obj)
		throw NeedObjectContext("GetCrewExtraData");

	const char *strDataName = FnStringPar(strDataName_C4V->getStr());
	// valid crew with info?
	if (!cthr->Obj->Info) return C4Value();
	// get pointer on info...
	C4ObjectInfo *pInfo = cthr->Obj->Info;
	// no name list?
	if (!pInfo->ExtraData.pNames) return C4Value();
	long ival;
	if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
	// return data
	return pInfo->ExtraData[ival];
}

static C4Void FnDoEnergy(C4AulObjectContext *cthr, long iChange, bool fExact, Nillable<long> iEngType, Nillable<long> iCausedBy)
{
	if (iEngType.IsNil()) iEngType = C4FxCall_EngScript;
	if (iCausedBy.IsNil())
		//if (cthr->Caller && cthr->Caller->Obj)
		//  iCausedBy = cthr->Caller->Obj->Controller;
		//else
		iCausedBy = NO_OWNER;
	cthr->Obj->DoEnergy(iChange, fExact, iEngType, iCausedBy);
	return C4Void();
}

static C4Void FnDoBreath(C4AulObjectContext *cthr, long iChange)
{
	cthr->Obj->DoBreath(iChange);
	return C4Void();
}

static C4Void FnDoDamage(C4AulObjectContext *cthr, long iChange, Nillable<long> iDmgType, Nillable<long> iCausedBy)
{
	if (iDmgType.IsNil()) iDmgType = C4FxCall_DmgScript;
	if (iCausedBy.IsNil())
		//if (cthr->Caller && cthr->Caller->Obj)
		//  iCausedBy = cthr->Caller->Obj->Controller;
		//else
		iCausedBy = NO_OWNER;
	cthr->Obj->DoDamage(iChange,iCausedBy, iDmgType);
	return C4Void();
}

static C4Void FnSetEntrance(C4AulObjectContext *cthr, bool e_status)
{
	cthr->Obj->EntranceStatus = e_status;
	return C4Void();
}


static C4Void FnSetXDir(C4AulObjectContext *cthr, long nxdir, long iPrec)
{
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update xdir
	cthr->Obj->xdir=itofix(nxdir, iPrec);
	// special: negative dirs must be rounded
	//if (nxdir<0) pObj->xdir += C4REAL100(-50)/iPrec;
	cthr->Obj->Mobile=1;
	// success
	return C4Void();
}

static C4Void FnSetRDir(C4AulObjectContext *cthr, long nrdir, long iPrec)
{
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update rdir
	cthr->Obj->rdir=itofix(nrdir, iPrec);
	// special: negative dirs must be rounded
	//if (nrdir<0) pObj->rdir += C4REAL100(-50)/iPrec;
	cthr->Obj->Mobile=1;
	// success
	return C4Void();
}

static C4Void FnSetYDir(C4AulObjectContext *cthr, long nydir, long iPrec)
{
	// precision (default 10.0)
	if (!iPrec) iPrec=10;
	// update ydir
	cthr->Obj->ydir=itofix(nydir, iPrec);
	// special: negative dirs must be rounded
	//if (nydir<0) pObj->ydir += C4REAL100(-50)/iPrec;
	cthr->Obj->Mobile=1;
	return C4Void();
}

static C4Void FnSetR(C4AulObjectContext *cthr, long nr)
{
	// set rotation
	cthr->Obj->SetRotation(nr);
	// success
	return C4Void();
}

static bool FnSetAction(C4AulObjectContext *cthr, C4String *szAction,
                        C4Object *pTarget, C4Object *pTarget2, bool fDirect)
{
	if (!szAction) return false;
	return !!cthr->Obj->SetActionByName(FnStringPar(szAction),pTarget,pTarget2,
	                                    C4Object::SAC_StartCall | C4Object::SAC_AbortCall,!!fDirect);
}

static bool FnSetBridgeActionData(C4AulObjectContext *cthr, long iBridgeLength, bool fMoveClonk, bool fWall, long iBridgeMaterial)
{
	if (!cthr->Obj->Status) return false;
	C4PropList* pActionDef = cthr->Obj->GetAction();
	// action must be BRIDGE
	if (!pActionDef) return false;
	if (pActionDef->GetPropertyP(P_Procedure) != DFA_BRIDGE) return false;
	// set data
	cthr->Obj->Action.SetBridgeData(iBridgeLength, fMoveClonk, fWall, iBridgeMaterial);
	return true;
}

static bool FnSetActionData(C4AulObjectContext *cthr, long iData)
{
	if (!cthr->Obj->Status) return false;
	C4PropList* pActionDef = cthr->Obj->GetAction();
	// bridge: Convert from old style
	if (pActionDef && (pActionDef->GetPropertyP(P_Procedure) == DFA_BRIDGE))
		return FnSetBridgeActionData(cthr, 0, false, false, iData);
	// attach: check for valid vertex indices
	if (pActionDef && (pActionDef->GetPropertyP(P_Procedure) == DFA_ATTACH))
		if (((iData&255) >= C4D_MaxVertex) || ((iData>>8) >= C4D_MaxVertex))
			return false;
	// set data
	cthr->Obj->Action.Data = iData;
	return true;
}

static C4Void FnSetComDir(C4AulObjectContext *cthr, long ncomdir)
{
	cthr->Obj->Action.ComDir=ncomdir;
	return C4Void();
}

static C4Void FnSetDir(C4AulObjectContext *cthr, long ndir)
{
	cthr->Obj->SetDir(ndir);
	return C4Void();
}

static C4Void FnSetCategory(C4AulObjectContext *cthr, long iCategory)
{
	cthr->Obj->SetCategory(iCategory);
	return C4Void();
}

static C4Void FnSetAlive(C4AulObjectContext *cthr, bool nalv)
{
	cthr->Obj->SetAlive(nalv);
	return C4Void();
}

static bool FnSetOwner(C4AulObjectContext *cthr, long iOwner)
{
	// Set owner
	return !!cthr->Obj->SetOwner(iOwner);
}

static bool FnSetPhase(C4AulObjectContext *cthr, long iVal)
{
	return !!cthr->Obj->SetPhase(iVal);
}

static bool FnExecuteCommand(C4AulObjectContext *cthr)
{
	return !!cthr->Obj->ExecuteCommand();
}

static C4Value FnSetCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(any, Data); PAR(int, iRetries);
	// Object
	if (!cthr->Obj)
		throw NeedObjectContext("SetCommand");
	// Command
	if (!szCommand) return C4VFalse;
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) { cthr->Obj->ClearCommands(); return C4VFalse; }
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		szText=Data.getStr();
	else
		Tx.ConvertTo(C4V_Int);
	// Set
	cthr->Obj->SetCommand(iCommand,pTarget,Tx,iTy,pTarget2,false,Data,iRetries,szText);
	// Success
	return C4VTrue;
}

static C4Value FnAddCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(int, iUpdateInterval); PAR(any, Data); PAR(int, iRetries); PAR(int, iBaseMode);
	// Object
	if (!cthr->Obj)
		throw NeedObjectContext("AddCommand");
	// Command
	if (!szCommand) return C4VFalse;
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		szText=Data.getStr();
	else
		Tx.ConvertTo(C4V_Int);
	// Add
	return C4VBool(cthr->Obj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,true,Data,false,iRetries,szText,iBaseMode));
}

static C4Value FnAppendCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(string, szCommand); PAR(object, pTarget); PAR(any, Tx); PAR(int, iTy);
	PAR(object, pTarget2); PAR(int, iUpdateInterval); PAR(any, Data); PAR(int, iRetries); PAR(int, iBaseMode);
	// Object
	if (!cthr->Obj)
		throw NeedObjectContext("AppendCommand");
	// Command
	if (!szCommand) return C4VFalse;
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return C4VFalse;
	// Special: convert iData to szText
	C4String *szText=NULL;
	if (iCommand==C4CMD_Call)
		szText=Data.getStr();
	else
		Tx.ConvertTo(C4V_Int);
	// Add
	return C4VBool(cthr->Obj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,true,Data,true,iRetries,szText,iBaseMode));
}

static C4Value FnGetCommand(C4AulContext *cthr, C4Value *pPars)
{
	PAR(int, iElement); PAR(int, iCommandNum);

	// safety
	if (!cthr->Obj)
		throw NeedObjectContext("GetCommand");

	C4Command * Command = cthr->Obj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return C4VNull;
	// Return command element
	switch (iElement)
	{
	case 0: // Name
		return C4VString(CommandName(Command->Command));
	case 1: // Target
		return C4VObj(Command->Target);
	case 2: // Tx
		return Command->Tx;
	case 3: // Ty
		return C4VInt(Command->Ty);
	case 4: // Target2
		return C4VObj(Command->Target2);
	case 5: // Data
		return Command->Command == C4CMD_Call ? C4VString(Command->Text) : Command->Data;
	}
	// Undefined element
	return C4VNull;
}

static bool FnFinishCommand(C4AulObjectContext *cthr, bool fSuccess, long iCommandNum)
{
	C4Command * Command = cthr->Obj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return false;
	if (!fSuccess) ++(Command->Failures);
	else Command->Finished = true;
	return true;
}

static C4String *FnGetAction(C4AulObjectContext *cthr)
{
	C4PropList* pActionDef = cthr->Obj->GetAction();
	if (!pActionDef) return String("Idle");
	return String(pActionDef->GetName());
}

static C4Object *FnGetActionTarget(C4AulObjectContext *cthr, long target_index)
{
	if (target_index==0) return cthr->Obj->Action.Target;
	if (target_index==1) return cthr->Obj->Action.Target2;
	return NULL;
}

static C4Void FnSetActionTargets(C4AulObjectContext *cthr, C4Object *pTarget1, C4Object *pTarget2)
{
	// set targets
	cthr->Obj->Action.Target=pTarget1;
	cthr->Obj->Action.Target2=pTarget2;
	return C4Void();
}

static long FnGetDir(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.Dir;
}

static bool FnGetEntrance(C4AulObjectContext *cthr)
{
	return cthr->Obj->EntranceStatus;
}

static long FnGetPhase(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.Phase;
}

static long FnGetEnergy(C4AulObjectContext *cthr)
{
	return 100*cthr->Obj->Energy/C4MaxPhysical;
}

static long FnGetBreath(C4AulObjectContext *cthr)
{
	return cthr->Obj->Breath;
}

static long FnGetMass(C4AulContext *cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetMass");
		else
			return cthr->Def->Mass;
	else
		return cthr->Obj->Mass;
}

static long FnGetRDir(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 10;
	return fixtoi(cthr->Obj->rdir, iPrec);
}

static long FnGetXDir(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 10;
	return fixtoi(cthr->Obj->xdir, iPrec);
}

static long FnGetYDir(C4AulObjectContext *cthr, long iPrec)
{
	if (!iPrec) iPrec = 10;
	return fixtoi(cthr->Obj->ydir, iPrec);
}

static long FnGetR(C4AulObjectContext *cthr)
{
	// Adjust range
	long iR = cthr->Obj->r;
	while (iR>180) iR-=360;
	while (iR<-180) iR+=360;
	return iR;
}

static long FnGetComDir(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.ComDir;
}

static long FnGetVertexNum(C4AulObjectContext *cthr)
{
	return cthr->Obj->Shape.VtxNum;
}

enum VertexDataIndex
{
	VTX_X,
	VTX_Y,
	VTX_CNAT,
	VTX_Friction
};
enum VertexUpdateMode
{
	VTX_SetNonpermanent,
	VTX_SetPermanent,
	VTX_SetPermanentUpd
};

static Nillable<long> FnGetVertex(C4AulObjectContext *cthr, long iIndex, long iValueToGet)
{
	if (cthr->Obj->Shape.VtxNum<1) return C4Void();
	iIndex=Min<long>(iIndex,cthr->Obj->Shape.VtxNum-1);
	switch (static_cast<VertexDataIndex>(iValueToGet))
	{
	case VTX_X: return cthr->Obj->Shape.VtxX[iIndex]; break;
	case VTX_Y: return cthr->Obj->Shape.VtxY[iIndex]; break;
	case VTX_CNAT: return cthr->Obj->Shape.VtxCNAT[iIndex]; break;
	case VTX_Friction: return cthr->Obj->Shape.VtxFriction[iIndex]; break;
	default:
		DebugLog(FormatString("GetVertex: Unknown vertex attribute: %ld", iValueToGet).getData());
		return C4Void();
		break;
	}
	// impossible mayhem!
	assert(!"FnGetVertex: unreachable code reached");
	return C4Void();
}

static bool FnSetVertex(C4AulObjectContext *cthr, long iIndex, long iValueToSet, long iValue, long iOwnVertexMode)
{
	// own vertex mode?
	if (iOwnVertexMode)
	{
		// enter own custom vertex mode if not already set
		if (!cthr->Obj->fOwnVertices)
		{
			cthr->Obj->Shape.CreateOwnOriginalCopy(cthr->Obj->Def->Shape);
			cthr->Obj->fOwnVertices = 1;
		}
		// set vertices at end of buffer
		iIndex += C4D_VertexCpyPos;
	}
	// range check
	if (!Inside<long>(iIndex,0,C4D_MaxVertex-1)) return false;
	// set desired value
	switch (static_cast<VertexDataIndex>(iValueToSet))
	{
	case VTX_X: cthr->Obj->Shape.VtxX[iIndex]=iValue; break;
	case VTX_Y: cthr->Obj->Shape.VtxY[iIndex]=iValue; break;
	case VTX_CNAT: cthr->Obj->Shape.VtxCNAT[iIndex]=iValue; break;
	case VTX_Friction: cthr->Obj->Shape.VtxFriction[iIndex]=iValue; break;
	default:
		DebugLogF("SetVertex: Unknown vertex attribute: %ld", iValueToSet);
		return false;
		break;
	}
	// vertex update desired?
	if (iOwnVertexMode==VTX_SetPermanentUpd) cthr->Obj->UpdateShape(true);
	return true;
}

static bool FnAddVertex(C4AulObjectContext *cthr, long iX, long iY)
{
	return !!cthr->Obj->Shape.AddVertex(iX,iY);
}

static bool FnRemoveVertex(C4AulObjectContext *cthr, long iIndex)
{
	return !!cthr->Obj->Shape.RemoveVertex(iIndex);
}

static C4Void FnSetContactDensity(C4AulObjectContext *cthr, long iDensity)
{
	cthr->Obj->Shape.ContactDensity = iDensity;
	return C4Void();
}

static bool FnGetAlive(C4AulObjectContext *cthr)
{
	return cthr->Obj->GetAlive();
}

static long FnGetOwner(C4AulObjectContext *cthr)
{
	return cthr->Obj->Owner;
}

static long FnGetController(C4AulObjectContext *cthr)
{
	return cthr->Obj->Controller;
}

static bool FnSetController(C4AulObjectContext *cthr, long iNewController)
{
	// validate player
	if (iNewController != NO_OWNER && !ValidPlr(iNewController)) return false;
	// Set controller
	cthr->Obj->Controller = iNewController;
	return true;
}

static long FnGetKiller(C4AulObjectContext *cthr)
{
	return cthr->Obj->LastEnergyLossCausePlayer;
}

static bool FnSetKiller(C4AulObjectContext *cthr, long iNewKiller)
{
	// validate player
	if (iNewKiller != NO_OWNER && !ValidPlr(iNewKiller)) return false;
	// set killer as last energy loss cause
	cthr->Obj->LastEnergyLossCausePlayer = iNewKiller;
	return true;
}

static long FnGetCategory(C4AulContext *cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetCategory");
		else
			return cthr->Def->Category;
	else
		return cthr->Obj->Category;
}

static long FnGetOCF(C4AulObjectContext *cthr)
{
	return cthr->Obj->OCF;
}

static long FnGetDamage(C4AulObjectContext *cthr)
{
	return cthr->Obj->Damage;
}

static long FnGetValue(C4AulContext *cthr, C4Object *pInBase, long iForPlayer)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetValue");
		else
			return cthr->Def->GetValue(pInBase, iForPlayer);
	else
		return cthr->Obj->GetValue(pInBase, iForPlayer);
}

static long FnGetRank(C4AulObjectContext *cthr)
{
	if (!cthr->Obj->Info) return 0;
	return cthr->Obj->Info->Rank;
}

static long FnGetActTime(C4AulObjectContext *cthr)
{
	return cthr->Obj->Action.Time;
}

static C4ID FnGetID(C4AulObjectContext *cthr)
{
	// return id of object
	return cthr->Obj->Def->id;
}

static Nillable<C4ID> FnGetMenu(C4AulObjectContext *cthr)
{
	if (cthr->Obj->Menu && cthr->Obj->Menu->IsActive())
		return C4ID(cthr->Obj->Menu->GetIdentification());
	return C4Void();
}

static bool FnCreateMenu(C4AulObjectContext *cthr, C4Def *pDef, C4Object *pCommandObj,
                         long iExtra, C4String *szCaption, long iExtraData,
                         long iStyle, bool fPermanent, C4ID idMenuID)
{
	if (pCommandObj)
		// object menu: Validate object
		if (!pCommandObj->Status) return false;
	// else scenario script callback: No command object OK

	// Create symbol
	C4FacetSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize,C4SymbolSize);
	if (pDef) pDef->Draw(fctSymbol);

	// Clear any old menu, init new menu
	if (!cthr->Obj->CloseMenu(false)) return false;
	if (!cthr->Obj->Menu) cthr->Obj->Menu = new C4ObjectMenu; else cthr->Obj->Menu->ClearItems();
	cthr->Obj->Menu->Init(fctSymbol,FnStringPar(szCaption),pCommandObj,iExtra,iExtraData,(idMenuID ? idMenuID : pDef ? pDef->id : C4ID::None).GetHandle(),iStyle,true);

	// Set permanent
	cthr->Obj->Menu->SetPermanent(fPermanent);

	return true;
}

const int C4MN_Add_ImgRank     =   1,
          C4MN_Add_ImgIndexed  =   2,
          C4MN_Add_ImgObjRank  =   3,
          C4MN_Add_ImgObject   =   4,
          C4MN_Add_ImgTextSpec =   5,
          C4MN_Add_ImgColor    =   6,
          C4MN_Add_MaxImage    = 127, // mask for param which decides what to draw as the menu symbol
          C4MN_Add_PassValue   = 128,
          C4MN_Add_ForceCount  = 256,
          C4MN_Add_ForceNoDesc = 512;

#ifndef _MSC_VER
#define _snprintf snprintf
#endif

static bool FnAddMenuItem(C4AulObjectContext *cthr, C4String * szCaption, C4String * szCommand, C4Def * pDef, int iCount, const C4Value & Parameter, C4String * szInfoCaption, int iExtra, const C4Value & XPar, const C4Value & XPar2)
{
	if (!cthr->Obj->Menu) return false;

	char caption[256+1];
	char parameter[256+1];
	char dummy[256+1];
	char command[512+1];
	char command2[512+1];
	char infocaption[C4MaxTitle+1];

	// get needed symbol size
	int iSymbolSize = cthr->Obj->Menu->GetSymbolSize();

	// Compose caption with def name
	if (szCaption)
	{
		const char * s = FnStringPar(szCaption);
		const char * sep = strstr(s, "%s");
		if (sep)
		{
			strncpy(caption, s, Min<intptr_t>(sep - s,256));
			caption[Min<intptr_t>(sep - s,256)] = 0;
			strncat(caption, pDef->GetName(), 256);
			strncat(caption, sep + 2, 256);
		}
		else
		{
			strncpy(caption, s, 256);
			caption[256] = 0;
		}
	}
	else
		caption[0] = 0;

	// create string to include type-information in command
	switch (Parameter.GetType())
	{
	case C4V_Int:
		sprintf(parameter, "%d", Parameter.getInt());
		break;
	case C4V_Bool:
		SCopy(Parameter.getBool() ? "true" : "false", parameter);
		break;
	case C4V_C4Object:
	case C4V_PropList:
		if (Parameter.getPropList()->GetObject())
			sprintf(parameter, "Object(%d)", Parameter.getPropList()->GetObject()->Number);
		else if (Parameter.getPropList()->GetDef())
			sprintf(parameter, "C4Id(\"%s\")", Parameter.getPropList()->GetDef()->id.ToString());
		else
			throw new C4AulExecError(cthr->Obj, "proplist as parameter to AddMenuItem");
		break;
	case C4V_String:
		// note this breaks if there is '"' in the string.
		parameter[0] = '"';
		SCopy(Parameter.getStr()->GetCStr(), parameter + 1, sizeof(command)-3);
		SAppendChar('"', command);
		break;
	case C4V_Nil:
		SCopy("nil", parameter);
		break;
	case C4V_Array:
		// Arrays were never allowed, so tell the scripter
		throw new C4AulExecError(cthr->Obj, "array as parameter to AddMenuItem");
	default:
		return false;
	}

	// own value
	bool fOwnValue = false; long iValue=0;
	if (iExtra & C4MN_Add_PassValue)
	{
		fOwnValue = true;
		iValue = XPar2.getInt();
	}

	// New Style: native script command
	size_t i = 0;
	for (; i < SLen(FnStringPar(szCommand)); i++)
		if (!IsIdentifier(FnStringPar(szCommand)[i]))
			break;
	if (i < SLen(FnStringPar(szCommand)))
	{
		// Search for "%d" an replace it by "%s" for insertion of formatted parameter
		SCopy(FnStringPar(szCommand), dummy, 256);
		char* pFound = const_cast<char*>(SSearch(dummy, "%d"));
		if (pFound != 0)
			*(pFound - 1) = 's';
		// Compose left-click command
		sprintf(command, dummy, parameter, 0);
		// Compose right-click command
		sprintf(command2, dummy, parameter, 1);
	}

	// Old style: function name with id and parameter
	else
	{
		const char *szScriptCom = FnStringPar(szCommand);
		if (szScriptCom && *szScriptCom)
		{
			if (iExtra & C4MN_Add_PassValue)
			{
				// with value
				sprintf(command,"%s(%s,%s,0,%ld)",szScriptCom,pDef ? pDef->id.ToString() : "nil",parameter,iValue);
				sprintf(command2,"%s(%s,%s,1,%ld)",szScriptCom,pDef ? pDef->id.ToString() : "nil",parameter,iValue);
			}
			else
			{
				// without value
				sprintf(command,"%s(%s,%s)",szScriptCom,pDef ? pDef->id.ToString() : "nil",parameter);
				sprintf(command2,"%s(%s,%s,1)",szScriptCom,pDef ? pDef->id.ToString() : "nil",parameter);
			}
		}
		else
		{
			// no command
			*command = *command2 = '\0';
		}
	}

	// Info caption
	SCopy(FnStringPar(szInfoCaption),infocaption,C4MaxTitle);
	// Default info caption by def desc
	//if (pDef && !infocaption[0] && !(iExtra & C4MN_Add_ForceNoDesc)) SCopy(pDef->GetDesc(),infocaption,C4MaxTitle);

	// Create symbol
	C4FacetSurface fctSymbol;
	switch (iExtra & C4MN_Add_MaxImage)
	{
	case C4MN_Add_ImgRank:
	{
		// symbol by rank
		C4Facet *pfctRankSym = &::GraphicsResource.fctRank;
		int32_t iRankSymNum = ::GraphicsResource.iNumRanks;
		if (pDef && pDef->pRankSymbols)
		{
			pfctRankSym = pDef->pRankSymbols;
			iRankSymNum = pDef->iNumRankSymbols;
		}
		C4RankSystem::DrawRankSymbol(&fctSymbol, iCount, pfctRankSym, iRankSymNum, true);
		iCount=0;
		break;
	}
	case C4MN_Add_ImgIndexed:
		// draw indexed facet
		fctSymbol.Create(iSymbolSize,iSymbolSize);
		if (pDef)
			pDef->Draw(fctSymbol, false, 0, NULL, XPar.getInt());
		break;
	case C4MN_Add_ImgObjRank:
	{
		// draw current gfx of XPar_C4V including rank
		if (XPar.GetType() != C4V_C4Object) return false;
		C4Object *pGfxObj = XPar.getObj();
		if (pGfxObj && pGfxObj->Status)
		{
			// create graphics
			// get rank gfx
			C4Facet *pRankRes=&::GraphicsResource.fctRank;
			long iRankCnt=::GraphicsResource.iNumRanks;
			C4Def *pDef=pGfxObj->Def;
			if (pDef->pRankSymbols)
			{
				pRankRes=pDef->pRankSymbols;
				iRankCnt=pDef->iNumRankSymbols;
			}
			// context menu
			C4Facet fctRank;
			if (cthr->Obj->Menu->IsContextMenu())
			{
				// context menu entry: left object gfx
				long C4MN_SymbolSize = cthr->Obj->Menu->GetItemHeight();
				fctSymbol.Create(C4MN_SymbolSize * 2,C4MN_SymbolSize);
				fctSymbol.Wdt = C4MN_SymbolSize;
				pGfxObj->Def->Draw(fctSymbol, false, pGfxObj->Color, pGfxObj);
				// right of it the rank
				fctRank = fctSymbol;
				fctRank.X = C4MN_SymbolSize;
				fctSymbol.Wdt *= 2;
			}
			else
			{
				// regular menu: draw object picture
				fctSymbol.Create(iSymbolSize,iSymbolSize);
				pGfxObj->Def->Draw(fctSymbol, false, pGfxObj->Color, pGfxObj);
				// rank at top-right corner
				fctRank = fctSymbol;
				fctRank.X = fctRank.Wdt - pRankRes->Wdt;
				fctRank.Wdt = pRankRes->Wdt;
				fctRank.Hgt = pRankRes->Hgt;
			}
			// draw rank
			if (pGfxObj->Info)
			{
				C4Facet fctBackup = (const C4Facet &) fctSymbol;
				fctSymbol.Set(fctRank);
				C4RankSystem::DrawRankSymbol(&fctSymbol, pGfxObj->Info->Rank, pRankRes, iRankCnt, true);
				fctSymbol.Set(fctBackup);
			}
		}
	}
	break;
	case C4MN_Add_ImgObject:
	{
		// draw object picture
		if (XPar.GetType() != C4V_C4Object)
			throw new C4AulExecError(cthr->Obj,
			                         FormatString("call to \"%s\" parameter %d: got \"%s\", but expected \"%s\"!",
			                                      "AddMenuItem", 8, XPar.GetTypeName(), GetC4VName(C4V_C4Object)
			                                     ).getData());
		C4Object *pGfxObj = XPar.getObj();
		fctSymbol.Wdt = fctSymbol.Hgt = iSymbolSize;
		pGfxObj->Picture2Facet(fctSymbol);
	}
	break;

	case C4MN_Add_ImgTextSpec:
	{
		C4FacetSurface fctSymSpec;
		uint32_t dwClr = XPar.getInt();
		if (!szCaption || !Game.DrawTextSpecImage(fctSymSpec, caption, dwClr ? dwClr : 0xff))
			return false;
		fctSymbol.Create(iSymbolSize,iSymbolSize);
		fctSymSpec.Draw(fctSymbol, true);
		*caption = '\0';
	}
	break;

	case C4MN_Add_ImgColor:
		// draw colored def facet
		fctSymbol.Create(iSymbolSize,iSymbolSize);
		if (pDef)
			pDef->Draw(fctSymbol, false, XPar.getInt());
		break;

	default:
		// default: by def, if it is not specifically NONE
		if (pDef)
		{
			fctSymbol.Create(iSymbolSize,iSymbolSize);
			pDef->Draw(fctSymbol);
		}
		else
		{
			// otherwise: Clear symbol!
		}
		break;
	}

	// Convert default zero count to no count
	if (iCount==0 && !(iExtra & C4MN_Add_ForceCount)) iCount=C4MN_Item_NoCount;

	// menuitems without commands are never selectable
	bool fIsSelectable = !!*command;

	// Add menu item
	cthr->Obj->Menu->Add(caption,fctSymbol,command,iCount,NULL,infocaption,pDef ? pDef->id : C4ID::None,command2,fOwnValue,iValue,fIsSelectable);

	return true;
}

static bool FnSelectMenuItem(C4AulObjectContext *cthr, long iItem)
{
	if (!cthr->Obj->Menu) return false;
	return !!cthr->Obj->Menu->SetSelection(iItem, false, true);
}

static bool FnSetMenuDecoration(C4AulObjectContext *cthr, C4ID idNewDeco)
{
	if (!cthr->Obj->Menu) return false;
	C4GUI::FrameDecoration *pNewDeco = new C4GUI::FrameDecoration();
	if (!pNewDeco->SetByDef(idNewDeco))
	{
		delete pNewDeco;
		return false;
	}
	cthr->Obj->Menu->SetFrameDeco(pNewDeco);
	return true;
}

static bool FnSetMenuTextProgress(C4AulObjectContext *cthr, long iNewProgress)
{
	if (!cthr->Obj->Menu) return false;
	return cthr->Obj->Menu->SetTextProgress(iNewProgress, false);
}


// Check / Status

static C4Object *FnContained(C4AulObjectContext *cthr)
{
	return cthr->Obj->Contained;
}

static C4Object *FnContents(C4AulObjectContext *cthr, long index)
{
	// Special: objects attaching to another object
	//          cannot be accessed by FnContents
	C4Object *cobj;
	while ((cobj=cthr->Obj->Contents.GetObject(index++)))
		if (cobj->GetProcedure()!=DFA_ATTACH) return cobj;

	return NULL;
}

static bool FnShiftContents(C4AulObjectContext *cthr, bool fShiftBack, C4ID idTarget, bool fDoCalls)
{
	// regular shift
	if (!idTarget) return !!cthr->Obj->ShiftContents(fShiftBack, fDoCalls);
	// check if ID is present within target
	C4Object *pNewFront = cthr->Obj->Contents.Find(idTarget);
	if (!pNewFront) return false;
	// select it
	cthr->Obj->DirectComContents(pNewFront, fDoCalls);
	// done, success
	return true;
}

static C4Object *FnScrollContents(C4AulObjectContext *cthr)
{
	C4Object *pMove = cthr->Obj->Contents.GetObject();
	if (pMove)
	{
		cthr->Obj->Contents.Remove(pMove);
		cthr->Obj->Contents.Add(pMove,C4ObjectList::stNone);
	}

	return cthr->Obj->Contents.GetObject();
}

static long FnContentsCount(C4AulObjectContext *cthr, C4ID id)
{
	return cthr->Obj->Contents.ObjectCount(id);
}

static C4Object *FnFindContents(C4AulObjectContext *cthr, C4ID c_id)
{
	return cthr->Obj->Contents.Find(c_id);
}

static C4Object *FnFindOtherContents(C4AulObjectContext *cthr, C4ID c_id)
{
	return cthr->Obj->Contents.FindOther(c_id);
}

static bool FnActIdle(C4AulObjectContext *cthr)
{
	return !cthr->Obj->GetAction();
}

static bool FnStuck(C4AulObjectContext *cthr)
{
	return !!cthr->Obj->Shape.CheckContact(cthr->Obj->GetX(),cthr->Obj->GetY());
}

static bool FnInLiquid(C4AulObjectContext *cthr)
{
	return cthr->Obj->InLiquid;
}

static bool FnOnFire(C4AulObjectContext *cthr)
{
	if (cthr->Obj->GetOnFire()) return true;
	// check for effect
	if (!cthr->Obj->pEffects) return false;
	return !!cthr->Obj->pEffects->Get(C4Fx_AnyFire);
}

static bool FnComponentAll(C4AulObjectContext *cthr, C4ID c_id)
{
	long cnt;
	C4IDList Components;
	cthr->Obj->Def->GetComponents(&Components, cthr->Obj);
	for (cnt=0; Components.GetID(cnt); cnt++)
		if (Components.GetID(cnt)!=c_id)
			if (Components.GetCount(cnt)>0)
				return false;
	return true;
}

static C4Object *FnCreateContents(C4AulObjectContext *cthr, C4PropList * PropList, Nillable<long> iCount)
{
	// default amount parameter
	if (iCount.IsNil()) iCount = 1;
	// create objects
	C4Object *pNewObj = NULL;
	while (iCount-- > 0) pNewObj = cthr->Obj->CreateContents(PropList);
	// controller will automatically be set upon entrance
	// return last created
	return pNewObj;
}

static C4Object *FnComposeContents(C4AulObjectContext *cthr, C4ID c_id)
{
	return cthr->Obj->ComposeContents(c_id);
}

static bool FnMakeCrewMember(C4AulObjectContext *cthr, long iPlayer)
{
	if (!ValidPlr(iPlayer)) return false;
	return !!::Players.Get(iPlayer)->MakeCrewMember(cthr->Obj);
}

static bool FnGrabObjectInfo(C4AulObjectContext *cthr, C4Object *pFrom)
{
	// local call, safety
	if (!pFrom) return false;
	// grab info
	return !!cthr->Obj->GrabInfo(pFrom);
}

static bool FnBlastObject(C4AulObjectContext *cthr, long iLevel, Nillable<long> iCausedBy)
{
	if (iCausedBy.IsNil() && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	if (!cthr->Obj->Status) return false;
	cthr->Obj->Blast(iLevel, iCausedBy);
	return true;
}

static bool FnSetComponent(C4AulObjectContext *cthr, C4ID idComponent, long iCount)
{
	return cthr->Obj->Component.SetIDCount(idComponent,iCount,true);
}

static bool FnSetCrewStatus(C4AulObjectContext *cthr, long iPlr, bool fInCrew)
{
	// validate player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return false;
	// set crew status
	return !!pPlr->SetObjectCrewStatus(cthr->Obj, fInCrew);
}

static long FnSetTransferZone(C4AulObjectContext *cthr, long iX, long iY, long iWdt, long iHgt)
{
	iX+=cthr->Obj->GetX(); iY+=cthr->Obj->GetY();
	return Game.TransferZones.Set(iX,iY,iWdt,iHgt,cthr->Obj);
}

static long FnObjectDistance(C4AulContext *cthr, C4Object *pObj2, C4Object *pObj)
{
	if (!pObj) pObj=cthr->Obj; if (!pObj || !pObj2) return 0;
	return Distance(pObj->GetX(),pObj->GetY(),pObj2->GetX(),pObj2->GetY());
}

static long FnObjectNumber(C4AulObjectContext *cthr)
{
	return cthr->Obj->Number;
	// See FnObject
}

static long FnShowInfo(C4AulObjectContext *cthr, C4Object *pObj)
{
	if (!pObj) pObj=cthr->Obj; if (!pObj) return false;
	return cthr->Obj->ActivateMenu(C4MN_Info,0,0,0,pObj);
}

static C4Void FnSetMass(C4AulObjectContext *cthr, long iValue)
{
	cthr->Obj->OwnMass=iValue-cthr->Obj->Def->Mass;
	cthr->Obj->UpdateMass();
	return C4Void();
}

static long FnGetColor(C4AulObjectContext *cthr)
{
	return cthr->Obj->Color;
}

static C4Void FnSetColor(C4AulObjectContext *cthr, long iValue)
{
	cthr->Obj->Color=iValue;
	cthr->Obj->UpdateGraphics(false);
	cthr->Obj->UpdateFace(false);
	return C4Void();
}

static C4Void FnSetPlrViewRange(C4AulObjectContext *cthr, long iRange)
{
	// set range
	cthr->Obj->SetPlrViewRange(iRange);
	// success
	return C4Void();
}

static C4Void FnSetPicture(C4AulObjectContext *cthr, long iX, long iY, long iWdt, long iHgt)
{
	// set new picture rect
	cthr->Obj->PictureRect.Set(iX, iY, iWdt, iHgt);
	// success
	return C4Void();
}

static C4String *FnGetProcedure(C4AulObjectContext *cthr)
{
	// no action?
	C4PropList* pActionDef = cthr->Obj->GetAction();
	if (!pActionDef) return NULL;
	// get proc
	return pActionDef->GetPropertyStr(P_Procedure);
}

static bool FnCheckVisibility(C4AulObjectContext *cthr, int plr) {
	return cthr->Obj->IsVisible(plr, false);
}

static bool FnSetClrModulation(C4AulObjectContext *cthr, Nillable<long> dwClr, long iOverlayID)
{
	uint32_t clr = 0xffffffff;
	if (!dwClr.IsNil()) clr = dwClr;

	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = cthr->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) cthr->Obj->Number, cthr->Obj->GetName());
			return false;
		}
		pOverlay->SetClrModulation(clr);
		// C4GraphicsOverlay Does not have an StdMeshInstance (yet), no need to
		// update faceordering
	}
	else
	{
		// set it
		cthr->Obj->ColorMod=clr;
		if (cthr->Obj->pMeshInstance)
			cthr->Obj->pMeshInstance->SetFaceOrderingForClrModulation(clr);
	}
	// success
	return true;
}

static Nillable<long> FnGetClrModulation(C4AulObjectContext *cthr, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = cthr->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("GetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) cthr->Obj->Number, cthr->Obj->GetName());
			return C4Void();
		}
		return pOverlay->GetClrModulation();
	}
	else
		// get it
		return cthr->Obj->ColorMod;
}

static bool FnCloseMenu(C4AulObjectContext *cthr)
{
	return !!cthr->Obj->CloseMenu(true);
}

static Nillable<long> FnGetMenuSelection(C4AulObjectContext *cthr)
{
	if (!cthr->Obj->Menu || !cthr->Obj->Menu->IsActive()) return C4Void();
	return cthr->Obj->Menu->GetSelection();
}

static bool FnCanConcatPictureWith(C4AulObjectContext *pCtx, C4Object *pObj)
{
	// safety
	if (!pCtx->Obj->Status || !pObj) return false;
	// Call the function in the object
	return pCtx->Obj->CanConcatPictureWith(pObj);
}

static bool FnSetGraphics(C4AulObjectContext *pCtx, C4String *pGfxName, C4Def *pSrcDef, long iOverlayID, long iOverlayMode, C4String *pAction, long dwBlitMode, C4Object *pOverlayObject)
{
	// safety
	if (!pCtx->Obj->Status) return false;
	// setting overlay?
	if (iOverlayID)
	{
		// any overlays must be positive for now
		if (iOverlayID<0) { Log("SetGraphics: Background overlays not implemented!"); return false; }
		// deleting overlay?
		C4DefGraphics *pGrp = NULL;
		if (iOverlayMode == C4GraphicsOverlay::MODE_Object || iOverlayMode == C4GraphicsOverlay::MODE_Rank || iOverlayMode == C4GraphicsOverlay::MODE_ObjectPicture)
		{
			if (!pOverlayObject) return pCtx->Obj->RemoveGraphicsOverlay(iOverlayID);
		}
		else
		{
			if (!pSrcDef) return pCtx->Obj->RemoveGraphicsOverlay(iOverlayID);
			pGrp = pSrcDef->Graphics.Get(FnStringPar(pGfxName));
			if (!pGrp) return false;
		}
		// adding/setting
		C4GraphicsOverlay *pOverlay = pCtx->Obj->GetGraphicsOverlay(iOverlayID, true);
		switch (iOverlayMode)
		{
		case C4GraphicsOverlay::MODE_Base:
			pOverlay->SetAsBase(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Action:
			pOverlay->SetAsAction(pGrp, FnStringPar(pAction), dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_IngamePicture:
			pOverlay->SetAsIngamePicture(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Picture:
			pOverlay->SetAsPicture(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Object:
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = NULL;
			pOverlay->SetAsObject(pOverlayObject, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_ExtraGraphics:
			pOverlay->SetAsExtraGraphics(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Rank:
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = NULL;
			pOverlay->SetAsRank(dwBlitMode, pOverlayObject);
			break;

		case C4GraphicsOverlay::MODE_ObjectPicture:
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = NULL;
			pOverlay->SetAsObjectPicture(pOverlayObject, dwBlitMode);
			break;

		default:
			DebugLog("SetGraphics: Invalid overlay mode");
			pOverlay->SetAsBase(NULL, 0); // make invalid, so it will be removed
			break;
		}
		// remove if invalid
		if (!pOverlay->IsValid(pCtx->Obj))
		{
			pCtx->Obj->RemoveGraphicsOverlay(iOverlayID);
			return false;
		}
		// Okay, valid overlay set!
		return true;
	}
	// no overlay: Base graphics
	// set graphics - pSrcDef==NULL defaults to pObj->pDef
	return pCtx->Obj->SetGraphics(FnStringPar(pGfxName), pSrcDef);
}

static long FnGetDefBottom(C4AulContext* cthr)
{
	if (!cthr->Obj)
		if (!cthr->Def)
			throw new NeedNonGlobalContext("GetDefBottom");

	assert(!cthr->Obj || cthr->Obj->Def == cthr->Def);
	return cthr->Def->Shape.y+cthr->Def->Shape.Hgt + (cthr->Obj ? cthr->Obj->GetY() : 0);
}

static bool FnSetMenuSize(C4AulObjectContext* cthr, long iCols, long iRows)
{
	// get menu
	C4Menu *pMnu=cthr->Obj->Menu;
	if (!pMnu || !pMnu->IsActive()) return false;
	pMnu->SetSize(BoundBy<long>(iCols, 0, 50), BoundBy<long>(iRows, 0, 50));
	return true;
}

static bool FnGetCrewEnabled(C4AulObjectContext *cctx)
{
	// return status
	return !cctx->Obj->CrewDisabled;
}

static C4Void FnSetCrewEnabled(C4AulObjectContext *cctx, bool fEnabled)
{
	bool change = (cctx->Obj->CrewDisabled == fEnabled) ? true : false;

	// set status
	cctx->Obj->CrewDisabled=!fEnabled;
	// deselect
	if (!fEnabled)
	{
		C4Player *pOwner;
		if ((pOwner=::Players.Get(cctx->Obj->Owner)))
		{
			// if viewed player cursor gets deactivated and no new cursor is found, follow the old in target mode
			bool fWasCursorMode = (pOwner->ViewMode == C4PVM_Cursor);
			if (pOwner->Cursor==cctx->Obj)
				pOwner->AdjustCursorCommand();
			if (!pOwner->ViewCursor && !pOwner->Cursor && fWasCursorMode)
				pOwner->SetViewMode(C4PVM_Target, cctx->Obj);
		}
	}

	// call to crew
	if (change)
	{
		if (fEnabled)
			cctx->Obj->Call(PSF_CrewEnabled);
		else
			cctx->Obj->Call(PSF_CrewDisabled);
	}

	// success
	return C4Void();
}

static C4Void FnDoCrewExp(C4AulObjectContext* ctx, long iChange)
{
	// do exp
	ctx->Obj->DoExperience(iChange);
	// success
	return C4Void();
}

static bool FnClearMenuItems(C4AulObjectContext *ctx)
{
	// check menu
	if (!ctx->Obj->Menu) return false;
	// clear the items
	ctx->Obj->Menu->ClearItems();
	// success
	return true;
}

static C4Object *FnGetObjectLayer(C4AulObjectContext *ctx)
{
	// get layer object
	return ctx->Obj->Layer;
}

static C4Void FnSetObjectLayer(C4AulObjectContext *ctx, C4Object *pNewLayer)
{
	// set layer object
	ctx->Obj->Layer = pNewLayer;
	// set for all contents as well
	for (C4ObjectLink *pLnk=ctx->Obj->Contents.First; pLnk; pLnk=pLnk->Next)
		if ((ctx->Obj=pLnk->Obj) && ctx->Obj->Status)
			ctx->Obj->Layer = pNewLayer;
	// success
	return C4Void();
}

static C4Void FnSetShape(C4AulObjectContext *ctx, long iX, long iY, long iWdt, long iHgt)
{
	// update shape
	ctx->Obj->Shape.x = iX;
	ctx->Obj->Shape.y = iY;
	ctx->Obj->Shape.Wdt = iWdt;
	ctx->Obj->Shape.Hgt = iHgt;
	// section list needs refresh
	ctx->Obj->UpdatePos();
	// done, success
	return C4Void();
}

static bool FnSetObjDrawTransform(C4AulObjectContext *ctx, long iA, long iB, long iC, long iD, long iE, long iF, long iOverlayID)
{
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = ctx->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return false;
		pTransform = pOverlay->GetTransform();
	}
	else
	{
		// set base transform
		pTransform = ctx->Obj->pDrawTransform;
		// del transform?
		if (!iB && !iC && !iD && !iF && iA==iE && (!iA || iA==1000))
		{
			// identity/0 and no transform defined: nothing to do
			if (!pTransform) return true;
			// transform has no flipdir?
			if (pTransform->FlipDir == 1)
			{
				// kill identity-transform, then
				delete pTransform;
				ctx->Obj->pDrawTransform=NULL;
				return true;
			}
			// flipdir must remain: set identity
			pTransform->Set(1,0,0,0,1,0,0,0,1);
			return true;
		}
		// create draw transform if not already present
		if (!pTransform) pTransform = ctx->Obj->pDrawTransform = new C4DrawTransform();
	}
	// assign values
	pTransform->Set((float) iA/1000, (float) iB/1000, (float) iC/1000, (float) iD/1000, (float) iE/1000, (float) iF/1000, 0, 0, 1);
	// done, success
	return true;
}

static bool FnSetObjDrawTransform2(C4AulObjectContext *ctx, long iA, long iB, long iC, long iD, long iE, long iF, long iG, long iH, long iI, long iOverlayID)
{
	// local call / safety
	C4Object * pObj = ctx->Obj;
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return false;
		pTransform = pOverlay->GetTransform();
	}
	else
	{
		// set base transform
		pTransform = pObj->pDrawTransform;
		// create draw transform if not already present
		if (!pTransform) pTransform = pObj->pDrawTransform = new C4DrawTransform(1);
	}
	// assign values
#define L2F(l) ((float)l/1000)
	CBltTransform matrix;
	matrix.Set(L2F(iA), L2F(iB), L2F(iC), L2F(iD), L2F(iE), L2F(iF), L2F(iG), L2F(iH), L2F(iI));
	*pTransform *= matrix;
#undef L2F
	// done, success
	return true;
}

static bool FnSetObjectStatus(C4AulObjectContext *ctx, long iNewStatus, bool fClearPointers)
{
	// local call / safety
	if (!ctx->Obj->Status) return false;
	// no change
	if (ctx->Obj->Status == iNewStatus) return true;
	// set new status
	switch (iNewStatus)
	{
	case C4OS_NORMAL:
		return ctx->Obj->StatusActivate();
	case C4OS_INACTIVE:
		return ctx->Obj->StatusDeactivate(fClearPointers);
	default:
		return false; // status unknown
	}
}

static long FnGetObjectStatus(C4AulObjectContext *ctx)
{
	return ctx->Obj->Status;
}

static bool FnAdjustWalkRotation(C4AulObjectContext *ctx, long iRangeX, long iRangeY, long iSpeed)
{
	// must be rotateable and attached to solid ground
	if (!ctx->Obj->Def->Rotateable || ~ctx->Obj->Action.t_attach&CNAT_Bottom || ctx->Obj->Shape.AttachMat == MNone)
		return false;
	// adjust rotation
	return ctx->Obj->AdjustWalkRotation(iRangeX, iRangeY, iSpeed);
}

static long FnGetContact(C4AulObjectContext *ctx, long iVertex, long dwCheck)
{
	// vertex not specified: check all
	if (iVertex == -1)
	{
		long iResult = 0;
		for (int i=0; i<ctx->Obj->Shape.VtxNum; ++i)
			iResult |= ctx->Obj->Shape.GetVertexContact(i, dwCheck, ctx->Obj->GetX(), ctx->Obj->GetY());
		return iResult;
	}
	// vertex specified: check it
	if (!Inside<long>(iVertex, 0, ctx->Obj->Shape.VtxNum-1)) return 0;
	return ctx->Obj->Shape.GetVertexContact(iVertex, dwCheck, ctx->Obj->GetX(), ctx->Obj->GetY());
}

static long FnSetObjectBlitMode(C4AulObjectContext *ctx, long dwNewBlitMode, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = ctx->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) ctx->Obj->Number, ctx->Obj->GetName());
			return false;
		}
		pOverlay->SetBlitMode(dwNewBlitMode);
		return true;
	}
	// get prev blit mode
	DWORD dwPrevMode = ctx->Obj->BlitMode;
	// iNewBlitMode = 0: reset to definition default
	if (!dwNewBlitMode)
		ctx->Obj->BlitMode = ctx->Obj->Def->BlitMode;
	else
		// otherwise, set the desired value
		// also ensure that the custom flag is set
		ctx->Obj->BlitMode = dwNewBlitMode | C4GFXBLIT_CUSTOM;
	// return previous value
	return dwPrevMode;
}

static Nillable<long> FnGetObjectBlitMode(C4AulObjectContext *ctx, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = ctx->Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) ctx->Obj->Number, ctx->Obj->GetName());
			return C4Void();
		}
		return pOverlay->GetBlitMode();
	}
	// get blitting mode
	return ctx->Obj->BlitMode;
}

static bool FnOnOwnerRemoved(C4AulObjectContext *cthr)
{
	// safety
	C4Object *pObj = cthr->Obj;
	C4Player *pPlr = ::Players.Get(pObj->Owner); if (!pPlr) return false;
	if (pPlr->Crew.IsContained(pObj))
	{
		// crew members: Those are removed later (AFTER the player has been removed, for backwards compatiblity with relaunch scripting)
	}
	else if ((~pObj->Category & C4D_StaticBack) || (pObj->id == C4ID::Flag))
	{
		// Regular objects: Try to find a new, suitable owner from the same team
		// Ignore StaticBack, because this would not be backwards compatible with many internal objects such as team account
		// Do not ignore flags which might be StaticBack if being attached to castle parts
		int32_t iNewOwner = NO_OWNER;
		C4Team *pTeam;
		if (pPlr->Team) if ((pTeam = Game.Teams.GetTeamByID(pPlr->Team)))
			{
				for (int32_t i=0; i<pTeam->GetPlayerCount(); ++i)
				{
					int32_t iPlrID = pTeam->GetIndexedPlayer(i);
					if (iPlrID && iPlrID != pPlr->ID)
					{
						C4PlayerInfo *pPlrInfo = Game.PlayerInfos.GetPlayerInfoByID(iPlrID);
						if (pPlrInfo) if (pPlrInfo->IsJoined())
							{
								// this looks like a good new owner
								iNewOwner = pPlrInfo->GetInGameNumber();
								break;
							}
					}
				}
			}
		// if noone from the same team was found, try to find another non-hostile player
		// (necessary for cooperative rounds without teams)
		if (iNewOwner == NO_OWNER)
			for (C4Player *pOtherPlr = ::Players.First; pOtherPlr; pOtherPlr = pOtherPlr->Next)
				if (pOtherPlr != pPlr) if (!pOtherPlr->Eliminated)
						if (!::Players.Hostile(pOtherPlr->Number, pPlr->Number))
							iNewOwner = pOtherPlr->Number;

		// set this owner
		pObj->SetOwner(iNewOwner);
	}
	return true;
}

static long FnGetUnusedOverlayID(C4AulObjectContext *ctx, long iBaseIndex)
{
	// safety
	if (!iBaseIndex) return 0;
	// find search first unused index from there on
	int iSearchDir = (iBaseIndex < 0) ? -1 : 1;
	while (ctx->Obj->GetGraphicsOverlay(iBaseIndex, false)) iBaseIndex += iSearchDir;
	return iBaseIndex;
}

static C4Void FnDoNoCollectDelay(C4AulObjectContext *ctx, int change)
{
	ctx->Obj->NoCollectDelay = Max<int32_t>(ctx->Obj->NoCollectDelay + change, 0);
	return C4Void();
}

static Nillable<int> FnPlayAnimation(C4AulObjectContext *ctx, C4String *szAnimation, int iSlot, C4ValueArray* PositionProvider, C4ValueArray* WeightProvider, Nillable<int> iSibling, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return C4Void();
	if (!ctx->Obj->pMeshInstance) return C4Void();
	if (iSlot == 0) return C4Void(); // Reserved for ActMap animations
	if (!PositionProvider) return C4Void();
	if (!WeightProvider) return C4Void();

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return C4Void();
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* s_node = NULL;
	if (!iSibling.IsNil())
	{
		s_node = Instance->GetAnimationNodeByNumber(iSibling);
		if (!s_node || s_node->GetSlot() != iSlot) return C4Void();
	}

	StdMeshInstance::ValueProvider* p_provider = CreateValueProviderFromArray(ctx->Obj, *PositionProvider);
	StdMeshInstance::ValueProvider* w_provider = CreateValueProviderFromArray(ctx->Obj, *WeightProvider);
	if (!p_provider || !w_provider)
	{
		delete p_provider;
		delete w_provider;
		return C4Void();
	}

	StdMeshInstance::AnimationNode* n_node = Instance->PlayAnimation(szAnimation->GetData(), iSlot, s_node, p_provider, w_provider);
	if (!n_node) return C4Void();

	return n_node->GetNumber();
}

static bool FnStopAnimation(C4AulObjectContext *ctx, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return false;
	if (!ctx->Obj->pMeshInstance) return false;
	if (iAnimationNumber.IsNil()) return false; // distinguish nil from 0

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return false;
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0) return false;
	Instance->StopAnimation(node);
	return true;
}

static Nillable<int> FnGetRootAnimation(C4AulObjectContext *ctx, int iSlot, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return C4Void();
	if (!ctx->Obj->pMeshInstance) return C4Void();

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return C4Void();
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetRootAnimationForSlot(iSlot);
	if (!node) return C4Void();
	return node->GetNumber();
}

static Nillable<int> FnGetAnimationLength(C4AulObjectContext *ctx, C4String *szAnimation, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return C4Void();
	if (!ctx->Obj->pMeshInstance) return C4Void();

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return C4Void();
		Instance = Attached->Child;
	}

	const StdMeshAnimation* animation = Instance->GetMesh().GetAnimationByName(szAnimation->GetData());
	if (!animation) return C4Void();
	return static_cast<int>(animation->Length * 1000.0f); // TODO: sync critical
}

static Nillable<C4String*> FnGetAnimationName(C4AulObjectContext *ctx, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return C4Void();
	if (!ctx->Obj->pMeshInstance) return C4Void();
	if (iAnimationNumber.IsNil()) return C4Void(); // distinguish nil from 0

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return C4Void();
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	if (!node || node->GetType() != StdMeshInstance::AnimationNode::LeafNode) return C4Void();
	return String(node->GetAnimation()->Name.getData());
}

static Nillable<int> FnGetAnimationPosition(C4AulObjectContext *ctx, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return C4Void();
	if (!ctx->Obj->pMeshInstance) return C4Void();
	if (iAnimationNumber.IsNil()) return C4Void(); // distinguish nil from 0

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return C4Void();
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	if (!node || node->GetType() != StdMeshInstance::AnimationNode::LeafNode) return C4Void();
	return fixtoi(node->GetPosition(), 1000);
}

static Nillable<int> FnGetAnimationWeight(C4AulObjectContext *ctx, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return C4Void();
	if (!ctx->Obj->pMeshInstance) return C4Void();
	if (iAnimationNumber.IsNil()) return C4Void(); // distinguish nil from 0

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return C4Void();
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	if (!node || node->GetType() != StdMeshInstance::AnimationNode::LinearInterpolationNode) return C4Void();
	return fixtoi(node->GetWeight(), 1000);
}

static bool FnSetAnimationPosition(C4AulObjectContext *ctx, Nillable<int> iAnimationNumber, C4ValueArray* PositionProvider, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return false;
	if (!ctx->Obj->pMeshInstance) return false;
	if (iAnimationNumber.IsNil()) return false; // distinguish nil from 0

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return false;
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0 || node->GetType() != StdMeshInstance::AnimationNode::LeafNode) return false;
	StdMeshInstance::ValueProvider* p_provider = CreateValueProviderFromArray(ctx->Obj, *PositionProvider);
	if (!p_provider) return false;
	Instance->SetAnimationPosition(node, p_provider);
	return true;
}

static bool FnSetAnimationWeight(C4AulObjectContext *ctx, Nillable<int> iAnimationNumber, C4ValueArray* WeightProvider, Nillable<int> iAttachNumber)
{
	if (!ctx->Obj) return false;
	if (!ctx->Obj->pMeshInstance) return false;
	if (iAnimationNumber.IsNil()) return false; // distinguish nil from 0

	StdMeshInstance* Instance = ctx->Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild) return false;
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0 || node->GetType() != StdMeshInstance::AnimationNode::LinearInterpolationNode) return false;
	StdMeshInstance::ValueProvider* w_provider = CreateValueProviderFromArray(ctx->Obj, *WeightProvider);
	if (!w_provider) return false;
	Instance->SetAnimationWeight(node, w_provider);
	return true;
}

static Nillable<int> FnAttachMesh(C4AulObjectContext *ctx, C4PropList* Mesh, C4String * szParentBone, C4String * szChildBone, C4ValueArray * Transformation, int Flags)
{
	if (!ctx->Obj->pMeshInstance) return C4Void();
	if (!Mesh) return C4Void();

	StdMeshMatrix trans = StdMeshMatrix::Identity();
	if (Transformation)
		if (!C4ValueToMatrix(*Transformation, &trans))
			throw new C4AulExecError(ctx->Obj, "AttachMesh: Transformation is not a valid 3x4 matrix");

	StdMeshInstance::AttachedMesh* attach;
	C4Object* pObj = Mesh->GetObject();
	if (pObj)
	{
		if (!pObj->pMeshInstance) return C4Void();
		attach = ctx->Obj->pMeshInstance->AttachMesh(*pObj->pMeshInstance, new C4MeshDenumerator(pObj), szParentBone->GetData(), szChildBone->GetData(), trans, Flags);
	}
	else
	{
		C4Def* pDef = Mesh->GetDef();
		if (!pDef) return C4Void();
		if (pDef->Graphics.Type != C4DefGraphics::TYPE_Mesh) return C4Void();
		attach = ctx->Obj->pMeshInstance->AttachMesh(*pDef->Graphics.Mesh, new C4MeshDenumerator(pDef), szParentBone->GetData(), szChildBone->GetData(), trans, Flags);
	}

	if (!attach) return C4Void();
	return attach->Number;
}

static bool FnDetachMesh(C4AulObjectContext *ctx, long iAttachNumber)
{
	if (!ctx->Obj || !ctx->Obj->pMeshInstance) return false;
	return ctx->Obj->pMeshInstance->DetachMesh(iAttachNumber);
}

static bool FnSetAttachBones(C4AulObjectContext* ctx, long iAttachNumber, Nillable<C4String*> szParentBone, Nillable<C4String*> szChildBone)
{
	if (!ctx->Obj || !ctx->Obj->pMeshInstance) return false;
	StdMeshInstance::AttachedMesh* attach = ctx->Obj->pMeshInstance->GetAttachedMeshByNumber(iAttachNumber);
	if (!attach) return false;

	if (!szParentBone.IsNil())
	{
		C4String* ParentBone = szParentBone;
		if (!attach->SetParentBone(ParentBone->GetData())) return false;
	}

	if (!szChildBone.IsNil())
	{
		C4String* ChildBone = szChildBone;
		if (!attach->SetChildBone(ChildBone->GetData())) return false;
	}

	return true;
}

static bool FnSetAttachTransform(C4AulObjectContext* ctx, long iAttachNumber, C4ValueArray* Transformation)
{
	if (!ctx->Obj || !ctx->Obj->pMeshInstance) return false;
	if (!Transformation) return false;
	StdMeshInstance::AttachedMesh* attach = ctx->Obj->pMeshInstance->GetAttachedMeshByNumber(iAttachNumber);
	if (!attach) return false;

	StdMeshMatrix trans;
	if (!C4ValueToMatrix(*Transformation, &trans))
		throw new C4AulExecError(ctx->Obj, "SetAttachTransform: Transformation is not a valid 3x4 matrix");

	attach->SetAttachTransformation(trans);
	return true;
}

static Nillable<C4String*> FnGetMeshMaterial(C4AulObjectContext* ctx, int iSubMesh)
{
	if (!ctx->Obj || !ctx->Obj->pMeshInstance) return C4Void();
	if (iSubMesh < 0 || (unsigned int)iSubMesh >= ctx->Obj->pMeshInstance->GetNumSubMeshes()) return C4Void();

	StdSubMeshInstance& submesh = ctx->Obj->pMeshInstance->GetSubMesh(iSubMesh);
	return String(submesh.GetMaterial().Name.getData());
}

static bool FnSetMeshMaterial(C4AulObjectContext* ctx, C4String* Material, int iSubMesh)
{
	if (!ctx->Obj || !ctx->Obj->pMeshInstance) return false;
	if (iSubMesh < 0 || (unsigned int)iSubMesh >= ctx->Obj->pMeshInstance->GetNumSubMeshes()) return false;
	if (!Material) return false;

	const StdMeshMaterial* material = ::MeshMaterialManager.GetMaterial(Material->GetData().getData());
	if (!material) return false;

	StdSubMeshInstance& submesh = ctx->Obj->pMeshInstance->GetSubMesh(iSubMesh);
	submesh.SetMaterial(*material);
	return true;
}

//=========================== C4Script Function Map ===================================

C4ScriptConstDef C4ScriptObjectConstMap[]=
{
	{ "C4D_All"                ,C4V_Int,          C4D_All},
	{ "C4D_StaticBack"         ,C4V_Int,          C4D_StaticBack},
	{ "C4D_Structure"          ,C4V_Int,          C4D_Structure},
	{ "C4D_Vehicle"            ,C4V_Int,          C4D_Vehicle},
	{ "C4D_Living"             ,C4V_Int,          C4D_Living},
	{ "C4D_Object"             ,C4V_Int,          C4D_Object},
	{ "C4D_Goal"               ,C4V_Int,          C4D_Goal},
	{ "C4D_Environment"        ,C4V_Int,          C4D_Environment},
	{ "C4D_Rule"               ,C4V_Int,          C4D_Rule},
	{ "C4D_Background"         ,C4V_Int,          C4D_Background},
	{ "C4D_Parallax"           ,C4V_Int,          C4D_Parallax},
	{ "C4D_MouseSelect"        ,C4V_Int,          C4D_MouseSelect},
	{ "C4D_Foreground"         ,C4V_Int,          C4D_Foreground},
	{ "C4D_MouseIgnore"        ,C4V_Int,          C4D_MouseIgnore},
	{ "C4D_IgnoreFoW"          ,C4V_Int,          C4D_IgnoreFoW},

	{ "C4D_GrabGet"            ,C4V_Int,          C4D_Grab_Get},
	{ "C4D_GrabPut"            ,C4V_Int,          C4D_Grab_Put},

	{ "COMD_None"              ,C4V_Int,          COMD_None},
	{ "COMD_Stop"              ,C4V_Int,          COMD_Stop},
	{ "COMD_Up"                ,C4V_Int,          COMD_Up},
	{ "COMD_UpRight"           ,C4V_Int,          COMD_UpRight},
	{ "COMD_Right"             ,C4V_Int,          COMD_Right},
	{ "COMD_DownRight"         ,C4V_Int,          COMD_DownRight},
	{ "COMD_Down"              ,C4V_Int,          COMD_Down},
	{ "COMD_DownLeft"          ,C4V_Int,          COMD_DownLeft},
	{ "COMD_Left"              ,C4V_Int,          COMD_Left},
	{ "COMD_UpLeft"            ,C4V_Int,          COMD_UpLeft},

	{ "DIR_Left"               ,C4V_Int,          DIR_Left},
	{ "DIR_Right"              ,C4V_Int,          DIR_Right},

	{ "OCF_Construct"          ,C4V_Int,          OCF_Construct},
	{ "OCF_Grab"               ,C4V_Int,          OCF_Grab},
	{ "OCF_Collectible"        ,C4V_Int,          OCF_Carryable},
	{ "OCF_OnFire"             ,C4V_Int,          OCF_OnFire},
	{ "OCF_HitSpeed1"          ,C4V_Int,          OCF_HitSpeed1},
	{ "OCF_Fullcon"            ,C4V_Int,          OCF_FullCon},
	{ "OCF_Inflammable"        ,C4V_Int,          OCF_Inflammable},
	{ "OCF_Rotate"             ,C4V_Int,          OCF_Rotate},
	{ "OCF_Exclusive"          ,C4V_Int,          OCF_Exclusive},
	{ "OCF_Entrance"           ,C4V_Int,          OCF_Entrance},
	{ "OCF_HitSpeed2"          ,C4V_Int,          OCF_HitSpeed2},
	{ "OCF_HitSpeed3"          ,C4V_Int,          OCF_HitSpeed3},
	{ "OCF_Collection"         ,C4V_Int,          OCF_Collection},
	{ "OCF_HitSpeed4"          ,C4V_Int,          OCF_HitSpeed4},
	{ "OCF_AttractLightning"   ,C4V_Int,          OCF_AttractLightning},
	{ "OCF_NotContained"       ,C4V_Int,          OCF_NotContained},
	{ "OCF_CrewMember"         ,C4V_Int,          OCF_CrewMember},
	{ "OCF_InLiquid"           ,C4V_Int,          OCF_InLiquid},
	{ "OCF_InSolid"            ,C4V_Int,          OCF_InSolid},
	{ "OCF_InFree"             ,C4V_Int,          OCF_InFree},
	{ "OCF_Available"          ,C4V_Int,          OCF_Available},
	{ "OCF_Container"          ,C4V_Int,          OCF_Container},
	{ "OCF_Alive"              ,C4V_Int,          (int) OCF_Alive},

	{ "VIS_All"                ,C4V_Int,          VIS_All},
	{ "VIS_None"               ,C4V_Int,          VIS_None},
	{ "VIS_Owner"              ,C4V_Int,          VIS_Owner},
	{ "VIS_Allies"             ,C4V_Int,          VIS_Allies},
	{ "VIS_Enemies"            ,C4V_Int,          VIS_Enemies},
	{ "VIS_Select"             ,C4V_Int,          VIS_Select},
	{ "VIS_God"                ,C4V_Int,          VIS_God},
	{ "VIS_LayerToggle"        ,C4V_Int,          VIS_LayerToggle},
	{ "VIS_OverlayOnly"        ,C4V_Int,          VIS_OverlayOnly},

	{ "C4MN_Style_Normal"      ,C4V_Int,          C4MN_Style_Normal},
	{ "C4MN_Style_Context"     ,C4V_Int,          C4MN_Style_Context},
	{ "C4MN_Style_Info"        ,C4V_Int,          C4MN_Style_Info},
	{ "C4MN_Style_Dialog"      ,C4V_Int,          C4MN_Style_Dialog},
	{ "C4MN_Style_EqualItemHeight",C4V_Int,       C4MN_Style_EqualItemHeight},

	{ "C4MN_Extra_None"        ,C4V_Int,          C4MN_Extra_None},
	{ "C4MN_Extra_Components"  ,C4V_Int,          C4MN_Extra_Components},
	{ "C4MN_Extra_Value"       ,C4V_Int,          C4MN_Extra_Value},
	{ "C4MN_Extra_Info"        ,C4V_Int,          C4MN_Extra_Info},

	{ "C4MN_Add_ImgRank"       ,C4V_Int,          C4MN_Add_ImgRank},
	{ "C4MN_Add_ImgIndexed"    ,C4V_Int,          C4MN_Add_ImgIndexed},
	{ "C4MN_Add_ImgObjRank"    ,C4V_Int,          C4MN_Add_ImgObjRank},
	{ "C4MN_Add_ImgObject"     ,C4V_Int,          C4MN_Add_ImgObject},
	{ "C4MN_Add_ImgTextSpec"   ,C4V_Int,          C4MN_Add_ImgTextSpec},
	{ "C4MN_Add_ImgColor"      ,C4V_Int,          C4MN_Add_ImgColor},
	{ "C4MN_Add_PassValue"     ,C4V_Int,          C4MN_Add_PassValue},
	{ "C4MN_Add_ForceCount"    ,C4V_Int,          C4MN_Add_ForceCount},
	{ "C4MN_Add_ForceNoDesc"   ,C4V_Int,          C4MN_Add_ForceNoDesc},

	{ "GFXOV_MODE_None"           ,C4V_Int,      C4GraphicsOverlay::MODE_None },    // gfx overlay modes
	{ "GFXOV_MODE_Base"           ,C4V_Int,      C4GraphicsOverlay::MODE_Base },    //
	{ "GFXOV_MODE_Action"         ,C4V_Int,      C4GraphicsOverlay::MODE_Action },  //
	{ "GFXOV_MODE_Picture"        ,C4V_Int,      C4GraphicsOverlay::MODE_Picture }, //
	{ "GFXOV_MODE_IngamePicture"  ,C4V_Int,      C4GraphicsOverlay::MODE_IngamePicture }, //
	{ "GFXOV_MODE_Object"         ,C4V_Int,      C4GraphicsOverlay::MODE_Object }, //
	{ "GFXOV_MODE_ExtraGraphics"  ,C4V_Int,      C4GraphicsOverlay::MODE_ExtraGraphics }, //
	{ "GFXOV_MODE_Rank"           ,C4V_Int,      C4GraphicsOverlay::MODE_Rank}, //
	{ "GFXOV_MODE_ObjectPicture"  ,C4V_Int,      C4GraphicsOverlay::MODE_ObjectPicture}, //
	{ "GFX_Overlay"               ,C4V_Int,      1},                                // default overlay index
	{ "GFXOV_Clothing"            ,C4V_Int,      1000},                             // overlay indices for clothes on Clonks, etc.
	{ "GFXOV_Tools"               ,C4V_Int,      2000},                             // overlay indices for tools, weapons, etc.
	{ "GFXOV_ProcessTarget"       ,C4V_Int,      3000},                             // overlay indices for objects processed by a Clonk
	{ "GFXOV_Misc"                ,C4V_Int,      5000},                             // overlay indices for other stuff
	{ "GFXOV_UI"                  ,C4V_Int,      6000},                             // overlay indices for user interface
	{ "GFX_BLIT_Additive"         ,C4V_Int,      C4GFXBLIT_ADDITIVE},               // blit modes
	{ "GFX_BLIT_Mod2"             ,C4V_Int,      C4GFXBLIT_MOD2},                   //
	{ "GFX_BLIT_ClrSfc_OwnClr"    ,C4V_Int,      C4GFXBLIT_CLRSFC_OWNCLR},          //
	{ "GFX_BLIT_ClrSfc_Mod2"      ,C4V_Int,      C4GFXBLIT_CLRSFC_MOD2},            //
	{ "GFX_BLIT_Custom"           ,C4V_Int,      C4GFXBLIT_CUSTOM},                 //
	{ "GFX_BLIT_Parent"           ,C4V_Int,      C4GFXBLIT_PARENT},                 //

	// contact attachment
	{ "CNAT_None"                 ,C4V_Int,      CNAT_None                  },
	{ "CNAT_Left"                 ,C4V_Int,      CNAT_Left                  },
	{ "CNAT_Right"                ,C4V_Int,      CNAT_Right                 },
	{ "CNAT_Top"                  ,C4V_Int,      CNAT_Top                   },
	{ "CNAT_Bottom"               ,C4V_Int,      CNAT_Bottom                },
	{ "CNAT_Center"               ,C4V_Int,      CNAT_Center                },
	{ "CNAT_MultiAttach"          ,C4V_Int,      CNAT_MultiAttach           },
	{ "CNAT_NoCollision"          ,C4V_Int,      CNAT_NoCollision           },

	// vertex data
	{ "VTX_X"                     ,C4V_Int,      VTX_X                      },
	{ "VTX_Y"                     ,C4V_Int,      VTX_Y                      },
	{ "VTX_CNAT"                  ,C4V_Int,      VTX_CNAT                   },
	{ "VTX_Friction"              ,C4V_Int,      VTX_Friction               },

	// vertex set mode
	{ "VTX_SetPermanent"          ,C4V_Int,      VTX_SetPermanent           },
	{ "VTX_SetPermanentUpd"       ,C4V_Int,      VTX_SetPermanentUpd        },

	{ "C4OS_DELETED"              ,C4V_Int,      C4OS_DELETED               },
	{ "C4OS_NORMAL"               ,C4V_Int,      C4OS_NORMAL                },
	{ "C4OS_INACTIVE"             ,C4V_Int,      C4OS_INACTIVE              },

	{ "C4CMD_Base"                ,C4V_Int,     C4CMD_Mode_Base },
	{ "C4CMD_SilentBase"          ,C4V_Int,     C4CMD_Mode_SilentBase },
	{ "C4CMD_Sub"                 ,C4V_Int,     C4CMD_Mode_Sub },
	{ "C4CMD_SilentSub"           ,C4V_Int,     C4CMD_Mode_SilentSub },

	{ "C4CMD_MoveTo_NoPosAdjust"  ,C4V_Int,     C4CMD_MoveTo_NoPosAdjust },
	{ "C4CMD_MoveTo_PushTarget"   ,C4V_Int,     C4CMD_MoveTo_PushTarget },
	{ "C4CMD_Enter_PushTarget"    ,C4V_Int,     C4CMD_Enter_PushTarget },

	{ "C4AVP_Const"               ,C4V_Int,      C4AVP_Const },
	{ "C4AVP_Linear"              ,C4V_Int,      C4AVP_Linear },
	{ "C4AVP_X"                   ,C4V_Int,      C4AVP_X },
	{ "C4AVP_Y"                   ,C4V_Int,      C4AVP_Y },
	{ "C4AVP_AbsX"                ,C4V_Int,      C4AVP_AbsX },
	{ "C4AVP_AbsY"                ,C4V_Int,      C4AVP_AbsY },
	{ "C4AVP_XDir"                ,C4V_Int,      C4AVP_XDir },
	{ "C4AVP_YDir"                ,C4V_Int,      C4AVP_YDir },
	{ "C4AVP_RDir"                ,C4V_Int,      C4AVP_RDir },
	{ "C4AVP_CosR"                ,C4V_Int,      C4AVP_CosR },
	{ "C4AVP_SinR"                ,C4V_Int,      C4AVP_SinR },
	{ "C4AVP_CosV"                ,C4V_Int,      C4AVP_CosV },
	{ "C4AVP_SinV"                ,C4V_Int,      C4AVP_SinV },
	{ "C4AVP_Action"              ,C4V_Int,      C4AVP_Action },

	{ "ANIM_Loop"                 ,C4V_Int,      ANIM_Loop },
	{ "ANIM_Hold"                 ,C4V_Int,      ANIM_Hold },
	{ "ANIM_Remove"               ,C4V_Int,      ANIM_Remove },

	{ "AM_None"                   ,C4V_Int,      StdMeshInstance::AM_None },
	{ "AM_DrawBefore"             ,C4V_Int,      StdMeshInstance::AM_DrawBefore },

	{ NULL, C4V_Nil, 0}
};

#define MkFnC4V (C4Value (*)(C4AulContext *cthr, C4Value*, C4Value*, C4Value*, C4Value*, C4Value*,\
                                                 C4Value*, C4Value*, C4Value*, C4Value*, C4Value*))

C4ScriptFnDef C4ScriptObjectFnMap[]=
{
	{ "SetCommand",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Any     ,C4V_Int    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnSetCommand },
	{ "AddCommand",           1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Int     ,C4V_Any    ,C4V_Int    ,C4V_Int    ,C4V_Any}  ,0 ,                                   FnAddCommand },
	{ "AppendCommand",        1  ,C4V_Bool     ,{ C4V_String  ,C4V_C4Object,C4V_Any     ,C4V_Int     ,C4V_C4Object,C4V_Int     ,C4V_Any    ,C4V_Int    ,C4V_Int    ,C4V_Any}  ,0 ,                                   FnAppendCommand },
	{ "GetCommand",           1  ,C4V_Any      ,{ C4V_Int     ,C4V_Int     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}  ,0 ,                                   FnGetCommand },
	{ "SetCrewExtraData",     1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnSetCrewExtraData,          0 },
	{ "GetCrewExtraData",     1  ,C4V_Any      ,{ C4V_String  ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,MkFnC4V FnGetCrewExtraData,          0 },

	{ NULL,                   0  ,C4V_Nil      ,{ C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil     ,C4V_Nil    ,C4V_Nil    ,C4V_Nil    ,C4V_Nil}   ,0,                                   0 }
};

void InitObjectFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptObjectConstMap[0]; pCDef->Identifier; pCDef++)
	{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		pEngine->RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
	}

	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptObjectFnMap[0]; pDef->Identifier; pDef++)
		pEngine->AddFunc(pDef->Identifier, pDef);
//  AddFunc(pEngine, "SetSaturation", FnSetSaturation); //public: 0
	AddFunc(pEngine, "DoCon", FnDoCon);
	AddFunc(pEngine, "GetCon", FnGetCon);
	AddFunc(pEngine, "DoDamage", FnDoDamage);
	AddFunc(pEngine, "DoEnergy", FnDoEnergy);
	AddFunc(pEngine, "DoBreath", FnDoBreath);
	AddFunc(pEngine, "GetEnergy", FnGetEnergy);
	AddFunc(pEngine, "OnFire", FnOnFire);
	AddFunc(pEngine, "Stuck", FnStuck);
	AddFunc(pEngine, "InLiquid", FnInLiquid);
	AddFunc(pEngine, "SetAction", FnSetAction);
	AddFunc(pEngine, "SetActionData", FnSetActionData);

	AddFunc(pEngine, "SetBridgeActionData", FnSetBridgeActionData);
	AddFunc(pEngine, "GetAction", FnGetAction);
	AddFunc(pEngine, "GetActTime", FnGetActTime);
	AddFunc(pEngine, "GetOwner", FnGetOwner);
	AddFunc(pEngine, "GetMass", FnGetMass);
	AddFunc(pEngine, "GetBreath", FnGetBreath);
	AddFunc(pEngine, "GetMenu", FnGetMenu);
	AddFunc(pEngine, "GetVertexNum", FnGetVertexNum);
	AddFunc(pEngine, "GetVertex", FnGetVertex);
	AddFunc(pEngine, "SetVertex", FnSetVertex);
	AddFunc(pEngine, "AddVertex", FnAddVertex);
	AddFunc(pEngine, "RemoveVertex", FnRemoveVertex);
	AddFunc(pEngine, "SetContactDensity", FnSetContactDensity, false);
	AddFunc(pEngine, "GetController", FnGetController);
	AddFunc(pEngine, "SetController", FnSetController);
	AddFunc(pEngine, "GetName", FnGetName);
	AddFunc(pEngine, "SetName", FnSetName);
	AddFunc(pEngine, "GetKiller", FnGetKiller);
	AddFunc(pEngine, "SetKiller", FnSetKiller);
	AddFunc(pEngine, "GetPhase", FnGetPhase);
	AddFunc(pEngine, "SetPhase", FnSetPhase);
	AddFunc(pEngine, "GetCategory", FnGetCategory);
	AddFunc(pEngine, "GetOCF", FnGetOCF);
	AddFunc(pEngine, "SetAlive", FnSetAlive);
	AddFunc(pEngine, "GetAlive", FnGetAlive);
	AddFunc(pEngine, "GetDamage", FnGetDamage);
	AddFunc(pEngine, "ComponentAll", FnComponentAll, false);
	AddFunc(pEngine, "SetComDir", FnSetComDir);
	AddFunc(pEngine, "GetComDir", FnGetComDir);
	AddFunc(pEngine, "SetDir", FnSetDir);
	AddFunc(pEngine, "GetDir", FnGetDir);
	AddFunc(pEngine, "SetEntrance", FnSetEntrance);
	AddFunc(pEngine, "GetEntrance", FnGetEntrance);
	AddFunc(pEngine, "SetCategory", FnSetCategory);
	AddFunc(pEngine, "FinishCommand", FnFinishCommand);
	AddFunc(pEngine, "ActIdle", FnActIdle);
	AddFunc(pEngine, "SetRDir", FnSetRDir);
	AddFunc(pEngine, "GetRDir", FnGetRDir);
	AddFunc(pEngine, "GetXDir", FnGetXDir);
	AddFunc(pEngine, "GetYDir", FnGetYDir);
	AddFunc(pEngine, "GetR", FnGetR);
	AddFunc(pEngine, "SetXDir", FnSetXDir);
	AddFunc(pEngine, "SetYDir", FnSetYDir);
	AddFunc(pEngine, "SetR", FnSetR);
	AddFunc(pEngine, "SetOwner", FnSetOwner);
	AddFunc(pEngine, "MakeCrewMember", FnMakeCrewMember);
	AddFunc(pEngine, "GrabObjectInfo", FnGrabObjectInfo);
	AddFunc(pEngine, "CreateContents", FnCreateContents);
	AddFunc(pEngine, "ShiftContents", FnShiftContents);
	AddFunc(pEngine, "ComposeContents", FnComposeContents);
	AddFunc(pEngine, "GetID", FnGetID);
	AddFunc(pEngine, "Contents", FnContents);
	AddFunc(pEngine, "ScrollContents", FnScrollContents);
	AddFunc(pEngine, "Contained", FnContained);
	AddFunc(pEngine, "ContentsCount", FnContentsCount);
	AddFunc(pEngine, "FindContents", FnFindContents, false);
	AddFunc(pEngine, "FindOtherContents", FnFindOtherContents, false);
	AddFunc(pEngine, "RemoveObject", FnRemoveObject);
	AddFunc(pEngine, "GetActionTarget", FnGetActionTarget);
	AddFunc(pEngine, "SetActionTargets", FnSetActionTargets);
	AddFunc(pEngine, "SetComponent", FnSetComponent);
	AddFunc(pEngine, "SetCrewStatus", FnSetCrewStatus, false);
	AddFunc(pEngine, "SetPosition", FnSetPosition);
	AddFunc(pEngine, "BlastObject", FnBlastObject);
	AddFunc(pEngine, "CreateMenu", FnCreateMenu);
	AddFunc(pEngine, "AddMenuItem", FnAddMenuItem);
	AddFunc(pEngine, "SelectMenuItem", FnSelectMenuItem);
	AddFunc(pEngine, "SetMenuDecoration", FnSetMenuDecoration);
	AddFunc(pEngine, "SetMenuTextProgress", FnSetMenuTextProgress);
	AddFunc(pEngine, "ObjectDistance", FnObjectDistance);
	AddFunc(pEngine, "GetValue", FnGetValue);
	AddFunc(pEngine, "GetRank", FnGetRank);
	AddFunc(pEngine, "SetTransferZone", FnSetTransferZone);
	AddFunc(pEngine, "SetMass", FnSetMass);
	AddFunc(pEngine, "GetColor", FnGetColor);
	AddFunc(pEngine, "SetColor", FnSetColor);
	AddFunc(pEngine, "SetPlrViewRange", FnSetPlrViewRange);
	AddFunc(pEngine, "SetPicture", FnSetPicture);
	AddFunc(pEngine, "GetProcedure", FnGetProcedure);
	AddFunc(pEngine, "CanConcatPictureWith", FnCanConcatPictureWith);
	AddFunc(pEngine, "SetGraphics", FnSetGraphics);
	AddFunc(pEngine, "ObjectNumber", FnObjectNumber);
	AddFunc(pEngine, "ShowInfo", FnShowInfo);
	AddFunc(pEngine, "CheckVisibility", FnCheckVisibility);
	AddFunc(pEngine, "SetClrModulation", FnSetClrModulation);
	AddFunc(pEngine, "GetClrModulation", FnGetClrModulation);
	AddFunc(pEngine, "CloseMenu", FnCloseMenu);
	AddFunc(pEngine, "GetMenuSelection", FnGetMenuSelection);
	AddFunc(pEngine, "GetDefBottom", FnGetDefBottom);
	AddFunc(pEngine, "SetMenuSize", FnSetMenuSize);
	AddFunc(pEngine, "GetCrewEnabled", FnGetCrewEnabled);
	AddFunc(pEngine, "SetCrewEnabled", FnSetCrewEnabled);
	AddFunc(pEngine, "DoCrewExp", FnDoCrewExp);
	AddFunc(pEngine, "ClearMenuItems", FnClearMenuItems);
	AddFunc(pEngine, "GetObjectLayer", FnGetObjectLayer);
	AddFunc(pEngine, "SetObjectLayer", FnSetObjectLayer);
	AddFunc(pEngine, "SetShape", FnSetShape);
	AddFunc(pEngine, "SetObjDrawTransform", FnSetObjDrawTransform);
	AddFunc(pEngine, "SetObjDrawTransform2", FnSetObjDrawTransform2, false);
	AddFunc(pEngine, "SetObjectStatus", FnSetObjectStatus, false);
	AddFunc(pEngine, "GetObjectStatus", FnGetObjectStatus, false);
	AddFunc(pEngine, "AdjustWalkRotation", FnAdjustWalkRotation, false);
	AddFunc(pEngine, "GetContact", FnGetContact);
	AddFunc(pEngine, "SetObjectBlitMode", FnSetObjectBlitMode);
	AddFunc(pEngine, "GetObjectBlitMode", FnGetObjectBlitMode);
	AddFunc(pEngine, PSF_OnOwnerRemoved, FnOnOwnerRemoved, false);
	AddFunc(pEngine, "GetUnusedOverlayID", FnGetUnusedOverlayID, false);
	AddFunc(pEngine, "ExecuteCommand", FnExecuteCommand);
	//FIXME new C4AulDefCastFunc(pEngine, "ScoreboardCol", C4V_C4ID, C4V_Int);

	AddFunc(pEngine, "PlayAnimation", FnPlayAnimation);
	AddFunc(pEngine, "StopAnimation", FnStopAnimation);
	AddFunc(pEngine, "GetRootAnimation", FnGetRootAnimation);
	AddFunc(pEngine, "GetAnimationLength", FnGetAnimationLength);
	AddFunc(pEngine, "GetAnimationName", FnGetAnimationName);
	AddFunc(pEngine, "GetAnimationPosition", FnGetAnimationPosition);
	AddFunc(pEngine, "GetAnimationWeight", FnGetAnimationWeight);
	AddFunc(pEngine, "SetAnimationPosition", FnSetAnimationPosition);
	AddFunc(pEngine, "SetAnimationWeight", FnSetAnimationWeight);
	AddFunc(pEngine, "AttachMesh", FnAttachMesh);
	AddFunc(pEngine, "DetachMesh", FnDetachMesh);
	AddFunc(pEngine, "SetAttachBones", FnSetAttachBones);
	AddFunc(pEngine, "SetAttachTransform", FnSetAttachTransform);
	AddFunc(pEngine, "GetMeshMaterial", FnGetMeshMaterial);
	AddFunc(pEngine, "SetMeshMaterial", FnSetMeshMaterial);
	AddFunc(pEngine, "ChangeDef", FnChangeDef);
	AddFunc(pEngine, "GrabContents", FnGrabContents);
	AddFunc(pEngine, "Punch", FnPunch);
	AddFunc(pEngine, "Kill", FnKill);
	AddFunc(pEngine, "Fling", FnFling);
	AddFunc(pEngine, "Jump", FnJump, false);
	AddFunc(pEngine, "Enter", FnEnter);
	AddFunc(pEngine, "DeathAnnounce", FnDeathAnnounce);
	AddFunc(pEngine, "SetSolidMask", FnSetSolidMask);
	AddFunc(pEngine, "Exit", FnExit);
	AddFunc(pEngine, "Collect", FnCollect);
	AddFunc(pEngine, "DoNoCollectDelay", FnDoNoCollectDelay);
}
