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

#ifndef INC_C4Def
#define INC_C4Def

#include "c4group/C4ComponentHost.h"
#include "c4group/C4LangStringTable.h"
#include "graphics/C4Facet.h"
#include "lib/C4InputValidation.h"
#include "object/C4DefGraphics.h"
#include "object/C4IDList.h"
#include "object/C4InfoCore.h"
#include "object/C4Shape.h"
#include "script/C4PropList.h"
#include "script/C4ScriptHost.h"
#include "script/C4ValueMap.h"

#include <functional>

const int32_t
C4D_None           =     0,
C4D_All            =    ~0,
C4D_StaticBack     =  1<<0,
C4D_Structure      =  1<<1,
C4D_Vehicle        =  1<<2,
C4D_Living         =  1<<3,
C4D_Object         =  1<<4,

C4D_Goal           =  1<<5,
C4D_Rule           =  1<<6,
C4D_Environment    =  1<<7,

C4D_Background     = 1<<12,
C4D_Parallax       = 1<<13,
C4D_MouseSelect    = 1<<14,
C4D_Foreground     = 1<<15,
C4D_MouseIgnore    = 1<<16,
C4D_IgnoreFoW      = 1<<17,

C4D_SortLimit      = C4D_StaticBack | C4D_Structure | C4D_Vehicle | C4D_Living | C4D_Object
                   | C4D_Background | C4D_Foreground;

const int32_t C4Plane_Structure = 200;

const int32_t C4D_Grab_Put = 1,
C4D_Grab_Get = 2,

C4D_Border_Sides  = 1,
C4D_Border_Top    = 2,
C4D_Border_Bottom = 4,
C4D_Border_Layer  = 8,

C4D_Place_Surface     = 0,
C4D_Place_Liquid      = 1,
C4D_Place_Air         = 2,
C4D_Place_Subsurface  = 3,
C4D_Place_BothSurface = 4;

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

class C4Def: public C4PropListStatic
{
public:
	C4ID id;
	int32_t rC4XVer[2];
	C4IDList RequireDef;
	C4Shape Shape;
	C4Rect Entrance;
	C4Rect Collection;
	C4Rect PictureRect;
	C4TargetRect SolidMask;
	C4TargetRect TopFace;
	C4ID BurnTurnTo;
	int32_t GrowthType;
	int32_t CrewMember;
	int32_t NativeCrew;
	int32_t Mass;
	int32_t Value;
	int32_t Exclusive;
	int32_t Category;
	int32_t Constructable;
	int32_t Rotateable;
	int32_t Float;
	int32_t ColorByOwner;
	int32_t NoHorizontalMove;
	int32_t LiftTop;
	int32_t GrabPutGet;
	int32_t UprightAttach;
	int32_t Line;
	int32_t LineIntersect;
	int32_t IncompleteActivity;
	int32_t Oversize;
	int32_t Fragile;
	int32_t Projectile;
	int32_t NoPushEnter;
	int32_t VehicleControl;
	int32_t Pathfinder;
	int32_t MoveToRange;
	int32_t NoMassFromContents;
	int32_t NoStabilize;
	int32_t ClosedContainer;  // if set, contained objects are not damaged by lava/acid etc. 1: Contained objects can't view out; 2: They can
	int32_t SilentCommands;   // if set, no command failure messages are printed
	int32_t TemporaryCrew;    // if set, info objects are not written into regular player files
	int32_t BlitMode;         // special blit mode for objects of this def. C4D_Blit_X
	int32_t NoBreath;         // object does not need to breath, although it's living
	int32_t ConSizeOff;       // number of pixels to be subtracted from the needed height for this building
	int32_t NoGet;            // if set, object can't be taken out of a containers manually (doesn't appear in get/activate-menus)
	int32_t RotatedEntrance;  // 0 entrance not rotateable, 1 entrance always, 2-360 entrance within this rotation
	int32_t NoTransferZones;
	int32_t AllowPictureStack; // allow stacking of multiple items in menus even if some attributes do not match. APS_*-values
	bool HideInCreator;       // If true, the definition is not visible in the object creator of the editor
public:
	void DefaultDefCore();
	bool LoadDefCore(C4Group &hGroup);
	bool Save(C4Group &hGroup);
	void CompileFunc(StdCompiler *pComp);
protected:
	bool Compile(const char *szSource, const char *szName);
	bool Decompile(StdStrBuf *pOut, const char *szName);
private:
	void LoadMeshMaterials(C4Group &hGroup, C4DefGraphicsPtrBackup *gfx_backup);
	bool LoadParticleDef(C4Group &hGroup);
	bool LoadSolidMask(C4Group &hGroup);
	bool LoadGraphics(C4Group &hGroup, StdMeshSkeletonLoader &loader);
	void LoadScript(C4Group &hGroup, const char* szLanguage);
	void LoadClonkNames(C4Group &hGroup, C4ComponentHost* pClonkNames, const char* szLanguage);
	void LoadRankNames(C4Group &hGroup, const char* szLanguage);
	void LoadRankFaces(C4Group &hGroup);
	void LoadSounds(C4Group &hGroup, C4SoundSystem* pSoundSystem);

