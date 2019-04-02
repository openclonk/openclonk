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

/* Logic for C4Object: Graphics and drawing */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "game/C4GraphicsSystem.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "landscape/C4Particles.h"
#include "lib/StdColors.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4MeshDenumerator.h"


static void DrawVertex(C4Facet &cgo, float tx, float ty, int32_t col, int32_t contact)
{
	if (Inside<int32_t>(tx,cgo.X,cgo.X+cgo.Wdt) && Inside<int32_t>(ty,cgo.Y,cgo.Y+cgo.Hgt))
	{
		pDraw->DrawLineDw(cgo.Surface, tx - 1, ty, tx + 1, ty, col, 0.5f);
		pDraw->DrawLineDw(cgo.Surface, tx, ty - 1, tx, ty + 1, col, 0.5f);
		if (contact) pDraw->DrawFrameDw(cgo.Surface,tx-1.5,ty-1.5,tx+1.5,ty+1.5,C4RGB(0xff, 0xff, 0xff));
	}
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
