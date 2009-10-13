/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2001, 2003-2004, 2007-2008  Matthes Bender
 * Copyright (c) 2001-2007  Peter Wortmann
 * Copyright (c) 2001-2009  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2001  Carlo Teubner
 * Copyright (c) 2004-2009  Günther Brammer
 * Copyright (c) 2005  Tobias Zwick
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
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

/* That which fills the world with life */

#include <C4Include.h>
#include <C4Object.h>

#ifndef BIG_C4INCLUDE
#include <C4ObjectInfo.h>
#include <C4Physics.h>
#include <C4ObjectCom.h>
#include <C4Command.h>
#include <C4Viewport.h>
#include <C4MaterialList.h>
#ifdef DEBUGREC
#include <C4Record.h>
#endif
#include <C4SolidMask.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4Player.h>
#include <C4ObjectMenu.h>
#include <C4RankSystem.h>
#include <C4GameMessage.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4Record.h>
#endif

void DrawVertex(C4Facet &cgo, int32_t tx, int32_t ty, int32_t col, int32_t contact)
  {
  if (Inside<int32_t>(tx,1,cgo.Wdt-2) && Inside<int32_t>(ty,1,cgo.Hgt-2))
    {
    Application.DDraw->DrawHorizontalLine(cgo.Surface,cgo.X+tx-1,cgo.X+tx+1,cgo.Y+ty,col);
    Application.DDraw->DrawVerticalLine(cgo.Surface,cgo.X+tx,cgo.Y+ty-1,cgo.Y+ty+1,col);
    if (contact) Application.DDraw->DrawFrame(cgo.Surface,cgo.X+tx-2,cgo.Y+ty-2,cgo.X+tx+2,cgo.Y+ty+2,CWhite);
    }
  }

void C4Action::SetBridgeData(int32_t iBridgeTime, bool fMoveClonk, bool fWall, int32_t iBridgeMaterial)
	{
	// validity
	iBridgeMaterial = Min<int32_t>(iBridgeMaterial, ::MaterialMap.Num-1);
	if (iBridgeMaterial < 0) iBridgeMaterial = 0xff;
	iBridgeTime = BoundBy<int32_t>(iBridgeTime, 0, 0xffff);
	// mask in this->Data
	Data = (uint32_t(iBridgeTime) << 16) + (uint32_t(fMoveClonk) << 8) + (uint32_t(fWall) << 9) + iBridgeMaterial;
	}

void C4Action::GetBridgeData(int32_t &riBridgeTime, bool &rfMoveClonk, bool &rfWall, int32_t &riBridgeMaterial)
	{
	// mask from this->Data
	uint32_t uiData = Data;
	riBridgeTime = (uint32_t(uiData) >> 16);
	rfMoveClonk = !!(uiData & 0x100);
	rfWall = !!(uiData & 0x200);
	riBridgeMaterial = (uiData & 0xff);
	if (riBridgeMaterial == 0xff) riBridgeMaterial = -1;
	}

C4Object::C4Object()
	{
	Default();
	}

void C4Object::Default()
	{
	id=C4ID_None;
	nInfo.Clear();
	RemovalDelay=0;
	Owner=NO_OWNER;
	Controller=NO_OWNER;
	LastEnergyLossCausePlayer=NO_OWNER;
	Category=0;
	NoCollectDelay=0;
	Con=0;
	Mass=OwnMass=0;
	Damage=0;
	Energy=0;
	MagicEnergy=0;
	Alive=0;
	Breath=0;
	FirePhase=0;
	InMat=MNone;
	Color=0;
	ViewEnergy=0;
	PlrViewRange=0;
	fix_x=fix_y=fix_r=0;
  xdir=ydir=rdir=0;
  Mobile=0;
  Select=0;
	Unsorted=false;
	Initializing=false;
  OnFire=0;
  InLiquid=0;
  EntranceStatus=0;
  Audible=0;
	NeedEnergy=0;
	Timer=0;
  t_contact=0;
  OCF=0;
  Action.Default();
  Shape.Default();
	fOwnVertices=0;
  Contents.Default();
  Component.Default();
	SolidMask.Default();
	PictureRect.Default();
	Def=NULL;
  Info=NULL;
  Command=NULL;
  Contained=NULL;
	TopFace.Default();
	nContained=nActionTarget1=nActionTarget2=0;
	Menu=NULL;
	PhysicalTemporary=false;
	TemporaryPhysical.Default();
	MaterialContents=NULL;
	LocalNamed.Reset();
	Marker=0;
	ColorMod=0xffffffff;
	BlitMode=0;
	CrewDisabled=false;
	pLayer=NULL;
	pSolidMaskData=NULL;
	pGraphics=NULL;
	pDrawTransform=NULL;
	pEffects=NULL;
	pGfxOverlay=NULL;
	iLastAttachMovementFrame=-1;
	}

bool C4Object::Init(C4PropList *pDef, C4Object *pCreator,
										int32_t iOwner, C4ObjectInfo *pInfo,
										int32_t nx, int32_t ny, int32_t nr,
										FIXED nxdir, FIXED nydir, FIXED nrdir, int32_t iController)
  {
	// currently initializing
	Initializing=true;

	// Def & basics
  Owner=iOwner;
	if (iController > NO_OWNER) Controller = iController; else Controller=iOwner;
	LastEnergyLossCausePlayer=NO_OWNER;
  Info=pInfo;
  Def=pDef->GetDef();assert(Def);
	prototype = pDef;
	id=Def->id;
	if (Info) SetName(pInfo->Name);
  Category=Def->Category;
	Def->Count++;
	if (pCreator) pLayer=pCreator->pLayer;

	// graphics
	pGraphics = &Def->Graphics;
	BlitMode = Def->BlitMode;

	// Position
	if (!Def->Rotateable) { nr=0; nrdir=0; }
	fix_x=itofix(nx);
	fix_y=itofix(ny);
  	r=nr;
	fix_r=itofix(r);
	xdir=nxdir; ydir=nydir; rdir=nrdir;

	// Initial mobility
	if (Category!=C4D_StaticBack)
    if (!!xdir || !!ydir || !!rdir)
      Mobile=1;

	// Mass
  Mass=Max<int32_t>(Def->Mass*Con/FullCon,1);

	// Life, energy, breath
	if (Category & C4D_Living) Alive=1;
  if (Alive) Energy=GetPhysical()->Energy;
  Breath=GetPhysical()->Breath;

	// Components
  Component=Def->Component;
  ComponentConCutoff();

	// Color
  if (Def->ColorByOwner)
    if (ValidPlr(Owner))
			Color=::Players.Get(Owner)->ColorDw;

	// Shape & face
	Shape=Def->Shape;
	SolidMask=Def->SolidMask;
	CheckSolidMaskRect();
	UpdateGraphics(false);
  UpdateFace(true);

  // Initial audibility
  Audible=::GraphicsSystem.GetAudibility(GetX(), GetY(), &AudiblePan);

	// Initial OCF
	SetOCF();

	// local named vars
	LocalNamed.SetNameList(&Def->Script.LocalNamed);

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

void C4Object::AssignRemoval(bool fExitContents)
	{
	// check status
	if (!Status) return;
#ifdef DEBUGREC
	C4RCCreateObj rc;
	rc.id=Def->id;
	rc.oei=Number;
	rc.x=GetX(); rc.y=GetY(); rc.ownr=Owner;
	AddDbgRec(RCT_DsObj, &rc, sizeof(rc));
#endif
	// Destruction call in container
	if (Contained)
		{
		C4AulParSet pars(C4VObj(this));
		Contained->Call(PSF_ContentsDestruction, &pars);
		if (!Status) return;
		}
	// Destruction call
	Call(PSF_Destruction);
	// Destruction-callback might have deleted the object already
	if (!Status) return;
  // remove all effects (extinguishes as well)
  if (pEffects)
		{
		pEffects->ClearAll(this, C4FxCall_RemoveClear);
		// Effect-callback might actually have deleted the object already
		if (!Status) return;
		// ...or just deleted the effects
		if (pEffects)
			{
			delete pEffects;
			pEffects = NULL;
			}
		}
	// remove particles
	if (FrontParticles) FrontParticles.Clear();
	if (BackParticles) BackParticles.Clear();
	// Action idle
	SetAction(0);
	// Object system operation
	if (Status == C4OS_INACTIVE)
		{
		// object was inactive: activate first, then delete
		::Objects.InactiveObjects.Remove(this);
		Status = C4OS_NORMAL;
		::Objects.Add(this);
		}
  Status=0;
	// count decrease
	Def->Count--;
  // Kill contents
  C4Object *cobj; C4ObjectLink *clnk,*next;
  for (clnk=Contents.First; clnk && (cobj=clnk->Obj); clnk=next)
    {
    next=clnk->Next;
		if (fExitContents)
			cobj->Exit(GetX(), GetY());
		else
			{
			Contents.Remove(cobj);
			cobj->AssignRemoval();
			}
    }
	// remove from container *after* contents have been removed!
	C4Object *pCont;
	if (pCont=Contained)
		{
		pCont->Contents.Remove(this);
		pCont->UpdateMass();
		pCont->SetOCF();
		Contained=NULL;
		}
  // Object info
  if (Info) Info->Retire();
	Info = NULL;
	// Object system operation
	C4PropList::AssignRemoval();
  ClearCommands();
	if (pSolidMaskData)
		{
		pSolidMaskData->Remove(true, false);
		delete pSolidMaskData;
		pSolidMaskData = NULL;
		}
	SolidMask.Wdt = 0;
  RemovalDelay=2;
  }

void C4Object::UpdateShape(bool bUpdateVertices)
  {

	// Line shape independent
	if (Def->Line) return;

	// Copy shape from def
	Shape.CopyFrom(Def->Shape, bUpdateVertices, !!fOwnVertices);

  // Construction zoom
  if (Con!=FullCon)
    if (Def->GrowthType)
      Shape.Stretch(Con, bUpdateVertices);
    else
      Shape.Jolt(Con, bUpdateVertices);

  // Rotation
  if (Def->Rotateable)
    if (r!=0)
      Shape.Rotate(r, bUpdateVertices);

	// covered area changed? to be on the save side, update pos
	UpdatePos();
  }

void C4Object::UpdatePos()
	{
	// get new area covered
	// do *NOT* do this while initializing, because object cannot be sorted by main list
	if (!Initializing && Status == C4OS_NORMAL)
		::Objects.UpdatePos(this);
	}

void C4Object::UpdateFace(bool bUpdateShape, bool fTemp)
  {

  // Update shape - NOT for temp call, because temnp calls are done in drawing routine
	// must not change sync relevant data here (although the shape and pos *should* be updated at that time anyway,
	// because a runtime join would desync otherwise)
  if (!fTemp) { if (bUpdateShape) UpdateShape(); else UpdatePos(); }

  // SolidMask
  if (!fTemp) UpdateSolidMask(false);

  // Null defaults
	TopFace.Default();

	// newgfx: TopFace only
  if (Con>=FullCon || Def->GrowthType)
    if (!Def->Rotateable || (r==0))
			if (Def->TopFace.Wdt>0) // Fullcon & no rotation
				TopFace.Set(GetGraphics()->GetBitmap(Color),
					Def->TopFace.x,Def->TopFace.y,
					Def->TopFace.Wdt,Def->TopFace.Hgt);

	// Active face
	UpdateActionFace();
	}

void C4Object::UpdateGraphics(bool fGraphicsChanged, bool fTemp)
	{
	// check color
	if (!fTemp) if (!pGraphics->IsColorByOwner()) Color=0;
	// new grafics: update face + solidmask
	if (fGraphicsChanged)
		{
		// update solid
		if (pSolidMaskData && !fTemp)
			{
			// remove if put
			pSolidMaskData->Remove(true, false);
			// delete
			delete pSolidMaskData; pSolidMaskData = NULL;
			// ensure SolidMask-rect lies within new graphics-rect
			CheckSolidMaskRect();
			}
		// update face - this also puts any SolidMask
		UpdateFace(false);
		}
	}

void C4Object::UpdateFlipDir()
	{
	int32_t iFlipDir;
	// We're active
	if (Action.pActionDef)
		// Get flipdir value from action
		if (iFlipDir = Action.pActionDef->GetPropertyInt(P_FlipDir))
			// Action dir is in flipdir range
			if (Action.Dir >= iFlipDir)
				{
				// Calculate flipped drawing dir (from the flipdir direction going backwards)
				Action.DrawDir = (iFlipDir - 1 - (Action.Dir - iFlipDir));
				// Set draw transform, creating one if necessary
				if (pDrawTransform)
					pDrawTransform->SetFlipDir(-1);
				else
					pDrawTransform = new C4DrawTransform(-1);
				// Done setting flipdir
				return;
				}
	// No flipdir necessary
	Action.DrawDir = Action.Dir;
	// Draw transform present?
	if (pDrawTransform)
		{
		// reset flip dir
		pDrawTransform->SetFlipDir(1);
		// if it's identity now, remove the matrix
		if (pDrawTransform->IsIdentity())
			{
			delete pDrawTransform;
			pDrawTransform=NULL;
			}
		}
	}

void C4Object::DrawFace(C4TargetFacet &cgo, float offX, float offY, int32_t iPhaseX, int32_t iPhaseY)
	{
	const float swdt = float(Def->Shape.Wdt);
	const float shgt = float(Def->Shape.Hgt);
 	// Grow Type Display
 	float fx = float(swdt * iPhaseX);
 	float fy = float(shgt * iPhaseY);
	float fwdt = float(swdt);
	float fhgt = float(shgt);

	float stretch_factor = static_cast<float>(Con) / FullCon;
	float tx = offX + Def->Shape.GetX() * stretch_factor;
	float ty = offY + Def->Shape.GetY() * stretch_factor;
	float twdt = swdt * stretch_factor;
	float thgt = shgt * stretch_factor;

	// Construction Type Display
	if (!Def->GrowthType)
		{
		tx = offX + Def->Shape.GetX();
		twdt = swdt;

		fy += fhgt - thgt;
		fhgt = thgt;
		}

	// Straight
	if ((!Def->Rotateable || (r==0)) && !pDrawTransform)
		{
		lpDDraw->Blit(GetGraphics()->GetBitmap(Color),
			fx, fy, fwdt, fhgt,
			cgo.Surface, tx, ty, twdt, thgt,
			true, NULL);
		}
	// Rotated or transformed
	else
		{
		C4DrawTransform rot;
		if (pDrawTransform)
			{
			rot.SetTransformAt(*pDrawTransform, offX, offY);
			if (r) rot.Rotate(r * 100, offX, offY);
			}
		else
			{
			rot.SetRotate(r * 100, offX, offY);
			}
		lpDDraw->Blit(GetGraphics()->GetBitmap(Color),
			fx, fy, fwdt, fhgt,
			cgo.Surface, tx, ty, twdt, thgt,
			true, &rot);
		}
	}

void C4Object::DrawActionFace(C4TargetFacet &cgo, float offX, float offY)
	{
	// Regular action facet
	const float swdt = float(Action.Facet.Wdt);
	const float shgt = float(Action.Facet.Hgt);
	int32_t iPhase = Action.Phase;
	if (Action.pActionDef->GetPropertyInt(P_Reverse)) iPhase = Action.pActionDef->GetPropertyInt(P_Length) - 1 - Action.Phase;

	// Grow Type Display
	float fx = float(Action.Facet.X + swdt * iPhase);
	float fy = float(Action.Facet.Y + shgt * Action.DrawDir);
	float fwdt = float(swdt);
	float fhgt = float(shgt);

	// draw stretched towards shape center with transform
	float stretch_factor = static_cast<float>(Con) / FullCon;
	float tx = (Def->Shape.GetX() + Action.FacetX) * stretch_factor + offX;
	float ty = (Def->Shape.GetY() + Action.FacetY) * stretch_factor + offY;
	float twdt = swdt * stretch_factor;
	float thgt = shgt * stretch_factor;

	// Construction Type Display
	if (!Def->GrowthType)
		{
		// FIXME
		if (Con != FullCon)
			{
			// incomplete constructions do not show actions
			DrawFace(cgo, offX, offY);
			return;
			}
		tx = Def->Shape.GetX() + Action.FacetX + offX;
		twdt = swdt;
		float offset_from_top = shgt * Max<float>(float(FullCon - Con), 0) / FullCon;
		fy += offset_from_top;
		fhgt -= offset_from_top;
		}

	// Straight
	if ((!Def->Rotateable || (r==0)) && !pDrawTransform)
		{
		lpDDraw->Blit(Action.Facet.Surface,
			fx, fy, fwdt, fhgt,
			cgo.Surface, tx, ty, twdt, thgt,
			true, NULL);
		}
	// Rotated or transformed
	else
		{
		// rotate midpoint of action facet around center of shape
		// combine with existing transform if necessary
		C4DrawTransform rot;
		if (pDrawTransform)
			{
			rot.SetTransformAt(*pDrawTransform, offX, offY);
			if (r) rot.Rotate(r * 100, offX, offY);
			}
		else
			{
			rot.SetRotate(r * 100, offX, offY);
			}
		lpDDraw->Blit(Action.Facet.Surface,
			fx, fy, fwdt, fhgt,
			cgo.Surface, tx, ty, twdt, thgt,
			true, &rot);
		}
	}

void C4Object::UpdateMass()
  {
  Mass=Max<int32_t>((Def->Mass+OwnMass)*Con/FullCon,1);
  if (!Def->NoComponentMass) Mass+=Contents.Mass;
  if (Contained)
    {
    Contained->Contents.MassCount();
    Contained->UpdateMass();
    }
  }

void C4Object::ComponentConCutoff()
  {
	// this is not ideal, since it does not know about custom builder components
  int32_t cnt;
  for (cnt=0; Component.GetID(cnt); cnt++)
    Component.SetCount(cnt,
      Min<int32_t>(Component.GetCount(cnt),Def->Component.GetCount(cnt)*Con/FullCon));
  }

void C4Object::ComponentConGain()
  {
	// this is not ideal, since it does not know about custom builder components
  int32_t cnt;
  for (cnt=0; Component.GetID(cnt); cnt++)
    Component.SetCount(cnt,
      Max<int32_t>(Component.GetCount(cnt),Def->Component.GetCount(cnt)*Con/FullCon));
  }

void C4Object::SetOCF()
  {
#ifdef DEBUGREC_OCF
	uint32_t dwOCFOld = OCF;
#endif
	// Update the object character flag according to the object's current situation
  FIXED cspeed=GetSpeed();
#ifdef _DEBUG
	if(Contained && !::Objects.ObjectNumber(Contained))
		{ LogF("Warning: contained in wild object %p!", Contained); }
	else if(Contained && !Contained->Status)
		{ LogF("Warning: contained in deleted object %p (%s)!", Contained, Contained->GetName()); }
#endif
	if(Contained)
		InMat = Contained->Def->ClosedContainer ? MNone : Contained->InMat;
	else
		InMat = GBackMat(GetX(), GetY());
	// OCF_Normal: The OCF is never zero
  OCF=OCF_Normal;
  // OCF_Construct: Can be built outside
  if (Def->Constructable && (Con<FullCon)
    && (r==0) && !OnFire)
      OCF|=OCF_Construct;
  // OCF_Grab: Can be pushed
  if (Def->Grab && !(Category & C4D_StaticBack))
      OCF|=OCF_Grab;
  // OCF_Carryable: Can be picked up
  if (GetPropertyInt(P_Collectible))
    OCF|=OCF_Carryable;
  // OCF_OnFire: Is burning
  if (OnFire)
    OCF|=OCF_OnFire;
  // OCF_Inflammable: Is not burning and is inflammable
  if (!OnFire && Def->ContactIncinerate>0)
		// Is not a dead living
    if (!(Category & C4D_Living) || Alive)
      OCF|=OCF_Inflammable;
  // OCF_FullCon: Is fully completed/grown
  if (Con>=FullCon)
    OCF|=OCF_FullCon;
  // OCF_Chop: Can be chopped
	DWORD cocf = OCF_Exclusive;
  if (Def->Chopable)
    if (Category & C4D_StaticBack) // Must be static back: this excludes trees that have already been chopped
			if (!::Objects.AtObject(GetX(), GetY(), cocf)) // Can only be chopped if the center is not blocked by an exclusive object
				OCF|=OCF_Chop;
  // OCF_Rotate: Can be rotated
  if (Def->Rotateable)
		// Don't rotate minimum (invisible) construction sites
    if (Con>100)
      OCF|=OCF_Rotate;
  // OCF_Exclusive: No action through this, no construction in front of this
  if (Def->Exclusive)
    OCF|=OCF_Exclusive;
  // OCF_Entrance: Can currently be entered/activated
  if ((Def->Entrance.Wdt>0) && (Def->Entrance.Hgt>0))
    if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (r <= Def->RotatedEntrance)))
      OCF|=OCF_Entrance;
  // HitSpeeds
  if (cspeed>=HitSpeed1) OCF|=OCF_HitSpeed1;
  if (cspeed>=HitSpeed2) OCF|=OCF_HitSpeed2;
  if (cspeed>=HitSpeed3) OCF|=OCF_HitSpeed3;
  if (cspeed>=HitSpeed4) OCF|=OCF_HitSpeed4;
  // OCF_Collection
  if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
    if ((Def->Collection.Wdt>0) && (Def->Collection.Hgt>0))
      if (!Def->CollectionLimit || (Contents.ObjectCount()<Def->CollectionLimit) )
        if (!Action.pActionDef || (!Action.pActionDef->GetPropertyInt(P_ObjectDisabled)))
          if (NoCollectDelay==0)
            OCF|=OCF_Collection;
  // OCF_Living
	if (Category & C4D_Living)
		{
    OCF|=OCF_Living;
		if (Alive) OCF|=OCF_Alive;
		}
  // OCF_FightReady
  if (OCF & OCF_Alive)
    if (!Action.pActionDef || (!Action.pActionDef->GetPropertyInt(P_ObjectDisabled)))
			if (!Def->NoFight)
				OCF|=OCF_FightReady;
	// OCF_LineConstruct
  if (OCF & OCF_FullCon)
		if (Def->LineConnect)
			OCF|=OCF_LineConstruct;
	// OCF_Prey
	if (Def->Prey)
		if (Alive)
			OCF|=OCF_Prey;
	// OCF_CrewMember
	if (Def->CrewMember)
		if (Alive)
			OCF|=OCF_CrewMember;
	// OCF_AttractLightning
	if (Def->AttractLightning)
	  if (OCF & OCF_FullCon)
			OCF|=OCF_AttractLightning;
	// OCF_NotContained
	if (!Contained)
		OCF|=OCF_NotContained;
	// OCF_Edible
	if (Def->Edible)
		OCF|=OCF_Edible;
	// OCF_InLiquid
	if (InLiquid)
		if (!Contained)
			OCF|=OCF_InLiquid;
	// OCF_InSolid
	if (!Contained)
		if (GBackSolid(GetX(), GetY()))
			OCF|=OCF_InSolid;
	// OCF_InFree
	if (!Contained)
		if (!GBackSemiSolid(GetX(), GetY()-1))
			OCF|=OCF_InFree;
	// OCF_Available
	if (!Contained || (Contained->Def->GrabPutGet & C4D_Grab_Get) || (Contained->OCF & OCF_Entrance))
		if (!GBackSemiSolid(GetX(), GetY()-1) || (!GBackSolid(GetX(), GetY()-1) && !GBackSemiSolid(GetX(), GetY()-8)))
			OCF|=OCF_Available;
	// OCF_PowerConsumer
	if (Def->LineConnect & C4D_Power_Consumer)
		if (OCF & OCF_FullCon)
			OCF|=OCF_PowerConsumer;
	// OCF_PowerSupply
	if ( (Def->LineConnect & C4D_Power_Generator)
		|| ((Def->LineConnect & C4D_Power_Output) && (Energy>0)) )
			if (OCF & OCF_FullCon)
				OCF|=OCF_PowerSupply;
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF|=OCF_Container;
#ifdef DEBUGREC_OCF
	assert(!dwOCFOld || ((dwOCFOld & OCF_Carryable) == (OCF & OCF_Carryable)));
	C4RCOCF rc = { dwOCFOld, OCF, false };
	AddDbgRec(RCT_OCF, &rc, sizeof(rc));
#endif
  }


void C4Object::UpdateOCF()
  {
#ifdef DEBUGREC_OCF
	uint32_t dwOCFOld = OCF;
#endif
	// Update the object character flag according to the object's current situation
  FIXED cspeed=GetSpeed();
#ifdef _DEBUG
	if(Contained && !::Objects.ObjectNumber(Contained))
		{ LogF("Warning: contained in wild object %p!", Contained); }
	else if(Contained && !Contained->Status)
		{ LogF("Warning: contained in deleted object %p (%s)!", Contained, Contained->GetName()); }
#endif
	if(Contained)
		InMat = Contained->Def->ClosedContainer ? MNone : Contained->InMat;
	else
		InMat = GBackMat(GetX(), GetY());
	// Keep the bits that only have to be updated with SetOCF (def, category, con, alive, onfire)
	OCF=OCF & (OCF_Normal | OCF_Exclusive | OCF_Edible | OCF_Grab | OCF_FullCon
	       /*| OCF_Chop - now updated regularly, see below */
					 | OCF_Rotate | OCF_OnFire | OCF_Inflammable | OCF_Living | OCF_Alive
	         | OCF_LineConstruct | OCF_Prey | OCF_CrewMember | OCF_AttractLightning
	         | OCF_PowerConsumer);
  // OCF_Carryable: Can be picked up
  if (GetPropertyInt(P_Collectible))
    OCF|=OCF_Carryable;
  // OCF_Construct: Can be built outside
  if (Def->Constructable && (Con<FullCon)
    && (r==0) && !OnFire)
      OCF|=OCF_Construct;
  // OCF_Entrance: Can currently be entered/activated
  if ((Def->Entrance.Wdt>0) && (Def->Entrance.Hgt>0))
    if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (r <= Def->RotatedEntrance)))
      OCF|=OCF_Entrance;
  // OCF_Chop: Can be chopped
	DWORD cocf = OCF_Exclusive;
  if (Def->Chopable)
    if (Category & C4D_StaticBack) // Must be static back: this excludes trees that have already been chopped
			if (!::Objects.AtObject(GetX(), GetY(), cocf)) // Can only be chopped if the center is not blocked by an exclusive object
				OCF|=OCF_Chop;
  // HitSpeeds
  if (cspeed>=HitSpeed1) OCF|=OCF_HitSpeed1;
  if (cspeed>=HitSpeed2) OCF|=OCF_HitSpeed2;
  if (cspeed>=HitSpeed3) OCF|=OCF_HitSpeed3;
  if (cspeed>=HitSpeed4) OCF|=OCF_HitSpeed4;
  // OCF_Collection
  if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
    if ((Def->Collection.Wdt>0) && (Def->Collection.Hgt>0))
      if (!Def->CollectionLimit || (Contents.ObjectCount()<Def->CollectionLimit) )
        if (!Action.pActionDef || (!Action.pActionDef->GetPropertyInt(P_ObjectDisabled)))
          if (NoCollectDelay==0)
            OCF|=OCF_Collection;
  // OCF_FightReady
  if (OCF & OCF_Alive)
    if (!Action.pActionDef || (!Action.pActionDef->GetPropertyInt(P_ObjectDisabled)))
			if (!Def->NoFight)
				OCF|=OCF_FightReady;
	// OCF_NotContained
	if (!Contained)
		OCF|=OCF_NotContained;
	// OCF_InLiquid
	if (InLiquid)
		if (!Contained)
			OCF|=OCF_InLiquid;
	// OCF_InSolid
	if (!Contained)
		if (GBackSolid(GetX(), GetY()))
			OCF|=OCF_InSolid;
	// OCF_InFree
	if (!Contained)
		if (!GBackSemiSolid(GetX(), GetY()-1))
			OCF|=OCF_InFree;
	// OCF_Available
	if (!Contained || (Contained->Def->GrabPutGet & C4D_Grab_Get) || (Contained->OCF & OCF_Entrance))
		if (!GBackSemiSolid(GetX(), GetY()-1) || (!GBackSolid(GetX(), GetY()-1) && !GBackSemiSolid(GetX(), GetY()-8)))
			OCF|=OCF_Available;
	// OCF_PowerSupply
	if ( (Def->LineConnect & C4D_Power_Generator)
		|| ((Def->LineConnect & C4D_Power_Output) && (Energy>0)) )
			if (OCF & OCF_FullCon)
				OCF|=OCF_PowerSupply;
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF|=OCF_Container;
#ifdef DEBUGREC_OCF
	C4RCOCF rc = { dwOCFOld, OCF, true };
	AddDbgRec(RCT_OCF, &rc, sizeof(rc));
