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

/* Core component of a scenario file */

#include "C4Include.h"
#include "landscape/C4Scenario.h"

#include "c4group/C4Components.h"
#include "c4group/C4Group.h"
#include "lib/C4InputValidation.h"
#include "lib/C4Random.h"
#include "lib/StdColors.h"

//==================================== C4SVal ==============================================

C4SVal::C4SVal(int32_t std, int32_t rnd, int32_t min, int32_t max)
		: Std(std), Rnd(rnd), Min(min), Max(max)
{
}

void C4SVal::Set(int32_t std, int32_t rnd, int32_t min, int32_t max)
{
	Std=std; Rnd=rnd; Min=min; Max=max;
}

void C4SVal::SetConstant(int32_t val)
{
	// Set to constant value and ensure limits allow it
	Std = val;
	Rnd = 0;
	Min = std::min<int32_t>(Min, val);
	Max = std::max<int32_t>(Max, val);
}

int32_t C4SVal::Evaluate()
{
	return Clamp<int32_t>(Std+Random(2*Rnd+1)-Rnd,Min,Max);
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

bool C4Scenario::Load(C4Group &hGroup, bool fLoadSection, bool suppress_errors)
{
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(C4CFN_ScenarioCore,&Buf)) return false;
	if (!fLoadSection) Default();
	if (suppress_errors)
	{
		if (!CompileFromBuf_Log<StdCompilerINIRead>(mkParAdapt(*this, fLoadSection), Buf, C4CFN_ScenarioCore))
		{
			return false;
		}
	}
	else
	{
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(*this, fLoadSection), Buf, C4CFN_ScenarioCore))
		{
			return false;
		}
	}
	return true;
}

bool C4Scenario::Save(C4Group &hGroup, bool fSaveSection)
{
	StdStrBuf Buf;
	try
	{
		Buf.Take(DecompileToBuf<StdCompilerINIWrite>(mkParAdapt(*this, fSaveSection)));
	}
	catch (StdCompiler::Exception *)
		{ return false; }
	if (!hGroup.Add(C4CFN_ScenarioCore,Buf,false,true))
		{ return false; }
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
	Title.clear();
	Loader.clear();
	Font.clear();
	Engine.clear();
	MissionAccess.clear();
	Secret = false;
	C4XVer[0] = C4XVer[1] = 0;
	Difficulty = RandomSeed = 0;
	SaveGame = Replay = NoInitialize = false;
	Film = 0;
	NetworkGame = NetworkRuntimeJoin = false;

	MaxPlayer=MaxPlayerLeague=C4S_MaxPlayerDefault;
	MinPlayer=0; // auto-determine by mode
	Title = "Default Title";
}

void C4SHead::CompileFunc(StdCompiler *pComp, bool fSection)
{
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(Icon,                      "Icon",                 18));
		pComp->Value(mkNamingAdapt(mkStringAdaptA(Title),     "Title",                "Default Title"));
		pComp->Value(mkNamingAdapt(mkStringAdaptA(Loader),    "Loader",               ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptA(Font),      "Font",                 ""));
		pComp->Value(mkNamingAdapt(mkArrayAdaptDM(C4XVer,0),  "Version"               ));
		pComp->Value(mkNamingAdapt(Difficulty,                "Difficulty",           0));
		pComp->Value(mkNamingAdapt(MaxPlayer,                 "MaxPlayer",            C4S_MaxPlayerDefault));
		pComp->Value(mkNamingAdapt(MaxPlayerLeague,           "MaxPlayerLeague",      MaxPlayer));
		pComp->Value(mkNamingAdapt(MinPlayer,                 "MinPlayer",            0));
		pComp->Value(mkNamingAdapt(SaveGame,                  "SaveGame",             false));
		pComp->Value(mkNamingAdapt(Replay,                    "Replay",               false));
		pComp->Value(mkNamingAdapt(Film,                      "Film",                 0));
	}
	pComp->Value(mkNamingAdapt(NoInitialize,              "NoInitialize",         false));
	pComp->Value(mkNamingAdapt(RandomSeed,                "RandomSeed",           0));
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(mkStringAdaptA(Engine),    "Engine",               ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptA(MissionAccess),"MissionAccess",     ""));
		pComp->Value(mkNamingAdapt(Secret,                    "Secret",               false));
		pComp->Value(mkNamingAdapt(NetworkGame,               "NetworkGame",          false));
		pComp->Value(mkNamingAdapt(NetworkRuntimeJoin,        "NetworkRuntimeJoin",   false));
		pComp->Value(mkNamingAdapt(mkStrValAdapt(mkParAdapt(Origin, StdCompiler::RCT_All), C4InVal::VAL_SubPathFilename),  "Origin",  StdCopyStrBuf()));
		// windows needs backslashes in Origin; other systems use forward slashes
		if (pComp->isDeserializer())
		{
			Origin.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
		}
	}
}

