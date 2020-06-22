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

/* Textures used by the landscape */

#include "C4Include.h"
#include "landscape/C4Texture.h"

#include "c4group/C4Components.h"
#include "c4group/C4Group.h"
#include "landscape/C4Landscape.h"
#include "lib/C4Random.h"
#include "lib/StdColors.h"

C4Texture::C4Texture()
{
	Surface32=nullptr;
	AvgColor = 0x00000000;
	Next=nullptr;
}

C4Texture::~C4Texture()
{
	delete Surface32;
}

C4TexMapEntry::C4TexMapEntry()
		: iMaterialIndex(MNone)
{
}

void C4TexMapEntry::Clear()
{
	Material.Clear(); Texture.Clear();
	iMaterialIndex = MNone;
	pMaterial = nullptr;
	MatPattern.Clear();
}

bool C4TexMapEntry::Create(const char *szMaterial, const char *szTexture)
{
	// Clear previous data
	Clear();
	// Save names
	Material = szMaterial; Texture = szTexture;
	return true;
}

bool C4TexMapEntry::Init()
{
	// Find material
	iMaterialIndex = ::MaterialMap.Get(Material.getData());
	if (!MatValid(iMaterialIndex))
	{
		DebugLogF("Error initializing material %s-%s: Invalid material!", Material.getData(), Texture.getData());
		return false;
	}
	pMaterial = &::MaterialMap.Map[iMaterialIndex];
	// Find texture
	StdStrBuf FirstTexture;
	FirstTexture.CopyUntil(Texture.getData(), '-');
	C4Texture * sfcTexture = ::TextureMap.GetTexture(FirstTexture.getData());
	if (!sfcTexture)
	{
		DebugLogF("Error initializing material %s-%s: Invalid texture!", Material.getData(), FirstTexture.getData());
		Clear();
		return false;
	}
	// Get overlay properties
	int32_t iOverlayType=pMaterial->OverlayType;
	int32_t iZoom=0;
	if (iOverlayType & C4MatOv_Exact) iZoom=1;
	if (iOverlayType & C4MatOv_HugeZoom) iZoom=4;
	// Create pattern
	MatPattern.Set(sfcTexture->Surface32, iZoom);
	return true;
}

C4TextureMap::C4TextureMap()
{
	Order.reserve(C4M_MaxTexIndex);
}

C4TextureMap::~C4TextureMap()
{
	Clear();
}

bool C4TextureMap::AddEntry(BYTE byIndex, const char *szMaterial, const char *szTexture)
{
	// Security
	if (byIndex <= 0 || byIndex >= C4M_MaxTexIndex)
		return false;
	// Set entry and initialize
	Entry[byIndex].Create(szMaterial, szTexture);
	if (fInitialized)
	{
		if (!Entry[byIndex].Init())
		{
			// Clear entry if it could not be initialized
			Entry[byIndex].Clear();
			return false;
		}
		// Landscape must be notified (new valid pixel clr)
		::Landscape.HandleTexMapUpdate();
	}
	// Add last in order list
	Order.push_back(byIndex);
	return true;
}

bool C4TextureMap::AddTexture(const char *szTexture, C4Surface * sfcSurface)
{
	C4Texture *pTexture;
	if (!(pTexture=new C4Texture)) return false;
	pTexture->Name.Copy(szTexture);
	pTexture->Surface32=sfcSurface;
	pTexture->Next=FirstTexture;
	FirstTexture=pTexture;

	// Compute average texture color
	if(sfcSurface)
	{
		sfcSurface->Lock();
		uint32_t avg_c[4] = { 0, 0, 0, 0 };
		for(int32_t y = 0; y < sfcSurface->Hgt; ++y)
		{
			for(int32_t x = 0; x < sfcSurface->Wdt; ++x)
			{
				DWORD c = sfcSurface->GetPixDw(x, y, false);
				avg_c[0] += c & 0xff;
				avg_c[1] += (c >> 8) & 0xff;
				avg_c[2] += (c >> 16) & 0xff;
				avg_c[3] += (c >> 24) & 0xff;
			}
		}
		sfcSurface->Unlock();

		double Size = sfcSurface->Wdt * sfcSurface->Hgt;
		avg_c[0] = static_cast<uint32_t>(avg_c[0] / Size + 0.5);
		avg_c[1] = static_cast<uint32_t>(avg_c[1] / Size + 0.5);
		avg_c[2] = static_cast<uint32_t>(avg_c[2] / Size + 0.5);
		avg_c[3] = static_cast<uint32_t>(avg_c[3] / Size + 0.5);
		pTexture->SetAverageColor(avg_c[0] | (avg_c[1] << 8) | (avg_c[2] << 16) | (avg_c[3] << 24));
	}
	else
	{
		pTexture->SetAverageColor(0x00000000);
	}

	return true;
}

