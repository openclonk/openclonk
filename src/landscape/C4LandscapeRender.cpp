/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "landscape/C4LandscapeRender.h"

#include "c4group/C4Components.h"
#include "c4group/C4GroupSet.h"
#include "graphics/C4DrawGL.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Texture.h"
#include "landscape/fow/C4FoWRegion.h"
#include "lib/StdColors.h"

#ifndef USE_CONSOLE

// Automatically reload shaders when changed at runtime?
#define AUTO_RELOAD_SHADERS

#ifdef _DEBUG

// Generate seperator textures into 3D texture so we can make sure that
// we are addressing textures using the right coordinates
#define DEBUG_SEPERATOR_TEXTURES

// Replace all textures by solid colors
//#define DEBUG_SOLID_COLOR_TEXTURES

#endif

// How much to look into each direction for bias
const int C4LR_BiasDistanceX = 7;
const int C4LR_BiasDistanceY = 7;

// Name used for the seperator texture
const char *const SEPERATOR_TEXTURE = "--SEP--";

C4LandscapeRenderGL::C4LandscapeRenderGL()
{
	ZeroMem(Surfaces, sizeof(Surfaces));
	hMaterialTexture = matMapTexture = 0;
	hVBO = 0;
	hVAOIDNoLight = 0;
	hVAOIDLight = 0;
}

C4LandscapeRenderGL::~C4LandscapeRenderGL()
{
	Clear();
}

bool C4LandscapeRenderGL::Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pTexs, C4GroupSet *pGraphics)
{
	Clear();

	// Safe info
	this->iWidth = iWidth;
	this->iHeight = iHeight;
	this->pTexs = pTexs;

	// Start timer
	this->TimerStart = std::chrono::steady_clock::now();

	// Allocate landscape textures
	if (!InitLandscapeTexture())
	{
		LogFatal("[!] Could not initialize landscape texture!");
		return false;
	}

	// Build texture, er, texture
	if (!InitMaterialTexture(pTexs))
	{
		LogFatal("[!] Could not initialize landscape textures for rendering!");
		return false;
	}
	
	// Load sclaer
	if (!LoadScaler(pGraphics))
	{
		LogFatal("[!] Could not load scaler!");
		return false;
	}

	// Load shader
	if (!LoadShaders(pGraphics))
	{
		LogFatal("[!] Could not initialize landscape shader!");
		return false;
	}

	if (!InitVBO())
	{
		LogFatal("[!] Could not initialize landscape VBO!");
		return false;
	}

	return true;
}

bool C4LandscapeRenderGL::ReInit(int32_t iWidth, int32_t iHeight)
{
	// Safe info
	this->iWidth = iWidth;
	this->iHeight = iHeight;

	// Clear old landscape textures
	for (auto & Surface : Surfaces)
	{
		delete Surface;
		Surface = nullptr;
	}

	// Allocate new landscape textures
	if (!InitLandscapeTexture())
	{
		LogFatal("[!] Could not initialize landscape texture!");
		return false;
	}
	return true;
}

void C4LandscapeRenderGL::Clear()
{
	ClearShaders();

	// free textures
	int i;
	for (i = 0; i < C4LR_SurfaceCount; i++)
	{
		delete Surfaces[i];
		Surfaces[i] = nullptr;
	}
	if (hMaterialTexture) glDeleteTextures(1, &hMaterialTexture);
	hMaterialTexture = 0;
	if (matMapTexture) glDeleteTextures(1, &matMapTexture);
	matMapTexture = 0;

	if (hVBO != 0)
	{
		glDeleteBuffers(1, &hVBO);
		hVBO = 0;
	}

	if (hVAOIDLight != 0)
	{
		pGL->FreeVAOID(hVAOIDLight);
		hVAOIDLight = 0;
	}

	if (hVAOIDNoLight != 0)
	{
		pGL->FreeVAOID(hVAOIDNoLight);
		hVAOIDNoLight = 0;
	}
}

bool C4LandscapeRenderGL::InitLandscapeTexture()
{
	// Don't round up to the nearest power-of-two here. Modern
	// hardware should handle NPOT-textures just fine, and they
	// are already used in other places in Clonk. This avoids
	// accessing one row or column of pixels below the actually
	// used surface in the shader, which can lead to strange
	// one-pixel edges at the bottom or right of the landscape
	// (see e.g. http://bugs.openclonk.org/view.php?id=771).
	int iSfcWdt = iWidth;
	int iSfcHgt = iHeight;

	// Create our surfaces
	for(auto & Surface : Surfaces)
	{
		Surface = new C4Surface();
		if(!Surface->Create(iSfcWdt, iSfcHgt))
			return false;
	}

	return true;
}

