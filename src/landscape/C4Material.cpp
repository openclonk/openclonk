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

/* Material definitions used by the landscape */

#include "C4Include.h"
#include "landscape/C4Material.h"

#include "c4group/C4Components.h"
#include "c4group/C4Group.h"
#include "editor/C4ToolsDlg.h" // For C4TLS_MatSky...
#include "game/C4Physics.h" // For GravAccel
#include "landscape/C4PXS.h"
#include "landscape/C4Texture.h"
#include "landscape/C4Landscape.h"
#include "lib/C4Random.h"
#include "platform/C4SoundSystem.h"
#include "script/C4Aul.h"
#include "script/C4Effect.h"


int32_t MVehic=MNone,MHalfVehic=MNone,MTunnel=MNone,MWater=MNone,MEarth=MNone;
BYTE MCVehic=0;
BYTE MCHalfVehic=0;
// -------------------------------------- C4MaterialReaction


struct ReactionFuncMapEntry { const char *szRFName; C4MaterialReactionFunc pFunc; };

const ReactionFuncMapEntry ReactionFuncMap[] =
{
	{ "Script",  &C4MaterialMap::mrfScript },
	{ "Convert", &C4MaterialMap::mrfConvert},
	{ "Poof",    &C4MaterialMap::mrfPoof },
	{ "Corrode", &C4MaterialMap::mrfCorrode },
	{ "Insert",  &C4MaterialMap::mrfInsert },
	{ nullptr, &C4MaterialReaction::NoReaction }
};


void C4MaterialReaction::CompileFunc(StdCompiler *pComp)
{
	if (pComp->isDeserializer()) pScriptFunc = nullptr;
	// compile reaction func ptr
	StdStrBuf sReactionFuncName;
	int32_t i=0; while (ReactionFuncMap[i].szRFName && (ReactionFuncMap[i].pFunc != pFunc)) ++i;
	sReactionFuncName = ReactionFuncMap[i].szRFName;
	pComp->Value(mkNamingAdapt(mkParAdapt(sReactionFuncName, StdCompiler::RCT_IdtfAllowEmpty),   "Type",                     StdCopyStrBuf() ));
	i=0; while (ReactionFuncMap[i].szRFName && !SEqual(ReactionFuncMap[i].szRFName, sReactionFuncName.getData())) ++i;
	pFunc = ReactionFuncMap[i].pFunc;
	// compile the rest
	pComp->Value(mkNamingAdapt(mkParAdapt(TargetSpec, StdCompiler::RCT_All),          "TargetSpec",               StdCopyStrBuf() ));
	pComp->Value(mkNamingAdapt(mkParAdapt(ScriptFunc, StdCompiler::RCT_IdtfAllowEmpty),          "ScriptFunc",               StdCopyStrBuf() ));
	pComp->Value(mkNamingAdapt(iExecMask,           "ExecMask",                 ~0u             ));
	pComp->Value(mkNamingAdapt(fReverse,            "Reverse",                  false           ));
	pComp->Value(mkNamingAdapt(fInverseSpec,        "InverseSpec",              false           ));
	pComp->Value(mkNamingAdapt(fInsertionCheck,     "CheckSlide",               true            ));
	pComp->Value(mkNamingAdapt(iDepth,              "Depth",                    0               ));
	pComp->Value(mkNamingAdapt(mkParAdapt(sConvertMat, StdCompiler::RCT_IdtfAllowEmpty),         "ConvertMat",               StdCopyStrBuf() ));
	pComp->Value(mkNamingAdapt(iCorrosionRate,      "CorrosionRate",            100             ));
}


void C4MaterialReaction::ResolveScriptFuncs(const char *szMatName)
{
	// get script func for script-defined behaviour
	if (pFunc == &C4MaterialMap::mrfScript)
	{
		pScriptFunc = ::ScriptEngine.GetPropList()->GetFunc(this->ScriptFunc.getData());
		if (!pScriptFunc)
			DebugLogF(R"(Error getting function "%s" for Material reaction of "%s")", this->ScriptFunc.getData(), szMatName);
	}
	else
		pScriptFunc = nullptr;
}

// -------------------------------------- C4MaterialCore

C4MaterialCore::C4MaterialCore()
{
	Clear();
}

void C4MaterialCore::Clear()
{
	CustomReactionList.clear();
	sTextureOverlay.Clear();
	sPXSGfx.Clear();
	sBlastShiftTo.Clear();
	sInMatConvert.Clear();
	sInMatConvertTo.Clear();
	sBelowTempConvertTo.Clear();
	sAboveTempConvertTo.Clear();
	*Name='\0';
	MapChunkType = C4M_Flat;
	Density = 0;
	Friction = 0;
	DigFree = 0;
	BlastFree = 0;
	Dig2Object = C4ID::None;
	Dig2ObjectRatio = 0;
	Dig2ObjectCollect = 0;
	Blast2Object = C4ID::None;
	Blast2ObjectRatio = 0;
	Blast2PXSRatio = 0;
	Instable = 0;
	MaxAirSpeed = 0;
	MaxSlide = 0;
	WindDrift = 0;
	Inflammable = 0;
	Incendiary = 0;
	Extinguisher = 0;
	Corrosive = 0;
	Corrode = 0;
	Soil = 0;
	Placement = 0;
	Light = 0;
	OverlayType = 0;
	PXSGfxRt.Default();
	PXSGfxSize = 0;
	InMatConvertDepth = 0;
	BelowTempConvert = 0;
	BelowTempConvertDir = 0;
	AboveTempConvert = 0;
	AboveTempConvertDir = 0;
	TempConvStrength = 0;
	MinHeightCount = 0;
	SplashRate=10;
	KeepSinglePixels=false;
	AnimationSpeed = 20;
	LightAngle = 255;
	for (int i = 0; i < 3; i++) {
		LightEmit[i] = 0;
		LightSpot[i] = 16;
	}
	MinShapeOverlap = 25;
}

