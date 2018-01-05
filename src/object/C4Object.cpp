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
#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Physics.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameMessage.h"
#include "landscape/C4MaterialList.h"
#include "landscape/C4PXS.h"
#include "landscape/C4Particles.h"
#include "landscape/C4SolidMask.h"
#include "landscape/fow/C4FoW.h"
#include "lib/C4Random.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4MeshAnimation.h"
#include "object/C4MeshDenumerator.h"
#include "object/C4ObjectCom.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectMenu.h"
#include "platform/C4SoundSystem.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4RankSystem.h"
#include "script/C4AulExec.h"
#include "script/C4Effect.h"

static void DrawVertex(C4Facet &cgo, float tx, float ty, int32_t col, int32_t contact)
{
	if (Inside<int32_t>(tx,cgo.X,cgo.X+cgo.Wdt) && Inside<int32_t>(ty,cgo.Y,cgo.Y+cgo.Hgt))
	{
		pDraw->DrawLineDw(cgo.Surface, tx - 1, ty, tx + 1, ty, col, 0.5f);
		pDraw->DrawLineDw(cgo.Surface, tx, ty - 1, tx, ty + 1, col, 0.5f);
		if (contact) pDraw->DrawFrameDw(cgo.Surface,tx-1.5,ty-1.5,tx+1.5,ty+1.5,C4RGB(0xff, 0xff, 0xff));
	}
}

void C4Action::SetBridgeData(int32_t iBridgeTime, bool fMoveClonk, bool fWall, int32_t iBridgeMaterial)
{
	// validity
	iBridgeMaterial = std::min(iBridgeMaterial, ::MaterialMap.Num-1);
	if (iBridgeMaterial < 0) iBridgeMaterial = 0xff;
	iBridgeTime = Clamp<int32_t>(iBridgeTime, 0, 0xffff);
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

void C4Object::AssignRemoval(bool fExitContents)
{
	// check status
	if (!Status) return;
	if (Config.General.DebugRec)
	{
		C4RCCreateObj rc;
		memset(&rc, '\0', sizeof(rc));
		rc.oei=Number;
		if (Def && Def->GetName()) strncpy(rc.id, Def->GetName(), 32+1);
		rc.x=GetX(); rc.y=GetY(); rc.ownr=Owner;
		AddDbgRec(RCT_DsObj, &rc, sizeof(rc));
	}
	// Destruction call in container
	if (Contained)
	{
		C4AulParSet pars(this);
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
		pEffects->ClearAll(C4FxCall_RemoveClear);
		// Effect-callback might actually have deleted the object already
		if (!Status) return;
	}
	// remove particles
	ClearParticleLists();
	// Action idle
	SetAction(nullptr);
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

	// get container for next actions
	C4Object *pCont = Contained;
	// remove or exit contents 
	for (C4Object *cobj : Contents)
	{
		if (fExitContents)
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
		if (!Def->Rotateable || (fix_r == Fix0))
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
	// new grafics: update face
	if (fGraphicsChanged)
	{
		// Keep mesh instance if it uses the same underlying mesh
		if(!pMeshInstance || pGraphics->Type != C4DefGraphics::TYPE_Mesh ||
		   &pMeshInstance->GetMesh() != pGraphics->Mesh)
		{
			// If this mesh is attached somewhere, detach it before deletion
			if(pMeshInstance && pMeshInstance->GetAttachParent() != nullptr)
			{
				// TODO: If the new mesh has a bone with the same name, we could try updating...
				StdMeshInstance::AttachedMesh* attach_parent = pMeshInstance->GetAttachParent();
				attach_parent->Parent->DetachMesh(attach_parent->Number);
			}

			delete pMeshInstance;

			if (pGraphics->Type == C4DefGraphics::TYPE_Mesh)
			{
				pMeshInstance = new StdMeshInstance(*pGraphics->Mesh, Def->GrowthType ? 1.0f : static_cast<float>(Con)/static_cast<float>(FullCon));
				pMeshInstance->SetFaceOrderingForClrModulation(ColorMod);
			}
			else
			{
				pMeshInstance = nullptr;
			}
		}

		// update face - this also puts any SolidMask
		UpdateFace(false);
	}
}

void C4Object::UpdateFlipDir()
{
	int32_t iFlipDir;
	// We're active
	C4PropList* pActionDef = GetAction();
	if (pActionDef)
		// Get flipdir value from action
		if ((iFlipDir = pActionDef->GetPropertyInt(P_FlipDir)))
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
			pDrawTransform=nullptr;
		}
	}
}

void C4Object::DrawFaceImpl(C4TargetFacet &cgo, bool action, float fx, float fy, float fwdt, float fhgt, float tx, float ty, float twdt, float thgt, C4DrawTransform* transform) const
{
	C4Surface* sfc;
	switch (GetGraphics()->Type)
	{
	case C4DefGraphics::TYPE_None:
		// no graphics.
		break;
	case C4DefGraphics::TYPE_Bitmap:
		sfc = action ? Action.Facet.Surface : GetGraphics()->GetBitmap(Color);

		pDraw->Blit(sfc,
		              fx, fy, fwdt, fhgt,
		              cgo.Surface, tx, ty, twdt, thgt,
		              true, transform);
		break;
	case C4DefGraphics::TYPE_Mesh:
		C4Value value;
		GetProperty(P_MeshTransformation, &value);
		StdMeshMatrix matrix;
		if (!C4ValueToMatrix(value, &matrix))
			matrix = StdMeshMatrix::Identity();

		if (fix_r != Fix0)
		{
			// Rotation should happen around the mesh center after application of any mesh transformation
			// So translate back by the transformed mesh center before rotation
			auto mesh_center = pMeshInstance->GetMesh().GetBoundingBox().GetCenter();
			mesh_center = matrix * mesh_center;
			matrix = StdMeshMatrix::Translate(-mesh_center.x, -mesh_center.y, -mesh_center.z) * matrix;
			matrix = StdMeshMatrix::Rotate(fixtof(fix_r) * (M_PI / 180.0f), 0.0f, 0.0f, 1.0f) * matrix;
			matrix = StdMeshMatrix::Translate(mesh_center.x, mesh_center.y, mesh_center.z) * matrix;
		}

		if(twdt != fwdt || thgt != fhgt)
		{
			// Also scale Z so that the mesh is not totally distorted and
			// so that normals halfway keep pointing into sensible directions.
			// We don't have a better guess so use the geometric mean for Z scale.
			matrix = StdMeshMatrix::Scale(twdt/fwdt,thgt/fhgt,std::sqrt(twdt*thgt/(fwdt*fhgt))) * matrix;
		}

		pDraw->SetMeshTransform(&matrix);

		pDraw->RenderMesh(*pMeshInstance, cgo.Surface, tx, ty, twdt, thgt, Color, transform);
		pDraw->SetMeshTransform(nullptr);
		break;
	}
}

void C4Object::DrawFace(C4TargetFacet &cgo, float offX, float offY, int32_t iPhaseX, int32_t iPhaseY) const
{
	const auto swdt = float(Def->Shape.Wdt);
	const auto shgt = float(Def->Shape.Hgt);
	// Grow Type Display
	auto fx = float(swdt * iPhaseX);
	auto fy = float(shgt * iPhaseY);
	auto fwdt = float(swdt);
	auto fhgt = float(shgt);

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

	C4DrawTransform transform;
	bool transform_active = false;
	if (pDrawTransform)
	{
		transform.SetTransformAt(*pDrawTransform, offX, offY);
		transform_active = true;
	}

	// Meshes aren't rotated via DrawTransform to ensure lighting is applied correctly.
	if (GetGraphics()->Type != C4DefGraphics::TYPE_Mesh && Def->Rotateable && fix_r != Fix0)
	{
		if (pDrawTransform)
			transform.Rotate(fixtof(fix_r), offX, offY);
		else
			transform.SetRotate(fixtof(fix_r), offX, offY);
		transform_active = true;
	}

	DrawFaceImpl(cgo, false, fx, fy, fwdt, fhgt, tx, ty, twdt, thgt, transform_active ? &transform : nullptr);
}

void C4Object::DrawActionFace(C4TargetFacet &cgo, float offX, float offY) const
{
	// This should not be called for meshes since Facet has no meaning
	// for them. Only use DrawFace() with meshes!
	assert(GetGraphics()->Type == C4DefGraphics::TYPE_Bitmap);
	C4PropList* pActionDef = GetAction();

	// Regular action facet
	const auto swdt = float(Action.Facet.Wdt);
	const auto shgt = float(Action.Facet.Hgt);
	int32_t iPhase = Action.Phase;
	if (pActionDef->GetPropertyInt(P_Reverse)) iPhase = pActionDef->GetPropertyInt(P_Length) - 1 - Action.Phase;

	// Grow Type Display
	auto fx = float(Action.Facet.X + swdt * iPhase);
	auto fy = float(Action.Facet.Y + shgt * Action.DrawDir);
	auto fwdt = float(swdt);
	auto fhgt = float(shgt);

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
		float offset_from_top = shgt * std::max(FullCon - Con, 0) / FullCon;
		fy += offset_from_top;
		fhgt -= offset_from_top;
	}
	
	C4DrawTransform transform;
	bool transform_active = false;
	if (pDrawTransform)
	{
		transform.SetTransformAt(*pDrawTransform, offX, offY);
		transform_active = true;
	}

	// Meshes aren't rotated via DrawTransform to ensure lighting is applied correctly.
	if (GetGraphics()->Type != C4DefGraphics::TYPE_Mesh && Def->Rotateable && fix_r != Fix0)
	{
		if (pDrawTransform)
			transform.Rotate(fixtof(fix_r), offX, offY);
		else
			transform.SetRotate(fixtof(fix_r), offX, offY);
		transform_active = true;
	}

	DrawFaceImpl(cgo, true, fx, fy, fwdt, fhgt, tx, ty, twdt, thgt, transform_active ? &transform : nullptr);
}

void C4Object::UpdateMass()
{
	Mass=std::max<int32_t>((Def->Mass+OwnMass)*Con/FullCon,1);
	if (!Def->NoMassFromContents) Mass+=Contents.Mass;
	if (Contained)
	{
		Contained->Contents.MassCount();
		Contained->UpdateMass();
	}
}

void C4Object::UpdateInMat()
{
	// get new mat
	int32_t newmat;
	if (Contained)
		newmat = Contained->Def->ClosedContainer ? MNone : Contained->InMat;
	else
		newmat = GBackMat(GetX(), GetY());

	// mat changed?
	if (newmat != InMat)
	{
		Call(PSF_OnMaterialChanged,&C4AulParSet(newmat,InMat));
		InMat = newmat;
	}
}

void C4Object::SetOCF()
{
	C4PropList* pActionDef = GetAction();
	uint32_t dwOCFOld = OCF;
	// Update the object character flag according to the object's current situation
	C4Real cspeed=GetSpeed();
#ifdef _DEBUG
	if (Contained && !C4PropListNumbered::CheckPropList(Contained))
		{ LogF("Warning: contained in wild object %p!", static_cast<void*>(Contained)); }
	else if (Contained && !Contained->Status)
		{ LogF("Warning: contained in deleted object (#%d) (%s)!", Contained->Number, Contained->GetName()); }
#endif
	// OCF_Normal: The OCF is never zero
	OCF=OCF_Normal;
	// OCF_Construct: Can be built outside
	if (Def->Constructable && (Con<FullCon)
	    && (fix_r==Fix0) && !OnFire)
		OCF|=OCF_Construct;
	// OCF_Grab: Can be pushed
	if (GetPropertyInt(P_Touchable))
		OCF|=OCF_Grab;
	// OCF_Carryable: Can be picked up
	if (GetPropertyInt(P_Collectible))
		OCF|=OCF_Carryable;
	// OCF_OnFire: Is burning
	if (OnFire)
		OCF|=OCF_OnFire;
	// OCF_Inflammable: Is not burning and is inflammable
	if (!OnFire && GetPropertyInt(P_ContactIncinerate) > 0)
		OCF|=OCF_Inflammable;
	// OCF_FullCon: Is fully completed/grown
	if (Con>=FullCon)
		OCF|=OCF_FullCon;
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
		if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (GetR() <= Def->RotatedEntrance)))
			OCF|=OCF_Entrance;
	// HitSpeeds
	if (cspeed>=HitSpeed1) OCF|=OCF_HitSpeed1;
	if (cspeed>=HitSpeed2) OCF|=OCF_HitSpeed2;
	if (cspeed>=HitSpeed3) OCF|=OCF_HitSpeed3;
	if (cspeed>=HitSpeed4) OCF|=OCF_HitSpeed4;
	// OCF_Collection
	if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
		if ((Def->Collection.Wdt>0) && (Def->Collection.Hgt>0))
			if (!pActionDef || (!pActionDef->GetPropertyInt(P_ObjectDisabled)))
					OCF|=OCF_Collection;
	// OCF_Alive
	if (Alive) OCF|=OCF_Alive;
	// OCF_CrewMember
	if (Def->CrewMember)
		if (Alive)
			OCF|=OCF_CrewMember;
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
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF|=OCF_Container;
	if (DEBUGREC_OCF && Config.General.DebugRec)
	{
		C4RCOCF rc = { dwOCFOld, OCF, false };
		AddDbgRec(RCT_OCF, &rc, sizeof(rc));
	}
}


