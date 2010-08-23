/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2002, 2004-2008  Sven Eberhardt
 * Copyright (c) 2004-2005  Peter Wortmann
 * Copyright (c) 2006  Günther Brammer
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

/* Core component of a scenario file */

#include <C4Include.h>
#include <C4Scenario.h>
#include <C4InputValidation.h>

#include <C4Random.h>
#include <C4Group.h>
#include <C4Components.h>
#include <C4Game.h>

//==================================== C4SVal ==============================================

C4SVal::C4SVal(int32_t std, int32_t rnd, int32_t min, int32_t max)
		: Std(std), Rnd(rnd), Min(min), Max(max)
{
}

void C4SVal::Set(int32_t std, int32_t rnd, int32_t min, int32_t max)
{
	Std=std; Rnd=rnd; Min=min; Max=max;
}

int32_t C4SVal::Evaluate()
{
	return BoundBy(Std+Random(2*Rnd+1)-Rnd,Min,Max);
}

void C4SVal::Default()
{
	Set();
}

void C4SVal::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkDefaultAdapt(Std, 0));
	if (!pComp->Separator()) return;
	pComp->Value(mkDefaultAdapt(Rnd, 0));
	if (!pComp->Separator()) return;
	pComp->Value(mkDefaultAdapt(Min, 0));
	if (!pComp->Separator()) return;
	pComp->Value(mkDefaultAdapt(Max, 100));
}

//================================ C4Scenario ==========================================

C4Scenario::C4Scenario()
{
	Default();
}

void C4Scenario::Default()
{
	int32_t cnt;
	Head.Default();
	Definitions.Default();
	Game.Default();
	for (cnt=0; cnt<C4S_MaxPlayer; cnt++) PlrStart[cnt].Default();
	Landscape.Default();
	Animals.Default();
	Weather.Default();
	Game.Realism.Default();
	Environment.Default();
}

bool C4Scenario::Load(C4Group &hGroup, bool fLoadSection)
{
	char *pSource;
	// Load
	if (!hGroup.LoadEntry(C4CFN_ScenarioCore,&pSource,NULL,1)) return false;
	// Compile
	if (!Compile(pSource, fLoadSection))  { delete [] pSource; return false; }
	delete [] pSource;
	// Success
	return true;
}

bool C4Scenario::Save(C4Group &hGroup, bool fSaveSection)
{
	char *Buffer; int32_t BufferSize;
	if (!Decompile(&Buffer,&BufferSize, fSaveSection))
		return false;
	if (!hGroup.Add(C4CFN_ScenarioCore,Buffer,BufferSize,false,true))
		{ StdBuf Buf; Buf.Take(Buffer, BufferSize); return false; }
	return true;
}

void C4Scenario::CompileFunc(StdCompiler *pComp, bool fSection)
{
	pComp->Value(mkNamingAdapt(mkParAdapt(Head, fSection), "Head"));
	if (!fSection) pComp->Value(mkNamingAdapt(Definitions, "Definitions"));
	pComp->Value(mkNamingAdapt(mkParAdapt(Game, fSection), "Game"));
	for (int32_t i = 0; i < C4S_MaxPlayer; i++)
		pComp->Value(mkNamingAdapt(PlrStart[i], FormatString("Player%d", i+1).getData()));
	pComp->Value(mkNamingAdapt(Landscape, "Landscape"));
	pComp->Value(mkNamingAdapt(Animals, "Animals"));
	pComp->Value(mkNamingAdapt(Weather, "Weather"));
	pComp->Value(mkNamingAdapt(Environment, "Environment"));
}

int32_t C4Scenario::GetMinPlayer()
{
	// MinPlayer is specified.
	if (Head.MinPlayer != 0)
		return Head.MinPlayer;
	// Melee? Need at least two.
	if (Game.IsMelee())
		return 2;
	// Otherwise/unknown: need at least one.
	return 1;
}

void C4SDefinitions::Default()
{
	LocalOnly=AllowUserChange=false;
	ZeroMem(Definition,sizeof (Definition));
	SkipDefs.Default();
}

const int32_t C4S_MaxPlayerDefault = 12;

