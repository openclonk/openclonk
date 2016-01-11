/*	
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Handles landscape and sky */

#include <C4Include.h>
#include <C4Landscape.h>

#include <C4DefList.h>
#include <C4SolidMask.h>
#include <C4Game.h>
#include <C4Group.h>
#include <C4Map.h>
#include <C4MapCreatorS2.h>
#include <C4SolidMask.h>
#include <C4Object.h>
#include <C4Physics.h>
#include <C4Random.h>
#include <C4Surface.h>
#include <C4ToolsDlg.h>
#include <C4Record.h>
#include <C4Material.h>
#include <C4GameMessage.h>
#include <C4Application.h>
#include <C4Log.h>
#include <C4Stat.h>
#include <C4MassMover.h>
#include <C4PXS.h>
#include <C4Weather.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Texture.h>
#include <C4Record.h>
#include <CSurface8.h>
#include <StdPNG.h>
#include <C4MaterialList.h>
#include <C4LandscapeRender.h>
#include <C4FoW.h>
#include <C4FindObject.h>
#include <C4GameObjects.h>
#include <C4MapScript.h>

namespace
{
	bool ForLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
		std::function<bool(int32_t, int32_t)> fnCallback,
		int32_t *lastx = 0, int32_t *lasty = 0)
	{
		int d, dx, dy, aincr, bincr, xincr, yincr, x, y;
		if (Abs(x2 - x1)<Abs(y2 - y1))
		{
			if (y1>y2) { std::swap(x1, x2); std::swap(y1, y2); }
			xincr = (x2>x1) ? 1 : -1;
			dy = y2 - y1; dx = Abs(x2 - x1);
			d = 2 * dx - dy; aincr = 2 * (dx - dy); bincr = 2 * dx; x = x1; y = y1;
			if (!fnCallback(x, y))
			{
				if (lastx) *lastx = x; if (lasty) *lasty = y;
				return false;
			}
			for (y = y1 + 1; y <= y2; ++y)
			{
				if (d >= 0) { x += xincr; d += aincr; }
				else d += bincr;
				if (!fnCallback(x, y))
				{
					if (lastx) *lastx = x; if (lasty) *lasty = y;
					return false;
				}
			}
		}
		else
		{
			if (x1>x2) { std::swap(x1, x2); std::swap(y1, y2); }
			yincr = (y2>y1) ? 1 : -1;
			dx = x2 - x1; dy = Abs(y2 - y1);
			d = 2 * dy - dx; aincr = 2 * (dy - dx); bincr = 2 * dy; x = x1; y = y1;
			if (!fnCallback(x, y))
			{
				if (lastx) *lastx = x; if (lasty) *lasty = y;
				return false;
			}
			for (x = x1 + 1; x <= x2; ++x)
			{
				if (d >= 0) { y += yincr; d += aincr; }
				else d += bincr;
				if (!fnCallback(x, y))
				{
					if (lastx) *lastx = x; if (lasty) *lasty = y;
					return false;
				}
			}
		}
		return true;
	}
}

C4Landscape::C4Landscape()
{
	Default();
}

