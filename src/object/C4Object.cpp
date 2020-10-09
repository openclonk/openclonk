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

/* That which fills the world with life */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "control/C4Record.h"
#include "game/C4Viewport.h"
#include "gui/C4GameMessage.h"
#include "landscape/C4MaterialList.h"
#include "landscape/C4PXS.h"
#include "landscape/C4Particles.h"
#include "landscape/C4SolidMask.h"
#include "landscape/fow/C4FoW.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4MeshDenumerator.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectMenu.h"
#include "platform/C4SoundSystem.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "script/C4Effect.h"

C4Object::C4Object()
{
	FrontParticles = BackParticles = nullptr;
	Default();
}

void C4Object::Default()
{
	id=C4ID::None;
	nInfo.Clear();
	RemovalDelay=0;
	Owner=NO_OWNER;
	Controller=NO_OWNER;
	LastEnergyLossCausePlayer=NO_OWNER;
	Category=0;
	Con=0;
	Mass=OwnMass=0;
	Damage=0;
	Energy=0;
	Alive=false;
	Breath=0;
	InMat=MNone;
	Color=0;
	lightRange=0;
	lightFadeoutRange=0;
	lightColor=0xffffffff;
	fix_x=fix_y=fix_r=0;
	xdir=ydir=rdir=0;
	Mobile=false;
	Unsorted=false;
	Initializing=false;
	OnFire=false;
	InLiquid=false;
	EntranceStatus=false;
	Audible=AudiblePan=0;
	AudiblePlayer = NO_OWNER;
	t_contact=0;
	OCF=0;
	Action.Default();
	Shape.Default();
	fOwnVertices=false;
	Contents.Default();
	SolidMask.Default();
	HalfVehicleSolidMask=false;
	PictureRect.Default();
	Def=nullptr;
	Info=nullptr;
	Command=nullptr;
	Contained=nullptr;
	TopFace.Default();
	Menu=nullptr;
	MaterialContents=nullptr;
	Marker=0;
	ColorMod=0xffffffff;
	BlitMode=0;
	CrewDisabled=false;
	Layer=nullptr;
	pSolidMaskData=nullptr;
	pGraphics=nullptr;
	pMeshInstance=nullptr;
	pDrawTransform=nullptr;
	pEffects=nullptr;
	pGfxOverlay=nullptr;
	iLastAttachMovementFrame=-1;

	ClearParticleLists();
}

bool C4Object::Init(C4PropList *pDef, C4Object *pCreator,
                    int32_t iOwner, C4ObjectInfo *pInfo,
                    int32_t nx, int32_t ny, int32_t nr,
                    C4Real nxdir, C4Real nydir, C4Real nrdir, int32_t iController)
{
	C4PropListNumbered::AcquireNumber();
	// currently initializing
	Initializing=true;

	// Def & basics
	Owner=iOwner;
	if (iController > NO_OWNER) Controller = iController; else Controller=iOwner;
	LastEnergyLossCausePlayer=NO_OWNER;
	Info=pInfo;
	Def=pDef->GetDef(); assert(Def);
	SetProperty(P_Prototype, C4VPropList(pDef));
	id=Def->id;
	if (Info) SetName(pInfo->Name);
	Category=Def->Category;
	Plane = Def->GetPlane(); assert(Plane);
	Def->Count++;
	if (pCreator) Layer=pCreator->Layer;

	// graphics
	pGraphics = &Def->Graphics;
	if (pGraphics->Type == C4DefGraphics::TYPE_Mesh)
	{
		pMeshInstance = new StdMeshInstance(*pGraphics->Mesh, Def->GrowthType ? 1.0f : static_cast<float>(Con)/static_cast<float>(FullCon));
		pMeshInstance->SetFaceOrderingForClrModulation(ColorMod);
	}
	else
	{
		pMeshInstance = nullptr;
	}
	BlitMode = Def->BlitMode;

	// Position
	if (!Def->Rotateable) { nr=0; nrdir=0; }
	fix_x=itofix(nx);
	fix_y=itofix(ny);
	fix_r=itofix(nr);
	xdir=nxdir; ydir=nydir; rdir=nrdir;

	// Initial mobility
	if (!!xdir || !!ydir || !!rdir)
		Mobile=true;

	// Mass
	Mass=std::max<int32_t>(Def->Mass*Con/FullCon,1);

	// Life, energy, breath
	if (Category & C4D_Living) Alive=true;
	if (Alive) Energy=GetPropertyInt(P_MaxEnergy);
	Breath=GetPropertyInt(P_MaxBreath);

	// Color
	if (Def->ColorByOwner)
	{
		if (ValidPlr(Owner))
			Color=::Players.Get(Owner)->ColorDw;
		else
			Color=0xff; // no-owner color: blue
	}

	// Shape & face
	Shape=Def->Shape;
	SolidMask=Def->SolidMask;
	CheckSolidMaskRect();
	UpdateGraphics(false);
	UpdateFace(true);

	// Initial audibility
	Audible=::Viewports.GetAudibility(GetX(), GetY(), &AudiblePan, 0, &AudiblePlayer);

	// Initial OCF
	SetOCF();

	// finished initializing
	Initializing=false;

	return true;
}

C4Object::~C4Object()
{
	Clear();

#if defined(_DEBUG)
	// debug: mustn't be listed in any list now
	::Objects.Sectors.AssertObjectNotInList(this);
#endif
}

void C4Object::ClearParticleLists()
{
	if (FrontParticles != nullptr)
		Particles.ReleaseParticleList(FrontParticles);
	if (BackParticles != nullptr)
		Particles.ReleaseParticleList(BackParticles);
	FrontParticles = BackParticles = nullptr;
}