void C4SHead::Default()
{
	Origin.Clear();
	Icon=18;
	*Title = *Loader = *Font = *Engine = *MissionAccess = '\0';
	C4XVer[0] = C4XVer[1] = C4XVer[2] = C4XVer[3] = 0;
	Difficulty = StartupPlayerCount = RandomSeed = 0;
	SaveGame = Replay = NoInitialize = false;
	Film = ForcedFairCrew = FairCrewStrength = 0;
	NetworkGame = NetworkRuntimeJoin = false;

	MaxPlayer=MaxPlayerLeague=C4S_MaxPlayerDefault;
	MinPlayer=0; // auto-determine by mode
	SCopy("Default Title",Title,C4MaxTitle);
}

void C4SHead::CompileFunc(StdCompiler *pComp, bool fSection)
{
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(Icon,                      "Icon",                 18));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Title),    "Title",                "Default Title"));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Loader),   "Loader",               ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Font),     "Font",                 ""));
		pComp->Value(mkNamingAdapt(mkArrayAdaptDM(C4XVer,0),  "Version"               ));
		pComp->Value(mkNamingAdapt(Difficulty,                "Difficulty",           0));
		pComp->Value(mkNamingAdapt(MaxPlayer,                 "MaxPlayer",            C4S_MaxPlayerDefault));
		pComp->Value(mkNamingAdapt(MaxPlayerLeague,           "MaxPlayerLeague",      MaxPlayer));
		pComp->Value(mkNamingAdapt(MinPlayer,                 "MinPlayer",            0));
		pComp->Value(mkNamingAdapt(SaveGame,                  "SaveGame",             false));
		pComp->Value(mkNamingAdapt(Replay,                    "Replay",               false));
		pComp->Value(mkNamingAdapt(Film,                      "Film",                 0));
		pComp->Value(mkNamingAdapt(StartupPlayerCount,        "StartupPlayerCount",   0));
	}
	pComp->Value(mkNamingAdapt(NoInitialize,              "NoInitialize",         false));
	pComp->Value(mkNamingAdapt(RandomSeed,                "RandomSeed",           0));
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Engine),   "Engine",               ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(MissionAccess), "MissionAccess", ""));
		pComp->Value(mkNamingAdapt(NetworkGame,               "NetworkGame",          false));
		pComp->Value(mkNamingAdapt(NetworkRuntimeJoin,        "NetworkRuntimeJoin",   false));
		pComp->Value(mkNamingAdapt(ForcedFairCrew,            "ForcedFairCrew",          0));
		pComp->Value(mkNamingAdapt(FairCrewStrength,          "FairCrewStrength",       0));
		pComp->Value(mkNamingAdapt(mkStrValAdapt(mkParAdapt(Origin, StdCompiler::RCT_All), C4InVal::VAL_SubPathFilename),  "Origin",  StdCopyStrBuf()));
		// windows needs backslashes in Origin; other systems use forward slashes
		if (pComp->isCompiler()) Origin.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
	}
}

void C4SGame::Default()
{
	Goals.Clear();
	Rules.Clear();
	FoWColor=0;
}

void C4SGame::CompileFunc(StdCompiler *pComp, bool fSection)
{
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(Realism.ValueOverloads,            "ValueOverloads",      C4IDList()));
	}
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapePushPull),         "LandscapePushPull",   false));
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapeInsertThrust),     "LandscapeInsertThrust",true));

	pComp->Value(mkNamingAdapt(Goals,                    "Goals",               C4IDList()));
	pComp->Value(mkNamingAdapt(Rules,                    "Rules",               C4IDList()));
	pComp->Value(mkNamingAdapt(FoWColor,                 "FoWColor",            0u));
}

void C4SPlrStart::Default()
{
	NativeCrew=C4ID::None;
	Crew.Set(1,0,1,10);
	Wealth.Set(0,0,0,250);
	Position[0]=Position[1]=-1;
	EnforcePosition=0;
	ReadyCrew.Default();
	ReadyBase.Default();
	ReadyVehic.Default();
	ReadyMaterial.Default();
	BuildKnowledge.Default();
	HomeBaseMaterial.Default();
	HomeBaseProduction.Default();
	Magic.Default();
}

bool C4SPlrStart::EquipmentEqual(C4SPlrStart &rhs)
{
	return *this == rhs;
}