C4Landscape::~C4Landscape()
{
	Clear();
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++ Execute and display +++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

void C4Landscape::Execute()
{
	// Landscape scan
	if (!NoScan)
		ExecuteScan();
	// move sky
	Sky.Execute();

	// Queued Relights -- note that normally we process them before drawing every frame;
	// this just makes sure relights don't accumulate over a long period of time if no
	// viewport is open (developer mode).
	if (!::Game.iTick35)
		DoRelights();
}


void C4Landscape::ExecuteScan()
{


	int32_t cy,mat;

	// Check: Scan needed?
	const int32_t iTemperature = ::Weather.GetTemperature();
	for (mat = 0; mat < ::MaterialMap.Num; mat++)
		if (MatCount[mat])
		{
			if (::MaterialMap.Map[mat].BelowTempConvertTo &&
			    iTemperature < ::MaterialMap.Map[mat].BelowTempConvert)
				break;
			else if (::MaterialMap.Map[mat].AboveTempConvertTo &&
			         iTemperature > ::MaterialMap.Map[mat].AboveTempConvert)
				break;
		}
	if (mat >= ::MaterialMap.Num)
		return;

#ifdef DEBUGREC_MATSCAN
	if (Config.General.DebugRec)
		AddDbgRec(RCT_MatScan, &ScanX, sizeof(ScanX));
#endif

	for (int32_t cnt=0; cnt<ScanSpeed; cnt++)
	{

		// Scan landscape column: sectors down
		int32_t last_mat = -1;
		for (cy=0; cy<Height; cy++)
		{
			mat=_GetMat(ScanX, cy);
			// material change?
			if (last_mat != mat)
			{
				// upwards
				if (last_mat != -1)
					DoScan(ScanX, cy-1, last_mat, 1);
				// downwards
				if (mat != -1)
					cy += DoScan(ScanX, cy, mat, 0);
			}
			last_mat = mat;
		}

		// Scan advance & rewind
		ScanX++;
		if (ScanX>=Width)
			ScanX=0;

	}

}

#define PRETTY_TEMP_CONV

int32_t C4Landscape::DoScan(int32_t cx, int32_t cy, int32_t mat, int32_t dir)
{
	int32_t conv_to_tex = 0;
	int32_t iTemperature = ::Weather.GetTemperature();
	// Check below conv
	if (::MaterialMap.Map[mat].BelowTempConvertDir == dir)
		if (::MaterialMap.Map[mat].BelowTempConvertTo)
			if (iTemperature< ::MaterialMap.Map[mat].BelowTempConvert)
				conv_to_tex=::MaterialMap.Map[mat].BelowTempConvertTo;
	// Check above conv
	if (::MaterialMap.Map[mat].AboveTempConvertDir == dir)
		if (::MaterialMap.Map[mat].AboveTempConvertTo)
			if (iTemperature>::MaterialMap.Map[mat].AboveTempConvert)
				conv_to_tex=::MaterialMap.Map[mat].AboveTempConvertTo;
	// nothing to do?
	if (!conv_to_tex) return 0;
	// find material
	int32_t conv_to = ::TextureMap.GetEntry(conv_to_tex)->GetMaterialIndex();
	// find mat top
	int32_t mconv = ::MaterialMap.Map[mat].TempConvStrength,
	                mconvs = mconv;
#ifdef DEBUGREC_MATSCAN
	if (Config.General.DebugRec)
	{
		C4RCMatScan rc = { cx, cy, mat, conv_to, dir, mconvs };
		AddDbgRec(RCT_MatScanDo, &rc, sizeof(C4RCMatScan));
	}
#endif
	int32_t ydir = (dir == 0 ? +1 : -1), cy2;
#ifdef PRETTY_TEMP_CONV
	// get left pixel
	int32_t lmat = (cx > 0 ? _GetMat(cx-1, cy) : -1);
	// left pixel not converted? do nothing
	if (lmat == mat) return 0;
	// left pixel converted? suppose it was done by a prior scan and skip check
	if (lmat != conv_to)
	{
		int32_t iSearchRange = std::max<int32_t>(5, mconvs);
		// search upper/lower bound
		int32_t cys = cy, cxs = cx;
		while (cxs < GBackWdt-1)
		{
			// one step right
			cxs++;
			if (_GetMat(cxs, cys) == mat)
			{
				// search surface
				cys -= ydir;
				while (Inside<int32_t>(cys, 0, GBackHgt-1) && _GetMat(cxs, cys) == mat)
				{
					cys -= ydir;
					if ((mconvs = std::min(mconv - Abs(cys - cy), mconvs)) < 0)
						return 0;
				}
				// out of bounds?
				if (!Inside<int32_t>(cys, 0, GBackHgt-1)) break;
				// back one step
				cys += ydir;
			}
			else
			{
				// search surface
				cys += ydir;
				while (Inside<int32_t>(cys, 0, GBackHgt-1) && _GetMat(cxs, cys) != mat)
				{
					cys += ydir;
					if (Abs(cys - cy) > iSearchRange)
						break;
				}
				// out of bounds?
				if (!Inside<int32_t>(cys, 0, GBackHgt-1)) break;
				if (Abs(cys - cy) > iSearchRange) break;
			}
		}
	}
#endif
	// Conversion
	bool conv_to_is_solid = (conv_to>-1) && DensitySolid(::MaterialMap.Map[conv_to].Density);
	for (cy2 = cy; mconvs >= 0 && Inside<int32_t>(cy2, 0, GBackHgt-1); cy2 += ydir, mconvs--)
	{
		// material changed?
		int32_t pix = _GetPix(cx, cy2);
		if (PixCol2Mat(pix) != mat) break;
#ifdef PRETTY_TEMP_CONV
		// get left pixel
		int32_t lmat = (cx > 0 ? _GetMat(cx-1, cy2) : -1);
		// left pixel not converted? break
		if (lmat == mat) break;
#endif
		// set mat (and keep background material)
		SetPix2(cx, cy2, MatTex2PixCol(conv_to_tex), Transparent);
		if (!conv_to_is_solid) CheckInstabilityRange(cx,cy2);
	}
	// return pixel converted
	return Abs(cy2 - cy);
}

void C4Landscape::ScanSideOpen()
{
	int32_t cy;
	for (cy=0; (cy<Height) && !GetPix(0,cy); cy++) {}
	LeftOpen=cy;
	for (cy=0; (cy<Height) && !GetPix(Width-1,cy); cy++) {}
	RightOpen=cy;
}



void C4Landscape::Draw(C4TargetFacet &cgo, C4FoWRegion *pLight)
{
	if (Modulation) pDraw->ActivateBlitModulation(Modulation);
	// blit landscape
	if (::GraphicsSystem.Show8BitSurface == 1)
		pDraw->Blit8Fast(Surface8, cgo.TargetX, cgo.TargetY, cgo.Surface, cgo.X,cgo.Y,cgo.Wdt,cgo.Hgt);
	else if (::GraphicsSystem.Show8BitSurface == 2)
		pDraw->Blit8Fast(Surface8Bkg, cgo.TargetX, cgo.TargetY, cgo.Surface, cgo.X,cgo.Y,cgo.Wdt,cgo.Hgt);
	else if(pLandscapeRender)
	{
		DoRelights();
		pLandscapeRender->Draw(cgo, pLight);
	}
	if (Modulation) pDraw->DeactivateBlitModulation();
}

bool C4Landscape::DoRelights()
{
	if (!pLandscapeRender) return true;
	for (int32_t i = 0; i < C4LS_MaxRelights; i++)
	{
		if (!Relights[i].Wdt)
			break;
		// Remove all solid masks in the (twice!) extended region around the change
		C4Rect SolidMaskRect = pLandscapeRender->GetAffectedRect(Relights[i]);
		C4SolidMask * pSolid;
		for (pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
			pSolid->RemoveTemporary(SolidMaskRect);
		// Perform the update
		pLandscapeRender->Update(Relights[i], this);
		if (pFoW) pFoW->Ambient.UpdateFromLandscape(*this, Relights[i]);
		// Restore Solidmasks
		for (pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
			pSolid->PutTemporary(SolidMaskRect);
		C4SolidMask::CheckConsistency();
		// Clear slot
		Relights[i].Default();
	}
	return true;
}


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++ Add and destroy landscape++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static std::vector<int32_t> GetRoundPolygon(int32_t x, int32_t y, int32_t size, int32_t smoothness)
{
	/*
	So what is this? It's basically a circle with the radius 'size'. The radius
	is adjusted by two sin/cos waves. The random lies in the phase of the sin/cos
	and in the factor the wave is added to the normal circle shape. smoothness from
	0 to 100. 0 gives an exagerated 'explosion' shape while 100 is almost a circle
	*/

	if(smoothness > 100) smoothness = 100;
	if(smoothness < 0) smoothness = 0;
	if(size <= 0) size = 1;

	// vertex count of the polygon
	int32_t count = 2*size/3 + 6;

	std::vector<int32_t> vertices;
	vertices.reserve(count*2);

	// varying phase of the sin/cos waves
	C4Real begin = itofix(360)*Random(100)/100;
	C4Real begin2 = itofix(360)*Random(100)/100;

	// parameters:
	// the bigger the factor, the smaller the divergence from a circle
	C4Real anticircle = itofix(3)+smoothness/16*smoothness/16;
	// the bigger the factor the more random is the divergence from the circle
	int random = 80*(200-smoothness);

	for(int i=0; i < count; ++i)
	{
		C4Real angle = itofix(360)*i/count;

		C4Real currsize = itofix(size);
		currsize += Sin(angle*3 + begin + itofix(Random(random))/100) * size/anticircle;		// +sin
		currsize += Cos(angle*5 + begin2 + itofix(Random(random))/100) * size/anticircle/2;	// +cos

		vertices.push_back(x+fixtoi(Sin(angle)*currsize));
		vertices.push_back(y-fixtoi(Cos(angle)*currsize));
	}

	return vertices;
}

static std::vector<int32_t> GetRectangle(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt)
{
	std::vector<int32_t> vertices;
	vertices.resize(8);

	vertices[0] = tx;      vertices[1] = ty;
	vertices[2] = tx;      vertices[3] = ty+hgt;
	vertices[4] = tx+wdt;  vertices[5] = ty+hgt;
	vertices[6] = tx+wdt;  vertices[7] = ty;

	return vertices;
}

static C4Rect getBoundingBox(int *vtcs, int length)
{
	C4Rect BoundingBox(vtcs[0], vtcs[1], 1, 1);
	for (int32_t i = 2; i + 1 < length; i += 2)
	{
		BoundingBox.Add(C4Rect(vtcs[i], vtcs[i + 1], 1, 1));
	}
	return BoundingBox;
}

void C4Landscape::ClearFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt)
{
	std::vector<int32_t> vertices(GetRectangle(tx,ty,wdt,hgt));
	C4Rect r(tx, ty, wdt, hgt);
	PrepareChange(r);
	ForPolygon(&vertices[0],vertices.size()/2,&C4Landscape::ClearPix);
	FinishChange(r);
}

int32_t C4Landscape::DigFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, C4Object *by_object, bool no_dig2objects, bool no_instability_check)
{
	std::vector<int32_t> vertices(GetRectangle(tx,ty,wdt,hgt));
	return DigFreeShape(&vertices[0],vertices.size(),by_object, no_dig2objects, no_instability_check);
}

int32_t C4Landscape::DigFree(int32_t tx, int32_t ty, int32_t rad, C4Object *by_object, bool no_dig2objects, bool no_instability_check)
{
	std::vector<int32_t> vertices(GetRoundPolygon(tx,ty,rad,80));
	return DigFreeShape(&vertices[0],vertices.size(),by_object, no_dig2objects, no_instability_check);
}

void C4Landscape::BlastFree(int32_t tx, int32_t ty, int32_t rad, int32_t caused_by, C4Object *by_object, int32_t iMaxDensity)
{
	std::vector<int32_t> vertices(GetRoundPolygon(tx,ty,rad,30));
	BlastFreeShape(&vertices[0],vertices.size(),by_object,caused_by,iMaxDensity);
}

void C4Landscape::ShakeFree(int32_t tx, int32_t ty, int32_t rad)
{
	std::vector<int32_t> vertices(GetRoundPolygon(tx,ty,rad,50));
	ForPolygon(&vertices[0],vertices.size()/2,&C4Landscape::ShakeFreePix);
}

C4ValueArray *C4Landscape::PrepareFreeShape(C4Rect &BoundingBox, C4Object *by_object)
{
	// Remember any in-earth objects in area
	C4FindObjectInRect fo_inrect(BoundingBox);
	C4FindObjectOCF fo_insolid(OCF_InSolid);
	C4FindObjectLayer fo_layer(by_object ? by_object->Layer : NULL);
	C4FindObject *fo_list[] = {&fo_inrect, &fo_insolid, &fo_layer};
	C4FindObjectAndStatic fo_srch(3, fo_list);
	return fo_srch.FindMany(::Objects, ::Objects.Sectors);
}

void C4Landscape::PostFreeShape(C4ValueArray *dig_objects, C4Object *by_object)
{
	// Do callbacks to digger and dug out objects for objects that are now dug free
	if (by_object)
	{
		for (int32_t i=0; i<dig_objects->GetSize(); ++i)
		{
			C4Object *dig_object = dig_objects->GetItem(i).getObj();
			if (dig_object && !GBackSolid(dig_object->GetX(), dig_object->GetY()))
				if (dig_object != by_object)
					if (!dig_object->Contained && dig_object->Status)
					{
						C4AulParSet pars(C4VObj(by_object));
						dig_object->Call(PSF_OnDugOut, &pars);
						if (dig_object->Status && by_object->Status)
						{
							C4AulParSet pars(C4VObj(dig_object));
							by_object->Call(PSF_DigOutObject, &pars);
						}
					}
		}
	}
}

int32_t C4Landscape::DigFreeShape(int *vtcs, int length, C4Object *by_object, bool no_dig2objects, bool no_instability_check)
{
	C4Rect BoundingBox = getBoundingBox(vtcs,length);
	int32_t amount;

	// Remember any collectible objects in area
	std::unique_ptr<C4ValueArray> dig_objects(PrepareFreeShape(BoundingBox, by_object));

	if(by_object)
	{
		if(!by_object->MaterialContents)
			by_object->MaterialContents = new C4MaterialList;
		if (no_instability_check)
			amount = ForPolygon(vtcs,length/2,&C4Landscape::DigFreePixNoInstability,by_object->MaterialContents);
		else
			amount = ForPolygon(vtcs,length/2,&C4Landscape::DigFreePix,by_object->MaterialContents);
	}
	else
		if (no_instability_check)
			amount = ForPolygon(vtcs,length/2,&C4Landscape::DigFreePixNoInstability,NULL);
		else
			amount = ForPolygon(vtcs,length/2,&C4Landscape::DigFreePix,NULL);

	// create objects from the material
	if(!::Game.iTick5)
	{
		if(!no_dig2objects)
			if(by_object)
				if(by_object->MaterialContents)
				{
					int32_t tx = BoundingBox.GetMiddleX(), ty = BoundingBox.GetBottom();
					DigMaterial2Objects(tx,ty,by_object->MaterialContents, by_object);
				}
	}

	// Do callbacks to digger for objects that are now dug free
	PostFreeShape(dig_objects.get(), by_object);

	return amount;
}

void C4Landscape::BlastFreeShape(int *vtcs, int length, C4Object *by_object, int32_t by_player, int32_t iMaxDensity)
{
	C4MaterialList *MaterialContents = NULL;

	C4Rect BoundingBox = getBoundingBox(vtcs,length);

	// Remember any collectible objects in area
	std::unique_ptr<C4ValueArray> dig_objects(PrepareFreeShape(BoundingBox, by_object));

	uint8_t *pblast_tbl = NULL, blast_tbl[C4M_MaxTexIndex];
	if (iMaxDensity < C4M_Vehicle)
	{
		for (int32_t i=0; i < C4M_MaxTexIndex; ++i) blast_tbl[i] = (GetPixDensity(i)<=iMaxDensity);
		pblast_tbl = blast_tbl;
	}

	if(by_object)
	{
		if(!by_object->MaterialContents)
			by_object->MaterialContents = new C4MaterialList;
		ForPolygon(vtcs,length/2,&C4Landscape::BlastFreePix,by_object->MaterialContents, 0, 0, pblast_tbl);
	}
	else
	{
		MaterialContents = new C4MaterialList;
		ForPolygon(vtcs,length/2,&C4Landscape::BlastFreePix,MaterialContents,iMaxDensity);
	}

	// create objects from the material
	C4MaterialList *mat_list = NULL;
	if(by_object)
		mat_list = by_object->MaterialContents;
	else
		mat_list = MaterialContents;

	int32_t tx = BoundingBox.GetMiddleX(), ty = BoundingBox.GetMiddleY();
	BlastMaterial2Objects(tx,ty,mat_list,by_player,(BoundingBox.Wdt+BoundingBox.Hgt)/4, dig_objects.get());

	if(MaterialContents) delete MaterialContents;

	// Do callbacks to digger for objects that are now dug free
	PostFreeShape(dig_objects.get(), by_object);
}

void C4Landscape::BlastMaterial2Objects(int32_t tx, int32_t ty, C4MaterialList *mat_list, int32_t caused_by, int32_t str, C4ValueArray *out_objects)
{
	for (int32_t mat=0; mat< ::MaterialMap.Num; mat++)
	{
		if (mat_list->Amount[mat])
		{
			int32_t cast_strength = str;
			int32_t pxsamount = 0, blastamount = 0;

			if (::MaterialMap.Map[mat].Blast2PXSRatio != 0)
			{
				pxsamount = mat_list->Amount[mat]/::MaterialMap.Map[mat].Blast2PXSRatio;
				::PXS.Cast(mat,pxsamount,tx,ty,cast_strength*2);
			}

			if (::MaterialMap.Map[mat].Blast2Object != C4ID::None)
			{
				if (::MaterialMap.Map[mat].Blast2ObjectRatio != 0)
				{
					blastamount = mat_list->Amount[mat]/::MaterialMap.Map[mat].Blast2ObjectRatio;
					Game.CastObjects(::MaterialMap.Map[mat].Blast2Object, NULL, blastamount, cast_strength, tx, ty, NO_OWNER, caused_by, out_objects);
				}
			}

			mat_list->Amount[mat] -= std::max(blastamount * ::MaterialMap.Map[mat].Blast2ObjectRatio,
			                             pxsamount * ::MaterialMap.Map[mat].Blast2PXSRatio);
		}
	}
}

void C4Landscape::DigMaterial2Objects(int32_t tx, int32_t ty, C4MaterialList *mat_list, C4Object *pCollect)
{
	C4AulParSet pars(C4VObj(pCollect));
	for (int32_t mat=0; mat< ::MaterialMap.Num; mat++)
	{
		if (mat_list->Amount[mat])
		{
			if (::MaterialMap.Map[mat].Dig2Object != C4ID::None)
				if (::MaterialMap.Map[mat].Dig2ObjectRatio != 0)
					while (mat_list->Amount[mat] >= ::MaterialMap.Map[mat].Dig2ObjectRatio)
					{
						mat_list->Amount[mat] -= ::MaterialMap.Map[mat].Dig2ObjectRatio;
						C4Object *pObj = Game.CreateObject(::MaterialMap.Map[mat].Dig2Object, NULL, NO_OWNER, tx, ty);
						if (!pObj || !pObj->Status) continue;
						// Do callbacks to dug object and digger
						pObj->Call(PSF_OnDugOut, &pars);
						if (!pObj->Status || !pCollect || !pCollect->Status || pObj->Contained) continue;
						C4AulParSet pars(C4VObj(pObj));
						pCollect->Call(PSF_DigOutObject, &pars);
						if (!pObj->Status || !pCollect->Status || pObj->Contained) continue;
						// Try to collect object
						if(::MaterialMap.Map[mat].Dig2ObjectCollect)
							if(pCollect && pCollect->Status && pObj && pObj->Status)
								if(!pCollect->Collect(pObj))
									// Collection forced? Don't generate objects
									if(::MaterialMap.Map[mat].Dig2ObjectCollect == 2)
									{
										pObj->AssignRemoval();
										// Cap so we never have more than one object worth of material in the store
										mat_list->Amount[mat] = ::MaterialMap.Map[mat].Dig2ObjectRatio;
										break;
									}
					}
		}
	}
}

bool C4Landscape::DigFreePixNoInstability(int32_t tx, int32_t ty)
{
	int32_t mat = GetMat(tx,ty);
	if (MatValid(mat))
		if (::MaterialMap.Map[mat].DigFree)
		{
			ClearPix(tx,ty);
			return true;
		}
	return false;
}

bool C4Landscape::DigFreePix(int32_t tx, int32_t ty)
{
	int32_t mat = GetMat(tx,ty);
	if (MatValid(mat))
		if (::MaterialMap.Map[mat].DigFree)
		{
			ClearPix(tx,ty);
			// check for instable materials to start moving by the cleared space
			CheckInstabilityRange(tx,ty);
			return true;
		}
	return false;
}

bool C4Landscape::BlastFreePix(int32_t tx, int32_t ty)
{
	int32_t mat = GetMat(tx,ty);
	if (MatValid(mat))
	{
		// for blast, either shift to different material or blast free
		if (::MaterialMap.Map[mat].BlastFree)
		{
			ClearPix(tx,ty);
			// check for instable materials to start moving by the cleared space
			CheckInstabilityRange(tx,ty);
			return true;
		}
		else
			if (::MaterialMap.Map[mat].BlastShiftTo)
				SetPix2(tx,ty,MatTex2PixCol(::MaterialMap.Map[mat].BlastShiftTo), Transparent);
	}
	return false;
}

bool C4Landscape::ShakeFreePix(int32_t tx, int32_t ty)
{
	int32_t mat=GetMat(tx,ty);
	if (MatValid(mat))
	{
		if (::MaterialMap.Map[mat].DigFree)
		{
			ClearPix(tx,ty);
			::PXS.Create(mat,itofix(tx),itofix(ty));
			// check for instable materials to start moving by the cleared space
			CheckInstabilityRange(tx,ty);
			return true;
		}
	}
	return false;
}

bool C4Landscape::ClearPix(int32_t tx, int32_t ty)
{
	// Replace pixel with background pixel
	BYTE bkgPix = Surface8Bkg->GetPix(tx, ty);
	return SetPix2(tx, ty, bkgPix, bkgPix);
}

bool C4Landscape::SetPix2(int32_t x, int32_t y, BYTE fgPix, BYTE bgPix)
{
	// check bounds
	if (x < 0 || y < 0 || x >= Width || y >= Height)
		return false;
	// no change?
	if ((fgPix == Transparent || fgPix == _GetPix(x, y)) && (bgPix == Transparent || bgPix == Surface8Bkg->_GetPix(x, y)))
		return true;
	// set pixel
	return _SetPix2(x, y, fgPix, bgPix);
}

bool C4Landscape::_SetPix2(int32_t x, int32_t y, BYTE fgPix, BYTE bgPix)
{
	if (Config.General.DebugRec)
	{
		C4RCSetPix rc;
		rc.x=x; rc.y=y; rc.clr=fgPix; rc.bgClr=fgPix;
		AddDbgRec(RCT_SetPix, &rc, sizeof(rc));
	}
	assert(x >= 0 && y >= 0 && x < Width && y < Height);
	// get pixel and resolve transparency to already existing pixel
	BYTE opix = _GetPix(x, y);
	if (fgPix == Transparent) fgPix = opix;
	if (bgPix == Transparent) bgPix = Surface8Bkg->_GetPix(x, y);
	// check pixel
	if (fgPix == opix && bgPix == Surface8Bkg->_GetPix(x, y)) return true;
	// count pixels
	if (Pix2Dens[fgPix])
		{ if (!Pix2Dens[opix]) PixCnt[(y / 15) + (x / 17) * PixCntPitch]++; }
	else
		{ if (Pix2Dens[opix]) PixCnt[(y / 15) + (x / 17) * PixCntPitch]--; }
	// count material
	assert(!fgPix || MatValid(Pix2Mat[fgPix]));
	int32_t omat = Pix2Mat[opix], nmat = Pix2Mat[fgPix];
	if (opix) MatCount[omat]--;
	if (fgPix) MatCount[nmat]++;
	// count effective material
	if (omat != nmat)
	{
		if (fgPix && ::MaterialMap.Map[nmat].MinHeightCount)
		{
			// Check for material above & below
			int iMinHeight = ::MaterialMap.Map[nmat].MinHeightCount,
			                 iBelow = GetMatHeight(x, y+1, +1, nmat, iMinHeight),
			                          iAbove = GetMatHeight(x, y-1, -1, nmat, iMinHeight);
			// Will be above treshold?
			if (iBelow + iAbove + 1 >= iMinHeight)
			{
				int iChange = 1;
				// Check for heights below threshold
				if (iBelow < iMinHeight) iChange += iBelow;
				if (iAbove < iMinHeight) iChange += iAbove;
				// Change
				EffectiveMatCount[nmat] += iChange;
			}
		}
		if (opix && ::MaterialMap.Map[omat].MinHeightCount)
		{
			// Check for material above & below
			int iMinHeight = ::MaterialMap.Map[omat].MinHeightCount,
			                 iBelow = GetMatHeight(x, y+1, +1, omat, iMinHeight),
			                          iAbove = GetMatHeight(x, y-1, -1, omat, iMinHeight);
			// Not already below threshold?
			if (iBelow + iAbove + 1 >= iMinHeight)
			{
				int iChange = 1;
				// Check for heights that will get below threshold
				if (iBelow < iMinHeight) iChange += iBelow;
				if (iAbove < iMinHeight) iChange += iAbove;
				// Change
				EffectiveMatCount[omat] -= iChange;
			}
		}
	}
	// set 8bpp-surface only!
	Surface8->SetPix(x, y, fgPix);
	Surface8Bkg->SetPix(x, y, bgPix);
	// note for relight
	if(pLandscapeRender)
	{
		C4Rect CheckRect = pLandscapeRender->GetAffectedRect(C4Rect(x, y, 1, 1));
		for (int32_t i = 0; i < C4LS_MaxRelights; i++)
			if (!Relights[i].Wdt || Relights[i].Overlap(CheckRect) || i + 1 >= C4LS_MaxRelights)
			{
				Relights[i].Add(CheckRect);
				break;
			}
		// Invalidate FoW
		if (pFoW)
			pFoW->Invalidate(CheckRect);
	}
	// success
	return true;
}

void C4Landscape::_SetPix2Tmp(int32_t x, int32_t y, BYTE fgPix, BYTE bgPix)
{
	// set 8bpp-surface only!
	assert(x >= 0 && y >= 0 && x < Width && y < Height);
	if (fgPix != Transparent) Surface8->SetPix(x, y, fgPix);
	if (bgPix != Transparent) Surface8Bkg->SetPix(x, y, bgPix);
}

bool C4Landscape::CheckInstability(int32_t tx, int32_t ty, int32_t recursion_count)
{
	int32_t mat=GetMat(tx,ty);
	if (MatValid(mat)) {
		const C4Material &material = MaterialMap.Map[mat];
		if (material.Instable)
			return ::MassMover.Create(tx,ty);
		// Get rid of single pixels
		else if (DensitySolid(material.Density) && !material.KeepSinglePixels && recursion_count<10) 
			if ((!::GBackSolid(tx,ty+1)) + (!::GBackSolid(tx,ty-1)) + (!::GBackSolid(tx+1,ty)) + (!::GBackSolid(tx-1,ty)) >= 3)
			{
				if (!ClearPix(tx,ty)) return false;
				// Diggable material drops; other material just gets removed
				if (material.DigFree) ::PXS.Create(mat,itofix(tx),itofix(ty));
				// check other pixels around this
				// Note this cannot lead to an endless recursion (unless you do funny stuff like e.g. set DigFree=1 in material Tunnel).
				// Check recursion anyway, because very large strips of single pixel width might cause sufficient recursion to crash
				CheckInstability(tx+1,ty,++recursion_count);
				CheckInstability(tx-1,ty,recursion_count);
				CheckInstability(tx,ty-1,recursion_count);
				CheckInstability(tx,ty+1,recursion_count);
				return true;
			}
	}
	return false;
}

void C4Landscape::CheckInstabilityRange(int32_t tx, int32_t ty)
{
	if (!CheckInstability(tx,ty))
	{
		CheckInstability(tx,ty-1);
		CheckInstability(tx,ty-2);
		CheckInstability(tx-1,ty);
		CheckInstability(tx+1,ty);
	}
}
void C4Landscape::DrawMaterialRect(int32_t mat, int32_t tx, int32_t ty, int32_t wdt, int32_t hgt)
{
	int32_t cx,cy;
	for (cy=ty; cy<ty+hgt; cy++)
		for (cx=tx; cx<tx+wdt; cx++)
			if ( (MatDensity(mat)>=GetDensity(cx,cy)))
				SetPix2(cx,cy,Mat2PixColDefault(mat), Surface8Bkg->GetPix(cx, cy));
}

void C4Landscape::RaiseTerrain(int32_t tx, int32_t ty, int32_t wdt)
{
	int32_t cx,cy;
	BYTE cpix;
	for (cx=tx; cx<tx+wdt; cx++)
	{
		for (cy=ty; (cy+1<GBackHgt) && !GBackSolid(cx,cy+1); cy++) {}
		if (cy+1<GBackHgt) if (cy-ty<20)
			{
				cpix=GetPix(cx,cy+1);
				if (!MatVehicle(PixCol2Mat(cpix)))
					while (cy>=ty) { SetPix2(cx,cy,cpix,GetBackPix(cx,cy+1)); cy--; }
			}
	}
}


int32_t C4Landscape::ExtractMaterial(int32_t fx, int32_t fy, bool distant_first)
{
	int32_t mat=GetMat(fx,fy);
	if (mat==MNone) return MNone;
	FindMatTop(mat,fx,fy,distant_first);
	ClearPix(fx,fy);
	CheckInstabilityRange(fx,fy);
	return mat;
}

bool C4Landscape::InsertMaterialOutsideLandscape(int32_t tx, int32_t ty, int32_t mdens)
{
	// Out-of-bounds insertion considered successful if inserted into same or lower density
	// This ensures pumping out of map works
	// Do allow insertion into same density because it covers the case of e.g. pumping water into the upper ocean of an underwater scenario
	return GetDensity(tx, ty) <= mdens;
}

bool C4Landscape::InsertMaterial(int32_t mat, int32_t *tx, int32_t *ty, int32_t vx, int32_t vy, bool query_only)
{
	assert(tx); assert(ty);
	int32_t mdens;
	if (!MatValid(mat)) return false;
	mdens=std::min(MatDensity(mat), C4M_Solid);
	if (!mdens) return true;

	// Bounds
	if (!Inside<int32_t>(*tx, 0, Width - 1) || !Inside<int32_t>(*ty, 0, Height)) return InsertMaterialOutsideLandscape(*tx, *ty, mdens);

	if (Game.C4S.Game.Realism.LandscapePushPull)
	{
		// Same or higher density?
		if (GetDensity(*tx, *ty) >= mdens)
			// Push
			if (!FindMatPathPush(*tx, *ty, mdens, ::MaterialMap.Map[mat].MaxSlide, !! ::MaterialMap.Map[mat].Instable))
				// Or die
				return false;
	}
	else
	{
		// Move up above same density
		while (mdens==std::min(GetDensity(*tx,*ty),C4M_Solid))
		{
			(*ty)--; if (*ty<0) return false;
			// Primitive slide (1)
			if (GetDensity(*tx-1,*ty)<mdens) (*tx)--;
			if (GetDensity(*tx+1,*ty)<mdens) (*tx)++;
		}
		// Stuck in higher density
		if (GetDensity(*tx,*ty)>mdens) return false;
	}

	// Try slide
	while (FindMatSlide(*tx,*ty,+1,mdens,::MaterialMap.Map[mat].MaxSlide))
		if (GetDensity(*tx,*ty+1)<mdens)
			{ if (!query_only) ::PXS.Create(mat,itofix(*tx),itofix(*ty),C4REAL10(vx),C4REAL10(vy)); return true; }

	if (query_only) 
	{
		// since tx and ty changed, we need to re-check the bounds here
		// if we really inserted it, the check is made again in InsertDeadMaterial
		if (!Inside<int32_t>(*tx,0,Width-1) || !Inside<int32_t>(*ty,0,Height)) return InsertMaterialOutsideLandscape(*tx, *ty, mdens);
		return true;		
	}

	// Try reaction with material below and at insertion position
	C4MaterialReaction *pReact; int32_t tmat;
	int32_t check_dir = 0;
	for (int32_t i=0; i<2; ++i)
	{
		if ((pReact = ::MaterialMap.GetReactionUnsafe(mat, tmat=GetMat(*tx,*ty+check_dir))))
		{
			C4Real fvx=C4REAL10(vx), fvy=C4REAL10(vy);
			if ((*pReact->pFunc)(pReact, *tx,*ty, *tx,*ty+check_dir, fvx,fvy, mat,tmat, meePXSPos,NULL))
			{
				// the material to be inserted killed itself in some material reaction below
				return true;
			}
		}
		if (!(check_dir = Sign(GravAccel))) break;
	}

	// Insert as dead material
	return InsertDeadMaterial(mat, *tx, *ty);
}

bool C4Landscape::InsertDeadMaterial(int32_t mat, int32_t tx, int32_t ty)
{
	// Check bounds
	if (tx < 0 || ty < 0 || tx >= Width || ty >= Height)
		return InsertMaterialOutsideLandscape(tx, ty, std::min(MatDensity(mat), C4M_Solid));

	// Save back original material so we can insert it later
	int omat = 0;
	if (Game.C4S.Game.Realism.LandscapeInsertThrust)
		omat = GetMat(tx, ty);

	// Check surroundings for inspiration for texture to use
	int n = 0; int pix = -1;
	if(tx > 0 && _GetMat(tx-1, ty) == mat)
		if(!Random(++n)) pix = _GetPix(tx-1, ty);
	if(ty > 0 && _GetMat(tx, ty-1) == mat)
		if(!Random(++n)) pix = _GetPix(tx, ty-1);
	if(tx+1 < Width && _GetMat(tx+1, ty) == mat)
		if(!Random(++n)) pix = _GetPix(tx+1, ty);
	if(ty+1 < Height && _GetMat(tx, ty+1) == mat)
		if(!Random(++n)) pix = _GetPix(tx, ty+1);
	if(pix < 0)
		pix = Mat2PixColDefault(mat);

	// Insert dead material
	SetPix2(tx,ty,pix, Transparent);

	// Search a position for the old material pixel
	if (Game.C4S.Game.Realism.LandscapeInsertThrust && MatValid(omat))
	{
		int32_t tyo = ty-1;
		InsertMaterial(omat, &tx, &tyo);
	}

	return true;
}

bool C4Landscape::Incinerate(int32_t x, int32_t y, int32_t cause_player)
{
	int32_t mat=GetMat(x,y);
	if (MatValid(mat))
		if (::MaterialMap.Map[mat].Inflammable)
		{
			C4AulParSet pars(C4VInt(x), C4VInt(y), C4VInt(cause_player));
			::ScriptEngine.GetPropList()->Call(P_InflameLandscape, &pars);
		}
	return false;
}

BYTE C4Landscape::DefaultBkgMat(BYTE fg) const
{
	return ::TextureMap.DefaultBkgMatTex(fg);
}

CSurface8* C4Landscape::CreateDefaultBkgSurface(CSurface8& sfcFg, bool msbAsIft) const
{
	CSurface8* sfcBg = new CSurface8();
	if (!sfcBg->Create(sfcFg.Wdt, sfcFg.Hgt))
		{ delete sfcBg; return NULL; }

	for (int32_t y=0; y<sfcFg.Hgt; ++y)
	{
		for (int32_t x=0; x<sfcFg.Wdt; ++x)
		{
			BYTE fgPix = sfcFg._GetPix(x, y);
			BYTE bgPix;

			// If we treat the most significant bit as the IFT flag
			// (compatibility option for pre-7.0 maps), then set
			// the background pixel to 0 if the foreground does not
			// have IFT set, and remove the IFT flag from the
			// foreground pixel.
			if (msbAsIft)
			{
				if (fgPix & 0x80)
				{
					fgPix &= ~0x80;
					sfcFg._SetPix(x, y, fgPix);
				 	bgPix = DefaultBkgMat(fgPix);
				}
				else
				{
					bgPix = 0;
				}
			}
			else
			{
				 bgPix = DefaultBkgMat(fgPix);
			}

			sfcBg->_SetPix(x, y, bgPix);
		}
	}

	return sfcBg;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++ Polygon drawing code extracted from ALLEGRO by Shawn Hargreaves ++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

struct CPolyEdge          // An edge for the polygon drawer
{
	int y;                  // Current (starting at the top) y position
	int bottom;             // bottom y position of this edge
	int x;                  // Fixed point x position
	int dx;                 // Fixed point x gradient
	int w;                  // Width of line segment
	struct CPolyEdge *prev; // Doubly linked list
	struct CPolyEdge *next;
};

#define POLYGON_FIX_SHIFT     16

static void fill_edge_structure(CPolyEdge *edge, int *i1, int *i2)
{
	if (i2[1] < i1[1]) // Swap
		{ int *t=i1; i1=i2; i2=t; }
	edge->y = i1[1];
	edge->bottom = i2[1] - 1;
	edge->dx = ((i2[0] - i1[0]) << POLYGON_FIX_SHIFT) / (i2[1] - i1[1]);
	edge->x = (i1[0] << POLYGON_FIX_SHIFT) + (1<<(POLYGON_FIX_SHIFT-1)) - 1;
	edge->prev = NULL;
	edge->next = NULL;
	if (edge->dx < 0)
		edge->x += std::min<int>(edge->dx+(1<<POLYGON_FIX_SHIFT), 0);
	edge->w = std::max<int>(Abs(edge->dx)-(1<<POLYGON_FIX_SHIFT), 0);
}

static CPolyEdge *add_edge(CPolyEdge *list, CPolyEdge *edge, int sort_by_x)
{
	CPolyEdge *pos = list;
	CPolyEdge *prev = NULL;
	if (sort_by_x)
	{
		while ((pos) && (pos->x+pos->w/2 < edge->x+edge->w/2))
			{ prev = pos; pos = pos->next; }
	}
	else
	{
		while ((pos) && (pos->y < edge->y))
			{ prev = pos; pos = pos->next; }
	}
	edge->next = pos;
	edge->prev = prev;
	if (pos) pos->prev = edge;
	if (prev) { prev->next = edge; return list; }
	else return edge;
}

static CPolyEdge *remove_edge(CPolyEdge *list, CPolyEdge *edge)
{
	if (edge->next) edge->next->prev = edge->prev;
	if (edge->prev) { edge->prev->next = edge->next; return list; }
	else return edge->next;
}

// Global polygon quick buffer
const int QuickPolyBufSize = 20;
CPolyEdge QuickPolyBuf[QuickPolyBufSize];

int32_t C4Landscape::ForPolygon(int *vtcs, int length, bool (C4Landscape::*fnCallback)(int32_t, int32_t),
														 C4MaterialList *mats_count, uint8_t col, uint8_t colBkg, uint8_t *conversion_table)
{
	// Variables for polygon drawer
	int c,x1,x2,y;
	int top = INT_MAX;
	int bottom = INT_MIN;
	int *i1, *i2;
	CPolyEdge *edge, *next_edge, *edgebuf;
	CPolyEdge *active_edges = NULL;
	CPolyEdge *inactive_edges = NULL;
	bool use_qpb=false;

	// Return value
	int32_t amount = 0;
	// Poly Buf
	if (length<=QuickPolyBufSize)
		{ edgebuf=QuickPolyBuf; use_qpb=true; }
	else if (!(edgebuf = new CPolyEdge [length])) { return 0; }

	// Fill the edge table
	edge = edgebuf;
	i1 = vtcs;
	i2 = vtcs + (length-1) * 2;
	for (c=0; c<length; c++)
	{
		if (i1[1] != i2[1])
		{
			fill_edge_structure(edge, i1, i2);
			if (edge->bottom >= edge->y)
			{
				if (edge->y < top)  top = edge->y;
				if (edge->bottom > bottom) bottom = edge->bottom;
				inactive_edges = add_edge(inactive_edges, edge, false);
				edge++;
			}
		}
		i2 = i1; i1 += 2;
	}

	// For each scanline in the polygon...
	for (c=top; c<=bottom; c++)
	{
		// Check for newly active edges
		edge = inactive_edges;
		while ((edge) && (edge->y == c))
		{
			next_edge = edge->next;
			inactive_edges = remove_edge(inactive_edges, edge);
			active_edges = add_edge(active_edges, edge, true);
			edge = next_edge;
		}

		// Draw horizontal line segments
		edge = active_edges;
		while ((edge) && (edge->next))
		{
			x1=edge->x>>POLYGON_FIX_SHIFT;
			x2=(edge->next->x+edge->next->w)>>POLYGON_FIX_SHIFT;
			y=c;
			// Fix coordinates
			if (x1>x2) std::swap(x1,x2);
			// Set line
			if (fnCallback)
			{
				for (int xcnt=x2-x1-1; xcnt>=0; xcnt--)
				{
					uint8_t pix = GetPix(x1+xcnt, y);
					if (!conversion_table || conversion_table[pix])
					{
						int32_t mat = GetPixMat(pix);
						if((this->*fnCallback)(x1+xcnt,y))
							if(mats_count)
							{
								mats_count->Add(mat,1);
								amount++;
							}
					}
				}
			}
			else if (conversion_table)
				for (int xcnt=x2-x1-1; xcnt>=0; xcnt--)
				{
					const uint8_t pix = conversion_table[uint8_t(GetPix(x1+xcnt, y))];
					Surface8->SetPix(x1+xcnt, y, pix);
					if (colBkg != Transparent) Surface8Bkg->SetPix(x1+xcnt, y, colBkg);
				}
			else
				for (int xcnt=x2-x1-1; xcnt>=0; xcnt--)
				{
					if (col != Transparent) Surface8->SetPix(x1+xcnt, y, col);
					if (colBkg != Transparent) Surface8Bkg->SetPix(x1+xcnt, y, colBkg);
				}
			edge = edge->next->next;
		}

		// Update edges, sorting and removing dead ones
		edge = active_edges;
		while (edge)
		{
			next_edge = edge->next;
			if (c >= edge->bottom)
			{
				active_edges = remove_edge(active_edges, edge);
			}
			else
			{
				edge->x += edge->dx;
				while ((edge->prev) && (edge->x+edge->w/2 < edge->prev->x+edge->prev->w/2))
				{
					if (edge->next) edge->next->prev = edge->prev;
					edge->prev->next = edge->next;
					edge->next = edge->prev;
					edge->prev = edge->prev->prev;
					edge->next->prev = edge;
					if (edge->prev) edge->prev->next = edge;
					else active_edges = edge;
				}
			}
			edge = next_edge;
		}
	}

	// Clear scratch memory
	if (!use_qpb) delete [] edgebuf;

	return amount;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++   Save, Init and load   +++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

void C4Landscape::ScenarioInit()
{
	// Gravity
	Gravity = C4REAL100(Game.C4S.Landscape.Gravity.Evaluate()) * DefaultGravAccel;
	// Opens
	LeftOpen=Game.C4S.Landscape.LeftOpen;
	RightOpen=Game.C4S.Landscape.RightOpen;
	TopOpen=Game.C4S.Landscape.TopOpen;
	BottomOpen=Game.C4S.Landscape.BottomOpen;
	// Side open scan
	if (Game.C4S.Landscape.AutoScanSideOpen) ScanSideOpen();
}

void C4Landscape::Clear(bool fClearMapCreator, bool fClearSky, bool fClearRenderer)
{
	if (pMapCreator && fClearMapCreator) { delete pMapCreator; pMapCreator=NULL; }
	// clear sky
	if (fClearSky) Sky.Clear();
	// clear surfaces, if assigned
	if (fClearRenderer) { delete pLandscapeRender; pLandscapeRender=NULL; }
	delete [] TopRowPix; TopRowPix=NULL;
	delete [] BottomRowPix; BottomRowPix=NULL;
	delete Surface8; Surface8=NULL;
	delete Surface8Bkg; Surface8Bkg=NULL;
	delete Map; Map=NULL;
	delete MapBkg; MapBkg=NULL;
	// clear initial landscape
	delete [] pInitial; pInitial = NULL;
	delete [] pInitialBkg; pInitialBkg = NULL;
	delete pFoW; pFoW = NULL;
	// clear relight array
	for (int32_t i = 0; i < C4LS_MaxRelights; ++i)
		Relights[i].Default();
	// clear scan
	ScanX=0;
	Mode=C4LSC_Undefined;
	// clear pixel count
	delete [] PixCnt; PixCnt = NULL;
	PixCntPitch = 0;
	// clear bridge material conversion temp buffers
	for (int32_t i = 0; i < C4M_MaxTexIndex; ++i)
	{
		delete [] BridgeMatConversion[i];
		BridgeMatConversion[i] = NULL;
	}
}

void C4Landscape::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(MapSeed,             "MapSeed",               0));
	pComp->Value(mkNamingAdapt(LeftOpen,            "LeftOpen",              0));
	pComp->Value(mkNamingAdapt(RightOpen,           "RightOpen",             0));
	pComp->Value(mkNamingAdapt(TopOpen,             "TopOpen",               0));
	pComp->Value(mkNamingAdapt(BottomOpen,          "BottomOpen",            0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(Gravity), "Gravity",           DefaultGravAccel));
	pComp->Value(mkNamingAdapt(Modulation,          "MatModulation",         0U));
	pComp->Value(mkNamingAdapt(Mode,                "Mode",                  C4LSC_Undefined));

	if(pComp->isCompiler())
	{
		int32_t ambient_brightness;
		pComp->Value(mkNamingAdapt(ambient_brightness,       "AmbientBrightness",  255));
		if(pFoW) pFoW->Ambient.SetBrightness(ambient_brightness / static_cast<double>(255));
	}
	else
	{
		if(pFoW)
		{
			int32_t ambient_brightness = static_cast<int32_t>(pFoW->Ambient.GetBrightness() * 255 + 0.5);
			pComp->Value(mkNamingAdapt(ambient_brightness,       "AmbientBrightness", 255));
		}
	}
}

static CSurface8 *GroupReadSurface8(C4Group &hGroup, const char *szWildCard)
{
	if (!hGroup.AccessEntry(szWildCard))
		return NULL;
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup))
		{ delete pSfc; return NULL; }
	return pSfc;
}

