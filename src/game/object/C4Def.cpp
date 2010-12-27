/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2004, 2007  Matthes Bender
 * Copyright (c) 2001-2007, 2009-2010  Sven Eberhardt
 * Copyright (c) 2003-2008  Peter Wortmann
 * Copyright (c) 2004-2006, 2008-2010  Günther Brammer
 * Copyright (c) 2005, 2009-2010  Armin Burgmeier
 * Copyright (c) 2009-2010  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Randrian
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

/* Object definition */

#include <C4Include.h>
#include <C4Def.h>
#include <C4Version.h>
#include <C4GameVersion.h>
#include <C4FileMonitor.h>

#include <C4SurfaceFile.h>
#include <C4Log.h>
#include <C4Components.h>
#include <C4Config.h>
#include <C4RankSystem.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4Object.h>
#include "C4Network2Res.h"
#include <C4Material.h>

//--------------------------------- C4DefCore ----------------------------------------------

void C4Def::DefaultDefCore()
{
	rC4XVer[0]=rC4XVer[1]=rC4XVer[2]=rC4XVer[3]=0;
	RequireDef.Clear();
	Shape.Default();
	Entrance.Default();
	Collection.Default();
	PictureRect.Default();
	SolidMask.Default();
	TopFace.Default();
	Component.Default();
	BurnTurnTo=C4ID::None;
	BuildTurnTo=C4ID::None;
	STimerCall[0]=0;
	Timer=35;
	GrowthType=0;
	CrewMember=0;
	NativeCrew=0;
	Mass=0;
	Value=0;
	Exclusive=0;
	Category=0;
	Growth=0;
	Rebuyable=0;
	ContactIncinerate=0;
	BlastIncinerate=0;
	Constructable=0;
	Grab=0;
	Carryable=0;
	Rotateable=0;
	RotatedEntrance=0;
	Chopable=0;
	Float=0;
	ColorByOwner=0;
	NoHorizontalMove=0;
	BorderBound=0;
	LiftTop=0;
	GrabPutGet=0;
	ContainBlast=0;
	UprightAttach=0;
	ContactFunctionCalls=0;
	Line=0;
	LineConnect=0;
	LineIntersect=0;
	NoBurnDecay=0;
	IncompleteActivity=0;
	Placement=0;
	Prey=0;
	Edible=0;
	AttractLightning=0;
	Oversize=0;
	Fragile=0;
	NoPushEnter=0;
	Explosive=0;
	Projectile=0;
	DragImagePicture=0;
	VehicleControl=0;
	Pathfinder=0;
	NoComponentMass=0;
	MoveToRange=0;
	NoStabilize=0;
	ClosedContainer=0;
	SilentCommands=0;
	NoBurnDamage=0;
	TemporaryCrew=0;
	SmokeRate=100;
	BlitMode=C4D_Blit_Normal;
	NoBreath=0;
	ConSizeOff=0;
	NoSell=NoGet=0;
	NeededGfxMode=0;
	NoTransferZones=0;
}