void C4TextureMap::Clear()
{
	for (int32_t i = 1; i < C4M_MaxTexIndex; i++)
		Entry[i].Clear();
	C4Texture *ctex,*next2;
	for (ctex=FirstTexture; ctex; ctex=next2)
	{
		next2=ctex->Next;
		delete ctex;
	}
	FirstTexture=nullptr;
	fInitialized = false;
	fEntriesAdded = false;
	fOverloadMaterials = false;
	fOverloadTextures = false;
	Order.clear();
	Order.reserve(C4M_MaxTexIndex);
}

bool C4TextureMap::LoadFlags(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures)
{
	// Load the file
	StdStrBuf TexMap;
	if (!hGroup.LoadEntryString(szEntryName, &TexMap))
		return false;
	// Reset flags
	if (pOverloadMaterials) *pOverloadMaterials = false;
	if (pOverloadTextures) *pOverloadTextures = false;
	// Check if there are flags in there
	for (const char *pPos = TexMap.getData(); pPos && *pPos; pPos = SSearch(pPos + 1, "\n"))
	{
		// Go over newlines
		while (*pPos == '\r' || *pPos == '\n') pPos++;
		// Flag?
		if (pOverloadMaterials && SEqual2(pPos, "OverloadMaterials"))
			*pOverloadMaterials = true;
		if (pOverloadTextures && SEqual2(pPos, "OverloadTextures"))
			*pOverloadTextures = true;
	}
	// Done
	return true;
}

int32_t C4TextureMap::LoadMap(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures)
{
	static std::regex line_terminator("\r?\n", static_cast<std::regex::flag_type>(std::regex_constants::optimize | std::regex_constants::ECMAScript));

	char *bpMap;
	size_t map_size;
	int32_t iTextures = 0;
	// Load text file into memory
	if (!hGroup.LoadEntry(szEntryName,&bpMap,&map_size,1)) return 0;

	char *begin = bpMap;
	char *end = begin + map_size;

	size_t line = 1; // Counter for error messages
	for (auto it = std::cregex_token_iterator(begin, end, line_terminator, -1); it != std::cregex_token_iterator(); ++it, ++line)
	{
		if (it->compare("OverloadMaterials") == 0)
		{
			fOverloadMaterials = true;
			if (pOverloadMaterials)
				*pOverloadMaterials = true;
		}
		else if (it->compare("OverloadTextures") == 0)
		{
			fOverloadTextures = true;
			if (pOverloadTextures)
				*pOverloadTextures = true;
		}
		else if (it->length() == 0 || it->first[0] == '#' || std::all_of(it->first, it->second, &isspace))
		{
			// Skip empty lines, comments, and all-whitespace lines
			continue;
		}
		else
		{
			// This must be a texmap entry now
			std::string value;
			
			// Read index
			unsigned long index;
			try
			{
				size_t separator;
				index = std::stoul(it->str(), &separator, 10);
				if (index >= C4M_MaxTexIndex)
					throw std::out_of_range("Texture index out of range");
				value.assign(it->first + separator + 1, it->second);
			}
			catch (std::invalid_argument &)
			{
				DebugLogF("TexMap line %u: Texture index is not numeric", static_cast<unsigned>(line));
				continue;
			}
			catch (std::out_of_range &)
			{
				DebugLogF("TexMap line %u: Texture index is out of range", static_cast<unsigned>(line));
				continue;
			}

			// Split material/texture combination
			std::string::const_iterator separator = std::find(value.cbegin(), value.cend(), '-');
			if (separator == value.cend())
			{
				DebugLogF(R"(TexMap line %u: Texture name "%s" is invalid (missing "-"))", static_cast<unsigned>(line), value.c_str());
				continue;
			}

			std::string material(value.cbegin(), separator);
			std::string texture(separator + 1, value.cend());

			if (AddEntry(index, material.c_str(), texture.c_str()))
				++iTextures;
		}
	}

	// Delete buffer, return entry count
	delete [] bpMap;
	fEntriesAdded=false;
	return iTextures;
}