bool C4SPlrStart::operator==(const C4SPlrStart& rhs)
{
	return (NativeCrew==rhs.NativeCrew)
	       && (Crew == rhs.Crew)
	       && (Wealth == rhs.Wealth)
	       && (ReadyCrew == rhs.ReadyCrew)
	       && (ReadyBase == rhs.ReadyBase)
	       && (ReadyVehic == rhs.ReadyVehic)
	       && (ReadyMaterial == rhs.ReadyMaterial)
	       && (BuildKnowledge == rhs.BuildKnowledge)
	       && (HomeBaseMaterial == rhs.HomeBaseMaterial)
	       && (HomeBaseProduction == rhs.HomeBaseProduction)
	       && (Magic == rhs.Magic);
}

void C4SPlrStart::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(NativeCrew, "StandardCrew",          C4ID::None));
	pComp->Value(mkNamingAdapt(Crew,                    "Clonks",                C4SVal(1, 0, 1, 10), true));
	pComp->Value(mkNamingAdapt(Wealth,                  "Wealth",                C4SVal(0, 0, 0,250), true));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Position,-1), "Position"           ));
	pComp->Value(mkNamingAdapt(EnforcePosition,         "EnforcePosition",       0));
	pComp->Value(mkNamingAdapt(ReadyCrew,               "Crew",                  C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyBase,               "Buildings",             C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyVehic,              "Vehicles",              C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyMaterial,           "Material",              C4IDList()));
	pComp->Value(mkNamingAdapt(BuildKnowledge,          "Knowledge",             C4IDList()));
	pComp->Value(mkNamingAdapt(HomeBaseMaterial,        "HomeBaseMaterial",      C4IDList()));
	pComp->Value(mkNamingAdapt(HomeBaseProduction,      "HomeBaseProduction",    C4IDList()));
	pComp->Value(mkNamingAdapt(Magic,                   "Magic",                 C4IDList()));
}

void C4SLandscape::Default()
{
	BottomOpen=0; TopOpen=1;
	LeftOpen=0; RightOpen=0;
	AutoScanSideOpen=1;
	SkyDef[0]=0;
	NoSky=0;
	for (int32_t cnt=0; cnt<6; cnt++) SkyDefFade[cnt]=0;
	VegLevel.Set(50,30,0,100);
	Vegetation.Default();
	InEarthLevel.Set(50,0,0,100);
	InEarth.Default();
	MapWdt.Set(100,0,64,250);
	MapHgt.Set(50,0,40,250);
	MapZoom.Set(10,0,5,15);
	Amplitude.Set(0,0);
	Phase.Set(50);
	Period.Set(15);
	Random.Set(0);
	LiquidLevel.Default();
	MapPlayerExtend=0;
	Layers.Clear();
	SCopy("Earth",Material,C4M_MaxName);
	SCopy("Water",Liquid,C4M_MaxName);
	ExactLandscape=0;
	Gravity.Set(100,0,10,200);
	NoScan=0;
	KeepMapCreator=0;
	SkyScrollMode=0;
	NewStyleLandscape=0;
	FoWRes=CClrModAddMap::DefResolutionX;
}

void C4SLandscape::GetMapSize(int32_t &rWdt, int32_t &rHgt, int32_t iPlayerNum)
{
	rWdt = MapWdt.Evaluate();
	rHgt = MapHgt.Evaluate();
	iPlayerNum = Max<int32_t>( iPlayerNum, 1 );
	if (MapPlayerExtend)
		rWdt = Min(rWdt * Min(iPlayerNum, C4S_MaxMapPlayerExtend), MapWdt.Max);
}

