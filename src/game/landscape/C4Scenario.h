/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001-2002, 2004-2007  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
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

#ifndef INC_C4Scenario
#define INC_C4Scenario

#include <C4NameList.h>
#include <C4IDList.h>

class C4Group;

class C4SVal
{
public:
	C4SVal(int32_t std=0, int32_t rnd=0, int32_t min=0, int32_t max=100);
public:
	int32_t Std,Rnd,Min,Max;
public:
	void Default();
	void Set(int32_t std=0, int32_t rnd=0, int32_t min=0, int32_t max=100);
	int32_t Evaluate();
	void CompileFunc(StdCompiler *pComp);
public:
	inline bool operator==(const C4SVal &rhs) const
	{
		return rhs.Std == Std && rhs.Rnd == Rnd && rhs.Min == Min && rhs.Max == Max;
	}
};

#define C4SGFXMODE_NEWGFX 1
#define C4SGFXMODE_OLDGFX 2

#define C4S_SECTIONLOAD 1 /* parts of the C4S that are modifyable for different landcape sections */

// flags for section reloading
#define C4S_SAVE_LANDSCAPE 1
#define C4S_SAVE_OBJECTS   2
#define C4S_KEEP_EFFECTS   4

enum C4SFilmMode
{
	C4SFilm_None      = 0,
	C4SFilm_Normal    = 1,
	C4SFilm_Cinematic = 2
};

class C4SHead
{
public:
	int32_t  C4XVer[4];
	char Title[C4MaxTitle+1];
	char Loader[C4MaxTitle+1];
	char Font[C4MaxTitle+1]; // scenario specific font; may be 0
	int32_t  Difficulty;
	int32_t  Icon;
	bool  NoInitialize;
	int32_t  MaxPlayer, MinPlayer, MaxPlayerLeague;
	bool  SaveGame;
	bool  Replay;
	int32_t  Film;
	int32_t  StartupPlayerCount; // set for Frame0-replay!
	int32_t  RandomSeed;
	char Engine[C4MaxTitle+1]; // Relative filename of engine to be used for this scenario
	char MissionAccess[C4MaxTitle+1];
	bool NetworkGame;
	bool NetworkRuntimeJoin;
	StdCopyStrBuf Origin; // original oath and filename to scenario (for records and savegames)
public:
	void Default();
	void CompileFunc(StdCompiler *pComp, bool fSection);
};


const int32_t C4S_MaxDefinitions = 10;

class C4SDefinitions
{
public:
	bool LocalOnly;
	bool AllowUserChange;
	char Definition[C4S_MaxDefinitions][_MAX_PATH+1];
	C4IDList SkipDefs;
public:
	void SetModules(const char *szList, const char *szRelativeToPath=NULL, const char *szRelativeToPath2=NULL);
	bool GetModules(StdStrBuf *psOutModules) const;
	bool AssertModules(const char *szPath=NULL, char *sMissing=NULL);
	void Default();
	void CompileFunc(StdCompiler *pComp);
};


class C4SRealism
{
public:
	C4IDList ValueOverloads;
	bool LandscapePushPull; // Use new experimental push-pull-algorithms
	bool LandscapeInsertThrust; // Inserted material may thrust material of lower density aside

public:
	void Default();
};


class C4SGame
{
public:
	C4IDList Goals;
	C4IDList Rules;

	uint32_t FoWColor;    // color of FoW; may contain transparency

	C4SRealism Realism;

public:
	bool IsMelee();
	void Default();
	void CompileFunc(StdCompiler *pComp, bool fSection);
};

// Maximum map player extend factor

const int32_t C4S_MaxMapPlayerExtend = 4;