int32_t C4TextureMap::Init()
{
	int32_t iRemoved = 0;
	// Initialize texture mappings
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++)
		if (!Entry[i].isNull())
			if (!Entry[i].Init())
			{
				LogF("Error in TextureMap initialization at entry %d", (int) i);
				Entry[i].Clear();
				iRemoved++;
			}
	fInitialized = true;
	return iRemoved;
}

bool C4TextureMap::SaveMap(C4Group &hGroup, const char *szEntryName)
{
	// build file in memory
	StdStrBuf sTexMapFile;
	// add desc
	sTexMapFile.Append("# Automatically generated texture map\n");
	sTexMapFile.Append("# Contains material-texture-combinations added at runtime\n");
	// add overload-entries
	if (fOverloadMaterials) sTexMapFile.Append("# Import materials from global file as well\nOverloadMaterials\n");
	if (fOverloadTextures) sTexMapFile.Append("# Import textures from global file as well\nOverloadTextures\n");
	sTexMapFile.Append("\n");
	// add entries
	for (auto i : Order)
	{
		if (!Entry[i].isNull())
		{
			// compose line
			sTexMapFile.AppendFormat("%d=%s-%s\n", i, Entry[i].GetMaterialName(), Entry[i].GetTextureName());
		}
	}
	// create new buffer allocated with new [], because C4Group cannot handle StdStrBuf-buffers
	size_t iBufSize = sTexMapFile.getLength();
	BYTE *pBuf = new BYTE[iBufSize];
	memcpy(pBuf, sTexMapFile.getData(), iBufSize);
	// add to group
	bool fSuccess = !!hGroup.Add(szEntryName, pBuf, iBufSize, false, true);
	if (!fSuccess) delete [] pBuf;
	// done
	return fSuccess;
}

int32_t C4TextureMap::LoadTextures(C4Group &hGroup, C4Group* OverloadFile)
{
	int32_t texnum=0;
	// overload: load from other file
	if (OverloadFile) texnum+=LoadTextures(*OverloadFile);

	char texname[256+1];
	C4Surface *ctex;
	size_t binlen;

	hGroup.ResetSearch();
	while (hGroup.AccessNextEntry("*",&binlen,texname))
	{
		// check if it already exists in the map
		const char *base_filename = GetFilenameOnly(texname);
		if (GetTexture(base_filename)) continue;
		// skip shape textures for now. Will be added later after all base textures have been loaded
		if (WildcardMatch("*" C4CFN_MaterialShapeFiles, texname)) continue;
		// create surface
		ctex = new C4Surface();
		if (ctex->Read(hGroup, GetExtension(texname), C4SF_MipMap))
		{
			SReplaceChar(texname,'.',0);
			if (AddTexture(texname,ctex)) texnum++;
			else delete ctex;
		}
		else
		{
			delete ctex;
		}
	}

	// Load texture shapes
	hGroup.ResetSearch();
	while (hGroup.AccessNextEntry("*" C4CFN_MaterialShapeFiles, &binlen, texname))
	{
		// get associated texture
		StdStrBuf texname4shape(texname, true);
		texname4shape.SetLength(texname4shape.getLength() - SLen(C4CFN_MaterialShapeFiles));
		C4Texture *base_tex = GetTexture(texname4shape.getData());
		if (!base_tex || !base_tex->Surface32)
		{
			LogF("ERROR: Texture shape %s not loaded because associated texture (%s) not found or invalid.", hGroup.GetFullName().getData(), texname4shape.getData());
			continue;
		}
		std::unique_ptr<C4TextureShape> shape(new C4TextureShape());
		int32_t scaler_zoom = 4;
		if (!shape->Load(hGroup, texname, base_tex->Surface32->Wdt / scaler_zoom, base_tex->Surface32->Hgt / scaler_zoom))
		{
			LogF("Error loading texture shape %s.", hGroup.GetFullName().getData());
			continue;
		}
		base_tex->SetMaterialShape(shape.release());
	}

	return texnum;
}