bool C4LandscapeRenderGL::InitMaterialTexture(C4TextureMap *pTexs)
{

	// Populate our map with all needed textures
	MaterialTextureMap.emplace_back("");
	AddTexturesFromMap(pTexs);

	// Determine depth to use
	iMaterialTextureDepth = 2*MaterialTextureMap.size();
	int32_t iNormalDepth = iMaterialTextureDepth / 2;

	// Find the largest texture
	C4Texture *pTex; C4Surface *pRefSfc = nullptr;
	for(int iTexIx = 0; (pTex = pTexs->GetTexture(pTexs->GetTexture(iTexIx))); iTexIx++)
		if(C4Surface *pSfc = pTex->Surface32)
			if (!pRefSfc || pRefSfc->Wdt < pSfc->Wdt || pRefSfc->Hgt < pSfc->Hgt)
				pRefSfc = pSfc;
	if(!pRefSfc)
		return false;

	// Get size for our textures. We might be limited by hardware
	int iTexWdt = pRefSfc->Wdt, iTexHgt = pRefSfc->Hgt;
	GLint iMaxTexSize, iMaxTexLayers;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &iMaxTexSize);
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &iMaxTexLayers);
	if (iTexWdt > iMaxTexSize || iTexHgt > iMaxTexSize)
	{
		iTexWdt = std::min(iTexWdt, iMaxTexSize);
		iTexHgt = std::min(iTexHgt, iMaxTexSize);
		LogF("   gl: Material textures too large, GPU only supports %dx%d! Cropping might occur!", iMaxTexSize, iMaxTexSize);
	}
	if(iMaterialTextureDepth >= iMaxTexLayers)
	{
		LogF("   gl: Too many material textures! GPU only supports 3D texture depth of %d!", iMaxTexSize);
		return false;
	}
	iMaterialWidth = iTexWdt;
	iMaterialHeight = iTexHgt;

	// Compose together data of all textures
	const int iTexSize = iTexWdt * iTexHgt * C4Draw::COLOR_DEPTH_BYTES;
	const int iSize = iTexSize * iMaterialTextureDepth;
	BYTE *pData = new BYTE [iSize];
	for(int i = 0; i < iMaterialTextureDepth; i++)
	{
		BYTE *p = pData + i * iTexSize;
		// Get texture at position
		StdStrBuf Texture;
		bool fNormal = i >= iNormalDepth;
		if(i < int32_t(MaterialTextureMap.size()))
			Texture.Ref(MaterialTextureMap[i]);
		else if(fNormal && i < iNormalDepth + int32_t(MaterialTextureMap.size()))
			Texture.Format("%s_NRM", MaterialTextureMap[i-iNormalDepth].getData());
		// Try to find the texture
		C4Texture *pTex; C4Surface *pSurface;
		if((pTex = pTexs->GetTexture(Texture.getData())) && (pSurface = pTex->Surface32))
		{
#ifdef DEBUG_SOLID_COLOR_TEXTURES
			// Just write a solid color that depends on the texture index
			DWORD *texdata = reinterpret_cast<DWORD *>(p);
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = RGBA((iTex & 48), (iTex & 3) * 16, (i & 12) * 4, 255);
			continue;
#else
			// Size recheck. It's fine if this texture's size is a divisor
			// of the maximum texture size, because then we can just tile
			// the smaller texture.
			if(pSurface->Wdt != iTexWdt || pSurface->Hgt != iTexHgt)
				if (iTexWdt % pSurface->Wdt != 0 || iTexHgt % pSurface->Hgt != 0)
					LogF("   gl: texture %s size mismatch (%dx%d vs %dx%d)!", Texture.getData(), pSurface->Wdt, pSurface->Hgt, iTexWdt, iTexHgt);

			// Copy bytes
			DWORD *texdata = reinterpret_cast<DWORD *>(p);
			pSurface->Lock();
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = pSurface->GetPixDw(x % pSurface->Wdt, y % pSurface->Hgt, false);
			pSurface->Unlock();
			continue;
#endif
		}
		// Seperator texture?
		if(SEqual(Texture.getData(), SEPERATOR_TEXTURE))
		{
			// Make some ugly stripes
			DWORD *texdata = reinterpret_cast<DWORD *>(p);
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = ((x + y) % 32 < 16 ? RGBA(255, 0, 0, 255) : RGBA(0, 255, 255, 255));
			continue;
		}
		// If we didn't "continue" yet, we haven't written the texture yet.
		// Make color texture transparent, and normal texture flat.
		if (fNormal)
		{
			DWORD *texdata = reinterpret_cast<DWORD *>(p);
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = RGBA(127, 127, 255, 255);
		}
		else
			memset(p, 0, iTexSize);
	}

	// Clear error error(s?)
	while(glGetError()) {}
	
	// Alloc 1D matmap texture
	glGenTextures(1, &matMapTexture);

	// Alloc 2D texture array
	glGenTextures(1, &hMaterialTexture);

	// Generate textures
	int iSizeSum = 0;

	// Select texture
	glBindTexture(GL_TEXTURE_2D_ARRAY, hMaterialTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// We fully expect to tile these
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	// Make it happen!
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, iTexWdt, iTexHgt, iMaterialTextureDepth, 0, GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8_REV,
				pData);

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	   
	// Statistics
	iSizeSum += iTexWdt * iTexHgt * iMaterialTextureDepth * C4Draw::COLOR_DEPTH_BYTES;
	
	// Dispose of data
	delete [] pData;
	
	// Check whether we were successful
	if(int err = glGetError())
	{
		LogF("   gl: Could not load textures (error %d)", err);
		return false;
	}

	// Announce the good news
	LogF("  gl: Texturing uses %d slots at %dx%d (%d MB total)",
		static_cast<int>(MaterialTextureMap.size()),
		iMaterialWidth, iMaterialHeight,
		iSizeSum / 1000000);

	return true;
}

C4Rect C4LandscapeRenderGL::GetAffectedRect(C4Rect Rect)
{
	Rect.Enlarge(C4LR_BiasDistanceX, C4LR_BiasDistanceY);
	return Rect;
}