// Start removing the object and do all the callbacks; See also FinishRemoval
void C4Object::AssignRemoval(bool exit_contents)
{
	// Multiple calls to this functions can cause really bad problems, so we have to cancel
	// in case the object is deleted or being deleted (final deletion happens after removal delay):
	// - the def count may be decreased several times. This is really hard to notice if there
	//   are a lot of objects, because you have to delete at least half of them to get to a
	//   negative count, and even then it will only have an effect for functions that
	//   actually evaluate the def count.
	// - object status and effects must be updated before the object is removed,
	//   but at the same time a lot if functions rely on the object being properly
	//   deleted when the status of an object is C4OS_DELETED.
	if (Status == C4OS_DELETED || RemovalDelay > 0)
	{
		return;
	}
	// Set status to deleting, so that callbacks in this function that might delete
	// the object do not delete it a second time.
	RemovalDelay = 2;

	// Debugging
	if (Config.General.DebugRec)
	{
		C4RCCreateObj rc;
		memset(&rc, '\0', sizeof(rc));
		rc.oei = Number;
		if (Def && Def->GetName())
		{
			strncpy(rc.id, Def->GetName(), 32+1);
		}
		rc.x = GetX();
		rc.y = GetY();
		rc.ownr = Owner;
		AddDbgRec(RCT_DsObj, &rc, sizeof(rc));
	}

	// Destruction call notification for container
	if (Contained)
	{
		C4AulParSet pars(this);
		Contained->Call(PSF_ContentsDestruction, &pars);
	}

	// Destruction call
	Call(PSF_Destruction);

	// Remove all effects (extinguishes as well)
	if (pEffects)
	{
		pEffects->ClearAll(C4FxCall_RemoveClear);
	}

	// Remove particles
	ClearParticleLists();

	// Action idle
	SetAction(nullptr);

	// Object system operation
	if (Status == C4OS_INACTIVE)
	{
		// Object was inactive: activate first, then delete
		::Objects.InactiveObjects.Remove(this);
		Status = C4OS_NORMAL;
		::Objects.Add(this);
	}

	Status = C4OS_DELETED;
	// count decrease
	Def->Count--;

	// get container for next actions
	C4Object *pCont = Contained;
	// remove or exit contents 
	for (C4Object *cobj : Contents)
	{
		if (exit_contents)
		{
			// move objects to parent container or exit them completely
			bool fRejectCollect;
			if (!pCont || !cobj->Enter(pCont, true, false, &fRejectCollect))
				cobj->Exit(GetX(), GetY());
		}
		else
		{
			Contents.Remove(cobj);
			cobj->Contained = nullptr;
			cobj->AssignRemoval();
		}
	}
	// remove this object from container *after* its contents have been removed!
	if (pCont)
	{
		pCont->Contents.Remove(this);
		pCont->UpdateMass();
		pCont->SetOCF();
		Contained=nullptr;
	}
	// Object info
	if (Info) Info->Retire();
	Info = nullptr;
	// Object system operation
	ClearRefs();
	Game.ClearPointers(this);
	ClearCommands();
	if (pSolidMaskData)
	{
		delete pSolidMaskData;
		pSolidMaskData = nullptr;
	}
	SolidMask.Wdt = 0;
}

void C4Object::UpdateShape(bool bUpdateVertices)
{

	// Line shape independent
	if (Def->Line) return;

	// Copy shape from def
	Shape.CopyFrom(Def->Shape, bUpdateVertices, !!fOwnVertices);

	// Construction zoom
	if (Con!=FullCon)
	{
		if (Def->GrowthType)
			Shape.Stretch(Con, bUpdateVertices);
		else
			Shape.Jolt(Con, bUpdateVertices);
	}

	// Rotation
	if (Def->Rotateable)
		if (fix_r != Fix0)
			Shape.Rotate(fix_r, bUpdateVertices);

	// covered area changed? to be on the save side, update pos
	UpdatePos();
}

bool C4Object::ExecLife()
{
	// Breathing
	if (!::Game.iTick5)
		if (Alive && !Def->NoBreath)
		{
			// Supply check
			bool Breathe=false;
			// Forcefields are breathable.
			if (GBackMat(GetX(), GetY()+Shape.GetY()/2)==MVehic)
				{ Breathe=true; }
			else if (GetPropertyInt(P_BreatheWater))
				{ if (GBackMat(GetX(), GetY())==MWater) Breathe=true; }
			else
				{ if (!GBackSemiSolid(GetX(), GetY()+Shape.GetY()/2)) Breathe=true; }
			if (Contained) Breathe=true;
			// No supply
			if (!Breathe)
			{
				// Reduce breath, then energy, bubble
				if (Breath > 0) DoBreath(-5);
				else DoEnergy(-1,false,C4FxCall_EngAsphyxiation, NO_OWNER);
			}
			// Supply
			else
			{
				// Take breath
				int32_t takebreath = GetPropertyInt(P_MaxBreath) - Breath;
				if (takebreath > 0) DoBreath(takebreath);
			}
		}

	// Corrosion energy loss
	if (!::Game.iTick10)
		if (Alive)
			if (InMat!=MNone)
				if (::MaterialMap.Map[InMat].Corrosive)
					if (!GetPropertyInt(P_CorrosionResist))
						DoEnergy(-::MaterialMap.Map[InMat].Corrosive/15,false,C4FxCall_EngCorrosion, NO_OWNER);

	// InMat incineration
	if (!::Game.iTick10)
		if (InMat!=MNone)
			if (::MaterialMap.Map[InMat].Incendiary)
				if (GetPropertyInt(P_ContactIncinerate) > 0 || GetPropertyBool(P_MaterialIncinerate) > 0)
				{
					Call(PSF_OnInIncendiaryMaterial, &C4AulParSet());
				}

	// birthday
	if (!::Game.iTick255)
		if (Alive)
			if (Info)
			{
				int32_t iPlayingTime = Info->TotalPlayingTime + (Game.Time - Info->InActionTime);

				int32_t iNewAge = iPlayingTime / 3600 / 5;

				if (Info->Age != iNewAge && Info->Age)
				{
					// message
					GameMsgObject(FormatString(LoadResStr("IDS_OBJ_BIRTHDAY"),GetName (), Info->TotalPlayingTime / 3600 / 5).getData(),this);
					StartSoundEffect("UI::Trumpet",false,100,this);
				}

				Info->Age = iNewAge;


			}

	return true;
}

