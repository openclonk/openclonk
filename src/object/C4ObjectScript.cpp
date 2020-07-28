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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"

#include "control/C4Teams.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameMessage.h"
#include "landscape/C4Material.h"
#include "landscape/C4Particles.h"
#include "lib/C4Random.h"
#include "lib/StdMeshMath.h"
#include "object/C4Command.h"
#include "object/C4DefList.h"
#include "object/C4MeshAnimation.h"
#include "object/C4MeshDenumerator.h"
#include "object/C4ObjectCom.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectMenu.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4RankSystem.h"
#include "script/C4Aul.h"
#include "script/C4AulDefFunc.h"

bool C4ValueToMatrix(C4Value& value, StdMeshMatrix* matrix)
{
	const C4ValueArray* array = value.getArray();
	if (!array)
	{
		return false;
	}
	return C4ValueToMatrix(*array, matrix);
}

bool C4ValueToMatrix(const C4ValueArray& array, StdMeshMatrix* matrix)
{
	if (array.GetSize() != 12)
	{
		return false;
	}

	StdMeshMatrix& trans = *matrix;
	trans(0, 0) =  array[0].getInt() / 1000.0f;
	trans(0, 1) =  array[1].getInt() / 1000.0f;
	trans(0, 2) =  array[2].getInt() / 1000.0f;
	trans(0, 3) =  array[3].getInt() / 1000.0f;
	trans(1, 0) =  array[4].getInt() / 1000.0f;
	trans(1, 1) =  array[5].getInt() / 1000.0f;
	trans(1, 2) =  array[6].getInt() / 1000.0f;
	trans(1, 3) =  array[7].getInt() / 1000.0f;
	trans(2, 0) =  array[8].getInt() / 1000.0f;
	trans(2, 1) =  array[9].getInt() / 1000.0f;
	trans(2, 2) = array[10].getInt() / 1000.0f;
	trans(2, 3) = array[11].getInt() / 1000.0f;

	return true;
}

static bool FnChangeDef(C4Object *Obj, C4ID to_id)
{
	return !!Obj->ChangeDef(to_id);
}

static void FnSetSolidMask(C4Object *Obj, long iX, long iY, long iWdt, long iHgt, long iTX, long iTY)
{
	Obj->SetSolidMask(iX,iY,iWdt,iHgt,iTX,iTY);
}

static void FnSetHalfVehicleSolidMask(C4Object *Obj, bool set)
{
	Obj->SetHalfVehicleSolidMask(set);
}

static void FnDeathAnnounce(C4Object *Obj)
{
	const long MaxDeathMsg=7;
	if (Game.C4S.Head.Film)
	{
		return;
	}
	// Check if crew member has an own death message
	if (Obj->Info && *(Obj->Info->DeathMessage))
	{
		GameMsgObject(Obj->Info->DeathMessage, Obj);
	}
	else
	{
		char idDeathMsg[128+1];
		sprintf(idDeathMsg, "IDS_OBJ_DEATH%d", 1 + UnsyncedRandom(MaxDeathMsg));
		GameMsgObject(FormatString(LoadResStr(idDeathMsg), Obj->GetName()).getData(), Obj);
	}
}

static bool FnGrabContents(C4Object *Obj, C4Object *from)
{
	if (!from || Obj == from)
	{
		return false;
	}
	Obj->GrabContents(from);
	return true;
}

static bool FnPunch(C4Object *Obj, C4Object *target, long punch)
{
	if (!target)
	{
		return false;
	}
	return !!ObjectComPunch(Obj, target, punch);
}

static bool FnKill(C4PropList * _this, C4Object *pObj, bool fForced)
{
	if (!pObj)
	{
		pObj = Object(_this);
	}
	if (!pObj || !pObj->GetAlive())
	{
		return false;
	}
	// Trace kills by player-owned objects
	// Do not trace for NO_OWNER, because that would include e.g. the Suicide-rule
	if (Object(_this) && ValidPlr(Object(_this)->Controller))
	{
		pObj->UpdatLastEnergyLossCause(Object(_this)->Controller);
	}
	// Do the kill
	pObj->AssignDeath(!!fForced);
	return true;
}

static void FnFling(C4Object *Obj, long iXDir, long iYDir, long iPrec, bool fAddSpeed)
{
	if (!iPrec)
	{
		iPrec = 1;
	}
	Obj->Fling(itofix(iXDir, iPrec), itofix(iYDir, iPrec), fAddSpeed);
	// unstick from ground, because Fling command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	Obj->Action.t_attach = 0;
}

static bool FnJump(C4Object *Obj)
{
	return !!ObjectComJump(Obj);
}

static bool FnEnter(C4Object *Obj, C4Object *pTarget)
{
	return !!Obj->Enter(pTarget, true, true, nullptr);
}

static bool FnExit(C4Object *Obj, long tx, long ty, long tr, long txdir, long tydir, long trdir)
{
	tx += Obj->GetX();
	ty += Obj->GetY();
	ObjectComCancelAttach(Obj);
	return !!Obj->Exit(tx,
	                   ty+Obj->Shape.y,
	                   tr,
	                   itofix(txdir),itofix(tydir),
	                   itofix(trdir) / 10);
}

static bool FnCollect(C4Object *Obj, C4Object *pItem, bool ignoreOCF)
{
	// Local call / safety
	if (!pItem)
	{
		return false;
	}
	// Check OCF of collector (MaxCarry)
	if ((Obj->OCF & OCF_Collection) || ignoreOCF)
	{
		// Collect
		return !!Obj->Collect(pItem);
	}
	// Failure
	return false;
}

static void FnRemoveObject(C4Object *Obj, bool fEjectContents)
{
	Obj->AssignRemoval(fEjectContents);
}

static void FnSetPosition(C4Object *Obj, long iX, long iY, bool fCheckBounds, long iPrec)
{
	if (!iPrec)
	{
		iPrec = 1;
	}
	C4Real i_x = itofix(iX, iPrec);
	C4Real i_y = itofix(iY, iPrec);
	if (fCheckBounds)
	{
		// BoundsCheck takes ref to C4Real and not to long
		Obj->BoundsCheck(i_x, i_y);
	}
	Obj->ForcePosition(i_x, i_y);
	// Update liquid
	Obj->UpdateInLiquid();
}

static void FnDoCon(C4Object *Obj, long iChange, long iPrec, bool bGrowFromCenter)
{
	if (!iPrec)
	{
		iPrec = 100;
	}
	Obj->DoCon(FullCon*iChange / iPrec, bGrowFromCenter);
}

static long FnGetCon(C4Object *Obj, long iPrec)
{
	if (!iPrec)
	{
		iPrec = 100;
	}
	return iPrec * Obj->GetCon() / FullCon;
}

static bool FnSetName(C4PropList * _this, C4String *pNewName, bool fSetInInfo, bool fMakeValidIfExists)
{
	if (!Object(_this))
	{
		if (!_this)
		{
			throw NeedNonGlobalContext("SetName");
		}
		else if (fSetInInfo)
		{
			return false;
		}
		// Definition name
		_this->SetName(FnStringPar(pNewName));
		return true;
	}
	else
	{
		// Object name
		if (fSetInInfo)
		{
			// Setting name in info
			C4ObjectInfo *pInfo = Object(_this)->Info;
			if (!pInfo)
			{
				return false;
			}
			const char *szName = pNewName->GetCStr();
			// Empty names are bad; e.g., could cause problems in savegames
			if (!szName || !*szName)
			{
				return false;
			}
			// Name must not be too long
			if (std::strlen(szName) > C4MaxName)
			{
				return false;
			}
			// Any change at all?
			if (SEqual(szName, pInfo->Name))
			{
				return true;
			}
			// Make sure names in info list aren't duplicated
			// querying owner info list here isn't 100% accurate, as infos might have been stolen by other players
			// however, there is no good way to track the original list ATM
			C4ObjectInfoList *pInfoList = nullptr;
			C4Player *pOwner = ::Players.Get(Object(_this)->Owner);
			if (pOwner)
			{
				pInfoList = &pOwner->CrewInfoList;
			}
			char NameBuf[C4MaxName+1];
			if (pInfoList && pInfoList->NameExists(szName))
			{
				if (!fMakeValidIfExists)
				{
					return false;
				}
				SCopy(szName, NameBuf, C4MaxName);
				pInfoList->MakeValidName(NameBuf);
				szName = NameBuf;
			}
			SCopy(szName, pInfo->Name, C4MaxName);
			Object(_this)->SetName(); // make sure object uses info name
			Object(_this)->Call(PSF_NameChange,&C4AulParSet(true));
		}
		else
		{
			if (!pNewName)
			{
				Object(_this)->SetName();
			}
			else
			{
				Object(_this)->SetName(pNewName->GetCStr());
			}
			Object(_this)->Call(PSF_NameChange, &C4AulParSet(false));
		}
	}
	return true;
}

static C4Value FnSetCrewExtraData(C4Object *Obj, C4String * DataName, const C4Value & Data)
{
	const char *strDataName = FnStringPar(DataName);
	// valid crew with info? (for great nullpointer prevention)
	if (!Obj->Info)
	{
		return C4Value();
	}
	// Do not allow data type C4V_Array or C4V_C4Object
	if (Data.GetType() != C4V_Nil
	&&  Data.GetType() != C4V_Int
	&&  Data.GetType() != C4V_Bool
	&&  Data.GetType() != C4V_String)
	{
		return C4VNull;
	}
	// Get pointer on info...
	C4ObjectInfo *pInfo = Obj->Info;
	// no name list created yet?
	if (!pInfo->ExtraData.pNames)
	{
		// Create name list
		pInfo->ExtraData.CreateTempNameList();
	}
	// data name already exists?
	long ival;
	if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) != -1)
	{
		pInfo->ExtraData[ival] = Data;
	}
	else
	{
		// Add name
		pInfo->ExtraData.pNames->AddName(strDataName);
		// Get val id & set
		if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1)
		{
			return C4Value();
		}
		pInfo->ExtraData[ival] = Data;
	}
	// Ok, return the value that has been set
	return Data;
}

static C4Value FnGetCrewExtraData(C4Object *Obj, C4String * DataName)
{
	const char *strDataName = FnStringPar(DataName);
	// Valid crew with info?
	if (!Obj->Info)
	{
		return C4Value();
	}
	// Get pointer on info...
	C4ObjectInfo *pInfo = Obj->Info;
	// No name list?
	if (!pInfo->ExtraData.pNames)
	{
		return C4Value();
	}
	long ival = pInfo->ExtraData.pNames->GetItemNr(strDataName);
	if (ival == -1)
	{
		return C4Value();
	}
	// return data
	return pInfo->ExtraData[ival];
}