void C4SLandscape::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ExactLandscape,          "ExactLandscape",        false));
	pComp->Value(mkNamingAdapt(Vegetation,              "Vegetation",            C4IDList()));
	pComp->Value(mkNamingAdapt(VegLevel,                "VegetationLevel",       C4SVal(50,30,0,100), true));
	pComp->Value(mkNamingAdapt(InEarth,                 "InEarth",               C4IDList()));
	pComp->Value(mkNamingAdapt(InEarthLevel,            "InEarthLevel",          C4SVal(50,0,0,100), true));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(SkyDef), "Sky",                   ""));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(SkyDefFade,0),"SkyFade"            ));
	pComp->Value(mkNamingAdapt(NoSky,                   "NoSky",                 false));
	pComp->Value(mkNamingAdapt(BottomOpen,              "BottomOpen",            false));
	pComp->Value(mkNamingAdapt(TopOpen,                 "TopOpen",               true));
	pComp->Value(mkNamingAdapt(LeftOpen,                "LeftOpen",              0));
	pComp->Value(mkNamingAdapt(RightOpen,               "RightOpen",             0));
	pComp->Value(mkNamingAdapt(AutoScanSideOpen,        "AutoScanSideOpen",      true));
	pComp->Value(mkNamingAdapt(MapWdt,                  "MapWidth",              C4SVal(100,0,64,250), true));
	pComp->Value(mkNamingAdapt(MapHgt,                  "MapHeight",             C4SVal(50,0,40,250), true));
	pComp->Value(mkNamingAdapt(MapZoom,                 "MapZoom",               C4SVal(10,0,5,15), true));
	pComp->Value(mkNamingAdapt(Amplitude,               "Amplitude",             C4SVal(0)));
	pComp->Value(mkNamingAdapt(Phase,                   "Phase",                 C4SVal(50)));
	pComp->Value(mkNamingAdapt(Period,                  "Period",                C4SVal(15)));
	pComp->Value(mkNamingAdapt(Random,                  "Random",                C4SVal(0)));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Material),"Material",             "Earth"));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Liquid), "Liquid",                "Water"));
	pComp->Value(mkNamingAdapt(LiquidLevel,             "LiquidLevel",           C4SVal()));
	pComp->Value(mkNamingAdapt(MapPlayerExtend,         "MapPlayerExtend",       0));
	pComp->Value(mkNamingAdapt(Layers,                  "Layers",                C4NameList()));
	pComp->Value(mkNamingAdapt(Gravity,                 "Gravity",               C4SVal(100,0,10,200), true));
	pComp->Value(mkNamingAdapt(NoScan,                  "NoScan",                false));
	pComp->Value(mkNamingAdapt(KeepMapCreator,          "KeepMapCreator",        false));
	pComp->Value(mkNamingAdapt(SkyScrollMode,           "SkyScrollMode",         0));
	pComp->Value(mkNamingAdapt(NewStyleLandscape,       "NewStyleLandscape",     0));
	pComp->Value(mkNamingAdapt(FoWRes,                  "FoWRes",                static_cast<int32_t>(CClrModAddMap::DefResolutionX)));
}

void C4SWeather::Default()
{
	Climate.Set(50,10);
	StartSeason.Set(50,50);
	YearSpeed.Set(50);
	Rain.Default(); Wind.Set(0,70,-100,+100);
	SCopy("Water",Precipitation,C4M_MaxName);
	NoGamma=1;
}

void C4SWeather::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Climate,                 "Climate",               C4SVal(50,10), true));
	pComp->Value(mkNamingAdapt(StartSeason,             "StartSeason",           C4SVal(50,50), true));
	pComp->Value(mkNamingAdapt(YearSpeed,               "YearSpeed",               C4SVal(50)));
	pComp->Value(mkNamingAdapt(Rain,                    "Rain",                  C4SVal()));
	pComp->Value(mkNamingAdapt(Wind,                    "Wind",                  C4SVal(0,70,-100,+100), true));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Precipitation),"Precipitation",   "Water"));
	pComp->Value(mkNamingAdapt(NoGamma,                 "NoGamma",               true));
}

void C4SAnimals::Default()
{
	FreeLife.Clear();
	EarthNest.Clear();
}

void C4SAnimals::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(FreeLife,                "Animal",               C4IDList()));
	pComp->Value(mkNamingAdapt(EarthNest,               "Nest",                  C4IDList()));
}

void C4SEnvironment::Default()
{
	Objects.Clear();
}

void C4SEnvironment::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Objects,                 "Objects",               C4IDList()));
}

void C4SRealism::Default()
{
	LandscapePushPull=0;
	LandscapeInsertThrust=0;
	ValueOverloads.Default();
}

bool C4Scenario::Compile(const char *szSource, bool fLoadSection)
{
	if (!fLoadSection) Default();
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(*this, fLoadSection), StdStrBuf(szSource), C4CFN_ScenarioCore);
}

bool C4Scenario::Decompile(char **ppOutput, int32_t *ipSize, bool fSaveSection)
{
	try
	{
		// Decompile
		StdStrBuf Buf = DecompileToBuf<StdCompilerINIWrite>(mkParAdapt(*this, fSaveSection));
		// Return
		*ppOutput = Buf.GrabPointer();
		*ipSize = Buf.getSize();
	}
	catch (StdCompiler::Exception *)
		{ return false; }
	return true;
}

void C4Scenario::Clear()
{

}