void C4Object::UpdateOCF()
{
	C4PropList* pActionDef = GetAction();
	uint32_t dwOCFOld = OCF;
	// Update the object character flag according to the object's current situation
	C4Real cspeed=GetSpeed();
#ifdef _DEBUG
	if (Contained && !C4PropListNumbered::CheckPropList(Contained))
		{ LogF("Warning: contained in wild object %p!", static_cast<void*>(Contained)); }
	else if (Contained && !Contained->Status)
		{ LogF("Warning: contained in deleted object %p (%s)!", static_cast<void*>(Contained), Contained->GetName()); }
#endif
	// Keep the bits that only have to be updated with SetOCF (def, category, con, alive, onfire)
	OCF=OCF & (OCF_Normal | OCF_Exclusive | OCF_FullCon | OCF_Rotate | OCF_OnFire
		| OCF_Alive | OCF_CrewMember);
	// OCF_inflammable: can catch fire and is not currently burning.
	if (!OnFire && GetPropertyInt(P_ContactIncinerate) > 0)
		OCF |= OCF_Inflammable;
	// OCF_Carryable: Can be picked up
	if (GetPropertyInt(P_Collectible))
		OCF|=OCF_Carryable;
	// OCF_Grab: Can be grabbed.
	if (GetPropertyInt(P_Touchable))
		OCF |= OCF_Grab;
	// OCF_Construct: Can be built outside
	if (Def->Constructable && (Con<FullCon)
	    && (fix_r == Fix0) && !OnFire)
		OCF|=OCF_Construct;
	// OCF_Entrance: Can currently be entered/activated
	if ((Def->Entrance.Wdt>0) && (Def->Entrance.Hgt>0))
		if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (GetR() <= Def->RotatedEntrance)))
			OCF|=OCF_Entrance;
	// HitSpeeds
	if (cspeed>=HitSpeed1) OCF|=OCF_HitSpeed1;
	if (cspeed>=HitSpeed2) OCF|=OCF_HitSpeed2;
	if (cspeed>=HitSpeed3) OCF|=OCF_HitSpeed3;
	if (cspeed>=HitSpeed4) OCF|=OCF_HitSpeed4;
	// OCF_Collection
	if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
		if ((Def->Collection.Wdt>0) && (Def->Collection.Hgt>0))
			if (!pActionDef || (!pActionDef->GetPropertyInt(P_ObjectDisabled)))
					OCF|=OCF_Collection;
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
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF|=OCF_Container;
	if (DEBUGREC_OCF && Config.General.DebugRec)
	{
		C4RCOCF rc = { dwOCFOld, OCF, true };
		AddDbgRec(RCT_OCF, &rc, sizeof(rc));
	}
#ifdef _DEBUG
	DEBUGREC_OFF
	uint32_t updateOCF = OCF;
	SetOCF();
	assert (updateOCF == OCF);
	DEBUGREC_ON
#endif
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

bool C4Object::At(int32_t ctx, int32_t cty) const
{
	if (Status) if (!Contained) if (Def)
				if (Inside<int32_t>(cty - (GetY() + Shape.GetY() - addtop()), 0, Shape.Hgt - 1 + addtop()))
					if (Inside<int32_t>(ctx - (GetX() + Shape.GetX()), 0, Shape.Wdt - 1))
						return true;
	return false;
}

bool C4Object::At(int32_t ctx, int32_t cty, DWORD &ocf) const
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

void C4Object::GetOCFForPos(int32_t ctx, int32_t cty, DWORD &ocf) const
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

void C4Object::DoExperience(int32_t change)
{
	const int32_t MaxExperience = 100000000;

	if (!Info) return;

	Info->Experience=Clamp<int32_t>(Info->Experience+change,0,MaxExperience);

	// Promotion check
	if (Info->Experience<MaxExperience)
		if (Info->Experience>=::DefaultRanks.Experience(Info->Rank+1))
			Promote(Info->Rank+1, false, false);
}

bool C4Object::Exit(int32_t iX, int32_t iY, int32_t iR, C4Real iXDir, C4Real iYDir, C4Real iRDir, bool fCalls)
{
	// 1. Exit the current container.
	// 2. Update Contents of container object and set Contained to nullptr.
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
	Contained=nullptr;
	// Position/motion
	fix_x=itofix(iX); fix_y=itofix(iY);
	fix_r=itofix(iR);
	BoundsCheck(fix_x, fix_y);
	xdir=iXDir; ydir=iYDir; rdir=iRDir;
	// Misc updates
	Mobile=true;
	InLiquid=false;
	CloseMenu(true);
	UpdateFace(true);
	SetOCF();
	// Object list callback (before script callbacks, because script callbacks may enter again)
	ObjectListChangeListener.OnObjectContainerChanged(this, pContainer, nullptr);
	// Engine calls
	if (fCalls) pContainer->Call(PSF_Ejection,&C4AulParSet(this));
	if (fCalls) Call(PSF_Departure,&C4AulParSet(pContainer));
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

	// No valid target or target is self
	if (!pTarget || (pTarget==this)) return false;
	// check if entrance is allowed
	if (!! Call(PSF_RejectEntrance, &C4AulParSet(pTarget))) return false;
	// check if we end up in an endless container-recursion
	for (C4Object *pCnt=pTarget->Contained; pCnt; pCnt=pCnt->Contained)
		if (pCnt==this) return false;
	// Check RejectCollect, if desired
	if (pfRejectCollect)
	{
		if (!!pTarget->Call(PSF_RejectCollection,&C4AulParSet(Def, this)))
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
	if (Menu)
	{
		CloseMenu(true);
		// CloseMenu might do bad stuff
		if (Contained || !Status || !pTarget->Status) return false;
	}
	SetOCF();
	// Set container
	Contained=pTarget;
	// Enter
	if (!Contained->Contents.Add(this, C4ObjectList::stContents))
	{
		Contained=nullptr;
		return false;
	}
	// Assume that the new container controls this object, if it cannot control itself (i.e.: Alive)
	// So it can be traced back who caused the damage, if a projectile hits its target
	if (!Alive)
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
	// Object list callback (before script callbacks, because script callbacks may exit again)
	ObjectListChangeListener.OnObjectContainerChanged(this, nullptr, Contained);
	// Collection call
	if (fCalls) pTarget->Call(PSF_Collection2,&C4AulParSet(this));
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Entrance call
	if (fCalls) Call(PSF_Entrance,&C4AulParSet(Contained));
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Success
	return true;
}

void C4Object::Fling(C4Real txdir, C4Real tydir, bool fAddSpeed)
{
	if (fAddSpeed) { txdir+=xdir/2; tydir+=ydir/2; }
	if (!ObjectActionTumble(this,(txdir<0),txdir,tydir))
		if (!ObjectActionJump(this,txdir,tydir,false))
		{
			xdir=txdir; ydir=tydir;
			Mobile=true;
			Action.t_attach&=~CNAT_Bottom;
		}
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

bool C4Object::Push(C4Real txdir, C4Real dforce, bool fStraighten)
{
	// Valid check
	if (!Status || !Def || Contained || !(OCF & OCF_Grab)) return false;
	// Grabbing okay, no pushing
	if (GetPropertyInt(P_Touchable)==2) return true;
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
	{
		if (Inside<int32_t>(GetR(),-StableRange,+StableRange))
		{
			rdir=0; // cheap way out
		}
		else
		{
			if (fix_r > Fix0) { if (rdir>-RotateAccel) rdir-=dforce; }
			else { if (rdir<+RotateAccel) rdir+=dforce; }
		}
	}

	// Mobilization check
	if (!!xdir || !!ydir || !!rdir) Mobile=true;

	// Stuck check
	if (!::Game.iTick35) if (txdir) if (!Def->NoHorizontalMove)
				if (ContactCheck(GetX(), GetY())) // Resets t_contact
				{
					GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_STUCK"),GetName()).getData(),this);
					Call(PSF_Stuck);
				}

	return true;
}

bool C4Object::Lift(C4Real tydir, C4Real dforce)
{
	// Valid check
	if (!Status || !Def || Contained) return false;
	// Mobilization check
	if (!Mobile)
		{ xdir=ydir=Fix0; Mobile=true; }
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
			GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_STUCK"),GetName()).getData(),this);
			Call(PSF_Stuck);
		}
	return true;
}

C4Object* C4Object::CreateContents(C4PropList * PropList)
{
	C4Object *nobj;
	if (!(nobj=Game.CreateObject(PropList,this,Owner))) return nullptr;
	if (!nobj->Enter(this)) { nobj->AssignRemoval(); return nullptr; }
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

static void DrawMenuSymbol(int32_t iMenu, C4Facet &cgo, int32_t iOwner)
{
	C4Facet ccgo;

	DWORD dwColor=0;
	if (ValidPlr(iOwner)) dwColor=::Players.Get(iOwner)->ColorDw;

	switch (iMenu)
	{
	case C4MN_Buy:
		::GraphicsResource.fctFlagClr.DrawClr(ccgo = cgo.GetFraction(75, 75), true, dwColor);
		::GraphicsResource.fctWealth.Draw(ccgo = cgo.GetFraction(100, 50, C4FCT_Left, C4FCT_Bottom));
		::GraphicsResource.fctArrow.Draw(ccgo = cgo.GetFraction(70, 70, C4FCT_Right, C4FCT_Center), false, 0);
		break;
	case C4MN_Sell:
		::GraphicsResource.fctFlagClr.DrawClr(ccgo = cgo.GetFraction(75, 75), true, dwColor);
		::GraphicsResource.fctWealth.Draw(ccgo = cgo.GetFraction(100, 50, C4FCT_Left, C4FCT_Bottom));
		::GraphicsResource.fctArrow.Draw(ccgo = cgo.GetFraction(70, 70, C4FCT_Right, C4FCT_Center), false, 1);
		break;
	}
}

bool C4Object::ActivateMenu(int32_t iMenu, int32_t iMenuSelect,
                            int32_t iMenuData, int32_t iMenuPosition,
                            C4Object *pTarget)
{
	// Variables
	C4FacetSurface fctSymbol;
	C4IDList ListItems;
	// Close any other menu
	if (Menu && Menu->IsActive()) if (!Menu->TryClose(true, false)) return false;
	// Create menu
	if (!Menu) Menu = new C4ObjectMenu; else Menu->ClearItems();
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
		// Init
		Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName()).getData(),this,C4MN_Extra_None,0,iMenu);
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
		DrawMenuSymbol(C4MN_Buy, fctSymbol, pTarget->Owner);
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
		DrawMenuSymbol(C4MN_Sell, fctSymbol, pTarget->Owner);
		Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName()).getData(),this,C4MN_Extra_Value,0,iMenu);
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
		Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName()).getData(),this,C4MN_Extra_None,0,iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Info:
		// Target by parameter
		if (!pTarget) break;
		// Create symbol & init menu
		fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,0,1);
		Menu->Init(fctSymbol, pTarget->GetName(), this, C4MN_Extra_None, 0, iMenu, C4MN_Style_Info);
		Menu->SetPermanent(true);
		Menu->SetAlignment(C4MN_Align_Free);
		C4Viewport *pViewport = ::Viewports.GetViewport(Controller); // Hackhackhack!!!
		if (pViewport) Menu->SetLocation((pTarget->GetX() + pTarget->Shape.GetX() + pTarget->Shape.Wdt + 10 - pViewport->GetViewX()) * pViewport->GetZoom(),
			                                 (pTarget->GetY() + pTarget->Shape.GetY() - pViewport->GetViewY()) * pViewport->GetZoom());
		// Add info item
		fctSymbol.Create(C4PictureSize, C4PictureSize); pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
		Menu->Add(pTarget->GetName(), fctSymbol, "", C4MN_Item_NoCount, nullptr, pTarget->GetInfoString().getData());
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
		if (!Menu->IsCloseQuerying()) { delete Menu; Menu=nullptr; } // protect menu deletion from recursive menu operation calls
	}
	return true;
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