static void FnDoEnergy(C4Object *Obj, long iChange, bool fExact, Nillable<long> iEngType, Nillable<long> iCausedBy)
{
	if (iEngType.IsNil())
	{
		iEngType = C4FxCall_EngScript;
	}
	if (iCausedBy.IsNil())
	{
		iCausedBy = NO_OWNER;
	}
	Obj->DoEnergy(iChange, fExact, iEngType, iCausedBy);
}

static void FnDoBreath(C4Object *Obj, long iChange)
{
	Obj->DoBreath(iChange);
}

static void FnDoDamage(C4Object *Obj, long iChange, Nillable<long> iDmgType, Nillable<long> iCausedBy)
{
	if (iDmgType.IsNil())
	{
		iDmgType = C4FxCall_DmgScript;
	}
	if (iCausedBy.IsNil())
	{
		iCausedBy = NO_OWNER;
	}
	Obj->DoDamage(iChange, iCausedBy, iDmgType);
}

static void FnSetEntrance(C4Object *Obj, bool e_status)
{
	Obj->EntranceStatus = e_status;
}


static void FnSetXDir(C4Object *Obj, long nxdir, long iPrec)
{
	// Precision (default 10.0)
	if (!iPrec)
	{
		iPrec = 10;
	}
	// Update xdir
	Obj->xdir = itofix(nxdir, iPrec);
	Obj->Mobile = true;
}

static void FnSetRDir(C4Object *Obj, long nrdir, long iPrec)
{
	// Precision (default 10.0)
	if (!iPrec)
	{
		iPrec = 10;
	}
	// Update rdir
	Obj->rdir = itofix(nrdir, iPrec);
	Obj->Mobile = true;
}

static void FnSetYDir(C4Object *Obj, long nydir, long iPrec)
{
	// Precision (default 10.0)
	if (!iPrec)
	{
		iPrec = 10;
	}
	// Update ydir
	Obj->ydir = itofix(nydir, iPrec);
	Obj->Mobile = true;
}

static void FnSetR(C4Object *Obj, long nr)
{
	Obj->SetRotation(nr);
}

static bool FnSetAction(C4Object *Obj, C4String *szAction,
                        C4Object *pTarget, C4Object *pTarget2, bool fDirect)
{
	if (!szAction)
	{
		return false;
	}
	return !!Obj->SetActionByName(FnStringPar(szAction), pTarget, pTarget2,
	                              C4Object::SAC_StartCall | C4Object::SAC_AbortCall, !!fDirect);
}

static bool FnSetActionData(C4Object *Obj, long iData)
{
	if (!Obj->Status)
	{
		return false;
	}
	C4PropList* pActionDef = Obj->GetAction();
	// Attach: check for valid vertex indices
	if (pActionDef && (pActionDef->GetPropertyP(P_Procedure) == DFA_ATTACH))
	{
		if (((iData&255) >= C4D_MaxVertex) || ((iData>>8) >= C4D_MaxVertex))
		{
			return false;
		}
	}
	// set data
	Obj->Action.Data = iData;
	return true;
}

static void FnSetComDir(C4Object *Obj, long ncomdir)
{
	Obj->Action.ComDir=ncomdir;
}

static void FnSetDir(C4Object *Obj, long ndir)
{
	Obj->SetDir(ndir);
}

static void FnSetCategory(C4Object *Obj, long iCategory)
{
	Obj->SetCategory(iCategory);
}

static void FnSetAlive(C4Object *Obj, bool nalv)
{
	Obj->SetAlive(nalv);
}

static bool FnSetOwner(C4Object *Obj, long iOwner)
{
	// Set owner
	return !!Obj->SetOwner(iOwner);
}

static bool FnSetPhase(C4Object *Obj, long iVal)
{
	return !!Obj->SetPhase(iVal);
}

static bool FnExecuteCommand(C4Object *Obj)
{
	return !!Obj->ExecuteCommand();
}

static bool FnSetCommand(C4Object *Obj, C4String * szCommand, C4Object * pTarget,
                         const C4Value & Tx, int iTy, C4Object * pTarget2,
                         const C4Value & Data, int iRetries)
{
	// Command
	if (!szCommand)
	{
		return false;
	}
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand)
	{
		Obj->ClearCommands();
		return false;
	}
	// Special: convert iData to szText
	C4String *szText = nullptr;
	if (iCommand == C4CMD_Call)
	{
		szText=Data.getStr();
	}
	// FIXME: throw if Tx isn't int
	// Set
	Obj->SetCommand(iCommand, pTarget, Tx, iTy, pTarget2, false, Data, iRetries, szText);
	// Success
	return true;
}

static bool FnAddCommand(C4Object *Obj, C4String * szCommand, C4Object * pTarget,
                            const C4Value & Tx, int iTy, C4Object * pTarget2,
                            int iUpdateInterval, const C4Value & Data, int iRetries, int iBaseMode)
{
	// Command
	if (!szCommand)
	{
		return false;
	}
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand)
	{
		return false;
	}
	// Special: convert iData to szText
	C4String *szText = nullptr;
	if (iCommand == C4CMD_Call)
	{
		szText=Data.getStr();
	}
	// Add
	return Obj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,true,Data,false,iRetries,szText,iBaseMode);
}

static bool FnAppendCommand(C4Object *Obj, C4String * szCommand, C4Object * pTarget,
                               const C4Value & Tx, int iTy, C4Object * pTarget2,
                               int iUpdateInterval, const C4Value & Data, int iRetries, int iBaseMode)
{
	// Command
	if (!szCommand)
	{
		return false;
	}
	long iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand)
	{
		return false;
	}
	// Special: convert iData to szText
	C4String *szText = nullptr;
	if (iCommand == C4CMD_Call)
	{
		szText=Data.getStr();
	}
	// Add
	return Obj->AddCommand(iCommand,pTarget,Tx,iTy,iUpdateInterval,pTarget2,true,Data,true,iRetries,szText,iBaseMode);
}

static C4Value FnGetCommand(C4Object *Obj, int iElement, int iCommandNum)
{
	C4Command * Command = Obj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--)
	{
		Command = Command->Next;
	}
	// Object has no command or iCommandNum was to high or < 0
	if (!Command)
	{
		return C4VNull;
	}
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

static bool FnFinishCommand(C4Object *Obj, bool fSuccess, long iCommandNum)
{
	C4Command * Command = Obj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--)
	{
		Command = Command->Next;
	}
	// Object has no command or iCommandNum was to high or < 0
	if (!Command)
	{
		return false;
	}
	if (!fSuccess)
	{
		++(Command->Failures);
	}
	else
	{
		Command->Finished = true;
	}
	return true;
}

static C4String *FnGetAction(C4Object *Obj)
{
	C4PropList* pActionDef = Obj->GetAction();
	if (!pActionDef)
	{
		return String("Idle");
	}
	return String(pActionDef->GetName());
}

static C4Object *FnGetActionTarget(C4Object *Obj, long target_index)
{
	if (target_index==0) return Obj->Action.Target;
	if (target_index==1) return Obj->Action.Target2;
	return nullptr;
}

static void FnSetActionTargets(C4Object *Obj, C4Object *pTarget1, C4Object *pTarget2)
{
	// set targets
	Obj->Action.Target = pTarget1;
	Obj->Action.Target2 = pTarget2;
}

static long FnGetDir(C4Object *Obj)
{
	return Obj->Action.Dir;
}

static bool FnGetEntrance(C4Object *Obj)
{
	return Obj->EntranceStatus;
}

static long FnGetPhase(C4Object *Obj)
{
	return Obj->Action.Phase;
}

static long FnGetEnergy(C4Object *Obj, bool fExact)
{
	if (fExact)
	{
		return Obj->Energy;
	}
	else
	{
		return 100 * Obj->Energy / C4MaxPhysical;
	}
}

static long FnGetBreath(C4Object *Obj)
{
	return Obj->Breath;
}

static long FnGetMass(C4PropList * _this)
{
	if (!Object(_this))
	{
		if (!_this || !_this->GetDef())
		{
			throw NeedNonGlobalContext("GetMass");
		}
		else
		{
			return _this->GetDef()->Mass;
		}
	}
	else
	{
		return Object(_this)->Mass;
	}
}

static long FnGetRDir(C4Object *Obj, long iPrec)
{
	if (!iPrec)
	{
		iPrec = 10;
	}
	return fixtoi(Obj->rdir, iPrec);
}

static long FnGetXDir(C4Object *Obj, long iPrec)
{
	if (!iPrec)
	{
		iPrec = 10;
	}
	return fixtoi(Obj->xdir, iPrec);
}

static long FnGetYDir(C4Object *Obj, long iPrec)
{
	if (!iPrec)
	{
		iPrec = 10;
	}
	return fixtoi(Obj->ydir, iPrec);
}

static long FnGetR(C4Object *Obj)
{
	// Adjust range
	long iR = Obj->GetR();
	while (iR > 180)
	{
		iR -= 360;
	}
	while (iR < -180)
	{
		iR += 360;
	}
	return iR;
}

static long FnGetComDir(C4Object *Obj)
{
	return Obj->Action.ComDir;
}