bool C4Landscape::Init(C4Group &hGroup, bool fOverloadCurrent, bool fLoadSky, bool &rfLoaded, bool fSavegame)
{
	// set map seed, if not pre-assigned
	if (!MapSeed) MapSeed=Random(3133700);

	// increase max map size, since developers might set a greater one here
	Game.C4S.Landscape.MapWdt.Max=10000;
	Game.C4S.Landscape.MapHgt.Max=10000;

	// map and landscape must be initialized with fixed random, so runtime joining clients may recreate it
	// with same seed
	// after map/landscape creation, the seed must be fixed again, so there's no difference between clients creating
	// and not creating the map
	// this, however, would cause syncloss to DebugRecs
	C4DebugRecOff DBGRECOFF(!!Game.C4S.Landscape.ExactLandscape);

	Game.FixRandom(Game.RandomSeed);

	// map is like it's loaded for regular gamestart
	// but it's changed and would have to be saved if a new section is loaded
	fMapChanged = fOverloadCurrent;

	// don't change landscape mode in runtime joins
	bool fLandscapeModeSet = (Mode != C4LSC_Undefined);

	// Make pixel maps
	// Pixel maps only depend on loaded materials and textures
	// They might be accessed in map scripts, so they should be ready before map creation
	UpdatePixMaps();

	Game.SetInitProgress(60);
	// create map if necessary
	if (!Game.C4S.Landscape.ExactLandscape)
	{
		CSurface8 * sfcMap=NULL;
		CSurface8 * sfcMapBkg=NULL;

		// Static map from scenario: Old-style Map.bmp with highest bit IFT
		if ((sfcMap=GroupReadSurface8(hGroup, C4CFN_Map)))
		{
			if (!fLandscapeModeSet) Mode=C4LSC_Static;
			sfcMapBkg = CreateDefaultBkgSurface(*sfcMap, true);
			if (!sfcMapBkg) return false;
		}

		// Static map from scenaruo: New-style MapFg.bmp and MapBg.bmp with
		// full 255 mat-tex combinations. Background map is optional, if not
		// given default will be created with tunnel background for all
		// semisolid pixels.
		if (!sfcMap)
		{
			if ((sfcMap=GroupReadSurface8(hGroup, C4CFN_MapFg)))
			{
				if (!fLandscapeModeSet) Mode=C4LSC_Static;
				sfcMapBkg = GroupReadSurface8(hGroup, C4CFN_MapBg);
				if (!sfcMapBkg) sfcMapBkg = CreateDefaultBkgSurface(*sfcMap, false);
				if (!sfcMapBkg) return false;
			}
		}

		// dynamic map from Landscape.txt
		if (!sfcMap)
			if (CreateMapS2(hGroup, sfcMap, sfcMapBkg))
				if (!fLandscapeModeSet) Mode=C4LSC_Dynamic;

		// script may create or edit map
		if (MapScript.InitializeMap(&Game.C4S.Landscape, &::TextureMap, &::MaterialMap, Game.StartupPlayerCount, &sfcMap, &sfcMapBkg))
			if (!fLandscapeModeSet) Mode=C4LSC_Dynamic;

		// Dynamic map by scenario
		if (!sfcMap && !fOverloadCurrent)
			if ((!CreateMap(sfcMap, sfcMapBkg)))
				if (!fLandscapeModeSet) Mode=C4LSC_Dynamic;

		// No map failure
		if (!sfcMap)
		{
			// no problem if only overloading
			if (!fOverloadCurrent) return false;
			if (fLoadSky) if (!Sky.Init(fSavegame)) return false;
			return true;
		}

		assert(sfcMapBkg != NULL);

		if (Config.General.DebugRec)
		{
			AddDbgRec(RCT_Block, "|---MAP---|", 12);
			AddDbgRec(RCT_Map, sfcMap->Bits, sfcMap->Pitch*sfcMap->Hgt);
		}

		// Store map size and calculate map zoom
		int iWdt, iHgt;
		sfcMap->GetSurfaceSize(iWdt,iHgt);
		MapWidth = iWdt; MapHeight = iHgt;
		MapZoom = Game.C4S.Landscape.MapZoom.Evaluate();

		// Calculate landscape size
		Width = MapZoom * MapWidth;
		Height = MapZoom * MapHeight;
		Width = std::max<int32_t>(Width,100);
		Height = std::max<int32_t>(Height,100);
//    Width = (Width/8)*8;

		// if overloading, clear current landscape (and sections, etc.)
		// must clear, of course, before new sky is eventually read
		if (fOverloadCurrent) Clear(!Game.C4S.Landscape.KeepMapCreator, fLoadSky, false);

		// assign new map
		assert(Map == NULL);
		assert(MapBkg == NULL);
		Map = sfcMap;
		MapBkg = sfcMapBkg;

		// Sky (might need to know landscape height)
		if (fLoadSky)
		{
			Game.SetInitProgress(70);
			if (!Sky.Init(fSavegame)) return false;
		}
	}

	// Exact landscape from scenario (no map or exact recreation)
	else /* if (Game.C4S.Landscape.ExactLandscape) */
	{
		C4DebugRecOff DBGRECOFF;
		// if overloading, clear current
		if (fOverloadCurrent) Clear(!Game.C4S.Landscape.KeepMapCreator, fLoadSky, false);
		// load it
		if (!fLandscapeModeSet) Mode=C4LSC_Exact;
		rfLoaded=true;
		if (!Load(hGroup, fLoadSky, fSavegame)) return false;
	}

	// progress
	Game.SetInitProgress(75);

	// copy noscan-var
	NoScan=Game.C4S.Landscape.NoScan!=0;

	// Scan settings
	ScanSpeed=Clamp(Width/500,2,15);

	// Create pixel count array before any SetPix operations may take place
	// Proper pixel counts will be done later, but needs to have the arrays redy to avoid dead pointer access.
	// We will use 15x17 blocks so the pixel count can't get over 255.
	int32_t PixCntWidth = (Width + 16) / 17;
	PixCntPitch = (Height + 14) / 15;
	PixCnt = new uint8_t[PixCntWidth * PixCntPitch];

	// map to big surface and sectionize it
	// (not for shaders though - they require continous textures)
	if (!Game.C4S.Landscape.ExactLandscape)
	{
		assert(Surface8 == NULL);
		assert(Surface8Bkg == NULL);

		// Create landscape surface
		Surface8 = new CSurface8();
		if (!Surface8->Create(Width, Height))
		{
			delete Surface8; Surface8 = NULL;
			return false;
		}

		Surface8Bkg = new CSurface8();
		if (!Surface8Bkg->Create(Width, Height) || !Mat2Pal())
		{
			delete Surface8Bkg; Surface8Bkg = NULL;
			delete Surface8; Surface8 = NULL;
			return false;
		}

		// Map to landscape
		// Landscape render disabled during initial landscape zoom (will be updated later)
		C4LandscapeRender *lsrender_backup  = pLandscapeRender;
		pLandscapeRender = NULL;
		bool map2landscape_success = MapToLandscape();
		pLandscapeRender = lsrender_backup;
		if (!map2landscape_success) return false;
	}

	// Init out-of-landscape pixels for bottom
	InitTopAndBottomRowPix();

	Game.SetInitProgress(80);

	if (Config.General.DebugRec)
	{
		AddDbgRec(RCT_Block, "|---LANDSCAPE---|", 18);
		AddDbgRec(RCT_Map, Surface8->Bits, Surface8->Pitch*Surface8->Hgt);

		AddDbgRec(RCT_Block, "|---LANDSCAPE BKG---|", 22);
		AddDbgRec(RCT_Map, Surface8Bkg->Bits, Surface8Bkg->Pitch*Surface8Bkg->Hgt);
	}

	// Create FoW
	assert(pFoW == NULL);
	if (Game.C4S.Game.FoWEnabled)
		pFoW = new C4FoW();

	// Create renderer
#ifndef USE_CONSOLE
	if (!pLandscapeRender && ::Config.Graphics.HighResLandscape)
		pLandscapeRender = new C4LandscapeRenderGL();
#endif
#ifndef USE_CONSOLE
	if (!pLandscapeRender)
		pLandscapeRender = new C4LandscapeRenderClassic();
#endif

	if(pLandscapeRender)
	{
		// Initialize renderer
		if (fOverloadCurrent)
		{
			if(!pLandscapeRender->ReInit(Width, Height))
				return false;
		}
		else
		{
			if(!pLandscapeRender->Init(Width, Height, &::TextureMap, &::GraphicsResource.Files))
				return false;
		}
	}

	// Save initial landscape
	if (!SaveInitial())
		return false;

	// Load diff, if existant
	ApplyDiff(hGroup);

	// Pixel count tracking from landscape zoom is incomplete, so recalculate it.
	UpdatePixCnt(C4Rect(0, 0, Width, Height));
	ClearMatCount();
	UpdateMatCnt(C4Rect(0, 0, Width, Height), true);

	// Create initial landscape render data (after applying diff so landscape is complete)
	if (pLandscapeRender) pLandscapeRender->Update(C4Rect(0, 0, Width, Height), this);
	Game.SetInitProgress(87);

	// after map/landscape creation, the seed must be fixed again, so there's no difference between clients creating
	// and not creating the map
	Game.FixRandom(Game.RandomSeed);

	// Create ambient light map after landscape creation
	if (pFoW) pFoW->Ambient.CreateFromLandscape(*this, 10., 50., 0.25);
	Game.SetInitProgress(84);

	// Success
	rfLoaded=true;
	return true;
}