bool C4TextureMap::HasTextures(C4Group &hGroup)
{
	return hGroup.EntryCount(C4CFN_PNGFiles) || hGroup.EntryCount(C4CFN_BitmapFiles);
}

void C4TextureMap::MoveIndex(BYTE byOldIndex, BYTE byNewIndex)
{
	if (byNewIndex == byOldIndex) return;
	Entry[byNewIndex] = Entry[byOldIndex];
	Entry[byOldIndex].Clear();
	auto old_entry = std::find_if(Order.begin(), Order.end(),
		[byOldIndex](const int32_t &entry) { return entry == byOldIndex; });
	if (old_entry != Order.end()) *old_entry = byNewIndex;
	fEntriesAdded = true;
}

int32_t C4TextureMap::GetIndex(const char *szMaterial, const char *szTexture, bool fAddIfNotExist, const char *szErrorIfFailed)
{
	BYTE byIndex;
	// Find existing
	for (byIndex = 1; byIndex < C4M_MaxTexIndex; byIndex++)
		if (!Entry[byIndex].isNull())
			if (SEqualNoCase(Entry[byIndex].GetMaterialName(), szMaterial))
				if (!szTexture || SEqualNoCase(Entry[byIndex].GetTextureName(), szTexture))
					return byIndex;
	// Add new entry
	if (fAddIfNotExist)
		for (byIndex=1; byIndex<C4M_MaxTexIndex; byIndex++)
			if (Entry[byIndex].isNull())
			{
				if (AddEntry(byIndex, szMaterial, szTexture))
				{
					fEntriesAdded=true;
					return byIndex;
				}
				if (szErrorIfFailed) DebugLogF("Error getting MatTex %s-%s for %s from TextureMap: Init failed.", szMaterial, szTexture, szErrorIfFailed);
				return 0;
			}
	// Else, fail
	if (szErrorIfFailed) DebugLogF("Error getting MatTex %s-%s for %s from TextureMap: %s.", szMaterial, szTexture, szErrorIfFailed, fAddIfNotExist ? "Map is full!" : "Entry not found.");
	return 0;
}

int32_t C4TextureMap::GetIndexMatTex(const char *szMaterialTexture, const char *szDefaultTexture, bool fAddIfNotExist, const char *szErrorIfFailed)
{
	// split material/texture pair
	StdStrBuf Material, Texture;
	Material.CopyUntil(szMaterialTexture, '-');
	Texture.Copy(SSearch(szMaterialTexture, "-"));
	// texture not given or invalid?
	int32_t iMatTex = 0;
	if (Texture.getData())
		if ((iMatTex = GetIndex(Material.getData(), Texture.getData(), fAddIfNotExist)))
			return iMatTex;
	if (szDefaultTexture)
		if ((iMatTex = GetIndex(Material.getData(), szDefaultTexture, fAddIfNotExist)))
			return iMatTex;
	// search material
	long iMaterial = ::MaterialMap.Get(szMaterialTexture);
	if (!MatValid(iMaterial))
	{
		if (szErrorIfFailed) DebugLogF("Error getting MatTex for %s: Invalid material", szErrorIfFailed);
		return 0;
	}
	// return default map entry
	return ::MaterialMap.Map[iMaterial].DefaultMatTex;
}