static long FnGetVertexNum(C4Object *Obj)
{
	return Obj->Shape.VtxNum;
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

static Nillable<long> FnGetVertex(C4Object *Obj, long iIndex, long iValueToGet)
{
	if (Obj->Shape.VtxNum < 1)
	{
		return C4Void();
	}
	if (iIndex < 0 || iIndex >= Obj->Shape.VtxNum)
	{
		return C4Void();
	}
	iIndex=std::min<long>(iIndex,Obj->Shape.VtxNum-1);
	switch (static_cast<VertexDataIndex>(iValueToGet))
	{
	case VTX_X:
		return Obj->Shape.VtxX[iIndex];
	case VTX_Y:
		return Obj->Shape.VtxY[iIndex];
	case VTX_CNAT:
		return Obj->Shape.VtxCNAT[iIndex];
	case VTX_Friction:
		return Obj->Shape.VtxFriction[iIndex];
	default:
		DebugLog(FormatString("GetVertex: Unknown vertex attribute: %ld", iValueToGet).getData());
		return C4Void();
	}
	// Impossible mayhem!
	assert(!"FnGetVertex: unreachable code reached");
	return C4Void();
}

static bool FnSetVertex(C4Object *Obj, long iIndex, long iValueToSet, long iValue, long iOwnVertexMode)
{
	// Own vertex mode?
	if (iOwnVertexMode)
	{
		// Enter own custom vertex mode if not already set
		if (!Obj->fOwnVertices)
		{
			Obj->Shape.CreateOwnOriginalCopy(Obj->Def->Shape);
			Obj->fOwnVertices = true;
		}
		// Set vertices at end of buffer
		iIndex += C4D_VertexCpyPos;
	}
	// Range check
	if (!Inside<long>(iIndex,0,C4D_MaxVertex-1)) return false;
	// Set desired value
	switch (static_cast<VertexDataIndex>(iValueToSet))
	{
	case VTX_X:
		Obj->Shape.VtxX[iIndex] = iValue;
		break;
	case VTX_Y:
		Obj->Shape.VtxY[iIndex] = iValue;
		break;
	case VTX_CNAT:
		Obj->Shape.VtxCNAT[iIndex] = iValue;
		break;
	case VTX_Friction:
		Obj->Shape.VtxFriction[iIndex] = iValue;
		break;
	default:
		DebugLogF("SetVertex: Unknown vertex attribute: %ld", iValueToSet);
		return false;
	}
	// Vertex update desired?
	if (iOwnVertexMode == VTX_SetPermanentUpd)
	{
		Obj->UpdateShape(true);
	}
	return true;
}

static bool FnAddVertex(C4Object *Obj, long iX, long iY)
{
	return !!Obj->Shape.AddVertex(iX, iY);
}

static bool FnInsertVertex(C4Object *Obj, long iIndex, long iX, long iY)
{
	return !!Obj->Shape.InsertVertex(iIndex, iX, iY);
}

static bool FnRemoveVertex(C4Object *Obj, long iIndex)
{
	return !!Obj->Shape.RemoveVertex(iIndex);
}

static void FnSetContactDensity(C4Object *Obj, long iDensity)
{
	Obj->Shape.ContactDensity = iDensity;
}

static bool FnGetAlive(C4Object *Obj)
{
	return Obj->GetAlive();
}

static long FnGetOwner(C4Object *Obj)
{
	return Obj->Owner;
}

static long FnGetController(C4Object *Obj)
{
	return Obj->Controller;
}

static bool FnSetController(C4Object *Obj, long iNewController)
{
	// validate player
	if (iNewController != NO_OWNER && !ValidPlr(iNewController))
	{
		return false;
	}
	// Set controller
	Obj->Controller = iNewController;
	return true;
}

static long FnGetKiller(C4Object *Obj)
{
	return Obj->LastEnergyLossCausePlayer;
}

static bool FnSetKiller(C4Object *Obj, long iNewKiller)
{
	// Validate player
	if (iNewKiller != NO_OWNER && !ValidPlr(iNewKiller))
	{
		return false;
	}
	// Set killer as last energy loss cause
	Obj->LastEnergyLossCausePlayer = iNewKiller;
	return true;
}

static long FnGetCategory(C4PropList * _this)
{
	if (!Object(_this))
	{
		if (!_this || !_this->GetDef())
		{
			throw NeedNonGlobalContext("GetCategory");
		}
		else
		{
			return _this->GetDef()->Category;
		}
	}
	else
	{
		return Object(_this)->Category;
	}
}

static long FnGetOCF(C4Object *Obj)
{
	return Obj->OCF;
}

static long FnGetDamage(C4Object *Obj)
{
	return Obj->Damage;
}

static long FnGetValue(C4PropList * _this, C4Object *pInBase, long iForPlayer)
{
	if (!Object(_this))
	{
		if (!_this || !_this->GetDef())
		{
			throw NeedNonGlobalContext("GetValue");
		}
		else
		{
			return _this->GetDef()->GetValue(pInBase, iForPlayer);
		}
	}
	else
	{
		return Object(_this)->GetValue(pInBase, iForPlayer);
	}
}

static long FnGetRank(C4Object *Obj)
{
	if (!Obj->Info)
	{
		return 0;
	}
	return Obj->Info->Rank;
}

static long FnGetActTime(C4Object *Obj)
{
	return Obj->Action.Time;
}

static C4PropList* FnGetID(C4Object *Obj)
{
	// Return id of object
	return Obj->GetPrototype();
}

static Nillable<C4Def*> FnGetMenu(C4Object *Obj)
{
	if (Obj->Menu && Obj->Menu->IsActive())
	{
		return C4Id2Def(C4ID(Obj->Menu->GetIdentification()));
	}
	return C4Void();
}

static bool FnCreateMenu(C4Object *Obj, C4Def *pDef, C4Object *pCommandObj,
                         long iExtra, C4String *szCaption, long iExtraData,
                         long iStyle, bool fPermanent, C4ID idMenuID)
{
	// Object menu: Validate object
	if (pCommandObj && !pCommandObj->Status)
	{
		return false;
	}
	// else scenario script callback: No command object OK

	// Create symbol
	C4FacetSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize,C4SymbolSize);
	if (pDef)
	{
		pDef->Draw(fctSymbol);
	}

	// Clear any old menu, init new menu
	if (!Obj->CloseMenu(false))
	{
		return false;
	}
	if (!Obj->Menu)
	{
		Obj->Menu = new C4ObjectMenu;
	}
	else
	{
		Obj->Menu->ClearItems();
	}
	Obj->Menu->Init(fctSymbol, FnStringPar(szCaption), pCommandObj, iExtra, iExtraData, (idMenuID ? idMenuID : pDef ? pDef->id : C4ID::None).GetHandle(), iStyle, true);

	// Set permanent
	Obj->Menu->SetPermanent(fPermanent);

	return true;
}

const int C4MN_Add_ImgRank         =   1,
          C4MN_Add_ImgIndexed      =   2,
          C4MN_Add_ImgObjRank      =   3,
          C4MN_Add_ImgObject       =   4,
          C4MN_Add_ImgTextSpec     =   5,
          C4MN_Add_ImgColor        =   6,
          C4MN_Add_ImgPropListSpec =   7,
          C4MN_Add_MaxImage        = 127, // mask for param which decides what to draw as the menu symbol
          C4MN_Add_PassValue       = 128,
          C4MN_Add_ForceCount      = 256,
          C4MN_Add_ForceNoDesc     = 512;

#ifndef _MSC_VER
#define _snprintf snprintf
#endif