void C4Object::Execute()
{
	if (Config.General.DebugRec)
	{
		// record debug
		C4RCExecObj rc;
		rc.Number=Number;
		rc.fx=fix_x;
		rc.fy=fix_y;
		rc.fr=fix_r;
		AddDbgRec(RCT_ExecObj, &rc, sizeof(rc));
	}
	// OCF
	UpdateOCF();
	// Command
	ExecuteCommand();
	// Action
	// need not check status, because dead objects have lost their action
	ExecAction();
	// commands and actions are likely to have removed the object, and movement
	// *must not* be executed for dead objects (SolidMask-errors)
	if (!Status) return;
	// Movement
	ExecMovement();
	if (!Status) return;
	// effects
	if (pEffects)
	{
		C4Effect::Execute(&pEffects);
		if (!Status) return;
	}
	// Life
	ExecLife();
	// Animation. If the mesh is attached, then don't execute animation here but let the parent object do it to make sure it is only executed once a frame.
	if (pMeshInstance && !pMeshInstance->GetAttachParent())
		pMeshInstance->ExecuteAnimation(1.0f/37.0f /* play smoothly at 37 FPS */);
	// Menu
	if (Menu) Menu->Execute();
}

void C4Object::AssignDeath(bool fForced)
{
	C4Object *thing;
	// Alive objects only
	if (!Alive) return;
	// clear all effects
	// do not delete effects afterwards, because they might have denied removal
	// set alive-flag before, so objects know what's up
	// and prevent recursive death-calls this way
	// get death causing player before doing effect calls, because those might meddle around with the flags
	int32_t iDeathCausingPlayer = LastEnergyLossCausePlayer;
	Alive=false;
	if (pEffects) pEffects->ClearAll(C4FxCall_RemoveDeath);
	// if the object is alive again, abort here if the kill is not forced
	if (Alive && !fForced) return;
	// Action
	SetActionByName("Dead");
	// Values
	Alive=false;
	ClearCommands();
	C4ObjectInfo * pInfo = Info;
	if (Info)
	{
		Info->HasDied=true;
		++Info->DeathCount;
		Info->Retire();
	}
	// Remove from crew/cursor/view
	C4Player *pPlr = ::Players.Get(Owner);
	if (pPlr) pPlr->ClearPointers(this, true);
	// Remove from light sources
	SetLightRange(0,0);
	// Engine script call
	C4AulParSet pars(iDeathCausingPlayer);
	Call(PSF_Death, &pars);
	// Lose contents
	while ((thing=Contents.GetObject())) thing->Exit(thing->GetX(),thing->GetY());
	// Update OCF. Done here because previously it would have been done in the next frame
	// Whats worse: Having the OCF change because of some unrelated script-call like
	// SetCategory, or slightly breaking compatibility?
	SetOCF();
	// Engine broadcast: relaunch player (in CR, this was called from clonk script.
	// Now, it is done for every crew member)
	if(pPlr)
		if(!pPlr->Crew.ObjectCount())
			::Game.GRBroadcast(PSF_RelaunchPlayer,
			                   &C4AulParSet(Owner, iDeathCausingPlayer, Status ? this : nullptr));
	if (pInfo)
		pInfo->HasDied = false;
}

bool C4Object::ChangeDef(C4ID idNew)
{
	// Get new definition
	C4Def *pDef=C4Id2Def(idNew);
	if (!pDef) return false;
	// Containment storage
	C4Object *pContainer=Contained;
	// Exit container (no Ejection/Departure)
	if (Contained) Exit(0,0,0,Fix0,Fix0,Fix0,false);
	// Pre change resets
	SetAction(nullptr);
	ResetProperty(&Strings.P[P_Action]); // Enforce ActIdle because SetAction may have failed due to NoOtherAction
	SetDir(0); // will drop any outdated flipdir
	if (pSolidMaskData) { delete pSolidMaskData; pSolidMaskData=nullptr; }
	Def->Count--;
	// Def change
	Def=pDef;
	SetProperty(P_Prototype, C4VPropList(pDef));
	id=pDef->id;
	Def->Count++;
	// new def: Needs to be resorted
	Unsorted=true;
	// graphics change
	pGraphics = &pDef->Graphics;
	// blit mode adjustment
	if (!(BlitMode & C4GFXBLIT_CUSTOM)) BlitMode = Def->BlitMode;
	// an object may have newly become an ColorByOwner-object
	// if it had been ColorByOwner, but is not now, this will be caught in UpdateGraphics()
	if (!Color && ValidPlr(Owner))
		Color=::Players.Get(Owner)->ColorDw;
	if (!Def->Rotateable) { fix_r=rdir=Fix0; }
	// Reset solid mask
	SolidMask=Def->SolidMask;
	HalfVehicleSolidMask=false;
	// Post change updates
	UpdateGraphics(true);
	UpdateMass();
	UpdateFace(true);
	SetOCF();
	// Any effect callbacks to this object might need to reinitialize their target functions
	// This is ugly, because every effect there is must be updated...
	if (::ScriptEngine.pGlobalEffects)
		::ScriptEngine.pGlobalEffects->OnObjectChangedDef(this);
	if (::GameScript.pScenarioEffects)
		::GameScript.pScenarioEffects->OnObjectChangedDef(this);
	for (C4Object *obj : Objects)
		if (obj->pEffects) obj->pEffects->OnObjectChangedDef(this);
	// Containment (no Entrance)
	if (pContainer) Enter(pContainer,false);
	// Done
	return true;
}