BYTE C4Object::GetMomentum(C4Real &rxdir, C4Real &rydir) const
{
	rxdir=rydir=0;
	if (!Status || !Def) return 0;
	rxdir=xdir; rydir=ydir;
	return 1;
}

C4Real C4Object::GetSpeed() const
{
	C4Real cobjspd=Fix0;
	if (xdir<0) cobjspd-=xdir; else cobjspd+=xdir;
	if (ydir<0) cobjspd-=ydir; else cobjspd+=ydir;
	return cobjspd;
}

StdStrBuf C4Object::GetDataString()
{
	StdStrBuf Output;
	// Type
	Output.AppendFormat(LoadResStr("IDS_CNS_TYPE"),GetName(),Def->id.ToString());
	// Owner
	if (ValidPlr(Owner))
	{
		Output.Append(LineFeed);
		Output.AppendFormat(LoadResStr("IDS_CNS_OWNER"),::Players.Get(Owner)->GetName());
	}
	// Contents
	if (Contents.ObjectCount())
	{
		Output.Append(LineFeed);
		Output.Append(LoadResStr("IDS_CNS_CONTENTS"));
		Output.Append(Contents.GetNameList(::Definitions));
	}
	// Action
	if (GetAction())
	{
		Output.Append(LineFeed);
		Output.Append(LoadResStr("IDS_CNS_ACTION"));
		Output.Append(GetAction()->GetName());
	}
	// Properties
	Output.Append(LineFeed);
	Output.Append(LoadResStr("IDS_CNS_PROPERTIES"));
	Output.Append(LineFeed "  ");
	AppendDataString(&Output, LineFeed "  ");
	// Effects
	if (pEffects)
	{
		Output.Append(LineFeed);
		Output.Append(LoadResStr("IDS_CNS_EFFECTS"));
		Output.Append(": ");
	}
	for (C4Effect *pEffect = pEffects; pEffect; pEffect = pEffect->pNext)
	{
		Output.Append(LineFeed);
		// Effect name
		Output.AppendFormat("  %s: Priority %d, Interval %d", pEffect->GetName(), pEffect->iPriority, pEffect->iInterval);
	}

	StdStrBuf Output2;
	C4ValueNumbers numbers;
	DecompileToBuf_Log<StdCompilerINIWrite>(mkNamingAdapt(mkInsertAdapt(mkParAdapt(*this, &numbers),
	                                                                    mkNamingAdapt(numbers, "Values"), false),
	                                                      "Object"), &Output2, "C4Object::GetDataString");
	Output.Append(LineFeed);
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

	// call to object
	Call(PSF_Promotion);

	StartSoundEffect("UI::Trumpet",false,100,this);
	return true;
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

bool C4Object::SetPhase(int32_t iPhase)
{
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return false;
	const int32_t length = pActionDef->GetPropertyInt(P_Length);
	Action.Phase=Clamp<int32_t>(iPhase,0,length);
	Action.PhaseDelay = 0;
	return true;
}

void C4Object::Draw(C4TargetFacet &cgo, int32_t iByPlayer, DrawMode eDrawMode, float offX, float offY)
{
#ifndef USE_CONSOLE
	C4Facet ccgo;

	// Status
	if (!Status || !Def) return;

	// visible?
	if (!IsVisible(iByPlayer, !!eDrawMode)) return;

	// Set up custom uniforms.
	auto uniform_popper = pDraw->scriptUniform.Push(this);

	// Line
	if (Def->Line) { DrawLine(cgo, iByPlayer); return; }

	// background particles (bounds not checked)
	if (BackParticles) BackParticles->Draw(cgo, this);

	// Object output position
	float newzoom = cgo.Zoom;
	if (eDrawMode!=ODM_Overlay)
	{
		if (!GetDrawPosition(cgo, offX, offY, newzoom)) return;
	}
	ZoomDataStackItem zdsi(newzoom);

	bool fYStretchObject=false;
	C4PropList* pActionDef = GetAction();
	if (pActionDef)
		if (pActionDef->GetPropertyInt(P_FacetTargetStretch))
			fYStretchObject=true;

	// Set audibility
	if (!eDrawMode) SetAudibilityAt(cgo, GetX(), GetY(), iByPlayer);

	// Output boundary
	if (!fYStretchObject && !eDrawMode && !(Category & C4D_Parallax))
	{
		// For actions with a custom facet set, check against that action facet. Otherwise (or with oversize objects), just check against shape.
		if (pActionDef && fix_r == Fix0 && !pActionDef->GetPropertyInt(P_FacetBase) && Con <= FullCon && Action.Facet.Wdt)
		{
			// active
			if ( !Inside<float>(offX+Shape.GetX()+Action.FacetX,cgo.X-Action.Facet.Wdt,cgo.X+cgo.Wdt)
			     || (!Inside<float>(offY+Shape.GetY()+Action.FacetY,cgo.Y-Action.Facet.Hgt,cgo.Y+cgo.Hgt)) )
				{
					if (FrontParticles && !Contained) FrontParticles->Draw(cgo, this);
					return;
				}
		}
		else
			// idle
			if ( !Inside<float>(offX+Shape.GetX(),cgo.X-Shape.Wdt,cgo.X+cgo.Wdt)
			     || (!Inside<float>(offY+Shape.GetY(),cgo.Y-Shape.Hgt,cgo.Y+cgo.Hgt)) )
				{
					if (FrontParticles && !Contained) FrontParticles->Draw(cgo, this);
					return; 
				}
	}

	// ensure correct color is set
	if (GetGraphics()->Type == C4DefGraphics::TYPE_Bitmap)
		if (GetGraphics()->Bmp.BitmapClr) GetGraphics()->Bmp.BitmapClr->SetClr(Color);

	// Debug Display //////////////////////////////////////////////////////////////////////
	if (::GraphicsSystem.ShowCommand && !eDrawMode)
	{
		C4Command *pCom;
		int32_t ccx=GetX(),ccy=GetY();
		float offX1, offY1, offX2, offY2, newzoom;
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
				if(GetDrawPosition(cgo, ccx, ccy, cgo.Zoom, offX1, offY1, newzoom) &&
				   GetDrawPosition(cgo, pCom->Tx._getInt(), pCom->Ty, cgo.Zoom, offX2, offY2, newzoom))
				{
					ZoomDataStackItem zdsi(newzoom);
					pDraw->DrawLineDw(cgo.Surface,offX1,offY1,offX2,offY2,C4RGB(0xca,0,0));
					pDraw->DrawFrameDw(cgo.Surface,offX2-1,offY2-1,offX2+1,offY2+1,C4RGB(0xca,0,0));
				}

				ccx=pCom->Tx._getInt(); ccy=pCom->Ty;
				// Message
				iMoveTos++; szCommand[0]=0;
				break;
			case C4CMD_Put:
				sprintf(szCommand,"%s %s to %s",CommandName(pCom->Command),pCom->Target2 ? pCom->Target2->GetName() : pCom->Data ? pCom->Data.GetDataString().getData() : "Content",pCom->Target ? pCom->Target->GetName() : "");
				break;
			case C4CMD_Buy: case C4CMD_Sell:
				sprintf(szCommand,"%s %s at %s",CommandName(pCom->Command),pCom->Data.GetDataString().getData(),pCom->Target ? pCom->Target->GetName() : "closest base");
				break;
			case C4CMD_Acquire:
				sprintf(szCommand,"%s %s",CommandName(pCom->Command),pCom->Data.GetDataString().getData());
				break;
			case C4CMD_Call:
				sprintf(szCommand,"%s %s in %s",CommandName(pCom->Command),pCom->Text->GetCStr(),pCom->Target ? pCom->Target->GetName() : "(null)");
				break;
			case C4CMD_None:
				szCommand[0]=0;
				break;
			case C4CMD_Transfer:
				// Path
				if(GetDrawPosition(cgo, ccx, ccy, cgo.Zoom, offX1, offY1, newzoom) &&
				   GetDrawPosition(cgo, pCom->Tx._getInt(), pCom->Ty, cgo.Zoom, offX2, offY2, newzoom))
				{
					ZoomDataStackItem zdsi(newzoom);
					pDraw->DrawLineDw(cgo.Surface,offX1,offY1,offX2,offY2,C4RGB(0,0xca,0));
					pDraw->DrawFrameDw(cgo.Surface,offX2-1,offY2-1,offX2+1,offY2+1,C4RGB(0,0xca,0));
				}

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
		pDraw->TextOut(Cmds.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,offX,offY+Shape.GetY()-10-cmhgt,C4Draw::DEFAULT_MESSAGE_COLOR,ACenter);
	}
	// Debug Display ///////////////////////////////////////////////////////////////////////////////

	// Don't draw (show solidmask)
	if (::GraphicsSystem.Show8BitSurface != 0)
		if (SolidMask.Wdt)
		{
			// DrawSolidMask(cgo); - no need to draw it, because the 8bit-surface will be shown
			return;
		}

	// Contained check
	if (Contained && !eDrawMode) return;

	// Visibility inside FoW
	const C4FoWRegion* pOldFoW = pDraw->GetFoW();
	if(pOldFoW && (Category & C4D_IgnoreFoW))
		pDraw->SetFoW(nullptr);

	// color modulation (including construction sign...)
	if (ColorMod != 0xffffffff || BlitMode) if (!eDrawMode) PrepareDrawing();

	// Not active or rotated: BaseFace only
	if (!pActionDef)
	{
		DrawFace(cgo, offX, offY);
	}

	// Active
	else
	{
		// FacetBase
		if (pActionDef->GetPropertyInt(P_FacetBase) || GetGraphics()->Type != C4DefGraphics::TYPE_Bitmap)
			DrawFace(cgo, offX, offY, 0, Action.DrawDir);

		// Special: stretched action facet
		if (Action.Facet.Surface && pActionDef->GetPropertyInt(P_FacetTargetStretch))
		{
			if (Action.Target)
				pDraw->Blit(Action.Facet.Surface,
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
	if (eDrawMode!=ODM_BaseOnly) 
	{
		if (FrontParticles)
			FrontParticles->Draw(cgo, this);
	}

	// Debug Display ////////////////////////////////////////////////////////////////////////
	if (::GraphicsSystem.ShowVertices) if (eDrawMode!=ODM_BaseOnly)
		{
			int32_t cnt;
			if (Shape.VtxNum>1)
				for (cnt=0; cnt<Shape.VtxNum; cnt++)
				{
					DrawVertex(cgo,
					           offX+Shape.VtxX[cnt],
					           offY+Shape.VtxY[cnt],
					           (Shape.VtxCNAT[cnt] & CNAT_NoCollision) ? C4RGB(0, 0, 0xff) : (Mobile ? C4RGB(0xff, 0, 0) : C4RGB(0xef, 0xef, 0)),
					           Shape.VtxContactCNAT[cnt]);
				}
		}

	if (::GraphicsSystem.ShowEntrance) if (eDrawMode!=ODM_BaseOnly)
		{
			if (OCF & OCF_Entrance)
				pDraw->DrawFrameDw(cgo.Surface,offX+Def->Entrance.x,
				                             offY+Def->Entrance.y,
				                             offX+Def->Entrance.x+Def->Entrance.Wdt-1,
				                             offY+Def->Entrance.y+Def->Entrance.Hgt-1,
				                             C4RGB(0, 0, 0xff));
			if (OCF & OCF_Collection)
				pDraw->DrawFrameDw(cgo.Surface,offX+Def->Collection.x,
				                             offY+Def->Collection.y,
				                             offX+Def->Collection.x+Def->Collection.Wdt-1,
				                             offY+Def->Collection.y+Def->Collection.Hgt-1,
				                             C4RGB(0xca, 0, 0));
		}

	if (::GraphicsSystem.ShowAction) if (eDrawMode!=ODM_BaseOnly)
		{
			if (pActionDef)
			{
				StdStrBuf str;
				str.Format("%s (%d)",pActionDef->GetName(),Action.Phase);
				int32_t cmwdt,cmhgt; ::GraphicsResource.FontRegular.GetTextExtent(str.getData(),cmwdt,cmhgt,true);
				pDraw->TextOut(str.getData(), ::GraphicsResource.FontRegular,
				                           1.0, cgo.Surface, offX, offY + Shape.GetY() - cmhgt,
				                           InLiquid ? 0xfa0000FF : C4Draw::DEFAULT_MESSAGE_COLOR, ACenter);
			}
		}
	// Debug Display ///////////////////////////////////////////////////////////////////////

	// Restore visibility inside FoW
	if (pOldFoW) pDraw->SetFoW(pOldFoW);
#endif
}

void C4Object::DrawTopFace(C4TargetFacet &cgo, int32_t iByPlayer, DrawMode eDrawMode, float offX, float offY)
{
#ifndef USE_CONSOLE
	// Status
	if (!Status || !Def) return;
	// visible?
	if (!IsVisible(iByPlayer, eDrawMode==ODM_Overlay)) return;
	// target pos (parallax)
	float newzoom = cgo.Zoom;
	if (eDrawMode!=ODM_Overlay) GetDrawPosition(cgo, offX, offY, newzoom);
	ZoomDataStackItem zdsi(newzoom);
	// TopFace
	if (!(TopFace.Surface || (OCF & OCF_Construct))) return;
	// Output bounds check
	if (!Inside<float>(offX, cgo.X - Shape.Wdt, cgo.X + cgo.Wdt)
	    || !Inside<float>(offY, cgo.Y - Shape.Hgt, cgo.Y + cgo.Hgt))
		return;
	// Don't draw (show solidmask)
	if (::GraphicsSystem.Show8BitSurface != 0 && SolidMask.Wdt) return;
	// Contained
	if (Contained) if (eDrawMode!=ODM_Overlay) return;
	// Construction sign
	if (OCF & OCF_Construct && fix_r == Fix0)
		if (eDrawMode!=ODM_BaseOnly)
		{
			C4Facet &fctConSign = ::GraphicsResource.fctConstruction;
			pDraw->Blit(fctConSign.Surface,
			              fctConSign.X, fctConSign.Y,
			              fctConSign.Wdt, fctConSign.Hgt,
			              cgo.Surface,
			              offX + Shape.GetX(), offY + Shape.GetY() + Shape.Hgt - fctConSign.Hgt,
			              fctConSign.Wdt, fctConSign.Hgt, true);
		}
	if(TopFace.Surface)
	{
		// FacetTopFace: Override TopFace.GetX()/GetY()
		C4PropList* pActionDef = GetAction();
		if (pActionDef && pActionDef->GetPropertyInt(P_FacetTopFace))
		{
			int32_t iPhase = Action.Phase;
			if (pActionDef->GetPropertyInt(P_Reverse)) iPhase = pActionDef->GetPropertyInt(P_Length) - 1 - Action.Phase;
			TopFace.X = pActionDef->GetPropertyInt(P_X) + Def->TopFace.x + pActionDef->GetPropertyInt(P_Wdt) * iPhase;
			TopFace.Y = pActionDef->GetPropertyInt(P_Y) + Def->TopFace.y + pActionDef->GetPropertyInt(P_Hgt) * Action.DrawDir;
		}
		// ensure correct color is set
		if (GetGraphics()->Bmp.BitmapClr) GetGraphics()->Bmp.BitmapClr->SetClr(Color);
		// color modulation
		if (!eDrawMode) PrepareDrawing();
		// Draw top face bitmap
		if (Con!=FullCon && Def->GrowthType)
			// stretched
			pDraw->Blit(TopFace.Surface,
				            TopFace.X, TopFace.Y, TopFace.Wdt, TopFace.Hgt,
				            cgo.Surface,
				            offX + Shape.GetX() + float(Def->TopFace.tx * Con) / FullCon, offY + Shape.GetY() + float(Def->TopFace.ty * Con) / FullCon,
				            float(TopFace.Wdt * Con) / FullCon, float(TopFace.Hgt * Con) / FullCon,
				            true, pDrawTransform ? &C4DrawTransform(*pDrawTransform, offX, offY) : nullptr);
		else
			// normal
			pDraw->Blit(TopFace.Surface,
				            TopFace.X,TopFace.Y,
				            TopFace.Wdt,TopFace.Hgt,
				            cgo.Surface,
				            offX + Shape.GetX() + Def->TopFace.tx, offY + Shape.GetY() + Def->TopFace.ty,
				            TopFace.Wdt, TopFace.Hgt,
				            true, pDrawTransform ? &C4DrawTransform(*pDrawTransform, offX, offY) : nullptr);
	}
	// end of color modulation
	if (!eDrawMode) FinishedDrawing();
#endif
}

void C4Object::DrawLine(C4TargetFacet &cgo, int32_t at_player)
{
	// Nothing to draw if the object has less than two vertices
	if (Shape.VtxNum < 2)
		return;
#ifndef USE_CONSOLE
	// Audibility
	SetAudibilityAt(cgo, Shape.VtxX[0], Shape.VtxY[0], at_player);
	SetAudibilityAt(cgo, Shape.VtxX[Shape.VtxNum - 1], Shape.VtxY[Shape.VtxNum - 1], at_player);
	// additive mode?
	PrepareDrawing();
	// Draw line segments
	C4Value colorsV; GetProperty(P_LineColors, &colorsV);
	C4ValueArray *colors = colorsV.getArray();
	// TODO: Edge color (color1) is currently ignored.
	int32_t color0 = 0xFFFF00FF;// , color1 = 0xFFFF00FF; // use bright colors so author notices
	if (colors)
	{
		color0 = colors->GetItem(0).getInt();
	}

	std::vector<C4BltVertex> vertices;
	vertices.resize( (Shape.VtxNum - 1) * 2);
	for (int32_t vtx=0; vtx+1<Shape.VtxNum; vtx++)
	{
		DwTo4UB(color0, vertices[2*vtx].color);
		DwTo4UB(color0, vertices[2*vtx+1].color);

		vertices[2*vtx].ftx = Shape.VtxX[vtx] + cgo.X - cgo.TargetX;
		vertices[2*vtx].fty = Shape.VtxY[vtx] + cgo.Y - cgo.TargetY;
		vertices[2*vtx+1].ftx = Shape.VtxX[vtx+1] + cgo.X - cgo.TargetX;
		vertices[2*vtx+1].fty = Shape.VtxY[vtx+1] + cgo.Y - cgo.TargetY;
	}

	pDraw->PerformMultiLines(cgo.Surface, &vertices[0], vertices.size(), 1.0f, nullptr);

	// reset blit mode
	FinishedDrawing();
#endif
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

void C4Object::DrawPicture(C4Facet &cgo, bool fSelected, C4DrawTransform* transform)
{
	// Draw def picture with object color
	Def->Draw(cgo,fSelected,Color,this,0,0,transform);
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
				pGfxOvrl->DrawPicture(cgo, this, nullptr);

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

bool C4Object::AssignLightRange()
{
	if (!lightRange && !lightFadeoutRange) return true;

	UpdateLight();
	return true;
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

	if (pEffects) { delete pEffects; pEffects=nullptr; }
	if (pSolidMaskData) { delete pSolidMaskData; pSolidMaskData=nullptr; }
	if (Menu) delete Menu; Menu=nullptr;
	if (MaterialContents) delete MaterialContents; MaterialContents=nullptr;
	// clear commands!
	C4Command *pCom, *pNext;
	for (pCom=Command; pCom; pCom=pNext)
	{
		pNext=pCom->Next; delete pCom; pCom=pNext;
	}
	if (pDrawTransform) { delete pDrawTransform; pDrawTransform=nullptr; }
	if (pGfxOverlay) { delete pGfxOverlay; pGfxOverlay=nullptr; }
	if (pMeshInstance) { delete pMeshInstance; pMeshInstance = nullptr; }
}

bool C4Object::MenuCommand(const char *szCommand)
{
	// Native script execution
	if (!Def || !Status) return false;
	return !! ::AulExec.DirectExec(this, szCommand, "MenuCommand");
}

void C4Object::SetSolidMask(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY)
{
	// remove old
	if (pSolidMaskData) { delete pSolidMaskData; pSolidMaskData=nullptr; }
	// set new data
	SolidMask.Set(iX,iY,iWdt,iHgt,iTX,iTY);
	// re-put if valid
	if (CheckSolidMaskRect()) UpdateSolidMask(false);
}

void C4Object::SetHalfVehicleSolidMask(bool set)
{
	if (!pSolidMaskData) return;
	HalfVehicleSolidMask = set;
	pSolidMaskData->SetHalfVehicle(set);
}

bool C4Object::CheckSolidMaskRect()
{
	// Ensure SolidMask rect lies within bounds of SolidMask bitmap in definition
	CSurface8 *sfcGraphics = Def->pSolidMask;
	if (!sfcGraphics)
	{
		// no graphics to set solid in
		SolidMask.Set(0,0,0,0,0,0);
		return false;
	}
	SolidMask.Set(std::max<int32_t>(SolidMask.x,0), std::max<int32_t>(SolidMask.y,0),
	              std::min<int32_t>(SolidMask.Wdt,sfcGraphics->Wdt-SolidMask.x), std::min<int32_t>(SolidMask.Hgt, sfcGraphics->Hgt-SolidMask.y),
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
	// Update OCF
	SetOCF();
	// Menu
	CloseMenu(true);
	// Material contents
	if (MaterialContents) delete MaterialContents; MaterialContents=nullptr;
	// reset speed of staticback-objects
	if (Category & C4D_StaticBack)
	{
		xdir = ydir = 0;
	}
}

void C4Object::DrawSelectMark(C4TargetFacet &cgo) const
{
	// Status
	if (!Status) return;
	// No select marks in film playback
	if (Game.C4S.Head.Film && Game.C4S.Head.Replay) return;
	// target pos (parallax)
	float offX, offY, newzoom;
	GetDrawPosition(cgo, offX, offY, newzoom);
	// Output boundary
	if (!Inside<float>(offX, cgo.X, cgo.X + cgo.Wdt)
	    || !Inside<float>(offY, cgo.Y, cgo.Y + cgo.Hgt)) return;
	// Draw select marks
	float cox = offX + Shape.GetX() - cgo.X + cgo.X - 2;
	float coy = offY + Shape.GetY() - cgo.Y + cgo.Y - 2;
	GfxR->fctSelectMark.Draw(cgo.Surface,cox,coy,0);
	GfxR->fctSelectMark.Draw(cgo.Surface,cox+Shape.Wdt,coy,1);
	GfxR->fctSelectMark.Draw(cgo.Surface,cox,coy+Shape.Hgt,2);
	GfxR->fctSelectMark.Draw(cgo.Surface,cox+Shape.Wdt,coy+Shape.Hgt,3);
}

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

void C4Object::Resort()
{
	// Flag resort
	Unsorted=true;
	Game.fResortAnyObject = true;
	// Must not immediately resort - link change/removal would crash Game::ExecObjects
}

C4PropList* C4Object::GetAction() const
{
	C4Value value;
	GetProperty(P_Action, &value);
	return value.getPropList();
}

bool C4Object::SetAction(C4PropList * Act, C4Object *pTarget, C4Object *pTarget2, int32_t iCalls, bool fForce)
{
	C4Value vLastAction;
	GetProperty(P_Action, &vLastAction);
	C4PropList * LastAction = vLastAction.getPropList();
	int32_t iLastPhase=Action.Phase;
	C4Object *pLastTarget = Action.Target;
	C4Object *pLastTarget2 = Action.Target2;
	// No other action
	if (LastAction)
		if (LastAction->GetPropertyInt(P_NoOtherAction) && !fForce)
			if (Act != LastAction)
				return false;
	// Set animation on instance. Abort if the mesh does not have
	// such an animation.
	if (pMeshInstance)
	{
		if (Action.Animation) pMeshInstance->StopAnimation(Action.Animation);
		Action.Animation = nullptr;

		C4String* Animation = Act ? Act->GetPropertyStr(P_Animation) : nullptr;
		if (Animation)
		{
			// note that weight is ignored
			Action.Animation = pMeshInstance->PlayAnimation(Animation->GetData(), 0, nullptr, new C4ValueProviderAction(this), new C4ValueProviderConst(itofix(1)), true);
		}
	}
	// Stop previous act sound
	if (LastAction)
		if (Act != LastAction)
			if (LastAction->GetPropertyStr(P_Sound))
				StopSoundEffect(LastAction->GetPropertyStr(P_Sound)->GetCStr(),this);
	// Unfullcon objects no action
	if (Con<FullCon)
		if (!Def->IncompleteActivity)
			Act = nullptr;
	// Reset action time on change
	if (Act!=LastAction)
	{
		Action.Time=0;
		// reset action data and targets if procedure is changed
		if ((Act ? Act->GetPropertyP(P_Procedure) : -1)
			!= (LastAction ? LastAction->GetPropertyP(P_Procedure) : -1))
		{
			Action.Data = 0;
			Action.Target = nullptr;
			Action.Target2 = nullptr;
		}
	}
	// Set new action
	SetProperty(P_Action, C4VPropList(Act));
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
	if (Act)
		if (Act != LastAction)
			if (Act->GetPropertyStr(P_Sound))
				StartSoundEffect(Act->GetPropertyStr(P_Sound)->GetCStr(),+1,100,this);
	// Reset OCF
	SetOCF();
	// issue calls
	// Execute EndCall for last action
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
	// Execute AbortCall for last action
	if (iCalls & SAC_AbortCall && !fForce)
		if (LastAction)
		{
			if (LastAction->GetPropertyStr(P_AbortCall))
			{
				C4Def *pOldDef = Def;
				if (pLastTarget && !pLastTarget->Status) pLastTarget = nullptr;
				if (pLastTarget2 && !pLastTarget2->Status) pLastTarget2 = nullptr;
				Call(LastAction->GetPropertyStr(P_AbortCall)->GetCStr(), &C4AulParSet(iLastPhase, pLastTarget, pLastTarget2));
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}
	// Execute StartCall for new action
	if (iCalls & SAC_StartCall)
		if (Act)
		{
			if (Act->GetPropertyStr(P_StartCall))
			{
				C4Def *pOldDef = Def;
				Call(Act->GetPropertyStr(P_StartCall)->GetCStr());
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}

	C4Def *pOldDef = Def;
	Call(PSF_OnActionChanged, &C4AulParSet(LastAction ? LastAction->GetName() : "Idle"));
	if (Def != pOldDef || !Status) return true;

	return true;
}

void C4Object::UpdateActionFace()
{
	// Default: no action face
	Action.Facet.Default();
	// Active: get action facet from action definition
	C4PropList* pActionDef = GetAction();
	if (pActionDef)
	{
		if (pActionDef->GetPropertyInt(P_Wdt)>0)
		{
			Action.Facet.Set(GetGraphics()->GetBitmap(Color),
			                 pActionDef->GetPropertyInt(P_X),pActionDef->GetPropertyInt(P_Y),
			                 pActionDef->GetPropertyInt(P_Wdt),pActionDef->GetPropertyInt(P_Hgt));
			Action.FacetX=pActionDef->GetPropertyInt(P_OffX);
			Action.FacetY=pActionDef->GetPropertyInt(P_OffY);
		}
	}
}

bool C4Object::SetActionByName(C4String *ActName,
                               C4Object *pTarget, C4Object *pTarget2,
                               int32_t iCalls, bool fForce)
{
	assert(ActName);
	// If we get the null string or ActIdle by name, set ActIdle
	if (!ActName || ActName == &Strings.P[P_Idle])
		return SetAction(nullptr,nullptr,nullptr,iCalls,fForce);
	C4Value ActMap; GetProperty(P_ActMap, &ActMap);
	if (!ActMap.getPropList()) return false;
	C4Value Action; ActMap.getPropList()->GetPropertyByS(ActName, &Action);
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
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return;
	// Invalid direction
	if (!Inside<int32_t>(iDir,0,pActionDef->GetPropertyInt(P_Directions)-1)) return;
	// Execute turn action
	if (iDir != Action.Dir)
		if (pActionDef->GetPropertyStr(P_TurnAction))
			{ SetActionByName(pActionDef->GetPropertyStr(P_TurnAction)); }
	// Set dir
	Action.Dir=iDir;
	// update by flipdir?
	if (pActionDef->GetPropertyInt(P_FlipDir))
		UpdateFlipDir();
	else
		Action.DrawDir=iDir;
}

int32_t C4Object::GetProcedure() const
{
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return -1;
	return pActionDef->GetPropertyP(P_Procedure);
}

void GrabLost(C4Object *cObj, C4Object *prev_target)
{
	// Grab lost script call on target (quite hacky stuff...)
	if (prev_target && prev_target->Status) prev_target->Call(PSF_GrabLost);
	// Clear commands down to first PushTo (if any) in command stack
	for (C4Command *pCom=cObj->Command; pCom; pCom=pCom->Next)
		if (pCom->Next && pCom->Next->Command==C4CMD_PushTo)
		{
			cObj->ClearCommand(pCom);
			break;
		}
}

static void DoGravity(C4Object *cobj);

void C4Object::NoAttachAction()
{
	// Active objects
	if (GetAction())
	{
		int32_t iProcedure = GetProcedure();
		C4Object *prev_target = Action.Target;
		// Scaling upwards: corner scale
		if (iProcedure == DFA_SCALE && Action.ComDir != COMD_Stop && ComDirLike(Action.ComDir, COMD_Up))
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Left && Action.Dir == DIR_Left)
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Right && Action.Dir == DIR_Right)
			if (ObjectActionCornerScale(this)) return;
		// Scaling and stopped: fall off to side (avoid zuppel)
		if ((iProcedure == DFA_SCALE) && (Action.ComDir == COMD_Stop))
		{
			if (Action.Dir == DIR_Left)
				{ if (ObjectActionJump(this,itofix(1),Fix0,false)) return; }
			else
				{ if (ObjectActionJump(this,itofix(-1),Fix0,false)) return; }
		}
		// Pushing: grab loss
		if (iProcedure==DFA_PUSH) GrabLost(this, prev_target);
		// Else jump
		ObjectActionJump(this,xdir,ydir,false);
	}
	// Inactive objects, simple mobile natural gravity
	else
	{
		DoGravity(this);
		Mobile=true;
	}
}

void C4Object::ContactAction()
{
	// Take certain action on contact. Evaluate t_contact-CNAT and Procedure.

	// Determine Procedure
	C4PropList* pActionDef = GetAction();
	if (!pActionDef) return;
	int32_t iProcedure=pActionDef->GetPropertyP(P_Procedure);
	int32_t fDisabled=pActionDef->GetPropertyInt(P_ObjectDisabled);

	//------------------------------- Hit Bottom ---------------------------------------------
	if (t_contact & CNAT_Bottom)
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			if (ydir < 0) return;
			// Jump: FlatHit / HardHit / Walk
			if ((OCF & OCF_HitSpeed4) || fDisabled)
				if (ObjectActionFlat(this,Action.Dir)) return;
			if (OCF & OCF_HitSpeed3)
				if (ObjectActionKneel(this)) return;
			ObjectActionWalk(this);
			ydir = 0;
			return;
		case DFA_SCALE:
			// Scale down: stand
			if (ComDirLike(Action.ComDir, COMD_Down))
			{
				ObjectActionStand(this);
				return;
			}
			break;
		case DFA_DIG:
			// no special action
			break;
		case DFA_SWIM:
			// Try corner scale out
			if (!GBackSemiSolid(GetX(),GetY()-1+Def->Float*Con/FullCon-1))
				if (ObjectActionCornerScale(this)) return;
			break;
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
				if (ObjectActionHangle(this))
				{
					SetDir(Action.Dir == DIR_Left ? DIR_Right : DIR_Left);
					return;
				}
				Action.ComDir=COMD_Stop;
			}
			break;
		case DFA_FLIGHT:
			// Jump: Try hangle, else bounce off
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
				{ ObjectActionTumble(this, Action.Dir, xdir, ydir); break; }
			if (ObjectActionHangle(this)) return;
			break;
		case DFA_DIG:
			// No action
			break;
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
				{ ObjectActionTumble(this, DIR_Left, xdir, ydir); break; }
			// Else
			else if (!ComDirLike(Action.ComDir, COMD_Right) && ObjectActionScale(this,DIR_Left)) return;
			break;
		case DFA_WALK:
			// Walk: Try scale
			if (ComDirLike(Action.ComDir, COMD_Left))
			{
				if (ObjectActionScale(this,DIR_Left))
				{
					ydir = C4REAL100(-1);
					return;
				}
			}
			// Heading away from solid
			if (ComDirLike(Action.ComDir, COMD_Right))
			{
				// Slide off
				ObjectActionJump(this,xdir/2,ydir,false);
			}
			return;
		case DFA_SWIM:
			// Only scale if swimming at the surface
			if (!GBackSemiSolid(GetX(),GetY()-1+Def->Float*Con/FullCon-1))
			{
				// Try scale, only if swimming at the surface.
				if (ComDirLike(Action.ComDir, COMD_Left))
					if (ObjectActionScale(this,DIR_Left)) return;
				// Try corner scale out
				if (ObjectActionCornerScale(this)) return;
			}
			return;
		case DFA_HANGLE:
			// Hangle: Try scale
			if (ObjectActionScale(this,DIR_Left))
			{
				ydir = C4REAL100(1);
				return;
			}
			return;
		case DFA_DIG:
			// Dig: no action
			break;
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
				{ ObjectActionTumble(this, DIR_Right, xdir, ydir); break; }
			// Else Scale
			else if (!ComDirLike(Action.ComDir, COMD_Left) && ObjectActionScale(this,DIR_Right)) return;
			break;
		case DFA_WALK:
			// Walk: Try scale
			if (ComDirLike(Action.ComDir, COMD_Right))
			{
				if (ObjectActionScale(this,DIR_Right))
				{
					ydir = C4REAL100(-1);
					return;
				}
			}
			// Heading away from solid
			if (ComDirLike(Action.ComDir, COMD_Left))
			{
				// Slide off
				ObjectActionJump(this,xdir/2,ydir,false);
			}
			return;
		case DFA_SWIM:
			// Only scale if swimming at the surface
			if (!GBackSemiSolid(GetX(),GetY()-1+Def->Float*Con/FullCon-1))
			{
				// Try scale
				if (ComDirLike(Action.ComDir, COMD_Right))
					if (ObjectActionScale(this,DIR_Right)) return;
				// Try corner scale out
				if (ObjectActionCornerScale(this)) return;
			}
			return;
		case DFA_HANGLE:
			// Hangle: Try scale
			if (ObjectActionScale(this,DIR_Right))
			{
				ydir = C4REAL100(1);
				return;
			}
			return;
		case DFA_DIG:
			// Dig: no action
			break;
		}
	}

	//---------------------------- Unresolved Cases ---------------------------------------

	// Flight stuck
	if (iProcedure==DFA_FLIGHT)
	{
		// Enforce slide free (might slide through tiny holes this way)
		if (!ydir)
		{
			int fAllowDown = !(t_contact & CNAT_Bottom);
			if (t_contact & CNAT_Right)
			{
				ForcePosition(fix_x - 1, fix_y + fAllowDown);
				xdir=ydir=0;
			}
			if (t_contact & CNAT_Left)
			{
				ForcePosition(fix_x + 1, fix_y + fAllowDown);
				xdir=ydir=0;
			}
		}
		if (!xdir)
		{
			if (t_contact & CNAT_Top)
			{
				ForcePosition(fix_x, fix_y + 1);
				xdir=ydir=0;
			}
		}
	}
}

void Towards(C4Real &val, C4Real target, C4Real step)
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
				// special for roof above Clonk: The nonmoving roof is started at bridgelength before the Clonk
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

static void DoGravity(C4Object *cobj)
{
	// Floatation in liquids
	if (cobj->InLiquid && cobj->Def->Float)
	{
		cobj->ydir-=GravAccel * C4REAL100(80);
		if (cobj->ydir<C4REAL100(-160)) cobj->ydir=C4REAL100(-160);
		if (cobj->xdir<-FloatFriction) cobj->xdir+=FloatFriction;
		if (cobj->xdir>+FloatFriction) cobj->xdir-=FloatFriction;
		if (cobj->rdir<-FloatFriction) cobj->rdir+=FloatFriction;
		if (cobj->rdir>+FloatFriction) cobj->rdir-=FloatFriction;
		// Reduce upwards speed when about to leave liquid to prevent eternal moving up and down.
		// Check both for no liquid one and two pixels above the surface, because the object could
		// skip one pixel as its speed can be more than 100.
		int32_t y_float = cobj->GetY() - 1 + cobj->Def->Float * cobj->GetCon() / FullCon;
		if (!GBackLiquid(cobj->GetX(), y_float - 1) || !GBackLiquid(cobj->GetX(), y_float - 2))
			if (cobj->ydir < 0)
			{
				// Reduce the upwards speed and set to zero for small values to prevent fluctuations.
				cobj->ydir /= 2;
				if (Abs(cobj->ydir) < C4REAL100(10))
					cobj->ydir = 0;
			}
	}
	// Free fall gravity
	else if (~cobj->Category & C4D_StaticBack)
		cobj->ydir+=GravAccel;
}

void StopActionDelayCommand(C4Object *cobj)
{
	ObjectComStop(cobj);
	cobj->AddCommand(C4CMD_Wait,nullptr,0,0,50);
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
	C4Real iTXDir;
	C4Real lftspeed,tydir;
	int32_t iTargetX;
	int32_t iPushRange,iPushDistance;

	// Standard phase advance
	int32_t iPhaseAdvance=1;

	// Upright attachment check
	if (!Mobile)
		if (Def->UprightAttach)
			if (Inside<int32_t>(GetR(),-StableRange,+StableRange))
			{
				Action.t_attach|=Def->UprightAttach;
				Mobile=true;
			}

	C4PropList* pActionDef = GetAction();
	// No IncompleteActivity? Reset action if there was one
	if (!(OCF & OCF_FullCon) && !Def->IncompleteActivity && pActionDef)
	{
		SetAction(nullptr);
		pActionDef = nullptr;
	}

	// InLiquidAction check
	if (InLiquid)
		if (pActionDef && pActionDef->GetPropertyStr(P_InLiquidAction))
		{
			SetActionByName(pActionDef->GetPropertyStr(P_InLiquidAction));
			pActionDef = GetAction();
		}
	
	// Idle objects do natural gravity only
	if (!pActionDef)
	{
		Action.t_attach = CNAT_None;
		if (Mobile) DoGravity(this);
		return;
	}

	C4Real fWalk,fMove;

	// Action time advance
	Action.Time++;

	C4Value Attach;
	pActionDef->GetProperty(P_Attach, &Attach);
	if (Attach.GetType() != C4V_Nil)
	{
		Action.t_attach = Attach.getInt();
	}
	else switch (pActionDef->GetPropertyP(P_Procedure))
	{
	case DFA_SCALE:
		if (Action.Dir == DIR_Left)  Action.t_attach = CNAT_Left;
		if (Action.Dir == DIR_Right) Action.t_attach = CNAT_Right;
		break;
	case DFA_HANGLE:
		Action.t_attach = CNAT_Top;
		break;
	case DFA_WALK:
	case DFA_KNEEL:
	case DFA_THROW:
	case DFA_BRIDGE:
	case DFA_PUSH:
	case DFA_PULL:
	case DFA_DIG:
		Action.t_attach = CNAT_Bottom;
		break;
	default:
		Action.t_attach = CNAT_None;
	}

	// if an object is in controllable state, so it can be assumed that if it dies later because of NO_OWNER's cause,
	// it has been its own fault and not the fault of the last one who threw a flint on it
	// do not reset for burning objects to make sure the killer is set correctly if they fall out of the map while burning
	if (!pActionDef->GetPropertyInt(P_ObjectDisabled) && pActionDef->GetPropertyP(P_Procedure) != DFA_FLIGHT && !OnFire)
		LastEnergyLossCausePlayer = NO_OWNER;

	// Handle Default Action Procedure: evaluates Procedure and Action.ComDir
	// Update xdir,ydir,Action.Dir,attachment,iPhaseAdvance
	int32_t dir = Action.Dir;
	C4Real accel = C4REAL100(pActionDef->GetPropertyInt(P_Accel));
	C4Real decel = accel;
	{
		C4Value decel_val;
		pActionDef->GetProperty(P_Decel, &decel_val);
		if (decel_val.GetType() != C4V_Nil)
			decel = C4REAL100(decel_val.getInt());
	}
	C4Real limit = C4REAL100(pActionDef->GetPropertyInt(P_Speed));

	switch (pActionDef->GetPropertyP(P_Procedure))
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_WALK:
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			// breaaak!!!
			if (dir == DIR_Right)
				xdir-=decel;
			else
				xdir-=accel;
			if (xdir<-limit) xdir=-limit;
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			if (dir == DIR_Left)
				xdir+=decel;
			else
				xdir+=accel;
			if (xdir>+limit) xdir=+limit;
			break;
		case COMD_Stop: case COMD_Up: case COMD_Down:
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			break;
		}
		iPhaseAdvance=0;
		if (xdir<0)
		{
			if (dir != DIR_Left) { SetDir(DIR_Left); xdir = -1; }
			iPhaseAdvance=-fixtoi(xdir*10);
		}
		if (xdir>0)
		{
			if (dir != DIR_Right) { SetDir(DIR_Right); xdir = 1; }
			iPhaseAdvance=+fixtoi(xdir*10);
		}

		Mobile=true;
		// object is rotateable? adjust to ground, if in horizontal movement or not attached to the center vertex
		if (Def->Rotateable && Shape.AttachMat != MNone && (!!xdir || Def->Shape.VtxX[Shape.iAttachVtx]))
			AdjustWalkRotation(20, 20, 100);
		else
			rdir=0;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_KNEEL:
		ydir=0;
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_SCALE:
	{
		int ComDir = Action.ComDir;
		if (Shape.CheckScaleToWalk(GetX(), GetY()))
		{
			ObjectActionWalk(this);
			return;
		}
		if ((Action.Dir == DIR_Left && ComDir == COMD_Left) || (Action.Dir == DIR_Right && ComDir == COMD_Right))
		{
			ComDir = COMD_Up;
		}
		switch (ComDir)
		{
		case COMD_Up: case COMD_UpRight:  case COMD_UpLeft:
			if (ydir > 0) ydir -= decel;
			else ydir -= accel;
			if (ydir < -limit) ydir = -limit; break;
		case COMD_Down: case COMD_DownRight: case COMD_DownLeft:
			if (ydir < 0) ydir += decel;
			else ydir += accel;
			if (ydir > +limit) ydir = +limit; break;
		case COMD_Left: case COMD_Right: case COMD_Stop:
			if (ydir < 0) ydir += decel;
			if (ydir > 0) ydir -= decel;
			if ((ydir > -decel) && (ydir < +decel)) ydir = 0;
			break;
		}
		iPhaseAdvance=0;
		if (ydir<0) iPhaseAdvance=-fixtoi(ydir*14);
		if (ydir>0) iPhaseAdvance=+fixtoi(ydir*14);
		xdir=0;
		Mobile=true;
		break;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_HANGLE:
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			if (xdir > 0) xdir -= decel;
			else xdir -= accel;
			if (xdir < -limit) xdir = -limit;
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			if (xdir < 0) xdir += decel;
			else xdir += accel;
			if (xdir > +limit) xdir = +limit;
			break;
		case COMD_Up:
			if (Action.Dir == DIR_Left)
				if (xdir > 0) xdir -= decel;
				else xdir -= accel;
			else
				if (xdir < 0) xdir += decel;
				else xdir += accel;
			if (xdir < -limit) xdir = -limit;
			if (xdir > +limit) xdir = +limit;
			break;
		case COMD_Stop: case COMD_Down:
			if (xdir < 0) xdir += decel;
			if (xdir > 0) xdir -= decel;
			if ((xdir > -decel) && (xdir < +decel)) xdir = 0;
			break;
		}
		iPhaseAdvance=0;
		if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left); }
		if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
		ydir=0;
		Mobile=true;
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

		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			xdir -= std::max(std::min(limit + xdir, xdir > 0 ? decel : accel), itofix(0));
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			xdir += std::max(std::min(limit - xdir, xdir < 0 ? decel : accel), itofix(0));
			break;
		}

		// Gravity/mobile
		DoGravity(this);
		Mobile=true;
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_DIG:
	{
		int32_t smpx = GetX(), smpy = GetY();
		bool fAttachOK = false;
		if (Action.t_attach & CNAT_Bottom && Shape.Attach(smpx,smpy,CNAT_Bottom)) fAttachOK = true;
		else if (Action.t_attach & CNAT_Left && Shape.Attach(smpx,smpy,CNAT_Left)) { fAttachOK = true; }
		else if (Action.t_attach & CNAT_Right && Shape.Attach(smpx,smpy,CNAT_Right)) { fAttachOK = true; }
		else if (Action.t_attach & CNAT_Top && Shape.Attach(smpx,smpy,CNAT_Top)) fAttachOK = true;
		if (!fAttachOK)
			{ ObjectComStopDig(this); return; }
		iPhaseAdvance=fixtoi(itofix(40)*limit);

		if (xdir < 0) SetDir(DIR_Left); else if (xdir > 0) SetDir(DIR_Right);
		Action.t_attach=CNAT_None;
		Mobile=true;
		break;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_SWIM:
		// ComDir changes xdir/ydir
		switch (Action.ComDir)
		{
		case COMD_Up:       ydir-=accel; break;
		case COMD_UpRight:  ydir-=accel;  xdir+=accel; break;
		case COMD_Right:                  xdir+=accel; break;
		case COMD_DownRight:ydir+=accel;  xdir+=accel; break;
		case COMD_Down:     ydir+=accel; break;
		case COMD_DownLeft: ydir+=accel;  xdir-=accel; break;
		case COMD_Left:                   xdir-=accel; break;
		case COMD_UpLeft:   ydir-=accel;  xdir-=accel; break;
		case COMD_Stop:
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		}

		// Out of liquid check
		if (!InLiquid)
		{
			// Just above liquid: move down
			if (GBackLiquid(GetX(),GetY()+1+Def->Float*Con/FullCon-1)) ydir=+accel;
			// Free fall: walk
			else { ObjectActionWalk(this); return; }
		}

		// xdir/ydir bounds, don't apply if COMD_None
		if (Action.ComDir != COMD_None)
		{
			if (ydir<-limit) ydir=-limit; if (ydir>+limit) ydir=+limit;
			if (xdir>+limit) xdir=+limit; if (xdir<-limit) xdir=-limit;
		}
		// Surface dir bound
		if (!GBackLiquid(GetX(),GetY()-1+Def->Float*Con/FullCon-1)) if (ydir<0) ydir=0;
		// Dir, Phase, Attach
		if (xdir<0) SetDir(DIR_Left);
		if (xdir>0) SetDir(DIR_Right);
		iPhaseAdvance=fixtoi(limit*10);
		Mobile=true;

		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_THROW:
		Mobile=true;
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
		Mobile=true;
	}
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
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_DownLeft:   iTXDir=-limit; break;
		case COMD_UpLeft:  fStraighten=true;     iTXDir=-limit; break;
		case COMD_Right: case COMD_DownRight: iTXDir=+limit; break;
		case COMD_UpRight: fStraighten=true;     iTXDir=+limit; break;
		case COMD_Up:      fStraighten=true; break;
		case COMD_Stop: case COMD_Down:       iTXDir=0;       break;
		}
		// Push object
		if (!Action.Target->Push(iTXDir,accel,fStraighten))
			{ StopActionDelayCommand(this); return; }
		// Set target controller
		Action.Target->Controller=Controller;
		// ObjectAction got hold check
		iPushDistance = std::max(Shape.Wdt/2-8,0);
		iPushRange = iPushDistance + 10;
		int32_t sax,say,sawdt,sahgt;
		Action.Target->GetArea(sax,say,sawdt,sahgt);
		// Object lost
		if (!Inside(GetX()-sax,-iPushRange,sawdt-1+iPushRange)
		    || !Inside(GetY()-say,-iPushRange,sahgt-1+iPushRange))
		{
			C4Object *prev_target = Action.Target;
			// Wait command (why, anyway?)
			StopActionDelayCommand(this);
			// Grab lost action
			GrabLost(this, prev_target);
			// Done
			return;
		}
		// Follow object (full xdir reset)
		// Vertical follow: If object moves out at top, assume it's being pushed upwards and the Clonk must run after it
		if (GetY()-iPushDistance > say+sahgt && iTXDir) { if (iTXDir>0) sax+=sawdt/2; sawdt/=2; }
		// Horizontal follow
		iTargetX=Clamp(GetX(),sax-iPushDistance,sax+sawdt-1+iPushDistance);
		if (GetX()==iTargetX) xdir=0;
		else { if (GetX()<iTargetX) xdir=+limit; if (GetX()>iTargetX) xdir=-limit; }
		// Phase by XDir
		if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
		if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
		// No YDir
		ydir=0;
		Mobile=true;
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

		fWalk = limit;

		fMove = 0;
		if (Action.ComDir==COMD_Right) fMove = +fWalk;
		if (Action.ComDir==COMD_Left) fMove = -fWalk;

		iTXDir = fMove + fWalk * Clamp<int32_t>(iPullX-Action.Target->GetX(),-10,+10) / 10;

		// Push object
		if (!Action.Target->Push(iTXDir,accel,false))
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
		iPushDistance = std::max(Shape.Wdt/2-8,0);
		iPushRange = iPushDistance + 20;
		Action.Target->GetArea(sax,say,sawdt,sahgt);
		// Object lost
		if (!Inside(GetX()-sax,-iPushRange,sawdt-1+iPushRange)
		    || !Inside(GetY()-say,-iPushRange,sahgt-1+iPushRange))
		{
			// Remember target. Will be lost on changing action.
			C4Object *prev_target = Action.Target;
			// Wait command (why, anyway?)
			StopActionDelayCommand(this);
			// Grab lost action
			GrabLost(this, prev_target);
			// Lose target
			Action.Target=nullptr;
			// Done
			return;
		}

		// Move to pulling position
		xdir = fMove + fWalk * Clamp<int32_t>(iTargetX-GetX(),-10,+10) / 10;

		// Phase by XDir
		iPhaseAdvance=0;
		if (xdir<0) { iPhaseAdvance=-fixtoi(xdir*10); SetDir(DIR_Left);  }
		if (xdir>0) { iPhaseAdvance=+fixtoi(xdir*10); SetDir(DIR_Right); }
		// No YDir
		ydir=0;
		Mobile=true;

		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_LIFT:
		// Valid check
		if (!Action.Target) { SetAction(nullptr); return; }
		// Target lifting force
		lftspeed=itofix(2); tydir=0;
		switch (Action.ComDir)
		{
		case COMD_Up:   tydir=-lftspeed; break;
		case COMD_Stop: tydir=-GravAccel; break;
		case COMD_Down: tydir=+lftspeed; break;
		}
		// Lift object
		if (!Action.Target->Lift(tydir,C4REAL100(50)))
			{ SetAction(nullptr); return; }
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
		// ComDir changes xdir/ydir
		switch (Action.ComDir)
		{
		case COMD_Up:
			ydir-=accel;
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			break;
		case COMD_UpRight:  
			ydir-=accel; xdir+=accel; break;
		case COMD_Right:
			xdir+=accel; 
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		case COMD_DownRight:
			ydir+=accel; xdir+=accel; break;
		case COMD_Down: 
			ydir+=accel;
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			break;
		case COMD_DownLeft:
			ydir+=accel; xdir-=accel; break;
		case COMD_Left:
			xdir-=accel; 
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		case COMD_UpLeft:
			ydir-=accel; xdir-=accel; break;
		case COMD_Stop:
			if (xdir<0) xdir+=decel;
			if (xdir>0) xdir-=decel;
			if ((xdir>-decel) && (xdir<+decel)) xdir=0;
			if (ydir<0) ydir+=decel;
			if (ydir>0) ydir-=decel;
			if ((ydir>-decel) && (ydir<+decel)) ydir=0;
			break;
		}
		// xdir/ydir bounds, don't apply if COMD_None
		if (Action.ComDir != COMD_None)
		{
			if (ydir<-limit) ydir=-limit; if (ydir>+limit) ydir=+limit;
			if (xdir>+limit) xdir=+limit; if (xdir<-limit) xdir=-limit;
		}

		Mobile=true;
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
				SetAction(nullptr);
				Call(PSF_AttachTargetLost);
			}
			return;
		}

		// Target incomplete and no incomplete activity
		if (!(Action.Target->OCF & OCF_FullCon))
			if (!Action.Target->Def->IncompleteActivity)
				{ SetAction(nullptr); return; }

		// Force containment
		if (Action.Target->Contained!=Contained)
		{
			if (Action.Target->Contained)
				Enter(Action.Target->Contained);
			else
				Exit(GetX(),GetY(),GetR());
		}

		// Object might have detached in Enter/Exit call
		if (!Action.Target) break;

		// Move position (so objects on solidmask move)
		MovePosition(Action.Target->fix_x + Action.Target->Shape.VtxX[Action.Data&255]
		              -Shape.VtxX[Action.Data>>8] - fix_x,
		              Action.Target->fix_y + Action.Target->Shape.VtxY[Action.Data&255]
		              -Shape.VtxY[Action.Data>>8] - fix_y);
		// must zero motion...
		xdir=ydir=0;

		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case DFA_CONNECT:
		{
			bool fBroke=false;
			bool fLineChange = false;

			// Line destruction check: Target missing or incomplete
			if (!Action.Target || (Action.Target->Con<FullCon)) fBroke=true;
			if (!Action.Target2 || (Action.Target2->Con<FullCon)) fBroke=true;
			if (fBroke)
			{
				Call(PSF_OnLineBreak,&C4AulParSet(true));
				AssignRemoval();
				return;
			}

			// Movement by Target
			// Connect to attach vertex
			C4Value lineAttachV; C4ValueArray *lineAttach;
			Action.Target->GetProperty(P_LineAttach, &lineAttachV);
			lineAttach = lineAttachV.getArray();
			int32_t iConnectX1, iConnectY1;
			iConnectX1 = Action.Target->GetX();
			iConnectY1 = Action.Target->GetY();
			if (lineAttach)
			{
				iConnectX1 += lineAttach->GetItem(0).getInt();
				iConnectY1 += lineAttach->GetItem(1).getInt();
			}
			if ((iConnectX1!=Shape.VtxX[0]) || (iConnectY1!=Shape.VtxY[0]))
			{
				// Regular wrapping line
				if (Def->LineIntersect == 0)
					if (!Shape.LineConnect(iConnectX1,iConnectY1,0,+1,
											Shape.VtxX[0],Shape.VtxY[0])) fBroke=true;
				// No-intersection line
				if (Def->LineIntersect == 1)
					{ Shape.VtxX[0]=iConnectX1; Shape.VtxY[0]=iConnectY1; }
					
				fLineChange = true;
			}

			// Movement by Target2
			// Connect to attach vertex
			Action.Target2->GetProperty(P_LineAttach, &lineAttachV);
			lineAttach = lineAttachV.getArray();
			int32_t iConnectX2, iConnectY2;
			iConnectX2 = Action.Target2->GetX();
			iConnectY2 = Action.Target2->GetY();
			if (lineAttach)
			{
				iConnectX2 += lineAttach->GetItem(0).getInt();
				iConnectY2 += lineAttach->GetItem(1).getInt();
			}
			if ((iConnectX2!=Shape.VtxX[Shape.VtxNum-1]) || (iConnectY2!=Shape.VtxY[Shape.VtxNum-1]))
			{
				// Regular wrapping line
				if (Def->LineIntersect == 0)
					if (!Shape.LineConnect(iConnectX2,iConnectY2,Shape.VtxNum-1,-1,
											Shape.VtxX[Shape.VtxNum-1],Shape.VtxY[Shape.VtxNum-1])) fBroke=true;
				// No-intersection line
				if (Def->LineIntersect == 1)
					{ Shape.VtxX[Shape.VtxNum-1]=iConnectX2; Shape.VtxY[Shape.VtxNum-1]=iConnectY2; }
					
				fLineChange = true;
			}

			// Line fBroke
			if (fBroke)
			{
				Call(PSF_OnLineBreak,nullptr);
				AssignRemoval();
				return;
			}

			// Reduce line segments
			if (!::Game.iTick35)
				if (ReduceLineSegments(Shape, !::Game.iTick2))
					fLineChange = true;
					
			// Line change callback
			if (fLineChange)
				Call(PSF_OnLineChange);
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default:
		// Attach
		if (Action.t_attach)
		{
			xdir = ydir = 0;
			Mobile = true;
		}
		// Free gravity
		else
			DoGravity(this);
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	// Phase Advance (zero delay means no phase advance)
	if (pActionDef->GetPropertyInt(P_Delay))
	{
		Action.PhaseDelay+=iPhaseAdvance;
		if (Action.PhaseDelay >= pActionDef->GetPropertyInt(P_Delay))
		{
			// Advance Phase
			Action.PhaseDelay=0;
			Action.Phase += pActionDef->GetPropertyInt(P_Step);
			// Phase call
			if (pActionDef->GetPropertyStr(P_PhaseCall))
			{
				Call(pActionDef->GetPropertyStr(P_PhaseCall)->GetCStr());
			}
			// Phase end
			if (Action.Phase>=pActionDef->GetPropertyInt(P_Length))
			{
				C4String *next_action = pActionDef->GetPropertyStr(P_NextAction);
				// Keep current action if there is no NextAction
				if (!next_action)
					Action.Phase = 0;
				// set new action if it's not Hold
				else if (next_action == &Strings.P[P_Hold])
				{
					Action.Phase = pActionDef->GetPropertyInt(P_Length)-1;
					Action.PhaseDelay = pActionDef->GetPropertyInt(P_Delay)-1;
				}
				else
				{
					// Set new action
					SetActionByName(next_action, nullptr, nullptr, SAC_StartCall | SAC_EndCall);
				}
			}
		}
	}

	return;
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


bool C4Object::SetLightRange(int32_t iToRange, int32_t iToFadeoutRange)
{
	// set new range
	lightRange = iToRange;
	lightFadeoutRange = iToFadeoutRange;
	// resort into player's FoW-repeller-list
	UpdateLight();
	// success
	return true;
}


bool C4Object::SetLightColor(uint32_t iValue)
{
	// set new color value
	lightColor = iValue;
	// resort into player's FoW-repeller-list
	UpdateLight();
	// success
	return true;
}


void C4Object::UpdateLight()
{
	if (Landscape.HasFoW()) Landscape.GetFoW()->Add(this);
}

void C4Object::SetAudibilityAt(C4TargetFacet &cgo, int32_t iX, int32_t iY, int32_t player)
{
	// target pos (parallax)
	float offX, offY, newzoom;
	GetDrawPosition(cgo, iX, iY, cgo.Zoom, offX, offY, newzoom);
	int32_t audible_at_pos = Clamp(100 - 100 * Distance(cgo.X + cgo.Wdt / 2, cgo.Y + cgo.Hgt / 2, offX, offY) / 700, 0, 100);
	if (audible_at_pos > Audible)
	{
		Audible = audible_at_pos;
		AudiblePan = Clamp<int>(200 * (offX - cgo.X - (cgo.Wdt / 2)) / cgo.Wdt, -100, 100);
		AudiblePlayer = player;
	}
}

bool C4Object::IsVisible(int32_t iForPlr, bool fAsOverlay) const
{
	bool fDraw;
	C4Value vis;
	if (!GetProperty(P_Visibility, &vis))
		return true;

	int32_t Visibility;
	C4ValueArray *parameters = vis.getArray();
	if (parameters && parameters->GetSize())
	{
		Visibility = parameters->GetItem(0).getInt();
	}
	else
	{
		Visibility = vis.getInt();
	}
	// check layer
	if (Layer && Layer != this && !fAsOverlay)
	{
		fDraw = Layer->IsVisible(iForPlr, false);
		if (Layer->GetPropertyInt(P_Visibility) & VIS_LayerToggle) fDraw = !fDraw;
		if (!fDraw) return false;
	}
	// no flags set?
	if (!Visibility) return true;
	// check overlay
	if (Visibility & VIS_OverlayOnly)
	{
		if (!fAsOverlay) return false;
		if (Visibility == VIS_OverlayOnly) return true;
	}
	// editor visibility
	if (::Application.isEditor)
	{
		if (Visibility & VIS_Editor) return true;
	}
	// check visibility
	fDraw=false;
	if (Visibility & VIS_Owner) fDraw = fDraw || (iForPlr==Owner);
	if (iForPlr!=NO_OWNER)
	{
		// check all
		if (Visibility & VIS_Allies)  fDraw = fDraw || (iForPlr!=Owner && !Hostile(iForPlr, Owner));
		if (Visibility & VIS_Enemies) fDraw = fDraw || (iForPlr!=Owner && Hostile(iForPlr, Owner));
		if (parameters)
		{
			if (Visibility & VIS_Select)  fDraw = fDraw || parameters->GetItem(1+iForPlr).getBool();
		}
	}
	else fDraw = fDraw || (Visibility & VIS_God);
	return fDraw;
}

bool C4Object::IsInLiquidCheck() const
{
	return GBackLiquid(GetX(),GetY()+Def->Float*Con/FullCon-1);
}

void C4Object::SetRotation(int32_t nr)
{
	while (nr<0) nr+=360; nr%=360;
	// remove solid mask
	if (pSolidMaskData) pSolidMaskData->Remove(false);
	// set rotation
	fix_r=itofix(nr);
	// Update face
	UpdateFace(true);
}

void C4Object::PrepareDrawing() const
{
	// color modulation
	if (ColorMod != 0xffffffff || (BlitMode & (C4GFXBLIT_MOD2 | C4GFXBLIT_CLRSFC_MOD2))) pDraw->ActivateBlitModulation(ColorMod);
	// other blit modes
	pDraw->SetBlitMode(BlitMode);
}

void C4Object::FinishedDrawing() const
{
	// color modulation
	pDraw->DeactivateBlitModulation();
	// extra blitting flags
	pDraw->ResetBlitMode();
}

void C4Object::DrawSolidMask(C4TargetFacet &cgo) const
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
	// Mask if enabled, fullcon, not contained
	if (SolidMask.Wdt > 0 && Con >= FullCon && !Contained)
	{
		// Recheck and put mask
		if (!pSolidMaskData)
		{
			pSolidMaskData = new C4SolidMask(this);
		}
		else
			pSolidMaskData->Remove(false);
		pSolidMaskData->Put(true, nullptr, fRestoreAttachedObjects);
		SetHalfVehicleSolidMask(HalfVehicleSolidMask);
	}
	// Otherwise, remove and destroy mask
	else if (pSolidMaskData)
	{
		delete pSolidMaskData; pSolidMaskData = nullptr;
	}
}