void C4SGame::Default()
{
	Goals.Clear();
	Rules.Clear();
	FoWEnabled = true;
	EvaluateOnAbort = false;
}

void C4SGame::CompileFunc(StdCompiler *pComp, bool fSection)
{
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(Realism.ValueOverloads,            "ValueOverloads",      C4IDList()));
	}
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapePushPull),         "LandscapePushPull",   false));
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapeInsertThrust),     "LandscapeInsertThrust",true));

	pComp->Value(mkNamingAdapt(mkParAdapt(Mode, StdCompiler::RCT_IdtfAllowEmpty), "Mode",            StdCopyStrBuf()));
	pComp->Value(mkNamingAdapt(Goals,                                             "Goals",           C4IDList()));
	pComp->Value(mkNamingAdapt(Rules,                                             "Rules",           C4IDList()));
	pComp->Value(mkNamingAdapt(FoWEnabled,                                        "FoWEnabled",      true));
	pComp->Value(mkNamingAdapt(EvaluateOnAbort,                                   "EvaluateOnAbort", false));
}

void C4SPlrStart::Default()
{
	Wealth.Set(0,0,0,250);
	Position[0]=Position[1]=-1;
	EnforcePosition=0;
	ReadyCrew.Default();
	ReadyBase.Default();
	ReadyVehic.Default();
	ReadyMaterial.Default();
	BuildKnowledge.Default();
	BaseMaterial.Default();
	BaseProduction.Default();
}

bool C4SPlrStart::EquipmentEqual(C4SPlrStart &rhs)
{
	return *this == rhs;
}

bool C4SPlrStart::operator==(const C4SPlrStart& rhs)
{
	return (Wealth == rhs.Wealth)
	       && (ReadyCrew == rhs.ReadyCrew)
	       && (ReadyBase == rhs.ReadyBase)
	       && (ReadyVehic == rhs.ReadyVehic)
	       && (ReadyMaterial == rhs.ReadyMaterial)
	       && (BuildKnowledge == rhs.BuildKnowledge)
	       && (BaseMaterial == rhs.BaseMaterial)
	       && (BaseProduction == rhs.BaseProduction);
}

void C4SPlrStart::CompileFunc(StdCompiler *pComp)
{
	C4IDList crewDefault;
	crewDefault.SetIDCount(C4ID::Clonk,1,true);
	pComp->Value(mkNamingAdapt(Wealth,                  "Wealth",                C4SVal(0, 0, 0,250), true));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Position,-1), "Position"           ));
	pComp->Value(mkNamingAdapt(EnforcePosition,         "EnforcePosition",       0));
	pComp->Value(mkNamingAdapt(ReadyCrew,               "Crew",                  crewDefault));
	pComp->Value(mkNamingAdapt(ReadyBase,               "Buildings",             C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyVehic,              "Vehicles",              C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyMaterial,           "Material",              C4IDList()));
	pComp->Value(mkNamingAdapt(BuildKnowledge,          "Knowledge",             C4IDList()));
	pComp->Value(mkNamingAdapt(BaseMaterial,        "BaseMaterial",      C4IDList()));
	pComp->Value(mkNamingAdapt(BaseProduction,      "BaseProduction",    C4IDList()));
}