bool C4Def::LoadDefCore(C4Group &hGroup)
{
	StdStrBuf Source;
	if (hGroup.LoadEntryString(C4CFN_DefCore,Source))
	{
		StdStrBuf Name = hGroup.GetFullName() + (const StdStrBuf &)FormatString("%cDefCore.txt", DirectorySeparator);
		if (!Compile(Source.getData(), Name.getData()))
			return false;
		Source.Clear();

		// Let's be bold: Rewrite, with current version
		/*rC4XVer[0] = C4XVER1; rC4XVer[1] = C4XVER2; rC4XVer[2] = C4XVER3; rC4XVer[3] = C4XVER4;
		hGroup.Rename(C4CFN_DefCore, C4CFN_DefCore ".old");
		Save(hGroup);*/

		// Adjust picture rect
		if ((PictureRect.Wdt==0) || (PictureRect.Hgt==0))
			PictureRect.Set(0,0,Shape.Wdt,Shape.Hgt);

		// Check category
		if (!(Category & C4D_SortLimit))
		{
			DebugLogF("WARNING: Def %s (%s) at %s has invalid category!", GetName(), id.ToString(), hGroup.GetFullName().getData());
			// assign a default category here
			Category = (Category & ~C4D_SortLimit) | 1;
		}
		// Check mass
		if (Mass < 0)
		{
			DebugLogF("WARNING: Def %s (%s) at %s has invalid mass!", GetName(), id.ToString(), hGroup.GetFullName().getData());
			Mass = 0;
		}

		// Register ID with script engine
		::ScriptEngine.RegisterGlobalConstant(id.ToString(), C4VPropList(this));
		/*
		int32_t index = ::ScriptEngine.GlobalNamedNames.GetItemNr(id.ToString());
		if (index == -1)
		{
		  index = ::ScriptEngine.GlobalNamedNames.AddName(id.ToString());
		  ::ScriptEngine.GlobalNamed.GetItem(index)->Set(C4VPropList(this));
		}*/

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
	//FIXME pComp->Value(mkNamingAdapt(toC4CStrBuf(Name),             "Name",               "Undefined"       ));
	pComp->Value(mkNamingAdapt(mkParAdapt(RequireDef, false), "RequireDef",         C4IDList()        ));

	const StdBitfieldEntry<int32_t> Categories[] =
	{

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

		{ NULL,                           0                       }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt<int32_t>(Category, Categories),
	                           "Category",           0             ));

	pComp->Value(mkNamingAdapt(Timer,                         "Timer",              35                ));
	pComp->Value(mkNamingAdapt(toC4CStr(STimerCall),          "TimerCall",          ""                ));
	pComp->Value(mkNamingAdapt(ContactFunctionCalls,          "ContactCalls",       0                 ));
	pComp->Value(mkParAdapt(Shape, false));
	pComp->Value(mkNamingAdapt(Value,                         "Value",              0                 ));
	pComp->Value(mkNamingAdapt(Mass,                          "Mass",               0                 ));
	pComp->Value(mkNamingAdapt(Component,                     "Components",         C4IDList()        ));
	pComp->Value(mkNamingAdapt(SolidMask,                     "SolidMask",          TargetRect0       ));
	pComp->Value(mkNamingAdapt(TopFace,                       "TopFace",            TargetRect0       ));
	pComp->Value(mkNamingAdapt(PictureRect,                   "Picture",            Rect0             ));
	pComp->Value(mkNamingAdapt(StdNullAdapt(),                "PictureFE"                             ));
	pComp->Value(mkNamingAdapt(Entrance,                      "Entrance",           Rect0             ));
	pComp->Value(mkNamingAdapt(Collection,                    "Collection",         Rect0             ));
	pComp->Value(mkNamingAdapt(Placement,                     "Placement",          0                 ));
	pComp->Value(mkNamingAdapt(Exclusive,                     "Exclusive",          0                 ));
	pComp->Value(mkNamingAdapt(ContactIncinerate,             "ContactIncinerate",  0                 ));
	pComp->Value(mkNamingAdapt(BlastIncinerate,               "BlastIncinerate",    0                 ));
	pComp->Value(mkNamingAdapt(BurnTurnTo,        "BurnTo",             C4ID::None          ));

	const StdBitfieldEntry<int32_t> LineTypes[] =
	{

		{ "C4D_LineSource"         ,C4D_Line_Source},
		{ "C4D_LineDrain"          ,C4D_Line_Drain},
		{ "C4D_LineColored"        ,C4D_Line_Colored},
		{ "C4D_LineVertex"         ,C4D_Line_Vertex},

		{ NULL,                     0}
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(Line, LineTypes),"Line",             0                 ));


	const StdBitfieldEntry<int32_t> LineConnectTypes[] =
	{

		{ "C4D_LiquidInput"        ,C4D_Liquid_Input},
		{ "C4D_LiquidOutput"       ,C4D_Liquid_Output},
		{ "C4D_LiquidPump"         ,C4D_Liquid_Pump},

		{ NULL,                     0}
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(LineConnect, LineConnectTypes),
	                           "LineConnect",        0                 ));

	pComp->Value(mkNamingAdapt(LineIntersect,                 "LineIntersect",      0                 ));
	pComp->Value(mkNamingAdapt(Prey,                          "Prey",               0                 ));
	pComp->Value(mkNamingAdapt(Edible,                        "Edible",             0                 ));
	pComp->Value(mkNamingAdapt(CrewMember,                    "CrewMember",         0                 ));
	pComp->Value(mkNamingAdapt(NativeCrew,                    "NoStandardCrew",     0                 ));
	pComp->Value(mkNamingAdapt(Growth,                        "Growth",             0                 ));
	pComp->Value(mkNamingAdapt(Rebuyable,                     "Rebuy",              0                 ));
	pComp->Value(mkNamingAdapt(Constructable,                 "Construction",       0                 ));
	pComp->Value(mkNamingAdapt(BuildTurnTo,     "ConstructTo",        C4ID::None                  ));
	pComp->Value(mkNamingAdapt(Grab,                      "Grab",               0                 ));

	const StdBitfieldEntry<int32_t> GrabPutGetTypes[] =
	{

		{ "C4D_GrabGet"            ,C4D_Grab_Get},
		{ "C4D_GrabPut"            ,C4D_Grab_Put},

		{ NULL,                     0}
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(GrabPutGet, GrabPutGetTypes),
	                           "GrabPutGet",         0                 ));

	pComp->Value(mkNamingAdapt(Carryable,                     "Collectible",        0                 ));
	pComp->Value(mkNamingAdapt(Rotateable,                    "Rotate",             0                 ));
	pComp->Value(mkNamingAdapt(RotatedEntrance,               "RotatedEntrance",    0                 ));
	pComp->Value(mkNamingAdapt(Chopable,                      "Chop",               0                 ));
	pComp->Value(mkNamingAdapt(Float,                         "Float",              0                 ));
	pComp->Value(mkNamingAdapt(ContainBlast,                  "ContainBlast",       0                 ));
	pComp->Value(mkNamingAdapt(ColorByOwner,                  "ColorByOwner",       0                 ));
	pComp->Value(mkNamingAdapt(NoHorizontalMove,              "HorizontalFix",      0                 ));
	pComp->Value(mkNamingAdapt(BorderBound,                   "BorderBound",        0                 ));
	pComp->Value(mkNamingAdapt(LiftTop,                       "LiftTop",            0                 ));
	pComp->Value(mkNamingAdapt(UprightAttach,                 "UprightAttach",      0                 ));
	pComp->Value(mkNamingAdapt(GrowthType,                    "StretchGrowth",      0                 ));
	pComp->Value(mkNamingAdapt(NoBurnDecay,                   "NoBurnDecay",        0                 ));
	pComp->Value(mkNamingAdapt(IncompleteActivity,            "IncompleteActivity", 0                 ));
	pComp->Value(mkNamingAdapt(AttractLightning,              "AttractLightning",   0                 ));
	pComp->Value(mkNamingAdapt(Oversize,                      "Oversize",           0                 ));
	pComp->Value(mkNamingAdapt(Fragile,                       "Fragile",            0                 ));
	pComp->Value(mkNamingAdapt(Explosive,                     "Explosive",          0                 ));
	pComp->Value(mkNamingAdapt(Projectile,                    "Projectile",         0                 ));
	pComp->Value(mkNamingAdapt(NoPushEnter,                   "NoPushEnter",        0                 ));
	pComp->Value(mkNamingAdapt(DragImagePicture,              "DragImagePicture",   0                 ));
	pComp->Value(mkNamingAdapt(VehicleControl,                "VehicleControl",     0                 ));
	pComp->Value(mkNamingAdapt(Pathfinder,                    "Pathfinder",         0                 ));
	pComp->Value(mkNamingAdapt(MoveToRange,                   "MoveToRange",        0                 ));
	pComp->Value(mkNamingAdapt(NoComponentMass,               "NoComponentMass",    0                 ));
	pComp->Value(mkNamingAdapt(NoStabilize,                   "NoStabilize",        0                 ));
	pComp->Value(mkNamingAdapt(ClosedContainer,               "ClosedContainer",    0                 ));
	pComp->Value(mkNamingAdapt(SilentCommands,                "SilentCommands",     0                 ));
	pComp->Value(mkNamingAdapt(NoBurnDamage,                  "NoBurnDamage",       0                 ));
	pComp->Value(mkNamingAdapt(TemporaryCrew,                 "TemporaryCrew",      0                 ));
	pComp->Value(mkNamingAdapt(SmokeRate,                     "SmokeRate",          100               ));
	pComp->Value(mkNamingAdapt(BlitMode,                      "BlitMode",           C4D_Blit_Normal   ));
	pComp->Value(mkNamingAdapt(NoBreath,                      "NoBreath",           0                 ));
	pComp->Value(mkNamingAdapt(ConSizeOff,                    "ConSizeOff",         0                 ));
	pComp->Value(mkNamingAdapt(NoSell,                        "NoSell",             0                 ));
	pComp->Value(mkNamingAdapt(NoGet,                         "NoGet",              0                 ));
	pComp->Value(mkNamingAdapt(NoTransferZones,               "NoTransferZones",    0                 ));
	pComp->Value(mkNamingAdapt(AutoContextMenu,               "AutoContextMenu",    0                 ));
	pComp->Value(mkNamingAdapt(NeededGfxMode,                 "NeededGfxMode",      0                 ));

	const StdBitfieldEntry<int32_t> AllowPictureStackModes[] =
	{

		{ "APS_Color",      APS_Color    },
		{ "APS_Graphics",   APS_Graphics },
		{ "APS_Name",       APS_Name     },
		{ "APS_Overlay",    APS_Overlay  },
		{ NULL,             0            }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt<int32_t>(AllowPictureStack, AllowPictureStackModes),
	                           "AllowPictureStack",   0                ));
}

