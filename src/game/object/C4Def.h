/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2001  Matthes Bender
 * Copyright (c) 2001-2007  Sven Eberhardt
 * Copyright (c) 2003  Peter Wortmann
 * Copyright (c) 2006, 2009  Günther Brammer
 * Copyright (c) 2010  Maikel de Vries
 * Copyright (c) 2010  Nicolas Hake
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

#ifndef INC_C4Def
#define INC_C4Def

#include <C4Shape.h>
#include <C4InfoCore.h>
#include <C4IDList.h>
#include <C4ValueMap.h>
#include <C4Facet.h>
#include <C4Surface.h>
#include <C4ComponentHost.h>
#include <C4PropList.h>

#include <C4ScriptHost.h>
#include <C4DefGraphics.h>
#include "C4LangStringTable.h"
#include "C4InputValidation.h"

#include <functional>
#include <set>

const int32_t C4D_None           =    0,
C4D_All            =    ~C4D_None,

C4D_StaticBack     =    1<<0,
C4D_Structure      =    1<<1,
C4D_Vehicle        =    1<<2,
C4D_Living         =    1<<3,
C4D_Object         =    1<<4,

C4D_SortLimit = C4D_StaticBack | C4D_Structure | C4D_Vehicle | C4D_Living | C4D_Object,

C4D_Goal             =  1<<5,
C4D_Rule             =  1<<6,
C4D_Environment      =  1<<7,

C4D_Background       =  1<<12,
C4D_Parallax         =  1<<13,
C4D_MouseSelect      =  1<<14,
C4D_Foreground       =  1<<15,
C4D_MouseIgnore      =  1<<16,
C4D_IgnoreFoW        =  1<<17,

C4D_BackgroundOrForeground = (C4D_Background | C4D_Foreground);

const int32_t C4D_Grab_Put = 1,
C4D_Grab_Get = 2,

C4D_Border_Sides  = 1,
C4D_Border_Top    = 2,
C4D_Border_Bottom = 4,
C4D_Border_Layer  = 8,

C4D_Place_Surface    = 0,
C4D_Place_Liquid     = 1,
C4D_Place_Air        = 2;

const int32_t C4D_VehicleControl_None     = 0,
C4D_VehicleControl_Outside  = 1,
C4D_VehicleControl_Inside   = 2;

const DWORD C4D_Load_None      =  0,
C4D_Load_Picture   =  1,
C4D_Load_Bitmap    =  2,
C4D_Load_Script    =  4,

C4D_Load_Image     = 32,
C4D_Load_Sounds    = 64,
C4D_Load_ClonkNames= 128,
C4D_Load_RankNames = 256,
C4D_Load_RankFaces = 512,
C4D_Load_FE        = C4D_Load_Image,
C4D_Load_RX        = C4D_Load_Bitmap | C4D_Load_Script | C4D_Load_ClonkNames | C4D_Load_Sounds | C4D_Load_RankNames | C4D_Load_RankFaces,
C4D_Load_Temporary = 1024;

#define C4D_Blit_Normal     0
#define C4D_Blit_Additive   1
#define C4D_Blit_ModAdd     2

