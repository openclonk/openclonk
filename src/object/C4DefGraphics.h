/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// graphics used by object definitions (object and portraits)

#ifndef INC_C4DefGraphics
#define INC_C4DefGraphics

#include "graphics/C4Facet.h"
#include "object/C4ObjectPtr.h"
#include "lib/C4InputValidation.h"
#include "lib/StdMeshUpdate.h"
#include "object/C4Id.h"

// defintion graphics
class C4AdditionalDefGraphics;
class C4DefGraphicsPtrBackup;

class C4DefGraphics
{
public:
	C4Def *pDef;                    // underlying definition

protected:
	C4AdditionalDefGraphics *pNext; // next graphics

	C4DefGraphics *GetLast(); // get last graphics in list
public:
	enum GraphicsType
	{
		TYPE_None,
		TYPE_Bitmap,
		TYPE_Mesh
	};

	GraphicsType Type;

	union
	{
		struct
		{
			C4Surface *Bitmap, *BitmapClr, *BitmapNormal;
		} Bmp;
		StdMesh *Mesh;
	};

	bool fColorBitmapAutoCreated;  // if set, the color-by-owner-bitmap has been created automatically by all blue shades of the bitmap

	C4Surface *GetBitmap(DWORD dwClr=0); 

	C4DefGraphics(C4Def *pOwnDef=nullptr);  // ctor
	virtual ~C4DefGraphics() { Clear(); }; // dtor

	bool LoadBitmap(C4Group &hGroup, const char *szFilenamePNG, const char *szOverlayPNG, const char *szNormal, bool fColorByOwner); // load specified graphics from group
	bool LoadBitmaps(C4Group &hGroup, bool fColorByOwner); // load graphics from group
	bool LoadMesh(C4Group &hGroup, const char* szFilename, StdMeshSkeletonLoader& loader);
	bool LoadSkeleton(C4Group &hGroup, const char* szFilename, StdMeshSkeletonLoader& loader);
	bool Load(C4Group &hGroup, StdMeshSkeletonLoader &loader, bool fColorByOwner); // load graphics from group
	C4DefGraphics *Get(const char *szGrpName); // get graphics by name
	void Clear(); // clear fields; delete additional graphics
	bool IsMesh() const { return Type == TYPE_Mesh; }
	bool IsColorByOwner() // returns whether ColorByOwner-surfaces have been created
	{ return Type == TYPE_Mesh || (Type == TYPE_Bitmap && !!Bmp.BitmapClr); } // Mesh can always apply PlayerColor (if used in its material)

	void Draw(C4Facet &cgo, DWORD iColor, C4Object *pObj, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform* trans);

	virtual const char *GetName() { return nullptr; } // return name to be stored in safe game files

	C4AdditionalDefGraphics *GetNext() { return pNext; }

	void DrawClr(C4Facet &cgo, bool fAspect=true, DWORD dwClr=0); // set surface color and draw

	void CompileFunc(StdCompiler *pComp);

	friend class C4DefGraphicsPtrBackup;
};

// additional definition graphics
class C4AdditionalDefGraphics : public C4DefGraphics
{
protected:
	char Name[C4MaxName+1];   // graphics name

public:
	C4AdditionalDefGraphics(C4Def *pOwnDef, const char *szName);  // ctor
	virtual const char *GetName() { return Name; }
};

// backup class holding dead graphics pointers and names
class C4DefGraphicsPtrBackupEntry
{
protected:
	C4DefGraphics *pGraphicsPtr; // dead graphics ptr
	C4Def *pDef;                 // definition of dead graphics
	char Name[C4MaxName+1];        // name of graphics
	StdMeshUpdate* pMeshUpdate;    // Dead mesh

public:
	C4DefGraphicsPtrBackupEntry(C4DefGraphics *pSourceGraphics); // ctor
	~C4DefGraphicsPtrBackupEntry();                              // dtor