//-------------------------------- C4Def -------------------------------------------------------

C4Def::C4Def()
{
	Graphics.pDef = this;
	Default();
}

void C4Def::Default()
{
	DefaultDefCore();
	Next=NULL;
	Temporary=false;
	Filename[0]=0;
	Creation=0;
	Count=0;
	TimerCall=NULL;
	MainFace.Set(NULL,0,0,0,0);
	Script.Default();
	StringTable.Default();
	pClonkNames=NULL;
	pRankNames=NULL;
	pRankSymbols=NULL;
	fClonkNamesOwned = fRankNamesOwned = fRankSymbolsOwned = false;
	iNumRankSymbols=1;
	PortraitCount = 0;
	Portraits = NULL;
}

C4Def::~C4Def()
{
	Clear();
}

void C4Def::Clear()
{

	Graphics.Clear();

	Script.Clear();
	StringTable.Clear();
	if (pClonkNames && fClonkNamesOwned) delete pClonkNames; pClonkNames=NULL;
	if (pRankNames && fRankNamesOwned) delete pRankNames; pRankNames=NULL;
	if (pRankSymbols && fRankSymbolsOwned) delete pRankSymbols; pRankSymbols=NULL;
	fClonkNamesOwned = fRankNamesOwned = fRankSymbolsOwned = false;

	PortraitCount = 0;
	Portraits = NULL;
	C4PropList::Clear();
}