class C4Def: public C4PropList
{
public:
	C4ID id;
	int32_t rC4XVer[4];
	C4IDList RequireDef;
	C4Shape Shape;
	C4Rect Entrance;
	C4Rect Collection;
	C4Rect PictureRect;
	C4TargetRect SolidMask;
	C4TargetRect TopFace;
	C4IDList Component;
	C4ID BurnTurnTo;
	C4ID BuildTurnTo;
	int32_t GrowthType;
	int32_t CrewMember;
	int32_t NativeCrew;
	int32_t Mass;
	int32_t Value;
	int32_t Exclusive;
	int32_t Category;
	int32_t Growth;
	int32_t Rebuyable;
	int32_t ContactIncinerate; // 0 off 1 high - 5 low
	int32_t BlastIncinerate; // 0 off 1 - x if > damage
	int32_t Constructable;
	int32_t Grab; // 0 not 1 push 2 grab only
	int32_t Rotateable;
	int32_t Float;
	int32_t ColorByOwner;
	int32_t NoHorizontalMove;
	int32_t BorderBound;
	int32_t LiftTop;
	int32_t GrabPutGet;
	int32_t ContainBlast;
	int32_t UprightAttach;
	int32_t ContactFunctionCalls;
	int32_t Line;
	int32_t LineIntersect;
	int32_t NoBurnDecay;
	int32_t IncompleteActivity;
	int32_t Placement;
	int32_t Prey;
	int32_t Edible;
	int32_t AttractLightning;
	int32_t Oversize;
	int32_t Fragile;
	int32_t Projectile;
	int32_t Explosive;
	int32_t NoPushEnter;
	int32_t DragImagePicture;
	int32_t VehicleControl;
	int32_t Pathfinder;
	int32_t MoveToRange;
	int32_t Timer;
	int32_t NoComponentMass;
	int32_t NoStabilize;
	char STimerCall[C4AUL_MAX_Identifier];
	int32_t ClosedContainer;  // if set, contained objects are not damaged by lava/acid etc. 1: Contained objects can't view out; 2: They can
	int32_t SilentCommands;   // if set, no command failure messages are printed
	int32_t NoBurnDamage;     // if set, the object won't take damage when burning
	int32_t TemporaryCrew;    // if set, info objects are not written into regular player files
	int32_t SmokeRate;        // amount of smoke produced when on fire. 100 is default
	int32_t BlitMode;         // special blit mode for objects of this def. C4D_Blit_X
	int32_t NoBreath;         // object does not need to breath, although it's living
	int32_t ConSizeOff;       // number of pixels to be subtracted from the needed height for this building
	int32_t NoSell;           // if set, object can't be sold (doesn't even appear in sell-menu)
	int32_t NoGet;            // if set, object can't be taken out of a containers manually (doesn't appear in get/activate-menus)
	int32_t NeededGfxMode;    // if set, the def will only be loaded in given gfx mode
	int32_t RotatedEntrance;  // 0 entrance not rotateable, 1 entrance always, 2-360 entrance within this rotation
	int32_t NoTransferZones;
	int32_t AllowPictureStack; // allow stacking of multiple items in menus even if some attributes do not match. APS_*-values
public:
	void DefaultDefCore();
	bool LoadDefCore(C4Group &hGroup);
	bool Save(C4Group &hGroup);
	void CompileFunc(StdCompiler *pComp);
protected:
	bool Compile(const char *szSource, const char *szName);
	bool Decompile(StdStrBuf *pOut, const char *szName);


// Here begins the C4Def
	friend class C4DefList;
public:
	C4Def();
	~C4Def();
public:
	char Filename[_MAX_FNAME+1];
	int32_t Creation;
	int32_t Count; // number of instanciations
	C4AulScriptFunc *TimerCall;

	// Currently cannot have script host in frontend because that
	// would need C4Script, C4AulScript, and all that as well...
	C4DefScriptHost Script;
	C4LangStringTable StringTable;

	// clonknames are simply not needed in frontend
	C4ComponentHost *pClonkNames; bool fClonkNamesOwned;

	// neither are ranknames nor the symbols...yet!
	C4RankSystem *pRankNames; bool fRankNamesOwned;
	C4FacetSurface *pRankSymbols; bool fRankSymbolsOwned;
	int32_t iNumRankSymbols;    // number of rank symbols available, if loaded
	C4DefGraphics Graphics; // base graphics. points to additional graphics
	int32_t PortraitCount;
	C4PortraitGraphics *Portraits; // Portraits (linked list of C4AdditionalDefGraphics)

protected:
	C4Facet MainFace;

protected:
	C4Def *Next;
	bool Temporary;
public:
	void Clear();
	void Default();
	bool Load(C4Group &hGroup,
	          DWORD dwLoadWhat, const char *szLanguage,
	          class C4SoundSystem *pSoundSystem = NULL);
	void Draw(C4Facet &cgo, bool fSelected=false, DWORD iColor=0, C4Object *pObj=NULL, int32_t iPhaseX=0, int32_t iPhaseY=0,C4DrawTransform* trans=NULL);