#endif
#ifdef _DEBUG
	DEBUGREC_OFF
	uint32_t updateOCF = OCF;
	SetOCF();
	assert (updateOCF == OCF);
	DEBUGREC_ON
#endif
  }

bool C4Object::ExecFire(int32_t iFireNumber, int32_t iCausedByPlr)
  {
  // Fire Phase
  FirePhase++; if (FirePhase>=MaxFirePhase) FirePhase=0;
  // Decay
	if (!Def->NoBurnDecay)
		DoCon(-100);
  // Damage
  if (!::Game.iTick10) if (!Def->NoBurnDamage) DoDamage(+2,iCausedByPlr,C4FxCall_DmgFire);
  // Energy
  if (!::Game.iTick5) DoEnergy(-1,false,C4FxCall_EngFire, iCausedByPlr);
  // Effects
  int32_t smoke_level=2*Shape.Wdt/3;
	int32_t smoke_rate=Def->SmokeRate;
	if (smoke_rate)
		{
		smoke_rate=50*smoke_level/smoke_rate;
		if (!((Game.FrameCounter+(Number*7))%Max<int32_t>(smoke_rate,3)) || (Abs(xdir)>2))
	    Smoke(GetX(), GetY(),smoke_level);
		}
  // Background Effects
  if (!::Game.iTick5)
    {
    int32_t mat;
    if (MatValid(mat=GBackMat(GetX(), GetY())))
      {
			// Extinguish
      if (::MaterialMap.Map[mat].Extinguisher)
        { Extinguish(iFireNumber); if (GBackLiquid(GetX(), GetY())) StartSoundEffect("Pshshsh",false,100,this); }
			// Inflame
			if (!Random(3))
				::Landscape.Incinerate(GetX(), GetY());
      }
    }

  return true;
  }

bool C4Object::ExecLife()
  {

  // Growth
  if (!::Game.iTick35)
		// Growth specified by definition
    if (Def->Growth)
			// Alive livings && trees only
			if ( ((Category & C4D_Living) && Alive)
				|| (Category & C4D_StaticBack) )
					// Not burning
					if (!OnFire)
						// Not complete yet
						if (Con<FullCon)
							// Grow
							DoCon(Def->Growth*100);

  // Magic reload
  int32_t transfer;
  if (!::Game.iTick3) if (Alive)
    if (Contained)
      if (!Hostile(Owner,Contained->Owner))
        if (MagicEnergy<GetPhysical()->Magic)
          {
          transfer=Min<int32_t>(Min<int32_t>(2*MagicPhysicalFactor,Contained->MagicEnergy),GetPhysical()->Magic-MagicEnergy) / MagicPhysicalFactor;
          if (transfer)
            {
						// do energy transfer via script, so it can be overloaded by No-Magic-Energy-rule
						// always use global func instead of local to save double search
						C4AulFunc *pMagicEnergyFn = ::ScriptEngine.GetFuncRecursive(PSF_DoMagicEnergy);
						if (pMagicEnergyFn) // should always be true
							{
							C4AulParSet pars(C4VInt(-transfer), C4VObj(Contained));
							if (!!pMagicEnergyFn->Exec(NULL, &pars))
								{
								C4AulParSet pars(C4VInt(+transfer), C4VObj(this));
								pMagicEnergyFn->Exec(NULL, &pars);
								}
							}
            }
          }

  // Breathing
  if (!::Game.iTick5)
    if (Alive && !Def->NoBreath)
			{
			// Supply check
			bool Breathe=false;
			// Forcefields are breathable.
			if (GBackMat(GetX(), GetY()+Shape.GetY()/2)==MVehic)
				{ Breathe=true; }
			else if (GetPhysical()->BreatheWater)
				{ if (GBackMat(GetX(), GetY())==MWater) Breathe=true; }
			else
				{ if (!GBackSemiSolid(GetX(), GetY()+Shape.GetY()/2)) Breathe=true; }
			if (Contained) Breathe=true;
      // No supply
      if (!Breathe)
        {
				// Reduce breath, then energy, bubble
        if (Breath>0) Breath=Max(Breath-2*C4MaxPhysical/100,0);
        else DoEnergy(-1,false,C4FxCall_EngAsphyxiation, NO_OWNER);
        BubbleOut(GetX() + Random(5) - 2, GetY() + Shape.GetY() / 2);
				ViewEnergy = C4ViewDelay;
				// Physical training
				TrainPhysical(&C4PhysicalInfo::Breath, 2, C4MaxPhysical);
				}
      // Supply
      else
        {
				// Take breath
        int32_t takebreath=GetPhysical()->Breath-Breath;
        if (takebreath>GetPhysical()->Breath/2)
          Call(PSF_DeepBreath);
        Breath+=takebreath;
        }
			}

	// Corrosion energy loss
  if (!::Game.iTick10)
    if (Alive)
			if (InMat!=MNone)
				if (::MaterialMap.Map[InMat].Corrosive)
					if (!GetPhysical()->CorrosionResist)
						DoEnergy(-::MaterialMap.Map[InMat].Corrosive/15,false,C4FxCall_EngCorrosion, NO_OWNER);

	// InMat incineration
  if (!::Game.iTick10)
		if (InMat!=MNone)
			if (::MaterialMap.Map[InMat].Incindiary)
				if (Def->ContactIncinerate)
					Incinerate(NO_OWNER);

  // Nonlife normal energy loss
	if (!::Game.iTick10) if (Energy)
		if (!(Category & C4D_Living))
			// don't loose if assigned as Energy-holder
			if (!(Def->LineConnect & C4D_EnergyHolder))
				DoEnergy(-1,false,C4FxCall_EngStruct, NO_OWNER);

	// birthday
	if (!::Game.iTick255)
		if (Alive)
			if(Info)
			{
				int32_t iPlayingTime = Info->TotalPlayingTime + (Game.Time - Info->InActionTime);

				int32_t iNewAge = iPlayingTime / 3600 / 5;

				if(Info->Age != iNewAge && Info->Age)
				{
					// message
					GameMsgObject(FormatString(LoadResStr("IDS_OBJ_BIRTHDAY"),GetName (), Info->TotalPlayingTime / 3600 / 5).getData(),this);
					StartSoundEffect("Trumpet",false,100,this);
				}

				Info->Age = iNewAge;


			}

  return true;
  }

void C4Object::ExecBase()
  {
  // Environmental action
  if (!::Game.iTick35)
    {
    // Structures dig free snow
    if ((Category & C4D_Structure) && !(Game.Rules & C4RULE_StructuresSnowIn))
      if (r==0)
				{
				::Landscape.DigFreeMat(GetX() + Shape.GetX(), GetY() + Shape.GetY(), Shape.Wdt, Shape.Hgt, MSnow);
				::Landscape.DigFreeMat(GetX() + Shape.GetX(), GetY() + Shape.GetY(), Shape.Wdt, Shape.Hgt, MFlyAshes);
				}
    }

  }

void C4Object::Execute()
	{
#ifdef DEBUGREC
	// record debug
	C4RCExecObj rc;
	rc.Number=Number;
	rc.id=Def->id;
	rc.fx=fix_x;
	rc.fy=fix_y;
	rc.fr=fix_r;
	AddDbgRec(RCT_ExecObj, &rc, sizeof(rc));
#endif
	// reset temporary marker
	Marker = 0;
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
	if(!Status) return;
	// particles
	if (BackParticles) BackParticles.Exec(this);
	if (FrontParticles) FrontParticles.Exec(this);
	// effects
	if (pEffects)
		{
		pEffects->Execute(this);
		if (!Status) return;
		}
	// Life
	ExecLife();
	// Base
	ExecBase();
	// Timer
	Timer++;
	if (Timer>=Def->Timer)
		{
		Timer=0;
		// TimerCall
		if (Def->TimerCall) Def->TimerCall->Exec(this);
		}
	// Menu
	if (Menu) Menu->Execute();
	// View delays
	if (ViewEnergy>0) ViewEnergy--;
	}

bool C4Object::At(int32_t ctx, int32_t cty)
	{
	if (Status) if (!Contained) if (Def)
		if (Inside<int32_t>(cty - (GetY() + Shape.GetY() - addtop()), 0, Shape.Hgt - 1 + addtop()))
			if (Inside<int32_t>(ctx - (GetX() + Shape.GetX()), 0, Shape.Wdt - 1))
				return true;
	return false;
	}

bool C4Object::At(int32_t ctx, int32_t cty, DWORD &ocf)
	{
	if (Status) if (!Contained) if (Def)
		if (OCF & ocf)
			if (Inside<int32_t>(cty - (GetY() + Shape.GetY() - addtop()), 0, Shape.Hgt - 1 + addtop()))
				if (Inside<int32_t>(ctx - (GetX() + Shape.GetX()), 0, Shape.Wdt - 1))
					{
					// Set ocf return value
					GetOCFForPos(ctx, cty, ocf);
					return true;
					}
	return false;
	}

void C4Object::GetOCFForPos(int32_t ctx, int32_t cty, DWORD &ocf)
	{
	DWORD rocf=OCF;
	// Verify entrance area OCF return
	if (rocf & OCF_Entrance)
		if (!Inside<int32_t>(cty - (GetY() + Def->Entrance.y), 0, Def->Entrance.Hgt - 1)
			|| !Inside<int32_t>(ctx - (GetX() + Def->Entrance.x), 0, Def->Entrance.Wdt - 1))
				rocf &= (~OCF_Entrance);
	// Verify collection area OCF return
	if (rocf & OCF_Collection)
		if (!Inside<int32_t>(cty - (GetY() + Def->Collection.y), 0, Def->Collection.Hgt - 1)
			|| !Inside<int32_t>(ctx - (GetX() + Def->Collection.x), 0, Def->Collection.Wdt - 1))
				rocf &= (~OCF_Collection);
	ocf=rocf;
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
	Alive=0;
	if (pEffects) pEffects->ClearAll(this, C4FxCall_RemoveDeath);
	// if the object is alive again, abort here if the kill is not forced
	if (Alive && !fForced) return;
	// Action
  SetActionByName("Dead");
	// Values
  Select=0;
	Alive=0;
  ClearCommands();
	if (Info)
		{
		Info->HasDied=true;
		++Info->DeathCount;
		Info->Retire();
		}
  // Lose contents
  while (thing=Contents.GetObject()) thing->Exit(thing->GetX(),thing->GetY());
  // Remove from crew/cursor/view
	C4Player *pPlr = ::Players.Get(Owner);
  if (pPlr) pPlr->ClearPointers(this, true);
	// ensure objects that won't be affected by dead-plrview-decay are handled properly
	if (!pPlr || !(Category & C4D_Living) || !pPlr->FoWViewObjs.IsContained(this))
		SetPlrViewRange(0);
	// Engine script call
	C4AulParSet pars(C4VInt(iDeathCausingPlayer));
  Call(PSF_Death, &pars);
	// Update OCF. Done here because previously it would have been done in the next frame
	// Whats worse: Having the OCF change because of some unrelated script-call like
	// SetCategory, or slightly breaking compatibility?
	SetOCF();
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
	SetAction(0);
	Action.pActionDef = 0; // Enforce ActIdle because SetAction may have failed due to NoOtherAction
	SetDir(0); // will drop any outdated flipdir
  if (pSolidMaskData) { pSolidMaskData->Remove(true, false); delete pSolidMaskData; pSolidMaskData=NULL; }
	Def->Count--;
  // Def change
  Def=pDef;
	prototype = pDef;
	id=pDef->id;
	Def->Count++;
	LocalNamed.SetNameList(&pDef->Script.LocalNamed);
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
	if (!Def->Rotateable) { r=0; fix_r=rdir=Fix0; }
	// Reset solid mask
	SolidMask=Def->SolidMask;
	// Post change updates
	UpdateGraphics(true);
	UpdateMass();
	UpdateFace(true);
	SetOCF();
	// Any effect callbacks to this object might need to reinitialize their target functions
	// This is ugly, because every effect there is must be updated...
	if (Game.pGlobalEffects) Game.pGlobalEffects->OnObjectChangedDef(this);
	for (C4ObjectLink *pLnk = ::Objects.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->pEffects) pLnk->Obj->pEffects->OnObjectChangedDef(this);
	// Containment (no Entrance)
	if (pContainer) Enter(pContainer,false);
	// Done
  return true;
  }

bool C4Object::Incinerate(int32_t iCausedBy, bool fBlasted, C4Object *pIncineratingObject)
	{
	// Already on fire
	if (OnFire) return false;
	// Dead living don't burn
	if ((Category & C4D_Living) && !Alive) return false;
	// add effect
	int32_t iEffNumber;
	C4Value Par1 = C4VInt(iCausedBy), Par2 = C4VBool(!!fBlasted), Par3 = C4VObj(pIncineratingObject), Par4;
	new C4Effect(this, C4Fx_Fire, C4Fx_FirePriority, C4Fx_FireTimer, NULL, 0, Par1, Par2, Par3, Par4, true, iEffNumber);
	return !!iEffNumber;
	}

bool C4Object::Extinguish(int32_t iFireNumber)
  {
	// any effects?
  if (!pEffects) return false;
	// fire number known: extinguish that fire
	C4Effect *pEffFire;
	if (iFireNumber)
		{
		pEffFire = pEffects->Get(iFireNumber, false);
		if (!pEffFire) return false;
		pEffFire->Kill(this);
		}
	else
		{
		// otherwise, kill all fires
    // (keep checking from beginning of pEffects, as Kill might delete or change effects)
		int32_t iFiresKilled = 0;
		while (pEffects && (pEffFire = pEffects->Get(C4Fx_AnyFire)))
			{
			while (pEffFire && WildcardMatch(C4Fx_Internal, pEffFire->Name))
				{
				pEffFire = pEffFire->pNext;
				if (pEffFire) pEffFire = pEffFire->Get(C4Fx_AnyFire);
				}
			if (!pEffFire) break;
			pEffFire->Kill(this);
			++iFiresKilled;
			}
		if (!iFiresKilled) return false;
		}
	// done, success
  return true;
  }

void C4Object::DoDamage(int32_t iChange, int32_t iCausedBy, int32_t iCause)
  {
	// non-living: ask effects first
	if (pEffects && !Alive)
		{
		pEffects->DoDamage(this, iChange, iCause, iCausedBy);
		if (!iChange) return;
		}
	// Change value
  Damage = Max<int32_t>( Damage+iChange, 0 );
	// Engine script call
  Call(PSF_Damage,&C4AulParSet(C4VInt(iChange), C4VInt(iCausedBy)));
  }

void C4Object::DoEnergy(int32_t iChange, bool fExact, int32_t iCause, int32_t iCausedByPlr)
  {
	// iChange 100% = Physical 100000
  if (!fExact) iChange=iChange*C4MaxPhysical/100;
	// Was zero?
  bool fWasZero=(Energy==0);
	// Mark last damage causing player to trace kills
	if (iChange < 0) UpdatLastEnergyLossCause(iCausedByPlr);
	// Living things: ask effects for change first
	if (pEffects && Alive)
		{
		pEffects->DoDamage(this, iChange, iCause, iCausedByPlr);
		if (!iChange) return;
		}
	// Do change
  Energy=BoundBy<int32_t>(Energy+iChange,0,GetPhysical()->Energy);
	// Alive and energy reduced to zero: death
  if (Alive) if (Energy==0) if (!fWasZero) AssignDeath(false);
	// View change
	ViewEnergy = C4ViewDelay;
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
	// iChange 100% = Physical 100000
  iChange=iChange*C4MaxPhysical/100;
	// Do change
  Breath=BoundBy<int32_t>(Breath+iChange,0,GetPhysical()->Breath);
	// View change
	ViewEnergy = C4ViewDelay;
  }

void C4Object::Blast(int32_t iLevel, int32_t iCausedBy)
  {
	// Damage
  DoDamage(iLevel,iCausedBy,C4FxCall_DmgBlast);
	// Energy (alive objects)
  if (Alive) DoEnergy(-iLevel/3,false,C4FxCall_EngBlast, iCausedBy);
  // Incinerate
  if (Def->BlastIncinerate)
    if (Damage>=Def->BlastIncinerate)
      Incinerate(iCausedBy,true);
  }

void C4Object::DoCon(int32_t iChange, bool fInitial, bool fNoComponentChange)
	{
	int32_t iStepSize=FullCon/100;
	int32_t lRHgt=Shape.Hgt,lRy=Shape.GetY();
	int32_t iLastStep=Con/iStepSize;
	FIXED strgt_con_b = fix_y + Shape.GetY() + Shape.Hgt;
	bool fWasFull = (Con>=FullCon);

	// Change con
	if (Def->Oversize)
		Con=Max<int32_t>(Con+iChange,0);
	else
		Con=BoundBy<int32_t>(Con+iChange,0,FullCon);
	int32_t iStepDiff = Con/iStepSize - iLastStep;

	// Update OCF
	SetOCF();

  // If step changed or limit reached or degraded from full: update mass, face, components, etc.
  if ( iStepDiff || (Con>=FullCon) || (Con==0) || (fWasFull && (Con<FullCon)) )
    {
    // Mass
    UpdateMass();
    // Decay from full remove mask before face is changed
    if (fWasFull && (Con<FullCon))
      if (pSolidMaskData) pSolidMaskData->Remove(true, false);
    // Face
    UpdateFace(true);
		// component update
		if (!fNoComponentChange)
			{
			// Decay: reduce components
			if (iChange<0)
	      ComponentConCutoff();
			// Growth: gain components
			else
	      ComponentConGain();
			}
    // Unfullcon
    if (Con<FullCon)
			{
			// Lose contents
			if (!Def->IncompleteActivity)
				{
				C4Object *cobj;
				while (cobj=Contents.GetObject())
					if (Contained) cobj->Enter(Contained);
					else cobj->Exit(cobj->GetX(),cobj->GetY());
				}
			// No energy need
			NeedEnergy=0;
			}
		// Decay from full stop action
		if (fWasFull && (Con<FullCon))
			if (!Def->IncompleteActivity)
				SetAction(0);
		}
	else
		// set first position
		if (fInitial) UpdatePos();

	// Straight Con bottom y-adjust
	if (!r || fInitial)
		{
		if ((Shape.Hgt!=lRHgt) || (Shape.GetY()!=lRy))
			{
			fix_y = strgt_con_b - Shape.Hgt - Shape.GetY();
			UpdatePos(); UpdateSolidMask(false);
			}
		}
	else if (Category & C4D_Structure) if (iStepDiff > 0)
		{
		// even rotated buildings need to be moved upwards
		// but by con difference, because with keep-bottom-method, they might still be sinking
		// besides, moving the building up may often stabilize it
		fix_y -= ((iLastStep+iStepDiff) * Def->Shape.Hgt / 100) - (iLastStep * Def->Shape.Hgt / 100);
		UpdatePos(); UpdateSolidMask(false);
		}
	// Completion (after bottom GetY()-adjust for correct position)
	if (!fWasFull && (Con>=FullCon))
		{
		Call(PSF_Completion);
		Call(PSF_Initialize);
		}

	// Con Zero Removal
	if (Con<=0)
		AssignRemoval();

	}

void C4Object::DoExperience(int32_t change)
  {
  const int32_t MaxExperience = 100000000;

  if (!Info) return;

  Info->Experience=BoundBy<int32_t>(Info->Experience+change,0,MaxExperience);

  // Promotion check
	if (Info->Experience<MaxExperience)
		if (Info->Experience>=::DefaultRanks.Experience(Info->Rank+1))
			Promote(Info->Rank+1, false, false);
  }

bool C4Object::Exit(int32_t iX, int32_t iY, int32_t iR, FIXED iXDir, FIXED iYDir, FIXED iRDir, bool fCalls)
  {
	// 1. Exit the current container.
	// 2. Update Contents of container object and set Contained to NULL.
	// 3. Set offset position/motion if desired.
	// 4. Call Ejection for container and Departure for object.

	// Not contained
	C4Object *pContainer=Contained;
  if (!pContainer) return false;
  // Remove object from container
  pContainer->Contents.Remove(this);
  pContainer->UpdateMass();
  pContainer->SetOCF();
  // No container
  Contained=NULL;
	// Position/motion
	BoundsCheck(iX, iY);
	r=iR;
	fix_x=itofix(iX); fix_y=itofix(iY); fix_r=itofix(r);
	xdir=iXDir; ydir=iYDir; rdir=iRDir;
	// Misc updates
  Mobile=1;
  InLiquid=0;
  CloseMenu(true);
  UpdateFace(true);
  SetOCF();
	// Engine calls
	if (fCalls) pContainer->Call(PSF_Ejection,&C4AulParSet(C4VObj(this)));
	if (fCalls) Call(PSF_Departure,&C4AulParSet(C4VObj(pContainer)));
	// Success (if the obj wasn't "re-entered" by script)
  return !Contained;
  }

bool C4Object::Enter(C4Object *pTarget, bool fCalls, bool fCopyMotion, bool *pfRejectCollect)
	{
	// 0. Query entrance and collection
	// 1. Exit if contained.
	// 2. Set new container.
	// 3. Update Contents and mass of the new container.
	// 4. Call collection for container
	// 5. Call entrance for object.

	// No target or target is self
	if (!pTarget || (pTarget==this)) return false;
	// check if entrance is allowed
	if (!! Call(PSF_RejectEntrance, &C4AulParSet(C4VObj(pTarget)))) return false;
	// check if we end up in an endless container-recursion
	for (C4Object *pCnt=pTarget->Contained; pCnt; pCnt=pCnt->Contained)
		if (pCnt==this) return false;
	// Check RejectCollect, if desired
	if (pfRejectCollect)
		{
		if (!!pTarget->Call(PSF_RejectCollection,&C4AulParSet(C4VID(Def->id), C4VObj(this))))
			{
			*pfRejectCollect = true;
			return false;
			}
		*pfRejectCollect = false;
		}
	// Exit if contained
	if (Contained) if (!Exit(GetX(),GetY())) return false;
	if (Contained || !Status || !pTarget->Status) return false;
	// Failsafe updates
	CloseMenu(true);
	SetOCF();
	// Set container
	Contained=pTarget;
	// Enter
	if (!Contained->Contents.Add(this, C4ObjectList::stContents))
		{
		Contained=NULL;
		return false;
		}
	// Assume that the new container controls this object, if it cannot control itself (i.e.: Alive)
	// So it can be traced back who caused the damage, if a projectile hits its target
	if (!(Alive && (Category & C4D_Living)))
		Controller = pTarget->Controller;
	// Misc updates
	// motion must be copied immediately, so the position will be correct when OCF is set, and
	// OCF_Available will be set for newly bought items, even if 50/50 is solid in the landscape
	// however, the motion must be preserved sometimes to keep flags like OCF_HitSpeed upon collection
	if (fCopyMotion)
		{
		// remove any solidmask before copying the motion...
		UpdateSolidMask(false);
		CopyMotion(Contained);
		}
	SetOCF();
  UpdateFace(true);
  // Update container
  Contained->UpdateMass();
  Contained->SetOCF();
	// Collection call
	if (fCalls) pTarget->Call(PSF_Collection2,&C4AulParSet(C4VObj(this)));
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Entrance call
	if (fCalls) Call(PSF_Entrance,&C4AulParSet(C4VObj(Contained)));
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Success
  return true;
  }

void C4Object::Fling(FIXED txdir, FIXED tydir, bool fAddSpeed)
  {
	if (fAddSpeed) { txdir+=xdir/2; tydir+=ydir/2; }
  if (!ObjectActionTumble(this,(txdir<0),txdir,tydir))
    if (!ObjectActionJump(this,txdir,tydir,false))
			{
			xdir=txdir; ydir=tydir;
			Mobile=1;
			Action.t_attach&=~CNAT_Bottom;
			}
  }

bool C4Object::ActivateEntrance(int32_t by_plr, C4Object *by_obj)
  {
  // Try entrance activation
  if (OCF & OCF_Entrance)
    if (!! Call(PSF_ActivateEntrance,&C4AulParSet(C4VObj(by_obj))))
      return true;
	// Failure
  return false;
  }

void C4Object::Explode(int32_t iLevel, C4ID idEffect, const char *szEffect)
  {
	// Called by FnExplode only
  C4Object *pContainer = Contained;
	int32_t iCausedBy = Controller;
  AssignRemoval();
  Explosion(GetX(),GetY(),iLevel,pContainer,iCausedBy,this, idEffect,szEffect);
  }

