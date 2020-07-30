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

#ifndef INC_C4Scenario
#define INC_C4Scenario

#include "lib/C4NameList.h"
#include "object/C4IDList.h"

class C4SVal
{
public:
	C4SVal(int32_t std=0, int32_t rnd=0, int32_t min=0, int32_t max=100);
public:
	int32_t Std,Rnd,Min,Max;
public:
	void Default();
	void Set(int32_t std=0, int32_t rnd=0, int32_t min=0, int32_t max=100);
	void SetConstant(int32_t val);
	int32_t Evaluate();
	void CompileFunc(StdCompiler *pComp);
public:
	inline bool operator==(const C4SVal &rhs) const
	{
		return rhs.Std == Std && rhs.Rnd == Rnd && rhs.Min == Min && rhs.Max == Max;
	}
};

#define C4S_SECTIONLOAD 1 /* parts of the C4S that are modifyable for different landcape sections */

// flags for section reloading
#define C4S_SAVE_LANDSCAPE 1
#define C4S_SAVE_OBJECTS   2
#define C4S_KEEP_EFFECTS   4
#define C4S_REINIT_SCENARIO 8

enum C4SFilmMode
{
	C4SFilm_None      = 0,
	C4SFilm_Normal    = 1,
	C4SFilm_Cinematic = 2
};

class C4SHead
{
public:
	int32_t  C4XVer[2];
	std::string Title;
	std::string Loader;
	std::string Font; // scenario specific font; may be 0
	int32_t  Difficulty;
	int32_t  Icon;
	bool  NoInitialize;
	int32_t  MaxPlayer, MinPlayer, MaxPlayerLeague;
	bool  SaveGame;
	bool  Replay;
	int32_t  Film;
	int32_t  RandomSeed;
	std::string Engine; // Relative filename of engine to be used for this scenario
	std::string MissionAccess;
	bool Secret; // if true, scenario is invisible if MissionAccess has not been granted
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
	C4IDList SkipDefs;
	void SetModules(const char *szList, const char *szRelativeToPath=nullptr, const char *szRelativeToPath2=nullptr);
	bool GetModules(StdStrBuf *psOutModules) const;
	std::list<const char *> GetModulesAsList() const; // get definitions as string pointers into this structure
	void Default();
	void CompileFunc(StdCompiler *pComp);

private:
	char Definition[C4S_MaxDefinitions][_MAX_PATH_LEN];
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
	StdCopyStrBuf Mode; // Game mode used by league to determine correct evaluation
	C4IDList Goals;
	C4IDList Rules;

	bool FoWEnabled;

	C4SRealism Realism;

	bool EvaluateOnAbort;

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
	C4SVal Wealth;
	int32_t Position[2];
	int32_t EnforcePosition;
	C4IDList ReadyCrew;
	C4IDList ReadyBase;
	C4IDList ReadyVehic;
	C4IDList ReadyMaterial;
	C4IDList BuildKnowledge;
	C4IDList BaseMaterial;
	C4IDList BaseProduction;
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
	int32_t BottomOpen,TopOpen;
	int32_t LeftOpen,RightOpen;
	int32_t AutoScanSideOpen;
	std::string SkyDef;
	int32_t SkyDefFade[6];
	bool NoScan;
	C4SVal Gravity;
	// Dynamic map
	C4SVal MapWdt,MapHgt,MapZoom;
	C4SVal Amplitude,Phase,Period,Random;
	C4SVal LiquidLevel;
	int32_t MapPlayerExtend;
	C4NameList Layers;
	std::string Material;
	std::string Liquid;
	bool KeepMapCreator; // set if the mapcreator will be needed in the scenario (for DrawDefMap)
	int32_t SkyScrollMode;  // sky scrolling mode for newgfx
	int32_t MaterialZoom;
	bool FlatChunkShapes; // if true, all material chunks are drawn flat
	bool Secret; // hide map from observers (except in dev mode and the like)
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
	C4SVal Wind;
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
	bool Load(C4Group &hGroup, bool fLoadSection = false, bool suppress_errors = false);
	bool Save(C4Group &hGroup, bool fSaveSection=false);
	void CompileFunc(StdCompiler *pComp, bool fSection);
	int32_t GetMinPlayer(); // will try to determine the minimum player count for this scenario
};

extern const char *C4ScenSect_Main;

// ref to one scenario section
class C4ScenarioSection
{
public:
	C4ScenarioSection(const char *szName);  // ctor
	~C4ScenarioSection(); // dtor

public:
	StdCopyStrBuf name;          // section name
	StdCopyStrBuf temp_filename; // filename of data file if in temp dir
	StdCopyStrBuf filename;      // filename of section in scenario file
	bool fModified;       // if set, the file is temp and contains runtime landscape and/or object data
	class C4ScenarioObjectsScriptHost *pObjectScripts; // points to loaded script file for section Objects.c

	C4ScenarioSection *pNext; // next member of linked list

public:
	bool ScenarioLoad(const char *szFilename, bool is_temp_file);  // called when scenario is loaded: extract to temp store
	C4Group *GetGroupfile(C4Group &rGrp); // get group at section file (returns temp group, scenario subgroup or scenario group itself)
	bool EnsureTempStore(bool fExtractLandscape, bool fExtractObjects);               // make sure that a temp file is created, and nothing is modified within the main scenario file
};



#endif // INC_C4Scenario