bool C4Landscape::Save(C4Group &hGroup) const
{
	C4SolidMask::RemoveSolidMasks();
	bool r = SaveInternal(hGroup);
	C4SolidMask::PutSolidMasks();
	return r;
}

bool C4Landscape::SaveInternal(C4Group &hGroup) const
{
	// Save landscape surface
	char szTempLandscape[_MAX_PATH+1];
	SCopy(Config.AtTempPath(C4CFN_TempLandscape), szTempLandscape);
	MakeTempFilename(szTempLandscape);
	if (!Surface8->Save(szTempLandscape))
		return false;

	// Move temp file to group
	if (!hGroup.Move( szTempLandscape, C4CFN_LandscapeFg ))
		return false;

	// Same for background surface
	SCopy(Config.AtTempPath(C4CFN_TempLandscapeBkg), szTempLandscape);
	MakeTempFilename(szTempLandscape);
	if (!Surface8Bkg->Save(szTempLandscape))
		return false;
	if (!hGroup.Move(szTempLandscape, C4CFN_LandscapeBg))
		return false;

	// Save map
	if (fMapChanged && Map)
		if (!SaveMap(hGroup))
			return false;

	// save textures (if changed)
	if (!SaveTextures(hGroup))
		return false;

	return true;
}

bool C4Landscape::SaveDiff(C4Group &hGroup, bool fSyncSave) const
{
	C4SolidMask::RemoveSolidMasks();
	bool r = SaveDiffInternal(hGroup, fSyncSave);
	C4SolidMask::PutSolidMasks();
	return r;
}

bool C4Landscape::SaveDiffInternal(C4Group &hGroup, bool fSyncSave) const
{
	assert(pInitial && pInitialBkg);
	if (!pInitial || !pInitialBkg) return false;

	// If it shouldn't be sync-save: Clear all bytes that have not changed, i.e.
	// set them to C4M_MaxTexIndex
	bool fChanged = false, fChangedBkg = false;;
	if (!fSyncSave)
		for (int y = 0; y < Height; y++)
			for (int x = 0; x < Width; x++)
			{
				if (pInitial[y * Width + x] == Surface8->_GetPix(x, y))
					Surface8->SetPix(x,y,C4M_MaxTexIndex);
				else
					fChanged = true;

				if (pInitialBkg[y * Width + x] == Surface8Bkg->_GetPix(x, y))
					Surface8Bkg->SetPix(x,y,C4M_MaxTexIndex);
				else
					fChangedBkg = true;
			}

	if (fSyncSave || fChanged)
	{
		// Save landscape surface
		if (!Surface8->Save(Config.AtTempPath(C4CFN_TempLandscape)))
			return false;

		// Move temp file to group
		if (!hGroup.Move( Config.AtTempPath(C4CFN_TempLandscape),
		                  C4CFN_DiffLandscape ))
			return false;
	}

	if (fSyncSave || fChangedBkg)
	{
		// Save landscape surface
		if (!Surface8Bkg->Save(Config.AtTempPath(C4CFN_TempLandscapeBkg)))
			return false;

		// Move temp file to group
		if (!hGroup.Move( Config.AtTempPath(C4CFN_TempLandscapeBkg),
		                  C4CFN_DiffLandscapeBkg ))
			return false;
	}

	// Restore landscape pixels
	if (!fSyncSave)
		for (int y = 0; y < Height; y++)
			for (int x = 0; x < Width; x++)
			{
				if (Surface8->_GetPix(x, y) == C4M_MaxTexIndex)
					Surface8->SetPix(x,y,pInitial[y * Width + x]);
				if (Surface8Bkg->_GetPix(x, y) == C4M_MaxTexIndex)
					Surface8Bkg->SetPix(x,y,pInitialBkg[y * Width + x]);
			}

	// Save changed map, too
	if (fMapChanged && Map)
		if (!SaveMap(hGroup)) return false;

	// and textures (if changed)
	if (!SaveTextures(hGroup)) return false;

	return true;
}

bool C4Landscape::SaveInitial()
{

	// Create array
	delete [] pInitial;
	delete [] pInitialBkg;
	pInitial = new BYTE [Width * Height];
	pInitialBkg = new BYTE [Width * Height];

	// Save material data
	for (int y = 0; y < Height; y++)
		for (int x = 0; x < Width; x++)
		{
			pInitial[y * Width + x] = Surface8->_GetPix(x, y);
			pInitialBkg[y * Width + x] = Surface8Bkg->_GetPix(x, y);
		}

	return true;
}

bool C4Landscape::Load(C4Group &hGroup, bool fLoadSky, bool fSavegame)
{
	assert(!Surface8 && !Surface8Bkg);

	// Load exact landscape from group
	if (!(Surface8=GroupReadSurface8(hGroup, C4CFN_Landscape)))
	{
		if (!(Surface8=GroupReadSurface8(hGroup, C4CFN_LandscapeFg))) return false;
		Surface8Bkg = GroupReadSurface8(hGroup, C4CFN_LandscapeBg);

		if (Surface8Bkg)
		{
			if ( (Surface8->Wdt != Surface8Bkg->Wdt || Surface8->Hgt != Surface8Bkg->Hgt))
			{
				LogFatal(FormatString("Landscape has different dimensions than background landscape (%dx%d vs. %dx%d)", Surface8->Wdt, Surface8->Hgt, Surface8Bkg->Wdt, Surface8Bkg->Hgt).getData());
				return false;
			}
		}
		else
		{
			// LandscapeFg.bmp loaded: Assume full 8bit mat-tex values
			// when creating background surface.
			Surface8Bkg = CreateDefaultBkgSurface(*Surface8, false);
		}
	}
	else
	{
		// Landscape.bmp loaded: Assume msb is IFT flag when creating
		// background surface.
		Surface8Bkg = CreateDefaultBkgSurface(*Surface8, true);
	}

	int iWidth, iHeight;
	Surface8->GetSurfaceSize(iWidth,iHeight);
	Width = iWidth; Height = iHeight;

	// adjust pal
	if (!Mat2Pal()) return false;
	// Landscape should be in correct format: Make sure it is!
	for (int32_t y=0; y<Height; ++y)
		for (int32_t x=0; x<Width; ++x)
		{
			BYTE byPix = Surface8->_GetPix(x, y);
			int32_t iMat = PixCol2Mat(byPix);

			if (byPix && !MatValid(iMat))
			{
				LogFatal(FormatString("Landscape loading error at (%d/%d): Pixel value %d not a valid material!", (int) x, (int) y, (int) byPix).getData());
				return false;
			}

			BYTE byPixBkg = Surface8Bkg->_GetPix(x, y);
			int32_t iMatBkg = PixCol2Mat(byPixBkg);

			if (byPixBkg && !MatValid(iMatBkg))
			{
				LogFatal(FormatString("Background Landscape loading error at (%d/%d): Pixel value %d not a valid material!", (int) x, (int) y, (int) byPixBkg).getData());
				return false;
			}
		}

	// Init sky
	if (fLoadSky)
	{
		Game.SetInitProgress(70);
		if (!Sky.Init(fSavegame)) return false;
	}
	// Success
	return true;
}
bool C4Landscape::ApplyDiff(C4Group &hGroup)
{
	CSurface8 *pDiff;
	CSurface8 *pDiffBkg;
	// Load diff landscape from group
	if (!(pDiff=GroupReadSurface8(hGroup, C4CFN_DiffLandscape))) return false;
	if (!(pDiffBkg=GroupReadSurface8(hGroup, C4CFN_DiffLandscapeBkg))) { delete pDiff; return false; }

	// convert all pixels: keep if same material; re-set if different material
	BYTE byPix;
	for (int32_t y=0; y<Height; ++y) for (int32_t x=0; x<Width; ++x)
	{
		if (pDiff->GetPix(x, y) != C4M_MaxTexIndex)
			if (Surface8->_GetPix(x,y) != (byPix=pDiff->_GetPix(x,y)))
				// material has changed here: readjust with new texture
				Surface8->SetPix(x,y, byPix);
		if (pDiffBkg->GetPix(x, y) != C4M_MaxTexIndex)
			if (Surface8Bkg->_GetPix(x, y) != (byPix=pDiffBkg->_GetPix(x, y)))
				Surface8Bkg->_SetPix(x, y, byPix);
	}

	// done; clear diff
	delete pDiff;
	delete pDiffBkg;
	return true;
}

void C4Landscape::Default()
{
	Mode=C4LSC_Undefined;
	Surface8=NULL;
	Surface8Bkg=NULL;
	pInitial=NULL;
	pInitialBkg=NULL;
	TopRowPix=NULL;
	BottomRowPix=NULL;
	pLandscapeRender=NULL;
	Map=NULL;
	MapBkg=NULL;
	Width=Height=0;
	MapWidth=MapHeight=MapZoom=0;
	ClearMatCount();
	ScanX=0;
	ScanSpeed=2;
	LeftOpen=RightOpen=TopOpen=BottomOpen=0;
	Gravity=DefaultGravAccel;
	MapSeed=0; NoScan=false;
	pMapCreator=NULL;
	Modulation=0;
	fMapChanged = false;
	for (int32_t i = 0; i < C4M_MaxTexIndex; ++i)
		BridgeMatConversion[i] = NULL;
	pFoW = NULL;
}

void C4Landscape::ClearMatCount()
{
	for (int32_t cnt=0; cnt<C4MaxMaterial; cnt++)
	{
		MatCount[cnt]=0;
		EffectiveMatCount[cnt]=0;
	}
}

void C4Landscape::Synchronize()
{
	ScanX=0;
}


int32_t PixCol2Mat(BYTE pixc)
{
	// Get texture
	int32_t iTex = PixCol2Tex(pixc);
	if (!iTex) return MNone;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
	// Return material
	return pTex ? pTex->GetMaterialIndex() : MNone;
}

bool C4Landscape::SaveMap(C4Group &hGroup) const
{
	// No map
	if (!Map) return false;
	assert(MapBkg != NULL);

	// Create map palette
	CStdPalette Palette;
	::TextureMap.StoreMapPalette(&Palette,::MaterialMap);

	// Save map surface
	if (!Map->Save(Config.AtTempPath(C4CFN_TempMapFg), &Palette))
		return false;

	// Move temp file to group
	if (!hGroup.Move(Config.AtTempPath(C4CFN_TempMapFg),
	                 C4CFN_MapFg ))
		return false;

	// Save background map surface
	if (!MapBkg->Save(Config.AtTempPath(C4CFN_TempMapBg), &Palette))
		return false;

	// Move temp file to group
	if (!hGroup.Move(Config.AtTempPath(C4CFN_TempMapBg),
	                 C4CFN_MapBg ))
		return false;
	
	// Success
	return true;
}

bool C4Landscape::SaveTextures(C4Group &hGroup) const
{
	// if material-texture-combinations have been added, write the texture map
	if (::TextureMap.fEntriesAdded)
	{
		C4Group *pMatGroup = new C4Group();
		bool fSuccess=false;
		// create local material group
		if (!hGroup.FindEntry(C4CFN_Material))
		{
			// delete previous item at temp path
			EraseItem(Config.AtTempPath(C4CFN_Material));
			// create at temp path
			if (pMatGroup->Open(Config.AtTempPath(C4CFN_Material), true))
				// write to it
				if (::TextureMap.SaveMap(*pMatGroup, C4CFN_TexMap))
					// close (flush)
					if (pMatGroup->Close())
						// add it
						if (hGroup.Move(Config.AtTempPath(C4CFN_Material), C4CFN_Material))
							fSuccess=true;
			// temp group must remain for scenario file closure
			// it will be deleted when the group is closed
		}
		else
			// simply write it to the local material file
			if (pMatGroup->OpenAsChild(&hGroup, C4CFN_Material))
				fSuccess = ::TextureMap.SaveMap(*pMatGroup, C4CFN_TexMap);
		// close material group again
		if (pMatGroup->IsOpen()) pMatGroup->Close();
		delete pMatGroup;
		// fail if unsuccessful
		if (!fSuccess) return false;
	}
	// done, success
	return true;
}

bool C4Landscape::InitTopAndBottomRowPix()
{
	// Init Top-/BottomRowPix array, which determines if out-of-landscape pixels on top/bottom side of the map are solid or not
	// In case of Top-/BottomOpen=2, unit by map and not landscape to avoid runtime join sync losses
	delete [] TopRowPix; delete [] BottomRowPix; // safety
	if (!Width) return true;
	TopRowPix = new uint8_t[Width];
	BottomRowPix = new uint8_t[Width];
	// must access Game.C4S here because Landscape.TopOpen / Landscape.BottomOpen may not be initialized yet
	// why is there a local copy of that static variable anyway?
	int32_t top_open_flag = Game.C4S.Landscape.TopOpen;
	int32_t bottom_open_flag = Game.C4S.Landscape.BottomOpen;
	if (top_open_flag == 2 && !Map) top_open_flag = 1;
	if (bottom_open_flag == 2 && !Map) bottom_open_flag = 1;

	// Init TopRowPix
	switch (top_open_flag)
	{
	// TopOpen=0: Top is closed
	case 0: for (int32_t x=0; x<Width; ++x) TopRowPix[x] = MCVehic; break;
	// TopOpen=2: Top is open when pixel below has sky background
	case 2:
		for (int32_t x=0; x<Width; ++x)
		{
			uint8_t map_pix = MapBkg->GetPix(x/MapZoom,0);
			TopRowPix[x] = ((map_pix != 0) ? MCVehic : 0);
		}
		break;
	// TopOpen=1: Top is open
	default: for (int32_t x=0; x<Width; ++x) TopRowPix[x] = 0; break;
	}

	// Init BottomRowPix
	switch (bottom_open_flag)
	{
	// BottomOpen=0: Bottom is closed
	case 0: for (int32_t x=0; x<Width; ++x) BottomRowPix[x] = MCVehic; break;
	// BottomOpen=2: Bottom is open when pixel above has sky background
	case 2:
		for (int32_t x=0; x<Width; ++x)
		{
			uint8_t map_pix = MapBkg->GetPix(x/MapZoom,Map->Hgt-1);
			BottomRowPix[x] = ((map_pix != 0) ? MCVehic : 0);
		}
		break;
	// BottomOpen=1: Bottom is open
	default: for (int32_t x=0; x<Width; ++x) BottomRowPix[x] = 0; break;
	}
	return true;
}

bool C4Landscape::MapToLandscape()
{
	// zoom map to landscape
	return MapToLandscape(Map,MapBkg,0,0,MapWidth,MapHeight);
}


uint32_t C4Landscape::ChunkyRandom(uint32_t & iOffset, uint32_t iRange) const
{
	if (!iRange) return 0;
	iOffset = (iOffset * 16807) % 2147483647;
	return (iOffset ^ MapSeed) % iRange;
}