void C4MaterialCore::Default()
{
	Clear();
}

bool C4MaterialCore::Load(C4Group &hGroup,
                          const char *szEntryName)
{
	StdStrBuf Source;
	if (!hGroup.LoadEntryString(szEntryName,&Source))
		return false;
	StdStrBuf Name = hGroup.GetFullName() + DirSep + szEntryName;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, Source, Name.getData()))
		return false;
	// adjust placement, if not specified
	if (!Placement)
	{
		if (DensitySolid(Density))
		{
			Placement=30;
			if (!DigFree) Placement+=20;
			if (!BlastFree) Placement+=10;
		}
		else if (DensityLiquid(Density))
			Placement=10;
		else Placement=5;
	}
	return true;
}

void C4MaterialCore::CompileFunc(StdCompiler *pComp)
{
	assert(pComp->hasNaming());
	if (pComp->isDeserializer()) Clear();
	pComp->Name("Material");
	pComp->Value(mkNamingAdapt(toC4CStr(Name),      "Name",                ""));

	const StdEnumEntry<C4MaterialCoreShape> Shapes[] =
	{
		{ "Flat",     C4M_Flat },
		{ "TopFlat",  C4M_TopFlat },
		{ "Smooth",   C4M_Smooth },
		{ "Rough",    C4M_Rough },
		{ "Octagon",  C4M_Octagon },
		{ "Smoother", C4M_Smoother },
		{ nullptr, C4M_Flat }
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(MapChunkType, Shapes),
	                                                "Shape",               C4M_Flat));
	pComp->Value(mkNamingAdapt(Density,             "Density",             0));
	pComp->Value(mkNamingAdapt(Friction,            "Friction",            0));
	pComp->Value(mkNamingAdapt(DigFree,             "DigFree",             0));
	pComp->Value(mkNamingAdapt(BlastFree,           "BlastFree",           0));
	pComp->Value(mkNamingAdapt(Blast2Object,        "Blast2Object",        C4ID::None));
	pComp->Value(mkNamingAdapt(Dig2Object,          "Dig2Object",          C4ID::None));
	pComp->Value(mkNamingAdapt(Dig2ObjectRatio,     "Dig2ObjectRatio",     0));
	pComp->Value(mkNamingAdapt(Dig2ObjectCollect,   "Dig2ObjectCollect",   0));
	pComp->Value(mkNamingAdapt(Blast2ObjectRatio,   "Blast2ObjectRatio",   0));
	pComp->Value(mkNamingAdapt(Blast2PXSRatio,      "Blast2PXSRatio",      0));
	pComp->Value(mkNamingAdapt(Instable,            "Instable",            0));
	pComp->Value(mkNamingAdapt(MaxAirSpeed,         "MaxAirSpeed",         0));
	pComp->Value(mkNamingAdapt(MaxSlide,            "MaxSlide",            0));
	pComp->Value(mkNamingAdapt(WindDrift,           "WindDrift",           0));
	pComp->Value(mkNamingAdapt(Inflammable,         "Inflammable",         0));
	if (pComp->isDeserializer())
	{
		// The value used to have a wrong spelling ("Incindiary"). If there's
		// no "Incendiary" value, use the wrong spelling instead
		try
		{
			pComp->Value(mkNamingAdapt(Incendiary, "Incendiary"));
		}
		catch (StdCompiler::NotFoundException *ex)
		{
			delete ex;
			pComp->Value(mkNamingAdapt(Incendiary, "Incindiary", 0));
		}
	}
	else
	{
		// When serializing, write both spellings because some script might be
		// calling GetMaterialVal with the wrong one
		pComp->Value(mkNamingAdapt(Incendiary, "Incendiary"));
		pComp->Value(mkNamingAdapt(Incendiary, "Incindiary"));
	}
	pComp->Value(mkNamingAdapt(Corrode,             "Corrode",             0));
	pComp->Value(mkNamingAdapt(Corrosive,           "Corrosive",           0));
	pComp->Value(mkNamingAdapt(Extinguisher,        "Extinguisher",        0));
	pComp->Value(mkNamingAdapt(Soil,                "Soil",                0));
	pComp->Value(mkNamingAdapt(Placement,           "Placement",           0));
	pComp->Value(mkNamingAdapt(Light,               "Light",               0));
	pComp->Value(mkNamingAdapt(mkParAdapt(sTextureOverlay, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "TextureOverlay",      ""));
	pComp->Value(mkNamingAdapt(OverlayType,         "OverlayType",         0));
	pComp->Value(mkNamingAdapt(mkParAdapt(sPXSGfx, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "PXSGfx",              ""));
	pComp->Value(mkNamingAdapt(PXSGfxRt,            "PXSGfxRt",            TargetRect0));
	pComp->Value(mkNamingAdapt(PXSGfxSize,          "PXSGfxSize",          PXSGfxRt.Wdt));
	pComp->Value(mkNamingAdapt(TempConvStrength,    "TempConvStrength",    0));
	pComp->Value(mkNamingAdapt(mkParAdapt(sBlastShiftTo, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "BlastShiftTo",        ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(sInMatConvert, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "InMatConvert",        ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(sInMatConvertTo, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "InMatConvertTo",      ""));
	pComp->Value(mkNamingAdapt(InMatConvertDepth,   "InMatConvertDepth",   0));
	pComp->Value(mkNamingAdapt(AboveTempConvert,    "AboveTempConvert",    0));
	pComp->Value(mkNamingAdapt(AboveTempConvertDir, "AboveTempConvertDir", 0));
	pComp->Value(mkNamingAdapt(mkParAdapt(sAboveTempConvertTo, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "AboveTempConvertTo",  ""));
	pComp->Value(mkNamingAdapt(BelowTempConvert,    "BelowTempConvert",    0));
	pComp->Value(mkNamingAdapt(BelowTempConvertDir, "BelowTempConvertDir", 0));
	pComp->Value(mkNamingAdapt(mkParAdapt(sBelowTempConvertTo, StdCompiler::RCT_IdtfAllowEmpty),
	                                                "BelowTempConvertTo",  ""));
	pComp->Value(mkNamingAdapt(MinHeightCount,      "MinHeightCount",      0));
	pComp->Value(mkNamingAdapt(SplashRate,          "SplashRate",          10));
	pComp->Value(mkNamingAdapt(KeepSinglePixels,    "KeepSinglePixels",    false));
	pComp->Value(mkNamingAdapt(AnimationSpeed,      "AnimationSpeed",      100));
	pComp->Value(mkNamingAdapt(LightAngle,          "LightAngle",          255));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(LightEmit, 0), "LightEmit"));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(LightSpot, 16),"LightSpot"));
	pComp->Value(mkNamingAdapt(MinShapeOverlap,     "MinShapeOverlap",     25));
	pComp->NameEnd();
	// material reactions
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(CustomReactionList),
	                                                "Reaction",            std::vector<C4MaterialReaction>()));
}