	inline C4Facet &GetMainFace(C4DefGraphics *pGraphics, DWORD dwClr=0) { MainFace.Surface=pGraphics->GetBitmap(dwClr); return MainFace; }
	int32_t GetValue(C4Object *pInBase, int32_t iBuyPlayer);         // get value of def; calling script functions if defined
	void Synchronize();
	virtual C4Def const * GetDef() const { return this; }	
	virtual C4Def * GetDef() { return this; }
	virtual bool IsDef() const { return true; }
protected:
	bool LoadPortraits(C4Group &hGroup);
	bool LoadActMap(C4Group &hGroup);
	void CrossMapActMap();
private:
	C4ValueArray *GetCustomComponents(C4Value *pvArrayHolder, C4Object *pBuilder, C4Object *pObjInstance=NULL);
public:
	// return def components - may be overloaded by script callback
	int32_t GetComponentCount(C4ID idComponent, C4Object *pBuilder=NULL);
	C4ID GetIndexedComponent(int32_t idx, C4Object *pBuilder=NULL);
	void GetComponents(C4IDList *pOutList, C4Object *pObjInstance=NULL, C4Object *pBuilder=NULL);

	void IncludeDefinition(C4Def *pIncludeDef); // inherit components from other definition
	void ResetIncludeDependencies(); // resets all pointers into foreign definitions caused by include chains

	C4PropList *GetActionByName(const char *actname);
	C4PropList *GetActionByName(C4String *actname);
};

class C4DefList
		: public CStdFont::CustomImages
{
public:
	C4DefList();
	virtual ~C4DefList();
public:
	bool LoadFailure;
	typedef std::map<C4ID, C4Def*> Table;
	Table table;
protected:
	C4Def *FirstDef;
public:
	void Default();
	void Clear();
	int32_t Load(C4Group &hGroup,
	             DWORD dwLoadWhat, const char *szLanguage,
	             C4SoundSystem *pSoundSystem = NULL,
	             bool fOverload = false,
	             bool fSearchMessage = false, int32_t iMinProgress=0, int32_t iMaxProgress=0, bool fLoadSysGroups = true);
	int32_t Load(const char *szSearch,
	             DWORD dwLoadWhat, const char *szLanguage,
	             C4SoundSystem *pSoundSystem = NULL,
	             bool fOverload = false, int32_t iMinProgress=0, int32_t iMaxProgress=0);
	int32_t LoadFolderLocal(const char *szPath,
	                        DWORD dwLoadWhat, const char *szLanguage,
	                        C4SoundSystem *pSoundSystem = NULL,
	                        bool fOverload = false, char *szStoreName=NULL, int32_t iMinProgress=0, int32_t iMaxProgress=0);
	int32_t LoadForScenario(const char *szScenario,
	                        const char *szSpecified,
	                        DWORD dwLoadWhat, const char *szLanguage,
	                        C4SoundSystem *pSoundSystem = NULL,
	                        bool fOverload = false, int32_t iMinProgress=0, int32_t iMaxProgress=0);
	C4Def *ID2Def(C4ID id);
	C4Def *GetDef(int32_t Index, DWORD dwCategory = C4D_All);
	C4Def *GetByPath(const char *szPath);
	int32_t GetDefCount(DWORD dwCategory = C4D_All);
	int32_t GetIndex(C4ID id);
	int32_t RemoveTemporary();
	int32_t CheckEngineVersion(int32_t ver1, int32_t ver2, int32_t ver3, int32_t ver4);
	int32_t CheckRequireDef();
	void Draw(C4ID id, C4Facet &cgo, bool fSelected, int32_t iColor);
	void Remove(C4Def *def);
	bool Remove(C4ID id);
	bool Reload(C4Def *pDef, DWORD dwLoadWhat, const char *szLanguage, C4SoundSystem *pSoundSystem = NULL);
	bool Add(C4Def *ndef, bool fOverload);
	void BuildTable();
	void ResetIncludeDependencies(); // resets all pointers into foreign definitions caused by include chains
	void CallEveryDefinition();
	void Synchronize();

	// callback from font renderer: get ID image
	virtual bool GetFontImage(const char *szImageTag, CFacet &rOutImgFacet);
};

extern C4DefList Definitions;

inline C4Def *C4Id2Def(C4ID id)
{
	return ::Definitions.ID2Def(id);
}

#endif
