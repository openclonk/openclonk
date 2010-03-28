/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2006  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de
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
// graphics used by object definitions (object and portraits)

#ifndef INC_C4DefGraphics
#define INC_C4DefGraphics

#include "C4FacetEx.h"
#include "C4Surface.h"
#include "C4InputValidation.h"
#include "C4Id.h"

#define C4Portrait_None   "none"
#define C4Portrait_Random "random"
#define C4Portrait_Custom "custom"

class C4Def;
class C4Object;

// defintion graphics
class C4AdditionalDefGraphics;
class C4DefGraphicsPtrBackup;
class C4PortraitGraphics;

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
		TYPE_Bitmap,
		TYPE_Mesh
	};

	GraphicsType Type;

	union
	{
		struct
		{
			C4Surface *Bitmap, *BitmapClr;
		} Bmp;
		StdMesh *Mesh;
	};

	bool fColorBitmapAutoCreated;  // if set, the color-by-owner-bitmap has been created automatically by all blue shades of the bitmap

	inline C4Surface *GetBitmap(DWORD dwClr=0) { if (Type != TYPE_Bitmap) return NULL; if (Bmp.BitmapClr) { Bmp.BitmapClr->SetClr(dwClr); return Bmp.BitmapClr; } else return Bmp.Bitmap; }

	C4DefGraphics(C4Def *pOwnDef=NULL);  // ctor
	virtual ~C4DefGraphics() { Clear(); }; // dtor

	bool LoadBitmap(C4Group &hGroup, const char *szFilename, const char *szFilenamePNG, const char *szOverlayPNG, bool fColorByOwner); // load specified graphics from group
	bool LoadBitmaps(C4Group &hGroup, bool fColorByOwner); // load graphics from group
	bool LoadMesh(C4Group &hGroup, StdMeshSkeletonLoader& loader);
	bool Load(C4Group &hGroup, bool fColorByOwner); // load graphics from group
	C4DefGraphics *Get(const char *szGrpName); // get graphics by name
	void Clear(); // clear fields; delete additional graphics
	bool IsColorByOwner() // returns whether ColorByOwner-surfaces have been created
	{ return Type == TYPE_Mesh || !!Bmp.BitmapClr; } // Mesh can always apply PlayerColor (if used in its material)
	bool CopyGraphicsFrom(C4DefGraphics &rSource); // copy bitmaps from source graphics

	virtual const char *GetName() { return NULL; } // return name to be stored in safe game files

	C4AdditionalDefGraphics *GetNext() { return pNext; }
	virtual C4PortraitGraphics *IsPortrait() { return NULL; }

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

// portrait graphics within object definition
class C4PortraitGraphics : public C4AdditionalDefGraphics
{
public:
	C4PortraitGraphics(C4Def *pOwnDef, const char *szName)  // ctor
			: C4AdditionalDefGraphics(pOwnDef, szName) { }

	virtual C4PortraitGraphics *IsPortrait() { return this; }
	C4PortraitGraphics *GetByIndex(int32_t iIndex);
	C4PortraitGraphics *Get(const char *szGrpName); // get portrait graphics by name
};

// backup class holding dead graphics pointers and names
class C4DefGraphicsPtrBackup
{
protected:
	C4DefGraphics *pGraphicsPtr; // dead graphics ptr
	C4Def *pDef;                 // definition of dead graphics
	char Name[C4MaxName+1];        // name of graphics
	C4DefGraphicsPtrBackup *pNext; // next member of linked list

public:
	C4DefGraphicsPtrBackup(C4DefGraphics *pSourceGraphics); // ctor
	~C4DefGraphicsPtrBackup();                              // dtor

	void AssignUpdate(C4DefGraphics *pNewGraphics); // update all game objects with new graphics pointers
	void AssignRemoval();                           // remove graphics of this def from all game objects
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
	ALLOW_TEMP_TO_REF(C4DefGraphicsAdapt)
};

// portrait link class
class C4Portrait
{
protected:
	C4DefGraphics *pGfxPortrait; // the portrait graphics
	bool fGraphicsOwned;         // if true, the portrait graphics are owned (and deleted upon destruction)

public:
	C4Portrait() : pGfxPortrait(NULL), fGraphicsOwned(false) {} // ctor
	~C4Portrait() { if (fGraphicsOwned) delete pGfxPortrait; }  // dtor
	void Default() { pGfxPortrait=NULL; fGraphicsOwned=false; }
	void Clear() { if (fGraphicsOwned) { delete pGfxPortrait; fGraphicsOwned=false; } pGfxPortrait=NULL; }

	bool CopyFrom(C4DefGraphics &rCopyGfx); // copy portrait from graphics
	bool CopyFrom(C4Portrait &rCopy); // copy portrait