static bool FnAddMenuItem(C4Object *Obj, C4String * szCaption, C4String * szCommand, C4Def * pDef, int iCount, const C4Value & Parameter, C4String * szInfoCaption, int iExtra, const C4Value & XPar, const C4Value & XPar2)
{
	if (!Obj->Menu)
	{
		return false;
	}

	char caption[256+1];
	char parameter[256+1];
	char dummy[256+1];
	char command[512+1];
	char command2[512+1];
	char infocaption[C4MaxTitle+1];

	// Get needed symbol size
	int iSymbolSize = Obj->Menu->GetSymbolSize();

	// Compose caption with def name
	if (szCaption)
	{
		const char * s = FnStringPar(szCaption);
		const char * sep = strstr(s, "%s");
		if (sep && pDef)
		{
			strncpy(caption, s, std::min<intptr_t>(sep - s,256));
			caption[std::min<intptr_t>(sep - s,256)] = 0;
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
	{
		caption[0] = 0;
	}

	// Create string to include type-information in command
	switch (Parameter.GetType())
	{
	case C4V_Int:
		sprintf(parameter, "%d", Parameter.getInt());
		break;
	case C4V_Bool:
		SCopy(Parameter.getBool() ? "true" : "false", parameter);
		break;
	case C4V_PropList:
		if (Parameter.getPropList()->GetObject())
		{
			sprintf(parameter, "Object(%d)", Parameter.getPropList()->GetObject()->Number);
		}
		else if (Parameter.getPropList()->GetDef())
		{
			sprintf(parameter, R"(C4Id("%s"))", Parameter.getPropList()->GetDef()->id.ToString());
		}
		else
		{
			throw C4AulExecError("proplist as parameter to AddMenuItem");
		}
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
		throw C4AulExecError("array as parameter to AddMenuItem");
	default:
		return false;
	}

	// Own value
	bool fOwnValue = false;
	long iValue=0;
	if (iExtra & C4MN_Add_PassValue)
	{
		fOwnValue = true;
		iValue = XPar2.getInt();
	}

	// New Style: native script command
	size_t i = 0;
	for (; i < SLen(FnStringPar(szCommand)); i++)
	{
		if (!IsIdentifier(FnStringPar(szCommand)[i]))
		{
			break;
		}
	}
	if (i < SLen(FnStringPar(szCommand)))
	{
		// Search for "%d" an replace it by "%s" for insertion of formatted parameter
		SCopy(FnStringPar(szCommand), dummy, 256);
		auto* pFound = const_cast<char*>(SSearch(dummy, "%d"));
		if (pFound != nullptr)
		{
			*(pFound - 1) = 's';
		}
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
				sprintf(command, "%s(%s,%s,0,%ld)", szScriptCom, pDef ? pDef->id.ToString() : "nil", parameter, iValue);
				sprintf(command2, "%s(%s,%s,1,%ld)", szScriptCom, pDef ? pDef->id.ToString() : "nil", parameter, iValue);
			}
			else
			{
				// without value
				sprintf(command, "%s(%s,%s)", szScriptCom, pDef ? pDef->id.ToString() : "nil", parameter);
				sprintf(command2, "%s(%s,%s,1)", szScriptCom, pDef ? pDef->id.ToString() : "nil", parameter);
			}
		}
		else
		{
			// no command
			*command = *command2 = '\0';
		}
	}

	// Info caption
	SCopy(FnStringPar(szInfoCaption), infocaption, C4MaxTitle);

	// Create symbol
	C4FacetSurface fctSymbol;
	C4DefGraphics* pGfx = nullptr;
	C4Object* pGfxObj = nullptr;
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
		iCount = 0;
		break;
	}
	case C4MN_Add_ImgIndexed:
		// draw indexed facet
		fctSymbol.Create(iSymbolSize,iSymbolSize);
		if (pDef)
		{
			pDef->Draw(fctSymbol, false, 0, nullptr, XPar.getInt());
		}
		break;
	case C4MN_Add_ImgObjRank:
	{
		// draw current gfx of XPar_C4V including rank
		if (!XPar.CheckConversion(C4V_Object))
		{
			return false;
		}
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
			if (Obj->Menu->IsContextMenu())
			{
				// context menu entry: left object gfx
				long C4MN_SymbolSize = Obj->Menu->GetItemHeight();
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
		if (!XPar.CheckConversion(C4V_Object))
			throw C4AulExecError(FormatString(R"(call to "%s" parameter %d: got "%s", but expected "%s"!)",
			                                      "AddMenuItem", 8, XPar.GetTypeName(), GetC4VName(C4V_Object)
			                                     ).getData());
		pGfxObj = XPar.getObj();
	}
	break;

	case C4MN_Add_ImgTextSpec:
	{
		C4Def* pDef = C4Id2Def(C4ID(std::string(caption)));
		if(pDef)
		{
			pGfx = &pDef->Graphics;
		}
		else
		{
			fctSymbol.Create(iSymbolSize,iSymbolSize);
			uint32_t dwClr = XPar.getInt();
			if (!szCaption || !Game.DrawTextSpecImage(fctSymbol, caption, nullptr, dwClr ? dwClr : 0xff))
				return false;
		}
		*caption = '\0';
	}
	break;

	case C4MN_Add_ImgPropListSpec:
	{
		C4PropList *gfx_proplist = XPar.getPropList();
		fctSymbol.Create(iSymbolSize,iSymbolSize);
		if (!Game.DrawPropListSpecImage(fctSymbol, gfx_proplist))
			return false;
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
	if(pGfxObj)
		Obj->Menu->Add(caption,pGfxObj,command,iCount,nullptr,infocaption,pDef ? pDef->id : C4ID::None,command2,fOwnValue,iValue,fIsSelectable);
	else if(pGfx)
		Obj->Menu->Add(caption,pGfx,command,iCount,nullptr,infocaption,pDef ? pDef->id : C4ID::None,command2,fOwnValue,iValue,fIsSelectable);
	else
		Obj->Menu->Add(caption,fctSymbol,command,iCount,nullptr,infocaption,pDef ? pDef->id : C4ID::None,command2,fOwnValue,iValue,fIsSelectable);

	return true;
}

static bool FnSelectMenuItem(C4Object *Obj, long iItem)
{
	if (!Obj->Menu)
	{
		return false;
	}
	return !!Obj->Menu->SetSelection(iItem, false, true);
}

static bool FnSetMenuDecoration(C4Object *Obj, C4ID idNewDeco)
{
	if (!Obj->Menu)
	{
		return false;
	}
	C4GUI::FrameDecoration *pNewDeco = new C4GUI::FrameDecoration();
	if (!pNewDeco->SetByDef(idNewDeco))
	{
		delete pNewDeco;
		return false;
	}
	Obj->Menu->SetFrameDeco(pNewDeco);
	return true;
}

static bool FnSetMenuTextProgress(C4Object *Obj, long iNewProgress)
{
	if (!Obj->Menu)
	{
		return false;
	}
	return Obj->Menu->SetTextProgress(iNewProgress, false);
}


// Check / Status

static C4Object *FnContained(C4Object *Obj)
{
	return Obj->Contained;
}

static C4Object *FnContents(C4Object *Obj, long index)
{
	// Special: objects attaching to another object
	//          cannot be accessed by FnContents
	C4Object *cobj;
	while ((cobj = Obj->Contents.GetObject(index++)))
	{
		if (cobj->GetProcedure() != DFA_ATTACH)
		{
			return cobj;
		}
	}

	return nullptr;
}

static bool FnShiftContents(C4Object *Obj, bool fShiftBack, C4Def * idTarget, bool fDoCalls)
{
	// regular shift
	if (!idTarget)
	{
		return !!Obj->ShiftContents(fShiftBack, fDoCalls);
	}
	// check if ID is present within target
	C4Object *pNewFront = Obj->Contents.Find(idTarget);
	if (!pNewFront)
	{
		return false;
	}
	// select it
	Obj->DirectComContents(pNewFront, fDoCalls);
	// done, success
	return true;
}

static C4Object *FnScrollContents(C4Object *Obj)
{
	C4Object *pMove = Obj->Contents.GetObject();
	if (pMove)
	{
		Obj->Contents.Remove(pMove);
		Obj->Contents.Add(pMove,C4ObjectList::stNone);
	}

	return Obj->Contents.GetObject();
}

static long FnContentsCount(C4Object *Obj, C4ID id)
{
	return Obj->Contents.ObjectCount(id);
}

static C4Object *FnFindContents(C4Object *Obj, C4Def * c_id)
{
	return Obj->Contents.Find(c_id);
}

static C4Object *FnFindOtherContents(C4Object *Obj, C4ID c_id)
{
	return Obj->Contents.FindOther(c_id);
}

static bool FnActIdle(C4Object *Obj)
{
	return !Obj->GetAction();
}

static bool FnStuck(C4Object *Obj, long off_x, long off_y)
{
	return !!Obj->Shape.CheckContact(Obj->GetX() + off_x, Obj->GetY() + off_y);
}

static bool FnInLiquid(C4Object *Obj)
{
	return Obj->InLiquid;
}

static bool FnOnFire(C4Object *Obj)
{
	if (Obj->GetOnFire())
	{
		return true;
	}
	// check for effect
	if (!Obj->pEffects)
	{
		return false;
	}
	return !!Obj->pEffects->Get(C4Fx_AnyFire);
}

static C4Object *FnCreateContents(C4Object *Obj, C4PropList * PropList, Nillable<long> iCount)
{
	// default amount parameter
	if (iCount.IsNil())
	{
		iCount = 1;
	}
	// create objects
	C4Object *pNewObj = nullptr;
	while (iCount-- > 0)
	{
		pNewObj = Obj->CreateContents(PropList);
	}
	// controller will automatically be set upon entrance
	// return last created
	return pNewObj;
}

static bool FnMakeCrewMember(C4Object *Obj, long iPlayer)
{
	if (!ValidPlr(iPlayer))
	{
		return false;
	}
	return !!::Players.Get(iPlayer)->MakeCrewMember(Obj);
}

static bool FnGrabObjectInfo(C4Object *Obj, C4Object *pFrom)
{
	// local call, safety
	if (!pFrom)
	{
		return false;
	}
	// grab info
	return !!Obj->GrabInfo(pFrom);
}

static bool FnSetCrewStatus(C4Object *Obj, long iPlr, bool fInCrew)
{
	// validate player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr)
	{
		return false;
	}
	// set crew status
	return !!pPlr->SetObjectCrewStatus(Obj, fInCrew);
}

static long FnSetTransferZone(C4Object *Obj, long iX, long iY, long iWdt, long iHgt)
{
	iX += Obj->GetX();
	iY += Obj->GetY();
	return Game.TransferZones.Set(iX, iY, iWdt, iHgt, Obj);
}

static long FnObjectDistance(C4PropList * _this, C4Object *pObj2, C4Object *pObj)
{
	if (!pObj)
	{
		pObj = Object(_this);
	}
	if (!pObj || !pObj2)
	{
		return 0;
	}
	return Distance(pObj->GetX(), pObj->GetY(), pObj2->GetX(), pObj2->GetY());
}

static long FnObjectNumber(C4Object *Obj)
{
	return Obj->Number;
	// See FnObject
}

static long FnShowInfo(C4Object *Obj, C4Object *pObj)
{
	if (!pObj)
	{
		pObj = Obj;
	}
	if (!pObj)
	{
		return false;
	}
	return Obj->ActivateMenu(C4MN_Info,0,0,0,pObj);
}

static void FnSetMass(C4Object *Obj, long iValue)
{
	Obj->OwnMass = iValue-Obj->Def->Mass;
	Obj->UpdateMass();
}

static long FnGetColor(C4Object *Obj)
{
	return Obj->Color;
}

static void FnSetColor(C4Object *Obj, long iValue)
{
	Obj->Color = iValue;
	Obj->UpdateGraphics(false);
	Obj->UpdateFace(false);
}

static void FnSetLightRange(C4Object *Obj, long iRange, Nillable<long> iFadeoutRange)
{
	if (iFadeoutRange.IsNil())
	{
		if (iRange == 0)
		{
			iFadeoutRange = 0;
		}
		else
		{
			iFadeoutRange = C4FOW_DefLightFadeoutRangeX;
		}
	}
	// set range
	Obj->SetLightRange(iRange, iFadeoutRange);
}

static long FnGetLightColor(C4Object *Obj)
{
	// get it
	return Obj->GetLightColor();
}

static void FnSetLightColor(C4Object *Obj, long iValue)
{
	Obj->SetLightColor(iValue);
}

static void FnSetPicture(C4Object *Obj, long iX, long iY, long iWdt, long iHgt)
{
	// set new picture rect
	Obj->PictureRect.Set(iX, iY, iWdt, iHgt);
}

static C4String *FnGetProcedure(C4Object *Obj)
{
	// no action?
	C4PropList* pActionDef = Obj->GetAction();
	if (!pActionDef)
	{
		return nullptr;
	}
	// get proc
	return pActionDef->GetPropertyStr(P_Procedure);
}

static bool FnCheckVisibility(C4Object *Obj, int plr)
{
	return Obj->IsVisible(plr, false);
}

static bool FnSetClrModulation(C4Object *Obj, Nillable<long> dwClr, long iOverlayID)
{
	uint32_t clr = 0xffffffff;
	if (!dwClr.IsNil())
	{
		clr = dwClr;
	}

	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) Obj->Number, Obj->GetName());
			return false;
		}
		pOverlay->SetClrModulation(clr);
		// C4GraphicsOverlay Does not have an StdMeshInstance (yet), no need to
		// update faceordering
	}
	else
	{
		// set it
		Obj->ColorMod=clr;
		if (Obj->pMeshInstance)
		{
			Obj->pMeshInstance->SetFaceOrderingForClrModulation(clr);
		}
	}
	// success
	return true;
}

static Nillable<long> FnGetClrModulation(C4Object *Obj, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("GetClrModulation: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) Obj->Number, Obj->GetName());
			return C4Void();
		}
		return pOverlay->GetClrModulation();
	}
	else
	{
		// get it
		return Obj->ColorMod;
	}
}

static bool FnCloseMenu(C4Object *Obj)
{
	return !!Obj->CloseMenu(true);
}

static Nillable<long> FnGetMenuSelection(C4Object *Obj)
{
	if (!Obj->Menu || !Obj->Menu->IsActive())
	{
		return C4Void();
	}
	return Obj->Menu->GetSelection();
}

static bool FnCanConcatPictureWith(C4Object *Obj, C4Object *pObj)
{
	// safety
	if (!Obj->Status || !pObj)
	{
		return false;
	}
	// Call the function in the object
	return Obj->CanConcatPictureWith(pObj);
}

