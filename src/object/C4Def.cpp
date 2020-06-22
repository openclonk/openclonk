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

/* Object definition */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Def.h"

#include "c4group/C4Components.h"
#include "graphics/C4Draw.h"
#include "graphics/C4DrawGL.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/CSurface8.h"
#include "landscape/C4Particles.h"
#include "landscape/C4SolidMask.h"
#include "lib/StdColors.h"
#include "lib/StdMeshLoader.h"
#include "object/C4Object.h"
#include "platform/C4FileMonitor.h"
#include "platform/C4SoundSystem.h"
#include "player/C4RankSystem.h"

// Helper class to load additional resources required for meshes from
// a C4Group.
class C4DefAdditionalResourcesLoader: public StdMeshMaterialLoader
{
public:
	C4DefAdditionalResourcesLoader(C4Group& hGroup): Group(hGroup) {}

	C4Surface* LoadTexture(const char* filename) override
	{
		if (!Group.AccessEntry(filename)) return nullptr;
		C4Surface* surface = new C4Surface;
		// Suppress error message here, StdMeshMaterial loader
		// will show one.
		if (!surface->Read(Group, GetExtension(filename), C4SF_MipMap))
			{ delete surface; surface = nullptr; }
		return surface;
	}

	StdStrBuf LoadShaderCode(const char* filename) override
	{
		StdStrBuf ret;
		if (!Group.LoadEntryString(filename, &ret)) return StdStrBuf();
		return ret;
	}

	void AddShaderSlices(C4Shader& shader, int ssc) override
	{
#ifndef USE_CONSOLE
		// Add mesh-independent slices
		shader.AddDefine("OPENCLONK");
		shader.AddDefine("OC_MESH");

		if (ssc & C4SSC_MOD2) shader.AddDefine("OC_CLRMOD_MOD2");
		if (ssc & C4SSC_LIGHT) shader.AddDefine("OC_DYNAMIC_LIGHT");

		// Note these are never set for meshes at the moment:
		if (ssc & C4SSC_BASE) shader.AddDefine("OC_HAVE_BASE");
		if (ssc & C4SSC_OVERLAY) shader.AddDefine("OC_HAVE_OVERLAY");

		shader.LoadFragmentSlices(&::GraphicsResource.Files, "CommonShader.glsl");
		shader.LoadFragmentSlices(&::GraphicsResource.Files, "ObjectShader.glsl");

		// Categories for script shaders.
		shader.SetScriptCategories({"Common", "Object"});
#endif
	}

private:
	C4Group& Group;
};



void C4Def::DefaultDefCore()
{
	rC4XVer[0]=rC4XVer[1]=0;
	RequireDef.Clear();
	Shape.Default();
	Entrance.Default();
	Collection.Default();
	PictureRect.Default();
	SolidMask.Default();
	TopFace.Default();
	BurnTurnTo=C4ID::None;
	GrowthType=0;
	CrewMember=0;
	NativeCrew=0;
	Mass=0;
	Value=0;
	Exclusive=0;
	Category=0;
	Constructable=0;
	Rotateable=0;
	RotatedEntrance=0;
	Float=0;
	ColorByOwner=0;
	NoHorizontalMove=0;
	LiftTop=0;
	GrabPutGet=0;
	UprightAttach=0;
	Line=0;
	LineIntersect=0;
	IncompleteActivity=0;
	Oversize=0;
	Fragile=0;
	NoPushEnter=0;
	Projectile=0;
	VehicleControl=0;
	Pathfinder=0;
	NoMassFromContents=0;
	MoveToRange=0;
	NoStabilize=0;
	ClosedContainer=0;
	SilentCommands=0;
	TemporaryCrew=0;
	BlitMode=C4D_Blit_Normal;
	NoBreath=0;
	ConSizeOff=0;
	NoGet=0;
	NoTransferZones=0;
	HideInCreator = false;
}

bool C4Def::LoadDefCore(C4Group &hGroup)
{
	StdStrBuf Source;
	if (hGroup.LoadEntryString(C4CFN_DefCore,&Source))
	{
		StdStrBuf Name = hGroup.GetFullName() + (const StdStrBuf &)FormatString("%cDefCore.txt", DirectorySeparator);
		if (!Compile(Source.getData(), Name.getData()))
			return false;
		Source.Clear();

		// Check mass
		if (Mass < 0)
		{
			DebugLogF("WARNING: Def %s (%s) at %s has invalid mass!", GetName(), id.ToString(), hGroup.GetFullName().getData());
			Mass = 0;
		}

		return true;
	}
	return false;
}