void C4Object::DoDamage(int32_t iChange, int32_t iCausedBy, int32_t iCause)
{
	// non-living: ask effects first
	if (pEffects && !Alive)
	{
		pEffects->DoDamage(iChange, iCause, iCausedBy);
		if (!iChange) return;
	}
	// Change value
	Damage = std::max<int32_t>( Damage+iChange, 0 );
	// Engine script call
	Call(PSF_Damage,&C4AulParSet(iChange, iCause, iCausedBy));
}

void C4Object::DoEnergy(int32_t iChange, bool fExact, int32_t iCause, int32_t iCausedByPlr)
{
	if (!fExact)
	{
		// Clamp range of change to prevent integer overflow errors
		// Do not clamp directly to (0...MaxEnergy)-current_energy, because
		// the change value calculated here may be reduced by effect callbacks
		int32_t scale = C4MaxPhysical / 100; // iChange 100% = Physical 100000
		iChange = Clamp<int32_t>(iChange, std::numeric_limits<int32_t>::min()/scale, std::numeric_limits<int32_t>::max()/scale)*scale;
	}
	// Was zero?
	bool fWasZero=(Energy==0);
	// Mark last damage causing player to trace kills
	if (iChange < 0) UpdatLastEnergyLossCause(iCausedByPlr);
	// Living things: ask effects for change first
	if (pEffects && Alive)
		pEffects->DoDamage(iChange, iCause, iCausedByPlr);
	// Do change
	iChange = Clamp<int32_t>(iChange, -Energy, GetPropertyInt(P_MaxEnergy) - Energy);
	Energy += iChange;
	// call to object
	Call(PSF_EnergyChange,&C4AulParSet(iChange, iCause, iCausedByPlr));
	// Alive and energy reduced to zero: death
	if (Alive) if (Energy==0) if (!fWasZero) AssignDeath(false);
}

void C4Object::UpdatLastEnergyLossCause(int32_t iNewCausePlr)
{
	// Mark last damage causing player to trace kills
	// do not regard self-administered damage if there was a previous damage causing player, because that would steal kills
	// if people tumble themselves via stop-stop-(left/right)-throw  while falling into teh abyss
	if (iNewCausePlr != Controller || LastEnergyLossCausePlayer < 0)
	{
		LastEnergyLossCausePlayer = iNewCausePlr;
	}
}

void C4Object::DoBreath(int32_t iChange)
{
	// Do change
	iChange = Clamp<int32_t>(iChange, -Breath, GetPropertyInt(P_MaxBreath) - Breath);
	Breath += iChange;
	// call to object
	Call(PSF_BreathChange,&C4AulParSet(iChange));
}

void C4Object::DoCon(int32_t iChange, bool grow_from_center)
{
	C4Real strgt_con_b = fix_y + Shape.GetBottom();
	bool fWasFull = (Con>=FullCon);
	int32_t old_con = Con;

	// Change con
	if (Def->Oversize)
		Con=std::max<int32_t>(Con+iChange,0);
	else
		Con=Clamp<int32_t>(Con+iChange,0,FullCon);

	// Update OCF
	SetOCF();

	// Mass
	UpdateMass();

	// shape and position
	UpdateShape();
	// make the bottom-most vertex stay in place
	if (!grow_from_center)
	{
		fix_y = strgt_con_b - Shape.GetBottom();
	}
	// Face (except for the shape)
	UpdateFace(false);

	// Do a callback on completion change.
	if (iChange != 0)
		Call(PSF_OnCompletionChange, &C4AulParSet(old_con, Con));

	// Unfullcon
	if (fWasFull && (Con<FullCon))
	{
		// Lose contents
		if (!Def->IncompleteActivity)
		{
			C4Object *cobj;
			while ((cobj=Contents.GetObject()))
				if (Contained) cobj->Enter(Contained);
				else cobj->Exit(cobj->GetX(),cobj->GetY());
			SetAction(nullptr);
		}
	}

	// Completion
	if (!fWasFull && (Con>=FullCon))
		Call(PSF_Initialize);

	// Con Zero Removal
	if (Con<=0)
		AssignRemoval();
	// Mesh Graphics Update
	else if(pMeshInstance)
		pMeshInstance->SetCompletion(Def->GrowthType ? 1.0f : static_cast<float>(Con)/static_cast<float>(FullCon));
}

bool C4Object::ActivateEntrance(int32_t by_plr, C4Object *by_obj)
{

	// Try entrance activation
	if (OCF & OCF_Entrance)
		if (!! Call(PSF_ActivateEntrance,&C4AulParSet(by_obj)))
			return true;
	// Failure
	return false;
}

BYTE C4Object::GetArea(int32_t &aX, int32_t &aY, int32_t &aWdt, int32_t &aHgt) const
{
	if (!Status || !Def) return 0;
	aX = GetX() + Shape.GetX(); aY = GetY() + Shape.GetY();
	aWdt=Shape.Wdt; aHgt=Shape.Hgt;
	return 1;
}

BYTE C4Object::GetEntranceArea(int32_t &aX, int32_t &aY, int32_t &aWdt, int32_t &aHgt) const
{
	if (!Status || !Def) return 0;
	// Return actual entrance
	if (OCF & OCF_Entrance)
	{
		aX=GetX() + Def->Entrance.x;
		aY=GetY() + Def->Entrance.y;
		aWdt=Def->Entrance.Wdt;
		aHgt=Def->Entrance.Hgt;
	}
	// Return object center
	else
	{
		aX=GetX(); aY=GetY();
		aWdt=0; aHgt=0;
	}
	// Done
	return 1;
}