void C4LandscapeRenderGL::Update(C4Rect To, C4Landscape *pSource)
{
	// clip to landscape size
	To.Intersect(C4Rect(0,0,iWidth,iHeight));
	// everything clipped?
	if (To.Wdt<=0 || To.Hgt<=0) return;

	// Lock surfaces
	// We clear the affected region here because ClearBoxDw allocates the
	// main memory buffer for the box, so that only that box needs to be
	// sent to the gpu, and not the whole texture, or every pixel
	// separately. It's an important optimization.
	for (auto & Surface : Surfaces)
	{
		if (!Surface->Lock()) return;
		Surface->ClearBoxDw(To.x, To.y, To.Wdt, To.Hgt);
	}

	// Initialize up & down placement arrays. These arrays are always updated
	// so that they contain the placement sums of C4LR_BiasDistanceY pixels
	// above and below the current row.
	int x, y;
	int placementSumsWidth = C4LR_BiasDistanceX * 2 + To.Wdt;
	int *placementSumsUp = new int [placementSumsWidth * 2];
	int *placementSumsDown = placementSumsUp + placementSumsWidth;
	for(x = 0; x < placementSumsWidth; x++)
	{
		placementSumsUp[x] = 0;
		placementSumsDown[x] = 0;
		if (To.x + x - C4LR_BiasDistanceX < 0 || To.x + x - C4LR_BiasDistanceX >= iWidth) continue;
		for(y = 1; y <= std::min(C4LR_BiasDistanceY, To.y); y++)
			placementSumsUp[x] += pSource->_GetPlacement(To.x+x-C4LR_BiasDistanceX, To.y-y);
		for(y = 1; y <= std::min(C4LR_BiasDistanceY, iHeight - 1 - To.y); y++)
			placementSumsDown[x] += pSource->_GetPlacement(To.x+x-C4LR_BiasDistanceX, To.y+y);
	}

	// Get tex refs (shortcut, we will use them quite heavily)
	C4TexRef *texture[C4LR_SurfaceCount];
	x = y = 0;
	for(int i = 0; i < C4LR_SurfaceCount; i++)
		texture[i] = Surfaces[i]->texture.get();

	// Go through it from top to bottom
	for(y = 0; y < To.Hgt; y++)
	{
		// Initialize left & right placement sums. These are meant to contain
		// the placement sum of a (C4LR_BiasDistanceX, 2*C4LR_BiasDistanceY+1)
		// rectangle left/right of the current pixel. So we initialise it to
		// be correct at x=0. Note that the placementSum arrays don't contain
		// information about the current row, therefore we need a special case
		// for those pixels.
		int sumLeft = 0, sumRight = 0;
		for(x = 1; x <= std::min(C4LR_BiasDistanceX, To.x); x++)
			sumLeft += pSource->_GetPlacement(To.x-x,To.y+y);
		for(x = 1; x <= std::min(C4LR_BiasDistanceX, iWidth - 1 - To.x ); x++)
			sumRight += pSource->_GetPlacement(To.x+x,To.y+y);
		for (int i = 1; i <= C4LR_BiasDistanceX; i++) {
			sumLeft += placementSumsUp[C4LR_BiasDistanceX - i];
			sumLeft += placementSumsDown[C4LR_BiasDistanceX - i];
			sumRight += placementSumsUp[C4LR_BiasDistanceX + i];
			sumRight += placementSumsDown[C4LR_BiasDistanceX + i];
		}

		// Initialise up & down sums. Same principle as above, but slightly
		// easier as we do not miss pixels if we just use the placement sums.
		int sumUp = 0, sumDown = 0;
		for (int i = -C4LR_BiasDistanceX; i <= C4LR_BiasDistanceX; i++) {
			sumUp += placementSumsUp[C4LR_BiasDistanceX + i];
			sumDown += placementSumsDown[C4LR_BiasDistanceX + i];
		}

		for(x = 0; x < To.Wdt; x++)
		{
			int pixel = pSource->_GetPix(To.x+x, To.y+y);
			int placement = pSource->_GetPlacement(To.x+x, To.y+y);

			// Calculate bias. The scale here is the size of the rectangle (see above)
			const int horizontalFactor = C4LR_BiasDistanceX * (2 * C4LR_BiasDistanceY + 1);
			int horizontalBias = std::max(0, placement * horizontalFactor - sumRight) -
			                     std::max(0, placement * horizontalFactor - sumLeft);
			const int verticalFactor = C4LR_BiasDistanceY * (2 * C4LR_BiasDistanceX + 1);
			int verticalBias = std::max(0, placement * verticalFactor - sumDown) -
			                   std::max(0, placement * verticalFactor - sumUp);

			// Maximum placement differences that make a difference in the result,  after which we are at the limits of
			// what can be packed into a byte
			const int maximumPlacementDifference = 40;
			int horizontalBiasScaled = Clamp(horizontalBias * 127 / maximumPlacementDifference / horizontalFactor + 128, 0, 255);
			int verticalBiasScaled = Clamp(verticalBias * 127 / maximumPlacementDifference / verticalFactor + 128, 0, 255);

			// Collect data to save per pixel
			unsigned char data[C4LR_SurfaceCount * 4];
			memset(data, 0, sizeof(data));

			data[C4LR_Material] = pixel;
			data[C4LR_BiasX] = horizontalBiasScaled;
			data[C4LR_BiasY] = verticalBiasScaled;
			data[C4LR_Scaler] = CalculateScalerBitmask(x, y, To, pSource);
			data[C4LR_Place] = placement;

			for(int i = 0; i < C4LR_SurfaceCount; i++)
				texture[i]->SetPix(To.x+x, To.y+y, 
					RGBA(data[i*4+0], data[i*4+1], data[i*4+2], data[i*4+3]));

			// Update sums (last column would be out-of-bounds, and not
			// necessary as we will re-initialise it for the next row)
			if (x < To.Wdt - 1) {
				sumLeft -= placementSumsUp[x] + placementSumsDown[x];
				sumLeft += placementSumsUp[x + C4LR_BiasDistanceX] + placementSumsDown[x + C4LR_BiasDistanceX];
				sumRight -= placementSumsUp[x + C4LR_BiasDistanceX + 1] + placementSumsDown[x + C4LR_BiasDistanceX + 1];
				sumUp -= placementSumsUp[x];
				sumDown -= placementSumsDown[x];
				sumRight += placementSumsUp[x + 2 * C4LR_BiasDistanceX + 1] + placementSumsDown[x + 2 * C4LR_BiasDistanceX + 1];
				sumUp += placementSumsUp[x + 2 * C4LR_BiasDistanceX + 1];
				sumDown += placementSumsDown[x + 2 * C4LR_BiasDistanceX + 1];
			}

			// Update left & right for next pixel in line
			if(x + To.x + 1 < iWidth)
				sumRight -= pSource->_GetPlacement(To.x+x + 1, To.y+y);
			if(To.x+x + C4LR_BiasDistanceX + 1 < iWidth)
				sumRight += pSource->_GetPlacement(To.x+x + C4LR_BiasDistanceX + 1, To.y+y);
			sumLeft += placement;
			if(To.x+x - C4LR_BiasDistanceX >= 0)
				sumLeft -= pSource->_GetPlacement(To.x+x - C4LR_BiasDistanceX, To.y+y);

			// Update up & down arrays (for next line already)
			if (To.x + x >= C4LR_BiasDistanceX) {
				if (To.y + y + 1 < iHeight)
					placementSumsDown[x] -= pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y + 1);
				if (To.y + y + C4LR_BiasDistanceY + 1 < iHeight)
					placementSumsDown[x] += pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y + C4LR_BiasDistanceY + 1);
				if (To.y + y - C4LR_BiasDistanceY >= 0)
					placementSumsUp[x] -= pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y - C4LR_BiasDistanceY);
				placementSumsUp[x] += pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y);
			}
		}

		// Finish updating up & down arrays for the next line
		if (To.x + x >= C4LR_BiasDistanceX)
		{
			for (; x < std::min(placementSumsWidth, iWidth - To.x + C4LR_BiasDistanceX); x++) {
				if (To.y + y + 1 < iHeight)
					placementSumsDown[x] -= pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y + 1);
				if (To.y + y + C4LR_BiasDistanceY + 1 < iHeight)
					placementSumsDown[x] += pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y + C4LR_BiasDistanceY + 1);
				if (To.y + y - C4LR_BiasDistanceY >= 0)
					placementSumsUp[x] -= pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y - C4LR_BiasDistanceY);
				placementSumsUp[x] += pSource->_GetPlacement(To.x + x - C4LR_BiasDistanceX, To.y + y);
			}
		}
	}

	// done
	delete[] placementSumsUp;
	for (auto & Surface : Surfaces)
		Surface->Unlock();
}