// -------------------------------------- C4Material

C4Material::C4Material()
{
	BlastShiftTo=0;
	InMatConvertTo=MNone;
	BelowTempConvertTo=0;
	AboveTempConvertTo=0;
}

void C4Material::UpdateScriptPointers()
{
	for (auto & i : CustomReactionList)
		i.ResolveScriptFuncs(Name);
}


// -------------------------------------- C4MaterialMap


C4MaterialMap::C4MaterialMap() : DefReactConvert(&mrfConvert), DefReactPoof(&mrfPoof), DefReactCorrode(&mrfCorrode), DefReactIncinerate(&mrfIncinerate), DefReactInsert(&mrfInsert)
{
	Default();
}


C4MaterialMap::~C4MaterialMap()
{
	Clear();
}

void C4MaterialMap::Clear()
{
	if (Map) delete [] Map; Map=nullptr; Num=0;
	delete [] ppReactionMap; ppReactionMap = nullptr;
}

int32_t C4MaterialMap::Load(C4Group &hGroup)
{
	char entryname[256+1];

	// Determine number of materials in files
	int32_t mat_num=hGroup.EntryCount(C4CFN_MaterialFiles);

	// Allocate new map
	C4Material *pNewMap = new C4Material [mat_num + Num];
	if (!pNewMap) return 0;

	// Load material cores to map
	hGroup.ResetSearch(); int32_t cnt=0;
	while (hGroup.FindNextEntry(C4CFN_MaterialFiles,entryname))
	{
		// Load mat
		if (!pNewMap[cnt].Load(hGroup,entryname))
			{ delete [] pNewMap; return 0; }
		// A new material?
		if (Get(pNewMap[cnt].Name) == MNone)
			cnt++;
	}

	// Take over old materials.
	for (int32_t i = 0; i < Num; i++)
	{
		pNewMap[cnt+i] = Map[i];
	}
	delete [] Map;
	Map = pNewMap;

	// set material number
	Num+=cnt;

	return cnt;
}

bool C4MaterialMap::HasMaterials(C4Group &hGroup) const
{
	return !!hGroup.EntryCount(C4CFN_MaterialFiles);
}

int32_t C4MaterialMap::Get(const char *szMaterial)
{
	int32_t cnt;
	for (cnt=0; cnt<Num; cnt++)
		if (SEqualNoCase(szMaterial,Map[cnt].Name))
			return cnt;
	return MNone;
}