class C4SPlrStart
{
public:
	C4ID NativeCrew; // Obsolete
	C4SVal Crew; // Obsolete
	C4SVal Wealth;
	int32_t Position[2];
	int32_t EnforcePosition;
	C4IDList ReadyCrew;
	C4IDList ReadyBase;
	C4IDList ReadyVehic;
	C4IDList ReadyMaterial;
	C4IDList BuildKnowledge;
	C4IDList HomeBaseMaterial;
	C4IDList HomeBaseProduction;
	C4IDList Magic;
public:
	void Default();
	bool EquipmentEqual(C4SPlrStart &rhs);
	bool operator==(const C4SPlrStart& rhs);
	void CompileFunc(StdCompiler *pComp);
};


class C4SLandscape
{
public:
	bool ExactLandscape;
	C4SVal VegLevel;
	C4IDList Vegetation;
	C4SVal InEarthLevel;
	C4IDList InEarth;
	bool BottomOpen,TopOpen;
	int32_t LeftOpen,RightOpen;
	bool AutoScanSideOpen;
	char SkyDef[C4MaxDefString+1];
	int32_t SkyDefFade[6];
	bool NoSky;
	bool NoScan;
	C4SVal Gravity;
	// Dynamic map
	C4SVal MapWdt,MapHgt,MapZoom;
	C4SVal Amplitude,Phase,Period,Random;
	C4SVal LiquidLevel;
	int32_t MapPlayerExtend;
	C4NameList Layers;
	char Material[C4M_MaxDefName+1];
	char Liquid[C4M_MaxDefName+1];
	bool KeepMapCreator; // set if the mapcreator will be needed in the scenario (for DrawDefMap)
	int32_t SkyScrollMode;  // sky scrolling mode for newgfx
	int32_t NewStyleLandscape; // if set to 2, the landscape uses up to 125 mat/texture pairs
	int32_t FoWRes; // chunk size of FoGOfWar
public:
	void Default();
	void GetMapSize(int32_t &rWdt, int32_t &rHgt, int32_t iPlayerNum);
	void CompileFunc(StdCompiler *pComp);
};

class C4SWeather
{
public:
	C4SVal Climate;
	C4SVal StartSeason,YearSpeed;
	C4SVal Rain,Wind;
	char Precipitation[C4M_MaxName+1];
	bool NoGamma;
public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4SAnimals
{
public:
	C4IDList FreeLife;
	C4IDList EarthNest;
public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4SEnvironment
{
public:
	C4IDList Objects;
public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4Scenario
{
public:
	C4Scenario();
public:
	C4SHead         Head;
	C4SDefinitions  Definitions;
	C4SGame         Game;
	C4SPlrStart     PlrStart[C4S_MaxPlayer];
	C4SLandscape    Landscape;
	C4SAnimals      Animals;
	C4SWeather      Weather;
	C4SEnvironment  Environment;
public:
	void SetExactLandscape();
	void Clear();
	void Default();
	bool Load(C4Group &hGroup, bool fLoadSection=false);
	bool Save(C4Group &hGroup, bool fSaveSection=false);
	void CompileFunc(StdCompiler *pComp, bool fSection);
	int32_t GetMinPlayer(); // will try to determine the minimum player count for this scenario
protected:
	bool Compile(const char *szSource, bool fLoadSection=false);
	bool Decompile(char **ppOutput, int32_t *ipSize, bool fSaveSection=false);
};

class C4ScenarioSection;


extern const char *C4ScenSect_Main;

// ref to one scenario section
class C4ScenarioSection
{
public:
	C4ScenarioSection(char *szName);  // ctor
	~C4ScenarioSection(); // dtor

public:
	char *szName;         // section name
	char *szTempFilename; // filename of data file if in temp dir
	char *szFilename;     // filename of section in scenario file
	bool fModified;       // if set, the file is temp and contains runtime landscape and/or object data

	C4ScenarioSection *pNext; // next member of linked list

public:
	bool ScenarioLoad(char *szFilename);  // called when scenario is loaded: extract to temp store
	C4Group *GetGroupfile(C4Group &rGrp); // get group at section file (returns temp group, scenario subgroup or scenario group itself)
	bool EnsureTempStore(bool fExtractLandscape, bool fExtractObjects);               // make sure that a temp file is created, and nothing is modified within the main scenario file
};


#endif // INC_C4Scenario