void C4SLandscape::Default()
{
	BottomOpen=0; TopOpen=1;
	LeftOpen=0; RightOpen=0;
	AutoScanSideOpen=1;
	SkyDef[0]=0;
	for (int & cnt : SkyDefFade) cnt=0;
	VegLevel.Set(50,30,0,100);
	Vegetation.Default();
	InEarthLevel.Set(50,0,0,100);
	InEarth.Default();
	MapWdt.Set(100,0,64,250);
	MapHgt.Set(50,0,40,250);
	MapZoom.Set(8,0,1,15);
	Amplitude.Set(0,0);
	Phase.Set(50);
	Period.Set(15);
	Random.Set(0);
	LiquidLevel.Default();
	MapPlayerExtend=0;
	Layers.Clear();
	Material = "Earth";
	Liquid = "Water";
	ExactLandscape=false;
	Gravity.Set(100,0,10,200);
	NoScan=false;
	KeepMapCreator=false;
	SkyScrollMode=0;
	MaterialZoom=4;
	FlatChunkShapes=false;
	Secret=false;
}

void C4SLandscape::GetMapSize(int32_t &rWdt, int32_t &rHgt, int32_t iPlayerNum)
{
	rWdt = MapWdt.Evaluate();
	rHgt = MapHgt.Evaluate();
	iPlayerNum = std::max<int32_t>( iPlayerNum, 1 );
	if (MapPlayerExtend)
		rWdt = std::min(rWdt * std::min(iPlayerNum, C4S_MaxMapPlayerExtend), MapWdt.Max);
}

void C4SLandscape::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ExactLandscape,          "ExactLandscape",        false));
	pComp->Value(mkNamingAdapt(Vegetation,              "Vegetation",            C4IDList()));
	pComp->Value(mkNamingAdapt(VegLevel,                "VegetationLevel",       C4SVal(50,30,0,100), true));
	pComp->Value(mkNamingAdapt(InEarth,                 "InEarth",               C4IDList()));
	pComp->Value(mkNamingAdapt(InEarthLevel,            "InEarthLevel",          C4SVal(50,0,0,100), true));
	pComp->Value(mkNamingAdapt(mkStringAdaptA(SkyDef),  "Sky",                   ""));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(SkyDefFade,0),"SkyFade"            ));
	pComp->Value(mkNamingAdapt(BottomOpen,              "BottomOpen",            0));
	pComp->Value(mkNamingAdapt(TopOpen,                 "TopOpen",               1));
	pComp->Value(mkNamingAdapt(LeftOpen,                "LeftOpen",              0));
	pComp->Value(mkNamingAdapt(RightOpen,               "RightOpen",             0));
	pComp->Value(mkNamingAdapt(AutoScanSideOpen,        "AutoScanSideOpen",      1));
	pComp->Value(mkNamingAdapt(MapWdt,                  "MapWidth",              C4SVal(100,0,64,250), true));
	pComp->Value(mkNamingAdapt(MapHgt,                  "MapHeight",             C4SVal(50,0,40,250), true));
	pComp->Value(mkNamingAdapt(MapZoom,                 "MapZoom",               C4SVal(8,0,1,15), true));
	pComp->Value(mkNamingAdapt(Amplitude,               "Amplitude",             C4SVal(0)));
	pComp->Value(mkNamingAdapt(Phase,                   "Phase",                 C4SVal(50)));
	pComp->Value(mkNamingAdapt(Period,                  "Period",                C4SVal(15)));
	pComp->Value(mkNamingAdapt(Random,                  "Random",                C4SVal(0)));
	pComp->Value(mkNamingAdapt(mkStringAdaptA(Material),"Material",              "Earth"));
	pComp->Value(mkNamingAdapt(mkStringAdaptA(Liquid),  "Liquid",                "Water"));
	pComp->Value(mkNamingAdapt(LiquidLevel,             "LiquidLevel",           C4SVal()));
	pComp->Value(mkNamingAdapt(MapPlayerExtend,         "MapPlayerExtend",       0));
	pComp->Value(mkNamingAdapt(Layers,                  "Layers",                C4NameList()));
	pComp->Value(mkNamingAdapt(Gravity,                 "Gravity",               C4SVal(100,0,10,200), true));
	pComp->Value(mkNamingAdapt(NoScan,                  "NoScan",                false));
	pComp->Value(mkNamingAdapt(KeepMapCreator,          "KeepMapCreator",        false));
	pComp->Value(mkNamingAdapt(SkyScrollMode,           "SkyScrollMode",         0));
	pComp->Value(mkNamingAdapt(MaterialZoom,            "MaterialZoom",          4));
	pComp->Value(mkNamingAdapt(FlatChunkShapes,         "FlatChunkShapes",       false));
	pComp->Value(mkNamingAdapt(Secret,                  "Secret",                false));
}