bool C4Def::Load(C4Group &hGroup,
                 DWORD dwLoadWhat,
                 const char *szLanguage,
                 C4SoundSystem *pSoundSystem)
{
	bool fSuccess=true;

	bool AddFileMonitoring = false;
	if (Game.pFileMonitor && !SEqual(hGroup.GetFullName().getData(),Filename) && !hGroup.IsPacked())
		AddFileMonitoring = true;

	// Store filename, maker, creation
	SCopy(hGroup.GetFullName().getData(),Filename);
	Creation = hGroup.GetCreation();

	// Verbose log filename
	if (Config.Graphics.VerboseObjectLoading>=3)
		Log(hGroup.GetFullName().getData());

	if (AddFileMonitoring) Game.pFileMonitor->AddDirectory(Filename);

	// particle def?
	if (hGroup.AccessEntry(C4CFN_ParticleCore))
	{
		// def loading not successful; abort after reading sounds
		fSuccess=false;
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


	// Read DefCore
	if (fSuccess) fSuccess=LoadDefCore(hGroup);

	// skip def: don't even read sounds!
	if (fSuccess && Game.C4S.Definitions.SkipDefs.GetIDCount(id, 1)) return false;

	if (!fSuccess)
	{

		// Read sounds even if not a valid def (for pure c4d sound folders)
		if (dwLoadWhat & C4D_Load_Sounds)
			if (pSoundSystem)
				pSoundSystem->LoadEffects(hGroup);

		return false;
	}

	// Read surface bitmap
	if (dwLoadWhat & C4D_Load_Bitmap)
		if (!Graphics.Load(hGroup, !!ColorByOwner))
		{
			DebugLogF("  Error loading graphics of %s (%s)", hGroup.GetFullName().getData(), id.ToString());
			return false;
		}

	// Read portraits
	if (dwLoadWhat & C4D_Load_Bitmap)
		if (!LoadPortraits(hGroup))
		{
			DebugLogF("  Error loading portrait graphics of %s (%s)", hGroup.GetFullName().getData(), id.ToString());
			return false;
		}

	// Read script
	if (dwLoadWhat & C4D_Load_Script)
	{
		// reg script to engine
		Script.Reg2List(&::ScriptEngine, &::ScriptEngine);
		// Load script - loads string table as well, because that must be done after script load
		// for downwards compatibility with packing order
		Script.Load("Script", hGroup, C4CFN_Script, szLanguage, this, &StringTable, true);
	}

	// read clonknames
	if (dwLoadWhat & C4D_Load_ClonkNames)
	{
		// clear any previous
		if (pClonkNames) delete pClonkNames; pClonkNames=NULL;
		if (hGroup.FindEntry(C4CFN_ClonkNameFiles))
		{
			// create new
			pClonkNames = new C4ComponentHost();
			if (!pClonkNames->LoadEx(LoadResStr("IDS_CNS_NAMES"), hGroup, C4CFN_ClonkNames, szLanguage))
			{
				delete pClonkNames; pClonkNames = NULL;
			}
			else
				fClonkNamesOwned = true;
		}
	}

	// read clonkranks
	if (dwLoadWhat & C4D_Load_RankNames)
	{
		// clear any previous
		if (pRankNames) delete pRankNames; pRankNames=NULL;
		if (hGroup.FindEntry(C4CFN_RankNameFiles))
		{
			// create new
			pRankNames = new C4RankSystem();
			// load from group
			if (!pRankNames->Load(hGroup, C4CFN_RankNames, 1000, szLanguage))
			{
				delete pRankNames; pRankNames=NULL;
			}
			else
				fRankNamesOwned = true;
		}
	}

	// read rankfaces
	if (dwLoadWhat & C4D_Load_RankFaces)
	{
		// clear any previous
		if (pRankSymbols) delete pRankSymbols; pRankSymbols=NULL;
		// load new: try png first
		if (hGroup.AccessEntry(C4CFN_RankFacesPNG))
		{
			pRankSymbols = new C4FacetSurface();
			if (!pRankSymbols->GetFace().ReadPNG(hGroup)) { delete pRankSymbols; pRankSymbols=NULL; }
		}
		else if (hGroup.AccessEntry(C4CFN_RankFaces))
		{
			pRankSymbols = new C4FacetSurface();
			if (!pRankSymbols->GetFace().ReadBMP(hGroup)) { delete pRankSymbols; pRankSymbols=NULL; }
		}
		// set size
		if (pRankSymbols)
		{
			pRankSymbols->Set(&pRankSymbols->GetFace(), 0,0, pRankSymbols->GetFace().Hgt,pRankSymbols->GetFace().Hgt);
			int32_t Q; pRankSymbols->GetPhaseNum(iNumRankSymbols, Q);
			if (!iNumRankSymbols) { delete pRankSymbols; pRankSymbols=NULL; }
			else
			{
				if (pRankNames)
				{
					// if extended rank names are defined, subtract those from the symbol count. The last symbols are used as overlay
					iNumRankSymbols = Max<int32_t>(1, iNumRankSymbols - pRankNames->GetExtendedRankNum());
				}
				fRankSymbolsOwned = true;
			}
		}
	}

	// Read sounds
	if (dwLoadWhat & C4D_Load_Sounds)
		if (pSoundSystem)
			pSoundSystem->LoadEffects(hGroup);

	// Bitmap post-load settings
	if (Graphics.GetBitmap())
	{
		// check SolidMask
		if (SolidMask.x<0 || SolidMask.y<0 || SolidMask.x+SolidMask.Wdt>Graphics.Bmp.Bitmap->Wdt || SolidMask.y+SolidMask.Hgt>Graphics.Bmp.Bitmap->Hgt) SolidMask.Default();
		// Set MainFace (unassigned bitmap: will be set by GetMainFace())
		MainFace.Set(NULL,0,0,Shape.Wdt,Shape.Hgt);
	}

	// validate TopFace
	if (TopFace.x<0 || TopFace.y<0 || TopFace.x+TopFace.Wdt>Graphics.Bmp.Bitmap->Wdt || TopFace.y+TopFace.Hgt>Graphics.Bmp.Bitmap->Hgt)
	{
		TopFace.Default();
		// warn in debug mode
		DebugLogF("invalid TopFace in %s(%s)", GetName(), id.ToString());
	}



	// Temporary flag
	if (dwLoadWhat & C4D_Load_Temporary) Temporary=true;

	if (Carryable) SetProperty(P_Collectible, C4VTrue);

	return true;
}

void C4Def::Draw(C4Facet &cgo, bool fSelected, DWORD iColor, C4Object *pObj, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform* trans)
{

	// default: def picture rect
	C4Rect fctPicRect = PictureRect;
	C4Facet fctPicture;

	// if assigned: use object specific rect and graphics
	if (pObj) if (pObj->PictureRect.Wdt) fctPicRect = pObj->PictureRect;

	if (fSelected)
		lpDDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, C4RGB(0xca, 0, 0));

	C4DefGraphics* graphics = pObj ? pObj->GetGraphics() : &Graphics;

	// specific object color?
	if (pObj) pObj->PrepareDrawing();

	switch (graphics->Type)
	{
	case C4DefGraphics::TYPE_Bitmap:
		fctPicture.Set(graphics->GetBitmap(iColor),fctPicRect.x,fctPicRect.y,fctPicRect.Wdt,fctPicRect.Hgt);
		fctPicture.DrawT(cgo,true,iPhaseX,iPhaseY,trans);
		break;
	case C4DefGraphics::TYPE_Mesh:
		// TODO: Allow rendering of a mesh directly, without instance (to render pose; no animation)
		std::auto_ptr<StdMeshInstance> dummy;
		StdMeshInstance* instance;

		C4Value value;
		if (pObj)
		{
			instance = pObj->pMeshInstance;
			pObj->GetProperty(P_PictureTransformation, &value);
		}
		else
		{
			dummy.reset(new StdMeshInstance(*graphics->Mesh));
			instance = dummy.get();
			GetProperty(P_PictureTransformation, &value);
		}

		StdMeshMatrix matrix;
		if (C4ValueToMatrix(value, &matrix))
			lpDDraw->SetMeshTransform(&matrix);

		lpDDraw->SetPerspective(true);
		lpDDraw->RenderMesh(*instance, cgo.Surface, cgo.X,cgo.Y, cgo.Wdt, cgo.Hgt, pObj ? pObj->Color : iColor, trans);
		lpDDraw->SetPerspective(false);
		lpDDraw->SetMeshTransform(NULL);

		break;
	}

	if (pObj) pObj->FinishedDrawing();

	// draw overlays
	if (pObj && pObj->pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pObj->pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrl->IsPicture())
				pGfxOvrl->DrawPicture(cgo, pObj, trans);
}

