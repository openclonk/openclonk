/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Textures used by the landscape */

#ifndef INC_C4Texture
#define INC_C4Texture

#include <C4Surface.h>
#include <C4Constants.h>
#include <C4Material.h>

// Custom texture drawing shape for Map2Landscape zooming
class C4TextureShape
{
private:
	enum { Shape_None = 0xff }; // special value in data surface: No shape defined here.
	CSurface8 data;
	int32_t num_shapes;
	std::vector<bool> shape_border_x, shape_border_y; // whether shapes are touching horizontal/vertical borders
	std::vector<int32_t> shape_pixnum; // number of pixels
public:
	C4TextureShape();

	void Clear();
	bool Load(C4Group &group, const char *filename, int32_t base_tex_wdt, int32_t base_tex_hgt);

	int32_t GetWidth() const { return data.Wdt; }
	int32_t GetHeight() const { return data.Hgt; }
	// Poly range used to ensure update range in editor mode is large enough
	// not calculated on loading for now. Just assume something reasonably safe
	int32_t GetMaxPolyWidth() const { return GetWidth() / 4; }
	int32_t GetMaxPolyHeight() const { return GetHeight() / 4; }

	void Draw(CSurface8 * sfcMap, CSurface8* sfcMapBkg, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint8_t iTexture, int32_t iOffX, int32_t iOffY, int32_t MapZoom, int32_t min_overlap_ratio);
};


class C4Texture
{
	friend class C4TextureMap;
public:
	C4Texture();
	~C4Texture();
	C4Surface * Surface32;

	void SetAverageColor(uint32_t Color) { AvgColor = Color; }
	uint32_t GetAverageColor() const { return AvgColor; }
	void SetMaterialShape(class C4TextureShape *s) { material_shape.reset(s); }
	class C4TextureShape *GetMaterialShape() const { return material_shape.get(); }
protected:
	StdStrBuf Name;
	uint32_t AvgColor;
	std::unique_ptr<class C4TextureShape> material_shape;
	C4Texture *Next;
};

class C4TexMapEntry
{
	friend class C4TextureMap;
public:
	C4TexMapEntry();
private:
	StdCopyStrBuf Material, Texture;
	int32_t iMaterialIndex;
	C4Material *pMaterial;
	C4Pattern MatPattern;
public:
	bool isNull() const { return Material.isNull(); }
	const char *GetMaterialName() const { return Material.getData(); }
	const char *GetTextureName() const { return Texture.getData(); }
	int32_t GetMaterialIndex() const { return iMaterialIndex; }
	C4Material *GetMaterial() const { return pMaterial; }
	const C4Pattern &GetPattern() const { return MatPattern; }
	void Clear();
	bool Create(const char *szMaterial, const char *szTexture);
	bool Init();
};

class C4TextureMap
{
public:
	C4TextureMap();
	~C4TextureMap();
protected:
	C4TexMapEntry Entry[C4M_MaxTexIndex];
	std::vector<int32_t> Order; // drawing order in map2landscape. Reflects order in MatMap.txt file.
	C4Texture *FirstTexture;
	bool fOverloadMaterials;
	bool fOverloadTextures;
	bool fInitialized; // Set after Init() - newly added entries initialized automatically
public:
	bool fEntriesAdded;
public:
	const C4TexMapEntry *GetEntry(int32_t iIndex) const { return Inside<int32_t>(iIndex, 0, C4M_MaxTexIndex-1) ? &Entry[iIndex] : NULL; }
	void RemoveEntry(int32_t iIndex);
	void Clear();
	void StoreMapPalette(CStdPalette *, C4MaterialMap &rMaterials);
	static bool LoadFlags(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures);
	int32_t LoadMap(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures);
	int32_t Init();
	bool SaveMap(C4Group &hGroup, const char *szEntryName);
	int32_t LoadTextures(C4Group &hGroup, C4Group* OverloadFile=0);
	bool HasTextures(C4Group &hGroup);
	const char *GetTexture(int32_t iIndex);
	void MoveIndex(BYTE byOldIndex, BYTE byNewIndex); // change index of texture
	int32_t GetIndex(const char *szMaterial, const char *szTexture, bool fAddIfNotExist=true, const char *szErrorIfFailed=NULL);
	int32_t GetIndexMatTex(const char *szMaterialTexture, const char *szDefaultTexture = NULL, bool fAddIfNotExist=true, const char *szErrorIfFailed=NULL);
	C4Texture * GetTexture(const char *szTexture);
	bool CheckTexture(const char *szTexture); // return whether texture exists
	bool AddEntry(BYTE byIndex, const char *szMaterial, const char *szTexture);
	bool AddTexture(const char *szTexture, C4Surface * sfcSurface);
	int32_t GetTextureIndex(const char *pTexName);
	BYTE DefaultBkgMatTex(BYTE fg) const;
protected:
	friend class C4Landscape;
};

extern C4TextureMap TextureMap;

#endif