bool C4Object::Build(int32_t iLevel, C4Object *pBuilder)
  {
	int32_t cnt;
	C4ID NeededMaterial;
	int32_t NeededMaterialCount = 0;
	C4Object *pMaterial;

  // Invalid or complete: no build
  if (!Status || !Def || (Con>=FullCon)) return false;

  // Material check (if rule set or any other than structure or castle-part)
	bool fNeedMaterial = (Game.Rules & C4RULE_ConstructionNeedsMaterial) || !(Category & (C4D_Structure|C4D_StaticBack));
  if (fNeedMaterial)
    {
		// Determine needed components (may be overloaded)
		C4IDList NeededComponents;
		Def->GetComponents(&NeededComponents, NULL, pBuilder);

    // Grab any needed components from builder
		C4ID idMat;
    for (cnt=0; idMat=NeededComponents.GetID(cnt); cnt++)
      if (Component.GetIDCount(idMat)<NeededComponents.GetCount(cnt))
        if ((pMaterial=pBuilder->Contents.Find(idMat)))
          if (!pMaterial->OnFire) if (pMaterial->OCF & OCF_FullCon)
            {
            Component.SetIDCount(idMat,Component.GetIDCount(idMat)+1, true);
            pBuilder->Contents.Remove(pMaterial);
            pMaterial->AssignRemoval();
            }
		// Grab any needed components from container
		if (Contained)
			for (cnt=0; idMat=NeededComponents.GetID(cnt); cnt++)
	      if (Component.GetIDCount(idMat)<NeededComponents.GetCount(cnt))
					 if ((pMaterial=Contained->Contents.Find(idMat)))
						 if (!pMaterial->OnFire) if (pMaterial->OCF & OCF_FullCon)
							 {
							 Component.SetIDCount(idMat,Component.GetIDCount(idMat)+1, true);
							 Contained->Contents.Remove(pMaterial);
							 pMaterial->AssignRemoval();
							 }
    // Check for needed components at current con
		for (cnt=0; idMat=NeededComponents.GetID(cnt); cnt++)
			if (NeededComponents.GetCount(cnt)!=0)
				if ( (100*Component.GetIDCount(idMat)/NeededComponents.GetCount(cnt)) < (100*Con/FullCon) )
					{
					NeededMaterial = NeededComponents.GetID(cnt);
					NeededMaterialCount = NeededComponents.GetCount(cnt)-Component.GetCount(cnt);
					break;
					}
		}

	// Needs components
	if (NeededMaterialCount)
		{
		// BuildNeedsMaterial call to builder script...
		if (!pBuilder->Call(PSF_BuildNeedsMaterial,
				&C4AulParSet(C4VID(NeededMaterial),C4VInt(NeededMaterialCount))))
			{
			// Builder is a crew member...
			if (pBuilder->OCF & OCF_CrewMember)
				// ...tell builder to acquire the material
				pBuilder->AddCommand(C4CMD_Acquire,NULL,0,0,50,NULL,true,C4VID(NeededMaterial),false,1);
			// ...game message if not overloaded
			::Messages.New(C4GM_Target,GetNeededMatStr(pBuilder),pBuilder,pBuilder->Controller);
			}
		// Still in need: done/fail
		return false;
    }

	// Do con (mass- and builder-relative)
	int32_t iBuildSpeed=100; C4PhysicalInfo *pPhys;
	if (pBuilder) if (pPhys=pBuilder->GetPhysical())
		{
		iBuildSpeed=pPhys->CanConstruct;
		if (!iBuildSpeed)
			{
			// shouldn't even have gotten here. Looks like the Clonk lost the ability to build recently
			return false;
			}
		if (iBuildSpeed<=1) iBuildSpeed=100;
		}
	DoCon(iLevel*iBuildSpeed*150/Def->Mass, false, fNeedMaterial);

	// TurnTo
	if (Def->BuildTurnTo!=C4ID_None)
		ChangeDef(Def->BuildTurnTo);

	// Repair
	Damage=0;

	// Done/success
	return true;
	}

bool C4Object::Chop(C4Object *pByObject)
  {
  // Valid check
  if (!Status || !Def || Contained || !(OCF & OCF_Chop))
    return false;
  // Chop
  if (!::Game.iTick10) DoDamage( +10, pByObject ? pByObject->Owner : NO_OWNER, C4FxCall_DmgChop);
  return true;
  }

bool C4Object::Push(FIXED txdir, FIXED dforce, bool fStraighten)
  {
  // Valid check
  if (!Status || !Def || Contained || !(OCF & OCF_Grab)) return false;
  // Grabbing okay, no pushing
  if (Def->Grab==2) return true;
  // Mobilization check (pre-mobilization zero)
  if (!Mobile)
    { xdir=ydir=Fix0; }
  // General pushing force vs. object mass
  dforce=dforce*100/Mass;
  // Set dir
  if (xdir<0) SetDir(DIR_Left);
  if (xdir>0) SetDir(DIR_Right);
  // Work towards txdir
  if (Abs(xdir-txdir)<=dforce) // Close-enough-set
    { xdir=txdir; }
  else // Work towards
    {
    if (xdir<txdir) xdir+=dforce;
    if (xdir>txdir) xdir-=dforce;
    }
  // Straighten
  if (fStraighten)
    if (Inside<int32_t>(r,-StableRange,+StableRange))
      {
      rdir=0; // cheap way out
      }
    else
      {
      if (r>0) { if (rdir>-RotateAccel) rdir-=dforce; }
      else { if (rdir<+RotateAccel) rdir+=dforce; }
      }

  // Mobilization check
  if (!!xdir || !!ydir || !!rdir) Mobile=1;

  // Stuck check
  if (!::Game.iTick35) if (txdir) if (!Def->NoHorizontalMove)
    if (ContactCheck(GetX(), GetY())) // Resets t_contact
      {
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_STUCK"),GetName()).getData(),this);
      Call(PSF_Stuck);
      }

  return true;
  }

bool C4Object::Lift(FIXED tydir, FIXED dforce)
  {
  // Valid check
  if (!Status || !Def || Contained) return false;
  // Mobilization check
  if (!Mobile)
    { xdir=ydir=Fix0; Mobile=1; }
  // General pushing force vs. object mass
  dforce=dforce*100/Mass;
  // If close enough, set tydir
  if (Abs(tydir-ydir)<=Abs(dforce))
    ydir=tydir;
  else // Work towards tydir
    {
    if (ydir<tydir) ydir+=dforce;
    if (ydir>tydir) ydir-=dforce;
    }
  // Stuck check
  if (tydir != -GravAccel)
    if (ContactCheck(GetX(), GetY())) // Resets t_contact
      {
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_STUCK"),GetName()).getData(),this);
      Call(PSF_Stuck);
      }
  return true;
  }

C4Object* C4Object::CreateContents(C4PropList * PropList)
  {
  C4Object *nobj;
  if (!(nobj=Game.CreateObject(PropList,this,Owner))) return NULL;
  if (!nobj->Enter(this)) { nobj->AssignRemoval(); return NULL; }
  return nobj;
  }

bool C4Object::CreateContentsByList(C4IDList &idlist)
  {
  int32_t cnt,cnt2;
  for (cnt=0; idlist.GetID(cnt); cnt++)
    for (cnt2=0; cnt2<idlist.GetCount(cnt); cnt2++)
      if (!CreateContents(C4Id2Def(idlist.GetID(cnt))))
        return false;
  return true;
  }

bool C4Object::ActivateMenu(int32_t iMenu, int32_t iMenuSelect,
														int32_t iMenuData, int32_t iMenuPosition,
														C4Object *pTarget)
  {
	// Variables
	C4FacetSurface fctSymbol;
	char szCaption[256+1],szCommand[256+1];
	int32_t cnt,iCount;
	C4Def *pDef;
	C4Player *pPlayer;
	C4IDList ListItems;
	// Close any other menu
	//CloseMenu(true);
	if (Menu && Menu->IsActive()) if (!Menu->TryClose(true, false)) return false;
	// Create menu
	if (!Menu) Menu = new C4ObjectMenu; else Menu->ClearItems(true);
	// Open menu
	switch (iMenu)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Activate:
			// No target specified: use own container as target
			if (!pTarget) if (!(pTarget=Contained)) break;
			// Opening contents menu blocked by RejectContents
			if (!!pTarget->Call(PSF_RejectContents)) return false;
			// Create symbol
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			pTarget->Def->Draw(fctSymbol,false,pTarget->Color,pTarget);
			sprintf(szCaption,LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName());
			// Init
			Menu->Init(fctSymbol,szCaption,this,C4MN_Extra_None,0,iMenu);
			Menu->SetPermanent(true);
			Menu->SetRefillObject(pTarget);
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Buy:
			// No target specified: container is base
			if (!pTarget) if (!(pTarget=Contained)) break;
			// Create symbol
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			//pTarget->Def->Draw(fctSymbol,false,pTarget->Color,pTarget);
			DrawMenuSymbol(C4MN_Buy, fctSymbol, pTarget->Owner, pTarget);
			// Init menu
			Menu->Init(fctSymbol,LoadResStr("IDS_PLR_NOBUY"),this,C4MN_Extra_Value,0,iMenu);
			Menu->SetPermanent(true);
			Menu->SetRefillObject(pTarget);
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Sell:
			// No target specified: container is base
			if (!pTarget) if (!(pTarget=Contained)) break;
			// Create symbol & init
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			//pTarget->Def->Draw(fctSymbol,false,pTarget->Color,pTarget);
			DrawMenuSymbol(C4MN_Sell, fctSymbol, pTarget->Owner, pTarget);
			sprintf(szCaption,LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName());
			Menu->Init(fctSymbol,szCaption,this,C4MN_Extra_Value,0,iMenu);
			Menu->SetPermanent(true);
			Menu->SetRefillObject(pTarget);
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Get:
		case C4MN_Contents:
			// No target specified
			if (!pTarget) break;
			// Opening contents menu blocked by RejectContents
			if (!!pTarget->Call(PSF_RejectContents)) return false;
			// Create symbol & init
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			pTarget->Def->Draw(fctSymbol,false,pTarget->Color,pTarget);
			sprintf(szCaption,LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName());
			Menu->Init(fctSymbol,szCaption,this,C4MN_Extra_None,0,iMenu);
			Menu->SetPermanent(true);
			Menu->SetRefillObject(pTarget);
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Context:
			{
			// Target by parameter
			if (!pTarget) break;

			// Create symbol & init menu
			pPlayer=::Players.Get(pTarget->Owner);
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			pTarget->Def->Draw(fctSymbol,false,pTarget->Color, pTarget);
			Menu->Init(fctSymbol,pTarget->GetName(),this,C4MN_Extra_None,0,iMenu,C4MN_Style_Context);

			Menu->SetPermanent(iMenuData);
			Menu->SetRefillObject(pTarget);

			// Preselect
			Menu->SetSelection(iMenuSelect, false, true);
			Menu->SetPosition(iMenuPosition);
			}
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Construction:
			// Check valid player
			if (!(pPlayer = ::Players.Get(Owner))) break;
			// Create symbol
			fctSymbol.Create(C4SymbolSize,C4SymbolSize);
			DrawMenuSymbol(C4MN_Construction,fctSymbol,-1,NULL);
			// Init menu
			Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_PLR_NOBKNOW"),pPlayer->GetName()).getData(),
				this,C4MN_Extra_Components,0,iMenu);
			// Add player's structure build knowledge
		  for (cnt=0; pDef=C4Id2Def(pPlayer->Knowledge.GetID(::Definitions,C4D_Structure,cnt,&iCount)); cnt++)
				{
				// Caption
				sprintf(szCaption,LoadResStr("IDS_MENU_CONSTRUCT"),pDef->GetName());
				// Picture
				fctSymbol.Set(pDef->Graphics.GetBitmap(),pDef->PictureRect.x,pDef->PictureRect.y,pDef->PictureRect.Wdt,pDef->PictureRect.Hgt);
				// Command
				sprintf(szCommand,"SetCommand(\"Construct\",nil,0,0,nil,%s)",C4IdText(pDef->id));
				// Add menu item
				Menu->AddRefSym(szCaption,fctSymbol,szCommand,C4MN_Item_NoCount,NULL,pDef->GetDesc(),pDef->id);
				}
			// Preselect
			Menu->SetSelection(iMenuSelect, false, true);
			Menu->SetPosition(iMenuPosition);
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MN_Info:
			// Target by parameter
			if (!pTarget) break;
			pPlayer=::Players.Get(pTarget->Owner);
			// Create symbol & init menu
			fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,0,1);
			Menu->Init(fctSymbol, pTarget->GetName(), this, C4MN_Extra_None, 0, iMenu, C4MN_Style_Info);
			Menu->SetPermanent(true);
			Menu->SetAlignment(C4MN_Align_Free);
			C4Viewport *pViewport = ::GraphicsSystem.GetViewport(Controller); // Hackhackhack!!!
			if (pViewport) Menu->SetLocation((pTarget->GetX() + pTarget->Shape.GetX() + pTarget->Shape.Wdt + 10 - pViewport->ViewX) * pViewport->Zoom,
			                                 (pTarget->GetY() + pTarget->Shape.GetY() - pViewport->ViewY) * pViewport->Zoom);
			// Add info item
			fctSymbol.Create(C4PictureSize, C4PictureSize); pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
			Menu->Add(pTarget->GetName(), fctSymbol, "", C4MN_Item_NoCount, NULL, pTarget->GetInfoString().getData());
			fctSymbol.Default();
			// Success
			return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
	// Invalid menu identification
	CloseMenu(true);
  return false;
  }

bool C4Object::CloseMenu(bool fForce)
  {
	if (Menu)
		{
		if (Menu->IsActive()) if (!Menu->TryClose(fForce, false)) return false;
		if (!Menu->IsCloseQuerying()) { delete Menu; Menu=NULL; } // protect menu deletion from recursive menu operation calls
		}
	return true;
  }

void C4Object::AutoContextMenu(int32_t iMenuSelect)
	{
	// Auto Context Menu - the "new structure menus"
	// No command set and no menu open
	if (!Command && !(Menu && Menu->IsActive()))
		// In a container with AutoContextMenu
		if (Contained && Contained->Def->AutoContextMenu)
			// Crew members only
			if (OCF & OCF_CrewMember)
				{
				// Player has AutoContextMenus enabled
				C4Player* pPlayer = ::Players.Get(Controller);
				if (pPlayer && pPlayer->PrefAutoContextMenu)
					{
					// Open context menu for structure
					ActivateMenu(C4MN_Context, iMenuSelect, 1, 0, Contained);
					// Closing the menu exits the building (all selected clonks)
					Menu->SetCloseCommand("PlayerObjectCommand(GetOwner(), \"Exit\") && ExecuteCommand()");
					}
				}
	}

BYTE C4Object::GetArea(int32_t &aX, int32_t &aY, int32_t &aWdt, int32_t &aHgt)
	{
	if (!Status || !Def) return 0;
	aX = GetX() + Shape.GetX(); aY = GetY() + Shape.GetY();
	aWdt=Shape.Wdt; aHgt=Shape.Hgt;
	return 1;
	}

BYTE C4Object::GetEntranceArea(int32_t &aX, int32_t &aY, int32_t &aWdt, int32_t &aHgt)
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

BYTE C4Object::GetMomentum(FIXED &rxdir, FIXED &rydir)
  {
  rxdir=rydir=0;
  if (!Status || !Def) return 0;
  rxdir=xdir; rydir=ydir;
  return 1;
  }

FIXED C4Object::GetSpeed()
  {
  FIXED cobjspd=Fix0;
  if (xdir<0) cobjspd-=xdir; else cobjspd+=xdir;
  if (ydir<0) cobjspd-=ydir; else cobjspd+=ydir;
  return cobjspd;
  }

void C4Object::SetName(const char * NewName)
	{
	if(!NewName && Info)
		C4PropList::SetName(Info->Name);
	else
		C4PropList::SetName(NewName);
	}

int32_t C4Object::GetValue(C4Object *pInBase, int32_t iForPlayer)
  {
	int32_t iValue;

	// value by script?
	if (C4AulScriptFunc *f = Def->Script.SFn_CalcValue)
		iValue = f->Exec(this, &C4AulParSet(C4VObj(pInBase), C4VInt(iForPlayer))).getInt();
	else
		{
		// get value of def
		// Caution: Do not pass pInBase here, because the def base value is to be queried
		//  - and not the value if you had to buy the object in this particular base
		iValue=Def->GetValue(NULL, iForPlayer);
		}
  // Con percentage
  iValue=iValue*Con/FullCon;
	// value adjustments buy base function
	if (pInBase)
		{
		C4AulFunc *pFn;
		if (pFn = pInBase->Def->Script.GetSFunc(PSF_CalcSellValue, AA_PROTECTED))
			iValue = pFn->Exec(pInBase, &C4AulParSet(C4VObj(this), C4VInt(iValue))).getInt();
		}
	// Return value
  return iValue;
  }

C4PhysicalInfo* C4Object::GetPhysical(bool fPermanent)
  {
	// Temporary physical
	if (PhysicalTemporary && !fPermanent) return &TemporaryPhysical;
	// Info physical: Available only if there's an info and it should be used
  if (Info)
		if (!Game.Parameters.UseFairCrew)
			return &(Info->Physical);
		else if (Info->pDef)
			return Info->pDef->GetFairCrewPhysicals();
		else
			// shouldn't really happen, but who knows.
			// Maybe some time it will be possible to have crew infos that aren't tied to a specific definition
			return Def->GetFairCrewPhysicals();
	// Definition physical
  return &(Def->Physical);
  }

bool C4Object::TrainPhysical(C4PhysicalInfo::Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain)
	{
	int i=0;
	// Train temp
	if (PhysicalTemporary) { TemporaryPhysical.Train(mpiOffset, iTrainBy, iMaxTrain); ++i; }
	// train permanent, if existant
	// this also trains if fair crew is used!
	if (Info) { Info->Physical.Train(mpiOffset, iTrainBy, iMaxTrain); ++i; }
	// return whether anything was trained
	return !!i;
	}

bool C4Object::Promote(int32_t torank, bool exception, bool fForceRankName)
	{
	if (!Info) return false;
	// get rank system
	C4Def *pUseDef = C4Id2Def(Info->id);
	C4RankSystem *pRankSys;
	if (pUseDef && pUseDef->pRankNames)
		pRankSys = pUseDef->pRankNames;
	else
		pRankSys = &::DefaultRanks;
	// always promote info
	Info->Promote(torank,*pRankSys, fForceRankName);
	// silent update?
	if (!pRankSys->GetRankName(torank,false)) return false;
	GameMsgObject(FormatString(LoadResStr("IDS_OBJ_PROMOTION"),GetName (),Info->sRankName.getData()).getData(),this);
	StartSoundEffect("Trumpet",0,100,this);
	return true;
	}

void C4Object::ClearPointers(C4Object *pObj)
  {
	// effects
	if (pEffects) pEffects->ClearPointers(pObj);
	// contents/contained: not necessary, because it's done in AssignRemoval and StatusDeactivate
	// Action targets
  if (Action.Target==pObj) Action.Target=NULL;
  if (Action.Target2==pObj) Action.Target2=NULL;
	// Commands
	C4Command *cCom;
  for (cCom=Command; cCom; cCom=cCom->Next)
		cCom->ClearPointers(pObj);
	// Menu
	if (Menu) Menu->ClearPointers(pObj);
	// Layer
	if (pLayer==pObj) pLayer=NULL;
	// gfx overlays
	if (pGfxOverlay)
		{
		C4GraphicsOverlay *pNextGfxOvrl = pGfxOverlay, *pGfxOvrl;
		while (pGfxOvrl = pNextGfxOvrl)
			{
			pNextGfxOvrl = pGfxOvrl->GetNext();
			if (pGfxOvrl->GetOverlayObject() == pObj)
				// overlay relying on deleted object: Delete!
				RemoveGraphicsOverlay(pGfxOvrl->GetID());
			}
		}
  }

C4Value C4Object::Call(const char *szFunctionCall, C4AulParSet *pPars, bool fPassError)
	{
	if (!Status) return C4VNull;
	assert(Def && szFunctionCall[0]);
	return Def->Script.Call(szFunctionCall, this, pPars, false, fPassError);
	}

bool C4Object::SetPhase(int32_t iPhase)
	{
	if (!Action.pActionDef) return false;
	Action.Phase=BoundBy<int32_t>(iPhase,0,Action.pActionDef->GetPropertyInt(P_Length));
	return true;
	}

void C4Object::Draw(C4TargetFacet &cgo, int32_t iByPlayer, DrawMode eDrawMode)
	{
	C4Facet ccgo;

	// Status
	if (!Status || !Def) return;

	// visible?
	if(!IsVisible(iByPlayer, !!eDrawMode)) return;

	// Line
	if (Def->Line) { DrawLine(cgo);	return;	}

	// background particles (bounds not checked)
	if (BackParticles && !Contained && eDrawMode!=ODM_BaseOnly) BackParticles.Draw(cgo,this);

	// Object output position
	float cotx = cgo.TargetX,coty=cgo.TargetY; if (eDrawMode!=ODM_Overlay) TargetPos(cotx, coty, cgo);
	float cox = fixtof(fix_x) + Shape.GetX() - cotx, coy = fixtof(fix_y) + Shape.GetY() - coty;
	float offX = cgo.X + fixtof(fix_x) - cotx, offY = cgo.Y + fixtof(fix_y) - coty;

	bool fYStretchObject=false;
	if (Action.pActionDef)
		if (Action.pActionDef->GetPropertyInt(P_FacetTargetStretch))
			fYStretchObject=true;

	// Set audibility
	if (!eDrawMode) SetAudibilityAt(cgo, GetX(), GetY());

  // Output boundary
	if (!fYStretchObject && !eDrawMode)
		if (Action.pActionDef && !r && !Action.pActionDef->GetPropertyInt(P_FacetBase) && Con<=FullCon)
			{
			// active
			if ( !Inside<float>(cox+Action.FacetX,1-Action.Facet.Wdt,cgo.Wdt)
				|| (!Inside<float>(coy+Action.FacetY,1-Action.Facet.Hgt,cgo.Hgt)) )
				{ if (FrontParticles && !Contained) FrontParticles.Draw(cgo,this); return; }
			}
		else
			// idle
			if ( !Inside<float>(cox,1-Shape.Wdt,cgo.Wdt)
				|| (!Inside<float>(coy,1-Shape.Hgt,cgo.Hgt)) )
				{ if (FrontParticles && !Contained) FrontParticles.Draw(cgo,this); return; }

	// ensure correct color is set
	if (GetGraphics()->BitmapClr) GetGraphics()->BitmapClr->SetClr(Color);

  // Debug Display //////////////////////////////////////////////////////////////////////
  if (::GraphicsSystem.ShowCommand && !eDrawMode)
    {
    C4Command *pCom;
    int32_t ccx=GetX(),ccy=GetY();
    int32_t x1,y1,x2,y2;
    char szCommand[200];
    StdStrBuf Cmds;
		int32_t iMoveTos=0;
    for (pCom=Command; pCom; pCom=pCom->Next)
      {
			switch (pCom->Command)
        {
        case C4CMD_MoveTo:
					// Angle
					int32_t iAngle; iAngle=Angle(ccx,ccy,pCom->Tx._getInt(),pCom->Ty); while (iAngle>180) iAngle-=360;
					// Path
          x1=ccx-cotx; y1=ccy-coty;
          x2=pCom->Tx._getInt()-cotx; y2=pCom->Ty-coty;
          Application.DDraw->DrawLine(cgo.Surface,cgo.X+x1,cgo.Y+y1,cgo.X+x2,cgo.Y+y2,CRed);
          Application.DDraw->DrawFrame(cgo.Surface,cgo.X+x2-1,cgo.Y+y2-1,cgo.X+x2+1,cgo.Y+y2+1,CRed);
          if (x1>x2) Swap(x1,x2); if (y1>y2) Swap(y1,y2);
          ccx=pCom->Tx._getInt(); ccy=pCom->Ty;
					// Message
					iMoveTos++; szCommand[0]=0;
					//sprintf(szCommand,"%s %d/%d",CommandName(pCom->Command),pCom->Tx,pCom->Ty,iAngle);
          break;
				case C4CMD_Put:
					sprintf(szCommand,"%s %s to %s",CommandName(pCom->Command),pCom->Target2 ? pCom->Target2->GetName() : pCom->Data ? C4IdText(pCom->Data.getC4ID()) : "Content",pCom->Target ? pCom->Target->GetName() : "");
					break;
				case C4CMD_Buy: case C4CMD_Sell:
					sprintf(szCommand,"%s %s at %s",CommandName(pCom->Command),C4IdText(pCom->Data.getC4ID()),pCom->Target ? pCom->Target->GetName() : "closest base");
					break;
				case C4CMD_Acquire:
					sprintf(szCommand,"%s %s",CommandName(pCom->Command),C4IdText(pCom->Data.getC4ID()));
					break;
				case C4CMD_Call:
					sprintf(szCommand,"%s %s in %s",CommandName(pCom->Command),pCom->Text->GetCStr(),pCom->Target ? pCom->Target->GetName() : "(null)");
					break;
				case C4CMD_Construct:
					C4Def *pDef; pDef=C4Id2Def(pCom->Data.getC4ID());
					sprintf(szCommand,"%s %s",CommandName(pCom->Command),pDef ? pDef->GetName() : "");
					break;
				case C4CMD_None:
					szCommand[0]=0;
					break;
				case C4CMD_Transfer:
					// Path
          x1=ccx-cotx; y1=ccy-coty;
          x2=pCom->Tx._getInt()-cotx; y2=pCom->Ty-coty;
          Application.DDraw->DrawLine(cgo.Surface,cgo.X+x1,cgo.Y+y1,cgo.X+x2,cgo.Y+y2,CGreen);
          Application.DDraw->DrawFrame(cgo.Surface,cgo.X+x2-1,cgo.Y+y2-1,cgo.X+x2+1,cgo.Y+y2+1,CGreen);
          if (x1>x2) Swap(x1,x2); if (y1>y2) Swap(y1,y2);
          ccx=pCom->Tx._getInt(); ccy=pCom->Ty;
					// Message
					sprintf(szCommand,"%s %s",CommandName(pCom->Command),pCom->Target ? pCom->Target->GetName() : "");
					break;
				default:
					sprintf(szCommand,"%s %s",CommandName(pCom->Command),pCom->Target ? pCom->Target->GetName() : "");
					break;
        }
			// Compose command stack message
			if (szCommand[0])
				{
				// End MoveTo stack first
				if (iMoveTos) { Cmds.AppendChar('|'); Cmds.AppendFormat("%dx MoveTo",iMoveTos); iMoveTos=0; }
				// Current message
				Cmds.AppendChar('|');
				if (pCom->Finished) Cmds.Append("<i>");
        Cmds.Append(szCommand);
				if (pCom->Finished) Cmds.Append("</i>");
				}
      }
		// Open MoveTo stack
		if (iMoveTos) { Cmds.AppendChar('|'); Cmds.AppendFormat("%dx MoveTo",iMoveTos); iMoveTos=0; }
		// Draw message
		int32_t cmwdt,cmhgt;  ::GraphicsResource.FontRegular.GetTextExtent(Cmds.getData(),cmwdt,cmhgt,true);
		Application.DDraw->TextOut(Cmds.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,cgo.X+cox-Shape.GetX(),cgo.Y+coy-10-cmhgt,CStdDDraw::DEFAULT_MESSAGE_COLOR,ACenter);
    }
  // Debug Display ///////////////////////////////////////////////////////////////////////////////

	// Don't draw (show solidmask)
	if (::GraphicsSystem.ShowSolidMask)
		if (SolidMask.Wdt)
			{
			// DrawSolidMask(cgo); - no need to draw it, because the 8bit-surface will be shown
			return;
			}

	// Contained check
	if (Contained && !eDrawMode) return;

	// Visibility inside FoW
	bool fOldClrModEnabled = !!(Category & C4D_IgnoreFoW);
	if (fOldClrModEnabled)
		{
		fOldClrModEnabled = lpDDraw->GetClrModMapEnabled();
		lpDDraw->SetClrModMapEnabled(false);
		}

	// Fire facet - always draw, even if particles are drawn as well
	if (OnFire /*&& !::Particles.IsFireParticleLoaded()*/) if (eDrawMode!=ODM_BaseOnly)
		{
		C4Facet fgo;
		// Straight: Full Shape.Rect on fire
		if (r==0)
			{
			fgo.Set(cgo.Surface,offX + Shape.GetX(),offY + Shape.GetY(),
							Shape.Wdt,Shape.Hgt-Shape.FireTop);
			}
		// Rotated: Reduced fire rect
		else
			{
			C4Rect fr;
			Shape.GetVertexOutline(fr);
			fgo.Set(cgo.Surface,
			        offX + fr.x,
			        offY + fr.y,
			        fr.Wdt, fr.Hgt);
			}
		::GraphicsResource.fctFire.Draw(fgo,false,FirePhase);
		}

	// color modulation (including construction sign...)
	if (ColorMod != 0xffffffff || BlitMode) if (!eDrawMode) PrepareDrawing();

	// Not active or rotated: BaseFace only
	if (!Action.pActionDef)
		{
		DrawFace(cgo, offX, offY);
		}

	// Active
	else
		{
		// FacetBase
		if (Action.pActionDef->GetPropertyInt(P_FacetBase))
			DrawFace(cgo, offX, offY, 0, Action.DrawDir);

		// Special: stretched action facet
		if (Action.Facet.Surface && Action.pActionDef->GetPropertyInt(P_FacetTargetStretch))
			{
			if (Action.Target)
				lpDDraw->Blit(Action.Facet.Surface,
					float(Action.Facet.X),float(Action.Facet.Y),float(Action.Facet.Wdt),float(Action.Facet.Hgt),
					cgo.Surface,
					offX + Shape.GetX() + Action.FacetX, offY + Shape.GetY() + Action.FacetY,Action.Facet.Wdt,
					(fixtof(Action.Target->fix_y) + Action.Target->Shape.GetY()) - (fixtof(fix_y) + Shape.GetY() + Action.FacetY),
					true);
			}
		else if (Action.Facet.Surface)
			DrawActionFace(cgo, offX, offY);
		}

	// end of color modulation
	if (ColorMod != 0xffffffff || BlitMode) if (!eDrawMode) FinishedDrawing();

	// draw overlays - after blit mode changes, because overlay gfx set their own
	if (pGfxOverlay) if (eDrawMode!=ODM_BaseOnly)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (!pGfxOvrl->IsPicture())
				pGfxOvrl->Draw(cgo, this, iByPlayer);

	// local particles in front of the object
	if (FrontParticles) if (eDrawMode!=ODM_BaseOnly) FrontParticles.Draw(cgo,this);

  // Select Mark
  if (Select)
		if (eDrawMode!=ODM_BaseOnly)
			if (ValidPlr(Owner))
				if (Owner == iByPlayer)
					if (::Players.Get(Owner)->SelectFlash)
						DrawSelectMark(cgo, 1);

	// Energy shortage
	if (NeedEnergy) if (::Game.iTick35>12) if (eDrawMode!=ODM_BaseOnly)
		{
		C4Facet &fctEnergy = ::GraphicsResource.fctEnergy;
		int32_t tx=cox+Shape.Wdt/2-fctEnergy.Wdt/2, ty=coy-fctEnergy.Hgt-5;
		fctEnergy.Draw(cgo.Surface,cgo.X+tx,cgo.Y+ty);
		}


	// Debug Display ////////////////////////////////////////////////////////////////////////
	if (::GraphicsSystem.ShowVertices) if (eDrawMode!=ODM_BaseOnly)
		{
		int32_t cnt;
		if (Shape.VtxNum>1)
			for (cnt=0; cnt<Shape.VtxNum; cnt++)
				{
				DrawVertex(cgo,
					cox-Shape.GetX()+Shape.VtxX[cnt],
					coy-Shape.GetY()+Shape.VtxY[cnt],
					(Shape.VtxCNAT[cnt] & CNAT_NoCollision) ? CBlue : (Mobile ? CRed : CYellow),
					Shape.VtxContactCNAT[cnt]);
				}
		}

	if (::GraphicsSystem.ShowEntrance) if (eDrawMode!=ODM_BaseOnly)
		{
		if (OCF & OCF_Entrance)
			Application.DDraw->DrawFrame(cgo.Surface,cgo.X+cox-Shape.x+Def->Entrance.x,
						 cgo.Y+coy-Shape.y+Def->Entrance.y,
						 cgo.X+cox-Shape.x+Def->Entrance.x+Def->Entrance.Wdt-1,
						 cgo.Y+coy-Shape.y+Def->Entrance.y+Def->Entrance.Hgt-1,
						 CBlue);
		if (OCF & OCF_Collection)
			Application.DDraw->DrawFrame(cgo.Surface,cgo.X+cox-Shape.x+Def->Collection.x,
						 cgo.Y+coy-Shape.y+Def->Collection.y,
						 cgo.X+cox-Shape.x+Def->Collection.x+Def->Collection.Wdt-1,
						 cgo.Y+coy-Shape.y+Def->Collection.y+Def->Collection.Hgt-1,
						 CRed);
		}

	if (::GraphicsSystem.ShowAction) if (eDrawMode!=ODM_BaseOnly)
		{
		if (Action.pActionDef)
			{
			StdStrBuf str;
			str.Format("%s (%d)",Action.pActionDef->GetName(),Action.Phase);
			int32_t cmwdt,cmhgt; ::GraphicsResource.FontRegular.GetTextExtent(str.getData(),cmwdt,cmhgt,true);
			Application.DDraw->TextOut(str.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,cgo.X+cox-Shape.GetX(),cgo.Y+coy-cmhgt,InLiquid ? 0xfa0000FF : CStdDDraw::DEFAULT_MESSAGE_COLOR,ACenter);
			}
		}
  // Debug Display ///////////////////////////////////////////////////////////////////////

	// Restore visibility inside FoW
	if (fOldClrModEnabled) lpDDraw->SetClrModMapEnabled(fOldClrModEnabled);

  }