	std::set<StdCopyStrBuf> mesh_materials;

// Here begins the C4Def
	friend class C4DefList;
public:
	C4Def();
	~C4Def() override;
public:
	char Filename[_MAX_FNAME_LEN];
	StdCopyStrBuf ConsoleGroupPath; // file path as used in the definition list viewer in the console. Only initialized in editor mode.
	int32_t Creation;
	int32_t Count; // number of instanciations

	C4DefScriptHost Script;
	C4LangStringTable StringTable;

	// clonknames are simply not needed in frontend
	C4ComponentHost *pClonkNames; bool fClonkNamesOwned;

	// neither are ranknames nor the symbols...yet!
	C4RankSystem *pRankNames; bool fRankNamesOwned;
	C4FacetSurface *pRankSymbols; bool fRankSymbolsOwned;
	int32_t iNumRankSymbols;    // number of rank symbols available, if loaded
	C4DefGraphics Graphics; // base graphics. points to additional graphics
	CSurface8 *pSolidMask; // SolidMask-bitmap. Nonzero pixels are solid.

protected:
	C4Facet MainFace;

protected:
	C4Def *Next;
	bool Temporary;
public:
	void Clear();
	void Default();
	bool Load(C4Group &hGroup,
		StdMeshSkeletonLoader &loader,
		DWORD dwLoadWhat, const char *szLanguage,
		class C4SoundSystem *pSoundSystem = nullptr,
		C4DefGraphicsPtrBackup *gfx_backup = nullptr);
	void Draw(C4Facet &cgo, bool fSelected=false, DWORD iColor=0, C4Object *pObj=nullptr, int32_t iPhaseX=0, int32_t iPhaseY=0, C4DrawTransform* trans=nullptr, const char * graphicsName=nullptr);

	inline C4Facet &GetMainFace(C4DefGraphics *pGraphics, DWORD dwClr=0) { MainFace.Surface=pGraphics->GetBitmap(dwClr); return MainFace; }
	int32_t GetPlane() { return GetPropertyInt(P_Plane); }
	int32_t GetValue(C4Object *pInBase, int32_t iBuyPlayer);         // get value of def; calling script functions if defined
	void Synchronize();
	C4Def const * GetDef() const override { return this; }	
	C4Def * GetDef() override { return this; }
	bool Delete() override { return false; }
protected:
	bool LoadActMap(C4Group &hGroup);
	void CrossMapActMap();
public:
	void IncludeDefinition(C4Def *pIncludeDef); // inherit components from other definition
	void ResetIncludeDependencies(); // resets all pointers into foreign definitions caused by include chains

	C4PropList *GetActionByName(const char *actname);
	C4PropList *GetActionByName(C4String *actname);
};

#endif