bool C4MaterialMap::CrossMapMaterials(const char* szEarthMaterial) // Called after load
{
	// Check material number
	if (::MaterialMap.Num>C4MaxMaterial)
		{ LogFatal(LoadResStr("IDS_PRC_TOOMANYMATS")); return false; }
	// build reaction function map
	delete [] ppReactionMap;
	typedef C4MaterialReaction * C4MaterialReactionPtr;
	ppReactionMap = new C4MaterialReactionPtr[(Num+1)*(Num+1)];
	for (int32_t iMatPXS=-1; iMatPXS<Num; iMatPXS++)
	{
		C4Material *pMatPXS = (iMatPXS+1) ? Map+iMatPXS : nullptr;
		for (int32_t iMatLS=-1; iMatLS<Num; iMatLS++)
		{
			C4MaterialReaction *pReaction = nullptr;
			C4Material *pMatLS  = ( iMatLS+1) ? Map+ iMatLS : nullptr;
			// natural stuff: material conversion here?
			if (pMatPXS && pMatPXS->sInMatConvert.getLength() && SEqualNoCase(pMatPXS->sInMatConvert.getData(), pMatLS ? pMatLS->Name : C4TLS_MatSky))
				pReaction = &DefReactConvert;
			// non-sky reactions
			else if (pMatPXS && pMatLS)
			{
				// incindiary vs extinguisher
				if ((pMatPXS->Incendiary && pMatLS->Extinguisher) || (pMatPXS->Extinguisher && pMatLS->Incendiary))
					pReaction = &DefReactPoof;
				// incindiary vs inflammable
				else if ((pMatPXS->Incendiary && pMatLS->Inflammable) || (pMatPXS->Inflammable && pMatLS->Incendiary))
					pReaction = &DefReactIncinerate;
				// corrosive vs corrode
				else if (pMatPXS->Corrosive && pMatLS->Corrode)
					pReaction = &DefReactCorrode;
				// liquid hitting liquid or solid: Material insertion
				else if (DensityLiquid(MatDensity(iMatPXS)) && DensitySemiSolid(MatDensity(iMatLS)))
					pReaction = &DefReactInsert;
				// solid hitting solid: Material insertion
				else if (DensitySolid(MatDensity(iMatPXS)) && DensitySolid(MatDensity(iMatLS)))
					pReaction = &DefReactInsert;
			}
			// assign the function; or nullptr for no reaction
			SetMatReaction(iMatPXS, iMatLS, pReaction);
		}
	}
	// reset max shape size
	max_shape_width=max_shape_height=0;
	// material-specific initialization
	int32_t cnt;
	for (cnt=0; cnt<Num; cnt++)
	{
		C4Material *pMat = Map+cnt;
		const char *szTextureOverlay = nullptr;
		// newgfx: init pattern
		if (Map[cnt].sTextureOverlay.getLength())
			if (::TextureMap.GetTexture(Map[cnt].sTextureOverlay.getLength()))
			{
				szTextureOverlay = Map[cnt].sTextureOverlay.getData();
				// backwards compatibility: if a pattern was specified although the no-pattern flag was set, overwrite that flag
				if (Map[cnt].OverlayType & C4MatOv_None)
				{
					DebugLogF("Error in overlay of material %s: Flag C4MatOv_None ignored because a custom overlay (%s) was specified!", Map[cnt].Name, szTextureOverlay);
					Map[cnt].OverlayType &= ~C4MatOv_None;
				}
			}
		// default to first texture in texture map
		int iTexMapIx;
		if (!szTextureOverlay && (iTexMapIx = ::TextureMap.GetIndex(Map[cnt].Name, nullptr, false)))
			szTextureOverlay = TextureMap.GetEntry(iTexMapIx)->GetTextureName();
		// default to smooth
		if (!szTextureOverlay)
			szTextureOverlay = "none";
		// search/create entry in texmap
		Map[cnt].DefaultMatTex = ::TextureMap.GetIndex(Map[cnt].Name, szTextureOverlay, true,
		                         FormatString("DefaultMatTex of mat %s", Map[cnt].Name).getData());
		// init PXS facet
		C4Surface * sfcTexture;
		C4Texture * Texture;
		if (Map[cnt].sPXSGfx.getLength())
			if ((Texture=::TextureMap.GetTexture(Map[cnt].sPXSGfx.getData())))
				if ((sfcTexture=Texture->Surface32))
					Map[cnt].PXSFace.Set(sfcTexture, Map[cnt].PXSGfxRt.x, Map[cnt].PXSGfxRt.y, Map[cnt].PXSGfxRt.Wdt, Map[cnt].PXSGfxRt.Hgt);
		// evaluate reactions for that material
		for (auto & iRCnt : pMat->CustomReactionList)
		{
			C4MaterialReaction *pReact = &iRCnt;
			if (pReact->sConvertMat.getLength()) pReact->iConvertMat = Get(pReact->sConvertMat.getData()); else pReact->iConvertMat = -1;
			// evaluate target spec
			int32_t tmat;
			if (MatValid(tmat=Get(pReact->TargetSpec.getData())))
			{
				// single material target
				if (pReact->fInverseSpec)
					for (int32_t cnt2=-1; cnt2<Num; cnt2++) {
						if (cnt2!=tmat)
							SetMatReaction(cnt, cnt2, pReact);
						else
							SetMatReaction(cnt, tmat, pReact);
					}
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "All"))
			{
				// add to all materials, including sky
				if (!pReact->fInverseSpec) for (int32_t cnt2=-1; cnt2<Num; cnt2++) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Solid"))
			{
				// add to all solid materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (DensitySolid(Map[cnt2].Density) != pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "SemiSolid"))
			{
				// add to all semisolid materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (DensitySemiSolid(Map[cnt2].Density) != pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Background"))
			{
				// add to all BG materials, including sky
				if (!pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (!Map[cnt2].Density != pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Sky"))
			{
				// add to sky
				if (!pReact->fInverseSpec)
					SetMatReaction(cnt, -1, pReact);
				else
					for (int32_t cnt2=0; cnt2<Num; cnt2++) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Incendiary") || SEqualNoCase(pReact->TargetSpec.getData(), "Incindiary"))
			{
				// add to all incendiary materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (!Map[cnt2].Incendiary == pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Extinguisher"))
			{
				// add to all incendiary materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (!Map[cnt2].Extinguisher == pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Inflammable"))
			{
				// add to all incendiary materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (!Map[cnt2].Inflammable == pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Corrosive"))
			{
				// add to all incendiary materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (!Map[cnt2].Corrosive == pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
			else if (SEqualNoCase(pReact->TargetSpec.getData(), "Corrode"))
			{
				// add to all incendiary materials
				if (pReact->fInverseSpec) SetMatReaction(cnt, -1, pReact);
				for (int32_t cnt2=0; cnt2<Num; cnt2++) if (!Map[cnt2].Corrode == pReact->fInverseSpec) SetMatReaction(cnt, cnt2, pReact);
			}
		}
	}
	// second loop (DefaultMatTex is needed by GetIndexMatTex)
	for (cnt=0; cnt<Num; cnt++)
	{
		if (Map[cnt].sBlastShiftTo.getLength())
			Map[cnt].BlastShiftTo=::TextureMap.GetIndexMatTex(Map[cnt].sBlastShiftTo.getData(), nullptr, true, FormatString("BlastShiftTo of mat %s", Map[cnt].Name).getData());
		if (Map[cnt].sInMatConvertTo.getLength())
			Map[cnt].InMatConvertTo=Get(Map[cnt].sInMatConvertTo.getData());
		if (Map[cnt].sBelowTempConvertTo.getLength())
			Map[cnt].BelowTempConvertTo=::TextureMap.GetIndexMatTex(Map[cnt].sBelowTempConvertTo.getData(), nullptr, true, FormatString("BelowTempConvertTo of mat %s", Map[cnt].Name).getData());
		if (Map[cnt].sAboveTempConvertTo.getLength())
			Map[cnt].AboveTempConvertTo=::TextureMap.GetIndexMatTex(Map[cnt].sAboveTempConvertTo.getData(), nullptr, true, FormatString("AboveTempConvertTo of mat %s", Map[cnt].Name).getData());
	}

	// Get hardcoded system material indices
	const C4TexMapEntry* earth_entry = ::TextureMap.GetEntry(::TextureMap.GetIndexMatTex(szEarthMaterial));
	if(!earth_entry)
		{ LogFatal(FormatString(R"(Earth material "%s" not found!)", szEarthMaterial).getData()); return false; }

	MVehic     = Get("Vehicle");     MCVehic     = Mat2PixColDefault(MVehic);
	MHalfVehic = Get("HalfVehicle"); MCHalfVehic = Mat2PixColDefault(MHalfVehic);
	MTunnel    = Get("Tunnel");
	MWater     = Get("Water");
	MEarth     = Get(earth_entry->GetMaterialName());

	if ((MVehic==MNone) || (MTunnel==MNone))
		{ LogFatal(LoadResStr("IDS_PRC_NOSYSMATS")); return false; }

	return true;
}


void C4MaterialMap::SetMatReaction(int32_t iPXSMat, int32_t iLSMat, C4MaterialReaction *pReact)
{
	// evaluate reaction swap
	if (pReact && pReact->fReverse) std::swap(iPXSMat, iLSMat);
	// set it
	ppReactionMap[(iLSMat+1)*(Num+1) + iPXSMat+1] = pReact;
}

bool C4MaterialMap::SaveEnumeration(C4Group &hGroup)
{
	char *mapbuf = new char [1000];
	mapbuf[0]=0;
	SAppend("[Enumeration]",mapbuf); SAppend(LineFeed,mapbuf);
	for (int32_t cnt=0; cnt<Num; cnt++)
	{
		SAppend(Map[cnt].Name,mapbuf);
		SAppend(LineFeed,mapbuf);
	}
	return hGroup.Add(C4CFN_MatMap,mapbuf,SLen(mapbuf),false,true);
}

bool C4MaterialMap::LoadEnumeration(C4Group &hGroup)
{
	// Load enumeration map (from savegame), succeed if not present
	StdStrBuf mapbuf;
	if (!hGroup.LoadEntryString(C4CFN_MatMap, &mapbuf)) return true;

	// Sort material array by enumeration map, fail if some missing
	const char *csearch;
	char cmatname[C4M_MaxName+1];
	int32_t cmat=0;
	if (!(csearch = SSearch(mapbuf.getData(),"[Enumeration]"))) { return false; }
	csearch=SAdvanceSpace(csearch);
	while (IsIdentifier(*csearch))
	{
		SCopyIdentifier(csearch,cmatname,C4M_MaxName);
		if (!SortEnumeration(cmat,cmatname))
		{
			// Output error message!
			return false;
		}
		cmat++;
		csearch+=SLen(cmatname);
		csearch=SAdvanceSpace(csearch);
	}

	return true;
}

bool C4MaterialMap::SortEnumeration(int32_t iMat, const char *szMatName)
{

	// Not enough materials loaded
	if (iMat>=Num) return false;

	// Find requested mat
	int32_t cmat;
	for (cmat=iMat; cmat<Num; cmat++)
		if (SEqual(szMatName,Map[cmat].Name))
			break;
	// Not found
	if (cmat>=Num) return false;

	// already the same?
	if (cmat == iMat) return true;

	// Move requested mat to indexed position
	C4Material mswap;
	mswap = Map[iMat];
	Map[iMat] = Map[cmat];
	Map[cmat] = mswap;

	return true;
}

void C4MaterialMap::Default()
{
	Num=0;
	Map=nullptr;
	ppReactionMap=nullptr;
	max_shape_width=max_shape_height=0;
}

C4MaterialReaction *C4MaterialMap::GetReaction(int32_t iPXSMat, int32_t iLandscapeMat)
{
	// safety
	if (!ppReactionMap) return nullptr;
	if (!Inside<int32_t>(iPXSMat, -1, Num-1)) return nullptr;
	if (!Inside<int32_t>(iLandscapeMat, -1, Num-1)) return nullptr;
	// values OK; get func!
	return GetReactionUnsafe(iPXSMat, iLandscapeMat);
}

static void Smoke(int32_t tx, int32_t ty, int32_t level)
{
	// Use scripted function (global func Smoke) to create smoke
	// Caution: This makes engine internal smoking a synced call.
	C4AulParSet pars(tx, ty, level);
	::ScriptEngine.GetPropList()->Call(P_Smoke, &pars);
}

bool mrfInsertCheck(int32_t &iX, int32_t &iY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, bool *pfPosChanged, bool no_slide = false)
{
	// always manipulating pos/speed here
	if (pfPosChanged) *pfPosChanged = true;

	// Move up by up to 3px to account for moving SolidMasks, other material insertions, etc.
	int32_t mdens = std::min(::MaterialMap.Map[iPxsMat].Density, C4M_Solid);
	int32_t max_upwards = 3;
	bool was_pushed_upwards = false;
	while (max_upwards-- && (::Landscape.GetDensity(iX, iY) >= mdens))
	{
		--iY;
		was_pushed_upwards = true;
	}

	// Rough contact? May splash
	if (fYDir > itofix(1))
		if (::MaterialMap.Map[iPxsMat].SplashRate && !Random(::MaterialMap.Map[iPxsMat].SplashRate))
		{
			fYDir = -fYDir/8;
			fXDir = fXDir/8 + C4REAL100(Random(200) - 100);
			if (fYDir) return false;
		}

	// Contact: Stop
	fYDir = -GravAccel;

	// Incendiary mats smoke on contact even before doing their slide
	if (::MaterialMap.Map[iPxsMat].Incendiary)
		if (!Random(25))
		{
			Smoke(iX, iY, 4 + Random(3));
		}

	// Move by mat path/slide
	int32_t iSlideX = iX, iSlideY = iY;
	
	if (!no_slide && ::Landscape.FindMatSlide(iSlideX,iSlideY,Sign(GravAccel),mdens,::MaterialMap.Map[iPxsMat].MaxSlide))
	{
		// Sliding on equal material: Move directly to optimize insertion of rain onto lakes
		// Also move directly when shifted upwards to ensure movement on permamently moving SolidMask
		if (iPxsMat == iLsMat || was_pushed_upwards)
		{
			iX = iSlideX;
			iY = iSlideY;
			fXDir = 0;
			if (was_pushed_upwards)
			{
				// When pushed upwards and slide was found into a target position, insert directly to allow additional PXS at same location to solidify in next position in same frame
				if (::Landscape.GetDensity(iX, iY + Sign(GravAccel)) >= mdens)
				{
					return true;
				}
			}
			// Continue existing (and fall down next frame)
			return false;
		}
		// Otherwise, just move using xdir/ydir for nice visuals when rain is moving over landscape
		// Accelerate into the direction
		fXDir = (fXDir * 10 + Sign(iSlideX - iX)) / 11 + C4REAL10(Random(5)-2);
		// Slide target in range? Move there directly.
		if (Abs(iX - iSlideX) <= Abs(fixtoi(fXDir)))
		{
			iX = iSlideX;
			iY = iSlideY;
			if (fYDir <= 0) fXDir = 0;
		}
		// Continue existance
		return false;
	}
	// insertion OK
	return true;
}

bool mrfUserCheck(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	// check execution mask
	if ((1<<evEvent) & ~pReaction->iExecMask) return false;
	// do splash/slide check, if desired
	if (pReaction->fInsertionCheck && evEvent == meePXSMove)
		if (!mrfInsertCheck(iX, iY, fXDir, fYDir, iPxsMat, iLsMat, pfPosChanged))
			return false;
	// checks OK; reaction may be applied
	return true;
}

bool C4MaterialMap::mrfConvert(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	if (pReaction->fUserDefined) if (!mrfUserCheck(pReaction, iX, iY, iLSPosX, iLSPosY, fXDir, fYDir, iPxsMat, iLsMat, evEvent, pfPosChanged)) return false;
	switch (evEvent)
	{
	case meePXSMove: // PXS movement
		// for hardcoded stuff: only InMatConvert is Snow in Water, which does not have any collision proc
		if (!pReaction->fUserDefined) break;
		// user-defined conversions may also convert upon hitting materials

	case meePXSPos: // PXS check before movement
	{
		// Check depth
		int32_t iDepth = pReaction->fUserDefined ? pReaction->iDepth : ::MaterialMap.Map[iPxsMat].InMatConvertDepth;
		if (!iDepth || GBackMat(iX, iY - iDepth) == iLsMat)
		{
			// Convert
			iPxsMat = pReaction->fUserDefined ? pReaction->iConvertMat : ::MaterialMap.Map[iPxsMat].InMatConvertTo;
			if (!MatValid(iPxsMat))
				// Convert failure (target mat not be loaded, or target may be C4TLS_MatSky): Kill Pix
				return true;
			// stop movement after conversion
			fXDir = fYDir = 0;
			if (pfPosChanged) *pfPosChanged = true;
		}
	}
	break;

	case meeMassMove: // MassMover-movement
		// Conversion-transfer to PXS
		::PXS.Create(iPxsMat,itofix(iX),itofix(iY));
		return true;
	}
	// not handled
	return false;
}

bool C4MaterialMap::mrfPoof(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	if (pReaction->fUserDefined) if (!mrfUserCheck(pReaction, iX, iY, iLSPosX, iLSPosY, fXDir, fYDir, iPxsMat, iLsMat, evEvent, pfPosChanged)) return false;
	switch (evEvent)
	{
	case meeMassMove: // MassMover-movement
	case meePXSPos: // PXS check before movement: Kill both landscape and PXS mat
		::Landscape.ExtractMaterial(iLSPosX,iLSPosY,false);
		if (!Random(3)) Smoke(iX,iY,3);
		if (!Random(3)) StartSoundEffectAt("Liquids::Pshshsh", iX, iY);
		return true;

	case meePXSMove: // PXS movement
		// incindiary/extinguisher/corrosives are always same density proc; so do insertion check first
		// Do not allow sliding though (e.g. water on lava).
		if (!pReaction->fUserDefined)
			if (!mrfInsertCheck(iX, iY, fXDir, fYDir, iPxsMat, iLsMat, pfPosChanged, true))
				// either splash or slide prevented interaction
				return false;
		// Always kill both landscape and PXS mat
		::Landscape.ExtractMaterial(iLSPosX,iLSPosY,false);
		if (!Random(3)) Smoke(iX,iY,3);
		if (!Random(3)) StartSoundEffectAt("Liquids::Pshshsh", iX, iY);
		return true;
	}
	// not handled
	return false;
}

bool C4MaterialMap::mrfCorrode(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	if (pReaction->fUserDefined) if (!mrfUserCheck(pReaction, iX, iY, iLSPosX, iLSPosY, fXDir, fYDir, iPxsMat, iLsMat, evEvent, pfPosChanged)) return false;
	switch (evEvent)
	{
	case meePXSPos: // PXS check before movement
		// No corrosion - it would make acid incredibly effective
		break;
	case meeMassMove: // MassMover-movement
	{
		// evaluate corrosion percentage
		bool fDoCorrode; int d100 = Random(100);
		if (pReaction->fUserDefined)
			fDoCorrode = (d100 < pReaction->iCorrosionRate);
		else
			fDoCorrode = (d100 < ::MaterialMap.Map[iPxsMat].Corrosive) && (d100 < ::MaterialMap.Map[iLsMat].Corrode);
		if (fDoCorrode)
		{
			::Landscape.ClearPix(iLSPosX,iLSPosY);
			//::Landscape.CheckInstabilityRange(iLSPosX,iLSPosY); - more correct, but makes acid too effective as well
			if (!Random(5))
			{
				Smoke(iX, iY, 3 + Random(3));
			}
			if (!Random(20)) StartSoundEffectAt("Liquids::Corrode", iX, iY);
			return true;
		}
	}
	break;

	case meePXSMove: // PXS movement
	{
		// corrodes to corrosives are always same density proc; so do insertion check first
		if (!pReaction->fUserDefined)
			if (!mrfInsertCheck(iX, iY, fXDir, fYDir, iPxsMat, iLsMat, pfPosChanged))
				// either splash or slide prevented interaction
				return false;
		// evaluate corrosion percentage
		bool fDoCorrode; int d100 = Random(100);
		if (pReaction->fUserDefined)
			fDoCorrode = (d100 < pReaction->iCorrosionRate);
		else
			fDoCorrode = (d100 < ::MaterialMap.Map[iPxsMat].Corrosive) && (d100 < ::MaterialMap.Map[iLsMat].Corrode);
		if (fDoCorrode)
		{
			::Landscape.ClearPix(iLSPosX,iLSPosY);
			::Landscape.CheckInstabilityRange(iLSPosX,iLSPosY);
			if (!Random(5))
			{
				Smoke(iX,iY,3+Random(3));
			}
			if (!Random(20)) StartSoundEffectAt("Liquids::Corrode", iX, iY);
			return true;
		}
		// Else: dead. Insert material here
		::Landscape.InsertMaterial(iPxsMat,&iX,&iY);
		return true;
	}
	}
	// not handled
	return false;
}

bool C4MaterialMap::mrfIncinerate(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	// not available as user reaction
	assert(!pReaction->fUserDefined);
	switch (evEvent)
	{
	case meeMassMove: // MassMover-movement
	case meePXSPos: // PXS check before movement
		if (::Landscape.Incinerate(iX, iY, NO_OWNER)) return true;
		break;

	case meePXSMove: // PXS movement
		// incinerate to inflammables are always same density proc; so do insertion check first
		if (!mrfInsertCheck(iX, iY, fXDir, fYDir, iPxsMat, iLsMat, pfPosChanged))
			// either splash or slide prevented interaction
			return false;
		// evaluate inflammation (should always succeed)
		if (::Landscape.Incinerate(iX, iY, NO_OWNER)) return true;
		// Else: dead. Insert material here
		::Landscape.InsertMaterial(iPxsMat,&iX,&iY);
		return true;
	}
	// not handled
	return false;
}

bool C4MaterialMap::mrfInsert(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	if (pReaction->fUserDefined) if (!mrfUserCheck(pReaction, iX, iY, iLSPosX, iLSPosY, fXDir, fYDir, iPxsMat, iLsMat, evEvent, pfPosChanged)) return false;
	switch (evEvent)
	{
	case meePXSPos: // PXS check before movement
		break;

	case meePXSMove: // PXS movement
	{
		// check for bounce/slide
		if (!pReaction->fUserDefined)
			if (!mrfInsertCheck(iX, iY, fXDir, fYDir, iPxsMat, iLsMat, pfPosChanged))
				// continue existing
				return false;
		// Else: dead. Insert material here
		::Landscape.InsertMaterial(iPxsMat,&iX,&iY);
		return true;
	}

	case meeMassMove: // MassMover-movement
		break;
	}
	// not handled
	return false;
}

bool C4MaterialMap::mrfScript(C4MaterialReaction *pReaction, int32_t &iX, int32_t &iY, int32_t iLSPosX, int32_t iLSPosY, C4Real &fXDir, C4Real &fYDir, int32_t &iPxsMat, int32_t iLsMat, MaterialInteractionEvent evEvent, bool *pfPosChanged)
{
	// do generic checks for user-defined reactions
	if (!mrfUserCheck(pReaction, iX, iY, iLSPosX, iLSPosY, fXDir, fYDir, iPxsMat, iLsMat, evEvent, pfPosChanged))
		return false;

	// check script func
	if (!pReaction->pScriptFunc) return false;
	// OK - let's call it!
	//                      0           1           2                3                        4                           5                      6               7              8
	int32_t iXDir1, iYDir1, iXDir2, iYDir2;
	C4AulParSet pars(iX, iY, iLSPosX, iLSPosY, iXDir1 = fixtoi(fXDir, 100), iYDir1 = fixtoi(fYDir, 100), iPxsMat, iLsMat, int(evEvent));
	if (!!pReaction->pScriptFunc->Exec(nullptr, &pars, false))
	{
		// PXS shall be killed!
		return true;
	}
	// PXS shall exist further: write back parameters
	iPxsMat = pars[6].getInt();
	int32_t iX2 = pars[0].getInt(), iY2 = pars[1].getInt();
	iXDir2 = pars[4].getInt(); iYDir2 = pars[5].getInt();
	if (iX!=iX2 || iY!=iY2 || iXDir1!=iXDir2 || iYDir1!=iYDir2)
	{
		// changes to pos/speed detected
		if (pfPosChanged) *pfPosChanged = true;
		iX=iX2; iY=iY2;
		fXDir = C4REAL100(iXDir2);
		fYDir = C4REAL100(iYDir2);
	}
	// OK; done
	return false;
}

void C4MaterialMap::UpdateScriptPointers()
{
	// update in all materials
	for (int32_t i=0; i<Num; ++i) Map[i].UpdateScriptPointers();
}

C4MaterialMap MaterialMap;