void C4Object::DrawTopFace(C4TargetFacet &cgo, int32_t iByPlayer, DrawMode eDrawMode)
  {
	// Status
  if (!Status || !Def) return;
	// visible?
	if(!IsVisible(iByPlayer, eDrawMode==ODM_Overlay)) return;
	// target pos (parallax)
	float cotx = cgo.TargetX, coty = cgo.TargetY; if (eDrawMode!=ODM_Overlay) TargetPos(cotx, coty, cgo);
	// Clonk name
	// Name of Owner/Clonk (only when Crew Member; never in films)
	if (OCF & OCF_CrewMember) if ((Config.Graphics.ShowCrewNames || Config.Graphics.ShowCrewCNames) && (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)) if (!eDrawMode)
		if (Owner != iByPlayer && !Contained)
			{
			// inside screen range?
			if (!Inside<int>(GetX() + Shape.GetX() - cotx, 1 - Shape.Wdt, cgo.Wdt)
			|| !Inside<int>(GetY() + Shape.GetY() - coty, 1 - Shape.Hgt, cgo.Hgt)) return;
			// get player
			C4Player* pOwner = ::Players.Get(Owner);
			if (pOwner) if (!Hostile(Owner, iByPlayer)) if (!pOwner->IsInvisible())
				{
				int32_t X = GetX();
				int32_t Y = GetY() - Def->Shape.Hgt / 2 - 20;
				// compose string
				char szText[C4GM_MaxText+1];
				if (Config.Graphics.ShowCrewNames)
					if (Config.Graphics.ShowCrewCNames)
						sprintf(szText, "%s (%s)", GetName(), pOwner->GetName());
					else
						SCopy(pOwner->GetName(),szText);
				else
					SCopy(GetName(),szText);
				// Word wrap to cgo width
				int32_t iCharWdt, dummy; ::GraphicsResource.FontRegular.GetTextExtent("m", iCharWdt, dummy, false);
				int32_t iMaxLine = Max<int32_t>( cgo.Wdt / iCharWdt, 20 );
				SWordWrap(szText,' ','|',iMaxLine);
				// Adjust position by output boundaries
				int32_t iTX,iTY,iTWdt,iTHgt;
				::GraphicsResource.FontRegular.GetTextExtent(szText,iTWdt,iTHgt, true);
				iTX = BoundBy<int>( X-cotx, iTWdt/2, cgo.Wdt-iTWdt/2 );
				iTY = BoundBy<int>( Y-coty-iTHgt, 0, cgo.Hgt-iTHgt );
				// Draw
				Application.DDraw->TextOut(szText, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, cgo.X + iTX, cgo.Y + iTY,
														pOwner->ColorDw|0x7f000000,ACenter);
				}
			}
	// TopFace
	if (!(TopFace.Surface || (OCF & OCF_Construct))) return;
	// Output position
  	float offX = cgo.X + fixtof(fix_x) - cotx, offY = cgo.Y + fixtof(fix_y) - coty;
	float cox = fixtof(fix_x) + Shape.GetX() - cotx, coy = fixtof(fix_y) + Shape.GetY() - coty;
	// Output bounds check
	if (!Inside<float>(cox,1-Shape.Wdt,cgo.Wdt)
		|| !Inside<float>(coy,1-Shape.Hgt,cgo.Hgt))
			return;
	// Don't draw (show solidmask)
	if (::GraphicsSystem.ShowSolidMask)
		if (SolidMask.Wdt)
			return;
	// Contained
	if (Contained) if (eDrawMode!=ODM_Overlay) return;
	// Construction sign
	if (OCF & OCF_Construct) if (r==0) if (eDrawMode!=ODM_BaseOnly)
		{
		C4Facet &fctConSign = ::GraphicsResource.fctConstruction;
		lpDDraw->Blit(fctConSign.Surface,
			fctConSign.X, fctConSign.Y,
			fctConSign.Wdt, fctConSign.Hgt,
			cgo.Surface,
			cgo.X + cox, cgo.Y + coy + Shape.Hgt - fctConSign.Hgt,
			fctConSign.Wdt, fctConSign.Hgt, true);
		}
	// FacetTopFace: Override TopFace.GetX()/GetY()
	if (Action.pActionDef && Action.pActionDef->GetPropertyInt(P_FacetTopFace))
		{
		int32_t iPhase = Action.Phase;
		if (Action.pActionDef->GetPropertyInt(P_Reverse)) iPhase = Action.pActionDef->GetPropertyInt(P_Length) - 1 - Action.Phase;
		TopFace.X = Action.pActionDef->GetPropertyInt(P_X) + Def->TopFace.x + Action.pActionDef->GetPropertyInt(P_Wdt) * iPhase;
		TopFace.Y = Action.pActionDef->GetPropertyInt(P_Y) + Def->TopFace.y + Action.pActionDef->GetPropertyInt(P_Hgt) * Action.DrawDir;
		}
	// ensure correct color is set
	if (GetGraphics()->BitmapClr) GetGraphics()->BitmapClr->SetClr(Color);
	// color modulation
	if (!eDrawMode) PrepareDrawing();
	// Draw top face bitmap
	if (Con!=FullCon && Def->GrowthType)
		// stretched
		lpDDraw->Blit(TopFace.Surface,
			TopFace.X, TopFace.Y, TopFace.Wdt, TopFace.Hgt,
			cgo.Surface,
			cgo.X + cox + float(Def->TopFace.tx * Con) / FullCon, cgo.Y + coy + float(Def->TopFace.ty * Con) / FullCon,
			float(TopFace.Wdt * Con) / FullCon, float(TopFace.Hgt * Con) / FullCon,
			true, pDrawTransform ? &C4DrawTransform(*pDrawTransform, offX, offY) : NULL);
	else
		// normal
		lpDDraw->Blit(TopFace.Surface,
			TopFace.X,TopFace.Y,
			TopFace.Wdt,TopFace.Hgt,
			cgo.Surface,
			cgo.X + cox + Def->TopFace.tx, cgo.Y + coy + Def->TopFace.ty,
			TopFace.Wdt, TopFace.Hgt,
			true, pDrawTransform ? &C4DrawTransform(*pDrawTransform, offX, offY) : NULL);
	// end of color modulation
	if (!eDrawMode) FinishedDrawing();
  }

void C4Object::DrawLine(C4TargetFacet &cgo)
	{
	// Audibility
	SetAudibilityAt(cgo, Shape.VtxX[0],Shape.VtxY[0]);
	SetAudibilityAt(cgo, Shape.VtxX[Shape.VtxNum-1],Shape.VtxY[Shape.VtxNum-1]);
	// additive mode?
	PrepareDrawing();
	// Draw line segments
	C4Value colorsV; GetProperty(Strings.P[P_LineColors], colorsV);
	C4ValueArray *colors = colorsV.getArray();
	int32_t color0 = 0xFFFF00FF, color1 = 0xFFFF00FF;	// use bright colors so author notices
	if (colors)
		{
		color0 = colors->GetItem(0).getInt(); color1 = colors->GetItem(1).getInt();
		}
  for (int32_t vtx=0; vtx+1<Shape.VtxNum; vtx++)
		switch (Def->Line)
			{
			case C4D_Line_Power:
			case C4D_Line_Source: case C4D_Line_Drain:
			case C4D_Line_Lightning:
			case C4D_Line_Rope:
			case C4D_Line_Vertex:
			case C4D_Line_Colored:
				cgo.DrawLineDw(Shape.VtxX[vtx],Shape.VtxY[vtx],
										 Shape.VtxX[vtx+1],Shape.VtxY[vtx+1],
										 color0, color1);
				break;
			}
	// reset blit mode
	FinishedDrawing();
	}

void C4Object::DrawEnergy(C4Facet &cgo)
	{
	//cgo.DrawEnergyLevel(Energy,GetPhysical()->Energy);
	cgo.DrawEnergyLevelEx(Energy,GetPhysical()->Energy, ::GraphicsResource.fctEnergyBars, 0);
	}

void C4Object::DrawMagicEnergy(C4Facet &cgo)
	{
	// draw in units of MagicPhysicalFactor, so you can get a full magic energy bar by script even if partial magic energy training is not fulfilled
	//cgo.DrawEnergyLevel(MagicEnergy/MagicPhysicalFactor,GetPhysical()->Magic/MagicPhysicalFactor,39);
	cgo.DrawEnergyLevelEx(MagicEnergy/MagicPhysicalFactor,GetPhysical()->Magic/MagicPhysicalFactor, ::GraphicsResource.fctEnergyBars, 1);
	}

void C4Object::DrawBreath(C4Facet &cgo)
	{
	//cgo.DrawEnergyLevel(Breath,GetPhysical()->Breath,99);
	cgo.DrawEnergyLevelEx(Breath,GetPhysical()->Breath, ::GraphicsResource.fctEnergyBars, 2);
	}

void C4Object::CompileFunc(StdCompiler *pComp)
	{
  bool fCompiler = pComp->isCompiler();
  if(fCompiler)
    Clear();

	// Compile ID, search definition
	pComp->Value(mkNamingAdapt( mkC4IDAdapt(id),									"id",									C4ID_None					));
	if(fCompiler)
		{
		Def = ::Definitions.ID2Def(id);
		prototype = Def;
		if(!Def)
			{ pComp->excNotFound(LoadResStr("IDS_PRC_UNDEFINEDOBJECT"),C4IdText(id)); return; }
		}

	// Write the name only if the object has an individual name, use def name as default for reading.
	// (Info may overwrite later, see C4Player::MakeCrewMember)
	/*if (pComp->isCompiler())
		pComp->Value(mkNamingAdapt(Name, "Name", Def->Name));
	else if (!Name.isRef())
		// Write the name only if the object has an individual name
		// 2do: And what about binary compilers?
		pComp->Value(mkNamingAdapt(Name, "Name"));*/

	pComp->Value(mkNamingAdapt( Number,														"Number",							-1								));
	pComp->Value(mkNamingAdapt( Status,														"Status",							1									));
	pComp->Value(mkNamingAdapt( toC4CStrBuf(nInfo),								"Info",								""								));
	pComp->Value(mkNamingAdapt( Owner,														"Owner",							NO_OWNER					));
	pComp->Value(mkNamingAdapt( Timer,														"Timer",							0									));
	pComp->Value(mkNamingAdapt( Controller,												"Controller",					NO_OWNER					));
	pComp->Value(mkNamingAdapt( LastEnergyLossCausePlayer,				"LastEngLossPlr", 		NO_OWNER					));
	pComp->Value(mkNamingAdapt( Category,													"Category",						0									));
	// old-style coordinates - compile dummy value to prevent warning
	// remove this once all Objects.txt have been rewritten without the values
	int32_t qX=0, qY=0, motion_x = 0, motion_y = 0;
	pComp->Value(mkNamingAdapt( qX,																"X",									0									));
	pComp->Value(mkNamingAdapt( qY,																"Y",									0									));
	pComp->Value(mkNamingAdapt( motion_x,													"MotionX",						0									));
	pComp->Value(mkNamingAdapt( motion_y,													"MotionY",						0									));

	pComp->Value(mkNamingAdapt( r,																"Rotation",						0									));

	pComp->Value(mkNamingAdapt( iLastAttachMovementFrame,					"LastSolidAtchFrame",	-1								));
	pComp->Value(mkNamingAdapt( NoCollectDelay,										"NoCollectDelay",			0 								));
	pComp->Value(mkNamingAdapt( Con,															"Size",								0									));
	pComp->Value(mkNamingAdapt( OwnMass,											    "OwnMass",						0									));
	pComp->Value(mkNamingAdapt( Mass,															"Mass",								0									));
	pComp->Value(mkNamingAdapt( Damage,													  "Damage",							0									));
	pComp->Value(mkNamingAdapt( Energy,														"Energy",							0									));
	pComp->Value(mkNamingAdapt( MagicEnergy,											"MagicEnergy",				0									));
	pComp->Value(mkNamingAdapt( Alive,														"Alive",							false							));
	pComp->Value(mkNamingAdapt( Breath,														"Breath",							0									));
	pComp->Value(mkNamingAdapt( FirePhase,												"FirePhase",					0									));
	pComp->Value(mkNamingAdapt( Color,														"Color",							0u								)); // TODO: Convert
	pComp->Value(mkNamingAdapt( Color,														"ColorDw",						0u								));
	// default to X/Y values to support objects where FixX/FixY was manually removed
	pComp->Value(mkNamingAdapt( fix_x,														"FixX",								itofix(qX)									));
	pComp->Value(mkNamingAdapt( fix_y,														"FixY",								itofix(qY)									));
	pComp->Value(mkNamingAdapt( fix_r,														"FixR",								0									));
	pComp->Value(mkNamingAdapt( xdir,															"XDir",								0									));
	pComp->Value(mkNamingAdapt( ydir,															"YDir",								0									));
	pComp->Value(mkNamingAdapt( rdir,															"RDir",								0									));
	pComp->Value(mkParAdapt(Shape, true));
	pComp->Value(mkNamingAdapt( fOwnVertices,											"OwnVertices",				false							));
	pComp->Value(mkNamingAdapt( SolidMask,												"SolidMask",					Def->SolidMask		));
	pComp->Value(mkNamingAdapt( PictureRect,											"Picture"																));
	pComp->Value(mkNamingAdapt( Mobile,														"Mobile",							false							));
	pComp->Value(mkNamingAdapt( Select,														"Selected",						false							));
	pComp->Value(mkNamingAdapt( OnFire,														"OnFire",							false							));
	pComp->Value(mkNamingAdapt( InLiquid,													"InLiquid",						false							));
	pComp->Value(mkNamingAdapt( EntranceStatus,										"EntranceStatus", 		false							));
	pComp->Value(mkNamingAdapt( PhysicalTemporary,								"PhysicalTemporary",	false							));
	pComp->Value(mkNamingAdapt( NeedEnergy,												"NeedEnergy",					false							));
	pComp->Value(mkNamingAdapt( OCF,															"OCF",								0u									));
	pComp->Value(Action);
	pComp->Value(mkNamingAdapt( nContained,												"Contained",					0									));
	pComp->Value(mkNamingAdapt( nActionTarget1,										"ActionTarget1",			0									));
	pComp->Value(mkNamingAdapt( nActionTarget2,										"ActionTarget2",			0									));
	pComp->Value(mkNamingAdapt( Component,												"Component"															));
	pComp->Value(mkNamingAdapt( Contents,													"Contents"															));
	pComp->Value(mkNamingAdapt( PlrViewRange,											"PlrViewRange",				0									));
	pComp->Value(mkNamingAdapt( LocalNamed,												"LocalNamed"														));
	pComp->Value(mkNamingAdapt( ColorMod,													"ColorMod",						0xffffffffu								));
	pComp->Value(mkNamingAdapt( BlitMode,													"BlitMode",						0u								));
	pComp->Value(mkNamingAdapt( CrewDisabled,											"CrewDisabled",				false							));
	pComp->Value(mkNamingAdapt( nLayer,         					        "Layer",          		0									));
	pComp->Value(mkNamingAdapt( C4DefGraphicsAdapt(pGraphics),		"Graphics",       		&Def->Graphics		));
	pComp->Value(mkNamingPtrAdapt( pDrawTransform,								"DrawTransform"													));
	pComp->Value(mkNamingPtrAdapt( pEffects,											"Effects"																));
	pComp->Value(mkNamingAdapt( C4GraphicsOverlayListAdapt(pGfxOverlay),"GfxOverlay",		(C4GraphicsOverlay *)NULL));

	if(PhysicalTemporary)
		{
		pComp->FollowName("Physical");
		pComp->Value(TemporaryPhysical);
		}


  // Commands
  if(pComp->FollowName("Commands"))
		if(fCompiler)
			{
			C4Command *pCmd = NULL;
			for(int i = 1; ; i++)
				{
				// Every command has its own naming environment
				StdStrBuf Naming = FormatString("Command%d", i);
				pComp->Value(mkNamingPtrAdapt(pCmd ? pCmd->Next : Command, Naming.getData()));
				// Last command?
				pCmd = (pCmd ? pCmd->Next : Command);
				if(!pCmd)
					break;
				pCmd->cObj = this;
				}
			}
		else
			{
			C4Command *pCmd = Command;
			for(int i = 1; pCmd; i++, pCmd = pCmd->Next)
				{
				StdStrBuf Naming = FormatString("Command%d", i);
				pComp->Value(mkNamingAdapt(*pCmd, Naming.getData()));
				}
			}

  // Compiling? Do initialization.
  if(fCompiler)
    {
	  // add to def count
	  Def->Count++;

	  // set local variable names
	  LocalNamed.SetNameList(&Def->Script.LocalNamed);

	  // Set action (override running data)
	  int32_t iTime=Action.Time;
	  int32_t iPhase=Action.Phase;
	  int32_t iPhaseDelay=Action.PhaseDelay;
	  /* FIXME if (SetActionByName(Action.pActionDef->GetName(),0,0,false)) 
		  {
		  Action.Time=iTime;
		  Action.Phase=iPhase; // No checking for valid phase
		  Action.PhaseDelay=iPhaseDelay;
		  }*/

	  // if on fire but no effect is present (old-style savegames), re-incinerate
	  int32_t iFireNumber;
	  C4Value Par1, Par2, Par3, Par4;
	  if (OnFire && !pEffects) new C4Effect(this, C4Fx_Fire, C4Fx_FirePriority, C4Fx_FireTimer, NULL, 0, Par1, Par2, Par3, Par4, false, iFireNumber);

	  // blit mode not assigned? use definition default then
	  if (!BlitMode) BlitMode = Def->BlitMode;

	  // object needs to be resorted? May happen if there's unsorted objects in savegame
	  if (Unsorted) Game.fResortAnyObject = true;

		// initial OCF update
		SetOCF();

    }

	}

void C4Object::EnumeratePointers()
	{

	// Standard enumerated pointers
	nContained = ::Objects.ObjectNumber(Contained);
	nActionTarget1 = ::Objects.ObjectNumber(Action.Target);
	nActionTarget2 = ::Objects.ObjectNumber(Action.Target2);
	nLayer = ::Objects.ObjectNumber(pLayer);

	// Info by name
	//if (Info) SCopy(Info->Name,nInfo,C4MaxName);
	if (Info) nInfo = Info->Name; else nInfo.Clear();

	// Commands
	for (C4Command *pCom=Command; pCom; pCom=pCom->Next)
		pCom->EnumeratePointers();

	// effects
	if (pEffects) pEffects->EnumeratePointers();

	// gfx overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			pGfxOvrl->EnumeratePointers();
	}

void C4Object::DenumeratePointers()
	{

	// Standard enumerated pointers
	Contained = ::Objects.ObjectPointer(nContained);
	Action.Target = ::Objects.ObjectPointer(nActionTarget1);
	Action.Target2 = ::Objects.ObjectPointer(nActionTarget2);
	pLayer = ::Objects.ObjectPointer(nLayer);

	// Post-compile object list
	Contents.DenumerateRead();

	// Local variable pointers
	LocalNamed.DenumeratePointers();

	// Commands
	for (C4Command *pCom=Command; pCom; pCom=pCom->Next)
		pCom->DenumeratePointers();

	// effects
	if (pEffects) pEffects->DenumeratePointers();

	// gfx overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			pGfxOvrl->DenumeratePointers();
	}

bool DrawCommandQuery(int32_t controller, C4ScriptHost& scripthost, int32_t* mask, int com)
  {
  int method = scripthost.GetControlMethod(com, mask[0], mask[1]);
  C4Player* player = ::Players.Get(controller);
  if(!player) return false;

  switch(method)
    {
    case C4AUL_ControlMethod_All: return true;
    case C4AUL_ControlMethod_None: return false;
    case C4AUL_ControlMethod_Classic: return !player->PrefControlStyle;
    case C4AUL_ControlMethod_JumpAndRun: return !!player->PrefControlStyle;
		default: return false;
    }
  }