void C4Landscape::DrawChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, uint8_t mcol, uint8_t mcolBkg, C4MaterialCoreShape Shape, uint32_t cro)
{
	unsigned int top_rough = 0, side_rough = 0, bottom_rough = 0;
	// what to do?
	switch (Shape)
	{
	case C4M_Flat: case C4M_Octagon:
		if (mcol != Transparent) Surface8->Box(tx, ty, tx + wdt, ty + hgt, mcol);
		if (mcolBkg != Transparent) Surface8Bkg->Box(tx, ty, tx + wdt, ty + hgt, mcolBkg);
		return;
	case C4M_TopFlat:
		top_rough = 0; side_rough = 2; bottom_rough = 4;
		break;
	case C4M_Smooth:
		top_rough = 2; side_rough = 2; bottom_rough = 2;
		break;
	case C4M_Rough:
		top_rough = 4; side_rough = 4; bottom_rough = 4;
		break;
	case C4M_Smoother:
		top_rough = 1; side_rough = 1; bottom_rough = 1;
		break;
	}
	int vtcs[16];
	unsigned int rx = std::max(wdt / 2, 1);

	vtcs[0] =  tx - ChunkyRandom(cro, rx * side_rough / 4);       vtcs[1] =  ty - ChunkyRandom(cro, rx * top_rough / 4);
	vtcs[2] =  tx - ChunkyRandom(cro, rx * side_rough / 2);       vtcs[3] =  ty + hgt / 2;
	vtcs[4] =  tx - ChunkyRandom(cro, rx * side_rough / 4);       vtcs[5] =  ty + hgt + ChunkyRandom(cro, rx * bottom_rough / 4);
	vtcs[6] =  tx + wdt / 2;                                      vtcs[7] =  ty + hgt + ChunkyRandom(cro, rx * bottom_rough / 2);
	vtcs[8] =  tx + wdt + ChunkyRandom(cro, rx * side_rough / 4); vtcs[9] =  ty + hgt + ChunkyRandom(cro, rx * bottom_rough / 4);
	vtcs[10] = tx + wdt + ChunkyRandom(cro, rx * side_rough / 2); vtcs[11] = ty + hgt / 2;
	vtcs[12] = tx + wdt + ChunkyRandom(cro, rx * side_rough / 4); vtcs[13] = ty - ChunkyRandom(cro, rx * top_rough / 4);
	vtcs[14] = tx + wdt / 2;                                      vtcs[15] = ty - ChunkyRandom(cro, rx * top_rough / 2);

	ForPolygon(vtcs, 8, NULL, NULL, mcol, mcolBkg);
}

void C4Landscape::DrawSmoothOChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, uint8_t mcol, uint8_t mcolBkg, int flip, uint32_t cro)
{
	int vtcs[8];
	unsigned int rx = std::max(wdt / 2, 1);

	vtcs[0] = tx;       vtcs[1] = ty;
	vtcs[2] = tx;       vtcs[3] = ty + hgt;
	vtcs[4] = tx + wdt; vtcs[5] = ty + hgt;
	vtcs[6] = tx + wdt; vtcs[7] = ty;

	switch (flip)
	{
	case 0: vtcs[0] = tx + wdt / 2; vtcs[1] += hgt / 3; vtcs[7] -= ChunkyRandom(cro, rx / 2); break;
	case 1: vtcs[2] = tx + wdt / 2; vtcs[3] -= hgt / 3; vtcs[5] += ChunkyRandom(cro, rx / 2); break;
	case 2: vtcs[4] = tx + wdt / 2; vtcs[5] -= hgt / 3; vtcs[3] += ChunkyRandom(cro, rx / 2); break;
	case 3: vtcs[6] = tx + wdt / 2; vtcs[7] += hgt / 3; vtcs[1] -= ChunkyRandom(cro, rx / 2); break;
	case 4: vtcs[0] = tx + wdt / 2; vtcs[1] += hgt / 2; break;
	case 5: vtcs[2] = tx + wdt / 2; vtcs[3] -= hgt / 2; break;
	case 6: vtcs[4] = tx + wdt / 2; vtcs[5] -= hgt / 2; break;
	case 7: vtcs[6] = tx + wdt / 2; vtcs[7] += hgt / 2; break;
	}

	ForPolygon(vtcs, 4, NULL, NULL, mcol, mcolBkg);
}

void C4Landscape::ChunkOZoom(CSurface8 * sfcMap, CSurface8 * sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint8_t iTexture, int32_t iOffX, int32_t iOffY)
{
	const C4TexMapEntry *entry = ::TextureMap.GetEntry(iTexture);
	C4Material *pMaterial = entry->GetMaterial();
	if (!pMaterial) return;
	const char *texture_name = entry->GetTextureName();
	C4Texture *texture = ::TextureMap.GetTexture(texture_name);
	C4TextureShape *shape = texture ? texture->GetMaterialShape() : NULL;
	// Chunk type by material
	C4MaterialCoreShape iChunkType = ::Game.C4S.Landscape.FlatChunkShapes ? C4M_Flat : pMaterial->MapChunkType;
	// Get map & landscape size
	int iMapWidth, iMapHeight;
	sfcMap->GetSurfaceSize(iMapWidth, iMapHeight);
	// Clip desired map segment to map size
	iMapX = Clamp<int32_t>(iMapX, 0, iMapWidth - 1);
	iMapY = Clamp<int32_t>(iMapY, 0, iMapHeight - 1);
	iMapWdt = Clamp<int32_t>(iMapWdt, 0, iMapWidth - iMapX);
	iMapHgt = Clamp<int32_t>(iMapHgt, 0, iMapHeight - iMapY);
	// get chunk size
	int iChunkWidth = MapZoom, iChunkHeight = MapZoom;
	// Scan map lines
	for (int iY = iMapY; iY < iMapY + iMapHgt; iY++)
	{
		// Landscape target coordinate vertical
		int iToY = iY * iChunkHeight + iOffY;
		// Scan map line
		for (int iX = iMapX; iX < iMapX + iMapWdt; iX++)
		{
			// Map scan line start
			uint8_t MapPixel=sfcMap->_GetPix(iX, iY);
			uint8_t MapPixelBkg=sfcMapBkg->_GetPix(iX, iY);
			// Landscape target coordinate horizontal
			int iToX = iX * iChunkWidth + iOffX;
			// Here's a chunk of the texture-material to zoom
			if (MapPixel == iTexture)
			{
				// Draw chunk
				DrawChunk(iToX, iToY, iChunkWidth, iChunkHeight, MapPixel, MapPixelBkg, iChunkType, (iX<<16)+iY);
			}
			// Other chunk, check for slope smoothers
			else if (iChunkType == C4M_Smooth || iChunkType == C4M_Smoother || iChunkType == C4M_Octagon)
			{
				// Map scan line pixel below
				uint8_t below = sfcMap->GetPix(iX, iY + 1);
				uint8_t above = sfcMap->GetPix(iX, iY - 1);
				uint8_t left  = sfcMap->GetPix(iX - 1, iY);
				uint8_t right = sfcMap->GetPix(iX + 1, iY);
				/*uint8_t belowBkg = sfcMapBkg->GetPix(iX, iY + 1);
				uint8_t aboveBkg = sfcMapBkg->GetPix(iX, iY - 1);*/
				uint8_t leftBkg  = sfcMapBkg->GetPix(iX - 1, iY);
				uint8_t rightBkg = sfcMapBkg->GetPix(iX + 1, iY);
				// do not fill a tiny hole
				if (below == iTexture && above == iTexture && left == iTexture && right == iTexture)
					continue;
				int flat = iChunkType == C4M_Octagon ? 4 : 0;
				// Smooth chunk & same texture-material below
				if (iY < iMapHeight - 1 && below == iTexture)
				{
					// Same texture-material on left
					if (iX > 0 && left == iTexture)
					{
						// Draw smoother
						DrawSmoothOChunk(iToX, iToY, iChunkWidth, iChunkHeight, left, leftBkg, 3 + flat, (iX<<16) + iY);
					}
					// Same texture-material on right
					if (iX < iMapWidth - 1 && right == iTexture)
					{
						// Draw smoother
						DrawSmoothOChunk(iToX, iToY, iChunkWidth, iChunkHeight, right, rightBkg, 0 + flat, (iX<<16)+iY);
					}
				}
				// Smooth chunk & same texture-material above
				if (iY > 0 && above == iTexture)
				{
					// Same texture-material on left
					if (iX > 0 && left == iTexture)
					{
						// Draw smoother
						DrawSmoothOChunk(iToX, iToY, iChunkWidth, iChunkHeight, left, leftBkg, 2 + flat, (iX<<16)+iY);
					}
					// Same texture-material on right
					if (iX < iMapWidth - 1 && right == iTexture)
					{
						// Draw smoother
						DrawSmoothOChunk(iToX, iToY, iChunkWidth, iChunkHeight, right, rightBkg, 1 + flat, (iX<<16)+iY);
					}
				}
			}
		}
	}
	// Draw custom shapes on top of regular materials
	if (shape && !::Game.C4S.Landscape.FlatChunkShapes) shape->Draw(sfcMap, sfcMapBkg, iMapX, iMapY, iMapWdt, iMapHgt, iTexture, iOffX, iOffY, MapZoom, pMaterial->MinShapeOverlap);
}

bool C4Landscape::GetTexUsage(CSurface8 * sfcMap, CSurface8 * sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, DWORD *dwpTextureUsage) const
{
	int iX,iY;
	// No good parameters
	if (!sfcMap || !sfcMapBkg || !dwpTextureUsage) return false;
	// Clip desired map segment to map size
	iMapX=Clamp<int32_t>(iMapX,0,sfcMap->Wdt-1); iMapY=Clamp<int32_t>(iMapY,0,sfcMap->Hgt-1);
	iMapWdt=Clamp<int32_t>(iMapWdt,0,sfcMap->Wdt-iMapX); iMapHgt=Clamp<int32_t>(iMapHgt,0,sfcMap->Hgt-iMapY);
	// Zero texture usage list
	for (int32_t cnt=0; cnt<C4M_MaxTexIndex; cnt++) dwpTextureUsage[cnt]=0;
	// Scan map pixels
	for (iY = iMapY; iY < iMapY+iMapHgt; iY++)
		for (iX = iMapX; iX < iMapX + iMapWdt; iX++)
		{
			// Count texture map index
			const int32_t tex = sfcMap->GetPix(iX, iY);
			assert(tex < C4M_MaxTexIndex);

			if (!dwpTextureUsage[tex]++) if (tex)
			{
				// Check if texture actually exists
				if (!::TextureMap.GetEntry(tex)->GetMaterial())
					LogF("Map2Landscape error: Texture index %d at (%d/%d) in map not defined in texture map!", (int) tex, (int) iX, (int) iY);
				// No error. Landscape is usually fine but might contain some holes where material should be
			}

			// Ignore background texture for now -- this is only used for ChunkOZoom,
			// for which only the foreground texture is relevant.

			/*
			// Count texture map index
			const int32_t texBkg = sfcMapBkg->GetPix(iX, iY);
			if (!dwpTextureUsage[texBkg]++) if (texBkg)
			{
				// Check if texture actually exists
				if (!::TextureMap.GetEntry(texBkg)->GetMaterial())
					LogF("Map2Landscape error: Texture index %d at (%d/%d) in background map not defined in texture map!", (int) texBkg, (int) iX, (int) iY);
				// No error. Landscape is usually fine but might contain some holes where material should be
			}
			*/
			
		}
	// Done
	return true;
}

bool C4Landscape::TexOZoom(CSurface8 * sfcMap, CSurface8 * sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, DWORD *dwpTextureUsage, int32_t iToX, int32_t iToY)
{
	int32_t iIndex;

	// ChunkOZoom all used textures
	for (auto i = ::TextureMap.Order.begin(); i != ::TextureMap.Order.end(); ++i)
	{
		iIndex = *i;
		if (dwpTextureUsage[iIndex] > 0)
		{
			// ChunkOZoom map to landscape
			ChunkOZoom(sfcMap, sfcMapBkg, iMapX, iMapY, iMapWdt, iMapHgt, iIndex, iToX, iToY);
		}
	}

	// Done
	return true;
}

bool C4Landscape::MapToSurface(CSurface8 * sfcMap, CSurface8 * sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY)
{

	// assign clipper
	Surface8->Clip(iToX,iToY,iToX+iToWdt-1,iToY+iToHgt-1);
	Surface8Bkg->Clip(iToX,iToY,iToX+iToWdt-1,iToY+iToHgt-1);
	pDraw->NoPrimaryClipper();

	// Enlarge map segment for chunky rim
	iMapX-=2+MaterialMap.max_shape_width/MapZoom;
	iMapY-=2+MaterialMap.max_shape_height/MapZoom;
	iMapWdt+=4+MaterialMap.max_shape_width/MapZoom*2;
	iMapHgt+=4+MaterialMap.max_shape_height/MapZoom*2;

	// Determine texture usage in map segment
	DWORD dwTexUsage[C4M_MaxTexIndex];
	if (!GetTexUsage(sfcMap,sfcMapBkg,iMapX,iMapY,iMapWdt,iMapHgt,dwTexUsage)) return false;
	// Texture zoom map to landscape
	if (!TexOZoom(sfcMap,sfcMapBkg,iMapX,iMapY,iMapWdt,iMapHgt,dwTexUsage,iOffX,iOffY)) return false;

	// remove clipper
	Surface8->NoClip();
	Surface8Bkg->NoClip();

	// success
	return true;
}

bool C4Landscape::MapToLandscape(CSurface8 * sfcMap, CSurface8 * sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iOffsX, int32_t iOffsY, bool noClear)
{
	assert(Surface8 && Surface8Bkg);
	// Clip to map/landscape segment
	int iMapWidth,iMapHeight,iLandscapeWidth,iLandscapeHeight;
	// Get map & landscape size
	sfcMap->GetSurfaceSize(iMapWidth,iMapHeight);
	Surface8->GetSurfaceSize(iLandscapeWidth,iLandscapeHeight);
	// Clip map segment to map size
	iMapX = Clamp<int32_t>(iMapX, 0, iMapWidth - 1); iMapY = Clamp<int32_t>(iMapY, 0, iMapHeight - 1);
	iMapWdt = Clamp<int32_t>(iMapWdt, 0, iMapWidth - iMapX); iMapHgt = Clamp<int32_t>(iMapHgt, 0, iMapHeight - iMapY);
	// No segment
	if (!iMapWdt || !iMapHgt) return true;

	// Get affected landscape rect
	C4Rect To;
	To.x = iMapX*MapZoom + iOffsX;
	To.y = iMapY*MapZoom + iOffsY;
	To.Wdt = iMapWdt*MapZoom;
	To.Hgt = iMapHgt*MapZoom;

	PrepareChange(To);

	// clear the old landscape if not supressed
	if(!noClear)
	{
		Surface8->ClearBox8Only(To.x, To.y, To.Wdt, To.Hgt);
		Surface8Bkg->ClearBox8Only(To.x, To.y, To.Wdt, To.Hgt);
	}

	MapToSurface(sfcMap, sfcMapBkg, iMapX, iMapY, iMapWdt, iMapHgt, To.x, To.y, To.Wdt, To.Hgt, iOffsX, iOffsY);
	FinishChange(To);
	return true;
}

bool C4Landscape::CreateMap(CSurface8*& sfcMap, CSurface8*& sfcMapBkg)
{
	int32_t iWidth=0,iHeight=0;

	// Create map surface
	Game.C4S.Landscape.GetMapSize(iWidth,iHeight,Game.StartupPlayerCount);
	if (!(sfcMap=new CSurface8(iWidth,iHeight)))
		return false;

	// Fill sfcMap
	C4MapCreator MapCreator;
	MapCreator.Create(sfcMap,
	                  Game.C4S.Landscape, ::TextureMap,
	                  true,Game.StartupPlayerCount);

	sfcMapBkg = CreateDefaultBkgSurface(*sfcMap, false);
	if (!sfcMapBkg) { delete sfcMap; sfcMap = NULL; return false; }
	return true;
}

bool C4Landscape::CreateMapS2(C4Group &ScenFile, CSurface8*& sfcMap, CSurface8*& sfcMapBkg)
{
	// file present?
	if (!ScenFile.AccessEntry(C4CFN_DynLandscape)) return false;

	// create map creator
	if (!pMapCreator)
		pMapCreator = new C4MapCreatorS2(&Game.C4S.Landscape, &::TextureMap, &::MaterialMap, Game.StartupPlayerCount);

	// read file
	pMapCreator->ReadFile(C4CFN_DynLandscape, &ScenFile);
	// render landscape
	if(!pMapCreator->Render(NULL, sfcMap, sfcMapBkg))
		return false;

	// keep map creator until script callbacks have been done
	return true;
}

bool C4Landscape::PostInitMap()
{
	// map creator present?
	if (!pMapCreator) return true;
	// call scripts
	pMapCreator->ExecuteCallbacks(MapZoom);
	// destroy map creator, if not needed later
	if (!Game.C4S.Landscape.KeepMapCreator) { delete pMapCreator; pMapCreator=NULL; }
	// done, success
	return true;
}
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++ Searching for features in the landscape +++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

bool C4Landscape::_PathFree(int32_t x, int32_t y, int32_t x2, int32_t y2) const
{
	x /= 17; y /= 15; x2 /= 17; y2 /= 15;
	while (x != x2 && y != y2)
	{
		if (PixCnt[x * PixCntPitch + y])
			return false;
		if (x > x2) x--; else x++;
		if (y > y2) y--; else y++;
	}
	if (x != x2)
		do
		{
			if (PixCnt[x * PixCntPitch + y])
				return false;
			if (x > x2) x--; else x++;
		}
		while (x != x2);
	else
		while (y != y2)
		{
			if (PixCnt[x * PixCntPitch + y])
				return false;
			if (y > y2) y--; else y++;
		}
	return !PixCnt[x * PixCntPitch + y];
}

int32_t C4Landscape::GetMatHeight(int32_t x, int32_t y, int32_t iYDir, int32_t iMat, int32_t iMax) const
{
	if (iYDir > 0)
	{
		iMax = std::min<int32_t>(iMax, Height - y);
		for (int32_t i = 0; i < iMax; i++)
			if (_GetMat(x, y + i) != iMat)
				return i;
	}
	else
	{
		iMax = std::min<int32_t>(iMax, y + 1);
		for (int32_t i = 0; i < iMax; i++)
			if (_GetMat(x, y - i) != iMat)
				return i;
	}
	return iMax;
}
// Nearest free above semi solid
bool AboveSemiSolid(int32_t &rx, int32_t &ry)
{
	int32_t cy1=ry,cy2=ry;
	bool UseUpwardsNextFree=false,UseDownwardsNextSolid=false;

	while ((cy1>=0) || (cy2<GBackHgt))
	{
		// Check upwards
		if (cy1>=0)
		{
			if (GBackSemiSolid(rx,cy1)) UseUpwardsNextFree=true;
			else if (UseUpwardsNextFree) { ry=cy1; return true; }
		}
		// Check downwards
		if (cy2<GBackHgt)
		{
			if (!GBackSemiSolid(rx,cy2)) UseDownwardsNextSolid=true;
			else if (UseDownwardsNextSolid) { ry=cy2; return true; }
		}
		// Advance
		cy1--; cy2++;
	}

	return false;
}

// Nearest free directly above solid
bool AboveSolid(int32_t &rx, int32_t &ry)
{
	int32_t cy1=ry,cy2=ry;

	while ((cy1>=0) || (cy2<GBackHgt))
	{
		// Check upwards
		if (cy1>=0)
			if (!GBackSemiSolid(rx,cy1))
				if (GBackSolid(rx,cy1+1))
					{ ry=cy1; return true; }
		// Check downwards
		if (cy2+1<GBackHgt)
			if (!GBackSemiSolid(rx,cy2))
				if (GBackSolid(rx,cy2+1))
					{ ry=cy2; return true; }
		// Advance
		cy1--; cy2++;
	}

	return false;
}