C4Texture * C4TextureMap::GetTexture(const char *szTexture)
{
	C4Texture *pTexture;
	for (pTexture=FirstTexture; pTexture; pTexture=pTexture->Next)
		if (SEqualNoCase(pTexture->Name.getData(),szTexture))
			return pTexture;
	return nullptr;
}

int32_t C4TextureMap::GetTextureIndex(const char *szName)
{
	C4Texture *pTexture;
	int32_t i=0;
	for (pTexture=FirstTexture; pTexture; pTexture=pTexture->Next, i++)
		if (SEqualNoCase(pTexture->Name.getData(),szName))
			return i;
	return -1;
}

bool C4TextureMap::CheckTexture(const char *szTexture)
{
	C4Texture *pTexture;
	for (pTexture=FirstTexture; pTexture; pTexture=pTexture->Next)
		if (SEqualNoCase(pTexture->Name.getData(),szTexture))
			return true;
	return false;
}

const char* C4TextureMap::GetTexture(int32_t iIndex)
{
	C4Texture *pTexture;
	int32_t cindex;
	for (pTexture=FirstTexture,cindex=0; pTexture; pTexture=pTexture->Next,cindex++)
		if (cindex==iIndex)
			return pTexture->Name.getData();
	return nullptr;
}

BYTE C4TextureMap::DefaultBkgMatTex(BYTE fg) const
{
	// For the given foreground index, find the default background index
	// If fg is semisolid, this is tunnel.
	// Otherwise it is fg itself, so that tunnel and background bricks
	// stay the way they are.
	int32_t iTex = PixCol2Tex(fg);
	if (!iTex) return fg; // sky

	// Get material-texture mapping
	const C4TexMapEntry *pTex = GetEntry(iTex);
	// Texmap entry does not exist
	if(!pTex || !pTex->GetMaterial()) return fg;

	if(DensitySemiSolid(pTex->GetMaterial()->Density))
		return Mat2PixColDefault(MTunnel);

	return fg;

}

void C4TextureMap::RemoveEntry(int32_t iIndex)
{
	// remove entry from table and order vector
	if (Inside<int32_t>(iIndex, 1, C4M_MaxTexIndex - 1))
	{
		Entry[iIndex].Clear();
		auto last_entry = std::remove_if(Order.begin(), Order.end(),
			[iIndex](const int32_t &entry) { return entry == iIndex; });
		Order.erase(last_entry, Order.end());
	}
}

void C4TextureMap::StoreMapPalette(CStdPalette *Palette, C4MaterialMap &rMaterial)
{
	// Sky color
	Palette->Colors[0] = C4RGB(192, 196, 252);
	// Material colors by texture map entries
	bool fSet[C4M_MaxTexIndex];
	ZeroMem(&fSet, sizeof (fSet));
	int32_t i;
	for (i = 1; i < C4M_MaxTexIndex; i++)
	{
		// Find material
		DWORD dwPix;
		auto texture = GetTexture(Entry[i].GetTextureName());
		if (texture)
			dwPix = texture->GetAverageColor();
		else
			dwPix = Entry[i].GetPattern().PatternClr(0, 0);
		Palette->Colors[i] = dwPix;
		fSet[i] = true;
	}
	// Crosscheck colors, change equal palette entries
	for (i = 0; i < C4M_MaxTexIndex; i++) if (fSet[i])
			for (;;)
			{
				// search equal entry
				int32_t j = 0;
				for (; j < i; j++)
					if (fSet[j] && Palette->Colors[i] == Palette->Colors[j])
							break;
				// not found? ok then
				if (j >= i) break;
				// change randomly
				Palette->Colors[i] = C4RGB(
					UnsyncedRandom(2) ? GetRedValue(Palette->Colors[i]) + 3 : GetRedValue(Palette->Colors[i]) - 3,
					UnsyncedRandom(2) ? GetGreenValue(Palette->Colors[i]) + 3 : GetGreenValue(Palette->Colors[i]) - 3,
					UnsyncedRandom(2) ? GetBlueValue(Palette->Colors[i]) + 3 : GetBlueValue(Palette->Colors[i]) - 3);
			}
}

C4TextureMap TextureMap;