void C4Scenario::SetExactLandscape()
{
	if (Landscape.ExactLandscape) return;
	//int32_t iMapZoom = Landscape.MapZoom.Std;
	// Set landscape
	Landscape.ExactLandscape = 1;
	/*FIXME: warum ist das auskommentiert?
	// - because Map and Landscape are handled differently in NET2 (Map.bmp vs Landscape.bmp), and the zoomed Map.bmp may be used
	//   to reconstruct the textures on the Landscape.bmp in case of e.g. runtime joins. In this sense, C4S.Landscape.ExactLandscape
	//   only marks that the landscape.bmp is an exact one, and there may or may not be an accompanying Map.bmp
	Landscape.MapZoom.Set(1,0,1,1);
	// Zoom player starting positions
	for (int32_t cnt=0; cnt<C4S_MaxPlayer; cnt++)
	  {
	  if (PlrStart[cnt].PositionX >= -1)
	    PlrStart[cnt].PositionX = PlrStart[cnt].PositionX * iMapZoom;
	  if (PlrStart[cnt].PositionY >= -1)
	    PlrStart[cnt].PositionY = PlrStart[cnt].PositionY * iMapZoom;
	  }
	  */
}

bool C4SDefinitions::GetModules(StdStrBuf *psOutModules) const
{
	// Local only
	if (LocalOnly) { psOutModules->Copy(""); return true; }
	// Scan for any valid entries
	bool fSpecified = false;
	int32_t cnt=0;
	for (; cnt<C4S_MaxDefinitions; cnt++)
		if (Definition[cnt][0])
			fSpecified = true;
	// No valid entries
	if (!fSpecified) return false;
	// Compose entry list
	psOutModules->Copy("");
	for (cnt=0; cnt<C4S_MaxDefinitions; cnt++)
		if (Definition[cnt][0])
		{
			if (psOutModules->getLength()) psOutModules->AppendChar(';');
			psOutModules->Append(Definition[cnt]);
		}
	// Done
	return true;
}


void C4SDefinitions::SetModules(const char *szList, const char *szRelativeToPath, const char *szRelativeToPath2)
{
	int32_t cnt;

	// Empty list: local only
	if (!SModuleCount(szList))
	{
		LocalOnly=true;
		for (cnt=0; cnt<C4S_MaxDefinitions; cnt++) Definition[cnt][0]=0;
		return;
	}

	// Set list
	LocalOnly=false;
	for (cnt=0; cnt<C4S_MaxDefinitions; cnt++)
	{
		SGetModule(szList,cnt,Definition[cnt],_MAX_PATH);
		// Make relative path
		if (szRelativeToPath && *szRelativeToPath)
			if (SEqualNoCase(Definition[cnt],szRelativeToPath,SLen(szRelativeToPath)))
				SCopy(Definition[cnt]+SLen(szRelativeToPath),Definition[cnt]);
		if (szRelativeToPath2 && *szRelativeToPath2)
			if (SEqualNoCase(Definition[cnt],szRelativeToPath2,SLen(szRelativeToPath2)))
				SCopy(Definition[cnt]+SLen(szRelativeToPath2),Definition[cnt]);
	}

}

bool C4SDefinitions::AssertModules(const char *szPath, char *sMissing)
{
	// Local only
	if (LocalOnly) return true;

	// Check all listed modules for availability
	bool fAllAvailable=true;
	char szModule[_MAX_PATH+1];
	if (sMissing) sMissing[0]=0;
	// Check all definition files
	for (int32_t cnt=0; cnt<C4S_MaxDefinitions; cnt++)
		if (Definition[cnt][0])
		{
			// Compose filename using path specified by caller
			szModule[0]=0;
			if (szPath) SCopy(szPath,szModule); if (szModule[0]) AppendBackslash(szModule);
			SAppend(Definition[cnt],szModule);
			// Missing
			if (!C4Group_IsGroup(szModule))
			{
				// Add to list
				if (sMissing) { SNewSegment(sMissing,", "); SAppend(Definition[cnt],sMissing); }
				fAllAvailable=false;
			}
		}

	return fAllAvailable;
}

void C4SDefinitions::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(LocalOnly,               "LocalOnly",             false));
	pComp->Value(mkNamingAdapt(AllowUserChange,         "AllowUserChange",       false));
	for (int32_t i = 0; i < C4S_MaxDefinitions; i++)
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Definition[i]), FormatString("Definition%i", i+1).getData(), ""));
	pComp->Value(mkNamingAdapt(SkipDefs,                "SkipDefs",              C4IDList()));
}