bool C4Def::Save(C4Group &hGroup)
{
	StdStrBuf Out;
	if (! Decompile(&Out, FormatString("%s::DefCore.txt", id.ToString()).getData()) )
		return false;
	return hGroup.Add(C4CFN_DefCore,Out,false,true);
}

bool C4Def::Compile(const char *szSource, const char *szName)
{
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "DefCore"), StdStrBuf(szSource), szName);
}

bool C4Def::Decompile(StdStrBuf *pOut, const char *szName)
{
	return DecompileToBuf_Log<StdCompilerINIWrite>(mkNamingAdapt(*this, "DefCore"), pOut, szName);
}

void C4Def::CompileFunc(StdCompiler *pComp)
{

	pComp->Value(mkNamingAdapt(id,                "id",                 C4ID::None          ));
	pComp->Value(mkNamingAdapt(toC4CArr(rC4XVer),             "Version"                               ));
	pComp->Value(mkNamingAdapt(mkParAdapt(RequireDef, false), "RequireDef",         C4IDList()        ));

	const StdBitfieldEntry<int32_t> Categories[] =
	{

		{ "C4D_None",                     C4D_None                },
		{ "C4D_StaticBack",               C4D_StaticBack          },
		{ "C4D_Structure",                C4D_Structure           },
		{ "C4D_Vehicle",                  C4D_Vehicle             },
		{ "C4D_Living",                   C4D_Living              },
		{ "C4D_Object",                   C4D_Object              },

		{ "C4D_Goal",                     C4D_Goal                },
		{ "C4D_Rule",                     C4D_Rule                },
		{ "C4D_Environment",              C4D_Environment         },
		
		{ "C4D_Background",               C4D_Background          },
		{ "C4D_Parallax",                 C4D_Parallax            },
		{ "C4D_MouseSelect",              C4D_MouseSelect         },
		{ "C4D_Foreground",               C4D_Foreground          },
		{ "C4D_MouseIgnore",              C4D_MouseIgnore         },
		{ "C4D_IgnoreFoW",                C4D_IgnoreFoW           },

		{ nullptr,                           0                       }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt<int32_t>(Category, Categories),
	                           "Category",           0             ));

	pComp->Value(mkParAdapt(Shape, static_cast<C4Shape*>(nullptr)));
	pComp->Value(mkNamingAdapt(Value,                         "Value",              0                 ));
	pComp->Value(mkNamingAdapt(Mass,                          "Mass",               0                 ));
	pComp->Value(mkNamingAdapt(SolidMask,                     "SolidMask",          TargetRect0       ));
	pComp->Value(mkNamingAdapt(TopFace,                       "TopFace",            TargetRect0       ));
	pComp->Value(mkNamingAdapt(PictureRect,                   "Picture",            Rect0             ));
	pComp->Value(mkNamingAdapt(Entrance,                      "Entrance",           Rect0             ));
	pComp->Value(mkNamingAdapt(Collection,                    "Collection",         Rect0             ));
	pComp->Value(mkNamingAdapt(Exclusive,                     "Exclusive",          0                 ));
	pComp->Value(mkNamingAdapt(Line,                          "Line",               0                 ));
	// <Newton> undocumented, but obsolete? I don't understand the sense of this value.
	pComp->Value(mkNamingAdapt(LineIntersect,                 "LineIntersect",      0                 ));
	pComp->Value(mkNamingAdapt(CrewMember,                    "CrewMember",         0                 ));
	pComp->Value(mkNamingAdapt(NativeCrew,                    "NoStandardCrew",     0                 ));
	pComp->Value(mkNamingAdapt(Constructable,                 "Construction",       0                 ));

	const StdBitfieldEntry<int32_t> GrabPutGetTypes[] =
	{

		{ "C4D_GrabGet"            ,C4D_Grab_Get},
		{ "C4D_GrabPut"            ,C4D_Grab_Put},

		{ nullptr,                     0}
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(GrabPutGet, GrabPutGetTypes),
	                           "GrabPutGet",         0                 ));

	pComp->Value(mkNamingAdapt(Rotateable,                    "Rotate",             0                 ));
	pComp->Value(mkNamingAdapt(RotatedEntrance,               "RotatedEntrance",    0                 ));
	pComp->Value(mkNamingAdapt(Float,                         "Float",              0                 ));
	pComp->Value(mkNamingAdapt(ColorByOwner,                  "ColorByOwner",       0                 ));
	pComp->Value(mkNamingAdapt(NoHorizontalMove,              "HorizontalFix",      0                 ));
	pComp->Value(mkNamingAdapt(LiftTop,                       "LiftTop",            0                 ));
	pComp->Value(mkNamingAdapt(UprightAttach,                 "UprightAttach",      0                 ));
	pComp->Value(mkNamingAdapt(GrowthType,                    "StretchGrowth",      0                 ));
	pComp->Value(mkNamingAdapt(IncompleteActivity,            "IncompleteActivity", 0                 ));
	pComp->Value(mkNamingAdapt(Oversize,                      "Oversize",           0                 ));
	// <Newton> Fragile and Projectile are kinda obsolete.
	// Only used at one point in the command system. Should rather be solved with properties if at all
	pComp->Value(mkNamingAdapt(Fragile,                       "Fragile",            0                 ));
	pComp->Value(mkNamingAdapt(Projectile,                    "Projectile",         0                 ));

	pComp->Value(mkNamingAdapt(NoPushEnter,                   "NoPushEnter",        0                 ));
	pComp->Value(mkNamingAdapt(VehicleControl,                "VehicleControl",     0                 ));
	pComp->Value(mkNamingAdapt(Pathfinder,                    "Pathfinder",         0                 ));
	pComp->Value(mkNamingAdapt(MoveToRange,                   "MoveToRange",        0                 ));
	pComp->Value(mkNamingAdapt(NoMassFromContents,            "NoMassFromContents", 0                 ));
	pComp->Value(mkNamingAdapt(NoStabilize,                   "NoStabilize",        0                 ));
	pComp->Value(mkNamingAdapt(ClosedContainer,               "ClosedContainer",    0                 ));
	pComp->Value(mkNamingAdapt(SilentCommands,                "SilentCommands",     0                 ));
	pComp->Value(mkNamingAdapt(TemporaryCrew,                 "TemporaryCrew",      0                 ));
	pComp->Value(mkNamingAdapt(BlitMode,                      "BlitMode",           C4D_Blit_Normal   ));
	pComp->Value(mkNamingAdapt(NoBreath,                      "NoBreath",           0                 ));
	pComp->Value(mkNamingAdapt(ConSizeOff,                    "ConSizeOff",         0                 ));
	pComp->Value(mkNamingAdapt(NoGet,                         "NoGet",              0                 ));
	pComp->Value(mkNamingAdapt(NoTransferZones,               "NoTransferZones",    0                 ));

	const StdBitfieldEntry<int32_t> AllowPictureStackModes[] =
	{

		{ "APS_Color",      APS_Color    },
		{ "APS_Graphics",   APS_Graphics },
		{ "APS_Name",       APS_Name     },
		{ "APS_Overlay",    APS_Overlay  },
		{ nullptr,             0            }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt<int32_t>(AllowPictureStack, AllowPictureStackModes),		//undocumented
	                           "AllowPictureStack",   0                ));
	pComp->Value(mkNamingAdapt(HideInCreator, "HideInCreator", false));
}