void C4SWeather::Default()
{
	Climate.Set(50,10);
	StartSeason.Set(50,50);
	YearSpeed.Set(50);
	Wind.Set(0,70,-100,+100);
	NoGamma=true;
}

void C4SWeather::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Climate,                 "Climate",               C4SVal(50,10), true));
	pComp->Value(mkNamingAdapt(StartSeason,             "StartSeason",           C4SVal(50,50), true));
	pComp->Value(mkNamingAdapt(YearSpeed,               "YearSpeed",               C4SVal(50)));
	pComp->Value(mkNamingAdapt(Wind,                    "Wind",                  C4SVal(0,70,-100,+100), true));
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
	LandscapePushPull=false;
	LandscapeInsertThrust=false;
	ValueOverloads.Default();
}

void C4Scenario::Clear()
{

}

void C4Scenario::SetExactLandscape()
{
	if (Landscape.ExactLandscape) return;
	// Set landscape
	Landscape.ExactLandscape = true;
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

std::list<const char *> C4SDefinitions::GetModulesAsList() const
{
	// get definitions as string pointers into this structure
	std::list<const char *> result;
	if (!LocalOnly)
	{
		for (const char *def : Definition)
		{
			if (*def)
			{
				result.push_back(def);
			}
		}
	}
	return result;
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
		{
			if (GetRelativePathS(Definition[cnt],szRelativeToPath) != Definition[cnt])
			{
				SCopy(GetRelativePathS(Definition[cnt],szRelativeToPath),Definition[cnt]);
				continue;
			}
		}
		if (szRelativeToPath2 && *szRelativeToPath2)
		{
			if (GetRelativePathS(Definition[cnt],szRelativeToPath2) != Definition[cnt])
			{
				SCopy(GetRelativePathS(Definition[cnt],szRelativeToPath2),Definition[cnt]);
				continue;
			}
		}
	}

}

void C4SDefinitions::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(LocalOnly,               "LocalOnly",             false));
	pComp->Value(mkNamingAdapt(AllowUserChange,         "AllowUserChange",       false));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Definition[0]), "Definition1", "Objects.ocd"));
	for (int32_t i = 1; i < C4S_MaxDefinitions; i++)
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Definition[i]), FormatString("Definition%i", i+1).getData(), ""));
	pComp->Value(mkNamingAdapt(SkipDefs,                "SkipDefs",              C4IDList()));
}

bool C4SGame::IsMelee()
{
	// Check for game modes known to be melees
	// Also allow it in parkours by default because that works fine
	if (Mode == "Melee" || Mode == "Parkour") return true;
	// Game mode not present or unknown?
	return false;
}