int32_t C4Def::GetValue(C4Object *pInBase, int32_t iBuyPlayer)
{
	// CalcDefValue defined?
	C4AulFunc *pCalcValueFn = Script.GetSFunc(PSF_CalcDefValue, AA_PROTECTED);
	int32_t iValue;
	if (pCalcValueFn)
		// then call it!
		iValue = pCalcValueFn->Exec(NULL, &C4AulParSet(C4VObj(pInBase), C4VInt(iBuyPlayer))).getInt();
	else
		// otherwise, use default value
		iValue = Value;
	// do any adjustments based on where the item is bought
	if (pInBase)
	{
		C4AulFunc *pFn;
		if ((pFn = pInBase->Def->Script.GetSFunc(PSF_CalcBuyValue, AA_PROTECTED)))
			iValue = pFn->Exec(pInBase, &C4AulParSet(C4VID(id), C4VInt(iValue))).getInt();
	}
	return iValue;
}

void C4Def::Synchronize()
{
}


//--------------------------------- C4DefList ----------------------------------------------

C4DefList::C4DefList()
{
	Default();
}

C4DefList::~C4DefList()
{
	Clear();
}

int32_t C4DefList::Load(C4Group &hGroup, DWORD dwLoadWhat,
                        const char *szLanguage,
                        C4SoundSystem *pSoundSystem,
                        bool fOverload,
                        bool fSearchMessage, int32_t iMinProgress, int32_t iMaxProgress, bool fLoadSysGroups)
{
	int32_t iResult=0;
	C4Def *nDef;
	char szEntryname[_MAX_FNAME+1];
	C4Group hChild;
	bool fPrimaryDef=false;
	bool fThisSearchMessage=false;

	// This search message
	if (fSearchMessage)
		if (SEqualNoCase(GetExtension(hGroup.GetName()),"c4d")
		    || SEqualNoCase(GetExtension(hGroup.GetName()),"c4s")
		    || SEqualNoCase(GetExtension(hGroup.GetName()),"c4f"))
		{
			fThisSearchMessage=true;
			fSearchMessage=false;
		}

	if (fThisSearchMessage) { LogF("%s...",GetFilename(hGroup.GetName())); }

	// Load primary definition
	if ((nDef=new C4Def))
	{
		if ( nDef->Load(hGroup,dwLoadWhat,szLanguage,pSoundSystem) && Add(nDef,fOverload) )
			{ iResult++; fPrimaryDef=true; }
		else
			{ delete nDef; }
	}

	// Load sub definitions
	int i = 0;
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_DefFiles,szEntryname))
		if (hChild.OpenAsChild(&hGroup,szEntryname))
		{
			// Hack: Assume that there are sixteen sub definitions to avoid unnecessary I/O
			int iSubMinProgress = Min<int32_t>(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * i) / 16);
			int iSubMaxProgress = Min<int32_t>(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * (i + 1)) / 16);
			++i;
			iResult += Load(hChild,dwLoadWhat,szLanguage,pSoundSystem,fOverload,fSearchMessage,iSubMinProgress,iSubMaxProgress);
			hChild.Close();
		}

	// load additional system scripts for def groups only
	C4Group SysGroup;
	char fn[_MAX_FNAME+1] = { 0 };
	if (!fPrimaryDef && fLoadSysGroups) if (SysGroup.OpenAsChild(&hGroup, C4CFN_System))
		{
			C4LangStringTable SysGroupString;
			SysGroupString.LoadEx("StringTbl", SysGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
			// load all scripts in there
			SysGroup.ResetSearch();
			while (SysGroup.FindNextEntry(C4CFN_ScriptFiles, (char *) &fn, NULL, NULL, !!fn[0]))
			{
				// host will be destroyed by script engine, so drop the references
				C4ScriptHost *scr = new C4ScriptHost();
				scr->Reg2List(&::ScriptEngine, &::ScriptEngine);
				scr->Load(NULL, SysGroup, fn, Config.General.LanguageEx, NULL, &SysGroupString);
			}
			// if it's a physical group: watch out for changes
			if (!SysGroup.IsPacked() && Game.pFileMonitor)
				Game.pFileMonitor->AddDirectory(SysGroup.GetFullName().getData());
			SysGroup.Close();
		}

	if (fThisSearchMessage) { LogF(LoadResStr("IDS_PRC_DEFSLOADED"),iResult); }

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

int32_t C4DefList::LoadFolderLocal( const char *szPath,
                                    DWORD dwLoadWhat, const char *szLanguage,
                                    C4SoundSystem *pSoundSystem,
                                    bool fOverload, char *sStoreName, int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iResult = 0;

	// Scan path for folder names
	int32_t cnt,iBackslash,iDefs;
	char szFoldername[_MAX_PATH+1];
	for (cnt=0; (iBackslash=SCharPos('\\',szPath,cnt)) > -1; cnt++)
	{
		SCopy(szPath,szFoldername,iBackslash);
		// Load from parent folder
		if (SEqualNoCase(GetExtension(szFoldername),"c4f"))
			if ((iDefs=Load(szFoldername,dwLoadWhat,szLanguage,pSoundSystem,fOverload)))
			{
				iResult+=iDefs;
				// Add any folder containing defs to store list
				if (sStoreName) { SNewSegment(sStoreName); SAppend(szFoldername,sStoreName); }
			}
	}

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

extern bool C4EngineLoadProcess(const char *szMessage, int32_t iProcess);

int32_t C4DefList::Load(const char *szSearch,
                        DWORD dwLoadWhat, const char *szLanguage,
                        C4SoundSystem *pSoundSystem,
                        bool fOverload, int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iResult=0;

	// Empty
	if (!szSearch[0]) return iResult;

	// Segments
	char szSegment[_MAX_PATH+1]; int32_t iGroupCount;
	if ((iGroupCount=SCharCount(';',szSearch)))
	{
		++iGroupCount; int32_t iPrg=iMaxProgress-iMinProgress;
		for (int32_t cseg=0; SCopySegment(szSearch,cseg,szSegment,';',_MAX_PATH); cseg++)
			iResult += Load(szSegment,dwLoadWhat,szLanguage,pSoundSystem,fOverload,
			                iMinProgress+iPrg*cseg/iGroupCount, iMinProgress+iPrg*(cseg+1)/iGroupCount);
		return iResult;
	}

	// Wildcard items
	if (SCharCount('*',szSearch))
	{
#ifdef _WIN32
		struct _finddata_t fdt; int32_t fdthnd;
		if ((fdthnd=_findfirst(szSearch,&fdt))<0) return false;
		do
		{
			iResult += Load(fdt.name,dwLoadWhat,szLanguage,pSoundSystem,fOverload);
		}
		while (_findnext(fdthnd,&fdt)==0);
		_findclose(fdthnd);
		// progress
		if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));
#else
		fputs("FIXME: C4DefList::Load\n", stderr);
#endif
		return iResult;
	}

	// File specified with creation (currently not used)
	char szCreation[25+1];
	int32_t iCreation=0;
	if (SCopyEnclosed(szSearch,'(',')',szCreation,25))
	{
		// Scan creation
		SClearFrontBack(szCreation);
		sscanf(szCreation,"%i",&iCreation);
		// Extract filename
		SCopyUntil(szSearch,szSegment,'(',_MAX_PATH);
		SClearFrontBack(szSegment);
		szSearch = szSegment;
	}

	// Load from specified file
	C4Group hGroup;
	if (!hGroup.Open(Config.AtDataReadPath(szSearch)))
	{
		// Specified file not found (failure)
		LogFatal(FormatString(LoadResStr("IDS_PRC_DEFNOTFOUND"),szSearch).getData());
		LoadFailure=true;
		return iResult;
	}
	iResult += Load(hGroup,dwLoadWhat,szLanguage,pSoundSystem,fOverload,true,iMinProgress,iMaxProgress);
	hGroup.Close();

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

bool C4DefList::Add(C4Def *pDef, bool fOverload)
{
	if (!pDef) return false;

	// Check old def to overload
	C4Def *pLastDef = ID2Def(pDef->id);
	if (pLastDef && !fOverload) return false;

	// Log overloaded def
	if (Config.Graphics.VerboseObjectLoading>=1)
		if (pLastDef)
		{
			LogF(LoadResStr("IDS_PRC_DEFOVERLOAD"),pDef->GetName(),pLastDef->id.ToString());
			if (Config.Graphics.VerboseObjectLoading >= 2)
			{
				LogF("      Old def at %s",pLastDef->Filename);
				LogF("     Overload by %s",pDef->Filename);
			}
		}

	// Remove old def
	Remove(pDef->id);

	// Add new def
	pDef->Next=FirstDef;
	FirstDef=pDef;

	return true;
}

bool C4DefList::Remove(C4ID id)
{
	C4Def *cdef,*prev;
	for (cdef=FirstDef,prev=NULL; cdef; prev=cdef,cdef=cdef->Next)
		if (cdef->id==id)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			return true;
		}
	return false;
}