	void AssignUpdate();   // update all game objects with new graphics pointers
	void AssignRemoval();  // remove graphics of this def from all game objects

private:
	void UpdateAttachedMeshes();
	void UpdateAttachedMesh(StdMeshInstance* instance);
};

// On definition reload, all graphics updates need to be performed in one
// batch using this class.
class C4DefGraphicsPtrBackup
{
public:
	C4DefGraphicsPtrBackup();
	~C4DefGraphicsPtrBackup();

	// Add a def graphics to the list of graphics to be updated.
	// Also adds additional graphics linked from pGraphics.
	void Add(C4DefGraphics *pGraphics);

	void AssignUpdate();   // update all game objects with new graphics pointers
	void AssignRemoval();  // remove graphics of all defs from all game objects

	StdMeshMaterialUpdate &GetUpdater() { return MeshMaterialUpdate; }

private:
	void UpdateMesh(StdMeshInstance* instance);

	StdMeshMaterialUpdate MeshMaterialUpdate; // Backup of dead mesh materials
	StdMeshAnimationUpdate MeshAnimationUpdate; // Backup of animation names in the animation tree

	std::list<C4DefGraphicsPtrBackupEntry*> Entries;

	bool fApplied;
};

// Helper to compile C4DefGraphics-Pointer
class C4DefGraphicsAdapt
{
protected:
	C4DefGraphics *&pDefGraphics;
public:
	C4DefGraphicsAdapt(C4DefGraphics *&pDefGraphics) : pDefGraphics(pDefGraphics) { }
	void CompileFunc(StdCompiler *pComp);
	// Default checking / setting
	bool operator == (C4DefGraphics *pDef2) { return pDefGraphics == pDef2; }
	void operator = (C4DefGraphics *pDef2) { pDefGraphics = pDef2; }
};

// graphics overlay used to attach additional graphics to objects
class C4GraphicsOverlay
{
	friend class C4DefGraphicsPtrBackupEntry;
	friend class C4DefGraphicsPtrBackup;
public:
	enum Mode
	{
		MODE_None=0,
		MODE_Base=1,     // display base facet
		MODE_Action=2,   // display action facet specified in Action
		MODE_Picture=3,  // overlay picture to this picture only
		MODE_IngamePicture=4, // draw picture of source def
		MODE_Object=5,        // draw another object gfx
		MODE_ExtraGraphics=6,       // draw like this were a ClrByOwner-surface
		MODE_Rank=7,                 // draw rank symbol
		MODE_ObjectPicture=8    // draw the picture of source object
	};
protected:
	Mode eMode;                // overlay mode

	C4DefGraphics *pSourceGfx; // source graphics - used for savegame saving and comparisons in ReloadDef
	char Action[C4MaxName+1];  // action used as overlay in source gfx
	C4TargetFacet fctBlit; // current blit data for bitmap graphics
	StdMeshInstance* pMeshInstance; // NoSave // - current blit data for mesh graphics 
	uint32_t dwBlitMode;          // extra parameters for additive blits, etc.
	uint32_t dwClrModulation;        // colormod for this overlay
	C4ObjectPtr OverlayObj; // object to be drawn as overlay in MODE_Object
	C4DrawTransform Transform; // drawing transformation: Rotation, zoom, etc.
	int32_t iPhase;                // action face for MODE_Action
	bool fZoomToShape;             // if true, overlay will be zoomed to match the target object shape

	int32_t iID; // identification number for Z-ordering and script identification

	C4GraphicsOverlay *pNext; // singly linked list

	void UpdateFacet();       // update fctBlit to reflect current data
	void Set(Mode aMode, C4DefGraphics *pGfx, const char *szAction, DWORD dwBMode, C4Object *pOvrlObj);

public:
	C4GraphicsOverlay() : eMode(MODE_None), pSourceGfx(nullptr), fctBlit(), pMeshInstance(nullptr), dwBlitMode(0), dwClrModulation(0xffffff),
			OverlayObj(nullptr), Transform(+1),
			iPhase(0), fZoomToShape(false), iID(0), pNext(nullptr) { *Action=0; } // std ctor
	~C4GraphicsOverlay(); // dtor