StdStrBuf C4Object::GetDataString()
{
	StdStrBuf Output;
	// Type
	Output.AppendFormat(LoadResStr("IDS_CNS_TYPE"),GetName(),Def->id.ToString());
	// Owner
	if (ValidPlr(Owner))
	{
		Output.Append("\n");
		Output.AppendFormat(LoadResStr("IDS_CNS_OWNER"),::Players.Get(Owner)->GetName());
	}
	// Contents
	if (Contents.ObjectCount())
	{
		Output.Append("\n");
		Output.Append(LoadResStr("IDS_CNS_CONTENTS"));
		Output.Append(Contents.GetNameList(::Definitions));
	}
	// Action
	if (GetAction())
	{
		Output.Append("\n");
		Output.Append(LoadResStr("IDS_CNS_ACTION"));
		Output.Append(GetAction()->GetName());
	}
	// Properties
	Output.Append("\n");
	Output.Append(LoadResStr("IDS_CNS_PROPERTIES"));
	Output.Append("\n  ");
	AppendDataString(&Output, "\n  ");
	// Effects
	if (pEffects)
	{
		Output.Append("\n");
		Output.Append(LoadResStr("IDS_CNS_EFFECTS"));
		Output.Append(": ");
	}
	for (C4Effect *pEffect = pEffects; pEffect; pEffect = pEffect->pNext)
	{
		Output.Append("\n");
		// Effect name
		Output.AppendFormat("  %s: Priority %d, Interval %d", pEffect->GetName(), pEffect->iPriority, pEffect->iInterval);
	}

	StdStrBuf Output2;
	C4ValueNumbers numbers;
	DecompileToBuf_Log<StdCompilerINIWrite>(mkNamingAdapt(mkInsertAdapt(mkParAdapt(*this, &numbers),
	                                                                    mkNamingAdapt(numbers, "Values"), false),
	                                                      "Object"), &Output2, "C4Object::GetDataString");
	Output.Append("\n");
	Output.Append(Output2);
	return Output;
}

void C4Object::SetName(const char * NewName)
{
	if (!NewName && Info)
		C4PropList::SetName(Info->Name);
	else
		C4PropList::SetName(NewName);
}

int32_t C4Object::GetValue(C4Object *pInBase, int32_t iForPlayer)
{
	C4Value r = Call(PSF_CalcValue, &C4AulParSet(pInBase, iForPlayer));
	int32_t iValue;
	if (r != C4VNull)
		iValue = r.getInt();
	else
	{
		// get value of def
		// Caution: Do not pass pInBase here, because the def base value is to be queried
		//  - and not the value if you had to buy the object in this particular base
		iValue = Def->GetValue(nullptr, iForPlayer);
	}
	// Con percentage
	iValue = iValue * Con / FullCon;
	// do any adjustments based on where the item is bought
	if (pInBase)
	{
		r = pInBase->Call(PSF_CalcSellValue, &C4AulParSet(this, iValue));
		if (r != C4VNull)
			iValue = r.getInt();
	}
	return iValue;
}

void C4Object::ClearPointers(C4Object *pObj)
{
	// mesh attachments and animation nodes
	if(pMeshInstance) pMeshInstance->ClearPointers(pObj);
	// effects
	if (pEffects) pEffects->ClearPointers(pObj);
	// contents/contained: although normally not necessery because it's done in AssignRemoval and StatusDeactivate,
	// it is also required during game destruction (because ClearPointers might do script callbacks)
	// Perform silent exit to avoid additional callbacks
	if (Contained == pObj)
	{
		Contained->Contents.Remove(this);
		Contained = nullptr;
	}
	Contents.Remove(pObj);
	// Action targets
	if (Action.Target==pObj) Action.Target=nullptr;
	if (Action.Target2==pObj) Action.Target2=nullptr;
	// Commands
	C4Command *cCom;
	for (cCom=Command; cCom; cCom=cCom->Next)
		cCom->ClearPointers(pObj);
	// Menu
	if (Menu) Menu->ClearPointers(pObj);
	// Layer
	if (Layer==pObj) Layer=nullptr;
	// gfx overlays
	if (pGfxOverlay)
	{
		C4GraphicsOverlay *pNextGfxOvrl = pGfxOverlay, *pGfxOvrl;
		while ((pGfxOvrl = pNextGfxOvrl))
		{
			pNextGfxOvrl = pGfxOvrl->GetNext();
			if (pGfxOvrl->GetOverlayObject() == pObj)
				// overlay relying on deleted object: Delete!
				RemoveGraphicsOverlay(pGfxOvrl->GetID());
		}
	}
}