bool C4Object::Collect(C4Object *pObj)
{
	// Object enter container
	bool fRejectCollect;
	if (!pObj->Enter(this, true, false, &fRejectCollect))
		return false;
	// Cancel attach (hacky)
	ObjectComCancelAttach(pObj);
	// Container Collection call
	Call(PSF_Collection,&C4AulParSet(pObj));
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

bool C4Object::ShiftContents(bool fShiftBack, bool fDoCalls)
{
	// get current object
	C4Object *c_obj = Contents.GetObject();
	if (!c_obj) return false;
	// get next/previous
	auto it = fShiftBack ? Contents.reverse().begin() : ++Contents.begin();
	while (!it.atEnd())
	{
		auto pObj = (*it);
		// check object
		if (pObj->Status)
			if (!c_obj->CanConcatPictureWith(pObj))
			{
				// object different: shift to this
				DirectComContents(pObj, !!fDoCalls);
				return true;
			}
		// next/prev item
		it++;
	}
	return false;
}

void C4Object::DirectComContents(C4Object *pTarget, bool fDoCalls)
{
	// safety
	if (!pTarget || !pTarget->Status || pTarget->Contained != this) return;
	// Desired object already at front?
	if (Contents.GetObject() == pTarget) return;
	// select object via script?
	if (fDoCalls)
		if (Call("~ControlContents", &C4AulParSet(pTarget)))
			return;
	// default action
	if (!(Contents.ShiftContents(pTarget))) return;
	// Selection sound
	if (fDoCalls) if (!Contents.GetObject()->Call("~Selection", &C4AulParSet(this))) StartSoundEffect("Clonk::Action::Grab",false,100,this);
	// update menu with the new item in "put" entry
	if (Menu && Menu->IsActive() && Menu->IsContextMenu())
	{
		Menu->Refill();
	}
	// Done
	return;
}

void C4Object::GetParallaxity(int32_t *parX, int32_t *parY) const
{
	assert(parX); assert(parY);
	*parX = 100; *parY = 100;
	if (Category & C4D_Foreground)
	{
		*parX = 0; *parY = 0;
		return;
	}
	if (!(Category & C4D_Parallax)) return;
	C4Value parV; GetProperty(P_Parallaxity, &parV);
	C4ValueArray *par = parV.getArray();
	if (!par) return;
	*parX = par->GetItem(0).getInt();
	*parY = par->GetItem(1).getInt();
}

bool C4Object::GetDragImage(C4Object **drag_object, C4Def **drag_def) const
{
	// drag is possible if MouseDragImage is assigned
	C4Value parV; GetProperty(P_MouseDragImage, &parV);
	if (!parV) return false;
	// determine drag object/id
	C4Object *obj = parV.getObj();
	C4Def * def = nullptr;
	if (!obj) def = parV.getDef();
	if (drag_object) *drag_object = obj;
	if (drag_def) *drag_def = def;
	// drag possible, even w./o image
	return true;
}

int32_t C4Object::AddObjectAndContentsToArray(C4ValueArray *target_array, int32_t index)
{
	// add self, contents and child contents count recursively to value array. Return index after last added item.
	target_array->SetItem(index++, C4VObj(this));
	for (C4Object *cobj : Contents)
	{
		index = cobj->AddObjectAndContentsToArray(target_array, index);
	}
	return index;
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

void C4Object::GetViewPos(float & riX, float & riY, float tx, float ty, const C4Facet & fctViewport) const       // get position this object is seen at (for given scroll)
{
	if (Category & C4D_Parallax) GetViewPosPar(riX, riY, tx, ty, fctViewport); else { riX = float(GetX()); riY = float(GetY()); }
}

bool C4Object::GetDrawPosition(const C4TargetFacet & cgo,
	float & resultx, float & resulty, float & resultzoom) const
{
	return GetDrawPosition(cgo, fixtof(fix_x), fixtof(fix_y), cgo.Zoom, resultx, resulty, resultzoom);
}

bool C4Object::GetDrawPosition(const C4TargetFacet & cgo, float objx, float objy, float zoom, float & resultx, float & resulty, float & resultzoom) const
{
	// for HUD
	if(Category & C4D_Foreground)
	{
		resultzoom = zoom;

		if(fix_x < 0)
			resultx = cgo.X + objx + cgo.Wdt;
		else
			resultx = cgo.X + objx;

		if(fix_y < 0)
			resulty = cgo.Y + objy + cgo.Hgt;
		else
			resulty = cgo.Y + objy;

		return true;
	}

	// zoom with parallaxity
	int iParX, iParY;
	GetParallaxity(&iParX, &iParY);
	float targetx = cgo.TargetX; float targety = cgo.TargetY;
	float parx = iParX / 100.0f; float pary = iParY / 100.0f;
	float par = parx; // and pary?

	// Step 1: project to landscape coordinates
	resultzoom = 1.0 / (1.0 - (par - par/zoom));
	// it would be par / (1.0 - (par - par/zoom)) if objects would get smaller farther away
	if (resultzoom <= 0 || resultzoom > 100) // FIXME: optimize treshhold
		return false;

	float rx = ((1 - parx) * cgo.ParRefX) * resultzoom + objx / (parx + zoom - parx * zoom);
	float ry = ((1 - pary) * cgo.ParRefY) * resultzoom + objy / (pary + zoom - pary * zoom);

	// Step 2: convert to screen coordinates
	if(parx == 0 && fix_x < 0)
		resultx = cgo.X + (objx + cgo.Wdt) * zoom / resultzoom;
	else
		resultx = cgo.X + (rx - targetx) * zoom / resultzoom;

	if(pary == 0 && fix_y < 0)
		resulty = cgo.Y + (objy + cgo.Hgt) * zoom / resultzoom;
	else
		resulty = cgo.Y + (ry - targety) * zoom / resultzoom;

	return true;
}

void C4Object::GetViewPosPar(float &riX, float &riY, float tx, float ty, const C4Facet &fctViewport) const
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

bool C4Object::SetGraphics(const char *szGraphicsName, C4Def *pSourceDef)
{
	// safety
	if (!Status) return false;
	// default def
	if (!pSourceDef) pSourceDef = Def;
	// get graphics
	C4DefGraphics *pGrp = pSourceDef->Graphics.Get(szGraphicsName);
	if (!pGrp) return false;
	// set new graphics
	pGraphics = pGrp;
	// update Color, etc.
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

C4GraphicsOverlay *C4Object::GetGraphicsOverlay(int32_t iForID) const
{
	// search in list until ID is found or passed
	C4GraphicsOverlay *pOverlay = pGfxOverlay;
	while (pOverlay && pOverlay->GetID() < iForID) pOverlay = pOverlay->GetNext();
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iForID) return pOverlay;
	// none found
	return nullptr;
}

C4GraphicsOverlay *C4Object::GetGraphicsOverlay(int32_t iForID, bool fCreate)
{
	// search in list until ID is found or passed
	C4GraphicsOverlay *pOverlay = pGfxOverlay, *pPrevOverlay = nullptr;
	while (pOverlay && pOverlay->GetID() < iForID) { pPrevOverlay = pOverlay; pOverlay = pOverlay->GetNext(); }
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iForID) return pOverlay;
	// ID has been passed: Create new if desired
	if (!fCreate) return nullptr;
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
	C4GraphicsOverlay *pOverlay = pGfxOverlay, *pPrevOverlay = nullptr;
	while (pOverlay && pOverlay->GetID() < iOverlayID) { pPrevOverlay = pOverlay; pOverlay = pOverlay->GetNext(); }
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iOverlayID)
	{
		// remove it
		if (pPrevOverlay) pPrevOverlay->SetNext(pOverlay->GetNext()); else pGfxOverlay = pOverlay->GetNext();
		pOverlay->SetNext(nullptr); // prevents deletion of following overlays
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
			if ((pGfxOvrlObj = pGfxOvrl->GetOverlayObject()))
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

void C4Object::ClearContentsAndContained(bool fDoCalls)
{
	// exit contents from container
	for (C4Object *cobj : Contents)
	{
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
		iDestAngle=(iSolidRight-iSolidLeft)*(35/std::max<int32_t>(iRangeX, 1));
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
	if (Abs(iDestAngle-GetR())>2)
	{
		rdir = itofix(Clamp<int32_t>(iDestAngle-GetR(), -15,+15));
		rdir/=(10000/iSpeed);
	}
	else rdir=0;
	// done, success
	return true;
}

static void BubbleOut(int32_t tx, int32_t ty)
{
	// No bubbles from nowhere
	if (!GBackSemiSolid(tx,ty)) return;
	// Enough bubbles out there already
	if (::Objects.ObjectCount(C4ID::Bubble) >= 150) return;
	// Create bubble
	Game.CreateObject(C4ID::Bubble,nullptr,NO_OWNER,tx,ty);
}

void C4Object::Splash()
{
	int32_t tx = GetX(); int32_t ty = GetY()+1;
	int32_t amt = std::min(Shape.Wdt*Shape.Hgt/10,20);
	// Splash only if there is free space above
	if (GBackSemiSolid(tx, ty - 15)) return;
	// get back mat
	int32_t iMat = GBackMat(tx, ty);
	// check liquid
	if (MatValid(iMat))
		if (DensityLiquid(::MaterialMap.Map[iMat].Density) && ::MaterialMap.Map[iMat].Instable)
		{
			int32_t sy = ty;
			while (GBackLiquid(tx, sy) && sy > ty - 20 && sy >= 0) sy--;
			// Splash bubbles and liquid
			for (int32_t cnt=0; cnt<amt; cnt++)
			{
				int32_t bubble_x = tx+Random(16)-8;
				int32_t bubble_y = ty+Random(16)-6;
				BubbleOut(bubble_x,bubble_y);
				if (GBackLiquid(tx,ty) && !GBackSemiSolid(tx, sy))
				{
					C4Real xdir = C4REAL100(Random(151)-75);
					C4Real ydir = C4REAL100(-int32_t(Random(200)));
					::PXS.Create(::Landscape.ExtractMaterial(tx,ty,false),
					             itofix(tx),itofix(sy),
					             xdir,
					             ydir);
				}
			}
		}
	// Splash sound
	if (amt>=20)
		StartSoundEffect("Liquids::Splash2", false, 50, this);
	else if (amt>1) StartSoundEffect("Liquids::Splash1", false, 50, this);
}

void C4Object::UpdateInLiquid()
{
	// InLiquid check
	if (IsInLiquidCheck()) // In Liquid
	{
		if (!InLiquid) // Enter liquid
		{
			if (OCF & OCF_HitSpeed2)
				if (Mass>3) Splash();
			InLiquid=true;
		}
	}
	else // Out of liquid
	{
		if (InLiquid) // Leave liquid
			InLiquid=false;
	}
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

void C4Object::GrabContents(C4Object *pFrom)
{
	// create a temp list of all objects and transfer it
	// this prevents nasty deadlocks caused by RejectEntrance-scripts
	C4ObjectList tmpList; tmpList.Copy(pFrom->Contents);
	for (C4Object *obj : tmpList)
		if (obj->Status)
			obj->Enter(this);
}

bool C4Object::CanConcatPictureWith(C4Object *pOtherObject) const
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
				if (!GetGraphicsOverlay(pOtherOverlay->GetID())) return false;
	}
	// concat OK
	return true;
}

bool C4Object::IsMoveableBySolidMask(int ComparisonPlane) const
{
	return (Status == C4OS_NORMAL)
		&& !(Category & C4D_StaticBack)
		&& (ComparisonPlane < GetPlane())
		&& !Contained
		;
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

int32_t C4Object::GetSolidMaskPlane() const
{
	// use SolidMaskPlane property. Fallback to object plane if unassigned.
	int32_t plane = GetPropertyInt(P_SolidMaskPlane);
	return plane ? plane : GetPlane();
}