/** Returns the data used for the scaler shader for the given pixel. It is a 8-bit bitmask. The bits stand for the 8
    neighbouring pixels in this order:
	  1 2 3
	  4 . 5
	  6 7 8
	... and denote whether the pixels belongs to the same group as this pixel.
	*/
int C4LandscapeRenderGL::CalculateScalerBitmask(int x, int y, C4Rect To, C4Landscape *pSource)
{
	int pixel = pSource->_GetPix(To.x+x, To.y+y);
	int placement = pSource->_GetPlacement(To.x+x, To.y+y);

	int neighbours[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	if(To.y+y > 0)
	{
		if(To.x+x > 0)
			neighbours[0] = pSource->_GetPix(To.x+x-1, To.y+y-1);
		neighbours[1] = pSource->_GetPix(To.x+x, To.y+y-1);
		if(To.x+x < iWidth-1)
			neighbours[2] = pSource->_GetPix(To.x+x+1, To.y+y-1);
	}
	if(To.x+x > 0)
		neighbours[3] = pSource->_GetPix(To.x+x-1, To.y+y);
	if(To.x+x < iWidth-1)
		neighbours[4] = pSource->_GetPix(To.x+x+1, To.y+y);
	if(To.y+y < iHeight-1)
	{
		if(To.x+x > 0)
			neighbours[5] = pSource->_GetPix(To.x+x-1, To.y+y+1);
		neighbours[6] = pSource->_GetPix(To.x+x, To.y+y+1);
		if(To.x+x < iWidth-1)
			neighbours[7] = pSource->_GetPix(To.x+x+1, To.y+y+1);
	}

	// Look for highest-placement material in our surroundings
	int maxPixel = pixel, maxPlacement = placement;
	for(int neighbour : neighbours)
	{
		int tempPlacement = MatPlacement(PixCol2Mat(neighbour));
		if(tempPlacement > maxPlacement || (tempPlacement == maxPlacement && neighbour > maxPixel) )
		{
			maxPixel = neighbour;
			maxPlacement = tempPlacement;
		}
	}

	// Scaler calculation depends on whether this is the highest-placement material around
	int scaler = 0;
	if(maxPixel == pixel)
	{
		// If yes, we consider all other materials as "other"
		for(int i = 0; i < 8; i++)
			if(neighbours[i] == pixel)
				scaler += (1<<i);

	} else {

		// Otherwise, we *only* consider the highest-placement material as "other"
		for(int i = 0; i < 8; i++)
			if(neighbours[i] != maxPixel)
				scaler += (1<<i);
	}
	return scaler;
}

const char *C4LandscapeRenderGL::UniformNames[C4LRU_Count+1];

bool C4LandscapeRenderGL::LoadShader(C4GroupSet *pGroups, C4Shader& shader, const char* name, int ssc)
{
	// Setup #defines
	shader.AddDefine("OPENCLONK");
	shader.AddDefine("OC_LANDSCAPE");
	if(ssc & C4SSC_LIGHT) shader.AddDefine("OC_DYNAMIC_LIGHT"); // sample light from light texture

	// Create vertex shader
	shader.LoadVertexSlices(pGroups, "LandscapeVertexShader.glsl");

	// Then load slices for fragment shader
	shader.LoadFragmentSlices(pGroups, "CommonShader.glsl");
	shader.LoadFragmentSlices(pGroups, "LandscapeShader.glsl");

	// Categories for script shaders.
	shader.SetScriptCategories({"Common", "Landscape"});

	// Make attribute name map
	const char* AttributeNames[C4LRA_Count + 1];
	AttributeNames[C4LRA_Position] = "oc_Position";
	AttributeNames[C4LRA_LandscapeTexCoord] = "oc_LandscapeTexCoord";
	AttributeNames[C4LRA_LightTexCoord] = "oc_LightTexCoord"; // unused if no dynamic light
	AttributeNames[C4LRA_Count] = nullptr;

	// Initialise!
	if (!shader.Init(name, UniformNames, AttributeNames)) {
		shader.ClearSlices();
		return false;
	}

	return true;
}

bool C4LandscapeRenderGL::LoadShaders(C4GroupSet *pGroups)
{
	// First, clear out all existing shaders
	ClearShaders();

	// Make uniform name map
	ZeroMem(UniformNames, sizeof(UniformNames));
	UniformNames[C4LRU_ProjectionMatrix]  = "projectionMatrix";
	UniformNames[C4LRU_LandscapeTex]      = "landscapeTex";
	UniformNames[C4LRU_ScalerTex]         = "scalerTex";
	UniformNames[C4LRU_MaterialTex]       = "materialTex";
	UniformNames[C4LRU_LightTex]          = "lightTex";
	UniformNames[C4LRU_AmbientTex]        = "ambientTex";
	UniformNames[C4LRU_Gamma]             = "gamma";
	UniformNames[C4LRU_Resolution]        = "resolution";
	UniformNames[C4LRU_Center]            = "center";
	UniformNames[C4LRU_MatMapTex]         = "matMapTex";
	UniformNames[C4LRU_MaterialDepth]     = "materialDepth";
	UniformNames[C4LRU_MaterialSize]      = "materialSize";
	UniformNames[C4LRU_AmbientBrightness] = "ambientBrightness";
	UniformNames[C4LRU_AmbientTransform]  = "ambientTransform";
	UniformNames[C4LRU_Modulation]        = "clrMod";
	UniformNames[C4LRU_FrameCounter]      = "frameCounter";
	UniformNames[C4LRU_Time]              = "time";

	if(!LoadShader(pGroups, Shader, "landscape", 0))
		return false;
	if(!LoadShader(pGroups, ShaderLight, "landscapeLight", C4SSC_LIGHT))
		return false;

	return true;
}

bool C4LandscapeRenderGL::InitVBO()
{
	// Our VBO needs to hold 4 vertices with 6 floats each.
	assert(hVBO == 0);
	glGenBuffers(1, &hVBO);
	glBindBuffer(GL_ARRAY_BUFFER, hVBO);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), nullptr, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Also allocate the VAO IDs
	assert(hVAOIDLight == 0);
	assert(hVAOIDNoLight == 0);
	hVAOIDLight = pGL->GenVAOID();
	hVAOIDNoLight = pGL->GenVAOID();
	return true;
}