void C4Object::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing)
		Clear();

	// Compile ID, search definition
	pComp->Value(mkNamingAdapt(id,                  "id",                 C4ID::None          ));
	if (deserializing)
	{
		Def = ::Definitions.ID2Def(id);
		if (!Def)
			{ pComp->excNotFound(LoadResStr("IDS_PRC_UNDEFINEDOBJECT"),id.ToString()); return; }
	}

	pComp->Value(mkNamingAdapt( mkParAdapt(static_cast<C4PropListNumbered&>(*this), numbers), "Properties"));
	pComp->Value(mkNamingAdapt( Status,                           "Status",             1                 ));
	if (Info) nInfo = Info->Name; else nInfo.Clear();
	pComp->Value(mkNamingAdapt( toC4CStrBuf(nInfo),               "Info",               ""                ));
	pComp->Value(mkNamingAdapt( Owner,                            "Owner",              NO_OWNER          ));
	pComp->Value(mkNamingAdapt( Controller,                       "Controller",         NO_OWNER          ));
	pComp->Value(mkNamingAdapt( LastEnergyLossCausePlayer,        "LastEngLossPlr",     NO_OWNER          ));
	pComp->Value(mkNamingAdapt( Category,                         "Category",           0                 ));
	pComp->Value(mkNamingAdapt( Plane,                            "Plane",              0                 ));

	pComp->Value(mkNamingAdapt( iLastAttachMovementFrame,         "LastSolidAtchFrame", -1                ));
	pComp->Value(mkNamingAdapt( Con,                              "Size",               0                 ));
	pComp->Value(mkNamingAdapt( OwnMass,                          "OwnMass",            0                 ));
	pComp->Value(mkNamingAdapt( Mass,                             "Mass",               0                 ));
	pComp->Value(mkNamingAdapt( Damage,                           "Damage",             0                 ));
	pComp->Value(mkNamingAdapt( Energy,                           "Energy",             0                 ));
	pComp->Value(mkNamingAdapt( Alive,                            "Alive",              false             ));
	pComp->Value(mkNamingAdapt( Breath,                           "Breath",             0                 ));
	pComp->Value(mkNamingAdapt( Color,                            "Color",              0u                ));
	pComp->Value(mkNamingAdapt( fix_x,                            "X",                  Fix0              ));
	pComp->Value(mkNamingAdapt( fix_y,                            "Y",                  Fix0              ));
	pComp->Value(mkNamingAdapt( fix_r,                            "R",                  Fix0              ));
	pComp->Value(mkNamingAdapt( xdir,                             "XDir",               0                 ));
	pComp->Value(mkNamingAdapt( ydir,                             "YDir",               0                 ));
	pComp->Value(mkNamingAdapt( rdir,                             "RDir",               0                 ));
	pComp->Value(mkParAdapt(Shape, &Def->Shape));
	pComp->Value(mkNamingAdapt( fOwnVertices,                     "OwnVertices",        false             ));
	pComp->Value(mkNamingAdapt( SolidMask,                        "SolidMask",          Def->SolidMask    ));
	pComp->Value(mkNamingAdapt( HalfVehicleSolidMask,             "HalfVehicleSolidMask", false           ));
	pComp->Value(mkNamingAdapt( PictureRect,                      "Picture"                               ));
	pComp->Value(mkNamingAdapt( Mobile,                           "Mobile",             false             ));
	pComp->Value(mkNamingAdapt( OnFire,                           "OnFire",             false             ));
	pComp->Value(mkNamingAdapt( InLiquid,                         "InLiquid",           false             ));
	pComp->Value(mkNamingAdapt( EntranceStatus,                   "EntranceStatus",     false             ));
	pComp->Value(mkNamingAdapt( OCF,                              "OCF",                0u                ));
	pComp->Value(Action);
	pComp->Value(mkNamingAdapt( Contained,                        "Contained",          C4ObjectPtr::Null ));
	pComp->Value(mkNamingAdapt( Action.Target,                    "ActionTarget1",      C4ObjectPtr::Null ));
	pComp->Value(mkNamingAdapt( Action.Target2,                   "ActionTarget2",      C4ObjectPtr::Null ));
	pComp->Value(mkNamingAdapt( mkParAdapt(Contents, numbers),    "Contents"                              ));
	pComp->Value(mkNamingAdapt( lightRange,                       "LightRange",         0                 ));
	pComp->Value(mkNamingAdapt( lightFadeoutRange,                "LightFadeoutRange",  0                 ));
	pComp->Value(mkNamingAdapt( lightColor,                       "lightColor",         0xffffffffu       ));
	pComp->Value(mkNamingAdapt( ColorMod,                         "ColorMod",           0xffffffffu       ));
	pComp->Value(mkNamingAdapt( BlitMode,                         "BlitMode",           0u                ));
	pComp->Value(mkNamingAdapt( CrewDisabled,                     "CrewDisabled",       false             ));
	pComp->Value(mkNamingAdapt( Layer,                            "Layer",              C4ObjectPtr::Null ));
	pComp->Value(mkNamingAdapt( C4DefGraphicsAdapt(pGraphics),    "Graphics",           &Def->Graphics    ));
	pComp->Value(mkNamingPtrAdapt( pDrawTransform,                "DrawTransform"                         ));
	pComp->Value(mkParAdapt(mkNamingPtrAdapt( pEffects,           "Effects"                               ), this, numbers));
	pComp->Value(mkNamingAdapt( C4GraphicsOverlayListAdapt(pGfxOverlay),"GfxOverlay",   (C4GraphicsOverlay *)nullptr));

	// Serialize mesh instance if we have a mesh graphics
	if(pGraphics->Type == C4DefGraphics::TYPE_Mesh)
	{
		if(pComp->isDeserializer())
		{
			assert(!pMeshInstance);
			pMeshInstance = new StdMeshInstance(*pGraphics->Mesh, Def->GrowthType ? 1.0f : static_cast<float>(Con)/static_cast<float>(FullCon));
		}

		pComp->Value(mkNamingAdapt(mkParAdapt(*pMeshInstance, C4MeshDenumeratorFactory), "Mesh"));

		// Does not work because unanimated meshes without attached meshes
		// do not even write a [Mesh] header so this does not create a mesh instance in that case
/*		pComp->Value(mkNamingContextPtrAdapt( pMeshInstance, *pGraphics->Mesh, "Mesh"));
		if(!pMeshInstance)
			pComp->excCorrupt("Mesh graphics without mesh instance");*/
	}

	// TODO: Animations / attached meshes

	// Commands
	if (pComp->FollowName("Commands"))
	{
		if (deserializing)
		{
			C4Command *pCmd = nullptr;
			for (int i = 1; ; i++)
			{
				// Every command has its own naming environment
				StdStrBuf Naming = FormatString("Command%d", i);
				pComp->Value(mkParAdapt(mkNamingPtrAdapt(pCmd ? pCmd->Next : Command, Naming.getData()), numbers));
				// Last command?
				pCmd = (pCmd ? pCmd->Next : Command);
				if (!pCmd)
					break;
				pCmd->cObj = this;
			}
		}
		else
		{
			C4Command *pCmd = Command;
			for (int i = 1; pCmd; i++, pCmd = pCmd->Next)
			{
				StdStrBuf Naming = FormatString("Command%d", i);
				pComp->Value(mkNamingAdapt(mkParAdapt(*pCmd, numbers), Naming.getData()));
			}
		}
	}

	// Compiling? Do initialization.
	if (deserializing)
	{
		// add to def count
		Def->Count++;


		// Set action (override running data)
		/* FIXME
		int32_t iTime=Action.Time;
		int32_t iPhase=Action.Phase;
		int32_t iPhaseDelay=Action.PhaseDelay;
		if (SetActionByName(Action.pActionDef->GetName(),0,0,false))
		  {
		  Action.Time=iTime;
		  Action.Phase=iPhase; // No checking for valid phase
		  Action.PhaseDelay=iPhaseDelay;
		  }*/

		if (pMeshInstance)
		{
			// Set Action animation by slot 0
			Action.Animation = pMeshInstance->GetRootAnimationForSlot(0);
			pMeshInstance->SetFaceOrderingForClrModulation(ColorMod);
		}

		// blit mode not assigned? use definition default then
		if (!BlitMode) BlitMode = Def->BlitMode;

		// object needs to be resorted? May happen if there's unsorted objects in savegame
		if (Unsorted) Game.fResortAnyObject = true;
	}

}