static bool FnSetGraphics(C4Object *Obj, C4String *pGfxName, C4Def *pSrcDef, long iOverlayID, long iOverlayMode, C4String *pAction, long dwBlitMode, C4Object *pOverlayObject)
{
	// safety
	if (!Obj->Status)
	{
		return false;
	}
	// setting overlay?
	if (iOverlayID)
	{
		// any overlays must be positive for now
		if (iOverlayID < 0)
		{
			Log("SetGraphics: Background overlays not implemented!");
			return false;
		}
		// deleting overlay?
		C4DefGraphics *pGrp = nullptr;
		if (iOverlayMode == C4GraphicsOverlay::MODE_Object || iOverlayMode == C4GraphicsOverlay::MODE_Rank || iOverlayMode == C4GraphicsOverlay::MODE_ObjectPicture)
		{
			if (!pOverlayObject)
			{
				return Obj->RemoveGraphicsOverlay(iOverlayID);
			}
		}
		else
		{
			if (!pSrcDef)
			{
				return Obj->RemoveGraphicsOverlay(iOverlayID);
			}
			pGrp = pSrcDef->Graphics.Get(FnStringPar(pGfxName));
			if (!pGrp)
			{
				return false;
			}
		}
		// adding/setting
		C4GraphicsOverlay *pOverlay = Obj->GetGraphicsOverlay(iOverlayID, true);
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
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = nullptr;
			pOverlay->SetAsObject(pOverlayObject, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_ExtraGraphics:
			pOverlay->SetAsExtraGraphics(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Rank:
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = nullptr;
			pOverlay->SetAsRank(dwBlitMode, pOverlayObject);
			break;

		case C4GraphicsOverlay::MODE_ObjectPicture:
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = nullptr;
			pOverlay->SetAsObjectPicture(pOverlayObject, dwBlitMode);
			break;

		default:
			DebugLog("SetGraphics: Invalid overlay mode");
			pOverlay->SetAsBase(nullptr, 0); // make invalid, so it will be removed
			break;
		}
		// remove if invalid
		if (!pOverlay->IsValid(Obj))
		{
			Obj->RemoveGraphicsOverlay(iOverlayID);
			return false;
		}
		// Okay, valid overlay set!
		return true;
	}
	// no overlay: Base graphics
	// set graphics - pSrcDef==nullptr defaults to pObj->pDef
	return Obj->SetGraphics(FnStringPar(pGfxName), pSrcDef);
}

static long FnGetDefBottom(C4PropList * _this)
{
	if (!_this || !_this->GetDef())
	{
		throw NeedNonGlobalContext("GetDefBottom");
	}

	C4Object *obj = Object(_this);
	C4Def *def = _this->GetDef();
	assert(!obj || obj->Def == def);
	
	if (obj)
	{
		return obj->GetY() + obj->Shape.GetBottom();
	}
	else if (def)
	{
		return def->Shape.GetBottom();
	}
	else
	{
		return 0;
	}
}

static bool FnSetMenuSize(C4Object *Obj, long iCols, long iRows)
{
	// get menu
	C4Menu *pMnu=Obj->Menu;
	if (!pMnu || !pMnu->IsActive())
	{
		return false;
	}
	pMnu->SetSize(Clamp<long>(iCols, 0, 50), Clamp<long>(iRows, 0, 50));
	return true;
}

static bool FnGetCrewEnabled(C4Object *Obj)
{
	// return status
	return !Obj->CrewDisabled;
}

static void FnSetCrewEnabled(C4Object *Obj, bool fEnabled)
{
	bool change = (Obj->CrewDisabled == fEnabled) ? true : false;

	// set status
	Obj->CrewDisabled=!fEnabled;
	// deselect
	if (!fEnabled)
	{
		C4Player *pOwner;
		if ((pOwner=::Players.Get(Obj->Owner)))
		{
			// if viewed player cursor gets deactivated and no new cursor is found, follow the old in target mode
			bool fWasCursorMode = (pOwner->ViewMode == C4PVM_Cursor);
			if (pOwner->Cursor==Obj)
			{
				pOwner->AdjustCursorCommand();
			}
			if (!pOwner->ViewCursor && !pOwner->Cursor && fWasCursorMode)
			{
				pOwner->SetViewMode(C4PVM_Target, Obj);
			}
		}
	}

	// call to crew
	if (change)
	{
		if (fEnabled)
		{
			Obj->Call(PSF_CrewEnabled);
		}
		else
		{
			Obj->Call(PSF_CrewDisabled);
		}
	}
}

static void FnDoCrewExp(C4Object *Obj, long iChange)
{
	// do exp
	Obj->DoExperience(iChange);
}

static bool FnClearMenuItems(C4Object *Obj)
{
	// check menu
	if (!Obj->Menu)
	{
		return false;
	}
	// clear the items
	Obj->Menu->ClearItems();
	// success
	return true;
}

static C4Object *FnGetObjectLayer(C4Object *Obj)
{
	// get layer object
	return Obj->Layer;
}

static void FnSetObjectLayer(C4Object *Obj, C4Object *pNewLayer)
{
	// set layer object
	Obj->Layer = pNewLayer;
	// set for all contents as well
	for (C4Object* contentObj : Obj->Contents)
	{
		if (contentObj && contentObj->Status)
		{
			contentObj->Layer = pNewLayer;
		}
	}
}

static void FnSetShape(C4Object *Obj, long iX, long iY, long iWdt, long iHgt)
{
	// update shape
	Obj->Shape.x = iX;
	Obj->Shape.y = iY;
	Obj->Shape.Wdt = iWdt;
	Obj->Shape.Hgt = iHgt;
	// section list needs refresh
	Obj->UpdatePos();
}

static bool FnSetObjDrawTransform(C4Object *Obj, long iA, long iB, long iC, long iD, long iE, long iF, long iOverlayID)
{
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			return false;
		}
		pTransform = pOverlay->GetTransform();
	}
	else
	{
		// set base transform
		pTransform = Obj->pDrawTransform;
		// del transform?
		if (!iB && !iC && !iD && !iF && iA==iE && (!iA || iA==1000))
		{
			// identity/0 and no transform defined: nothing to do
			if (!pTransform)
			{
				return true;
			}
			// transform has no flipdir?
			if (pTransform->FlipDir == 1)
			{
				// kill identity-transform, then
				delete pTransform;
				Obj->pDrawTransform = nullptr;
				return true;
			}
			// flipdir must remain: set identity
			pTransform->Set(1,0,0,0,1,0,0,0,1);
			return true;
		}
		// create draw transform if not already present
		if (!pTransform)
		{
			pTransform = Obj->pDrawTransform = new C4DrawTransform();
		}
	}
	// assign values
	pTransform->Set((float) iA/1000, (float) iB/1000, (float) iC/1000, (float) iD/1000, (float) iE/1000, (float) iF/1000, 0, 0, 1);
	// done, success
	return true;
}

static bool FnSetObjDrawTransform2(C4Object *Obj, long iA, long iB, long iC, long iD, long iE, long iF, long iG, long iH, long iI, long iOverlayID)
{
	// local call / safety
	C4Object * pObj = Obj;
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			return false;
		}
		pTransform = pOverlay->GetTransform();
	}
	else
	{
		// set base transform
		pTransform = pObj->pDrawTransform;
		// create draw transform if not already present
		if (!pTransform)
		{
			pTransform = pObj->pDrawTransform = new C4DrawTransform(1);
		}
	}
	// assign values
#define L2F(l) ((float)l/1000)
	C4BltTransform matrix;
	matrix.Set(L2F(iA), L2F(iB), L2F(iC), L2F(iD), L2F(iE), L2F(iF), L2F(iG), L2F(iH), L2F(iI));
	*pTransform *= matrix;
#undef L2F
	// done, success
	return true;
}

static bool FnSetObjectStatus(C4Object *Obj, long iNewStatus, bool fClearPointers)
{
	// local call / safety
	if (!Obj->Status)
	{
		return false;
	}
	// no change
	if (Obj->Status == iNewStatus)
	{
		return true;
	}
	// set new status
	switch (iNewStatus)
	{
	case C4OS_NORMAL:
		return Obj->StatusActivate();
	case C4OS_INACTIVE:
		return Obj->StatusDeactivate(fClearPointers);
	default:
		return false; // status unknown
	}
}

static long FnGetObjectStatus(C4Object *Obj)
{
	return Obj->Status;
}

static bool FnAdjustWalkRotation(C4Object *Obj, long iRangeX, long iRangeY, long iSpeed)
{
	// must be rotateable and attached to solid ground
	if (!Obj->Def->Rotateable || (~Obj->Action.t_attach & CNAT_Bottom) || Obj->Shape.AttachMat == MNone)
	{
		return false;
	}
	// adjust rotation
	return Obj->AdjustWalkRotation(iRangeX, iRangeY, iSpeed);
}

static long FnGetContact(C4Object *Obj, long iVertex, long dwCheck)
{
	// vertex not specified: check all
	if (iVertex == -1)
	{
		long iResult = 0;
		for (int i=0; i<Obj->Shape.VtxNum; ++i)
		{
			iResult |= Obj->Shape.GetVertexContact(i, dwCheck, Obj->GetX(), Obj->GetY());
		}
		return iResult;
	}
	// vertex specified: check it
	if (!Inside<long>(iVertex, 0, Obj->Shape.VtxNum-1))
	{
		return 0;
	}
	return Obj->Shape.GetVertexContact(iVertex, dwCheck, Obj->GetX(), Obj->GetY());
}

static long FnSetObjectBlitMode(C4Object *Obj, long dwNewBlitMode, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) Obj->Number, Obj->GetName());
			return false;
		}
		pOverlay->SetBlitMode(dwNewBlitMode);
		return true;
	}
	// get prev blit mode
	DWORD dwPrevMode = Obj->BlitMode;
	// iNewBlitMode = 0: reset to definition default
	if (!dwNewBlitMode)
	{
		Obj->BlitMode = Obj->Def->BlitMode;
	}
	else
	{
		// otherwise, set the desired value
		// also ensure that the custom flag is set
		Obj->BlitMode = dwNewBlitMode | C4GFXBLIT_CUSTOM;
	}
	// return previous value
	return dwPrevMode;
}

static Nillable<long> FnGetObjectBlitMode(C4Object *Obj, long iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = Obj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLogF("SetObjectBlitMode: Overlay %d not defined for object %d (%s)", (int) iOverlayID, (int) Obj->Number, Obj->GetName());
			return C4Void();
		}
		return pOverlay->GetBlitMode();
	}
	// get blitting mode
	return Obj->BlitMode;
}

static long FnGetUnusedOverlayID(C4Object *Obj, long iBaseIndex)
{
	// safety
	if (!iBaseIndex)
	{
		return 0;
	}
	// find search first unused index from there on
	int iSearchDir = (iBaseIndex < 0) ? -1 : 1;
	while (Obj->GetGraphicsOverlay(iBaseIndex, false))
	{
		iBaseIndex += iSearchDir;
	}
	return iBaseIndex;
}