void C4LandscapeRenderGL::ClearShaders()
{
	if (Shader.Initialised())
	{
		Shader.Clear();
		Shader.ClearSlices();
	}

	if (ShaderLight.Initialised())
	{
		ShaderLight.Clear();
		ShaderLight.ClearSlices();
	}
}

bool C4LandscapeRenderGL::LoadScaler(C4GroupSet *pGroups)
{
	// Search for scaler
	C4Group *pGroup = pGroups->FindEntry(C4CFN_LandscapeScaler);
	if(!pGroup) return false;
	// Load scaler from group
	if(!fctScaler.Load(*pGroup, C4CFN_LandscapeScaler, C4FCT_Full, C4FCT_Full, false, 0))
		return false;
	// Check size
	const int iOrigWdt = 8 * 3, iOrigHgt = 4 * 8 * 3;
	const int iFactor = fctScaler.Wdt / iOrigWdt;
	if(fctScaler.Wdt != iFactor * iOrigWdt || fctScaler.Hgt != iFactor * iOrigHgt)
	{
		LogF("  gl: Unexpected scaler size - should be multiple of %dx%d!", iOrigWdt, iOrigHgt);
		return false;
	}
	// Walk through all lookups we have in the texture and decide where
	// to look for the "other" pixel. This might not be unique later on,
	// so it is a good idea to have a proper priority order here.
	fctScaler.Surface->Lock();
	int i;
	for (i = 0; i < 8 * 4 * 8; i++) {
		// Decode from ID what pixels are expected to be set in this case
		enum Px      { NW,  N, NE,  W,  E, SW, S, SE, X };
		int p_x[9] = { -1,  0,  1, -1,  1, -1, 0, 1,  0 };
		int p_y[9] = { -1, -1, -1,  0,  0,  1, 1, 1,  0 };
		bool pxAt[X];
		for(int j = 0; j < X; j++)
			pxAt[j] = !!(i & (1 << j));
		// Oc = octant borders. Set up arrays to get righthand border
		// of an octant, in a way that we can easily rotate further.
		enum Oc { NWW, NEE, SWW, SEE, NNW, NNE, SSW, SSE };
		int p2a[8] = { 5, 6, 7, 4, 0, 3, 2, 1 };
		Oc a2o[8] = { SEE, SSE, SSW, SWW, NWW, NNW, NNE, NEE };
		// Decide in which octant we want to interpolate towards
		// which pixel. Pick the nearest unset pixel using a special
		// priority order.
		Px opx[8] = { X,X,X,X,X,X,X,X };
		#define INTERPOLATE(x,da) do { \
			int y = a2o[(8+p2a[x]+(da)) % 8];\
			if (!pxAt[x] && opx[y] == X) opx[y] = x; \
		} while(false)
		for(int j = 0; j < 4; j++) {
			// vertical
			INTERPOLATE(N, j); INTERPOLATE(N, -j-1);
			INTERPOLATE(S, j); INTERPOLATE(S, -j-1);
			// horizontal
			INTERPOLATE(W, j); INTERPOLATE(W, -j-1);
			INTERPOLATE(E, j); INTERPOLATE(E, -j-1);
			// diagonals
			INTERPOLATE(NW, j); INTERPOLATE(NW, -j-1);
			INTERPOLATE(SW, j); INTERPOLATE(SW, -j-1);
			INTERPOLATE(NE, j); INTERPOLATE(NE, -j-1);
			INTERPOLATE(SE, j); INTERPOLATE(SE, -j-1);			
		}
		// Decide in which octants we will not interpolate normals.
		// It doesn't make sense when there's another material in that
		// general direction, as then the bias of that will factor into
		// the interpolation, giving bright borders on dark shading,
		// and vice-versa.
		bool noNormals[8];
		noNormals[NNW] = noNormals[NWW] = !pxAt[W] || !pxAt[NW] || !pxAt[N];
		noNormals[NNE] = noNormals[NEE] = !pxAt[E] || !pxAt[NE] || !pxAt[N];
		noNormals[SSW] = noNormals[SWW] = !pxAt[W] || !pxAt[SW] || !pxAt[S];
		noNormals[SSE] = noNormals[SEE] = !pxAt[E] || !pxAt[SE] || !pxAt[S];
		// Set blue and green components to relative coordinates of
		// "other" pixel, and alpha to mix param for normals
		const int x0 = (i % 8) * 3 * iFactor;
		const int y0 = (i / 8) * 3 * iFactor;
		const int iPxs = 3 * iFactor;
		int y, x;

		for(y = 0; y < iPxs; y++)
		{
			for(x = 0; x < iPxs; x++)
			{
				// Find out in which octagon we are
				int oct = 0;
				if(2 * x >= iPxs) oct+=1;
				if(2 * y >= iPxs) oct+=2;
				if((x >= y) != (x >= iPxs - y)) oct+=4;
				// Get pixel, do processing
				DWORD pix = fctScaler.Surface->GetPixDw(x0+x, y0+y, false);
				BYTE val = GetGreenValue(pix);
				if(val >= 250) val = 255;
				BYTE bx = 64 * (p_x[opx[oct]] + 1);
				BYTE by = 64 * (p_y[opx[oct]] + 1);
				BYTE bn = (noNormals[oct] ? 255 : 1);
				fctScaler.Surface->SetPixDw(x0+x, y0+y, RGBA(val, bx, by, bn));
			}
		}
	}
	return fctScaler.Surface->Unlock();
}