void C4DefList::Remove(C4Def *def)
{
	C4Def *cdef,*prev;
	for (cdef=FirstDef,prev=NULL; cdef; prev=cdef,cdef=cdef->Next)
		if (cdef==def)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			return;
		}
}

void C4DefList::Clear()
{
	C4Def *cdef,*next;
	for (cdef=FirstDef; cdef; cdef=next)
	{
		next=cdef->Next;
		delete cdef;
	}
	FirstDef=NULL;
	// clear quick access table
	table.clear();
}

C4Def* C4DefList::ID2Def(C4ID id)
{
	if (id==C4ID::None) return NULL;
	if (table.empty())
	{
		// table not yet built: search list
		C4Def *cdef;
		for (cdef=FirstDef; cdef; cdef=cdef->Next)
			if (cdef->id==id) return cdef;
	}
	else
	{
		Table::const_iterator it = table.find(id);
		if (it != table.end())
			return it->second;
	}
	// none found
	return NULL;
}

int32_t C4DefList::GetIndex(C4ID id)
{
	C4Def *cdef;
	int32_t cindex;
	for (cdef=FirstDef,cindex=0; cdef; cdef=cdef->Next,cindex++)
		if (cdef->id==id) return cindex;
	return -1;
}

int32_t C4DefList::GetDefCount(DWORD dwCategory)
{
	C4Def *cdef; int32_t ccount=0;
	for (cdef=FirstDef; cdef; cdef=cdef->Next)
		if (cdef->Category & dwCategory)
			ccount++;
	return ccount;
}