void C4Object::Denumerate(C4ValueNumbers * numbers)
{
	C4PropList::Denumerate(numbers);
	// Standard enumerated pointers
	Contained.DenumeratePointers();
	Action.Target.DenumeratePointers();
	Action.Target2.DenumeratePointers();
	Layer.DenumeratePointers();

	// Post-compile object list
	Contents.DenumeratePointers();

	// Commands
	for (C4Command *pCom=Command; pCom; pCom=pCom->Next)
		pCom->Denumerate(numbers);

	// effects
	if (pEffects) pEffects->Denumerate(numbers);

	// gfx overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			pGfxOvrl->DenumeratePointers();

	// mesh instance
	if (pMeshInstance) pMeshInstance->DenumeratePointers();
}

bool C4Object::ValidateOwner()
{
	// Check owner and controller
	if (!ValidPlr(Owner)) Owner=NO_OWNER;
	if (!ValidPlr(Controller)) Controller=NO_OWNER;
	// Color is not reset any more, because many scripts change colors to non-owner-colors these days
	// Additionally, player colors are now guarantueed to remain the same in savegame resumes
	return true;
}

bool C4Object::AssignInfo()
{
	if (Info || !ValidPlr(Owner)) return false;
	// In crew list?
	C4Player *pPlr = ::Players.Get(Owner);
	if (pPlr->Crew.GetLink(this))
	{
		// Register with player
		if (!::Players.Get(Owner)->MakeCrewMember(this, true, false))
			pPlr->Crew.Remove(this);
		return true;
	}
	// Info set, but not in crew list, so
	//    a) The savegame is old-style (without crew list)
	// or b) The clonk is dead
	// or c) The clonk belongs to a script player that's restored without Game.txt
	else if (nInfo.getLength())
	{
		if (!::Players.Get(Owner)->MakeCrewMember(this, true, false))
			return false;
		// Dead and gone (info flags, remove from crew/cursor)
		if (!Alive)
		{
			if (ValidPlr(Owner)) ::Players.Get(Owner)->ClearPointers(this, true);
		}
		return true;
	}
	return false;
}

void C4Object::ClearInfo(C4ObjectInfo *pInfo)
{
	if (Info==pInfo)
	{
		Info=nullptr;
	}
}

void C4Object::Clear()
{
	ClearParticleLists();

	delete pEffects; pEffects = nullptr;
	delete pSolidMaskData; pSolidMaskData = nullptr;
	delete Menu; Menu = nullptr;
	delete MaterialContents; MaterialContents = nullptr;
	// clear commands!
	C4Command *pCom, *pNext;
	for (pCom=Command; pCom; pCom=pNext)
	{
		pNext=pCom->Next; delete pCom; pCom=pNext;
	}
	delete pDrawTransform; pDrawTransform = nullptr;
	delete pGfxOverlay; pGfxOverlay = nullptr;
	delete pMeshInstance; pMeshInstance = nullptr;
}

void C4Object::SyncClearance()
{
	// Misc. no-save safeties
	Action.t_attach = CNAT_None;
	InMat = MNone;
	t_contact = 0;
	// Update OCF
	SetOCF();
	// Menu
	CloseMenu(true);
	// Material contents
	delete MaterialContents; MaterialContents=nullptr;
	// reset speed of staticback-objects
	if (Category & C4D_StaticBack)
	{
		xdir = ydir = 0;
	}
}

void C4Object::Resort()
{
	// Flag resort
	Unsorted=true;
	Game.fResortAnyObject = true;
	// Must not immediately resort - link change/removal would crash Game::ExecObjects
}

bool C4Object::SetOwner(int32_t iOwner)
{
	// Check valid owner
	if (!(ValidPlr(iOwner) || iOwner == NO_OWNER)) return false;
	// always set color, even if no owner-change is done
	if (iOwner != NO_OWNER)
		if (GetGraphics()->IsColorByOwner())
		{
			Color=::Players.Get(iOwner)->ColorDw;
			UpdateFace(false);
		}
	// no change?
	if (Owner == iOwner) return true;
	// set new owner
	int32_t iOldOwner=Owner;
	Owner=iOwner;
	// this automatically updates controller
	Controller = Owner;
	// script callback
	Call(PSF_OnOwnerChanged, &C4AulParSet(Owner, iOldOwner));
	// done
	return true;
}