int32_t C4LandscapeRenderGL::LookupTextureTransition(const char *szFrom, const char *szTo)
{
	// Is this actually a transition? Otherwise we're looking for a single texture
	bool fTransit = !SEqual(szFrom, szTo);
	// Look for a position in the map where the textures appear in sequence
	uint32_t i;
	for(i = 1; i < MaterialTextureMap.size(); i++)
	{
		if(SEqual(szFrom, MaterialTextureMap[i].getData()))
		{
			// Single texture: We're done
			if(!fTransit) return i;
			// Check next texture as well
			if(i + 1 >= MaterialTextureMap.size())
				return -1;
			if(SEqual(szTo, MaterialTextureMap[i+1].getData()))
				return i;
		}
	}
	return -1;
}

void C4LandscapeRenderGL::AddTextureTransition(const char *szFrom, const char *szTo)
{
	// Empty?
	if (!szFrom || !szTo) return;
	// First try the lookup (both directions)
	if (LookupTextureTransition(szFrom, szTo) >= 0) return;
	if (LookupTextureTransition(szTo, szFrom) >= 0) return;
	// Single texture? Add it as single
	if (SEqual(szTo, szFrom))
		MaterialTextureMap.emplace_back(szFrom);
	// Have one of the textures at the end of the list?
	else if(SEqual(MaterialTextureMap.back().getData(), szFrom))
		MaterialTextureMap.emplace_back(szTo);
	else if(SEqual(MaterialTextureMap.back().getData(), szTo))
		MaterialTextureMap.emplace_back(szFrom);
	else
	{
		// Otherwise add both
		MaterialTextureMap.emplace_back(szFrom);
		MaterialTextureMap.emplace_back(szTo);
	}
}

void C4LandscapeRenderGL::AddTextureAnim(const char *szTextureAnim)
{
	if(!szTextureAnim) return;
#ifdef DEBUG_SEPERATOR_TEXTURES
	// Save back count of textures at start
	uint32_t iStartTexCount = MaterialTextureMap.size();
#endif
	// Add all individual transitions
	const char *pFrom = szTextureAnim;
	for(;;)
	{
		// Get next phase
		const char *pTo = strchr(pFrom, '-');
		if(!pTo) pTo = szTextureAnim; else pTo++;
		// Add transition
		StdStrBuf From, To;
		From.CopyUntil(pFrom, '-');
		To.CopyUntil(pTo, '-');
		AddTextureTransition(From.getData(), To.getData());
		// Advance
		if(pTo == szTextureAnim) break;
		pFrom = pTo;
	}
#ifdef DEBUG_SEPERATOR_TEXTURES
	// Add a seperator texture, if we added any new ones
	if(MaterialTextureMap.size() > iStartTexCount)
		MaterialTextureMap.push_back(StdCopyStrBuf(SEPERATOR_TEXTURE));
#endif
}

void C4LandscapeRenderGL::AddTexturesFromMap(C4TextureMap *pMap)
{
	// Go through used texture (animations) and add all phases to our map

	// Note: We can be smarter here, for example add longer animations
	//       first in order to make better reuse of 3D texture slots.
	//       We could even make a full-blown optimization problem out of it.
	//       Future work...

	const C4TexMapEntry *pEntry;
	for(int32_t i = 0; (pEntry = pMap->GetEntry(i)); i++)
		// ToDo: Properly handle jumping back
		AddTextureAnim(pEntry->GetTextureName());

}