//-------------------------------- C4Def -------------------------------------------------------

C4Def::C4Def(): Script(), C4PropListStatic(ScriptEngine.GetPropList(), nullptr, nullptr)
{
	Script.SetDef(this);
	assert(ScriptEngine.GetPropList());
	Graphics.pDef = this;
	Default();
}

void C4Def::Default()
{
	DefaultDefCore();
	Next=nullptr;
	Temporary=false;
	Filename[0]=0;
	Creation=0;
	Count=0;
	MainFace.Set(nullptr,0,0,0,0);
	Script.Clear();
	StringTable.Clear();
	pClonkNames=nullptr;
	pRankNames=nullptr;
	pRankSymbols=nullptr;
	fClonkNamesOwned = fRankNamesOwned = fRankSymbolsOwned = false;
	iNumRankSymbols=1;
	pSolidMask = nullptr;
}

C4Def::~C4Def()
{
	Clear();
}

void C4Def::Clear()
{
	Script.Clear();
	C4PropList::Clear();

	Graphics.Clear();

	StringTable.Clear();
	
	if (fClonkNamesOwned)
		delete pClonkNames;
	pClonkNames = nullptr;

	if (fRankNamesOwned)
		delete pRankNames;
	pRankNames = nullptr;

	if (fRankSymbolsOwned)
		delete pRankSymbols;
	pRankSymbols = nullptr;
	
	fClonkNamesOwned = fRankNamesOwned = fRankSymbolsOwned = false;
	delete pSolidMask; pSolidMask = nullptr;
}