static Nillable<int> FnPlayAnimation(C4Object *Obj, C4String *szAnimation, int iSlot, C4ValueArray* PositionProvider, Nillable<C4ValueArray*> WeightProvider, Nillable<int> iSibling, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}
	if (iSlot == 0)
	{
		return C4Void(); // Reserved for ActMap animations
	}
	if (!PositionProvider)
	{
		return C4Void();
	}
	// If no weight provider is passed, this animation should be played exclusively.
	bool stop_previous_animations = WeightProvider.IsNil();
	// Exclusive mode cannot work with a sibling
	if (!iSibling.IsNil() && stop_previous_animations)
	{
		return C4Void();
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* s_node = nullptr;
	if (!iSibling.IsNil())
	{
		s_node = Instance->GetAnimationNodeByNumber(iSibling);
		if (!s_node || s_node->GetSlot() != iSlot)
		{
			return C4Void();
		}
	}

	const StdMeshAnimation* animation = Instance->GetMesh().GetSkeleton().GetAnimationByName(szAnimation->GetData());
	if (!animation)
	{
		return C4Void();
	}

	StdMeshInstance::ValueProvider* p_provider = CreateValueProviderFromArray(Obj, *PositionProvider, animation);
	StdMeshInstance::ValueProvider* w_provider;
	if (stop_previous_animations)
	{
		w_provider = new C4ValueProviderConst(Fix1);
	}
	else
	{
		w_provider = CreateValueProviderFromArray(Obj, *WeightProvider);
	}
	if (!p_provider || !w_provider)
	{
		delete p_provider;
		delete w_provider;
		return C4Void();
	}

	StdMeshInstance::AnimationNode* n_node = Instance->PlayAnimation(*animation, iSlot, s_node, p_provider, w_provider, stop_previous_animations);
	if (!n_node)
	{
		return C4Void();
	}

	return n_node->GetNumber();
}

static Nillable<int> FnTransformBone(C4Object *Obj, C4String *szBoneName, C4ValueArray* Transformation, int iSlot, C4ValueArray* WeightProvider, Nillable<int> iSibling, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}
	if (iSlot == 0)
	{
		return C4Void(); // Reserved for ActMap animations
	}
	if (!Transformation)
	{
		return C4Void();
	}
	if (!WeightProvider)
	{
		return C4Void();
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* s_node = nullptr;
	if (!iSibling.IsNil())
	{
		s_node = Instance->GetAnimationNodeByNumber(iSibling);
		if (!s_node || s_node->GetSlot() != iSlot)
		{
			return C4Void();
		}
	}

	const StdMeshBone* bone = Instance->GetMesh().GetSkeleton().GetBoneByName(szBoneName->GetData());
	if (!bone)
	{
		return C4Void();
	}

	StdMeshInstance::ValueProvider* w_provider = CreateValueProviderFromArray(Obj, *WeightProvider);
	if (!w_provider)
	{
		return C4Void();
	}

	StdMeshMatrix matrix;
	if (!C4ValueToMatrix(*Transformation, &matrix))
	{
		throw C4AulExecError("TransformBone: Transformation is not a valid 3x4 matrix");
	}

	// For bone transformations we cannot use general matrix transformations, but we use decomposed
	// translate, scale and rotation components (represented by the StdMeshTransformation class). This
	// is less generic since it does not support skewing.
	// Still, in the script API we want to expose a matrix parameter so that the convenient Trans_*
	// functions can be used. We decompose the passed matrix at this point. If the matrix indeed has
	// skewing components, the results will probably look strange since the decomposition would yield
	// bogus values, however I don't think that's a practical use case. In the worst case we could add
	// a check here and return nil if the matrix cannot be decomposed.
	StdMeshTransformation trans = matrix.Decompose();

	StdMeshInstance::AnimationNode* n_node = Instance->PlayAnimation(bone, trans, iSlot, s_node, w_provider, false);
	if (!n_node)
	{
		return C4Void();
	}

	return n_node->GetNumber();
}

static bool FnStopAnimation(C4Object *Obj, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return false;
	}
	if (!Obj->pMeshInstance)
	{
		return false;
	}
	if (iAnimationNumber.IsNil())
	{
		return false; // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return false;
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0)
	{
		return false;
	}
	Instance->StopAnimation(node);
	return true;
}

static Nillable<int> FnGetRootAnimation(C4Object *Obj, int iSlot, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetRootAnimationForSlot(iSlot);
	if (!node)
	{
		return C4Void();
	}
	return node->GetNumber();
}

static Nillable<C4ValueArray*> FnGetAnimationList(C4PropList* _this, Nillable<int> iAttachNumber)
{
	C4Object *Obj = Object(_this);
	const StdMeshSkeleton* skeleton;
	if (!Obj)
	{
		if (!_this || !_this->GetDef())
		{
			throw NeedNonGlobalContext("GetAnimationList");
		}
		C4Def *def = _this->GetDef();
		if (!def->Graphics.IsMesh())
		{
			return C4Void();
		}

		skeleton = &def->Graphics.Mesh->GetSkeleton();
	}
	else
	{
		if (!Obj)
		{
			return C4Void();
		}
		if (!Obj->pMeshInstance)
		{
			return C4Void();
		}

		StdMeshInstance* Instance = Obj->pMeshInstance;
		if (!iAttachNumber.IsNil())
		{
			const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
			// OwnChild is set if an object's instance is attached. In that case the animation list should be obtained directly on that object.
			if (!Attached || !Attached->OwnChild)
			{
				return C4Void();
			}
			Instance = Attached->Child;
		}

		skeleton = &Instance->GetMesh().GetSkeleton();
	}

	const std::vector<const StdMeshAnimation*> animations = skeleton->GetAnimations();

	C4ValueArray* retval = new C4ValueArray(animations.size());
	for(unsigned int i = 0; i < animations.size(); ++i)
	{
		(*retval)[i] = C4VString(animations[i]->Name);
	}
	return retval;
}

static Nillable<int> FnGetAnimationLength(C4Object *Obj, C4String *szAnimation, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	const StdMeshAnimation* animation = Instance->GetMesh().GetSkeleton().GetAnimationByName(szAnimation->GetData());
	if (!animation)
	{
		return C4Void();
	}
	return fixtoi(ftofix(animation->Length), 1000); // sync critical!
}

static Nillable<C4String*> FnGetAnimationName(C4Object *Obj, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}
	if (iAnimationNumber.IsNil())
	{
		return C4Void(); // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	if (!node || node->GetType() != StdMeshInstance::AnimationNode::LeafNode)
	{
		return C4Void();
	}
	return String(node->GetAnimation()->Name.getData());
}

static Nillable<int> FnGetAnimationPosition(C4Object *Obj, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}
	if (iAnimationNumber.IsNil())
	{
		return C4Void(); // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	if (!node || node->GetType() != StdMeshInstance::AnimationNode::LeafNode)
	{
		return C4Void();
	}
	return fixtoi(node->GetPosition(), 1000);
}

static Nillable<int> FnGetAnimationWeight(C4Object *Obj, Nillable<int> iAnimationNumber, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return C4Void();
	}
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}
	if (iAnimationNumber.IsNil())
	{
		return C4Void(); // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return C4Void();
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	if (!node || node->GetType() != StdMeshInstance::AnimationNode::LinearInterpolationNode)
	{
		return C4Void();
	}
	return fixtoi(node->GetWeight(), 1000);
}

static bool FnSetAnimationPosition(C4Object *Obj, Nillable<int> iAnimationNumber, C4ValueArray* PositionProvider, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return false;
	}
	if (!Obj->pMeshInstance)
	{
		return false;
	}
	if (iAnimationNumber.IsNil())
	{
		return false; // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return false;
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0 || node->GetType() != StdMeshInstance::AnimationNode::LeafNode)
	{
		return false;
	}
	StdMeshInstance::ValueProvider* p_provider = CreateValueProviderFromArray(Obj, *PositionProvider);
	if (!p_provider)
	{
		return false;
	}
	Instance->SetAnimationPosition(node, p_provider);
	return true;
}

static bool FnSetAnimationBoneTransform(C4Object *Obj, Nillable<int> iAnimationNumber, C4ValueArray* Transformation, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return false;
	}
	if (!Obj->pMeshInstance)
	{
		return false;
	}
	if (iAnimationNumber.IsNil())
	{
		return false; // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return false;
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0 || node->GetType() != StdMeshInstance::AnimationNode::CustomNode)
	{
		return false;
	}

	StdMeshMatrix matrix;
	if (!C4ValueToMatrix(*Transformation, &matrix))
	{
		throw C4AulExecError("TransformBone: Transformation is not a valid 3x4 matrix");
	}
	// Here the same remark applies as in FnTransformBone
	StdMeshTransformation trans = matrix.Decompose();

	Instance->SetAnimationBoneTransform(node, trans);
	return true;
}

static bool FnSetAnimationWeight(C4Object *Obj, Nillable<int> iAnimationNumber, C4ValueArray* WeightProvider, Nillable<int> iAttachNumber)
{
	if (!Obj)
	{
		return false;
	}
	if (!Obj->pMeshInstance)
	{
		return false;
	}
	if (iAnimationNumber.IsNil())
	{
		return false; // distinguish nil from 0
	}

	StdMeshInstance* Instance = Obj->pMeshInstance;
	if (!iAttachNumber.IsNil())
	{
		const StdMeshInstance::AttachedMesh* Attached = Instance->GetAttachedMeshByNumber(iAttachNumber);
		// OwnChild is set if an object's instance is attached. In that case the animation should be set directly on that object.
		if (!Attached || !Attached->OwnChild)
		{
			return false;
		}
		Instance = Attached->Child;
	}

	StdMeshInstance::AnimationNode* node = Instance->GetAnimationNodeByNumber(iAnimationNumber);
	// slot 0 is reserved for ActMap animations
	if (!node || node->GetSlot() == 0 || node->GetType() != StdMeshInstance::AnimationNode::LinearInterpolationNode)
	{
		return false;
	}
	StdMeshInstance::ValueProvider* w_provider = CreateValueProviderFromArray(Obj, *WeightProvider);
	if (!w_provider)
	{
		return false;
	}
	Instance->SetAnimationWeight(node, w_provider);
	return true;
}

static Nillable<int> FnAttachMesh(C4Object *Obj, C4PropList* Mesh, C4String * szParentBone, C4String * szChildBone, C4ValueArray * Transformation, int Flags, int AttachNumber)
{
	if (!Obj->pMeshInstance)
	{
		return C4Void();
	}
	if (!Mesh)
	{
		return C4Void();
	}

	StdMeshMatrix trans = StdMeshMatrix::Identity();
	if (Transformation && !C4ValueToMatrix(*Transformation, &trans))
	{
		throw C4AulExecError("AttachMesh: Transformation is not a valid 3x4 matrix");
	}

	StdMeshInstance::AttachedMesh* attach;
	C4Object* pObj = Mesh->GetObject();
	if (pObj)
	{
		if (!pObj->pMeshInstance)
		{
			return C4Void();
		}
		attach = Obj->pMeshInstance->AttachMesh(*pObj->pMeshInstance, new C4MeshDenumerator(pObj), szParentBone->GetData(), szChildBone->GetData(), trans, Flags, false, AttachNumber);
	}
	else
	{
		C4Def* pDef = Mesh->GetDef();
		if (!pDef)
		{
			return C4Void();
		}
		if (pDef->Graphics.Type != C4DefGraphics::TYPE_Mesh)
		{
			return C4Void();
		}
		attach = Obj->pMeshInstance->AttachMesh(*pDef->Graphics.Mesh, new C4MeshDenumerator(pDef), szParentBone->GetData(), szChildBone->GetData(), trans, Flags, AttachNumber);
		if (attach)
		{
			attach->Child->SetFaceOrderingForClrModulation(Obj->ColorMod);
		}
	}

	if (!attach)
	{
		return C4Void();
	}
	return attach->Number;
}