void C4LandscapeRenderGL::BuildMatMap(uint32_t *pTex)
{
	// TODO: Still merely an inefficient placeholder for things to come...

	// Build material-texture map (depth parameter where to find appropriate texture)
	for(int pix = 0; pix < 256; pix++)
	{
		// Look up indexed entry
		const C4TexMapEntry *pEntry = pTexs->GetEntry(PixCol2Tex(BYTE(pix)));
		if(!pEntry->GetTextureName())
		{
			// Undefined textures transparent
			pTex[2*pix] = 0;
			pTex[2*pix+1] = RGBA(0,0,0,255);
			continue;
		}

		// Got animation?
		int iPhases = 1; const char *p = pEntry->GetTextureName();
		while((p = strchr(p, '-'))) { p++; iPhases++; }
		// Hard-coded hack. Fix me!
		C4Material *pMaterial = pEntry->GetMaterial();
		const int iPhaseLength = pMaterial->AnimationSpeed;
		float phase = 0;
		if (iPhases > 1) {
			phase = C4TimeMilliseconds::Now().AsInt() % (iPhases * iPhaseLength);
			phase /= iPhaseLength;
		}

		// Find our transition
		const char *pFrom = pEntry->GetTextureName();
		float gTexCoo = 0;
		for(int iP = 0;; iP++)
		{
			// Get next phase
			const char *pTo = strchr(pFrom, '-');
			if(!pTo) pTo = pEntry->GetTextureName(); else pTo++;
			// Add transition
			if(iP == int(phase))
			{
				StdStrBuf From, To;
				From.CopyUntil(pFrom, '-');
				To.CopyUntil(pTo, '-');
				// Find transition
				int iTrans;
				if ((iTrans = LookupTextureTransition(From.getData(), To.getData())) >= 0)
					gTexCoo = float(iTrans) + fmod(phase, 1.0f);
				else if ((iTrans = LookupTextureTransition(To.getData(), From.getData())) >= 0)
					gTexCoo = float(iTrans) + 1.0 - fmod(phase, 1.0f);
				break;
			}
			// Advance
			pFrom = pTo;
		}

		// Assign texture
		int iTexCoo = int((gTexCoo * 256.0 / iMaterialTextureDepth) + 0.5);
		pTex[2*pix] = RGBA(
			Clamp(pMaterial->LightEmit[0], 0, 255),
			Clamp(pMaterial->LightEmit[1], 0, 255),
			Clamp(pMaterial->LightEmit[2], 0, 255),
			iTexCoo);
		pTex[2*pix+1] = RGBA(
			Clamp(pMaterial->LightSpot[0], 0, 255),
			Clamp(pMaterial->LightSpot[1], 0, 255),
			Clamp(pMaterial->LightSpot[2], 0, 255),
			Clamp(pMaterial->LightAngle, 0, 255));
	}
}