// Nearest free/semi above solid
bool SemiAboveSolid(int32_t &rx, int32_t &ry)
{
	int32_t cy1=ry,cy2=ry;

	while ((cy1>=0) || (cy2<GBackHgt))
	{
		// Check upwards
		if (cy1>=0)
			if (!GBackSolid(rx,cy1))
				if (GBackSolid(rx,cy1+1))
					{ ry=cy1; return true; }
		// Check downwards
		if (cy2+1<GBackHgt)
			if (!GBackSolid(rx,cy2))
				if (GBackSolid(rx,cy2+1))
					{ ry=cy2; return true; }
		// Advance
		cy1--; cy2++;
	}

	return false;
}

bool FindLiquidHeight(int32_t cx, int32_t &ry, int32_t hgt)
{
	int32_t cy1=ry,cy2=ry,rl1=0,rl2=0;

	while ((cy1>=0) || (cy2<GBackHgt))
	{
		// Check upwards
		if (cy1>=0)
		{
			if (GBackLiquid(cx,cy1))
				{ rl1++; if (rl1>=hgt) { ry=cy1+hgt/2; return true; } }
			else rl1=0;
		}
		// Check downwards
		if (cy2+1<GBackHgt)
		{
			if (GBackLiquid(cx,cy2))
				{ rl2++; if (rl2>=hgt) { ry=cy2-hgt/2; return true; } }
			else rl2=0;
		}
		// Advance
		cy1--; cy2++;
	}

	return false;
}

bool FindTunnelHeight(int32_t cx, int32_t &ry, int32_t hgt)
{
	int32_t cy1=ry,cy2=ry,rl1=0,rl2=0;

	while ((cy1>=0) || (cy2<GBackHgt))
	{
		// Check upwards
		if (cy1>=0)
		{
			if (Landscape.GetBackPix(cx, cy1) != 0 && MatDensity(GBackMat(cx,cy1)) < C4M_Liquid)
				{ rl1++; if (rl1>=hgt) { ry=cy1+hgt/2; return true; } }
			else rl1=0;
		}
		// Check downwards
		if (cy2+1<GBackHgt)
		{
			if (Landscape.GetBackPix(cx, cy2) != 0 && MatDensity(GBackMat(cx,cy2)) < C4M_Liquid)
				{ rl2++; if (rl2>=hgt) { ry=cy2-hgt/2; return true; } }
			else rl2=0;
		}
		// Advance
		cy1--; cy2++;
	}

	return false;
}

// Starting from rx/ry, searches for a width of solid ground.
// Returns bottom center of surface space found.
bool FindSolidGround(int32_t &rx, int32_t &ry, int32_t width)
{
	bool fFound=false;

	int32_t cx1,cx2,cy1,cy2,rl1=0,rl2=0;

	for (cx1=cx2=rx,cy1=cy2=ry; (cx1>0) || (cx2<GBackWdt); cx1--,cx2++)
	{
		// Left search
		if (cx1>=0) // Still going
		{
			if (AboveSolid(cx1,cy1)) rl1++; // Run okay
			else rl1=0; // No run
		}
		// Right search
		if (cx2<GBackWdt) // Still going
		{
			if (AboveSolid(cx2,cy2)) rl2++; // Run okay
			else rl2=0; // No run
		}
		// Check runs
		if (rl1>=width) { rx=cx1+rl1/2; ry=cy1; fFound=true; break; }
		if (rl2>=width) { rx=cx2-rl2/2; ry=cy2; fFound=true; break; }
	}

	if (fFound) AboveSemiSolid(rx,ry);

	return fFound;
}

bool FindSurfaceLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height)
{
	bool fFound=false;

	int32_t cx1,cx2,cy1,cy2,rl1=0,rl2=0,cnt;
	bool lokay;
	for (cx1=cx2=rx,cy1=cy2=ry; (cx1>0) || (cx2<GBackWdt); cx1--,cx2++)
	{
		// Left search
		if (cx1>0) // Still going
		{
			if (!AboveSemiSolid(cx1,cy1)) cx1=-1; // Abort left
			else
			{
				for (lokay=true,cnt=0; cnt<height; cnt++) if (!GBackLiquid(cx1,cy1+1+cnt)) lokay=false;
				if (lokay) rl1++; // Run okay
				else rl1=0; // No run
			}
		}
		// Right search
		if (cx2<GBackWdt) // Still going
		{
			if (!AboveSemiSolid(cx2,cy2)) cx2=GBackWdt; // Abort right
			else
			{
				for (lokay=true,cnt=0; cnt<height; cnt++) if (!GBackLiquid(cx2,cy2+1+cnt)) lokay=false;
				if (lokay) rl2++; // Run okay
				else rl2=0; // No run
			}
		}
		// Check runs
		if (rl1>=width) { rx=cx1+rl1/2; ry=cy1; fFound=true; break; }
		if (rl2>=width) { rx=cx2-rl2/2; ry=cy2; fFound=true; break; }
	}

	if (fFound) AboveSemiSolid(rx,ry);

	return fFound;
}

bool FindLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height)
{
	int32_t cx1,cx2,cy1,cy2,rl1=0,rl2=0;

	for (cx1=cx2=rx,cy1=cy2=ry; (cx1>0) || (cx2<GBackWdt); cx1--,cx2++)
	{
		// Left search
		if (cx1>0)
		{
			if (FindLiquidHeight(cx1,cy1,height)) rl1++;
			else rl1=0;
		}
		// Right search
		if (cx2<GBackWdt)
		{
			if (FindLiquidHeight(cx2,cy2,height)) rl2++;
			else rl2=0;
		}
		// Check runs
		if (rl1>=width) { rx=cx1+rl1/2; ry=cy1; return true; }
		if (rl2>=width) { rx=cx2-rl2/2; ry=cy2; return true; }
	}

	return false;
}

// Starting from rx/ry, searches for tunnel background
// Tunnel background == no sky && no semi/solid material (density < 25)
bool FindTunnel(int32_t &rx, int32_t &ry, int32_t width, int32_t height)
{
	int32_t cx1,cx2,cy1,cy2,rl1=0,rl2=0;

	for (cx1=cx2=rx,cy1=cy2=ry; (cx1>0) || (cx2<GBackWdt); cx1--,cx2++)
	{
		// Left search
		if (cx1>0)
		{
			if (FindTunnelHeight(cx1,cy1,height)) rl1++;
			else rl1=0;
		}
		// Right search
		if (cx2<GBackWdt)
		{
			if (FindTunnelHeight(cx2,cy2,height)) rl2++;
			else rl2=0;
		}
		// Check runs
		if (rl1>=width) { rx=cx1+rl1/2; ry=cy1; return true; }
		if (rl2>=width) { rx=cx2-rl2/2; ry=cy2; return true; }
	}

	return false;
}

// Starting from rx/ry, searches for a width of solid ground. Extreme distances
// may not exceed hrange. Returns bottom center of surface found.
bool FindLevelGround(int32_t &rx, int32_t &ry, int32_t width, int32_t hrange)
{
	bool fFound=false;

	int32_t cx1,cx2,cy1,cy2,rh1,rh2,rl1,rl2;

	cx1=cx2=rx; cy1=cy2=ry;
	rh1=cy1; rh2=cy2;
	rl1=rl2=0;

	for (cx1--,cx2++; (cx1>0) || (cx2<GBackWdt); cx1--,cx2++)
	{
		// Left search
		if (cx1>0) // Still going
		{
			if (!AboveSemiSolid(cx1,cy1)) cx1=-1; // Abort left
			else
			{
				if (GBackSolid(cx1,cy1+1) && (Abs(cy1-rh1)<hrange))
					rl1++; // Run okay
				else
					{ rl1=0; rh1=cy1; } // No run
			}
		}

		// Right search
		if (cx2<GBackWdt) // Still going
		{
			if (!AboveSemiSolid(cx2,cy2)) cx2=GBackWdt; // Abort right
			else
			{
				if (GBackSolid(cx2,cy2+1) && (Abs(cy2-rh2)<hrange))
					rl2++; // Run okay
				else
					{ rl2=0; rh2=cy2; } // No run
			}
		}

		// Check runs
		if (rl1>=width) { rx=cx1+rl1/2; ry=cy1; fFound=true; break; }
		if (rl2>=width) { rx=cx2-rl2/2; ry=cy2; fFound=true; break; }
	}

	if (fFound) AboveSemiSolid(rx,ry);

	return fFound;
}

// Starting from rx/ry, searches for a width of solid level ground with
// structure clearance (category). Returns bottom center of surface found.
bool FindConSiteSpot(int32_t &rx, int32_t &ry, int32_t wdt, int32_t hgt,
                     int32_t Plane, int32_t hrange)
{
	bool fFound=false;

	// No hrange limit, use standard smooth surface limit
	if (hrange==-1) hrange=std::max(wdt/4,5);

	int32_t cx1,cx2,cy1,cy2,rh1,rh2,rl1,rl2;

	// Left offset starting position
	cx1=std::min(rx+wdt/2,GBackWdt-1); cy1=ry;
	// No good: use centered starting position
	if (!AboveSemiSolid(cx1,cy1)) { cx1=std::min<int32_t>(rx,GBackWdt-1); cy1=ry; }
	// Right offset starting position
	cx2=std::max(rx-wdt/2,0); cy2=ry;
	// No good: use centered starting position
	if (!AboveSemiSolid(cx2,cy2)) { cx2=std::min<int32_t>(rx,GBackWdt-1); cy2=ry; }

	rh1=cy1; rh2=cy2; rl1=rl2=0;

	for (cx1--,cx2++; (cx1>0) || (cx2<GBackWdt); cx1--,cx2++)
	{
		// Left search
		if (cx1>0) // Still going
		{
			if (!AboveSemiSolid(cx1,cy1))
				cx1=-1; // Abort left
			else
			{
				if (GBackSolid(cx1,cy1+1) && (Abs(cy1-rh1)<hrange))
					rl1++; // Run okay
				else
					{ rl1=0; rh1=cy1; } // No run
			}
		}

		// Right search
		if (cx2<GBackWdt) // Still going
		{
			if (!AboveSemiSolid(cx2,cy2))
				cx2=GBackWdt; // Abort right
			else
			{
				if (GBackSolid(cx2,cy2+1) && (Abs(cy2-rh2)<hrange))
					rl2++; // Run okay
				else
					{ rl2=0; rh2=cy2; } // No run
			}
		}

		// Check runs & object overlap
		if (rl1>=wdt) if (cx1>0)
				if (!Game.OverlapObject(cx1,cy1-hgt-10,wdt,hgt+40,Plane))
					{ rx=cx1+wdt/2; ry=cy1; fFound=true; break; }
		if (rl2>=wdt) if (cx2<GBackWdt)
				if (!Game.OverlapObject(cx2-wdt,cy2-hgt-10,wdt,hgt+40,Plane))
					{ rx=cx2-wdt/2; ry=cy2; fFound=true; break; }
	}

	if (fFound) AboveSemiSolid(rx,ry);

	return fFound;
}

// Returns false on any solid pix in path.
bool PathFreePix(int32_t x, int32_t y)
{
	return !GBackSolid(x,y);
}

bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	return ForLine(x1,y1,x2,y2,&PathFreePix);
}

bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix, int32_t *iy)
{
	// use the standard Bresenham algorithm and just adjust it to behave correctly in the inversed case
	bool reverse = false;
	bool steep = Abs(y2 - y1) > Abs(x2 - x1);

	if (steep)
	{
		std::swap(x1, y1);
		std::swap(x2, y2);
	}

	if (x1 > x2)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
		reverse = true;
	}

	if (!reverse)
	{
		int32_t deltax = x2 - x1;
		int32_t deltay = Abs(y2 - y1);
		int32_t error = 0;
		int32_t ystep = (y1 < y2) ? 1 : -1;
		int32_t y = y1;

		for (int32_t x = x1; x <= x2; x++)
		{
			if (steep)
			{
				if (GBackSolid(y, x))
				{
					if (ix) {*ix = y; *iy = x;}
					return false;
				}
			}
			else
			{
				if (GBackSolid(x, y))
				{
					if (ix) {*ix = x; *iy = y;}
					return false;
				}
			}

			error += deltay;
			if (2 * error >= deltax)
			{
				y += ystep;
				error -= deltax;
			}
		}
	}
	else // reverse
	{
		int32_t deltax = x2 - x1;
		int32_t deltay = Abs(y2 - y1);
		int32_t error = 0;
		int32_t ystep = (y1 < y2) ? 1 : -1;
		int32_t y = y2;

		// normal (inverse) routine
		for (int32_t x = x2; x >= x1; x--)
		{
			if (steep)
			{
				if (GBackSolid(y, x))
				{
					if (ix) {*ix = y; *iy = x;}
					return false;
				}
			}
			else
			{
				if (GBackSolid(x, y))
					{
					if (ix) {*ix = x; *iy = y;}
					return false;
				}
			}

			error -= deltay;
			if (2 * error <= -deltax)
			{
				y -= ystep;
				error += deltax;
			}

		}
	}
	// no solid material encountered: path free!
	return true;
}

bool PathFreeIgnoreVehiclePix(int32_t x, int32_t y)
{
	BYTE byPix=GBackPix(x,y);
	return !byPix || !DensitySolid(::Landscape.GetPixMat(byPix)) || ::Landscape.GetPixMat(byPix) == MVehic;
}

bool PathFreeIgnoreVehicle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix, int32_t *iy)
{
	return ForLine(x1,y1,x2,y2,&PathFreeIgnoreVehiclePix,ix,iy);
}

int32_t TrajectoryDistance(int32_t iFx, int32_t iFy, C4Real iXDir, C4Real iYDir, int32_t iTx, int32_t iTy)
{
	int32_t iClosest = Distance(iFx,iFy,iTx,iTy);
	// Follow free trajectory, take closest point distance
	C4Real cx = itofix(iFx), cy = itofix(iFy);
	int32_t cdis;
	while (Inside(fixtoi(cx),0,GBackWdt-1) && Inside(fixtoi(cy),0,GBackHgt-1) && !GBackSolid(fixtoi(cx), fixtoi(cy)))
	{
		cdis = Distance(fixtoi(cx),fixtoi(cy),iTx,iTy);
		if (cdis<iClosest) iClosest=cdis;
		cx+=iXDir; cy+=iYDir; iYDir+=GravAccel;
	}
	return iClosest;
}

const int32_t C4LSC_Throwing_MaxVertical = 50,
              C4LSC_Throwing_MaxHorizontal = 60;

bool FindThrowingPosition(int32_t iTx, int32_t iTy, C4Real fXDir, C4Real fYDir, int32_t iHeight, int32_t &rX, int32_t &rY)
{

	// Start underneath throwing target
	rX=iTx; rY=iTy;                             // improve: check from overhanging cliff
	if (!SemiAboveSolid(rX,rY)) return false;

	// Target too far above surface
	if (!Inside(rY-iTy,-C4LSC_Throwing_MaxVertical,+C4LSC_Throwing_MaxVertical)) return false;

	// Search in direction according to launch fXDir
	int32_t iDir=+1; if (fXDir>0) iDir=-1;

	// Move along surface
	for (int32_t cnt=0; Inside<int32_t>(rX,0,GBackWdt-1) && (cnt<=C4LSC_Throwing_MaxHorizontal); rX+=iDir,cnt++)
	{
		// Adjust to surface
		if (!SemiAboveSolid(rX,rY)) return false;

		// Check trajectory distance
		int32_t itjd = TrajectoryDistance(rX,rY-iHeight,fXDir,fYDir,iTx,iTy);

		// Hitting range: success
		if (itjd<=2) return true;
	}

	// Failure
	return false;

}

const int32_t C4LSC_Closest_MaxRange = 200,
              C4LSC_Closest_Step     = 10;

bool FindClosestFree(int32_t &rX, int32_t &rY, int32_t iAngle1, int32_t iAngle2,
                     int32_t iExcludeAngle1, int32_t iExcludeAngle2)
{
	int32_t iX,iY;
	for (int32_t iR=C4LSC_Closest_Step; iR<C4LSC_Closest_MaxRange; iR+=C4LSC_Closest_Step)
		for (int32_t iAngle=iAngle1; iAngle<iAngle2; iAngle+=C4LSC_Closest_Step)
			if (!Inside(iAngle,iExcludeAngle1,iExcludeAngle2))
			{
				iX = rX + fixtoi(Sin(itofix(iAngle))*iR);
				iY = rY - fixtoi(Cos(itofix(iAngle))*iR);
				if (Inside<int32_t>(iX,0,GBackWdt-1))
					if (Inside<int32_t>(iY,0,GBackHgt-1))
						if (!GBackSemiSolid(iX,iY))
							{ rX=iX; rY=iY; return true; }
			}
	return false;
}

bool ConstructionCheck(C4PropList * PropList, int32_t iX, int32_t iY, C4Object *pByObj)
{
	C4Def *ndef;
	// Check def
	if (!(ndef=PropList->GetDef()))
	{
		if (pByObj) GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_UNDEF"), PropList->GetName()).getData(),pByObj);
		return false;
	}
	// Constructable?
	if (!ndef->Constructable)
	{
		if (pByObj) GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_NOCON"),ndef->GetName()).getData(),pByObj);
		return false;
	}
	// Check area
	int32_t rtx,rty,wdt,hgt;
	wdt=ndef->Shape.Wdt; hgt=ndef->Shape.Hgt-ndef->ConSizeOff;
	rtx=iX-wdt/2; rty=iY-hgt;
	if (::Landscape.AreaSolidCount(rtx,rty,wdt,hgt)>(wdt*hgt/20))
	{
		if (pByObj) GameMsgObjectError(LoadResStr("IDS_OBJ_NOROOM"),pByObj);
		return false;
	}
	if (::Landscape.AreaSolidCount(rtx,rty+hgt,wdt,5)<(wdt*2))
	{
		if (pByObj) GameMsgObjectError(LoadResStr("IDS_OBJ_NOLEVEL"),pByObj);
		return false;
	}
	// Check other structures
	C4Object *other;
	if ((other=Game.OverlapObject(rtx,rty,wdt,hgt,ndef->GetPlane())))
	{
		if (pByObj) GameMsgObjectError(FormatString(LoadResStr("IDS_OBJ_NOOTHER"),other->GetName ()).getData(),pByObj);
		return false;
	}
	return true;
}