bool C4SGame::IsMelee()
{
	return (Goals.GetIDCount(C4ID::Melee) || Goals.GetIDCount(C4ID::TeamworkMelee));
}

// scenario sections

const char *C4ScenSect_Main = "main";

C4ScenarioSection::C4ScenarioSection(char *szName)
{
	// copy name
	if (szName && !SEqualNoCase(szName, C4ScenSect_Main) && *szName)
	{
		this->szName = new char[strlen(szName)+1];
		SCopy(szName, this->szName);
	}
	else
		this->szName = const_cast<char *>(C4ScenSect_Main);
	// zero fields
	szTempFilename = szFilename = 0;
	fModified = false;
	// link into main list
	pNext = Game.pScenarioSections;
	Game.pScenarioSections = this;
}

C4ScenarioSection::~C4ScenarioSection()
{
	// del following scenario sections
	while (pNext)
	{
		C4ScenarioSection *pDel = pNext;
		pNext = pNext->pNext;
		pDel->pNext = NULL;
		delete pDel;
	}
	// del temp file
	if (szTempFilename)
	{
		EraseItem(szTempFilename);
		delete szTempFilename;
	}
	// del filename if assigned
	if (szFilename) delete szFilename;
	// del name if owned
	if (szName != C4ScenSect_Main) delete szName;
}

bool C4ScenarioSection::ScenarioLoad(char *szFilename)
{
	// safety
	if (this->szFilename || !szFilename) return false;
	// store name
	this->szFilename = new char[strlen(szFilename)+1];
	SCopy(szFilename, this->szFilename, _MAX_FNAME);
	// extract if it's not an open folder
	if (Game.ScenarioFile.IsPacked()) if (!EnsureTempStore(true, true)) return false;
	// donce, success
	return true;
}

C4Group *C4ScenarioSection::GetGroupfile(C4Group &rGrp)
{
	// check temp filename
	if (szTempFilename)
	{
		if (rGrp.Open(szTempFilename)) return &rGrp;
		else return NULL;
	}
	// check filename within scenario
	if (szFilename)
	{
		if (rGrp.OpenAsChild(&Game.ScenarioFile, szFilename)) return &rGrp;
		else return NULL;
	}
	// unmodified main section: return main group
	if (SEqualNoCase(szName, C4ScenSect_Main)) return &Game.ScenarioFile;
	// failure
	return NULL;
}

bool C4ScenarioSection::EnsureTempStore(bool fExtractLandscape, bool fExtractObjects)
{
	// if it's temp store already, don't do anything
	if (szTempFilename) return true;
	// make temp filename
	char *szTmp = const_cast<char *>(Config.AtTempPath(szFilename ? GetFilename(szFilename) : szName));
	MakeTempFilename(szTmp);
	// main section: extract section files from main scenario group (create group as open dir)
	if (!szFilename)
	{
		if (!CreatePath(szTmp)) return false;
		C4Group hGroup;
		if (!hGroup.Open(szTmp, true)) { EraseItem(szTmp); return false; }
		// extract all desired section files
		Game.ScenarioFile.ResetSearch();
		char fn[_MAX_FNAME+1]; *fn=0;
		while (Game.ScenarioFile.FindNextEntry(C4FLS_Section, fn))
			if (fExtractLandscape || !WildcardMatch(C4FLS_SectionLandscape, fn))
				if (fExtractObjects || !WildcardMatch(C4FLS_SectionObjects, fn))
					Game.ScenarioFile.ExtractEntry(fn, szTmp);
		hGroup.Close();
	}
	else
	{
		// subsection: simply extract section from main group
		if (!Game.ScenarioFile.ExtractEntry(szFilename, szTmp)) return false;
		// delete undesired landscape/object files
		if (!fExtractLandscape || !fExtractObjects)
		{
			C4Group hGroup;
			if (hGroup.Open(szFilename))
			{
				if (!fExtractLandscape) hGroup.Delete(C4FLS_SectionLandscape);
				if (!fExtractObjects) hGroup.Delete(C4FLS_SectionObjects);
			}
		}
	}
	// copy temp filename
	szTempFilename = new char[strlen(szTmp)+1];
	SCopy(szTmp, szTempFilename, _MAX_PATH);
	// done, success
	return true;
}