	void CompileFunc(StdCompiler *pComp);

	// object pointer management
	void DenumeratePointers();

	void SetAsBase(C4DefGraphics *pBaseGfx, DWORD dwBMode) // set in MODE_Base
	{ Set(MODE_Base, pBaseGfx, nullptr, dwBMode, nullptr); }
	void SetAsAction(C4DefGraphics *pBaseGfx, const char *szAction, DWORD dwBMode)
	{ Set(MODE_Action, pBaseGfx, szAction, dwBMode, nullptr); }
	void SetAsPicture(C4DefGraphics *pBaseGfx, DWORD dwBMode)
	{ Set(MODE_Picture, pBaseGfx, nullptr, dwBMode, nullptr); }
	void SetAsIngamePicture(C4DefGraphics *pBaseGfx, DWORD dwBMode)
	{ Set(MODE_IngamePicture, pBaseGfx, nullptr, dwBMode, nullptr); }
	void SetAsObject(C4Object *pOverlayObj, DWORD dwBMode)
	{ Set(MODE_Object, nullptr, nullptr, dwBMode, pOverlayObj); }
	void SetAsObjectPicture(C4Object *pOverlayObj, DWORD dwBMode)
	{ Set(MODE_ObjectPicture, nullptr, nullptr, dwBMode, pOverlayObj); }
	void SetAsExtraGraphics(C4DefGraphics *pGfx, DWORD dwBMode)
	{ Set(MODE_ExtraGraphics, pGfx, nullptr, dwBMode, nullptr); }
	void SetAsRank(DWORD dwBMode, C4Object *rank_obj)
	{ Set(MODE_Rank, nullptr, nullptr, dwBMode, rank_obj); }

	bool IsValid(const C4Object *pForObj) const;

	C4DrawTransform *GetTransform() { return &Transform; }
	C4Object *GetOverlayObject() const { return OverlayObj; }
	int32_t GetID() const { return iID; }
	void SetID(int32_t aID) { iID = aID; }
	void SetPhase(int32_t iToPhase);
	C4GraphicsOverlay *GetNext() const { return pNext; }
	void SetNext(C4GraphicsOverlay *paNext) { pNext = paNext; }
	bool IsPicture() { return eMode == MODE_Picture; }
	C4DefGraphics *GetGfx() const { return pSourceGfx; }

	void Draw(C4TargetFacet &cgo, C4Object *pForObj, int32_t iByPlayer);
	void DrawPicture(C4Facet &cgo, C4Object *pForObj, C4DrawTransform* trans);
	void DrawRankSymbol(C4Facet &cgo, C4Object *rank_obj);

	bool operator == (const C4GraphicsOverlay &rCmp) const; // comparison operator

	uint32_t GetClrModulation() const { return dwClrModulation; }
	void SetClrModulation(uint32_t dwToMod) { dwClrModulation = dwToMod; }

	uint32_t GetBlitMode() const { return dwBlitMode; }
	void SetBlitMode(uint32_t dwToMode) { dwBlitMode = dwToMode; }

};

// Helper to compile lists of C4GraphicsOverlay
class C4GraphicsOverlayListAdapt
{
protected:
	C4GraphicsOverlay *&pOverlay;
public:
	C4GraphicsOverlayListAdapt(C4GraphicsOverlay *&pOverlay) : pOverlay(pOverlay) { }
	void CompileFunc(StdCompiler *pComp);
	// Default checking / setting
	bool operator == (C4GraphicsOverlay *pDefault) { return pOverlay == pDefault; }
	void operator = (C4GraphicsOverlay *pDefault) { delete pOverlay; pOverlay = pDefault; }
};

#endif // INC_C4DefGraphics