// Finds the next pixel position moving to desired slide.
bool C4Landscape::FindMatPath(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide) const
{
	assert(mdens<=C4M_Solid); // mdens normalized in InsertMaterial

	int32_t cslide;
	bool fLeft=true,fRight=true;

	// One downwards
	if (GetDensity(fx,fy+ydir)<mdens) { fy+=ydir; return true; }

	// Find downwards slide path
	for (cslide=1; (cslide<=mslide) && (fLeft || fRight); cslide++)
	{
		// Check left
		if (fLeft)
		{
			if (GetDensity(fx-cslide,fy)>=mdens) // Left clogged
				fLeft=false;
			else if (GetDensity(fx-cslide,fy+ydir)<mdens) // Left slide okay
				{ fx--; return true; }
		}
		// Check right
		if (fRight)
		{
			if (GetDensity(fx+cslide,fy)>=mdens) // Right clogged
				fRight=false;
			else if (GetDensity(fx+cslide,fy+ydir)<mdens) // Right slide okay
				{ fx++; return true; }
		}
	}

	return false;
}

// Finds the closest immediate slide position.
bool C4Landscape::FindMatSlide(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide) const
{
	assert(mdens<=C4M_Solid); // mdens normalized in InsertMaterial and mrfInsertCheck
	int32_t cslide;
	bool fLeft=true,fRight=true;

	// One downwards
	if (GetDensity(fx,fy+ydir)<mdens) { fy+=ydir; return true; }

	// Find downwards slide path
	for (cslide=1; (cslide<=mslide) && (fLeft || fRight); cslide++)
	{
		// Check left
		if (fLeft)
		{
			if (GetDensity(fx-cslide,fy)>=mdens && GetDensity(fx-cslide,fy+ydir)>=mdens) // Left clogged
				fLeft=false;
			else if (GetDensity(fx-cslide,fy+ydir)<mdens) // Left slide okay
				{ fx-=cslide; fy+=ydir; return true; }
		}
		// Check right
		if (fRight)
		{
			if (GetDensity(fx+cslide,fy)>=mdens && GetDensity(fx+cslide,fy+ydir)>=mdens) // Right clogged
				fRight=false;
			else if (GetDensity(fx+cslide,fy+ydir)<mdens) // Right slide okay
				{ fx+=cslide; fy+=ydir; return true; }
		}
	}

	return false;
}

// Find closest point with density below mdens. Note this may return a point outside of the landscape,
// Assumption: There are no holes with smaller density inside of material with greater
//             density.
bool C4Landscape::FindMatPathPush(int32_t &fx, int32_t &fy, int32_t mdens, int32_t mslide, bool liquid) const
{
	// Startpoint must be inside landscape
	fx = Clamp<int32_t>(fx, 0, Width - 1);
	fy = Clamp<int32_t>(fy, 0, Height - 1);
	// Range to search, calculate bounds
	const int32_t iPushRange = 500;
	int32_t left = std::max<int32_t>(0, fx - iPushRange), right = std::min<int32_t>(Width - 1, fx + iPushRange),
	               top = std::max<int32_t>(0, fy - iPushRange), bottom = std::min<int32_t>(Height - 1, fy + iPushRange);
	// Direction constants
	const int8_t R = 0, D = 1, L = 2, U = 3;
	int8_t dir = 0;
	int32_t x = fx, y = fy;
	// Get startpoint density
	int32_t dens = GetDensity(fx, fy);
	// Smaller density? We're done.
	if (dens < mdens)
		return true;
	// Right density?
	else if (dens == mdens)
	{
		// Find start point for border search
		for (int32_t i = 0; ; i++)
			if (x - i - 1 < left || GetDensity(x - i - 1, y) != mdens)
				{ x -= i; dir = L; break; }
			else if (y - i - 1 < top || GetDensity(x, y - i - 1) != mdens)
				{ y -= i; dir = U; break; }
			else if (x + i + 1 > right || GetDensity(x + i + 1, y) != mdens)
				{ x += i; dir = R; break; }
			else if (y + i + 1 > bottom || GetDensity(x, y + i + 1) != mdens)
				{ y += i; dir = D; break; }
	}
	// Greater density
	else
	{
		// Try to find a way out
		int i = 1;
		for (; i < iPushRange; i++)
			if (GetDensity(x - i, y) <= mdens)
				{ x -= i; dir = R; break; }
			else if (GetDensity(x, y - i) <= mdens)
				{ y -= i; dir = D; break; }
			else if (GetDensity(x + i, y) <= mdens)
				{ x += i; dir = L; break; }
			else if (GetDensity(x, y + i) <= mdens)
				{ y += i; dir = U; break; }
		// Not found?
		if (i >= iPushRange) return false;
		// Done?
		if (GetDensity(x, y) < mdens)
		{
			fx = x; fy = y;
			return true;
		}
	}
	// Save startpoint of search
	int32_t sx = x, sy = y, sdir = dir;
	// Best point so far
	bool fGotBest = false; int32_t bx = 0, by = 0, bdist = 0;
	// Start searching
	do
	{
		// We should always be in a material of same density
		assert(x >= left && y >= top && x <= right && y <= bottom && GetDensity(x, y) == mdens);
		// Calc new position
		int nx = x, ny = y;
		switch (dir)
		{
		case R: nx++; break;
		case D: ny++; break;
		case L: nx--; break;
		case U: ny--; break;
		default: assert(false);
		}
		// In bounds?
		bool fInBounds = (nx >= left && ny >= top && nx <= right && ny <= bottom);
		// Get density. Not this performs an SideOpen-check if outside landscape bounds.
		int32_t dens = GetDensity(nx, ny);
		// Flow possible?
		if (dens < mdens)
		{
			// Calculate "distance".
			int32_t dist = Abs(nx - fx) + mslide * (liquid ? fy - ny : Abs(fy - ny));
			// New best point?
			if (!fGotBest || dist < bdist)
			{
				// Save it
				bx = nx; by = ny; bdist = dist; fGotBest = true;
				// Adjust borders: We can obviously safely ignore anything at greater distance
				top = std::max<int32_t>(top, fy - dist / mslide - 1);
				if (!liquid)
				{
					bottom = std::min<int32_t>(bottom, fy + dist / mslide + 1);
					left = std::max<int32_t>(left, fx - dist - 1);
					right = std::min<int32_t>(right, fx + dist + 1);
				}
				// Set new startpoint
				sx = x; sy = y; sdir = dir;
			}
		}
		// Step?
		if (fInBounds && dens == mdens)
		{
			// New point
			x = nx; y = ny;
			// Turn left
			(dir += 3) %= 4;
		}
		// Otherwise: Turn right
		else
		{
			++dir;
			dir %= 4;
		}
	}
	while (x != sx || y != sy || dir != sdir);
	// Nothing found?
	if (!fGotBest) return false;
	// Return it
	fx = bx; fy = by;
	return true;
}

int32_t C4Landscape::AreaSolidCount(int32_t x, int32_t y, int32_t wdt, int32_t hgt) const
{
	int32_t cx,cy,ascnt=0;
	for (cy=y; cy<y+hgt; cy++)
		for (cx=x; cx<x+wdt; cx++)
			if (GBackSolid(cx,cy))
				ascnt++;
	return ascnt;
}

void C4Landscape::FindMatTop(int32_t mat, int32_t &x, int32_t &y, bool distant_first) const
{
	int32_t mslide,cslide,tslide,distant_x=0;
	bool fLeft,fRight;

	if (!MatValid(mat)) return;
	mslide=::MaterialMap.Map[mat].MaxSlide;

	do
	{
		// Catch most common case: Walk upwards until material changes
		while (GetMat(x,y-1)==mat) --y;

		// Find upwards slide
		fLeft=true; fRight=true; tslide=0; distant_x=x;
		for (cslide=1; (cslide<=mslide) && (fLeft || fRight); cslide++)
		{
			// Left
			if (fLeft)
			{
				if (GetMat(x-cslide,y)!=mat) fLeft=false;
				else
				{
					distant_x = x-cslide;
					if (GetMat(distant_x,y-1)==mat) { tslide=-cslide; break; }
				}
			}
			// Right
			if (fRight)
			{
				if (GetMat(x+cslide,y)!=mat) fRight=false;
				else
				{
					distant_x = x+cslide;
					if (GetMat(distant_x,y-1)==mat) { tslide=+cslide; break; }
				}
			}
		}

		// Slide
		if (tslide) { x+=tslide; y--; }

	}
	while (tslide);

	// return top pixel max slide away from center if desired
	if (distant_first) x = distant_x;

}
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++++++++++++ Editor mode (draw landscape with brush)+++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

bool C4Landscape::SetMode(int32_t iMode)
{
	// Invalid mode
	if (!Inside<int32_t>(iMode,C4LSC_Dynamic,C4LSC_Exact)) return false;
	// Set mode
	Mode=iMode;
	// Done
	return true;
}

bool C4Landscape::GetMapColorIndex(const char *szMaterial, const char *szTexture, BYTE & rbyCol) const
{
	// Sky
	if (SEqual(szMaterial,C4TLS_MatSky))
		rbyCol=0;
	// Material-Texture
	else
	{
		if (!(rbyCol=::TextureMap.GetIndex(szMaterial,szTexture))) return false;
	}
	// Found
	return true;
}

bool C4Landscape::DrawBrush(int32_t iX, int32_t iY, int32_t iGrade, const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture)
{
	BYTE byCol, byColBkg;
	// Get map color index by material-texture
	if (!GetMapColorIndex(szMaterial,szTexture,byCol)) return false;
	if (!GetMapColorIndex(szBackMaterial,szBackTexture,byColBkg)) return false;
	// Get material shape size
	C4Texture *texture = ::TextureMap.GetTexture(szTexture);
	int32_t shape_wdt=0, shape_hgt=0;
	if (texture && texture->GetMaterialShape())
	{
		shape_wdt = texture->GetMaterialShape()->GetMaxPolyWidth() / MapZoom;
		shape_hgt = texture->GetMaterialShape()->GetMaxPolyHeight() / MapZoom;
	}
	// Draw
	switch (Mode)
	{
		// Dynamic: ignore
	case C4LSC_Dynamic:
		break;
		// Static: draw to map by material-texture-index, chunk-o-zoom to landscape
	case C4LSC_Static:
		// Draw to map
		int32_t iRadius; iRadius=std::max<int32_t>(2*iGrade/MapZoom,1);
		if (iRadius==1) { Map->SetPix(iX/MapZoom,iY/MapZoom,byCol); MapBkg->SetPix(iX/MapZoom, iY/MapZoom, byColBkg); }
		else { Map->Circle(iX/MapZoom,iY/MapZoom,iRadius,byCol); MapBkg->Circle(iX/MapZoom, iY/MapZoom, iRadius, byColBkg); }
		// Update landscape
		MapToLandscape(Map,MapBkg,iX/MapZoom-iRadius-1-shape_wdt,iY/MapZoom-iRadius-1-shape_hgt,2*iRadius+2+shape_wdt*2,2*iRadius+2+shape_hgt*2);
		SetMapChanged();
		break;
		// Exact: draw directly to landscape by color & pattern
	case C4LSC_Exact:
		C4Rect BoundingBox(iX-iGrade-1, iY-iGrade-1, iGrade*2+2, iGrade*2+2);
		// Draw to landscape
		PrepareChange(BoundingBox);
		Surface8->Circle(iX,iY,iGrade, byCol);
		Surface8Bkg->Circle(iX,iY,iGrade, byColBkg);
		FinishChange(BoundingBox);
		break;
	}
	return true;
}

bool C4Landscape::DrawLineLandscape(int32_t iX, int32_t iY, int32_t iGrade, uint8_t line_color, uint8_t line_color_bkg)
{
	Surface8->Circle(iX, iY, iGrade, line_color);
	Surface8Bkg->Circle(iX, iY, iGrade, line_color_bkg);
	return true;
}

bool C4Landscape::DrawLineMap(int32_t iX, int32_t iY, int32_t iRadius, uint8_t line_color, uint8_t line_color_bkg)
{
	if (!Map) return false;
	if (iRadius == 1)
		{ Map->SetPix(iX, iY, line_color); MapBkg->SetPix(iX, iY, line_color_bkg); }
	else
		{ Map->Circle(iX, iY, iRadius, line_color); MapBkg->Circle(iX, iY, iRadius, line_color_bkg); }
	return true;
}

bool C4Landscape::DrawLine(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture)
{
	// Get map color index by material-texture
	uint8_t line_color, line_color_bkg;
	if (!GetMapColorIndex(szMaterial,szTexture,line_color)) return false;
	if (!GetMapColorIndex(szBackMaterial,szBackTexture,line_color_bkg)) return false;
	// Get material shape size
	C4Texture *texture = ::TextureMap.GetTexture(szTexture);
	int32_t shape_wdt = 0, shape_hgt = 0;
	if (texture && texture->GetMaterialShape())
	{
		shape_wdt = texture->GetMaterialShape()->GetMaxPolyWidth() / MapZoom;
		shape_hgt = texture->GetMaterialShape()->GetMaxPolyHeight() / MapZoom;
	}
	// Draw
	switch (Mode)
	{
		// Dynamic: ignore
	case C4LSC_Dynamic:
		break;
		// Static: draw to map by material-texture-index, chunk-o-zoom to landscape
	case C4LSC_Static:
		// Draw to map
		int32_t iRadius; iRadius=std::max<int32_t>(2*iGrade/MapZoom,1);
		iX1/=MapZoom; iY1/=MapZoom; iX2/=MapZoom; iY2/=MapZoom;
		ForLine(iX1, iY1, iX2, iY2, [this, line_color, line_color_bkg, iRadius](int32_t x, int32_t y) { return DrawLineMap(x, y, iRadius, line_color, line_color_bkg); });
		// Update landscape
		int32_t iUpX,iUpY,iUpWdt,iUpHgt;
		iUpX=std::min(iX1,iX2)-iRadius-1; iUpY=std::min(iY1,iY2)-iRadius-1;
		iUpWdt=Abs(iX2-iX1)+2*iRadius+2; iUpHgt=Abs(iY2-iY1)+2*iRadius+2;
		MapToLandscape(Map,MapBkg,iUpX-shape_wdt,iUpY-shape_hgt,iUpWdt+shape_wdt*2,iUpHgt+shape_hgt*2);
		SetMapChanged();
		break;
		// Exact: draw directly to landscape by color & pattern
	case C4LSC_Exact:
		// Set texture pattern & get material color
		C4Rect BoundingBox(iX1 - iGrade, iY1 - iGrade, iGrade*2+1, iGrade*2+1);
		BoundingBox.Add(C4Rect(iX2 - iGrade, iY2 - iGrade, iGrade*2+1, iGrade*2+1));
		// Draw to landscape
		PrepareChange(BoundingBox);
		ForLine(iX1, iY1, iX2, iY2, [this, line_color, line_color_bkg, iGrade](int32_t x, int32_t y) { return DrawLineLandscape(x, y, iGrade, line_color, line_color_bkg); });
		FinishChange(BoundingBox);
		break;
	}
	return true;
}

bool C4Landscape::DrawBox(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture)
{
	// get upper-left/lower-right - corners
	int32_t iX0=std::min(iX1, iX2); int32_t iY0=std::min(iY1, iY2);
	iX2=std::max(iX1, iX2); iY2=std::max(iY1, iY2); iX1=iX0; iY1=iY0;
	BYTE byCol, byColBkg;
	// Get map color index by material-texture
	if (!GetMapColorIndex(szMaterial,szTexture,byCol)) return false;
	if (!GetMapColorIndex(szBackMaterial,szBackTexture,byColBkg)) return false;
	// Get material shape size
	C4Texture *texture = ::TextureMap.GetTexture(szTexture);
	int32_t shape_wdt = 0, shape_hgt = 0;
	if (texture && texture->GetMaterialShape())
	{
		shape_wdt = texture->GetMaterialShape()->GetMaxPolyWidth() / MapZoom;
		shape_hgt = texture->GetMaterialShape()->GetMaxPolyHeight() / MapZoom;
	}
	// Draw
	switch (Mode)
	{
		// Dynamic: ignore
	case C4LSC_Dynamic:
		break;
		// Static: draw to map by material-texture-index, chunk-o-zoom to landscape
	case C4LSC_Static:
		// Draw to map
		iX1/=MapZoom; iY1/=MapZoom; iX2/=MapZoom; iY2/=MapZoom;
		Map->Box(iX1,iY1,iX2,iY2,byCol);
		MapBkg->Box(iX1, iY1, iX2, iY2, byColBkg);
		// Update landscape
		MapToLandscape(Map,MapBkg,iX1-1-shape_wdt,iY1-1-shape_hgt,iX2-iX1+3+shape_wdt*2,iY2-iY1+3+shape_hgt*2);
		SetMapChanged();
		break;
		// Exact: draw directly to landscape by color & pattern
	case C4LSC_Exact:
		C4Rect BoundingBox(iX1, iY1, iX2 - iX1+1, iY2 - iY1+1);
		// Draw to landscape
		PrepareChange(BoundingBox);
		Surface8->Box(iX1,iY1,iX2,iY2,byCol);
		Surface8Bkg->Box(iX1,iY1,iX2,iY2,byColBkg);
		FinishChange(BoundingBox);
		break;
	}
	return true;
}

bool C4Landscape::DrawChunks(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t icntx, int32_t icnty, const char *szMaterial, const char *szTexture, bool bIFT)
{
	BYTE byColor;
	if (!GetMapColorIndex(szMaterial, szTexture, byColor)) return false;

	int32_t iMaterial = ::MaterialMap.Get(szMaterial); if (!MatValid(iMaterial)) return false;
	C4MaterialCoreShape shape = ::Game.C4S.Landscape.FlatChunkShapes ? C4M_Flat : ::MaterialMap.Map[iMaterial].MapChunkType;

	C4Rect BoundingBox(tx - 5, ty - 5, wdt + 10, hgt + 10);
	PrepareChange(BoundingBox);

	// assign clipper
	Surface8->Clip(BoundingBox.x,BoundingBox.y,BoundingBox.x+BoundingBox.Wdt,BoundingBox.y+BoundingBox.Hgt);
	Surface8Bkg->Clip(BoundingBox.x,BoundingBox.y,BoundingBox.x+BoundingBox.Wdt,BoundingBox.y+BoundingBox.Hgt);
	pDraw->NoPrimaryClipper();

	// draw all chunks
	int32_t x, y;
	for (x = 0; x < icntx; x++)
		for (y = 0; y < icnty; y++)
			DrawChunk(tx+wdt*x/icntx,ty+hgt*y/icnty,wdt/icntx,hgt/icnty,byColor,bIFT ? DefaultBkgMat(byColor) : 0, shape,Random(1000));

	// remove clipper
	Surface8->NoClip();
	Surface8Bkg->NoClip();

	FinishChange(BoundingBox);

	// success
	return true;
}