void C4Object::DrawCommands(C4Facet &cgoBottom, C4Facet &cgoSide, C4RegionList *pRegions)
	{
	int32_t cnt;
	C4Facet ccgo, ccgo2;
	C4Object *tObj;
	int32_t iDFA = GetProcedure();
	bool fContainedDownOverride = false;
	bool fContainedLeftOverride = false; // carlo
	bool fContainedRightOverride = false; // carlo
	bool fContentsActivationOverride = false;

	// Active menu (does not consider owner's active player menu)
	if (Menu && Menu->IsActive()) return;

	DWORD ocf = OCF_Construct;
	if(Action.ComDir == COMD_Stop && iDFA == DFA_WALK && (tObj = ::Objects.AtObject(GetX(), GetY(), ocf, this)))
		{
		int32_t com = COM_Down_D;
		if(::Players.Get(Controller)->PrefControlStyle) com = COM_Down;

		tObj->DrawCommand(cgoBottom,C4FCT_Right,NULL,com,pRegions,Owner,
			FormatString(LoadResStr("IDS_CON_BUILD"), tObj->GetName()).getData(),&ccgo);
		tObj->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, tObj->Color, tObj);
		::GraphicsResource.fctBuild.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true);
		}

	// Grab target control (control flag)
	if (iDFA==DFA_PUSH && Action.Target)
		{
		bool letgobydouble = !::Players.Get(Controller)->PrefControlStyle
			|| DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, 3)
			|| DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, 11)
			|| DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, 19);
		for (cnt=ComOrderNum-1; cnt>=0; cnt--)
			if(DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, cnt))
				{
				Action.Target->DrawCommand(cgoBottom,C4FCT_Right,PSF_Control,ComOrder(cnt),pRegions,Owner);
				}
			else if ((ComOrder(cnt) == COM_Down_D && letgobydouble)
					|| (ComOrder(cnt) == COM_Down && !letgobydouble))
				{
				// Let Go
				Action.Target->DrawCommand(cgoBottom, C4FCT_Right, NULL, ComOrder(cnt), pRegions, Owner, 
					FormatString(LoadResStr("IDS_CON_UNGRAB"), Action.Target->GetName()).getData(), &ccgo);
				Action.Target->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Action.Target->Color, Action.Target);
				::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 6);
				}
			else if (ComOrder(cnt) == COM_Throw)
				{
				// Put
				if ((tObj = Contents.GetObject()) && (Action.Target->Def->GrabPutGet & C4D_Grab_Put))
					{
					Action.Target->DrawCommand(cgoBottom, C4FCT_Right, NULL, COM_Throw, pRegions, Owner,
						FormatString(LoadResStr("IDS_CON_PUT"), tObj->GetName(), Action.Target->GetName()).getData(), &ccgo);
					tObj->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, tObj->Color, tObj);
					::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
					}
				// Get
				else if (Action.Target->Contents.ListIDCount(C4D_Get) && (Action.Target->Def->GrabPutGet & C4D_Grab_Get))
					{
					Action.Target->DrawCommand(cgoBottom,C4FCT_Right,NULL,COM_Throw,pRegions,Owner,
						FormatString(LoadResStr("IDS_CON_GET"),Action.Target->GetName()).getData(), &ccgo);
					Action.Target->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Action.Target->Color, Action.Target);
					::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 1);
					}
				}
		}

	// Contained control (contained control flag)
	if (Contained)
		{
		for (cnt=ComOrderNum-1; cnt>=0; cnt--)
			if(DrawCommandQuery(Controller, Contained->Def->Script, Contained->Def->Script.ContainedControlMethod, cnt))
				{
				Contained->DrawCommand(cgoBottom,C4FCT_Right,PSF_ContainedControl,ComOrder(cnt),pRegions,Owner);
				// Contained control down overrides contained exit control
				if (Com2Control(ComOrder(cnt))==CON_Down) fContainedDownOverride=true;
				// carlo - Contained controls left/right override contained Take, Take2 controls
				if (Com2Control(ComOrder(cnt))==CON_Left)  fContainedLeftOverride=true;
				if (Com2Control(ComOrder(cnt))==CON_Right) fContainedRightOverride=true;
				}
		// Contained exit
		if (!fContainedDownOverride)
			{
			DrawCommand(cgoBottom,C4FCT_Right,NULL,COM_Down,pRegions,Owner,
								LoadResStr("IDS_CON_EXIT"),&ccgo);
			::GraphicsResource.fctExit.Draw(ccgo);
			}
		// Contained put & activate
		// carlo
		int32_t nContents = Contained->Contents.ListIDCount(C4D_Get);
		if (nContents)
			{
			// carlo: Direct get ("Take2")
			if (!fContainedRightOverride)
				{
				Contained->DrawCommand(cgoBottom,C4FCT_Right,NULL,COM_Right,pRegions,Owner,
					FormatString(LoadResStr("IDS_CON_GET"),Contained->GetName()).getData(),&ccgo);
				Contained->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Contained->Color, Contained);
				::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 1);
				}
			// carlo: Get ("Take")
			if (!fContainedLeftOverride)
				{
				Contained->DrawCommand(cgoBottom,C4FCT_Right,NULL,COM_Left,pRegions,Owner,
					FormatString(LoadResStr("IDS_CON_ACTIVATEFROM"),Contained->GetName()).getData(),&ccgo);
				Contained->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Contained->Color, Contained);
				::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
				}
			}
		if (tObj=Contents.GetObject())
			{
			// Put
			Contained->DrawCommand(cgoBottom,C4FCT_Right,NULL,COM_Throw,pRegions,Owner,
				FormatString(LoadResStr("IDS_CON_PUT"),tObj->GetName(),Contained->GetName()).getData(),&ccgo);
			tObj->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, tObj->Color, tObj);
			::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
			}
		else if (nContents)
			{
			// Get
			Contained->DrawCommand(cgoBottom,C4FCT_Right,NULL,COM_Throw,pRegions,Owner,
				FormatString(LoadResStr("IDS_CON_ACTIVATEFROM"),Contained->GetName()).getData(),&ccgo);
			Contained->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Contained->Color, Contained);
			::GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
			}
		}

	// Contents activation (activation control flag)
	if (!Contained)
		{
		if ((iDFA==DFA_WALK) || (iDFA==DFA_SWIM) || (iDFA==DFA_DIG))
			if (tObj=Contents.GetObject())
				if (DrawCommandQuery(Controller, tObj->Def->Script, tObj->Def->Script.ActivationControlMethod, COM_Dig_D))
					{
					tObj->DrawCommand(cgoBottom,C4FCT_Right,PSF_Activate,COM_Dig_D,pRegions,Owner);
					// Flag override self-activation
					fContentsActivationOverride=true;
					}

		// Self activation (activation control flag)
		if (!fContentsActivationOverride)
			if ((iDFA==DFA_WALK) || (iDFA==DFA_SWIM) || (iDFA==DFA_FLOAT))
				if (DrawCommandQuery(Controller, Def->Script, Def->Script.ActivationControlMethod, COM_Dig_D))
					DrawCommand(cgoSide,C4FCT_Bottom | C4FCT_Half,PSF_Activate,COM_Dig_D,pRegions,Owner);
		}

	// Self special control (control flag)
	for (cnt=6; cnt<ComOrderNum; cnt++)
		{
		// Hardcoded com order indexes for COM_Specials
		if (cnt==8) cnt=14; if (cnt==16) cnt=22;
		// Control in control flag?
		if (DrawCommandQuery(Controller, Def->Script, Def->Script.ControlMethod, cnt))
			DrawCommand(cgoSide,C4FCT_Bottom | C4FCT_Half,PSF_Control,ComOrder(cnt),pRegions,Owner);
		}
	}

void C4Object::DrawPicture(C4Facet &cgo, bool fSelected, C4RegionList *pRegions)
	{
	// Draw def picture with object color
	Def->Draw(cgo,fSelected,Color,this);
	// Region
	if (pRegions) pRegions->Add(cgo.X,cgo.Y,cgo.Wdt,cgo.Hgt,GetName(),COM_None,this);
	}

void C4Object::Picture2Facet(C4FacetSurface &cgo)
	{
	// set picture rect to facet
	C4Rect fctPicRect = PictureRect;
	if (!fctPicRect.Wdt) fctPicRect = Def->PictureRect;
	C4Facet fctPicture;
	fctPicture.Set(GetGraphics()->GetBitmap(Color),fctPicRect.x,fctPicRect.y,fctPicRect.Wdt,fctPicRect.Hgt);

	// use direct facet w/o own data if possible
	if (ColorMod == 0xffffffff && BlitMode == C4GFXBLIT_NORMAL && !pGfxOverlay)
		{
		cgo.Set(fctPicture);
		return;
		}

	// otherwise, draw to picture facet
	if (!cgo.Create(cgo.Wdt, cgo.Hgt)) return;

	// specific object color?
	PrepareDrawing();

	// draw picture itself
	fctPicture.Draw(cgo,true);

	// draw overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrl->IsPicture())
				pGfxOvrl->DrawPicture(cgo, this);

	// done; reset drawing states
	FinishedDrawing();
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
		if(!::Players.Get(Owner)->MakeCrewMember(this, true, false))
			pPlr->Crew.Remove(this);
		return true;
		}
	// Info set, but not in crew list, so
	//    a) The savegame is old-style (without crew list)
	// or b) The clonk is dead
	// or c) The clonk belongs to a script player that's restored without Game.txt
	else if (nInfo.getLength())
		{
		if(!::Players.Get(Owner)->MakeCrewMember(this, true, false))
			return false;
		// Dead and gone (info flags, remove from crew/cursor)
		if (!Alive)
			{
			Info->HasDied=true;
			if (ValidPlr(Owner)) ::Players.Get(Owner)->ClearPointers(this, true);
			}
		return true;
		}
	return false;
	}

bool C4Object::AssignPlrViewRange()
	{
	// no range?
	if (!PlrViewRange) return true;
	// add to FoW-repellers
	PlrFoWActualize();
	// success
	return true;
	}

void C4Object::ClearInfo(C4ObjectInfo *pInfo)
	{
	if (Info==pInfo)
		{
		Info=NULL;
		}
	}

void C4Object::Clear()
	{
	if (pEffects) { delete pEffects; pEffects=NULL; }
	if (FrontParticles) FrontParticles.Clear();
	if (BackParticles) BackParticles.Clear();
  if (pSolidMaskData) { delete pSolidMaskData; pSolidMaskData=NULL; }
	if (Menu) delete Menu; Menu=NULL;
	if (MaterialContents) delete MaterialContents; MaterialContents=NULL;
	// clear commands!
	C4Command *pCom, *pNext;
	for (pCom=Command; pCom; pCom=pNext)
		{
		pNext=pCom->Next; delete pCom; pCom=pNext;
		}
	if (pDrawTransform) { delete pDrawTransform; pDrawTransform=NULL; }
	if (pGfxOverlay) { delete pGfxOverlay; pGfxOverlay=NULL; }
	}

bool C4Object::ContainedControl(BYTE byCom)
	{
	// Check
  if (!Contained) return false;
	// Check if object is about to exit; if so, return
	// dunno, maybe I should check all the commands, not just the first one?
	if ((byCom == COM_Left || byCom == COM_Right) && Command)
		if (Command->Command == C4CMD_Exit)
			// hack: in structures only; not in vehicles
			// they might have a pending Exit-command due to a down-control
			if (Contained->Category & C4D_Structure)
				return false; // or true? Currently it doesn't matter.
	// get script function if defined
	C4AulFunc* sf = Contained->Def->Script.GetSFunc(FormatString(PSF_ContainedControl,ComName(byCom)).getData());
	// objects may overload hardcoded actions 
	C4Def *pCDef = Contained->Def;
	bool result = false;
	C4Player * pPlr = ::Players.Get(Controller);
	if (sf && !!sf->Exec(Contained, &C4AulParSet(C4VObj(this)))) result = true;
	// AutoStopControl: Also notify container about controlupdate
	// Note Contained may be nulled now due to ContainedControl call
	if(Contained && !(byCom & (COM_Single | COM_Double)) && pPlr->PrefControlStyle)
		{
		int32_t PressedComs = pPlr->PressedComs;
		C4AulParSet set(C4VObj(this),
			C4VInt(Coms2ComDir(PressedComs)),
			C4VBool(!!(PressedComs & (1 << COM_Dig))),
			C4VBool(!!(PressedComs & (1 << COM_Throw))));
		Contained->Call(PSF_ContainedControlUpdate, &set);
		}
	if(result) return true;

	// hardcoded actions
	switch (byCom)
		{
		case COM_Down:
			PlayerObjectCommand(Owner,C4CMD_Exit);
			break;
		case COM_Throw:
			PlayerObjectCommand(Owner,C4CMD_Throw);
			break;
		case COM_Left:
			PlayerObjectCommand(Owner,C4CMD_Take);
			break;
		case COM_Right:
			PlayerObjectCommand(Owner,C4CMD_Take2);
			break;
		}
	// Success
	return true;
	}

bool C4Object::CallControl(C4Player *pPlr, BYTE byCom, C4AulParSet *pPars)
	{
	assert(pPlr);

	bool result = !!Call(FormatString(PSF_Control,ComName(byCom)).getData(),pPars);

	// Call ControlUpdate when using Jump'n'Run control
	if(pPlr->PrefControlStyle)
		{
		int32_t PressedComs = pPlr->PressedComs;
		C4AulParSet set(pPars ? pPars->Par[0] : C4VObj(this),
			C4VInt(Coms2ComDir(PressedComs)),
			C4VBool(!!(PressedComs & (1 << COM_Dig))),
			C4VBool(!!(PressedComs & (1 << COM_Throw))),
			C4VBool(!!(PressedComs & (1 << COM_Special))),
			C4VBool(!!(PressedComs & (1 << COM_Special2))));
		Call(PSF_ControlUpdate, &set);
		}
	return result;
	}