bool C4Object::GrabInfo(C4Object *pFrom)
{
	// safety
	if (!pFrom) return false;
	if (!Status || !pFrom->Status) return false;
	// even more safety (own info: success)
	if (pFrom == this) return true;
	// only if other object has info
	if (!pFrom->Info) return false;
	// clear own info object
	if (Info)
	{
		Info->Retire();
		ClearInfo (Info);
	}
	// remove objects from any owning crews
	::Players.ClearPointers(pFrom);
	::Players.ClearPointers(this);
	// set info
	Info = pFrom->Info; pFrom->ClearInfo (pFrom->Info);
	// set name
	SetName(Info->Name);
	// retire from old crew
	Info->Retire();
	// if alive, recruit to new crew
	if (Alive) Info->Recruit();
	// make new crew member
	C4Player *pPlr = ::Players.Get(Owner);
	if (pPlr) pPlr->MakeCrewMember(this);
	// done, success
	return true;
}

bool C4Object::DoSelect()
{
	// selection allowed?
	if (CrewDisabled) return false;
	// do callback
	Call(PSF_CrewSelection, &C4AulParSet(false));
	// done
	return true;
}

void C4Object::UnSelect()
{
	// do callback
	Call(PSF_CrewSelection, &C4AulParSet(true));
}

bool C4Object::StatusActivate()
{
	// readd to main list
	::Objects.InactiveObjects.Remove(this);
	Status = C4OS_NORMAL;
	::Objects.Add(this);
	// update some values
	UpdateGraphics(false);
	UpdateFace(true);
	UpdatePos();
	UpdateLight();
	Call(PSF_OnSynchronized);
	// done, success
	return true;
}

bool C4Object::StatusDeactivate(bool fClearPointers)
{
	// clear particles
	ClearParticleLists();

	// put into inactive list
	::Objects.Remove(this);
	Status = C4OS_INACTIVE;
	if (Landscape.HasFoW()) Landscape.GetFoW()->Remove(this);
	::Objects.InactiveObjects.Add(this, C4ObjectList::stMain);
	// if desired, clear game pointers
	if (fClearPointers)
	{
		// in this case, the object must also exit any container, and any contained objects must be exited
		ClearContentsAndContained();
		Game.ClearPointers(this);
	}
	else
	{
		// always clear transfer
		Game.TransferZones.ClearPointers(this);
	}
	// done, success
	return true;
}

StdStrBuf C4Object::GetInfoString()
{
	StdStrBuf sResult;
	// no info for invalid objects
	if (!Status) return sResult;
	// go through all effects and add their desc
	for (C4Effect *pEff = pEffects; pEff; pEff = pEff->pNext)
	{
		C4Value par[7];
		C4Value vInfo = pEff->DoCall(this, PSFS_FxInfo, par[0], par[1], par[2], par[3], par[4], par[5], par[6]);
		if (!vInfo) continue;
		// debug: warn for wrong return types
		if (vInfo.GetType() != C4V_String)
			DebugLogF("Effect %s(#%d) on object %s (#%d) returned wrong info type %d.", pEff->GetName(), pEff->Number, GetName(), Number, vInfo.GetType());
		// get string val
		C4String *psInfo = vInfo.getStr(); const char *szEffInfo;
		if (psInfo && (szEffInfo = psInfo->GetCStr()))
			if (*szEffInfo)
			{
				// OK; this effect has a desc. Add it!
				if (sResult.getLength()) sResult.AppendChar('|');
				sResult.Append(szEffInfo);
			}
	}
	// done
	return sResult;
}

void C4Object::UpdateScriptPointers()
{
	if (pEffects)
		pEffects->ReAssignAllCallbackFunctions();
}

bool C4Object::IsPlayerObject(int32_t iPlayerNumber) const
{
	bool fAnyPlr = (iPlayerNumber == NO_OWNER);
	// if an owner is specified: only owned objects
	if (fAnyPlr && !ValidPlr(Owner)) return false;
	// and crew objects
	if (fAnyPlr || Owner == iPlayerNumber)
	{
		C4Player *pOwner = ::Players.Get(Owner);
		if (pOwner)
		{
			if (pOwner && pOwner->Crew.IsContained(this)) return true;
		}
		else
		{
			// Do not force that the owner exists because the function must work for unjoined players (savegame resume)
			if (Def->CrewMember)
				return true;
		}
	}
	// otherwise, not a player object
	return false;
}

bool C4Object::IsUserPlayerObject()
{
	// must be a player object at all
	if (!IsPlayerObject()) return false;
	// and the owner must not be a script player
	C4Player *pOwner = ::Players.Get(Owner);
	if (!pOwner || pOwner->GetType() != C4PT_User) return false;
	// otherwise, it's a user playeer object
	return true;
}

void C4Object::SetPropertyByS(C4String * k, const C4Value & to)
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Plane:
				if (!to.getInt()) throw C4AulExecError("invalid Plane 0");
				SetPlane(to.getInt());
				return;
		}
	}
	C4PropListNumbered::SetPropertyByS(k, to);
}

void C4Object::ResetProperty(C4String * k)
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Plane:
				SetPlane(GetPropertyInt(P_Plane));
				return;
		}
	}
	return C4PropListNumbered::ResetProperty(k);
}

bool C4Object::GetPropertyByS(const C4String *k, C4Value *pResult) const
{
	if (k >= &Strings.P[0] && k < &Strings.P[P_LAST])
	{
		switch(k - &Strings.P[0])
		{
			case P_Plane: *pResult = C4VInt(Plane); return true;
		}
	}
	return C4PropListNumbered::GetPropertyByS(k, pResult);
}

C4ValueArray * C4Object::GetProperties() const
{
	C4ValueArray * a = C4PropList::GetProperties();
	int i;
	i = a->GetSize();
	a->SetSize(i + 1);
	(*a)[i++] = C4VString(&::Strings.P[P_Plane]);
	return a;
}