void C4LandscapeRenderGL::Draw(const C4TargetFacet &cgo, const C4FoWRegion *Light, uint32_t clrMod)
{
	// Must have GL and be initialized
	if(!pGL && !Shader.Initialised() && !ShaderLight.Initialised()) return;
	
	// prepare rendering to surface
	C4Surface *sfcTarget = cgo.Surface;
	if (!pGL->PrepareRendering(sfcTarget)) return;

	// Choose the right shader depending on whether we have dynamic lighting or not
	const C4Shader* shader = &Shader;
	if (Light) shader = &ShaderLight;
	if (!shader->Initialised()) return;

	// Activate shader
	C4ShaderCall ShaderCall(shader);
	ShaderCall.Start();

	// Bind data
	ShaderCall.SetUniformMatrix4x4(C4LRU_ProjectionMatrix, pGL->GetProjectionMatrix());
	ShaderCall.SetUniform3fv(C4LRU_Gamma, 1, pGL->gammaOut);
	ShaderCall.SetUniform2f(C4LRU_Resolution, Surfaces[0]->Wdt, Surfaces[0]->Hgt);
	float centerX = float(cgo.TargetX)+float(cgo.Wdt)/2,
		  centerY = float(cgo.TargetY)+float(cgo.Hgt)/2;
	ShaderCall.SetUniform2f(C4LRU_Center,
	                        centerX / float(Surfaces[0]->Wdt),
	                        centerY / float(Surfaces[0]->Hgt));
	ShaderCall.SetUniform1f(C4LRU_MaterialDepth, float(iMaterialTextureDepth));
	ShaderCall.SetUniform2f(C4LRU_MaterialSize,
	                        float(iMaterialWidth) / ::Game.C4S.Landscape.MaterialZoom,
	                        float(iMaterialHeight) / ::Game.C4S.Landscape.MaterialZoom);
	const float fMod[4] = {
		((clrMod >> 16) & 0xff) / 255.0f,
		((clrMod >>  8) & 0xff) / 255.0f,
		((clrMod      ) & 0xff) / 255.0f,
		((clrMod >> 24) & 0xff) / 255.0f
	};
	ShaderCall.SetUniform4fv(C4LRU_Modulation, 1, fMod);

	if (Light)
	{
		const FLOAT_RECT ViewportRect = Light->getViewportRegion();
		const C4Rect ClipRect = pDraw->GetClipRect();
		const C4Rect OutRect = pDraw->GetOutRect();
		float ambientTransform[6];
		Light->getFoW()->Ambient.GetFragTransform(ViewportRect, ClipRect, OutRect, ambientTransform);
		ShaderCall.SetUniformMatrix2x3fv(C4LRU_AmbientTransform, 1, ambientTransform);
		ShaderCall.SetUniform1f(C4LRU_AmbientBrightness, Light->getFoW()->Ambient.GetBrightness());
	}

	// time information
	ShaderCall.SetUniform1i(C4LRU_FrameCounter, ::Game.FrameCounter);
	const auto now = std::chrono::steady_clock::now();
	const int epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now - TimerStart).count();
	ShaderCall.SetUniform1i(C4LRU_Time, epoch);

	pDraw->scriptUniform.Apply(ShaderCall);

	// Start binding textures
	if(shader->HaveUniform(C4LRU_LandscapeTex))
	{
		GLint iLandscapeUnits[C4LR_SurfaceCount];
		for(int i = 0; i < C4LR_SurfaceCount; i++)
		{
			iLandscapeUnits[i] = ShaderCall.AllocTexUnit(-1) - GL_TEXTURE0;
			glBindTexture(GL_TEXTURE_2D, Surfaces[i]->texture->texName);
			if (pGL->Zoom != 1.0)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			}
		}
		ShaderCall.SetUniform1iv(C4LRU_LandscapeTex, C4LR_SurfaceCount, iLandscapeUnits);
	}
	if(Light && ShaderCall.AllocTexUnit(C4LRU_LightTex))
	{
		glBindTexture(GL_TEXTURE_2D, Light->getSurfaceName());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if(Light && ShaderCall.AllocTexUnit(C4LRU_AmbientTex))
	{
		glBindTexture(GL_TEXTURE_2D, Light->getFoW()->Ambient.Tex);
	}
	if(ShaderCall.AllocTexUnit(C4LRU_ScalerTex))
	{
		glBindTexture(GL_TEXTURE_2D, fctScaler.Surface->texture->texName);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if(ShaderCall.AllocTexUnit(C4LRU_MaterialTex))
	{
		glBindTexture(GL_TEXTURE_2D_ARRAY, hMaterialTexture);
	}
	if(ShaderCall.AllocTexUnit(C4LRU_MatMapTex))
	{
		uint32_t MatMap[2*256];
		BuildMatMap(MatMap);
		glBindTexture(GL_TEXTURE_1D, matMapTexture);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 2*256, 0, GL_RGBA, GL_UNSIGNED_BYTE, MatMap);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// Calculate coordinates into landscape texture
	FLOAT_RECT fTexBlt;
	float fx = float(cgo.TargetX), fy = float(cgo.TargetY);
	fTexBlt.left  = fx / Surfaces[0]->Wdt;
	fTexBlt.top   = fy / Surfaces[0]->Hgt;
	fTexBlt.right = (fx + float(cgo.Wdt)) / Surfaces[0]->Wdt;
	fTexBlt.bottom= (fy + float(cgo.Hgt)) / Surfaces[0]->Hgt;

	// Calculate coordinates on screen (zoomed!)
	FLOAT_RECT tTexBlt;
	float tx = float(cgo.X), ty = float(cgo.Y);
	pGL->ApplyZoom(tx, ty);
	tTexBlt.left  = tx;
	tTexBlt.top   = ty;
	tTexBlt.right = tx + float(cgo.Wdt) * pGL->Zoom;
	tTexBlt.bottom= ty + float(cgo.Hgt) * pGL->Zoom;

	// Blend it
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Prepare vertex data
	float vtxData[24];
	float* pos = &vtxData[0];
	float* tex = &vtxData[8];
	float* lightTex = &vtxData[16];

	pos[0] = tTexBlt.left;
	pos[1] = tTexBlt.top;
	pos[2] = tTexBlt.right;
	pos[3] = tTexBlt.top;
	pos[4] = tTexBlt.left;
	pos[5] = tTexBlt.bottom;
	pos[6] = tTexBlt.right;
	pos[7] = tTexBlt.bottom;

	tex[0] = fTexBlt.left;
	tex[1] = fTexBlt.top;
	tex[2] = fTexBlt.right;
	tex[3] = fTexBlt.top;
	tex[4] = fTexBlt.left;
	tex[5] = fTexBlt.bottom;
	tex[6] = fTexBlt.right;
	tex[7] = fTexBlt.bottom;

	unsigned int nFloats = 16;
	if (Light)
	{
		FLOAT_RECT lTexBlt;
		const C4Rect LightRect = Light->getRegion();
		int32_t iLightWdt = Light->getSurfaceWidth(),
			iLightHgt = Light->getSurfaceHeight();
		lTexBlt.left = (fx - LightRect.x) / iLightWdt;
		lTexBlt.top = 1.0 - (fy - LightRect.y) / iLightHgt;
		lTexBlt.right = (fx + cgo.Wdt - LightRect.x) / iLightWdt;
		lTexBlt.bottom = 1.0 - (fy + cgo.Hgt - LightRect.y) / iLightHgt;

		lightTex[0] = lTexBlt.left;
		lightTex[1] = lTexBlt.top;
		lightTex[2] = lTexBlt.right;
		lightTex[3] = lTexBlt.top;
		lightTex[4] = lTexBlt.left;
		lightTex[5] = lTexBlt.bottom;
		lightTex[6] = lTexBlt.right;
		lightTex[7] = lTexBlt.bottom;
		nFloats = 24;
	}

	// Upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, hVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nFloats * sizeof(float), vtxData);

	// Bind VAO
	unsigned int vaoid = Light ? hVAOIDLight : hVAOIDNoLight;
	GLuint vao;
	const bool has_vao = pGL->GetVAO(vaoid, vao);
	glBindVertexArray(vao);
	if (!has_vao)
	{
		// Setup state
		glEnableVertexAttribArray(shader->GetAttribute(C4LRA_Position));
		glEnableVertexAttribArray(shader->GetAttribute(C4LRA_LandscapeTexCoord));
		if (Light)
			glEnableVertexAttribArray(shader->GetAttribute(C4LRA_LightTexCoord));

		glVertexAttribPointer(shader->GetAttribute(C4LRA_Position), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glVertexAttribPointer(shader->GetAttribute(C4LRA_LandscapeTexCoord), 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const uint8_t*>(8 * sizeof(float)));
		if (Light)
			glVertexAttribPointer(shader->GetAttribute(C4LRA_LightTexCoord), 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const uint8_t*>(16 * sizeof(float)));
	}

	// Do the blit
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Reset state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ShaderCall.Finish();
}

#endif // #ifndef USE_CONSOLE