bool C4Def::Load(C4Group &hGroup,
	StdMeshSkeletonLoader &loader,
	DWORD dwLoadWhat,
	const char *szLanguage,
	C4SoundSystem *pSoundSystem,
	C4DefGraphicsPtrBackup *gfx_backup
	)
{
	bool AddFileMonitoring = false;
	if (Game.pFileMonitor && !SEqual(hGroup.GetFullName().getData(),Filename) && !hGroup.IsPacked())
		AddFileMonitoring = true;

	// Store filename
	SCopy(hGroup.GetFullName().getData(),Filename);

	// Verbose log filename
	if (Config.Graphics.VerboseObjectLoading>=3)
		Log(hGroup.GetFullName().getData());

	if (AddFileMonitoring) Game.pFileMonitor->AddDirectory(Filename);

	// Pre-read all images and shader stuff because they ar eaccessed in unpredictable order during loading
	hGroup.PreCacheEntries(C4CFN_ShaderFiles);
	hGroup.PreCacheEntries(C4CFN_ImageFiles);

	LoadMeshMaterials(hGroup, gfx_backup);
	bool fSuccess = LoadParticleDef(hGroup);

	// Read DefCore
	if (fSuccess) fSuccess = LoadDefCore(hGroup);

	// Skip def: don't even read sounds!
	if (fSuccess && Game.C4S.Definitions.SkipDefs.GetIDCount(id, 1)) return false;

	// Read sounds, even if not a valid def (for pure ocd sound folders)
	if (dwLoadWhat & C4D_Load_Sounds) LoadSounds(hGroup, pSoundSystem);

	// cancel if not a valid definition
	if (!fSuccess) return false;

	// Read and parse SolidMask bitmap
	if (!LoadSolidMask(hGroup)) return false;

	// Read surface bitmap, meshes, skeletons
	if ((dwLoadWhat & C4D_Load_Bitmap) && !LoadGraphics(hGroup, loader)) return false;

	// Read string table
	C4Language::LoadComponentHost(&StringTable, hGroup, C4CFN_ScriptStringTbl, szLanguage);

	// Register ID with script engine
	::ScriptEngine.RegisterGlobalConstant(id.ToString(), C4VPropList(this));
	ParentKeyName = ::Strings.RegString(id.ToString());

	// Read script
	if (dwLoadWhat & C4D_Load_Script) LoadScript(hGroup, szLanguage);

	// Read clonknames
	if (dwLoadWhat & C4D_Load_ClonkNames) LoadClonkNames(hGroup, pClonkNames, szLanguage);

	// Read clonkranks
	if (dwLoadWhat & C4D_Load_RankNames) LoadRankNames(hGroup, szLanguage);

	// Read rankfaces
	if (dwLoadWhat & C4D_Load_RankFaces) LoadRankFaces(hGroup);

	// Temporary flag
	if (dwLoadWhat & C4D_Load_Temporary) Temporary=true;

	return true;
}

void C4Def::LoadMeshMaterials(C4Group &hGroup, C4DefGraphicsPtrBackup *gfx_backup)
{
	// Load all mesh materials from this folder
	C4DefAdditionalResourcesLoader loader(hGroup);
	hGroup.ResetSearch();
	char MaterialFilename[_MAX_PATH + 1]; *MaterialFilename = 0;
	
	for (const auto &mat : mesh_materials)
	{
		::MeshMaterialManager.Remove(mat, &gfx_backup->GetUpdater());
	}
	mesh_materials.clear();
	while (hGroup.FindNextEntry(C4CFN_DefMaterials, MaterialFilename, nullptr, !!*MaterialFilename))
	{
		StdStrBuf material;
		if (hGroup.LoadEntryString(MaterialFilename, &material))
		{
			try
			{
				StdStrBuf buf;
				buf.Copy(hGroup.GetName());
				buf.Append("/"); buf.Append(MaterialFilename);
				auto new_materials = ::MeshMaterialManager.Parse(material.getData(), buf.getData(), loader);
				mesh_materials.insert(new_materials.begin(), new_materials.end());
			}
			catch (const StdMeshMaterialError& ex)
			{
				DebugLogF("Failed to read material script: %s", ex.what());
			}
		}
	}
}