bool C4Landscape::DrawPolygon(int *vtcs, int length, const char *szMaterial, const char* szBackMaterial, bool fDrawBridge)
{
	if(length < 6) return false;
	if(length % 2 == 1) return false;
	// get texture
	int32_t iMatTex = ::TextureMap.GetIndexMatTex(szMaterial);
	if (!iMatTex) return false;
	uint8_t mcol = MatTex2PixCol(iMatTex);
	// get background texture
	uint8_t mcolBkg = 0;
	if (szBackMaterial != NULL)
	{
		const int32_t iBackMatTex = ::TextureMap.GetIndexMatTex(szBackMaterial);
		if (!iBackMatTex) return false;
		mcolBkg = MatTex2PixCol(iBackMatTex);
	}
	// do bridging?
	uint8_t *conversion_map = NULL;
	if (fDrawBridge)
	{
		conversion_map = GetBridgeMatConversion(MatTex2PixCol(iMatTex));
		mcolBkg = Transparent;
	}
	// prepare pixel count update
	C4Rect BoundingBox = getBoundingBox(vtcs,length);
	// draw polygon
	PrepareChange(BoundingBox);
	ForPolygon(vtcs,length/2,NULL,NULL, mcol, mcolBkg, conversion_map);
	FinishChange(BoundingBox);
	return true;
}

uint8_t *C4Landscape::GetBridgeMatConversion(int32_t for_material_col) const
{
	// safety
	int32_t for_material = GetPixMat(for_material_col);
	if (for_material < 0 || for_material >= MaterialMap.Num) return NULL;
	// query map. create if not done yet
	uint8_t *conv_map = BridgeMatConversion[for_material_col];
	if (!conv_map)
	{
		conv_map = new uint8_t[C4M_MaxTexIndex];
		for (int32_t i=0; i < C4M_MaxTexIndex; ++i)
		{
			if ( (MatDensity(for_material)>=GetPixDensity(i)))
			{
				// bridge pixel OK here. change pixel.
				conv_map[i] = for_material_col;
			}
			else
			{
				// bridge pixel not OK - keep current pixel
				conv_map[i] = i;
			}
		}
		BridgeMatConversion[for_material_col] = conv_map;
	}
	return conv_map;
}

bool C4Landscape::DrawQuad(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iX3, int32_t iY3, int32_t iX4, int32_t iY4, const char *szMaterial, const char *szBackMaterial, bool fDrawBridge)
{
	// set vertices
	int32_t vtcs[8];
	vtcs[0] = iX1; vtcs[1] = iY1;
	vtcs[2] = iX2; vtcs[3] = iY2;
	vtcs[4] = iX3; vtcs[5] = iY3;
	vtcs[6] = iX4; vtcs[7] = iY4;
	return DrawPolygon(vtcs, 8, szMaterial, szBackMaterial, fDrawBridge);
}

BYTE C4Landscape::GetMapIndex(int32_t iX, int32_t iY) const
{
	if (!Map) return 0;
	return Map->GetPix(iX,iY);
}

BYTE C4Landscape::GetBackMapIndex(int32_t iX, int32_t iY) const
{
	if (!MapBkg) return 0;
	return MapBkg->GetPix(iX,iY);
}

void C4Landscape::PrepareChange(C4Rect BoundingBox)
{
	// move solidmasks out of the way
	C4Rect SolidMaskRect = BoundingBox;
	if (pLandscapeRender)
		SolidMaskRect = pLandscapeRender->GetAffectedRect(pLandscapeRender->GetAffectedRect(SolidMaskRect));
	for (C4SolidMask * pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
	{
		pSolid->RemoveTemporary(SolidMaskRect);
	}
	UpdateMatCnt(BoundingBox, false);
}

void C4Landscape::FinishChange(C4Rect BoundingBox)
{
	// Intersect bounding box with landscape
	BoundingBox.Intersect(C4Rect(0, 0, Width, Height));
	if (!BoundingBox.Wdt || !BoundingBox.Hgt) return;
	// update render
	if(pLandscapeRender)
		pLandscapeRender->Update(BoundingBox, this);
	UpdateMatCnt(BoundingBox, true);
	// Restore Solidmasks
	C4Rect SolidMaskRect = BoundingBox;
	if (pLandscapeRender)
		SolidMaskRect = pLandscapeRender->GetAffectedRect(pLandscapeRender->GetAffectedRect(SolidMaskRect));
	for (C4SolidMask * pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
	{
		pSolid->Repair(SolidMaskRect);
	}
	C4SolidMask::CheckConsistency();
	UpdatePixCnt(BoundingBox);
	// update FoW
	if (pFoW)
	{
		pFoW->Invalidate(BoundingBox);
		pFoW->Ambient.UpdateFromLandscape(*this, BoundingBox);
	}
}


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++++++++++++++++ Functions for Script interface +++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

bool C4Landscape::DrawMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef, bool ignoreSky)
{
	// safety
	if (!szMapDef) return false;
	// clip to landscape size
	if (!ClipRect(iX, iY, iWdt, iHgt)) return false;
	// get needed map size
	int32_t iMapWdt=(iWdt-1)/MapZoom+1;
	int32_t iMapHgt=(iHgt-1)/MapZoom+1;
	C4SLandscape FakeLS=Game.C4S.Landscape;
	FakeLS.MapWdt.Set(iMapWdt, 0, iMapWdt, iMapWdt);
	FakeLS.MapHgt.Set(iMapHgt, 0, iMapHgt, iMapHgt);
	// create map creator
	C4MapCreatorS2 MapCreator(&FakeLS, &::TextureMap, &::MaterialMap, Game.StartupPlayerCount);
	// read file
	MapCreator.ReadScript(szMapDef);
	// render map
	CSurface8* sfcMap = NULL;
	CSurface8* sfcMapBkg = NULL;
	if (!MapCreator.Render(NULL, sfcMap, sfcMapBkg))
		return false;
	// map it to the landscape
	bool fSuccess=MapToLandscape(sfcMap, sfcMapBkg, 0, 0, iMapWdt, iMapHgt, iX, iY, ignoreSky);
	// cleanup
	delete sfcMap;
	delete sfcMapBkg;
	// return whether successful
	return fSuccess;
}

bool C4Landscape::DrawDefMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef, bool ignoreSky)
{
	// safety
	if (!szMapDef || !pMapCreator) return false;
	// clip to landscape size
	if (!ClipRect(iX, iY, iWdt, iHgt)) return false;
	// get needed map size
	int32_t iMapWdt=(iWdt-1)/MapZoom+1;
	int32_t iMapHgt=(iHgt-1)/MapZoom+1;
	bool fSuccess=false;
	// render map
	C4MCMap *pMap=pMapCreator->GetMap(szMapDef);
	if (!pMap) return false;
	pMap->SetSize(iMapWdt, iMapHgt);
	CSurface8* sfcMap = NULL;
	CSurface8* sfcMapBkg = NULL;
	if(pMapCreator->Render(szMapDef, sfcMap, sfcMapBkg))
	{
		// map to landscape
		fSuccess = MapToLandscape(sfcMap, sfcMapBkg, 0, 0, iMapWdt, iMapHgt, iX, iY, ignoreSky);
		// cleanup
		delete sfcMap;
		delete sfcMapBkg;
	}
	// done
	return fSuccess;
}

bool C4Landscape::ClipRect(int32_t &rX, int32_t &rY, int32_t &rWdt, int32_t &rHgt) const
{
	// clip by bounds
	if (rX<0) { rWdt+=rX; rX=0; }
	if (rY<0) { rHgt+=rY; rY=0; }
	int32_t iOver;
	iOver=rX+rWdt-Width; if (iOver>0) { rWdt-=iOver; }
	iOver=rY+rHgt-Height; if (iOver>0) { rHgt-=iOver; }
	// anything left inside the bounds?
	return rWdt>0 && rHgt>0;
}

bool C4Landscape::ReplaceMapColor(BYTE iOldIndex, BYTE iNewIndex)
{
	// find every occurance of iOldIndex in map; replace it by new index
	if (!Map) return false;
	int iPitch, iMapWdt, iMapHgt;
	BYTE *pMap = Map->Bits;
	iMapWdt = Map->Wdt;
	iMapHgt = Map->Hgt;
	iPitch = Map->Pitch;
	if (!pMap) return false;
	for (int32_t y=0; y<iMapHgt; ++y)
	{
		for (int32_t x=0; x<iMapWdt; ++x)
		{
			if (*pMap == iOldIndex)
				*pMap = iNewIndex;
			++pMap;
		}
		pMap += iPitch - iMapWdt;
	}
	return true;
}

bool C4Landscape::SetTextureIndex(const char *szMatTex, BYTE iNewIndex, bool fInsert)
{
	if (((!szMatTex || !*szMatTex) && !fInsert) || !Inside<int>(iNewIndex, 1, C4M_MaxTexIndex - 1))
	{
		DebugLogF("Cannot insert new texture %s to index %d: Invalid parameters.", (const char *) szMatTex, (int) iNewIndex);
		return false;
	}
	// get last mat index - returns zero for not found (valid for insertion mode)
	StdStrBuf Material, Texture;
	Material.CopyUntil(szMatTex, '-'); Texture.Copy(SSearch(szMatTex, "-"));
	BYTE iOldIndex = (szMatTex && *szMatTex) ? ::TextureMap.GetIndex(Material.getData(), Texture.getData(), false) : 0;
	// insertion mode?
	if (fInsert)
	{
		// there must be room to move up to
		BYTE byLastMoveIndex = C4M_MaxTexIndex - 1;
		while (::TextureMap.GetEntry(byLastMoveIndex))
			if (--byLastMoveIndex == iNewIndex)
			{
				DebugLogF("Cannot insert new texture %s to index %d: No room for insertion.", (const char *) szMatTex, (int) iNewIndex);
				return false;
			}
		// then move up all other textures first
		// could do this in one loop, but it's just a developement call anyway, so move one index at a time
		while (--byLastMoveIndex >= iNewIndex)
			if (::TextureMap.GetEntry(byLastMoveIndex))
			{
				ReplaceMapColor(byLastMoveIndex, byLastMoveIndex+1);
				::TextureMap.MoveIndex(byLastMoveIndex, byLastMoveIndex+1);
			}
		// new insertion desired?
		if (szMatTex && *szMatTex)
		{
			// move from old or create new
			if (iOldIndex)
			{
				ReplaceMapColor(iOldIndex, iNewIndex);
				::TextureMap.MoveIndex(iOldIndex, iNewIndex);
			}
			else
			{
				StdStrBuf Material, Texture;
				Material.CopyUntil(szMatTex, '-'); Texture.Copy(SSearch(szMatTex, "-"));
				// new insertion
				if (!::TextureMap.AddEntry(iNewIndex, Material.getData(), Texture.getData()))
				{
					LogF("Cannot insert new texture %s to index %d: Texture map entry error", (const char *) szMatTex, (int) iNewIndex);
					return false;
				}
			}
		}
		// done, success
		return true;
	}
	else
	{
		// new index must not be occupied
		const C4TexMapEntry *pOld;
		if ((pOld = ::TextureMap.GetEntry(iNewIndex)) && !pOld->isNull())
		{
			DebugLogF("Cannot move texture %s to index %d: Index occupied by %s-%s.", (const char *) szMatTex, (int) iNewIndex, pOld->GetMaterialName(), pOld->GetTextureName());
			return false;
		}
		// must only move existing textures
		if (!iOldIndex)
		{
			DebugLogF("Cannot move texture %s to index %d: Texture not found.", (const char *) szMatTex, (int) iNewIndex);
			return false;
		}
		// update map
		ReplaceMapColor(iOldIndex, iNewIndex);
		// change to new index in texmap
		::TextureMap.MoveIndex(iOldIndex, iNewIndex);
		// done, success
		return true;
	}
}

void C4Landscape::RemoveUnusedTexMapEntries()
{
	// check usage in landscape
	bool fTexUsage[C4M_MaxTexIndex];
	int32_t iMatTex;
	for (iMatTex = 0; iMatTex < C4M_MaxTexIndex; ++iMatTex)
		fTexUsage[iMatTex] = false;
	for (int32_t y=0; y<Height; ++y)
		for (int32_t x=0; x<Width; ++x)
		{
			const BYTE pix = Surface8->GetPix(x, y);
			const BYTE backPix = Surface8Bkg->GetPix(x, y);
			assert(pix < C4M_MaxTexIndex);
			assert(backPix < C4M_MaxTexIndex);

			fTexUsage[pix] = true;
			fTexUsage[backPix] = true;
		}

	// check usage by materials
	for (int32_t iMat = 0; iMat < ::MaterialMap.Num; ++iMat)
	{
		C4Material *pMat = ::MaterialMap.Map + iMat;
		if (pMat->BlastShiftTo >= 0) fTexUsage[pMat->BlastShiftTo] = true;
		if (pMat->BelowTempConvertTo >= 0) fTexUsage[pMat->BelowTempConvertTo] = true;
		if (pMat->AboveTempConvertTo >= 0) fTexUsage[pMat->AboveTempConvertTo] = true;
		if (pMat->DefaultMatTex >= 0) fTexUsage[pMat->DefaultMatTex] = true;
	}
	// remove unused
	for (iMatTex = 1; iMatTex < C4M_MaxTexIndex; ++iMatTex)
		if (!fTexUsage[iMatTex])
			::TextureMap.RemoveEntry(iMatTex);
	// flag rewrite
	::TextureMap.fEntriesAdded = true;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++ Update functions ++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

void C4Landscape::HandleTexMapUpdate()
{
	// Pixel maps must be update
	UpdatePixMaps();
	// Update landscape palette
	Mat2Pal();
}

void C4Landscape::UpdatePixMaps()
{
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Mat[i] = PixCol2Mat(i);
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Dens[i] = MatDensity(Pix2Mat[i]);
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Place[i] = MatValid(Pix2Mat[i]) ? ::MaterialMap.Map[Pix2Mat[i]].Placement : 0;
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Light[i] = MatValid(Pix2Mat[i]) && (::MaterialMap.Map[Pix2Mat[i]].Light>0);
	Pix2Place[0] = 0;
	// clear bridge mat conversion buffers
	for (int32_t i = 0; i < C4M_MaxTexIndex; ++i)
	{
		delete [] BridgeMatConversion[i];
		BridgeMatConversion[i] = NULL;
	}
}

bool C4Landscape::Mat2Pal()
{
	if (!Surface8 || !Surface8Bkg) return false;
	// set landscape pal
	int32_t tex;
	for (tex=0; tex<C4M_MaxTexIndex; tex++)
	{
		const C4TexMapEntry *pTex = ::TextureMap.GetEntry(tex);
		if (!pTex || pTex->isNull())
			continue;
		// colors
		DWORD dwPix = pTex->GetPattern().PatternClr(0, 0);
		Surface8->pPal->Colors[MatTex2PixCol(tex)] = dwPix;
		Surface8Bkg->pPal->Colors[MatTex2PixCol(tex)] = dwPix;
	}
	// success
	return true;
}


void C4Landscape::UpdatePixCnt(const C4Rect &Rect, bool fCheck)
{
	int32_t PixCntWidth = (Width + 16) / 17;
	for (int32_t y = std::max<int32_t>(0, Rect.y / 15); y < std::min<int32_t>(PixCntPitch, (Rect.y + Rect.Hgt + 14) / 15); y++)
		for (int32_t x = std::max<int32_t>(0, Rect.x / 17); x < std::min<int32_t>(PixCntWidth, (Rect.x + Rect.Wdt + 16) / 17); x++)
		{
			int iCnt = 0;
			for (int32_t x2 = x * 17; x2 < std::min<int32_t>(x * 17 + 17, Width); x2++)
				for (int32_t y2 = y * 15; y2 < std::min<int32_t>(y * 15 + 15, Height); y2++)
					if (_GetDensity(x2, y2))
						iCnt++;
			if (fCheck)
				assert(iCnt == PixCnt[x * PixCntPitch + y]);
			PixCnt[x * PixCntPitch + y] = iCnt;
		}
}

void C4Landscape::UpdateMatCnt(C4Rect Rect, bool fPlus)
{
	Rect.Intersect(C4Rect(0, 0, Width, Height));
	if (!Rect.Hgt || !Rect.Wdt) return;
	// Multiplicator for changes
	const int32_t iMul = fPlus ? +1 : -1;
	// Count pixels
	for (int32_t x = 0; x < Rect.Wdt; x++)
	{
		int iHgt = 0;
		int32_t y;
		for (y = 1; y < Rect.Hgt; y++)
		{
			int32_t iMat = _GetMat(Rect.x+x, Rect.y+y - 1);
			// Same material? Count it.
			if (iMat == _GetMat(Rect.x+x, Rect.y+y))
				iHgt++;
			else
			{
				if (iMat >= 0)
				{
					// Normal material counting
					MatCount[iMat] += iMul * (iHgt + 1);
					// Effective material counting enabled?
					if (int32_t iMinHgt = ::MaterialMap.Map[iMat].MinHeightCount)
					{
						// First chunk? Add any material above when checking chunk height
						int iAddedHeight = 0;
						if (Rect.y && iHgt + 1 == y)
							iAddedHeight = GetMatHeight(Rect.x+x, Rect.y-1, -1, iMat, iMinHgt);
						// Check the chunk height
						if (iHgt + 1 + iAddedHeight >= iMinHgt)
						{
							EffectiveMatCount[iMat] += iMul * (iHgt + 1);
							if (iAddedHeight < iMinHgt)
								EffectiveMatCount[iMat] += iMul * iAddedHeight;
						}
					}
				}
				// Next chunk of material
				iHgt = 0;
			}
		}
		// Check last pixel
		int32_t iMat = _GetMat(Rect.x+x, Rect.y+Rect.Hgt-1);
		if (iMat >= 0)
		{
			// Normal material counting
			MatCount[iMat] += iMul * (iHgt + 1);
			// Minimum height counting?
			if (int32_t iMinHgt = ::MaterialMap.Map[iMat].MinHeightCount)
			{
				int iAddedHeight1 = 0, iAddedHeight2 = 0;
				// Add any material above for chunk size check
				if (Rect.y && iHgt + 1 == Rect.Hgt)
					iAddedHeight1 = GetMatHeight(Rect.x+x, Rect.y-1, -1, iMat, iMinHgt);
				// Add any material below for chunk size check
				if (Rect.y+y < Height)
					iAddedHeight2 = GetMatHeight(Rect.x+x, Rect.y+Rect.Hgt, 1, iMat, iMinHgt);
				// Chunk tall enough?
				if (iHgt + 1 + iAddedHeight1 + iAddedHeight2 >= ::MaterialMap.Map[iMat].MinHeightCount)
				{
					EffectiveMatCount[iMat] += iMul * (iHgt + 1);
					if (iAddedHeight1 < iMinHgt)
						EffectiveMatCount[iMat] += iMul * iAddedHeight1;
					if (iAddedHeight2 < iMinHgt)
						EffectiveMatCount[iMat] += iMul * iAddedHeight2;
				}
			}
		}
	}
}



C4Landscape Landscape;