C4Def* C4DefList::GetDef(int32_t iIndex, DWORD dwCategory)
{
	C4Def *pDef; int32_t iCurrentIndex;
	if (iIndex<0) return NULL;
	for (pDef=FirstDef,iCurrentIndex=-1; pDef; pDef=pDef->Next)
		if (pDef->Category & dwCategory)
		{
			iCurrentIndex++;
			if (iCurrentIndex==iIndex) return pDef;
		}
	return NULL;
}

C4Def *C4DefList::GetByPath(const char *szPath)
{
	// search defs
	const char *szDefPath;
	for (C4Def *pDef = FirstDef; pDef; pDef = pDef->Next)
		if ((szDefPath = Config.AtRelativePath(pDef->Filename)))
			if (SEqual2NoCase(szPath, szDefPath))
			{
				// the definition itself?
				if (!szPath[SLen(szDefPath)])
					return pDef;
				// or a component?
				else if (szPath[SLen(szDefPath)] == '\\')
					if (!strchr(szPath + SLen(szDefPath) + 1, '\\'))
						return pDef;
			}
	// not found
	return NULL;
}

int32_t C4DefList::RemoveTemporary()
{
	C4Def *cdef,*prev,*next;
	int32_t removed=0;
	for (cdef=FirstDef,prev=NULL; cdef; cdef=next)
	{
		next=cdef->Next;
		if (cdef->Temporary)
		{
			if (prev) prev->Next=next;
			else FirstDef=next;
			delete cdef;
			removed++;
		}
		else
			prev=cdef;
	}
	// rebuild quick access table
	BuildTable();
	return removed;
}

int32_t C4DefList::CheckEngineVersion(int32_t ver1, int32_t ver2, int32_t ver3, int32_t ver4)
{
	int32_t rcount=0;
	C4Def *cdef,*prev,*next;
	for (cdef=FirstDef,prev=NULL; cdef; cdef=next)
	{
		next=cdef->Next;
		if (CompareVersion(cdef->rC4XVer[0],cdef->rC4XVer[1],cdef->rC4XVer[2],cdef->rC4XVer[3],ver1,ver2,ver3,ver4) > 0)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			rcount++;
		}
		else prev=cdef;
	}
	return rcount;
}

int32_t C4DefList::CheckRequireDef()
{
	int32_t rcount=0, rcount2;
	C4Def *cdef,*prev,*next;
	do
	{
		rcount2 = rcount;
		for (cdef=FirstDef,prev=NULL; cdef; cdef=next)
		{
			next=cdef->Next;
			for (int32_t i = 0; i < cdef->RequireDef.GetNumberOfIDs(); i++)
				if (GetIndex(cdef->RequireDef.GetID(i)) < 0)
				{
					(prev ? prev->Next : FirstDef) = cdef->Next;
					delete cdef;
					rcount++;
				}
		}
	}
	while (rcount != rcount2);
	return rcount;
}

void C4DefList::Draw(C4ID id, C4Facet &cgo, bool fSelected, int32_t iColor)
{
	C4Def *cdef = ID2Def(id);
	if (cdef) cdef->Draw(cgo,fSelected,iColor);
}

void C4DefList::Default()
{
	FirstDef=NULL;
	LoadFailure=false;
	table.clear();
}

// Load scenario specified or all selected plus scenario & folder local

int32_t C4DefList::LoadForScenario(const char *szScenario,
                                   const char *szSelection,
                                   DWORD dwLoadWhat, const char *szLanguage,
                                   C4SoundSystem *pSoundSystem, bool fOverload,
                                   int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iDefs=0;
	StdStrBuf sSpecified;

	// User selected modules
	sSpecified.Copy(szSelection);

	// Open scenario file & load core
	C4Group hScenario;
	C4Scenario C4S;
	if ( !hScenario.Open(szScenario)
	     || !C4S.Load(hScenario) )
		return 0;

	// Scenario definition specifications (override user selection)
	if (!C4S.Definitions.AllowUserChange)
		C4S.Definitions.GetModules(&sSpecified);

	// Load specified
	iDefs += Load(sSpecified.getData(),dwLoadWhat,szLanguage,pSoundSystem,fOverload);
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress+iMinProgress)/2);

	// Load folder local
	iDefs += LoadFolderLocal(szScenario,dwLoadWhat,szLanguage,pSoundSystem,fOverload);
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress*3+iMinProgress)/4);

	// Load local
	iDefs += Load(hScenario,dwLoadWhat,szLanguage,pSoundSystem,fOverload);

	// build quick access table
	BuildTable();

	// progress
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	// Done
	return iDefs;
}

bool C4DefList::Reload(C4Def *pDef, DWORD dwLoadWhat, const char *szLanguage, C4SoundSystem *pSoundSystem)
{
	// Safety
	if (!pDef) return false;
	// backup graphics names and pointers
	// GfxBackup-dtor will ensure that upon loading-failure all graphics are reset to default
	C4DefGraphicsPtrBackup GfxBackup(&pDef->Graphics);
	// Clear def
	pDef->Clear(); // Assume filename is being kept
	// Reload def
	C4Group hGroup;
	if (!hGroup.Open(pDef->Filename)) return false;
	if (!pDef->Load( hGroup, dwLoadWhat, szLanguage, pSoundSystem )) return false;
	hGroup.Close();
	// rebuild quick access table
	BuildTable();
	// update script engine - this will also do include callbacks and Freeze() this
	::ScriptEngine.ReLink(this);
	// restore graphics
	GfxBackup.AssignUpdate(&pDef->Graphics);
	// Success
	return true;
}



bool C4Def::LoadPortraits(C4Group &hGroup)
{
	// reset any previous portraits
	Portraits = NULL; PortraitCount = 0;
	// search for portraits within def graphics
	for (C4DefGraphics *pGfx = &Graphics; pGfx; pGfx=pGfx->GetNext())
		if (pGfx->IsPortrait())
		{
			// assign first portrait
			if (!Portraits) Portraits = pGfx->IsPortrait();
			// count
			++PortraitCount;
		}
	return true;
}