bool C4Def::LoadParticleDef(C4Group &hGroup)
{
	bool fSuccess = true;
	// particle def?
	if (hGroup.AccessEntry(C4CFN_ParticleCore))
	{
		// def loading not successful; abort after reading sounds
		fSuccess = false;
		// create new particle def
		C4ParticleDef *pParticleDef = new C4ParticleDef();
		// load it
		if (!pParticleDef->Load(hGroup))
		{
			// not successful :( - destroy it again
			delete pParticleDef;
		}
		// done
	}
	return fSuccess;
}

bool C4Def::LoadSolidMask(C4Group &hGroup)
{
	if (hGroup.FindEntry(C4CFN_SolidMask))
	{
		pSolidMask = C4SolidMask::LoadMaskFromFile(hGroup, C4CFN_SolidMask);
		if (!pSolidMask)
		{
			DebugLogF("  Error loading SolidMask of %s (%s)", hGroup.GetFullName().getData(), id.ToString());
			return false;
		}
		// check SolidMask size
		if (SolidMask.x<0 || SolidMask.y<0 || SolidMask.x + SolidMask.Wdt>pSolidMask->Wdt || SolidMask.y + SolidMask.Hgt>pSolidMask->Hgt) SolidMask.Default();
	}
	else if (SolidMask.Wdt)
	{
		// Warning in case someone wants to define SolidMasks the old way (in the main graphics file)
		DebugLogF("WARNING: Definition %s (%s) defines SolidMask in DefCore but has no SolidMask file!", hGroup.GetFullName().getData(), id.ToString());
		SolidMask.Default();
	}

	return true;
}

bool C4Def::LoadGraphics(C4Group &hGroup, StdMeshSkeletonLoader &loader)
{
	// Try to load graphics
	// No fail on error - just have an object without graphics.
	Graphics.Load(hGroup, loader, !!ColorByOwner);

	if (Graphics.Type == C4DefGraphics::TYPE_Bitmap)
	{
		// Bitmap post-load settings
		if (Graphics.GetBitmap())
		{
			// Set MainFace (unassigned bitmap: will be set by GetMainFace())
			MainFace.Set(nullptr, 0, 0, Shape.Wdt, Shape.Hgt);
		}

		// Adjust picture rect
		if ((PictureRect.Wdt == 0) || (PictureRect.Hgt == 0))
			PictureRect.Set(0, 0, Shape.Wdt*Graphics.Bmp.Bitmap->Scale, Shape.Hgt*Graphics.Bmp.Bitmap->Scale);

		// validate TopFace
		if (TopFace.x<0 || TopFace.y<0 || TopFace.x + TopFace.Wdt>Graphics.Bmp.Bitmap->Wdt || TopFace.y + TopFace.Hgt>Graphics.Bmp.Bitmap->Hgt)
		{
			TopFace.Default();
			// warn in debug mode
			DebugLogF("invalid TopFace in %s (%s)", GetName(), id.ToString());
		}
	}
	else
	{
		TopFace.Default();
	}

	return true;
}

void C4Def::LoadScript(C4Group &hGroup, const char* szLanguage)
{
	// reg script to engine
	Script.Reg2List(&::ScriptEngine);
	// Load script
	Script.Load(hGroup, C4CFN_Script, szLanguage, &StringTable);
}

void C4Def::LoadClonkNames(C4Group &hGroup, C4ComponentHost* pClonkNames, const char* szLanguage)
{
	// clear any previous
	delete pClonkNames; pClonkNames = nullptr;
	if (hGroup.FindEntry(C4CFN_ClonkNameFiles))
	{
		// create new
		pClonkNames = new C4ComponentHost();
		if (!C4Language::LoadComponentHost(pClonkNames, hGroup, C4CFN_ClonkNames, szLanguage))
		{
			delete pClonkNames; pClonkNames = nullptr;
		}
		else
			fClonkNamesOwned = true;
	}
}

void C4Def::LoadRankNames(C4Group &hGroup, const char* szLanguage)
{
	// clear any previous
	delete pRankNames; pRankNames = nullptr;
	if (hGroup.FindEntry(C4CFN_RankNameFiles))
	{
		// create new
		pRankNames = new C4RankSystem();
		// load from group
		if (!pRankNames->Load(hGroup, C4CFN_RankNames, 1000, szLanguage))
		{
			delete pRankNames; pRankNames = nullptr;
		}
		else
			fRankNamesOwned = true;
	}
}

