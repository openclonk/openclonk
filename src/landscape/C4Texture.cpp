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

#include <C4Include.h>
#include <C4Texture.h>

#include <C4Group.h>
#include <C4Game.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Material.h>
#include <C4Landscape.h>
#include <C4Log.h>

#include <ctype.h>
#include <algorithm>

C4Texture::C4Texture()
{
	Surface32=NULL;
	AvgColor = 0x00000000;
	Next=NULL;
}

C4Texture::~C4Texture()
{
	delete Surface32;
}

C4TexMapEntry::C4TexMapEntry()
		: iMaterialIndex(MNone), pMaterial(NULL)
{
}

void C4TexMapEntry::Clear()
{
	Material.Clear(); Texture.Clear();
	iMaterialIndex = MNone;
	pMaterial = NULL;
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
	Default();
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
	FirstTexture=NULL;
	fInitialized = false;
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
	static re::regex line_terminator("\r?\n", static_cast<re::regex::flag_type>(re::regex_constants::optimize | re::regex_constants::ECMAScript));

	char *bpMap;
	size_t map_size;
	int32_t iTextures = 0;
	// Load text file into memory
	if (!hGroup.LoadEntry(szEntryName,&bpMap,&map_size,1)) return 0;

	char *begin = bpMap;
	char *end = begin + map_size;

	size_t line = 1; // Counter for error messages
	for (auto it = re::cregex_token_iterator(begin, end, line_terminator, -1); it != re::cregex_token_iterator(); ++it, ++line)
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
				DebugLogF("TexMap line %u: Texture name \"%s\" is invalid (missing \"-\")", static_cast<unsigned>(line), value.c_str());
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
	sTexMapFile.Append("# Automatically generated texture map" LineFeed);
	sTexMapFile.Append("# Contains material-texture-combinations added at runtime" LineFeed);
	// add overload-entries
	if (fOverloadMaterials) sTexMapFile.Append("# Import materials from global file as well" LineFeed "OverloadMaterials" LineFeed);
	if (fOverloadTextures) sTexMapFile.Append("# Import textures from global file as well" LineFeed "OverloadTextures" LineFeed);
	sTexMapFile.Append(LineFeed);
	// add entries
	for (int32_t i = 0; i < C4M_MaxTexIndex; i++)
		if (!Entry[i].isNull())
		{
			// compose line
			sTexMapFile.AppendFormat("%d=%s-%s" LineFeed, i, Entry[i].GetMaterialName(), Entry[i].GetTextureName());
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
		if (GetTexture(GetFilenameOnly(texname))) continue;
		// create surface
		ctex = new C4Surface();
		if (ctex->Read(hGroup, GetExtension(texname)))
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

	return texnum;
}

bool C4TextureMap::HasTextures(C4Group &hGroup)
{
	return hGroup.EntryCount(C4CFN_PNGFiles) || hGroup.EntryCount(C4CFN_BitmapFiles);
}

void C4TextureMap::MoveIndex(BYTE byOldIndex, BYTE byNewIndex)
{
	Entry[byNewIndex] = Entry[byOldIndex];
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
	return NULL;
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
	return NULL;
}

void C4TextureMap::Default()
{
	FirstTexture=NULL;
	fEntriesAdded=false;
	fOverloadMaterials=false;
	fOverloadTextures=false;
	fInitialized = false;
}

void C4TextureMap::StoreMapPalette(CStdPalette *Palette, C4MaterialMap &rMaterial)
{
	// Sky color
	Palette->Colors[0] = C4RGB(192, 196, 252);
	// Material colors by texture map entries
	bool fSet[256];
	ZeroMem(&fSet, sizeof (fSet));
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++)
	{
		// Find material
		DWORD dwPix = Entry[i].GetPattern().PatternClr(0, 0);
		Palette->Colors[i] = dwPix;
		Palette->Colors[i + IFT] = dwPix | 0x0F; // IFT arbitrarily gets more blue
		fSet[i] = fSet[i + IFT] = true;
	}
	// Crosscheck colors, change equal palette entries
	for (i = 0; i < 256; i++) if (fSet[i])
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
					(rand() < RAND_MAX / 2) ? GetRedValue(Palette->Colors[i]) + 3 : GetRedValue(Palette->Colors[i]) - 3,
					(rand() < RAND_MAX / 2) ? GetGreenValue(Palette->Colors[i]) + 3 : GetGreenValue(Palette->Colors[i]) - 3,
					(rand() < RAND_MAX / 2) ? GetBlueValue(Palette->Colors[i]) + 3 : GetBlueValue(Palette->Colors[i]) - 3);
			}
}

C4TextureMap TextureMap;