static bool FnDetachMesh(C4Object *Obj, long iAttachNumber)
{
	if (!Obj || !Obj->pMeshInstance)
	{
		return false;
	}
	return Obj->pMeshInstance->DetachMesh(iAttachNumber);
}

static bool FnSetAttachBones(C4Object *Obj, long iAttachNumber, Nillable<C4String*> szParentBone, Nillable<C4String*> szChildBone)
{
	if (!Obj || !Obj->pMeshInstance)
	{
		return false;
	}
	StdMeshInstance::AttachedMesh* attach = Obj->pMeshInstance->GetAttachedMeshByNumber(iAttachNumber);
	if (!attach)
	{
		return false;
	}

	if (!szParentBone.IsNil())
	{
		C4String* ParentBone = szParentBone;
		if (!attach->SetParentBone(ParentBone->GetData()))
		{
			return false;
		}
	}

	if (!szChildBone.IsNil())
	{
		C4String* ChildBone = szChildBone;
		if (!attach->SetChildBone(ChildBone->GetData()))
		{
			return false;
		}
	}

	return true;
}

static bool FnSetAttachTransform(C4Object *Obj, long iAttachNumber, C4ValueArray* Transformation)
{
	if (!Obj || !Obj->pMeshInstance)
	{
		return false;
	}
	if (!Transformation)
	{
		return false;
	}
	StdMeshInstance::AttachedMesh* attach = Obj->pMeshInstance->GetAttachedMeshByNumber(iAttachNumber);
	if (!attach)
	{
		return false;
	}

	StdMeshMatrix trans;
	if (!C4ValueToMatrix(*Transformation, &trans))
	{
		throw C4AulExecError("SetAttachTransform: Transformation is not a valid 3x4 matrix");
	}

	attach->SetAttachTransformation(trans);
	return true;
}

static Nillable<C4ValueArray*> FnGetBoneNames(C4PropList * _this) {
	C4Object *obj = Object(_this);

	auto fnMakeC4ValueArray = [](const size_t len, const std::function<C4Value(size_t i)> iterable) -> C4ValueArray* {
		auto retval = new C4ValueArray(len);
		for (size_t i = 0; i < len; ++i) {
			retval->SetItem(i, iterable(i));
		}
		return retval;
	};

	if (!obj)
	{
		if (!_this || !_this->GetDef())
		{
			throw NeedNonGlobalContext("GetBoneNames");
		}
		// Called in definition context -> Use definition default mesh
		C4Def *def = _this->GetDef();
		if (!def->Graphics.IsMesh()) 
		{
			return C4Void();
		}
		const StdMeshSkeleton & skeleton = def->Graphics.Mesh->GetSkeleton();
		const std::function<C4Value(size_t i)> fnGetBoneName = [&skeleton](size_t i = 0) { return C4VString(skeleton.GetBone(i).Name); };
		return fnMakeC4ValueArray(skeleton.GetNumBones(), fnGetBoneName);
	}
	else
	{
		// Called in object context: Use current object mesh
		if (!obj->pMeshInstance)
		{
			return C4Void();
		}
		const StdMeshSkeleton & skeleton = obj->pMeshInstance->GetMesh().GetSkeleton();
		const std::function<C4Value(size_t i)> fnGetBoneName = [&skeleton](size_t i = 0) { return C4VString(skeleton.GetBone(i).Name); };
		return fnMakeC4ValueArray(skeleton.GetNumBones(), fnGetBoneName);;
	}
}

static Nillable<C4String*> FnGetMeshMaterial(C4PropList * _this, int iSubMesh)
{
	// Called in object or definition context?
	C4Object *Obj = Object(_this);
	if (!Obj)
	{
		if (!_this || !_this->GetDef())
		{
			throw NeedNonGlobalContext("GetMeshMaterial");
		}
		// Called in definition context: Get definition default mesh material
		C4Def *def = _this->GetDef();
		if (!def->Graphics.IsMesh())
		{
			return C4Void();
		}
		if (iSubMesh < 0 || (unsigned int)iSubMesh >= def->Graphics.Mesh->GetNumSubMeshes())
		{
			return C4Void();
		}
		const StdSubMesh &submesh = def->Graphics.Mesh->GetSubMesh(iSubMesh);
		return String(submesh.GetMaterial().Name.getData());
	}
	else
	{
		// Called in object context: Get material of mesh instance
		if (!Obj->pMeshInstance)
		{
			return C4Void();
		}
		if (iSubMesh < 0 || (unsigned int)iSubMesh >= Obj->pMeshInstance->GetNumSubMeshes())
		{
			return C4Void();
		}
		StdSubMeshInstance& submesh = Obj->pMeshInstance->GetSubMesh(iSubMesh);
		return String(submesh.GetMaterial().Name.getData());
	}
}

static bool FnSetMeshMaterial(C4Object *Obj, C4String* Material, int iSubMesh)
{
	if (!Obj || !Obj->pMeshInstance)
	{
		return false;
	}
	if (iSubMesh < 0 || (unsigned int)iSubMesh >= Obj->pMeshInstance->GetNumSubMeshes())
	{
		return false;
	}
	if (!Material)
	{
		return false;
	}

	const StdMeshMaterial* material = ::MeshMaterialManager.GetMaterial(Material->GetData().getData());
	if (!material)
	{
		return false;
	}

	Obj->pMeshInstance->SetMaterial(iSubMesh, *material);
	return true;
}


static bool FnCreateParticleAtBone(C4Object* Obj, C4String* szName, C4String* szBoneName, C4ValueArray* Pos, C4ValueArray* Dir, C4Value lifetime, C4PropList *properties, int amount)
{
	// safety
	if(!Obj || !Obj->Status)
	{
		return false;
	}
	// Get bone
	if(!Obj->pMeshInstance)
	{
		return false;
	}
	const StdMesh& mesh = Obj->pMeshInstance->GetMesh();
	const StdMeshBone* bone = mesh.GetSkeleton().GetBoneByName(szBoneName->GetData());
	if (!bone)
	{
		return false;
	}
	// get particle
	C4ParticleDef *pDef=::Particles.definitions.GetDef(FnStringPar(szName));
	if (!pDef)
	{
		return false;
	}
#ifndef USE_CONSOLE
	// Get transform
	Obj->pMeshInstance->UpdateBoneTransforms();
	const StdMeshMatrix transform = Obj->pMeshInstance->GetBoneTransform(bone->Index) * StdMeshMatrix::Transform(bone->Transformation);
	// Get offset and direction
	StdMeshVector x, dir;
	if (Pos)
	{
		if(Pos->GetSize() != 3)
		{
			throw C4AulExecError("CreateParticleAtBone: Pos is not a three-vector");
		}
		x.x = (*Pos).GetItem(0).getInt();
		x.y = (*Pos).GetItem(1).getInt();
		x.z = (*Pos).GetItem(2).getInt();
	}
	else
	{
		x.x = x.y = x.z = 0.0f;
	}

	if(Dir)
	{
		if(Dir->GetSize() != 3)
		{
			throw C4AulExecError("CreateParticleAtBone: Dir is not a three-vector");
		}
		dir.x = (*Dir).GetItem(0).getInt() / 10.0f;
		dir.y = (*Dir).GetItem(1).getInt() / 10.0f;
		dir.z = (*Dir).GetItem(2).getInt() / 10.0f;
	}
	else
	{
		dir.x = dir.y = dir.z = 0.0f;
	}
	// Apply the bone transformation to them, to go from bone coordinates
	// to mesh coordinates.
	// This is a good example why we should have different types for
	// position vectors and displacement vectors. TODO.
	StdMeshVector transformed_x = transform * x;
	transformed_x.x += transform(0,3);
	transformed_x.y += transform(1,3);
	transformed_x.z += transform(2,3);
	x = transformed_x;
	dir = transform * dir;
	// Apply MeshTransformation in the mesh reference frame
	C4Value value;
	Obj->GetProperty(P_MeshTransformation, &value);
	StdMeshMatrix MeshTransform;
	if (!C4ValueToMatrix(value, &MeshTransform))
	{
		MeshTransform = StdMeshMatrix::Identity();
	}
	x = MeshTransform * x;
	dir = MeshTransform * dir;
	x.x += MeshTransform(0,3);
	x.y += MeshTransform(1,3);
	x.z += MeshTransform(2,3);
	// Now go to world coordinates -- this code is copied from and needs to
	// stay in sync with C4DrawGL::PerformMesh, so the particles are
	// created at the correct position.
	// TODO: This should be moved into a common function.
	const StdMeshBox& box = mesh.GetBoundingBox();
	StdMeshVector v1;
	v1.x = box.x1;
	v1.y = box.y1;
	v1.z = box.z1;
	StdMeshVector v2;
	v2.x = box.x2;
	v2.y = box.y2;
	v2.z = box.z2;
	const float tx = fixtof(Obj->fix_x) + Obj->Def->Shape.GetX();
	const float ty = fixtof(Obj->fix_y) + Obj->Def->Shape.GetY();
	const float twdt = Obj->Def->Shape.Wdt;
	const float thgt = Obj->Def->Shape.Hgt;
	const float rx = -std::min(v1.x,v2.x) / fabs(v2.x - v1.x);
	const float ry = -std::min(v1.y,v2.y) / fabs(v2.y - v1.y);
	const float dx = tx + rx*twdt;
	const float dy = ty + ry*thgt;
	x.x += dx;
	x.y += dy;
	// This was added in the block before and could also just be removed from tx/ty.
	// However, the block would no longer be equal to where it came from.
	x.x -= fixtof(Obj->fix_x);
	x.y -= fixtof(Obj->fix_y);
	// Finally, apply DrawTransform to the world coordinates,
	// and incorporate object rotation into the transformation
	C4DrawTransform draw_transform;
	if (Obj->pDrawTransform)
	{
		draw_transform.SetTransformAt(*Obj->pDrawTransform, fixtof(Obj->fix_x), fixtof(Obj->fix_y));
		draw_transform.Rotate(fixtof(Obj->fix_r), 0.0f, 0.0f);
	}
	else
	{
		draw_transform.SetRotate(fixtof(Obj->fix_r), 0.0f, 0.0f);
	}

	StdMeshMatrix DrawTransform;
	DrawTransform(0, 0) = draw_transform.mat[0];
	DrawTransform(0, 1) = draw_transform.mat[1];
	DrawTransform(0, 2) = 0.0f;
	DrawTransform(0, 3) = draw_transform.mat[2];
	DrawTransform(1, 0) = draw_transform.mat[3];
	DrawTransform(1, 1) = draw_transform.mat[4];
	DrawTransform(1, 2) = 0.0f;
	DrawTransform(1, 3) = draw_transform.mat[5];
	DrawTransform(2, 0) = 0.0f;
	DrawTransform(2, 1) = 0.0f;
	DrawTransform(2, 2) = 1.0f;
	DrawTransform(2, 3) = 0.0f;

	x = DrawTransform * x;
	dir = DrawTransform * dir;
	x.x += DrawTransform(0,3);
	x.y += DrawTransform(1,3);
	x.z += DrawTransform(2,3);

	// construct data
	C4ParticleValueProvider valueX, valueY, valueSpeedX, valueSpeedY, valueLifetime;
	valueX.Set(x.x);
	valueY.Set(x.y);
	valueSpeedX.Set(dir.x);
	valueSpeedY.Set(dir.y);
	valueLifetime.Set(lifetime);

	// cast
	if (amount < 1)
	{
		amount = 1;
	}
	::Particles.Create(pDef, valueX, valueY, valueSpeedX, valueSpeedY, valueLifetime, properties, amount, Obj);
#endif
	// success, even if not created
	return true;

}