void C4Def::LoadRankFaces(C4Group &hGroup)
{
	// clear any previous
	delete pRankSymbols; pRankSymbols = nullptr;
	// load new
	if (hGroup.AccessEntry(C4CFN_RankFacesPNG))
	{
		pRankSymbols = new C4FacetSurface();
		if (!pRankSymbols->GetFace().ReadPNG(hGroup, false)) { delete pRankSymbols; pRankSymbols = nullptr; }
	}
	// set size
	if (pRankSymbols)
	{
		pRankSymbols->Set(&pRankSymbols->GetFace(), 0, 0, pRankSymbols->GetFace().Hgt, pRankSymbols->GetFace().Hgt);
		int32_t Q; pRankSymbols->GetPhaseNum(iNumRankSymbols, Q);
		if (!iNumRankSymbols) { delete pRankSymbols; pRankSymbols = nullptr; }
		else
		{
			if (pRankNames)
			{
				// if extended rank names are defined, subtract those from the symbol count. The last symbols are used as overlay
				iNumRankSymbols = std::max<int32_t>(1, iNumRankSymbols - pRankNames->GetExtendedRankNum());
			}
			fRankSymbolsOwned = true;
		}
	}
}

void C4Def::LoadSounds(C4Group &hGroup, C4SoundSystem* pSoundSystem)
{
	if (pSoundSystem)
		pSoundSystem->LoadEffects(hGroup, (id == C4ID::None) ? nullptr : id.ToString(), true);
}

void C4Def::Draw(C4Facet &cgo, bool fSelected, DWORD iColor, C4Object *pObj, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform* trans, const char *graphicsName)
{
	if(fSelected)
		pDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, C4RGB(0xca, 0, 0));

	C4DefGraphics* graphics = pObj ? pObj->GetGraphics() : &Graphics;
	if (graphicsName)
	{
		C4DefGraphics *other = graphics->Get(graphicsName);
		if (other) graphics = other;
	}
	graphics->Draw(cgo, iColor, pObj, iPhaseX, iPhaseY, trans);
}

int32_t C4Def::GetValue(C4Object *pInBase, int32_t iBuyPlayer)
{
	C4Value r = Call(PSF_CalcDefValue, &C4AulParSet(pInBase, iBuyPlayer));
	int32_t iValue = Value;
	if (r != C4VNull)
		iValue = r.getInt();
	// do any adjustments based on where the item is bought
	if (pInBase)
	{
		r = pInBase->Call(PSF_CalcBuyValue, &C4AulParSet(this, iValue));
		if (r != C4VNull)
			iValue = r.getInt();
	}
	return iValue;
}

void C4Def::Synchronize()
{
}

void C4Def::IncludeDefinition(C4Def *pIncludeDef)
{
	// inherited rank infos and clonk names, if this definition doesn't have its own
	if (!fClonkNamesOwned) pClonkNames = pIncludeDef->pClonkNames;
	if (!fRankNamesOwned) pRankNames = pIncludeDef->pRankNames;
	if (!fRankSymbolsOwned) { pRankSymbols = pIncludeDef->pRankSymbols; iNumRankSymbols = pIncludeDef->iNumRankSymbols; }
}

void C4Def::ResetIncludeDependencies()
{
	// clear all pointers into foreign defs
	if (!fClonkNamesOwned) pClonkNames = nullptr;
	if (!fRankNamesOwned) pRankNames = nullptr;
	if (!fRankSymbolsOwned) { pRankSymbols = nullptr; iNumRankSymbols = 0; }
}

C4PropList *C4Def::GetActionByName(const char *actname)
{
	if (!actname) return nullptr;
	C4String * actname_str = Strings.RegString(actname);
	actname_str->IncRef();
	C4PropList *r = GetActionByName(actname_str);
	actname_str->DecRef();
	return r;
}

C4PropList *C4Def::GetActionByName(C4String *actname)
{
	assert(actname);
	// If we get the null string or ActIdle by name, return nullptr action
	if (!actname || actname == &Strings.P[P_Idle]) return nullptr;
	// otherwise, query actmap
	C4Value ActMap; GetProperty(P_ActMap, &ActMap);
	if (!ActMap.getPropList()) return nullptr;
	C4Value Action; ActMap.getPropList()->GetPropertyByS(actname, &Action);
	if (!Action.getPropList()) return nullptr;
	return Action.getPropList();
}