void C4Object::DirectCom(BYTE byCom, int32_t iData) // By player ObjectCom
	{
#ifdef DEBUGREC_OBJCOM
	C4RCObjectCom rc = { byCom, iData, Number };
	AddDbgRec(RCT_ObjCom, &rc, sizeof(C4RCObjectCom));
#endif

	// COM_Special and COM_Contents specifically bypass the menu and always go to the object
	bool fBypassMenu = ((byCom == COM_Special) || (byCom == COM_Contents));

	// Menu control
	if (!fBypassMenu)
		if (Menu && Menu->Control(byCom,iData)) return;

	// Ignore any menu com leftover in control queue from closed menu
	if (Inside(byCom,COM_MenuNavigation1,COM_MenuNavigation2)) return;

	// Wether this is a KeyRelease-event
	bool IsRelease = Inside(byCom, COM_ReleaseFirst, COM_ReleaseLast);

	// Decrease NoCollectDelay
	if (!(byCom & COM_Single) && !(byCom & COM_Double) && !IsRelease)
		if (NoCollectDelay>0)
			NoCollectDelay--;

	// COM_Contents contents shift (data is target number (not ID!))
	// contents shift must always be done to container object, which is not necessarily this
	if (byCom==COM_Contents)
		{
		C4Object *pTarget = ::Objects.SafeObjectPointer(iData);
		if (pTarget && pTarget->Contained)
			pTarget->Contained->DirectComContents(pTarget, true);
		return;
		}

	// Contained control (except specials - hey, doesn't catch singles or doubles)
	if (Contained)
		if (byCom!=COM_Special && byCom!=COM_Special2 && byCom!=COM_WheelUp && byCom!=COM_WheelDown)
			{	ContainedControl(byCom); return; }

	// Regular DirectCom clears commands
	if (!(byCom & COM_Single) && !(byCom & COM_Double) && !IsRelease)
		ClearCommands();

	// Object script override
	C4Player *pController;
	if (pController = ::Players.Get(Controller))
		if (CallControl(pController, byCom))
			return;

	// direct wheel control
	if (byCom==COM_WheelUp || byCom==COM_WheelDown)
		// scroll contents
		{ ShiftContents(byCom==COM_WheelUp, true); return; }

	// The Player updates Controller before calling this, so trust Players.Get will return it
	if (pController && pController->PrefControlStyle)
		{
		AutoStopDirectCom(byCom, iData);
		return;
		}

	// Control by procedure
	switch (GetProcedure())
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_WALK:
			switch (byCom)
				{
				case COM_Left:   ObjectComMovement(this,COMD_Left); break;
				case COM_Right:  ObjectComMovement(this,COMD_Right); break;
				case COM_Down:   ObjectComMovement(this,COMD_Stop); break;
				case COM_Up:     ObjectComUp(this); break;
				case COM_Down_D: ObjectComDownDouble(this); break;
				case COM_Dig_S:
					if (ObjectComDig(this))
						{
						Action.ComDir = (Action.Dir==DIR_Right) ? COMD_DownRight : COMD_DownLeft;
						}
					break;
				case COM_Dig_D:  ObjectComDigDouble(this); break;
				case COM_Throw:  PlayerObjectCommand(Owner,C4CMD_Throw); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_FLIGHT: case DFA_KNEEL: case DFA_THROW:
			switch (byCom)
				{
				case COM_Left:   ObjectComMovement(this,COMD_Left); break;
				case COM_Right:  ObjectComMovement(this,COMD_Right); break;
				case COM_Down:   ObjectComMovement(this,COMD_Stop); break;
				case COM_Throw:  PlayerObjectCommand(Owner,C4CMD_Throw); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_SCALE:
			switch (byCom)
				{
				case COM_Left:
					if (Action.Dir==DIR_Left) ObjectComMovement(this,COMD_Stop);
					else { ObjectComMovement(this,COMD_Left); ObjectComLetGo(this,-1); }
					break;
				case COM_Right:
					if (Action.Dir==DIR_Right) ObjectComMovement(this,COMD_Stop);
					else { ObjectComMovement(this,COMD_Right); ObjectComLetGo(this,+1); }
					break;
				case COM_Up:   ObjectComMovement(this,COMD_Up); break;
				case COM_Down: ObjectComMovement(this,COMD_Down); break;
				case COM_Throw:	PlayerObjectCommand(Owner,C4CMD_Drop); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_HANGLE:
			switch (byCom)
				{
				case COM_Left:  ObjectComMovement(this,COMD_Left); break;
				case COM_Right: ObjectComMovement(this,COMD_Right); break;
				case COM_Up:    ObjectComMovement(this,COMD_Stop); break;
				case COM_Down:  ObjectComLetGo(this,0); break;
				case COM_Throw:	PlayerObjectCommand(Owner,C4CMD_Drop); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_DIG:
			switch (byCom)
				{
				case COM_Left:  if (Inside<int32_t>(Action.ComDir,COMD_UpRight,COMD_Left)) Action.ComDir++; break;
				case COM_Right: if (Inside<int32_t>(Action.ComDir,COMD_Right,COMD_UpLeft)) Action.ComDir--; break;
				case COM_Down:  ObjectComStop(this); break;
				case COM_Dig_D: ObjectComDigDouble(this); break;
				case COM_Dig_S: Action.Data = (!Action.Data); break; // Dig mat 2 object request
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_SWIM:
			switch (byCom)
				{
				case COM_Left:  ObjectComMovement(this,COMD_Left); break;
				case COM_Right: ObjectComMovement(this,COMD_Right); break;
				case COM_Up:
					ObjectComMovement(this,COMD_Up);
					ObjectComUp(this); break;
				case COM_Down:  ObjectComMovement(this,COMD_Down); break;
				case COM_Throw: PlayerObjectCommand(Owner,C4CMD_Drop); break;
				case COM_Dig_D:  ObjectComDigDouble(this); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_BRIDGE: case DFA_BUILD: case DFA_CHOP:
			switch (byCom)
				{
				case COM_Down: ObjectComStop(this); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_FIGHT:
			switch (byCom)
				{
				case COM_Left:  ObjectComMovement(this,COMD_Left); break;
				case COM_Right: ObjectComMovement(this,COMD_Right); break;
				case COM_Down:  ObjectComStop(this); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_PUSH:
			{
			// Call object control first in case it overloads
			if (Action.Target)
				if (Action.Target->CallControl(pController, byCom, &C4AulParSet(C4VObj(this))))
					return;
			// Clonk direct control
			switch (byCom)
				{
				case COM_Left:  ObjectComMovement(this,COMD_Left); break;
				case COM_Right: ObjectComMovement(this,COMD_Right); break;
				case COM_Up:
					// Target -> enter
					if (ObjectComEnter(Action.Target))
						ObjectComMovement(this,COMD_Stop);
					// Else, comdir up for target straightening
					else
						ObjectComMovement(this,COMD_Up);
					break;
				case COM_Down: ObjectComMovement(this,COMD_Stop); break;
				case COM_Down_D: ObjectComUnGrab(this); break;
				case COM_Throw: PlayerObjectCommand(Owner,C4CMD_Throw); break;
				}
			break;
			}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
	}

void C4Object::AutoStopDirectCom(BYTE byCom, int32_t iData) // By DirecCom
	{
	C4Player * pPlayer = ::Players.Get(Controller);
	// Control by procedure
	switch (GetProcedure())
		{
		case DFA_WALK:
			switch (byCom)
				{
				case COM_Up:      ObjectComUp(this); break;
				case COM_Down:
					// inhibit controldownsingle on freshly grabbed objects
					if (ObjectComDownDouble(this))
						pPlayer->LastCom = COM_None;
					break;
				case COM_Dig_S:   ObjectComDig(this); break;
				case COM_Dig_D:   ObjectComDigDouble(this); break;
				case COM_Throw:   PlayerObjectCommand(Owner,C4CMD_Throw); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_FLIGHT:
			switch (byCom)
				{
				case COM_Throw:
					// Drop when pressing left, right or down
					if (pPlayer->PressedComs & ((1<<COM_Left)|(1<<COM_Right)|(1<<COM_Down)))
						PlayerObjectCommand(Owner,C4CMD_Drop);
					else
					// This will fail, but whatever.
						PlayerObjectCommand(Owner,C4CMD_Throw);
					break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_KNEEL: case DFA_THROW:
			switch (byCom)
				{
				case COM_Throw:   PlayerObjectCommand(Owner,C4CMD_Throw); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_SCALE:
			switch (byCom)
				{
				case COM_Left:
					if (Action.Dir == DIR_Right) ObjectComLetGo(this,-1);
					else AutoStopUpdateComDir();
					break;
				case COM_Right:
					if (Action.Dir == DIR_Left) ObjectComLetGo(this,+1);
					else AutoStopUpdateComDir();
					break;
				case COM_Dig:    ObjectComLetGo(this,(Action.Dir == DIR_Left) ? +1 : -1);
				case COM_Throw:	 PlayerObjectCommand(Owner,C4CMD_Drop); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_HANGLE:
			switch (byCom)
				{
				case COM_Down:    ObjectComLetGo(this,0); break;
				case COM_Dig:     ObjectComLetGo(this,0); break;
				case COM_Throw:   PlayerObjectCommand(Owner,C4CMD_Drop); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_DIG:
			switch (byCom)
				{
				// Dig mat 2 object request
				case COM_Throw: case COM_Dig: Action.Data = (!Action.Data); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_SWIM:
			switch (byCom)
				{
				case COM_Up:
					AutoStopUpdateComDir();
					ObjectComUp(this);
					break;
				case COM_Throw: PlayerObjectCommand(Owner,C4CMD_Drop); break;
				case COM_Dig_D: ObjectComDigDouble(this); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_BRIDGE: case DFA_BUILD: case DFA_CHOP:
			switch (byCom)
				{
				case COM_Down: ObjectComStop(this); break;
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_FIGHT:
			switch (byCom)
				{
				case COM_Down:  ObjectComStop(this); break;
				default: AutoStopUpdateComDir();
				}
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_PUSH:
			{
			// Call object control first in case it overloads
			if(Action.Target)
				if (Action.Target->CallControl(pPlayer, byCom, &C4AulParSet(C4VObj(this))))
					return;
			// Clonk direct control
			switch (byCom)
				{
				case COM_Down: case COM_Down_D: ObjectComUnGrab(this); break;
				case COM_Throw:  PlayerObjectCommand(Owner,C4CMD_Drop); break;
				case COM_Up:
					// Target -> enter
					if (ObjectComEnter(Action.Target))
						{
						ObjectComMovement(this,COMD_Stop);
						break;
						}
					// Else, comdir up for target straightening
				default:
					AutoStopUpdateComDir();
				}
			break;
			}
		}
	}

void C4Object::AutoStopUpdateComDir()
	{
	C4Player * pPlr = ::Players.Get(Controller);
	if (!pPlr || pPlr->Cursor != this) return;
	int32_t NewComDir = Coms2ComDir(pPlr->PressedComs);
	if (Action.ComDir == NewComDir) return;
	if (NewComDir == COMD_Stop && GetProcedure() == DFA_DIG)
		{
		ObjectComStop(this);
		return;
		}
	ObjectComMovement(this, NewComDir);
	}

bool C4Object::MenuCommand(const char *szCommand)
	{
	// Native script execution
	if (!Def || !Status) return false;
	return !! Def->Script.DirectExec(this, szCommand, "MenuCommand");
	}

C4Object *C4Object::ComposeContents(C4ID id)
	{
	int32_t cnt,cnt2;
	C4ID c_id;
	bool fInsufficient = false;
	C4Object *pObj;
	C4ID idNeeded=C4ID_None;
	int32_t iNeeded=0;
	// Get def
	C4Def *pDef = C4Id2Def(id); if (!pDef) return NULL;
	// get needed contents
	C4IDList NeededComponents;
	pDef->GetComponents(&NeededComponents, NULL, this);
	// Check for sufficient components
  StdStrBuf Needs; Needs.Format(LoadResStr("IDS_CON_BUILDMATNEED"),pDef->GetName());
	for (cnt=0; c_id=NeededComponents.GetID(cnt); cnt++)
		if (NeededComponents.GetCount(cnt) > Contents.ObjectCount(c_id))
			{
			Needs.AppendFormat("|%ix %s", NeededComponents.GetCount(cnt) - Contents.ObjectCount(c_id), C4Id2Def(c_id) ? C4Id2Def(c_id)->GetName() : C4IdText(c_id) );
			if (!idNeeded) { idNeeded=c_id; iNeeded=NeededComponents.GetCount(cnt)-Contents.ObjectCount(c_id); }
			fInsufficient = true;
			}
	// Insufficient
	if (fInsufficient)
		{
		// BuildNeedsMaterial call to object...
		if (!Call(PSF_BuildNeedsMaterial,&C4AulParSet(C4VID(idNeeded), C4VInt(iNeeded))))
			// ...game message if not overloaded
			GameMsgObject(Needs.getData(),this);
		// Return
		return NULL;
		}
	// Remove components
	for (cnt=0; c_id=NeededComponents.GetID(cnt); cnt++)
		for (cnt2=0; cnt2<NeededComponents.GetCount(cnt); cnt2++)
			if (!( pObj = Contents.Find(c_id) ))
				return NULL;
			else
				pObj->AssignRemoval();
	// Create composed object
	// the object is created with default components instead of builder components
	// this is done because some objects (e.g. arrow packs) will set custom components during initialization, which should not be overriden
	return CreateContents(C4Id2Def(id));
	}

void C4Object::SetSolidMask(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY)
	{
	// remove osld
	if (pSolidMaskData) { pSolidMaskData->Remove(true, false); delete pSolidMaskData; pSolidMaskData=NULL; }
	// set new data
	SolidMask.Set(iX,iY,iWdt,iHgt,iTX,iTY);
	// re-put if valid
	if (CheckSolidMaskRect()) UpdateSolidMask(false);
	}

bool C4Object::CheckSolidMaskRect()
	{
	// check NewGfx only, because invalid SolidMask-rects are OK in OldGfx
	// the bounds-check is done in CStdDDraw::GetPixel()
	CSurface *sfcGraphics = GetGraphics()->GetBitmap();
	SolidMask.Set(Max<int32_t>(SolidMask.x,0), Max<int32_t>(SolidMask.y,0),
	              Min<int32_t>(SolidMask.Wdt,sfcGraphics->Wdt-SolidMask.x), Min<int32_t>(SolidMask.Hgt, sfcGraphics->Hgt-SolidMask.y),
	              SolidMask.tx, SolidMask.ty);
	if (SolidMask.Hgt<=0) SolidMask.Wdt=0;
	return SolidMask.Wdt>0;
	}

void C4Object::SyncClearance()
	{
	// Misc. no-save safeties
	Action.t_attach = CNAT_None;
	InMat = MNone;
	t_contact = 0;
	// Fixed point values - precision reduction
	fix_r = itofix( r );
	// Update OCF
	SetOCF();
	// Menu
	CloseMenu(true);
	// Material contents
	if (MaterialContents) delete MaterialContents; MaterialContents=NULL;
	// reset speed of staticback-objects
	if (Category & C4D_StaticBack)
		{
		xdir = ydir = 0;
		}
	}

void C4Object::DrawSelectMark(C4TargetFacet &cgo, float Zoom)
	{
	// Status
	if (!Status) return;
	// No select marks in film playback
	if (Game.C4S.Head.Film && Game.C4S.Head.Replay) return;
	// target pos (parallax)
	float cotx=cgo.TargetX,coty=cgo.TargetY; TargetPos(cotx, coty, cgo);
	// Output boundary
	if (!Inside<float>(fixtof(fix_x) - cotx, 0, cgo.Wdt - 1)
		|| !Inside<float>(fixtof(fix_y) - coty, 0, cgo.Hgt - 1)) return;
	// Draw select marks
	float cox = (fixtof(fix_x) + Shape.GetX() - cotx) * Zoom + cgo.X - 2;
	float coy = (fixtof(fix_y) + Shape.GetY() - coty) * Zoom + cgo.Y - 2;
	GfxR->fctSelectMark.Draw(cgo.Surface,cox,coy,0);
	GfxR->fctSelectMark.Draw(cgo.Surface,cox+Shape.Wdt*Zoom,coy,1);
	GfxR->fctSelectMark.Draw(cgo.Surface,cox,coy+Shape.Hgt*Zoom,2);
	GfxR->fctSelectMark.Draw(cgo.Surface,cox+Shape.Wdt*Zoom,coy+Shape.Hgt*Zoom,3);
	}

void C4Object::ClearCommands()
	{
  C4Command *pNext;
  while (Command)
    {
		pNext=Command->Next;
    if(!Command->iExec)
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
    if (pCom==pUntil) pNext=NULL;
		// Next one to clear after this
    else pNext=pCom->Next;
    Command=pCom->Next;
		if(!pCom->iExec)
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
	//sprintf(OSTR,"%s command %s added: %i/%i %s %s (%i)",GetName(),CommandName(iCommand),iTx,iTy,pTarget ? pTarget->GetName() : "O",pTarget2 ? pTarget2->GetName() : "O",iUpdateInterval); Log(OSTR);
  return true;
	}

void C4Object::SetCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy,
													C4Object *pTarget2, bool fControl, C4Value iData,
													int32_t iRetries, C4String *szText)
	{
  // Decrease NoCollectDelay
  if (NoCollectDelay>0) NoCollectDelay--;
	// Clear stack
  ClearCommands();
	// Close menu
	if (fControl)
		if (!CloseMenu(false)) return;
	// Script overload
	if (fControl)
		if (!!Call(PSF_ControlCommand,&C4AulParSet(C4VString(CommandName(iCommand)),
																							 C4VObj(pTarget),
																							 iTx,
																							 C4VInt(iTy),
																							 C4VObj(pTarget2),
																							 iData)))
			return;
	// Inside vehicle control overload
  if (Contained)
		if (Contained->Def->VehicleControl & C4D_VehicleControl_Inside)
			{
			Contained->Controller=Controller;
			if (!!Contained->Call(PSF_ControlCommand,&C4AulParSet(C4VString(CommandName(iCommand)),
																														C4VObj(pTarget),
																														iTx,
																														C4VInt(iTy),
																														C4VObj(pTarget2),
																														iData,
                                                            C4VObj(this))))
				return;
			}
	// Outside vehicle control overload
	if (GetProcedure()==DFA_PUSH)
		if (Action.Target)  if (Action.Target->Def->VehicleControl & C4D_VehicleControl_Outside)
			{
			Action.Target->Controller=Controller;
			if (!!Action.Target->Call(PSF_ControlCommand,&C4AulParSet(C4VString(CommandName(iCommand)),
																																C4VObj(pTarget),
																																iTx,
																																C4VInt(iTy),
																																C4VObj(pTarget2),
																																iData)))
				return;
			}
	// Add new command
	AddCommand(iCommand,pTarget,iTx,iTy,0,pTarget2,true,iData,false,iRetries,szText,C4CMD_Mode_Base);
	}

C4Command *C4Object::FindCommand(int32_t iCommandType)
	{
	// seek all commands
	for (C4Command *pCom = Command; pCom; pCom=pCom->Next)
		if (pCom->Command == iCommandType) return pCom;
	// nothing found
	return NULL;
	}

bool C4Object::ExecuteCommand()
	{
	// Execute first command
	if (Command) Command->Execute();
	// Command finished: engine call
	if (Command && Command->Finished)
		Call(PSF_ControlCommandFinished,&C4AulParSet(C4VString(CommandName(Command->Command)), C4VObj(Command->Target), Command->Tx, C4VInt(Command->Ty), C4VObj(Command->Target2), Command->Data));
	// Clear finished commands
	while (Command && Command->Finished) ClearCommand(Command);
	// Done
	return true;
	}

void C4Object::AddMaterialContents(int32_t iMaterial, int32_t iAmount)
	{
	// Create contents list if necessary
	if (!MaterialContents) MaterialContents = new C4MaterialList;
	// Add amount
	MaterialContents->Add(iMaterial,iAmount);
	}

void C4Object::DigOutMaterialCast(bool fRequest)
	{
	// Check material contents for sufficient object cast amounts
	if (!MaterialContents) return;
  for (int32_t iMaterial=0; iMaterial< ::MaterialMap.Num; iMaterial++)
    if (MaterialContents->Amount[iMaterial])
      if (::MaterialMap.Map[iMaterial].Dig2Object!=C4ID_None)
        if (::MaterialMap.Map[iMaterial].Dig2ObjectRatio!=0)
          if (fRequest || !::MaterialMap.Map[iMaterial].Dig2ObjectOnRequestOnly)
            if (MaterialContents->Amount[iMaterial]>=::MaterialMap.Map[iMaterial].Dig2ObjectRatio)
              {
              Game.CreateObject(::MaterialMap.Map[iMaterial].Dig2Object,this,NO_OWNER,GetX(), GetY()+Shape.GetY()+Shape.Hgt,Random(360));
              MaterialContents->Amount[iMaterial]=0;
              }
	}

void C4Object::DrawCommand(C4Facet &cgoBar, int32_t iAlign, const char *szFunctionFormat,
													 int32_t iCom, C4RegionList *pRegions, int32_t iPlayer,
													 const char *szDesc, C4Facet *pfctImage)
	{
	const char *cpDesc = szDesc;
	C4ID idDescImage = id;
	C4Def *pDescImageDef=NULL;
	int32_t iDescImagePhase = 0;
	C4Facet cgoLeft,cgoRight;
	bool fFlash=false;

	// Flash
	C4Player *pPlr;
	if (pPlr=::Players.Get(Owner))
		if (iCom==pPlr->FlashCom)
			fFlash=true;

	// Get desc from def script function desc
	if (szFunctionFormat)
		cpDesc = Def->Script.GetControlDesc(szFunctionFormat,iCom,&idDescImage,&iDescImagePhase);

	// Image def by id
	if (idDescImage && idDescImage!=C4ID_Contents)
		pDescImageDef = C4Id2Def(idDescImage);

	// Symbol sections
	cgoRight = cgoBar.TruncateSection(iAlign);
	if (!cgoRight.Wdt) return;
	if (iAlign & C4FCT_Bottom) cgoLeft = cgoRight.TruncateSection(C4FCT_Left);
	else cgoLeft = cgoBar.TruncateSection(iAlign);
	if (!cgoLeft.Wdt) return;

	// Flash background
	//if (fFlash)
	//	Application.DDraw->DrawBox(cgoLeft.Surface,cgoLeft.X-1,cgoLeft.Y-1,cgoLeft.X+cgoLeft.Wdt*2,cgoLeft.Y+cgoLeft.Hgt,CRed);

	// image drawn by caller
	if (pfctImage)
		*pfctImage = cgoRight;
	// Specified def
	else if (pDescImageDef)
		pDescImageDef->Draw(cgoRight, false, Color, NULL, iDescImagePhase); // ...use specified color, but not object.
	// Contents image
	else if (idDescImage == C4ID_Contents)
		{
		// contents object
		C4Object *pContents = Contents.GetObject();
		if (pContents)
			pContents->DrawPicture(cgoRight);
		else
			DrawPicture(cgoRight);
		}
	// Picture
	else
		DrawPicture(cgoRight);

	// Command
	if (!fFlash || ::Game.iTick35>15)
		DrawCommandKey(cgoLeft,iCom,false,
									 Config.Graphics.ShowCommandKeys ? PlrControlKeyName(iPlayer,Com2Control(iCom), true).getData() : NULL );

	// Region (both symbols)
	if (pRegions)
		pRegions->Add(cgoLeft.X,cgoLeft.Y,cgoLeft.Wdt*2,cgoLeft.Hgt,cpDesc ? cpDesc : GetName(),iCom);

	}

void C4Object::Resort()
	{
	// Flag resort
	Unsorted=true;
	Game.fResortAnyObject = true;
	// Must not immediately resort - link change/removal would crash Game::ExecObjects
	}

bool C4Object::SetAction(C4PropList * Act, C4Object *pTarget, C4Object *pTarget2, int32_t iCalls, bool fForce)
	{
	C4PropList * LastAction = Action.pActionDef;
	int32_t iLastPhase=Action.Phase;
	// No other action
	if (LastAction)
		if (LastAction->GetPropertyInt(P_NoOtherAction) && !fForce)
			if (Act != LastAction)
				return false;
	// Stop previous act sound
	if (LastAction)
		if (Act != LastAction)
			if (LastAction->GetPropertyStr(P_Sound))
				StopSoundEffect(LastAction->GetPropertyStr(P_Sound)->GetCStr(),this);
	// Unfullcon objects no action
	if (Con<FullCon)
		if (!Def->IncompleteActivity)
			Act = 0;
	// Reset action time on change
	if (Act!=LastAction)
		{
		Action.Time=0;
		// reset action data if procedure is changed
		if ((Act ? Act->GetPropertyInt(P_Procedure) : DFA_NONE)
			!= (LastAction ? LastAction->GetPropertyInt(P_Procedure) : DFA_NONE))
				Action.Data = 0;
		}
	// Set new action
	Action.pActionDef = Act;
	Action.Phase=Action.PhaseDelay=0;
	// Set target if specified
	if (pTarget) Action.Target=pTarget;
	if (pTarget2) Action.Target2=pTarget2;
	// Set Action Facet
	UpdateActionFace();
	// update flipdir
	if ((LastAction ? LastAction->GetPropertyInt(P_FlipDir) : 0)
	 != (Act ? Act->GetPropertyInt(P_FlipDir) : 0)) UpdateFlipDir();
	// Start act sound
	if (Action.pActionDef)
		if (Action.pActionDef != LastAction)
			if (Action.pActionDef->GetPropertyStr(P_Sound))
				StartSoundEffect(Action.pActionDef->GetPropertyStr(P_Sound)->GetCStr(),+1,100,this);
	// Reset OCF
	SetOCF();
	// issue calls
	// Execute StartCall
	if (iCalls & SAC_StartCall)
		if (Action.pActionDef)
			{
			if (Action.pActionDef->GetPropertyStr(P_StartCall))
				{
				C4Def *pOldDef = Def;
				Call(Action.pActionDef->GetPropertyStr(P_StartCall)->GetCStr());
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
				}
			}
	// Execute EndCall
	if (iCalls & SAC_EndCall && !fForce)
		if (LastAction)
			{
			if (LastAction->GetPropertyStr(P_EndCall))
				{
				C4Def *pOldDef = Def;
				Call(LastAction->GetPropertyStr(P_EndCall)->GetCStr());
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
				}
			}
	// Execute AbortCall
	if (iCalls & SAC_AbortCall && !fForce)
		if (LastAction)
			{
			if (LastAction->GetPropertyStr(P_AbortCall))
				{
				C4Def *pOldDef = Def;
				Call(LastAction->GetPropertyStr(P_AbortCall)->GetCStr(), &C4AulParSet(C4VInt(iLastPhase)));
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
				}
			}
	return true;
	}

void C4Object::UpdateActionFace()
	{
	// Default: no action face
	Action.Facet.Default();
	// Active: get action facet from action definition
	if (Action.pActionDef)
		{
		if (Action.pActionDef->GetPropertyInt(P_Wdt)>0)
			{
			Action.Facet.Set(GetGraphics()->GetBitmap(Color),
				Action.pActionDef->GetPropertyInt(P_X),Action.pActionDef->GetPropertyInt(P_Y),
				Action.pActionDef->GetPropertyInt(P_Wdt),Action.pActionDef->GetPropertyInt(P_Hgt));
			Action.FacetX=Action.pActionDef->GetPropertyInt(P_OffX);
			Action.FacetY=Action.pActionDef->GetPropertyInt(P_OffY);
			}
		}
	}

bool C4Object::SetActionByName(C4String * ActName,
															 C4Object *pTarget, C4Object *pTarget2,
															 int32_t iCalls, bool fForce)
	{
	int32_t cnt;
	// Check for ActIdle passed by name
	if (ActName == Strings.P[P_Idle])
		return SetAction(0,0,0,iCalls,fForce);
	C4Value ActMap; GetProperty(Strings.P[P_ActMap], ActMap);
	if (!ActMap.getPropList()) return false;
	C4Value Action; ActMap.getPropList()->GetProperty(ActName, Action);
	if (!Action.getPropList()) return false;
	return SetAction(Action.getPropList(),pTarget,pTarget2,iCalls,fForce);      
	}

bool C4Object::SetActionByName(const char * szActName, 
															 C4Object *pTarget, C4Object *pTarget2, 
															 int32_t iCalls, bool fForce)
	{
	C4String * ActName = Strings.RegString(szActName);
	ActName->IncRef();
	bool r = SetActionByName(ActName, pTarget, pTarget2, iCalls, fForce);
	ActName->DecRef();
	return r;
	}

void C4Object::SetDir(int32_t iDir)
  {
	// Not active
	if (!Action.pActionDef) return;
	// Invalid direction
	if (!Inside<int32_t>(iDir,0,Action.pActionDef->GetPropertyInt(P_Directions)-1)) return;
	// Execute turn action
	if (iDir != Action.Dir)
		if (Action.pActionDef->GetPropertyStr(P_TurnAction))
			{ SetActionByName(Action.pActionDef->GetPropertyStr(P_TurnAction)); }
	// Set dir
	Action.Dir=iDir;
	// update by flipdir?
	if (Action.pActionDef->GetPropertyInt(P_FlipDir))
		UpdateFlipDir();
	else
		Action.DrawDir=iDir;
  }

int32_t C4Object::GetProcedure()
	{
	if (!Action.pActionDef) return DFA_NONE;
	return Action.pActionDef->GetPropertyInt(P_Procedure);
	}

void GrabLost(C4Object *cObj)
	{
	// Grab lost script call on target (quite hacky stuff...)
	cObj->Action.Target->Call(PSF_GrabLost);
	// Clear commands down to first PushTo (if any) in command stack
	for (C4Command *pCom=cObj->Command; pCom; pCom=pCom->Next)
		if (pCom->Next && pCom->Next->Command==C4CMD_PushTo)
			{
			cObj->ClearCommand(pCom);
			break;
			}
	}

void DoGravity(C4Object *cobj, bool fFloatFriction=true);

void C4Object::NoAttachAction()
	{
	// Active objects
	if (Action.pActionDef)
		{
		int32_t iProcedure = GetProcedure();
		// Scaling upwards: corner scale
		if (iProcedure == DFA_SCALE && Action.ComDir != COMD_Stop && ComDirLike(Action.ComDir, COMD_Up))
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Left && Action.Dir == DIR_Left)
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Right && Action.Dir == DIR_Right)
			if (ObjectActionCornerScale(this)) return;
		// Scaling and stopped: fall off to side (avoid zuppel)
		if ((iProcedure == DFA_SCALE) && (Action.ComDir == COMD_Stop))
			if (Action.Dir == DIR_Left)
				{ if (ObjectActionJump(this,itofix(1),Fix0,false)) return; }
			else
				{ if (ObjectActionJump(this,itofix(-1),Fix0,false)) return; }
		// Pushing: grab loss
		if (iProcedure==DFA_PUSH) GrabLost(this);
		// Else jump
		ObjectActionJump(this,xdir,ydir,false);
		}
	// Inactive objects, simple mobile natural gravity
	else
		{
		DoGravity(this);
		Mobile=1;
		}
  }

bool ContactVtxCNAT(C4Object *cobj, BYTE cnat_dir);

void C4Object::ContactAction()
	{
	// Take certain action on contact. Evaluate t_contact-CNAT and Procedure.
	FIXED last_xdir;

	int32_t iDir;
	C4PhysicalInfo *pPhysical=GetPhysical();

	// Determine Procedure
	if (!Action.pActionDef) return;
	int32_t iProcedure=Action.pActionDef->GetPropertyInt(P_Procedure);
	int32_t fDisabled=Action.pActionDef->GetPropertyInt(P_ObjectDisabled);

	//------------------------------- Hit Bottom ---------------------------------------------
	if (t_contact & CNAT_Bottom)
		switch (iProcedure)
			{
			case DFA_FLIGHT:
				if(ydir < 0) break;
				// Jump: FlatHit / HardHit / Walk
				if ((OCF & OCF_HitSpeed4) || fDisabled)
					if (ObjectActionFlat(this,Action.Dir)) return;
				if (OCF & OCF_HitSpeed3)
					if (ObjectActionKneel(this)) return;
				// Walk, but keep horizontal momentum (momentum is reset
				// by ObjectActionWalk) to avoid walk-jump-flipflop on
				// sideways corner hit if initial walk acceleration is
				// not enough to reach the next pixel for attachment.
				// Urks, all those special cases...
				last_xdir=xdir;
				ObjectActionWalk(this);
				xdir=last_xdir;
				return;
			case DFA_SCALE:
				// Scale up: try corner scale
				if (!ComDirLike(Action.ComDir, COMD_Down))
					{
					if (ObjectActionCornerScale(this)) return;
					return;
					}
				// Any other: Stand
				ObjectActionStand(this);
				return;
			case DFA_DIG:
				// Redirect downleft/downright
				if (Action.ComDir==COMD_DownLeft)
					{ Action.ComDir=COMD_Left; break; }
				if (Action.ComDir==COMD_DownRight)
					{ Action.ComDir=COMD_Right; break; }
				// Else stop
				ObjectComStopDig(this);
				return;
			case DFA_SWIM:
				// Try corner scale out
				if (!GBackLiquid(GetX(), GetY() - 1))
					if (ObjectActionCornerScale(this)) return;
				return;
			}

	//------------------------------- Hit Ceiling -----------------------------------------
	if (t_contact & CNAT_Top)
		switch (iProcedure)
		{
			case DFA_WALK:
				// Walk: Stop
				ObjectActionStand(this); return;
			case DFA_SCALE:
				// Scale: Try hangle, else stop if going upward
				if (ComDirLike(Action.ComDir, COMD_Up))
					{
					if (pPhysical->CanHangle)
						{
						iDir=DIR_Left;
						if (Action.Dir==DIR_Left) { iDir=DIR_Right; }
						ObjectActionHangle(this,iDir); return;
						}
					else
						Action.ComDir=COMD_Stop;
					}
				break;
			case DFA_FLIGHT:
				// Jump: Try hangle, else bounce off
				// High Speed Flight: Tumble
				if ((OCF & OCF_HitSpeed3) || fDisabled)
					{ ObjectActionTumble(this,Action.Dir,Fix0,Fix0); break; }
				if (pPhysical->CanHangle)
					{ ObjectActionHangle(this,Action.Dir); return; }
				break;
			case DFA_DIG:
				// Dig: Stop
				ObjectComStopDig(this); return;
			case DFA_HANGLE:
				Action.ComDir=COMD_Stop;
				break;
		}

	//---------------------------- Hit Left Wall ----------------------------------------
	if (t_contact & CNAT_Left)
		{
		switch (iProcedure)
			{
			case DFA_FLIGHT:
				// High Speed Flight: Tumble
				if ((OCF & OCF_HitSpeed3) || fDisabled)
					{ ObjectActionTumble(this,DIR_Left,FIXED100(+150),Fix0); break; }
				// Else
				else if (pPhysical->CanScale)
					{ ObjectActionScale(this,DIR_Left); return; }
				break;
			case DFA_WALK:
				// Walk: Try scale, else stop
				if (ComDirLike(Action.ComDir, COMD_Left))
					{
					if (pPhysical->CanScale)
						{ ObjectActionScale(this,DIR_Left); return; }
					// Else stop
					Action.ComDir=COMD_Stop;
					}
				// Heading away from solid
				if (ComDirLike(Action.ComDir, COMD_Right))
					{
					// Slide off
					ObjectActionJump(this,xdir/2,ydir,false);
					}
				return;
			case DFA_SWIM:
				// Try scale
				if (ComDirLike(Action.ComDir, COMD_Left))
					if (pPhysical->CanScale)
						{ ObjectActionScale(this,DIR_Left); return; }
				// Try corner scale out
				if (ObjectActionCornerScale(this)) return;
				return;
			case DFA_HANGLE:
				// Hangle: Try scale, else stop
				if (pPhysical->CanScale)
					if (ObjectActionScale(this,DIR_Left))
						return;
				Action.ComDir=COMD_Stop;
				return;
			case DFA_DIG:
				// Dig: Stop
				ObjectComStopDig(this);
				return;
			}
		}

	//------------------------------ Hit Right Wall --------------------------------------
	if (t_contact & CNAT_Right)
		{
		switch (iProcedure)
			{
			case DFA_FLIGHT:
				// High Speed Flight: Tumble
				if ((OCF & OCF_HitSpeed3) || fDisabled)
					{ ObjectActionTumble(this,DIR_Right,FIXED100(-150),Fix0); break; }
				// Else Scale
				else if (pPhysical->CanScale)
					{ ObjectActionScale(this,DIR_Right); return; }
				break;
			case DFA_WALK:
				// Walk: Try scale, else stop
				if (ComDirLike(Action.ComDir, COMD_Right))
					{
					if (pPhysical->CanScale)
						{ ObjectActionScale(this,DIR_Right); return; }
					Action.ComDir=COMD_Stop;
					}
				// Heading away from solid
				if (ComDirLike(Action.ComDir, COMD_Left))
					{
					// Slide off
					ObjectActionJump(this,xdir/2,ydir,false);
					}
				return;
			case DFA_SWIM:
				// Try scale
				if (ComDirLike(Action.ComDir, COMD_Right))
					if (pPhysical->CanScale)
						{ ObjectActionScale(this,DIR_Right); return; }
				// Try corner scale out
				if (ObjectActionCornerScale(this)) return;
				// Skip to enable walk out
				return;
			case DFA_HANGLE:
				// Hangle: Try scale, else stop
				if (pPhysical->CanScale)
					if (ObjectActionScale(this,DIR_Right))
						return;
				Action.ComDir=COMD_Stop;
				return;
			case DFA_DIG:
				// Dig: Stop
				ObjectComStopDig(this);
				return;
			}
		}

	//---------------------------- Unresolved Cases ---------------------------------------

	// Flight stuck
	if (iProcedure==DFA_FLIGHT)
		{
		// Enforce slide free (might slide through tiny holes this way)
		if (!ydir)
			{
			bool fAllowDown = !(t_contact & CNAT_Bottom);
			if (t_contact & CNAT_Right)
				{
				ForcePosition(GetX() - 1, GetY() + fAllowDown);
				xdir=ydir=0;
				}
			if (t_contact & CNAT_Left)
				{
				ForcePosition(GetX() + 1, GetY() + fAllowDown);
				xdir=ydir=0;
				}
			}
		if (!xdir)
			{
			if (t_contact & CNAT_Top)
				{
				ForcePosition(GetX(), GetY() + 1);
				xdir=ydir=0;
				}
			}
		}
	}

void Towards(FIXED &val, FIXED target, FIXED step)
  {
  if (val==target) return;
  if (Abs(val-target)<=step) { val=target; return; }
  if (val<target) val+=step; else val-=step;
  }

bool DoBridge(C4Object *clk)
  {
	int32_t iBridgeTime; bool fMoveClonk, fWall; int32_t iBridgeMaterial;
	clk->Action.GetBridgeData(iBridgeTime, fMoveClonk, fWall, iBridgeMaterial);
	if (!iBridgeTime) iBridgeTime = 100; // default bridge time
	if (clk->Action.Time>=iBridgeTime) { ObjectActionStand(clk); return false; }
	// get bridge advancement
	int32_t dtp;
	if (fWall) switch (clk->Action.ComDir)
		{
		case COMD_Left: case COMD_Right: dtp = 4; fMoveClonk = false; break; // vertical wall: default 25 pixels
		case COMD_UpLeft: case COMD_UpRight: dtp = 5; fMoveClonk = false; break; // diagonal roof over Clonk: default 20 pixels up and 20 pixels side (28 pixels - optimized to close tunnels completely)
		case COMD_Up: dtp = 5; break; // horizontal roof over Clonk
		default: return true; // bridge procedure just for show
		}
	else switch (clk->Action.ComDir)
		{
		case COMD_Left: case COMD_Right: dtp = 5; break; // horizontal bridges: default 20 pixels
		case COMD_Up: dtp = 4; break; // vertical bridges: default 25 pixels (same as
		case COMD_UpLeft: case COMD_UpRight: dtp = 6; break; // diagonal bridges: default 16 pixels up and 16 pixels side (23 pixels)
		default: return true; // bridge procedure  just for show
		}
	if (clk->Action.Time % dtp) return true; // no advancement in this frame
	// get target pos for Clonk and bridge
	int32_t cx=clk->GetX(), cy=clk->GetY(), cw=clk->Shape.Wdt, ch=clk->Shape.Hgt;
  int32_t tx=cx,ty=cy+ch/2;
	int32_t dt;
	if (fMoveClonk) dt = 0; else dt = clk->Action.Time / dtp;
	if (fWall) switch (clk->Action.ComDir)
		{
		case COMD_Left: tx-=cw/2; ty+=-dt; break;
		case COMD_Right: tx+=cw/2; ty+=-dt; break;
		case COMD_Up:
			{
			int32_t x0;
			if (fMoveClonk) x0=-3; else x0=(iBridgeTime/dtp)/-2;
			tx+=(x0+dt)*((clk->Action.Dir==DIR_Right)*2-1); cx+=((clk->Action.Dir==DIR_Right)*2-1); ty-=ch+3; break;
			}
		case COMD_UpLeft: tx-=-4+dt; ty+=-ch-7+dt; break;
		case COMD_UpRight: tx+=-4+dt; ty+=-ch-7+dt; break;
		}
	else switch (clk->Action.ComDir)
		{
		case COMD_Left: tx+=-3-dt; --cx; break;
		case COMD_Right: tx+=+2+dt; ++cx; break;
		case COMD_Up: tx+=(-cw/2+(cw-1)*(clk->Action.Dir==DIR_Right))*(!fMoveClonk); ty+=-dt-fMoveClonk; --cy; break;
		case COMD_UpLeft: tx+=-5-dt+fMoveClonk*3; ty+=2-dt-fMoveClonk*3; --cx; --cy; break;
		case COMD_UpRight: tx+=+5+dt-fMoveClonk*2; ty+=2-dt-fMoveClonk*3; ++cx; --cy; break;
		}
	// check if Clonk movement is posible
	if (fMoveClonk)
		{
		int32_t cx2=cx, cy2=cy;
		if (/*!clk->Shape.Attach(cx2, cy2, (clk->Action.t_attach & CNAT_Flags) | CNAT_Bottom) ||*/ clk->Shape.CheckContact(cx2, cy2-1))
			{
			// Clonk would collide here: Change to nonmoving Clonk mode and redo bridging
			iBridgeTime -= clk->Action.Time;
			clk->Action.Time = 0;
			if (fWall && clk->Action.ComDir==COMD_Up)
				{
				// special for roof above Clonk: The nonmoving roof is started at bridgelength before the Clonkl
				// so, when interrupted, an action time halfway through the action must be set
				clk->Action.Time = iBridgeTime;
				iBridgeTime += iBridgeTime;
				}
			clk->Action.SetBridgeData(iBridgeTime, false, fWall, iBridgeMaterial);
			return DoBridge(clk);
			}
		}
	// draw bridge into landscape
  ::Landscape.DrawMaterialRect(iBridgeMaterial,tx-2,ty,4,3);
	// Move Clonk
	if (fMoveClonk) clk->MovePosition(cx-clk->GetX(), cy-clk->GetY());
	return true;
  }

void DoGravity(C4Object *cobj, bool fFloatFriction)
  {
	// Floatation in liquids
  if (cobj->InLiquid && cobj->Def->Float)
    {
    cobj->ydir-=FloatAccel;
    if (cobj->ydir<FloatAccel*-10) cobj->ydir=FloatAccel*-10;
    if (fFloatFriction)
      {
      if (cobj->xdir<-FloatFriction) cobj->xdir+=FloatFriction;
      if (cobj->xdir>+FloatFriction) cobj->xdir-=FloatFriction;
      if (cobj->rdir<-FloatFriction) cobj->rdir+=FloatFriction;
      if (cobj->rdir>+FloatFriction) cobj->rdir-=FloatFriction;
      }
    if (!GBackLiquid(cobj->GetX(),cobj->GetY()-1+ cobj->Def->Float*cobj->GetCon()/FullCon -1 ))
      if (cobj->ydir<0) cobj->ydir=0;
    }
	// Free fall gravity
  else if (~cobj->Category & C4D_StaticBack)
    cobj->ydir+=GravAccel;
  }

void StopActionDelayCommand(C4Object *cobj)
  {
  ObjectComStop(cobj);
  cobj->AddCommand(C4CMD_Wait,NULL,0,0,50);
  }

bool ReduceLineSegments(C4Shape &rShape, bool fAlternate)
	{
	// try if line could go by a path directly when skipping on evertex. If fAlternate is true, try by skipping two vertices
	for (int32_t cnt=0; cnt+2+fAlternate<rShape.VtxNum; cnt++)
		if (PathFree(rShape.VtxX[cnt],rShape.VtxY[cnt],
								 rShape.VtxX[cnt+2+fAlternate],rShape.VtxY[cnt+2+fAlternate]))
			{
			if (fAlternate) rShape.RemoveVertex(cnt+2);
			rShape.RemoveVertex(cnt+1);
			return true;
			}
	return false;
	}

void C4Object::ExecAction()
  {
  Action.t_attach=CNAT_None;
  DWORD ocf;
	FIXED iTXDir;
	FIXED lftspeed,tydir;
	int32_t iTargetX;
	int32_t iPushRange,iPushDistance;

	// Standard phase advance
  int32_t iPhaseAdvance=1;

  // Upright attachment check
  if (!Mobile)
    if (Def->UprightAttach)
      if (Inside<int32_t>(r,-StableRange,+StableRange))
        {
        Action.t_attach|=Def->UprightAttach;
        Mobile=1;
        }

	// Idle objects do natural gravity only
	if (!Action.pActionDef)
		{
		if (Mobile) DoGravity(this);
		return;
		}
  
	// No IncompleteActivity? Reset action
	if (!(OCF & OCF_FullCon) && !Def->IncompleteActivity)
		{ SetAction(0); return; }

	// Determine ActDef & Physical Info
	C4PropList * pAction = Action.pActionDef;
	C4PhysicalInfo *pPhysical=GetPhysical();
	FIXED lLimit;
	FIXED fWalk,fMove;
	int32_t smpx,smpy;

	// Energy usage
	if (Game.Rules & C4RULE_StructuresNeedEnergy)
		if (pAction->GetPropertyInt(P_EnergyUsage))
			if (pAction->GetPropertyInt(P_EnergyUsage) <= Energy ) 
				{
				Energy -= pAction->GetPropertyInt(P_EnergyUsage); 
				// No general DoEnergy-Process
				NeedEnergy=0;
				}
			// Insufficient energy for action: same as idle
			else
				{
				NeedEnergy=1;
				if (Mobile) DoGravity(this);
				return;
				}

	// Action time advance
	Action.Time++;
  
	// InLiquidAction check
	if (InLiquid)
		if (pAction->GetPropertyStr(P_InLiquidAction))
			{ SetActionByName(pAction->GetPropertyStr(P_InLiquidAction)); return; }

	// assign extra action attachment (CNAT_MultiAttach)
	// regular attachment values cannot be set for backwards compatibility reasons
	// this parameter had always been ignored for actions using an internal procedure,
	// but is for some obscure reasons set in the KneelDown-actions of the golems
	Action.t_attach |= (pAction->GetPropertyInt(P_Attach) & CNAT_MultiAttach);

	// if an object is in controllable state, so it can be assumed that if it dies later because of NO_OWNER's cause,
	// it has been its own fault and not the fault of the last one who threw a flint on it
	// do not reset for burning objects to make sure the killer is set correctly if they fall out of the map while burning
	if (!pAction->GetPropertyInt(P_ObjectDisabled) && pAction->GetPropertyInt(P_Procedure) != DFA_FLIGHT && !OnFire)
		LastEnergyLossCausePlayer = NO_OWNER;

	// Handle Default Action Procedure: evaluates Procedure and Action.ComDir
	// Update xdir,ydir,Action.Dir,attachment,iPhaseAdvance
	switch (pAction->GetPropertyInt(P_Procedure))
		{
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_WALK:
      lLimit=ValByPhysical(280, pPhysical->Walk);
      switch (Action.ComDir)
        {
        case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
          xdir-=WalkAccel; if (xdir<-lLimit) xdir=-lLimit;
          break;
        case COMD_Right: case COMD_UpRight: case COMD_DownRight:
          xdir+=WalkAccel; if (xdir>+lLimit) xdir=+lLimit;
          break;
        case COMD_Stop: case COMD_Up: case COMD_Down:
          if (xdir<0) xdir+=WalkAccel;
          if (xdir>0) xdir-=WalkAccel;
          if ((xdir>-WalkAccel) && (xdir<+WalkAccel)) xdir=0;
          break;
        }
      iPhaseAdvance=0;
      if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
      if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
      Action.t_attach|=CNAT_Bottom;
      Mobile=1;
			// object is rotateable? adjust to ground, if in horizontal movement or not attached to the center vertex
			if (Def->Rotateable && Shape.AttachMat != MNone && (!!xdir || Def->Shape.VtxX[Shape.iAttachVtx]))
				AdjustWalkRotation(20, 20, 100);
			else
				rdir=0;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_KNEEL:
      ydir=0;
      Action.t_attach|=CNAT_Bottom;
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_SCALE:
			{
			lLimit=ValByPhysical(200, pPhysical->Scale);

			// Physical training
			if (!::Game.iTick5)
				if (Abs(ydir)==lLimit)
					TrainPhysical(&C4PhysicalInfo::Scale, 1, C4MaxPhysical);
			int ComDir = Action.ComDir;
			if (Action.Dir == DIR_Left && ComDir == COMD_Left)
				ComDir = COMD_Up;
			else if (Action.Dir == DIR_Right && ComDir == COMD_Right)
				ComDir = COMD_Up;
			switch (ComDir)
				{
				case COMD_Up: case COMD_UpRight:  case COMD_UpLeft:
					ydir-=WalkAccel; if (ydir<-lLimit) ydir=-lLimit; break;
				case COMD_Down: case COMD_DownRight: case COMD_DownLeft:
					ydir+=WalkAccel; if (ydir>+lLimit) ydir=+lLimit; break;
				case COMD_Left: case COMD_Right: case COMD_Stop:
					if (ydir<0) ydir+=WalkAccel;
					if (ydir>0) ydir-=WalkAccel;
					if ((ydir>-WalkAccel) && (ydir<+WalkAccel)) ydir=0;
					break;
				}
			iPhaseAdvance=0;
			if (ydir<0) iPhaseAdvance=-fixtoi(ydir*14);
			if (ydir>0) iPhaseAdvance=+fixtoi(ydir*14);
			xdir=0;
			if (Action.Dir==DIR_Left)  Action.t_attach|=CNAT_Left;
			if (Action.Dir==DIR_Right) Action.t_attach|=CNAT_Right;
			Mobile=1;
			break;
			}
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_HANGLE:

			lLimit=ValByPhysical(160, pPhysical->Hangle);

			// Physical training
			if (!::Game.iTick5)
				if (Abs(xdir)==lLimit)
					TrainPhysical(&C4PhysicalInfo::Hangle, 1, C4MaxPhysical);

			switch (Action.ComDir)
				{
				case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
					xdir-=WalkAccel; if (xdir<-lLimit) xdir=-lLimit;
					break;
				case COMD_Right: case COMD_UpRight: case COMD_DownRight:
					xdir+=WalkAccel; if (xdir>+lLimit) xdir=+lLimit;
					break;
				case COMD_Up:
					xdir += (Action.Dir == DIR_Left) ? -WalkAccel : WalkAccel;
					if (xdir<-lLimit) xdir=-lLimit;
					if (xdir>+lLimit) xdir=+lLimit;
					break;
				case COMD_Stop: case COMD_Down:
					if (xdir<0) xdir+=WalkAccel;
					if (xdir>0) xdir-=WalkAccel;
					if ((xdir>-WalkAccel) && (xdir<+WalkAccel)) xdir=0;
					break;
				}
			iPhaseAdvance=0;
			if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left); }
			if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
			ydir=0;
			Action.t_attach|=CNAT_Top;
			Mobile=1;
			break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_FLIGHT:
			// Contained: fall out (one try only)
      if (!::Game.iTick10)
        if (Contained)
					{
					StopActionDelayCommand(this);
					SetCommand(C4CMD_Exit);
					}
			// Gravity/mobile
      DoGravity(this);
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_DIG:
			smpx=GetX(); smpy=GetY();
			if (!Shape.Attach(smpx,smpy,CNAT_Bottom))
				{ ObjectComStopDig(this); return; }
			lLimit=ValByPhysical(125, pPhysical->Dig);
			iPhaseAdvance=fixtoi(lLimit*40);
			switch (Action.ComDir)
				{
				case COMD_Up:
					ydir=-lLimit/2;
					if (Action.Dir==DIR_Left) xdir=-lLimit;
					else xdir=+lLimit;
					break;
				case COMD_UpLeft: xdir=-lLimit; ydir=-lLimit/2;		break;
				case COMD_Left:  xdir=-lLimit; ydir=0;						break;
				case COMD_DownLeft: xdir=-lLimit; ydir=+lLimit;   break;
				case COMD_Down:  xdir=0; ydir=+lLimit;						break;
				case COMD_DownRight: xdir=+lLimit; ydir=+lLimit;  break;
				case COMD_Right: xdir=+lLimit; ydir=0;						break;
				case COMD_UpRight: xdir=+lLimit; ydir=-lLimit/2;	break;
				case COMD_Stop:
					xdir=0; ydir=0;
					iPhaseAdvance=0;
					break;
				}
			if (xdir < 0) SetDir(DIR_Left); else if (xdir > 0) SetDir(DIR_Right);
			Action.t_attach=CNAT_None;
			Mobile=1;
			break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_SWIM:
      lLimit=ValByPhysical(160, pPhysical->Swim);

      // Physical training
			if (!::Game.iTick10)
				if (Abs(xdir)==lLimit)
					TrainPhysical(&C4PhysicalInfo::Swim, 1, C4MaxPhysical);

      // ComDir changes xdir/ydir
      switch (Action.ComDir)
        {
        case COMD_Up:				ydir-=SwimAccel; break;
        case COMD_UpRight:	ydir-=SwimAccel;	xdir+=SwimAccel; break;
        case COMD_Right:											xdir+=SwimAccel; break;
        case COMD_DownRight:ydir+=SwimAccel;	xdir+=SwimAccel; break;
        case COMD_Down:			ydir+=SwimAccel; break;
        case COMD_DownLeft:	ydir+=SwimAccel;	xdir-=SwimAccel; break;
        case COMD_Left:												xdir-=SwimAccel; break;
        case COMD_UpLeft:		ydir-=SwimAccel;	xdir-=SwimAccel; break;
        case COMD_Stop:
          if (xdir<0) xdir+=SwimAccel;
          if (xdir>0) xdir-=SwimAccel;
          if ((xdir>-SwimAccel) && (xdir<+SwimAccel)) xdir=0;
          if (ydir<0) ydir+=SwimAccel;
          if (ydir>0) ydir-=SwimAccel;
          if ((ydir>-SwimAccel) && (ydir<+SwimAccel)) ydir=0;
					break;
        }

			// Out of liquid check
			if (!InLiquid)
        {
				// Just above liquid: move down
				if (GBackLiquid(GetX(),GetY()+1+Def->Float*Con/FullCon-1)) ydir=+SwimAccel;
				// Free fall: walk
				else { ObjectActionWalk(this); return; }
				}

			// xdir/ydir bounds
      if (ydir<-lLimit) ydir=-lLimit; if (ydir>+lLimit) ydir=+lLimit;
      if (xdir>+lLimit) xdir=+lLimit; if (xdir<-lLimit) xdir=-lLimit;
			// Surface dir bound
			if (!GBackLiquid(GetX(),GetY()-1+Def->Float*Con/FullCon-1)) if (ydir<0) ydir=0;
      // Dir, Phase, Attach
      if (xdir<0) SetDir(DIR_Left);
      if (xdir>0) SetDir(DIR_Right);
      iPhaseAdvance=fixtoi(lLimit*10);
      Action.t_attach=CNAT_None;
      Mobile=1;

      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_THROW:
      ydir=0; xdir=0;
      Action.t_attach|=CNAT_Bottom;
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_BRIDGE:
			{
			if (!DoBridge(this)) return;
      switch (Action.ComDir)
        {
        case COMD_Left:  case COMD_UpLeft: SetDir(DIR_Left); break;
        case COMD_Right: case COMD_UpRight: SetDir(DIR_Right); break;
        }
      ydir=0; xdir=0;
      Action.t_attach|=CNAT_Bottom;
      Mobile=1;
			}
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_BUILD:
			// Woa, structures can build without target
			if ((Category & C4D_Structure) || (Category & C4D_StaticBack))
	      if (!Action.Target) break;
      // No target
      if (!Action.Target) { ObjectComStop(this); return; }
			// Target internal: container needs to support by own DFA_BUILD
			if (Action.Target->Contained)
				if ( (Action.Target->Contained->GetProcedure()!=DFA_BUILD)
					|| (Action.Target->Contained->NeedEnergy) )
					return;
      // Build speed
      int32_t iLevel;
			// Clonk-standard
			iLevel=10;
			// Internal builds slower
      if (Action.Target->Contained) iLevel=1;
			// Out of target area: stop
			if ( !Inside<int32_t>(GetX()-(Action.Target->GetX()+Action.Target->Shape.GetX()),0,Action.Target->Shape.Wdt)
				|| !Inside<int32_t>(GetY()-(Action.Target->GetY()+Action.Target->Shape.GetY()),-16,Action.Target->Shape.Hgt+16) )
					{ ObjectComStop(this); return; }
      // Build target
      if (!Action.Target->Build(iLevel,this))
        {
        // Cannot build because target is complete (or removed, ugh): we're done
        if (!Action.Target || Action.Target->Con>=FullCon)
          {
          // Stop
          ObjectComStop(this);
          // Exit target if internal
          if (Action.Target) if (Action.Target->Contained==this)
            Action.Target->SetCommand(C4CMD_Exit);
          }
				// Cannot build because target needs material (assumeably)
        else
					{
					// Stop
					ObjectComStop(this);
					}
        return;
        }
      xdir=ydir=0;
      Action.t_attach|=CNAT_Bottom;
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_PUSH:
      // No target
      if (!Action.Target) { StopActionDelayCommand(this); return; }
      // Inside target
      if (Contained==Action.Target) { StopActionDelayCommand(this); return; }
      // Target pushing force
      bool fStraighten;
      iTXDir=0; fStraighten=false;
      lLimit=ValByPhysical(280, pPhysical->Walk);
      switch (Action.ComDir)
        {
        case COMD_Left: case COMD_DownLeft:   iTXDir=-lLimit; break;
        case COMD_UpLeft:  fStraighten=1;     iTXDir=-lLimit; break;
        case COMD_Right: case COMD_DownRight: iTXDir=+lLimit; break;
        case COMD_UpRight: fStraighten=1;     iTXDir=+lLimit; break;
        case COMD_Up:      fStraighten=1; break;
        case COMD_Stop: case COMD_Down:       iTXDir=0;       break;
        }
      // Push object
      if (!Action.Target->Push(iTXDir,ValByPhysical(250, pPhysical->Push),fStraighten))
        { StopActionDelayCommand(this); return; }
			// Set target controller
			Action.Target->Controller=Controller;
      // ObjectAction got hold check
			iPushDistance = Max(Shape.Wdt/2-8,0);
			iPushRange = iPushDistance + 10;
      int32_t sax,say,sawdt,sahgt;
      Action.Target->GetArea(sax,say,sawdt,sahgt);
			// Object lost
      if (!Inside(GetX()-sax,-iPushRange,sawdt-1+iPushRange)
        || !Inside(GetY()-say,-iPushRange,sahgt-1+iPushRange))
          {
					// Wait command (why, anyway?)
					StopActionDelayCommand(this);
					// Grab lost action
					GrabLost(this);
					// Done
					return;
					}
      // Follow object (full xdir reset)
			// Vertical follow: If object moves out at top, assume it's being pushed upwards and the Clonk must run after it
			if (GetY()-iPushDistance > say+sahgt && iTXDir) { if (iTXDir>0) sax+=sawdt/2; sawdt/=2; }
			// Horizontal follow
      iTargetX=BoundBy(GetX(),sax-iPushDistance,sax+sawdt-1+iPushDistance);
      if (GetX()==iTargetX) xdir=0;
      else { if (GetX()<iTargetX) xdir=+lLimit; if (GetX()>iTargetX) xdir=-lLimit; }
      // Phase by XDir
      if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
      if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
			// No YDir
      ydir=0;
			// Attachment
      Action.t_attach|=CNAT_Bottom;
			// Mobile
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_PULL:
      // No target
      if (!Action.Target) { StopActionDelayCommand(this); return; }
      // Inside target
      if (Contained==Action.Target) { StopActionDelayCommand(this); return; }
      // Target contained
      if (Action.Target->Contained) { StopActionDelayCommand(this); return; }

			int32_t iPullDistance;
			int32_t iPullX;

			iPullDistance = Action.Target->Shape.Wdt/2 + Shape.Wdt/2;

			iTargetX=GetX();
			if (Action.ComDir==COMD_Right) iTargetX = Action.Target->GetX()+iPullDistance;
			if (Action.ComDir==COMD_Left) iTargetX = Action.Target->GetX()-iPullDistance;

			iPullX=Action.Target->GetX();
			if (Action.ComDir==COMD_Right) iPullX = GetX()-iPullDistance;
			if (Action.ComDir==COMD_Left) iPullX = GetX()+iPullDistance;

			fWalk = ValByPhysical(280, pPhysical->Walk);

			fMove = 0;
			if (Action.ComDir==COMD_Right) fMove = +fWalk;
			if (Action.ComDir==COMD_Left) fMove = -fWalk;

			iTXDir = fMove + fWalk * BoundBy<int32_t>(iPullX-Action.Target->GetX(),-10,+10) / 10;

      // Push object
			if (!Action.Target->Push(iTXDir,ValByPhysical(250, pPhysical->Push),false))
				{ StopActionDelayCommand(this); return; }
			// Set target controller
			Action.Target->Controller=Controller;

			// Train pulling: com dir transfer
			if ( (Action.Target->GetProcedure()==DFA_WALK)
				|| (Action.Target->GetProcedure()==DFA_PULL) )
				{
				Action.Target->Action.ComDir=COMD_Stop;
				if (iTXDir<0) Action.Target->Action.ComDir=COMD_Left;
				if (iTXDir>0) Action.Target->Action.ComDir=COMD_Right;
				}

			// Pulling range
			iPushDistance = Max(Shape.Wdt/2-8,0);
			iPushRange = iPushDistance + 20;
      Action.Target->GetArea(sax,say,sawdt,sahgt);
			// Object lost
      if (!Inside(GetX()-sax,-iPushRange,sawdt-1+iPushRange)
        || !Inside(GetY()-say,-iPushRange,sahgt-1+iPushRange))
          {
					// Wait command (why, anyway?)
					StopActionDelayCommand(this);
					// Grab lost action
					GrabLost(this);
					// Lose target
					Action.Target=NULL;
					// Done
					return;
					}

			// Move to pulling position
			xdir = fMove + fWalk * BoundBy<int32_t>(iTargetX-GetX(),-10,+10) / 10;

      // Phase by XDir
			iPhaseAdvance=0;
      if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
      if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
			// No YDir
      ydir=0;
			// Attachment
      Action.t_attach|=CNAT_Bottom;
			// Mobile
      Mobile=1;

      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_CHOP:
      // Valid check
      if (!Action.Target) { ObjectActionStand(this); return; }
      // Chop
      if (!::Game.iTick3)
        if (!Action.Target->Chop(this))
          { ObjectActionStand(this); return; }
      // Valid check (again, target might have been destroyed)
      if (!Action.Target) { ObjectActionStand(this); return; }
      // AtObject check
      ocf=OCF_Chop;
      if (!Action.Target->At(GetX(),GetY(),ocf)) { ObjectActionStand(this); return; }
      // Position
			SetDir( (GetX()>Action.Target->GetX()) ? DIR_Left : DIR_Right );
      xdir=ydir=0;
      Action.t_attach|=CNAT_Bottom;
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_FIGHT:
      // Valid check
      if (!Action.Target || (Action.Target->GetProcedure()!=DFA_FIGHT))
        { ObjectActionStand(this); return; }

			// Fighting through doors only if doors open
			if (Action.Target->Contained != Contained)
				if ((Contained && !Contained->EntranceStatus) || (Action.Target->Contained && !Action.Target->Contained->EntranceStatus))
					{ ObjectActionStand(this); return; }

      // Physical training
			if (!::Game.iTick5)
				TrainPhysical(&C4PhysicalInfo::Fight, 1, C4MaxPhysical);

			// Direction
      if (Action.Target->GetX()>GetX()) SetDir(DIR_Right);
      if (Action.Target->GetX()<GetX()) SetDir(DIR_Left);
      // Position
      iTargetX=GetX();
      if (Action.Dir==DIR_Left)  iTargetX=Action.Target->GetX()+Action.Target->Shape.Wdt/2+2;
      if (Action.Dir==DIR_Right) iTargetX=Action.Target->GetX()-Action.Target->Shape.Wdt/2-2;
      lLimit=ValByPhysical(95, pPhysical->Walk);
      if (GetX()==iTargetX) Towards(xdir,Fix0,lLimit);
      if (GetX()<iTargetX) Towards(xdir,+lLimit,lLimit);
      if (GetX()>iTargetX) Towards(xdir,-lLimit,lLimit);
      // Distance check
			if ( (Abs(GetX()-Action.Target->GetX())>Shape.Wdt)
				|| (Abs(GetY()-Action.Target->GetY())>Shape.Wdt) )
					{ ObjectActionStand(this); return; }
      // Other
      Action.t_attach|=CNAT_Bottom;
      ydir=0;
      Mobile=1;
			// Experience
			if (!::Game.iTick35) DoExperience(+2);
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_LIFT:
      // Valid check
      if (!Action.Target) { SetAction(0); return; }
      // Target lifting force
      lftspeed=itofix(2); tydir=0;
      switch (Action.ComDir)
        {
        case COMD_Up:   tydir=-lftspeed; break;
        case COMD_Stop: tydir=-GravAccel; break;
        case COMD_Down: tydir=+lftspeed; break;
        }
      // Lift object
      if (!Action.Target->Lift(tydir,FIXED100(50)))
        { SetAction(0); return; }
      // Check LiftTop
      if (Def->LiftTop)
        if (Action.Target->GetY()<=(GetY()+Def->LiftTop))
          if (Action.ComDir==COMD_Up)
            Call(PSF_LiftTop);
      // General
      DoGravity(this);
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case DFA_FLOAT:
      // Float speed
      lLimit=FIXED100(pPhysical->Float);
      // ComDir changes xdir/ydir
      switch (Action.ComDir)
        {
        case COMD_Up:    ydir-=FloatAccel; break;
        case COMD_Down:  ydir+=FloatAccel; break;
        case COMD_Right: xdir+=FloatAccel; break;
        case COMD_Left:  xdir-=FloatAccel; break;
        case COMD_UpRight: ydir-=FloatAccel; xdir+=FloatAccel; break;
        case COMD_DownRight: ydir+=FloatAccel; xdir+=FloatAccel; break;
        case COMD_DownLeft: ydir+=FloatAccel; xdir-=FloatAccel; break;
        case COMD_UpLeft: ydir-=FloatAccel; xdir-=FloatAccel; break;
        }
      // xdir/ydir bounds
      if (ydir<-lLimit) ydir=-lLimit; if (ydir>+lLimit) ydir=+lLimit;
      if (xdir>+lLimit) xdir=+lLimit; if (xdir<-lLimit) xdir=-lLimit;
      Mobile=1;
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // ATTACH: Force position to target object
		//   own vertex index is determined by high-order byte of action data
		//   target vertex index is determined by low-order byte of action data
    case DFA_ATTACH:

			// No target
      if (!Action.Target)
				{
				if (Status)
					{
					SetAction(0);
					Call(PSF_AttachTargetLost);
					}
				return;
				}

			// Target incomplete and no incomplete activity
			if (!(Action.Target->OCF & OCF_FullCon))
				if (!Action.Target->Def->IncompleteActivity)
					{ SetAction(0); return; }

      // Force containment
      if (Action.Target->Contained!=Contained)
        if (Action.Target->Contained)
          Enter(Action.Target->Contained);
        else
          Exit(GetX(),GetY(),r);

      // Force position
      ForcePosition(Action.Target->GetX()+Action.Target->Shape.VtxX[Action.Data&255]
                    -Shape.VtxX[Action.Data>>8],
                    Action.Target->GetY()+Action.Target->Shape.VtxY[Action.Data&255]
                    -Shape.VtxY[Action.Data>>8]);
			// must zero motion...
			xdir=ydir=0;

      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case DFA_CONNECT:

			bool fBroke;
			fBroke=false;
			int32_t iConnectX,iConnectY;
			int32_t attachVertex0,attachVertex1;
			attachVertex0=attachVertex1=0;
				{
				C4Value lineAttachV; GetProperty(Strings.P[P_LineAttach], lineAttachV);
				C4ValueArray *lineAttach = lineAttachV.getArray();
				if (lineAttach)
					{
					attachVertex0 = lineAttach->GetItem(0).getInt();
					attachVertex1 = lineAttach->GetItem(1).getInt();
					}
				}

			// Line destruction check: Target missing or incomplete
			if (!Action.Target || (Action.Target->Con<FullCon)) fBroke=true;
			if (!Action.Target2	|| (Action.Target2->Con<FullCon))	fBroke=true;
			if (fBroke)
				{
				Call(PSF_LineBreak,&C4AulParSet(C4VBool(true)));
				AssignRemoval();
				return;
				}

			// Movement by Target
			if (Action.Target)
				{
				// Connect to vertex
				if (Def->Line == C4D_Line_Vertex)
					{ iConnectX=Action.Target->GetX()+Action.Target->Shape.GetVertexX(attachVertex0);
						iConnectY=Action.Target->GetY()+Action.Target->Shape.GetVertexY(attachVertex0); }
				// Connect to bottom center
				else
					{ iConnectX=Action.Target->GetX();
						iConnectY=Action.Target->GetY()+Action.Target->Shape.Hgt/4; }
				if ((iConnectX!=Shape.VtxX[0]) || (iConnectY!=Shape.VtxY[0]))
					{
					// Regular wrapping line
					if (Def->LineIntersect == 0)
						if (!Shape.LineConnect(iConnectX,iConnectY,0,+1,
							Shape.VtxX[0],Shape.VtxY[0])) fBroke=true;
					// No-intersection line
					if (Def->LineIntersect == 1)
						{ Shape.VtxX[0]=iConnectX; Shape.VtxY[0]=iConnectY; }
					}
				}
			// Movement by Target2
			if (Action.Target2)
				{
				// Connect to vertex
				if (Def->Line == C4D_Line_Vertex)
					{ iConnectX=Action.Target2->GetX()+Action.Target2->Shape.GetVertexX(attachVertex1);
						iConnectY=Action.Target2->GetY()+Action.Target2->Shape.GetVertexY(attachVertex1); }
				// Connect to bottom center
				else
					{	iConnectX=Action.Target2->GetX();
						iConnectY=Action.Target2->GetY()+Action.Target2->Shape.Hgt/4; }
				if ((iConnectX!=Shape.VtxX[Shape.VtxNum-1]) || (iConnectY!=Shape.VtxY[Shape.VtxNum-1]))
					{
					// Regular wrapping line
					if (Def->LineIntersect == 0)
						if (!Shape.LineConnect(iConnectX,iConnectY,Shape.VtxNum-1,-1,
							Shape.VtxX[Shape.VtxNum-1],Shape.VtxY[Shape.VtxNum-1])) fBroke=true;
					// No-intersection line
					if (Def->LineIntersect == 1)
						{ Shape.VtxX[Shape.VtxNum-1]=iConnectX; Shape.VtxY[Shape.VtxNum-1]=iConnectY; }
					}
				}

			// Line fBroke
			if (fBroke)
				{
				Call(PSF_LineBreak,0);
				AssignRemoval();
				return;
				}

			// Reduce line segments
			if (!::Game.iTick35)
				ReduceLineSegments(Shape, !::Game.iTick2);

			break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    default:
			// Attach
			if (pAction->GetPropertyInt(P_Attach))
				{
				Action.t_attach |= pAction->GetPropertyInt(P_Attach);
				xdir = ydir = 0;
				Mobile = 1;
				}
			// Free gravity
			else
				DoGravity(this);
      break;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    }

	// Phase Advance (zero delay means no phase advance)
	if (pAction->GetPropertyInt(P_Delay))
		{  
		Action.PhaseDelay+=iPhaseAdvance;
		if (Action.PhaseDelay >= pAction->GetPropertyInt(P_Delay))
			{
			// Advance Phase
			Action.PhaseDelay=0;
			Action.Phase += pAction->GetPropertyInt(P_Step);
			// Phase call
			if (pAction->GetPropertyStr(P_PhaseCall))
				{
				Call(pAction->GetPropertyStr(P_PhaseCall)->GetCStr());
				}
			// Phase end
			if (Action.Phase>=pAction->GetPropertyInt(P_Length)) 
				{
				// set new action if it's not Hold
				if (pAction->GetPropertyStr(P_NextAction) == Strings.P[P_Hold])
					Action.Phase = pAction->GetPropertyInt(P_Length)-1;
				else
					{
					// Set new action
					SetActionByName(pAction->GetPropertyStr(P_NextAction), NULL, NULL, SAC_StartCall | SAC_EndCall);
					}
				}
			}  
		}
	return;
	}


bool C4Object::SetOwner(int32_t iOwner)
	{
	C4Player *pPlr;
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
	// remove old owner view
	if (ValidPlr(Owner))
		{
		pPlr = ::Players.Get(Owner);
		while (pPlr->FoWViewObjs.Remove(this)) {}
		}
	else
		for (pPlr = ::Players.First; pPlr; pPlr = pPlr->Next)
			while (pPlr->FoWViewObjs.Remove(this)) {}
	// set new owner
	int32_t iOldOwner=Owner;
  Owner=iOwner;
	if (Owner != NO_OWNER)
		// add to plr view
		PlrFoWActualize();
	// this automatically updates controller
	Controller = Owner;
	// script callback
	Call(PSF_OnOwnerChanged, &C4AulParSet(C4VInt(Owner), C4VInt(iOldOwner)));
	// done
  return true;
	}


bool C4Object::SetPlrViewRange(int32_t iToRange)
	{
	// set new range
	PlrViewRange = iToRange;
	// resort into player's FoW-repeller-list
	PlrFoWActualize();
	// success
	return true;
	}


void C4Object::PlrFoWActualize()
	{
	C4Player *pPlr;
	// single owner?
	if (ValidPlr(Owner))
		{
		// single player's FoW-list
		pPlr = ::Players.Get(Owner);
		while (pPlr->FoWViewObjs.Remove(this)) {}
		if (PlrViewRange) pPlr->FoWViewObjs.Add(this, C4ObjectList::stNone);
		}
	// no owner?
	else
		{
		// all players!
		for (pPlr = ::Players.First; pPlr; pPlr = pPlr->Next)
			{
			while (pPlr->FoWViewObjs.Remove(this)) {}
			if (PlrViewRange) pPlr->FoWViewObjs.Add(this, C4ObjectList::stNone);
			}
		}
	}

void C4Object::SetAudibilityAt(C4TargetFacet &cgo, int32_t iX, int32_t iY)
	{
	// target pos (parallax)
	float cotx=cgo.TargetX,coty=cgo.TargetY; TargetPos(cotx, coty, cgo);
	Audible = Max<int>(Audible, BoundBy(100 - 100 * Distance(cotx + cgo.Wdt / 2, coty + cgo.Hgt / 2, iX,iY) / 700, 0, 100));
	AudiblePan = BoundBy<int>(AudiblePan + (iX - (cotx + cgo.Wdt / 2)) / 5, -100, 100);
	}

bool C4Object::IsVisible(int32_t iForPlr, bool fAsOverlay)
	{
	bool fDraw;
	C4Value vis;
	if (!GetProperty(Strings.P[P_Visibility], vis))
		return true;

	int32_t Visibility;
	C4ValueArray *parameters = vis.getArray();
	if (parameters && parameters->GetSize())
	{
		Visibility = parameters->GetItem(0).getInt();
	} else {
		Visibility = vis.getInt();
	}
	// check overlay
	if (Visibility & VIS_OverlayOnly)
	{
		if (!fAsOverlay) return false;
		if (Visibility == VIS_OverlayOnly) return true;
	}
	// check layer
	if (pLayer && pLayer != this && !fAsOverlay)
	{
		fDraw = pLayer->IsVisible(iForPlr, false);
		if (pLayer->GetPropertyInt(P_Visibility) & VIS_LayerToggle) fDraw = !fDraw;
		if (!fDraw) return false;
	}
	// no flags set?
	if (!Visibility) return true;
	// check visibility
	fDraw=false;
	if (Visibility & VIS_Owner) fDraw = fDraw || (iForPlr==Owner);
	if (iForPlr!=NO_OWNER)
	{
		// check all
		if (Visibility & VIS_Allies)	fDraw = fDraw || (iForPlr!=Owner && !Hostile(iForPlr, Owner));
		if (Visibility & VIS_Enemies)	fDraw = fDraw || (iForPlr!=Owner && Hostile(iForPlr, Owner));
		if (parameters) {
			if (Visibility & VIS_Select)	fDraw = fDraw || parameters->GetItem(1+iForPlr).getBool();
		}
	}
	else fDraw = fDraw || (Visibility & VIS_God);
	return fDraw;
}

bool C4Object::IsInLiquidCheck()
	{
	return GBackLiquid(GetX(),GetY()+Def->Float*Con/FullCon-1);
	}

void C4Object::SetRotation(int32_t nr)
{
	while(nr<0) nr+=360; nr%=360;
	// remove solid mask
	if (pSolidMaskData) pSolidMaskData->Remove(true, false);
	// set rotation
  r=nr;
  fix_r=itofix(nr);
	// Update face
	UpdateFace(true);
}

void C4Object::PrepareDrawing()
	{
	// color modulation
	if (ColorMod != 0xffffffff || (BlitMode & (C4GFXBLIT_MOD2 | C4GFXBLIT_CLRSFC_MOD2))) Application.DDraw->ActivateBlitModulation(ColorMod);
	// other blit modes
	Application.DDraw->SetBlitMode(BlitMode);
	}

void C4Object::FinishedDrawing()
	{
	// color modulation
	Application.DDraw->DeactivateBlitModulation();
	// extra blitting flags
	Application.DDraw->ResetBlitMode();
	}

void C4Object::DrawSolidMask(C4TargetFacet &cgo)
	{
	// mask must exist
	if (!pSolidMaskData) return;
	// draw it
	pSolidMaskData->Draw(cgo);
	}

void C4Object::UpdateSolidMask(bool fRestoreAttachedObjects)
	{
	// solidmask doesn't make sense with non-existant objects
	// (the solidmask has already been destroyed in AssignRemoval -
	//  do not reset it!)
	if (!Status) return;
	// Determine necessity, update cSolidMask, put or remove mask
	// Mask if enabled, fullcon, no rotation, not contained
	if (SolidMask.Wdt>0)
		if (Con>=FullCon)
			if (!Contained)
				if (!r || Def->RotatedSolidmasks)
					{
					// Recheck and put mask
					if (!pSolidMaskData)
						{
						pSolidMaskData = new C4SolidMask(this);
						}
					else
						pSolidMaskData->Remove(true, false);
					pSolidMaskData->Put(true, NULL, fRestoreAttachedObjects);
					return;
					}
	// Otherwise, remove and destroy mask
	if (pSolidMaskData)
		{
		pSolidMaskData->Remove(true, false);
		delete pSolidMaskData; pSolidMaskData = NULL;
		}
	}

bool C4Object::Collect(C4Object *pObj)
	{
	// Special: attached Flag may not be collectable
	if (pObj->Def->id==C4ID_Flag)
		if (!(Game.Rules & C4RULE_FlagRemoveable))
			if (pObj->Action.pActionDef)
				if (SEqual(pObj->Action.pActionDef->GetName(),"FlyBase"))
					return false;       
	// Object enter container
	bool fRejectCollect;
	if(!pObj->Enter(this, true, false, &fRejectCollect))
		return false;
	// Cancel attach (hacky)
	ObjectComCancelAttach(pObj);
	// Container Collection call
	Call(PSF_Collection,&C4AulParSet(C4VObj(pObj)));
	// Object Hit call
	if (pObj->Status && pObj->OCF & OCF_HitSpeed1) pObj->Call(PSF_Hit);
	if (pObj->Status && pObj->OCF & OCF_HitSpeed2) pObj->Call(PSF_Hit2);
	if (pObj->Status && pObj->OCF & OCF_HitSpeed3) pObj->Call(PSF_Hit3);
	// post-copy the motion of the new container
	if (pObj->Contained == this) pObj->CopyMotion(this);
	// done, success
	return true;
	}

bool C4Object::GrabInfo(C4Object *pFrom)
	{
	// safety
	if (!pFrom) return false; if (!Status || !pFrom->Status) return false;
	// even more safety (own info: success)
	if(pFrom == this) return true;
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
	if(!Properties.Has(Strings.P[P_Name])) SetName(Info->Name);
	// retire from old crew
	Info->Retire();
	// set death status
	Info->HasDied = !Alive;
	// if alive, recruit to new crew
	if (Alive) Info->Recruit();
	// make new crew member
	C4Player *pPlr = ::Players.Get(Owner);
	if (pPlr) pPlr->MakeCrewMember(this);
	// done, success
	return true;
	}

bool C4Object::ShiftContents(bool fShiftBack, bool fDoCalls)
	{
	// get current object
	C4Object *c_obj = Contents.GetObject();
	if (!c_obj) return false;
	// get next/previous
	C4ObjectLink *pLnk = fShiftBack ? (Contents.Last) : (Contents.First->Next);
	for(;;)
		{
		// end reached without success
		if (!pLnk) return false;
		// check object
		C4Object *pObj = pLnk->Obj;
		if (pObj->Status)
			if (!c_obj->CanConcatPictureWith(pObj))
				{
				// object different: shift to this
				DirectComContents(pObj, !!fDoCalls);
				return true;
				}
		// next/prev item
		pLnk = fShiftBack ? (pLnk->Prev) : (pLnk->Next);
		}
	// not reached
	}

void C4Object::DirectComContents(C4Object *pTarget, bool fDoCalls)
	{
	// safety
	if (!pTarget || !pTarget->Status || pTarget->Contained != this) return;
	// Desired object already at front?
	if (Contents.GetObject() == pTarget) return;
	// select object via script?
	if (fDoCalls)
		if (!! Call("~ControlContents", &C4AulParSet(C4VID(pTarget->id))))
			return;
	// default action
	if (!(Contents.ShiftContents(pTarget))) return;
	// Selection sound
	if (fDoCalls) if (!Contents.GetObject()->Call("~Selection", &C4AulParSet(C4VObj(this)))) StartSoundEffect("Grab",false,100,this);
	// update menu with the new item in "put" entry
	if (Menu && Menu->IsActive() && Menu->IsContextMenu())
		{
		Menu->Refill();
		}
	// Done
	return;
	}

void C4Object::GetParallaxity(int32_t *parX, int32_t *parY)
{
	assert(parX); assert(parY);
	*parX = 100; *parY = 100;
	if (!(Category & C4D_Parallax)) return;
	C4Value parV; GetProperty(Strings.P[P_Parallaxity], parV);
	C4ValueArray *par = parV.getArray();
	if (!par) return;
	*parX = par->GetItem(0).getInt();
	*parY = par->GetItem(1).getInt();
}

void C4Object::ApplyParallaxity(float &riTx, float &riTy, const C4Facet &fctViewport)
	{
	// parallaxity by locals
	// special: Negative positions with parallaxity 0 mean HUD elements positioned to the right/bottom
	int iParX, iParY;
	GetParallaxity(&iParX, &iParY);
	if (!iParX && GetX()<0)
		riTx = -fctViewport.Wdt;
	else
		riTx = riTx * iParX / 100;
	if (!iParY && GetY()<0)
		riTy = -fctViewport.Hgt;
	else
		riTy = riTy * iParY / 100;
	}

bool C4Object::DoSelect(bool fCursor)
	{
	// selection allowed?
	if (CrewDisabled) return true;
	// select
	if (!fCursor) Select=1;
	// do callback
	Call(PSF_CrewSelection, &C4AulParSet(C4VBool(false), C4VBool(!!fCursor)));
	// done
	return true;
	}

void C4Object::UnSelect(bool fCursor)
	{
	// unselect
	if (!fCursor) Select=0;
	// do callback
	Call(PSF_CrewSelection, &C4AulParSet(C4VBool(true), C4VBool(!!fCursor)));
	}

void C4Object::GetViewPosPar(float &riX, float &riY, float tx, float ty, const C4Facet &fctViewport)
	{
	int iParX, iParY;
	GetParallaxity(&iParX, &iParY);
	// get drawing pos, then subtract original target pos to get drawing pos on landscape
	if (!iParX && GetX()<0)
		// HUD element at right viewport pos
		riX=fixtof(fix_x)+tx+fctViewport.Wdt;
	else
		// regular parallaxity
		riX=fixtof(fix_x)-(tx*(iParX-100)/100);
	if (!iParY && GetY()<0)
		// HUD element at bottom viewport pos
		riY=fixtof(fix_y)+ty+fctViewport.Hgt;
	else
		// regular parallaxity
		riY=fixtof(fix_y)-(ty*(iParY-100)/100);
	}

bool C4Object::PutAwayUnusedObject(C4Object *pToMakeRoomForObject)
	{
	// get unused object
	C4Object *pUnusedObject;
	C4AulFunc *pFnObj2Drop;
	if (pFnObj2Drop = Def->Script.GetSFunc(PSF_GetObject2Drop))
		pUnusedObject = pFnObj2Drop->Exec(this, pToMakeRoomForObject ? &C4AulParSet(C4VObj(pToMakeRoomForObject)) : NULL).getObj();
	else
	{
		// is there any unused object to put away?
		if(!Contents.Last) return false;
		// defaultly, it's the last object in the list
		// (contents list cannot have invalid status-objects)
		pUnusedObject = Contents.Last->Obj;
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

bool C4Object::SetGraphics(const char *szGraphicsName, C4Def *pSourceDef)
	{
	// safety
	if (!Status) return false;
	// default def
	if (!pSourceDef) pSourceDef = Def;
	// get graphics
	C4DefGraphics *pGrp = pSourceDef->Graphics.Get(szGraphicsName);
	if (!pGrp) return false;
	// no change? (no updates need to be done, then)
	if (pGraphics == pGrp) return true;
	// set new graphics
	pGraphics = pGrp;
	// update Color, SolidMask, etc.
	UpdateGraphics(true);
	// success
	return true;
	}

bool C4Object::SetGraphics(C4DefGraphics *pNewGfx, bool fTemp)
	{
	// safety
	if (!pNewGfx) return false;
	// set it and update related stuff
	pGraphics = pNewGfx;
	UpdateGraphics(true, fTemp);
	return true;
	}

C4GraphicsOverlay *C4Object::GetGraphicsOverlay(int32_t iForID, bool fCreate)
	{
	// search in list until ID is found or passed
	C4GraphicsOverlay *pOverlay = pGfxOverlay, *pPrevOverlay = NULL;
	while (pOverlay && pOverlay->GetID() < iForID) { pPrevOverlay = pOverlay; pOverlay = pOverlay->GetNext(); }
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iForID) return pOverlay;
	// ID has been passed: Create new if desired
	if (!fCreate) return NULL;
	C4GraphicsOverlay *pNewOverlay = new C4GraphicsOverlay();
	pNewOverlay->SetID(iForID);
	pNewOverlay->SetNext(pOverlay);
	if (pPrevOverlay) pPrevOverlay->SetNext(pNewOverlay); else pGfxOverlay = pNewOverlay;
	// return newly created overlay
	return pNewOverlay;
	}

bool C4Object::RemoveGraphicsOverlay(int32_t iOverlayID)
	{
	// search in list until ID is found or passed
	C4GraphicsOverlay *pOverlay = pGfxOverlay, *pPrevOverlay = NULL;
	while (pOverlay && pOverlay->GetID() < iOverlayID) { pPrevOverlay = pOverlay; pOverlay = pOverlay->GetNext(); }
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iOverlayID)
		{
		// remove it
		if (pPrevOverlay) pPrevOverlay->SetNext(pOverlay->GetNext()); else pGfxOverlay = pOverlay->GetNext();
		pOverlay->SetNext(NULL); // prevents deletion of following overlays
		delete pOverlay;
		// removed
		return true;
		}
	// no match found
	return false;
	}

bool C4Object::HasGraphicsOverlayRecursion(const C4Object *pCheckObj) const
	{
	C4Object *pGfxOvrlObj;
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrlObj = pGfxOvrl->GetOverlayObject())
				{
				if (pGfxOvrlObj == pCheckObj) return true;
				if (pGfxOvrlObj->HasGraphicsOverlayRecursion(pCheckObj)) return true;
				}
	return false;
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
	Call(PSF_UpdateTransferZone);
	// done, success
	return true;
	}

bool C4Object::StatusDeactivate(bool fClearPointers)
	{
	// clear particles
	if (FrontParticles) FrontParticles.Clear();
	if (BackParticles) BackParticles.Clear();
	// put into inactive list
	::Objects.Remove(this);
	Status = C4OS_INACTIVE;
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

void C4Object::ClearContentsAndContained(bool fDoCalls)
	{
  // exit contents from container
  C4Object *cobj; C4ObjectLink *clnk,*next;
  for (clnk=Contents.First; clnk && (cobj=clnk->Obj); clnk=next)
    {
    next=clnk->Next;
		cobj->Exit(GetX(), GetY(), 0,Fix0,Fix0,Fix0, fDoCalls);
    }
	// remove from container *after* contents have been removed!
	if (Contained) Exit(GetX(), GetY(), 0, Fix0, Fix0, Fix0, fDoCalls);
	}

bool C4Object::AdjustWalkRotation(int32_t iRangeX, int32_t iRangeY, int32_t iSpeed)
	{
	int32_t iDestAngle;
	// attachment at middle (bottom) vertex?
	if (Shape.iAttachVtx<0 || !Def->Shape.VtxX[Shape.iAttachVtx])
		{
		// evaluate floor around attachment pos
		int32_t iSolidLeft=0, iSolidRight=0;
		// left
		int32_t iXCheck = Shape.iAttachX-iRangeX;
		if (GBackSolid(iXCheck, Shape.iAttachY))
			{
			// up
			while (--iSolidLeft>-iRangeY)
				if (GBackSolid(iXCheck, Shape.iAttachY+iSolidLeft))
					{ ++iSolidLeft; break; }
			}
		else
			// down
			while (++iSolidLeft<iRangeY)
				if (GBackSolid(iXCheck, Shape.iAttachY+iSolidLeft))
					{ --iSolidLeft; break; }
		// right
		iXCheck += 2*iRangeX;
		if (GBackSolid(iXCheck, Shape.iAttachY))
			{
			// up
			while (--iSolidRight>-iRangeY)
				if (GBackSolid(iXCheck, Shape.iAttachY+iSolidRight))
					{ ++iSolidRight; break; }
			}
		else
			// down
			while (++iSolidRight<iRangeY)
				if (GBackSolid(iXCheck, Shape.iAttachY+iSolidRight))
					{ --iSolidRight; break; }
		// calculate destination angle
		// 100% accurate for large values of Pi ;)
		iDestAngle=(iSolidRight-iSolidLeft)*(35/Max<int32_t>(iRangeX, 1));
		}
	else
		{
		// attachment at other than horizontal middle vertex: get your feet to the ground!
		// rotate target to large angle is OK, because rotation will stop once the real
		// bottom vertex hits solid ground
		if (Shape.VtxX[Shape.iAttachVtx] > 0)
			iDestAngle = -50;
		else
			iDestAngle = 50;
		}
	// move to destination angle
	if (Abs(iDestAngle-r)>2)
		{
		rdir = itofix(BoundBy<int32_t>(iDestAngle-r, -15,+15));
		rdir/=(10000/iSpeed);
		}
	else rdir=0;
	// done, success
	return true;
	}

void C4Object::UpdateInLiquid()
	{
  // InLiquid check
  if (IsInLiquidCheck()) // In Liquid
    {
    if (!InLiquid) // Enter liquid
      {
      if (OCF & OCF_HitSpeed2) if (Mass>3)
        Splash(GetX(),GetY()+1,Min(Shape.Wdt*Shape.Hgt/10,20),this);
      InLiquid=1;
      }
    }
  else // Out of liquid
    {
    if (InLiquid) // Leave liquid
      InLiquid=0;
    }
	}

StdStrBuf C4Object::GetInfoString()
	{
	StdStrBuf sResult;
	// no info for invalid objects
	if (!Status) return sResult;
	// first part always description
	sResult.Copy(Def->GetDesc());
	// go through all effects and add their desc
	for (C4Effect *pEff = pEffects; pEff; pEff = pEff->pNext)
		{
		C4Value par[7];
		C4Value vInfo = pEff->DoCall(this, PSFS_FxInfo, par[0], par[1], par[2], par[3], par[4], par[5], par[6]);
		if (!vInfo) continue;
		// debug: warn for wrong return types
		if (vInfo.GetType() != C4V_String)
			DebugLogF("Effect %s(%d) on object %s (#%d) returned wrong info type %d.", pEff->Name, pEff->iNumber, Def->GetName(), Number, vInfo.GetType());
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

void C4Object::GrabContents(C4Object *pFrom)
	{
	// create a temp list of all objects and transfer it
	// this prevents nasty deadlocks caused by RejectEntrance-scripts
	C4ObjectList tmpList; tmpList.Copy(pFrom->Contents);
	C4ObjectLink *cLnk;
	for (cLnk=tmpList.First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->Enter(this);
	}

bool C4Object::CanConcatPictureWith(C4Object *pOtherObject)
	{
	// check current definition ID
	if (id != pOtherObject->id) return false;
	// def overwrite of stack conditions
	int32_t allow_picture_stack = Def->AllowPictureStack;
	if (!(allow_picture_stack & APS_Color))
	{
		// check color if ColorByOwner (flags)
		if (Color != pOtherObject->Color && Def->ColorByOwner) return false;
		// check modulation
		if (ColorMod != pOtherObject->ColorMod) return false;
		if (BlitMode != pOtherObject->BlitMode) return false;
	}
	if (!(allow_picture_stack & APS_Graphics))
	{
		// check graphics
		if (pGraphics != pOtherObject->pGraphics) return false;
		// check any own picture rect
		if (PictureRect != pOtherObject->PictureRect) return false;
	}
	if (!(allow_picture_stack & APS_Name))
	{
		// check name, so zagabar's sandwiches don't stack
		if (GetName() != pOtherObject->GetName()) return false;
	}
	if (!(allow_picture_stack & APS_Overlay))
	{
		// check overlay graphics
		for (C4GraphicsOverlay *pOwnOverlay = pGfxOverlay; pOwnOverlay; pOwnOverlay = pOwnOverlay->GetNext())
			if (pOwnOverlay->IsPicture())
				{
				C4GraphicsOverlay *pOtherOverlay = pOtherObject->GetGraphicsOverlay(pOwnOverlay->GetID(), false);
				if (!pOtherOverlay || !(*pOtherOverlay == *pOwnOverlay)) return false;
				}
		for (C4GraphicsOverlay *pOtherOverlay = pOtherObject->pGfxOverlay; pOtherOverlay; pOtherOverlay = pOtherOverlay->GetNext())
			if (pOtherOverlay->IsPicture())
				if (!GetGraphicsOverlay(pOtherOverlay->GetID(), false)) return false;
	}
	// concat OK
	return true;
	}

int32_t C4Object::GetFireCausePlr()
	{
	// get fire effect
	if (!pEffects) return NO_OWNER;
	C4Effect *pFire = pEffects->Get(C4Fx_Fire);
	if (!pFire) return NO_OWNER;
	// get causing player
	int32_t iFireCausePlr = FxFireVarCausedBy(pFire).getInt();
	// return if valid
	if (ValidPlr(iFireCausePlr)) return iFireCausePlr; else return NO_OWNER;
	}

void C4Object::UpdateScriptPointers()
	{
	if (pEffects)
		pEffects->ReAssignAllCallbackFunctions();
	}

StdStrBuf C4Object::GetNeededMatStr(C4Object *pBuilder)
	{
	C4Def* pComponent;
	int32_t cnt, ncnt;
	StdStrBuf NeededMats;

	C4IDList NeededComponents;
	Def->GetComponents(&NeededComponents, NULL, pBuilder);

	C4ID idComponent;

	for(cnt = 0; idComponent=NeededComponents.GetID(cnt); cnt ++)
		{
		if(NeededComponents.GetCount(cnt)!=0)
			{
			ncnt = NeededComponents.GetCount(cnt) - Component.GetIDCount(idComponent);
			if(ncnt > 0)
				{
				//if(!NeededMats) NeededMats.Append(", "); what was this for...?
        NeededMats.AppendFormat("|%dx ", ncnt);
				if((pComponent = C4Id2Def(idComponent)))
					NeededMats.Append(pComponent->GetName());
				else
					NeededMats.Append(C4IdText(idComponent));
				}
			}
		}

	StdStrBuf result;
	if(!!NeededMats)
		{ result.Format(LoadResStr("IDS_CON_BUILDMATNEED"), GetName()); result.Append(NeededMats.getData()); }
	else
		result.Format(LoadResStr("IDS_CON_BUILDMATNONE"), GetName());

	return result;
	}

bool C4Object::IsPlayerObject(int32_t iPlayerNumber)
	{
	bool fAnyPlr = (iPlayerNumber == NO_OWNER);
	// if an owner is specified: only owned objects
	if (fAnyPlr && !ValidPlr(Owner)) return false;
	// and crew objects
	if (fAnyPlr || Owner == iPlayerNumber)
		{
	  // flags are player objects
	  if (id == C4ID_Flag) return true;

		C4Player *pOwner = ::Players.Get(Owner);
		if (pOwner)
			{
			if (pOwner && pOwner->Crew.IsContained(this)) return true;
			}
		else
			{
			// Do not force that the owner exists because the function must work for unjoined players (savegame resume)
			if (Category & C4D_CrewMember)
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