static Nillable<long> FnGetDefWidth(C4PropList * _this)
{
	if (!_this)
	{
		return C4Void();
	}
	C4Def *def = _this->GetDef();
	if (!def)
	{
		return  C4Void();
	}
	return def->Shape.Wdt;
}

static Nillable<long> FnGetDefHeight(C4PropList * _this)
{
	if (!_this)
	{
		return C4Void();
	}
	C4Def *def = _this->GetDef();
	if (!def)
	{
		return  C4Void();
	}
	return def->Shape.Hgt;
}

//=========================== C4Script Function Map ===================================

C4ScriptConstDef C4ScriptObjectConstMap[]=
{
	{ "C4D_None"               ,C4V_Int,          C4D_None},
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

	{ "C4D_Border_Sides"       ,C4V_Int,          C4D_Border_Sides},
	{ "C4D_Border_Top"         ,C4V_Int,          C4D_Border_Top},
	{ "C4D_Border_Bottom"      ,C4V_Int,          C4D_Border_Bottom},
	{ "C4D_Border_Layer"       ,C4V_Int,          C4D_Border_Layer},

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
	{ "VIS_Editor"             ,C4V_Int,          VIS_Editor},

	{ "C4MN_Style_Normal"      ,C4V_Int,          C4MN_Style_Normal},
	{ "C4MN_Style_Context"     ,C4V_Int,          C4MN_Style_Context},
	{ "C4MN_Style_Info"        ,C4V_Int,          C4MN_Style_Info},
	{ "C4MN_Style_Dialog"      ,C4V_Int,          C4MN_Style_Dialog},
	{ "C4MN_Style_EqualItemHeight",C4V_Int,       C4MN_Style_EqualItemHeight},

	{ "C4MN_Extra_None"        ,C4V_Int,          C4MN_Extra_None},
	{ "C4MN_Extra_Value"       ,C4V_Int,          C4MN_Extra_Value},
	{ "C4MN_Extra_Info"        ,C4V_Int,          C4MN_Extra_Info},

	{ "C4MN_Add_ImgRank"       ,C4V_Int,          C4MN_Add_ImgRank},
	{ "C4MN_Add_ImgIndexed"    ,C4V_Int,          C4MN_Add_ImgIndexed},
	{ "C4MN_Add_ImgObjRank"    ,C4V_Int,          C4MN_Add_ImgObjRank},
	{ "C4MN_Add_ImgObject"     ,C4V_Int,          C4MN_Add_ImgObject},
	{ "C4MN_Add_ImgTextSpec"   ,C4V_Int,          C4MN_Add_ImgTextSpec},
	{ "C4MN_Add_ImgPropListSpec",C4V_Int,         C4MN_Add_ImgPropListSpec},
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
	{ "GFX_BLIT_Wireframe"        ,C4V_Int,      C4GFXBLIT_WIREFRAME},              //
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
	{ "CNAT_PhaseHalfVehicle"     ,C4V_Int,      CNAT_PhaseHalfVehicle      },

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
	{ "C4AVP_R"                   ,C4V_Int,      C4AVP_R },
	{ "C4AVP_AbsX"                ,C4V_Int,      C4AVP_AbsX },
	{ "C4AVP_AbsY"                ,C4V_Int,      C4AVP_AbsY },
	{ "C4AVP_Dist"                ,C4V_Int,      C4AVP_Dist },
	{ "C4AVP_XDir"                ,C4V_Int,      C4AVP_XDir },
	{ "C4AVP_YDir"                ,C4V_Int,      C4AVP_YDir },
	{ "C4AVP_RDir"                ,C4V_Int,      C4AVP_RDir },
	{ "C4AVP_AbsRDir"             ,C4V_Int,      C4AVP_AbsRDir },
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
	{ "AM_MatchSkeleton"          ,C4V_Int,      StdMeshInstance::AM_MatchSkeleton },

	{ nullptr, C4V_Nil, 0}
};


void InitObjectFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptObjectConstMap[0]; pCDef->Identifier; pCDef++)
	{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		pEngine->RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
	}
	C4PropListStatic * p = pEngine->GetPropList();
#define F(f) ::AddFunc(p, #f, Fn##f)

	F(DoCon);
	F(GetCon);
	F(DoDamage);
	F(DoEnergy);
	F(DoBreath);
	F(GetEnergy);
	F(OnFire);
	F(Stuck);
	F(InLiquid);
	F(SetAction);
	F(SetActionData);

	F(GetAction);
	F(GetActTime);
	F(GetOwner);
	F(GetMass);
	F(GetBreath);
	F(GetMenu);
	F(GetVertexNum);
	F(GetVertex);
	F(SetVertex);
	F(AddVertex);
	F(InsertVertex);
	F(RemoveVertex);
	::AddFunc(p, "SetContactDensity", FnSetContactDensity, false);
	F(GetController);
	F(SetController);
	F(SetName);
	F(GetKiller);
	F(SetKiller);
	F(GetPhase);
	F(SetPhase);
	F(GetCategory);
	F(GetOCF);
	F(SetAlive);
	F(GetAlive);
	F(GetDamage);
	F(SetComDir);
	F(GetComDir);
	F(SetDir);
	F(GetDir);
	F(SetEntrance);
	F(GetEntrance);
	F(SetCategory);
	F(FinishCommand);
	F(ActIdle);
	F(SetRDir);
	F(GetRDir);
	F(GetXDir);
	F(GetYDir);
	F(GetR);
	F(SetXDir);
	F(SetYDir);
	F(SetR);
	F(SetOwner);
	F(MakeCrewMember);
	F(GrabObjectInfo);
	F(CreateContents);
	F(ShiftContents);
	F(GetID);
	F(Contents);
	F(ScrollContents);
	F(Contained);
	F(ContentsCount);
	::AddFunc(p, "FindContents", FnFindContents, false);
	::AddFunc(p, "FindOtherContents", FnFindOtherContents, false);
	F(RemoveObject);
	F(GetActionTarget);
	F(SetActionTargets);
	::AddFunc(p, "SetCrewStatus", FnSetCrewStatus, false);
	F(SetPosition);
	F(CreateMenu);
	F(AddMenuItem);
	F(SelectMenuItem);
	F(SetMenuDecoration);
	F(SetMenuTextProgress);
	F(ObjectDistance);
	F(GetValue);
	F(GetRank);
	F(SetTransferZone);
	F(SetMass);
	F(GetColor);
	F(SetColor);
	F(SetLightRange);
	F(GetLightColor);
	F(SetLightColor);
	F(SetPicture);
	F(GetProcedure);
	F(CanConcatPictureWith);
	F(SetGraphics);
	F(ObjectNumber);
	F(ShowInfo);
	F(CheckVisibility);
	F(SetClrModulation);
	F(GetClrModulation);
	F(CloseMenu);
	F(GetMenuSelection);
	F(GetDefBottom);
	F(SetMenuSize);
	F(GetCrewEnabled);
	F(SetCrewEnabled);
	F(DoCrewExp);
	F(ClearMenuItems);
	F(GetObjectLayer);
	F(SetObjectLayer);
	F(SetShape);
	F(SetObjDrawTransform);
	::AddFunc(p, "SetObjDrawTransform2", FnSetObjDrawTransform2, false);
	::AddFunc(p, "SetObjectStatus", FnSetObjectStatus, false);
	::AddFunc(p, "GetObjectStatus", FnGetObjectStatus, false);
	::AddFunc(p, "AdjustWalkRotation", FnAdjustWalkRotation, false);
	F(GetContact);
	F(SetObjectBlitMode);
	F(GetObjectBlitMode);
	::AddFunc(p, "GetUnusedOverlayID", FnGetUnusedOverlayID, false);
	F(ExecuteCommand);

	F(PlayAnimation);
	F(TransformBone);
	F(StopAnimation);
	F(GetRootAnimation);
	F(GetAnimationList);
	F(GetAnimationLength);
	F(GetAnimationName);
	F(GetAnimationPosition);
	F(GetAnimationWeight);
	F(SetAnimationPosition);
	F(SetAnimationBoneTransform);
	F(SetAnimationWeight);
	F(AttachMesh);
	F(DetachMesh);
	F(SetAttachBones);
	F(SetAttachTransform);
	F(GetBoneNames);
	F(GetMeshMaterial);
	F(SetMeshMaterial);
	F(CreateParticleAtBone);
	F(ChangeDef);
	F(GrabContents);
	F(Punch);
	F(Kill);
	F(Fling);
	::AddFunc(p, "Jump", FnJump, false);
	F(Enter);
	F(DeathAnnounce);
	F(SetSolidMask);
	F(SetHalfVehicleSolidMask);
	F(Exit);
	F(Collect);

	F(SetCommand);
	F(AddCommand);
	F(AppendCommand);
	F(GetCommand);
	F(SetCrewExtraData);
	F(GetCrewExtraData);
	F(GetDefWidth);
	F(GetDefHeight);
#undef F
}