	bool Load(C4Group &rGrp, const char *szFilename, const char *szFilenamePNG, const char *szOverlayPNG); // load own portrait from group
	bool Link(C4DefGraphics *pGfxPortrait);        // link with a present portrait surface
	bool SavePNG(C4Group &rGroup, const char *szFilename, const char *szOverlayFN); // store portrait gfx to file (including overlay)

	C4DefGraphics *GetGfx() { return pGfxPortrait; }
	bool IsOwnedGfx() { return fGraphicsOwned; }         // return if it's a custom portrait

	static const char *EvaluatePortraitString(const char *szPortrait, C4ID &rIDOut, C4ID idDefaultID, uint32_t *pdwClrOut); // get portrait source and def from string
};

// graphics overlay used to attach additional graphics to objects
class C4GraphicsOverlay
{
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
	StdMeshInstance* pMeshInstance; // current blit data for mesh graphics
	uint32_t dwBlitMode;          // extra parameters for additive blits, etc.
	uint32_t dwClrModulation;        // colormod for this overlay
	C4Object *pOverlayObj; // object to be drawn as overlay in MODE_Object
	int32_t nOverlayObj; // compiled ptr
	C4DrawTransform Transform; // drawing transformation: Rotation, zoom, etc.
	int32_t iPhase;                // action face for MODE_Action
	bool fZoomToShape;             // if true, overlay will be zoomed to match the target object shape

	int32_t iID; // identification number for Z-ordering and script identification

	C4GraphicsOverlay *pNext; // singly linked list

	void UpdateFacet();       // update fctBlit to reflect current data
	void Set(Mode aMode, C4DefGraphics *pGfx, const char *szAction, DWORD dwBMode, C4Object *pOvrlObj);

public:
	C4GraphicsOverlay() : eMode(MODE_None), pSourceGfx(NULL), fctBlit(), pMeshInstance(NULL), dwBlitMode(0), dwClrModulation(0xffffff),
			pOverlayObj(NULL), nOverlayObj(0), Transform(+1),
			iPhase(0), fZoomToShape(false), iID(0), pNext(NULL) { *Action=0; } // std ctor
	~C4GraphicsOverlay(); // dtor

	void Read(const char **ppInput);
	void Write(char *ppOutput);
	void CompileFunc(StdCompiler *pComp);

	// object pointer management
	void EnumeratePointers();
	void DenumeratePointers();

	void SetAsBase(C4DefGraphics *pBaseGfx, DWORD dwBMode) // set in MODE_Base
	{ Set(MODE_Base, pBaseGfx, NULL, dwBMode, NULL); }
	void SetAsAction(C4DefGraphics *pBaseGfx, const char *szAction, DWORD dwBMode)
	{ Set(MODE_Action, pBaseGfx, szAction, dwBMode, NULL); }
	void SetAsPicture(C4DefGraphics *pBaseGfx, DWORD dwBMode)
	{ Set(MODE_Picture, pBaseGfx, NULL, dwBMode, NULL); }
	void SetAsIngamePicture(C4DefGraphics *pBaseGfx, DWORD dwBMode)
	{ Set(MODE_IngamePicture, pBaseGfx, NULL, dwBMode, NULL); }
	void SetAsObject(C4Object *pOverlayObj, DWORD dwBMode)
	{ Set(MODE_Object, NULL, NULL, dwBMode, pOverlayObj); }
	void SetAsObjectPicture(C4Object *pOverlayObj, DWORD dwBMode)
	{ Set(MODE_ObjectPicture, NULL, NULL, dwBMode, pOverlayObj); }
	void SetAsExtraGraphics(C4DefGraphics *pGfx, DWORD dwBMode)
	{ Set(MODE_ExtraGraphics, pGfx, NULL, dwBMode, NULL); }
	void SetAsRank(DWORD dwBMode, C4Object *rank_obj)
	{ Set(MODE_Rank, NULL, NULL, dwBMode, rank_obj); }

	bool IsValid(const C4Object *pForObj) const;

	C4DrawTransform *GetTransform() { return &Transform; }
	C4Object *GetOverlayObject() const { return pOverlayObj; }
	int32_t GetID() const { return iID; }
	void SetID(int32_t aID) { iID = aID; }
	void SetPhase(int32_t iToPhase);
	C4GraphicsOverlay *GetNext() const { return pNext; }
	void SetNext(C4GraphicsOverlay *paNext) { pNext = paNext; }
	bool IsPicture() { return eMode == MODE_Picture; }
	C4DefGraphics *GetGfx() const { return pSourceGfx; }

	void Draw(C4TargetFacet &cgo, C4Object *pForObj, int32_t iByPlayer);
	void DrawPicture(C4Facet &cgo, C4Object *pForObj);
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
	ALLOW_TEMP_TO_REF(C4GraphicsOverlayListAdapt)
};

#endif // INC_C4DefGraphics