C4ValueArray *C4Def::GetCustomComponents(C4Value *pvArrayHolder, C4Object *pBuilder, C4Object *pObjInstance)
{
	// return custom components array if script function is defined and returns an array
	if (Script.SFn_CustomComponents)
	{
		C4AulParSet pars(C4VObj(pBuilder));
		*pvArrayHolder = Script.SFn_CustomComponents->Exec(pObjInstance, &pars);
		return pvArrayHolder->getArray();
	}
	return NULL;
}

int32_t C4Def::GetComponentCount(C4ID idComponent, C4Object *pBuilder)
{
	// script overload?
	C4Value vArrayHolder;
	C4ValueArray *pArray = GetCustomComponents(&vArrayHolder, pBuilder);
	if (pArray)
	{
		int32_t iCount = 0;
		for (int32_t i=0; i<pArray->GetSize(); ++i)
			if (pArray->GetItem(i).getC4ID() == idComponent)
				++iCount;
		return iCount;
	}
	// no valid script overload: Assume definition components
	return Component.GetIDCount(idComponent);
}

C4ID C4Def::GetIndexedComponent(int32_t idx, C4Object *pBuilder)
{
	// script overload?
	C4Value vArrayHolder;
	C4ValueArray *pArray = GetCustomComponents(&vArrayHolder, pBuilder);
	if (pArray)
	{
		// assume that components are always returned ordered ([a, a, b], but not [a, b, a])
		if (!pArray->GetSize()) return C4ID::None;
		C4ID idLast = pArray->GetItem(0).getC4ID();
		if (!idx) return idLast;
		for (int32_t i=1; i<pArray->GetSize(); ++i)
		{
			C4ID idCurr = pArray->GetItem(i).getC4ID();
			if (idCurr != idLast)
			{
				if (!--idx) return (idCurr);
				idLast = idCurr;
			}
		}
		// index out of bounds
		return C4ID::None;
	}
	// no valid script overload: Assume definition components
	return Component.GetID(idx);
}

void C4Def::GetComponents(C4IDList *pOutList, C4Object *pObjInstance, C4Object *pBuilder)
{
	assert(pOutList);
	assert(!pOutList->GetNumberOfIDs());
	// script overload?
	C4Value vArrayHolder;
	C4ValueArray *pArray = GetCustomComponents(&vArrayHolder, pBuilder, pObjInstance);
	if (pArray)
	{
		// transform array into IDList
		// assume that components are always returned ordered ([a, a, b], but not [a, b, a])
		C4ID idLast; int32_t iCount = 0;
		for (int32_t i=0; i<pArray->GetSize(); ++i)
		{
			C4ID idCurr = pArray->GetItem(i).getC4ID();
			if (!idCurr) continue;
			if (i && idCurr != idLast)
			{
				pOutList->SetIDCount(idLast, iCount, true);
				iCount = 0;
			}
			idLast = idCurr;
			++iCount;
		}
		if (iCount) pOutList->SetIDCount(idLast, iCount, true);
	}
	else
	{
		// no valid script overload: Assume object or definition components
		if (pObjInstance)
			*pOutList = pObjInstance->Component;
		else
			*pOutList = Component;
	}
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
	if (!fClonkNamesOwned) pClonkNames = NULL;
	if (!fRankNamesOwned) pRankNames = NULL;
	if (!fRankSymbolsOwned) { pRankSymbols = NULL; iNumRankSymbols = 0; }
}

C4PropList *C4Def::GetActionByName(const char *actname)
{
	if (!actname) return NULL;
	C4String * actname_str = Strings.RegString(actname);
	actname_str->IncRef();
	C4PropList *r = GetActionByName(actname_str);
	actname_str->DecRef();
	return r;
}

C4PropList *C4Def::GetActionByName(C4String *actname)
{
	assert(actname);
	// If we get the null string or ActIdle by name, return NULL action
	if (!actname || actname == &Strings.P[P_Idle]) return NULL;
	// otherwise, query actmap
	C4Value ActMap; GetProperty(P_ActMap, &ActMap);
	if (!ActMap.getPropList()) return false;
	C4Value Action; ActMap.getPropList()->GetPropertyByS(actname, &Action);
	if (!Action.getPropList()) return false;
	return Action.getPropList();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// C4DefList

bool C4DefList::GetFontImage(const char *szImageTag, CFacet &rOutImgFacet)
{
	// extended: images by game
	C4FacetSurface fctOut;
	if (!Game.DrawTextSpecImage(fctOut, szImageTag)) return false;
	if (fctOut.Surface == &fctOut.GetFace()) return false; // cannot use facets that are drawn on the fly right now...
	rOutImgFacet.Set(fctOut.Surface, fctOut.X, fctOut.Y, fctOut.Wdt, fctOut.Hgt);
	// done, found
	return true;
}

void C4DefList::Synchronize()
{
	for (Table::iterator it = table.begin(); it != table.end(); ++it)
		it->second->Synchronize();
}

void C4DefList::ResetIncludeDependencies()
{
	for (Table::iterator it = table.begin(); it != table.end(); ++it)
		it->second->ResetIncludeDependencies();
}

void C4DefList::CallEveryDefinition()
{
	for (Table::iterator it = table.begin(); it != table.end(); ++it)
	{
		C4AulParSet Pars(C4VPropList(it->second));
		it->second->Script.Call(PSF_Definition, 0, &Pars, true);
		it->second->Freeze();
	}
}

void C4DefList::BuildTable()
{
	table.clear();
	for (C4Def *def = FirstDef; def; def = def->Next)
		table.insert(std::make_pair(def->id, def));
}
