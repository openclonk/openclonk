
#include "C4Include.h"
#include "C4LandscapeRender.h"

#include "C4Landscape.h"
#include "C4Texture.h"

#include "C4GroupSet.h"
#include "C4Components.h"

#include "StdGL.h"
#include "StdColors.h"

#ifdef USE_GL

// Automatically reload shaders when changed at runtime?
#define AUTO_RELOAD_SHADERS

// How much to look into each direction for bias
const int C4LR_BiasDistanceX = 8;
const int C4LR_BiasDistanceY = 8;

C4LandscapeRenderGL::C4LandscapeRenderGL()
	: iLandscapeShaderTime(0),
	hVert(0), hFrag(0), hProg(0),
	hLandscapeUnit(0), hScalerUnit(0), hMaterialUnit(0),
	hResolutionUniform(0), hMatTexMapUniform(0),
	iTexCount(0),
	hMaterialTexture(0)
{
	ZeroMem(MatTexMap, sizeof(MatTexMap));
	ZeroMem(Surfaces, sizeof(Surfaces));
}

C4LandscapeRenderGL::~C4LandscapeRenderGL()
{
	Clear();
}

bool C4LandscapeRenderGL::Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pTexs, C4GroupSet *pGraphics)
{
	Clear();

	// Create our surfaces
	for(int i = 0; i < C4LR_SurfaceCount; i++)
	{
		Surfaces[i] = new CSurface();
		if(!Surfaces[i]->Create(iWidth, iHeight))
		{
			Clear();
			return false;
		}
	}

	// Safe info
	this->iWidth = iWidth;
	this->iHeight = iHeight;

	// Count the textures
	iTexCount = 0;
	while(pTexs->GetTexture(iTexCount))
		iTexCount++;

	// Build material-texture map (depth parameter where to find appropriate texture)
	for(int pix = 0; pix < 256; pix++)
	{
		// Look up indexed entry
		const C4TexMapEntry *pEntry = pTexs->GetEntry(PixCol2Tex(BYTE(pix)));
		if(!pEntry->GetTextureName())
		{
			MatTexMap[pix] = 0.5 / (iTexCount - 1);
			continue;
		}
		// Assign texture
		int32_t iTexIndex = pTexs->GetTextureIndex(pEntry->GetTextureName());
		if(iTexIndex < 0) iTexIndex = 0;
		MatTexMap[pix] = (float(iTexIndex) + 0.5) / (iTexCount - 1);
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

	return true;
}

void C4LandscapeRenderGL::Clear()
{
	ClearShaders();

	// free textures
	for (int i = 0; i < C4LR_SurfaceCount; i++)
	{
		delete Surfaces[i];
		Surfaces[i] = NULL;
	}
	glDeleteObjectARB(hMaterialTexture);
	hMaterialTexture = 0;

	LandscapeShader.Clear();
	LandscapeShaderPath.Clear();
	iLandscapeShaderTime = 0;
}

bool C4LandscapeRenderGL::InitMaterialTexture(C4TextureMap *pTexs)
{

	// Find first (actual) texture
	int iRefTexIx = 0; C4Texture *pRefTex; CSurface *pRefSfc = NULL;
	for(; pRefTex = pTexs->GetTexture(pTexs->GetTexture(iRefTexIx)); iRefTexIx++)
		if(pRefSfc = pRefTex->Surface32)
			break;
	if(!pRefSfc)
		return false;
	
	// Compose together data of all textures
	const int iTexWdt = pRefSfc->Wdt, iTexHgt = pRefSfc->Hgt;
	const int iBytesPP = pRefSfc->byBytesPP;
	const int iTexSize = iTexWdt * iTexHgt * iBytesPP;
	const int iSize = iTexSize * iTexCount;
	char *pData = new char [iSize];
	for(int i = 0; i < iTexCount; i++)
	{
		char *p = pData + i * iTexSize;
		C4Texture *pTex; CSurface *pSurface;
		if(!(pTex = pTexs->GetTexture(pTexs->GetTexture(i))))
			{}
		if(!(pSurface = pTex->Surface32))
			{}
		else if(pSurface->iTexX != 1 || pSurface->iTexY != 1)
			Log("   gl: Halp! Material texture is fragmented!");
		else if(pSurface->Wdt != iTexWdt || pSurface->Hgt != iTexHgt)
		{
			LogF("   gl: texture %s size mismatch (%dx%d vs %dx%d)!", pTexs->GetTexture(i), pSurface->Wdt, pSurface->Hgt, iTexWdt, iTexHgt);
			int32_t *texdata = reinterpret_cast<int32_t*>(p);
			pSurface->ppTex[0]->Lock();
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = *reinterpret_cast<int32_t*>(pSurface->ppTex[0]->texLock.pBits + pSurface->ppTex[0]->texLock.Pitch * (y % pSurface->Hgt) + pSurface->byBytesPP * (x % pSurface->Wdt));
					// *texdata++ = !!((y ^ x) & 64) ? 0xFFFFFF00 : 0xFF000000;
			pSurface->ppTex[0]->Unlock();
			continue;
		}
		else
		{
			memcpy(p, pSurface->ppTex[0]->texLock.pBits, iTexSize);
			continue;
		}
		memset(p, 0, iTexSize);
	}

	// Clear error error(s?)
	while(glGetError()) {}

	// Alloc a 3D texture
	glEnable(GL_TEXTURE_3D);
	glGenTextures(1, &hMaterialTexture);
	glBindTexture(GL_TEXTURE_3D, hMaterialTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	// We fully expect to tile these
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Make it happen!
	glTexImage3D(GL_TEXTURE_3D, 0, 4, iTexWdt, iTexHgt, iTexCount, 0, GL_BGRA,
		iBytesPP == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV,
		pData);
	glDisable(GL_TEXTURE_3D);

	// Dispose of data
	delete [] pData;

	// Check whether we were successful
	if(int err = glGetError())
	{
		LogF("  gl: Could not load textures (error %d)", err);
		return false;
	}
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
	for (int i = 0; i < C4LR_SurfaceCount; i++) {
		if (!Surfaces[i]->Lock()) return;
		Surfaces[i]->ClearBoxDw(To.x, To.y, To.Wdt, To.Hgt);
	}

	// Initialize up & down arrays
	int x, y;
	int *pUp = new int [To.Wdt * 2];
	int *pDown = pUp + To.Wdt;
	for(x = 0; x < To.Wdt; x++) {
		int iSum = 0;
		for(y = 1; y < Min(C4LR_BiasDistanceY, To.y+1); y++)
			iSum += pSource->_GetPlacement(To.x+x, To.y-y);
		pUp[x] = iSum;
		iSum = 0;
		for(y = 1; y < Min(C4LR_BiasDistanceY, iHeight - To.y); y++)
			iSum += pSource->_GetPlacement(To.x+x, To.y+y);
		pDown[x] = iSum;
	}

	// Get tex refs (shortcut, we will use them quite heavily)
	CTexRef *TexRefs[C4LR_SurfaceCount];
	x = y = 0;
	for(int i = 0; i < C4LR_SurfaceCount; i++)
		Surfaces[i]->GetTexAt(&TexRefs[i], x, y);

	// Go through it from top to bottom
	for(y = 0; y < To.Hgt; y++) {

		// Initialize left & right
		int iLeft = 0;
		int iRight = 0;
		for(x = 1; x < Min(C4LR_BiasDistanceX, To.x+1); x++)
			iLeft += pSource->_GetPlacement(To.x-x,To.y+y);
		for(x = 1; x < Min(C4LR_BiasDistanceX, iWidth - To.x); x++)
			iRight += pSource->_GetPlacement(To.x+x,To.y+y);

		for(x = 0; x < To.Wdt; x++) {

			// Biases
			int iPlac = pSource->_GetPlacement(To.x+x, To.y+y);
			int iHBias = Max(0, iPlac * (C4LR_BiasDistanceY-1) - iRight) -
			             Max(0, iPlac * (C4LR_BiasDistanceY-1) - iLeft);
			int iVBias = Max(0, iPlac * (C4LR_BiasDistanceY-1) - pDown[x]) -
			             Max(0, iPlac * (C4LR_BiasDistanceY-1) - pUp[x]);

			// Maximum placement differences that make a difference in the result, 
			// after which we are at the limits of what can be packed into a byte
			const int iMaxPlacDiff = 40;
			int iHBiasScaled = BoundBy(iHBias * 127 / iMaxPlacDiff / C4LR_BiasDistanceX + 128, 0, 255);
			int iVBiasScaled = BoundBy(iVBias * 127 / iMaxPlacDiff / C4LR_BiasDistanceY + 128, 0, 255);

			// Collect data to save per pixel
			unsigned char data[C4LR_SurfaceCount * 4];
			memset(data, 0, sizeof(data));

			data[C4LR_Material] = pSource->_GetPix(To.x+x, To.y+y);
			data[C4LR_BiasX] = iHBiasScaled;
			data[C4LR_BiasY] = iVBiasScaled;

			for(int i = 0; i < C4LR_SurfaceCount; i++)
				TexRefs[i]->SetPix4(To.x+x, To.y+y, 
					RGBA(data[i*4+0], data[i*4+1], data[i*4+2], data[i*4+3]));

			// Update left & right
			if(To.x+x + 1 < iWidth)
				iRight -= pSource->_GetPlacement(To.x+x + 1, To.y+y);
			if(To.x+x + C4LR_BiasDistanceX < iWidth)
				iRight += pSource->_GetPlacement(To.x+x + C4LR_BiasDistanceX, To.y+y);
			iLeft += pSource->_GetPlacement(To.x+x, To.y+y);
			if(To.x+x - C4LR_BiasDistanceX - 1 >= 0)
				iLeft -= pSource->_GetPlacement(To.x+x - C4LR_BiasDistanceX - 1, To.y+y);

			// Update up & down arrays
			if(To.y+y + 1 < iHeight)
				pDown[x] -= pSource->_GetPlacement(To.x+x, To.y+y + 1);
			if(To.y+y + C4LR_BiasDistanceY < iHeight)
				pDown[x] += pSource->_GetPlacement(To.x+x, To.y+y + C4LR_BiasDistanceY);
			pUp[x] += pSource->_GetPlacement(To.x+x, To.y+y);
			if(To.y+y - C4LR_BiasDistanceY + 1 >= 0) {
				pUp[x] -= pSource->_GetPlacement(To.x+x, To.y+y - C4LR_BiasDistanceY + 1);
			}
		}
	}

	// done
	delete[] pUp;
	for (int i = 0; i < C4LR_SurfaceCount; i++)
		Surfaces[i]->Unlock();

}

void C4LandscapeRenderGL::DumpInfoLog(const char *szWhat, GLhandleARB hShader)
{
	// Get length of info line
	int iLength = 0;
	glGetObjectParameterivARB(hShader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &iLength);
	if(iLength <= 1) return;

	// Allocate buffer, get data
	char *pBuf = new char [iLength + 1];
	int iActualLength = 0;
    glGetInfoLogARB(hShader, iLength, &iActualLength, pBuf);
	if(iActualLength > iLength || iActualLength <= 0) return;
	
	// Terminate, log
	pBuf[iActualLength + 1] = '\0';
	Log(pBuf);
	delete[] pBuf;
}

int C4LandscapeRenderGL::GetObjectStatus(GLhandleARB hObj, GLenum type)
{
	int iStatus = 0;
	glGetObjectParameterivARB(hObj, type, &iStatus);
	return iStatus;
}

GLhandleARB C4LandscapeRenderGL::CreateShader(GLenum iShaderType, const char *szWhat, const char *szCode)
{
	const char *szCodes[1] = { szCode };
    GLhandleARB hShader = glCreateShaderObjectARB(iShaderType);
	glShaderSourceARB(hShader, 1, szCodes, 0);
    glCompileShaderARB(hShader);

	// Dump any information to log
	DumpInfoLog(szWhat, hShader);

	// Success?
	if(GetObjectStatus(hShader, GL_OBJECT_COMPILE_STATUS_ARB) != 1)
		return 0;
	else
		return hShader;
}

bool C4LandscapeRenderGL::InitShaders()
{
	// Already initialized or no shader load?
	if(hProg || LandscapeShader.getLength() <= 0)
		return false;

	// No support?
	if(!GLEW_ARB_fragment_program)
	{
		Log("   gl: no shader support!");
		return false;
	}

	// Create trivial fragment shader
	const char *szVert = "#version 110\nvoid main() { gl_TexCoord[0] = gl_MultiTexCoord0; gl_Position = ftransform(); } ";
	hVert = CreateShader(GL_VERTEX_SHADER_ARB, "Vertex shader", szVert);
    hFrag = CreateShader(GL_FRAGMENT_SHADER_ARB, "Fragment shader", LandscapeShader.getData());
	if(!hFrag || !hVert) return false;

	// Link program
	hProg = glCreateProgramObjectARB();
    glAttachObjectARB(hProg, hVert);
    glAttachObjectARB(hProg, hFrag);
    glLinkProgramARB(hProg);

	// Link successful?
	DumpInfoLog("Shader program", hProg);
	if(GetObjectStatus(hProg, GL_OBJECT_LINK_STATUS_ARB) != 1)
	{
		ClearShaders();
		return false;
	}

	// Get variable locations
	hLandscapeUnit = glGetUniformLocationARB(hProg, "landscapeTex");
	hScalerUnit = glGetUniformLocationARB(hProg, "scalerTex");
	hMaterialUnit = glGetUniformLocationARB(hProg, "materialTex");
	hResolutionUniform = glGetUniformLocationARB(hProg, "resolution");
	hMatTexMapUniform = glGetUniformLocationARB(hProg, "matTexMap");

	// Success?
	if(int err = glGetError())
	{
		LogF("  gl: error code %d", err);
		return false;
	}
	return true;
}

void C4LandscapeRenderGL::ClearShaders()
{
	if (!hProg) return;

	// Need to be detached, then deleted
	glDetachShader(hProg, hFrag);
	glDetachShader(hProg, hVert);
	glDeleteObjectARB(hFrag);
	glDeleteObjectARB(hVert);
	glDeleteObjectARB(hProg);
	hFrag = hVert = hProg = 0;

	hLandscapeUnit = hScalerUnit = hMaterialUnit = 0;
	hResolutionUniform = hMatTexMapUniform = 0;
}

bool C4LandscapeRenderGL::LoadShaders(C4GroupSet *pGroups)
{
	// First, clear out all existing shaders
	ClearShaders();
	// Search for our shaders
	C4Group *pGroup = pGroups->FindEntry(C4CFN_LandscapeShader);
	if(!pGroup) return false;
	// Load it, save the path for later reloading
	if(!pGroup->LoadEntryString(C4CFN_LandscapeShader, &LandscapeShader))
		return false;
	// If it physically exists, save back file name
	if(FileExists(pGroup->GetFullName().getData()))
	{
		LandscapeShaderPath.Format("%s" DirSep C4CFN_LandscapeShader, pGroup->GetFullName().getData());
		iLandscapeShaderTime = FileTime(LandscapeShaderPath.getData());
	}
	// Initialize
	return InitShaders();
}

bool C4LandscapeRenderGL::LoadScaler(C4GroupSet *pGroups)
{
	// Search for scaler
	C4Group *pGroup = pGroups->FindEntry(C4CFN_LandscapeScaler);
	if(!pGroup) return false;
	// Load scaler from group
	return fctScaler.Load(*pGroup, C4CFN_LandscapeScaler);
}

void C4LandscapeRenderGL::Draw(const C4TargetFacet &cgo)
{
	// Must have GL and be initialized
	if(!pGL && !hProg) return;
	
	// prepare rendering to surface
	CSurface *sfcTarget = cgo.Surface;
	if (!pGL->PrepareRendering(sfcTarget)) return;

#ifdef AUTO_RELOAD_SHADERS
	// File changed?
	if(!LandscapeShaderPath.isNull() && 
		FileTime(LandscapeShaderPath.getData()) != iLandscapeShaderTime)
	{
		ClearShaders();
		// Load new shader
		char szParentPath[_MAX_PATH+1]; C4Group Group;
		GetParentPath(LandscapeShaderPath.getData(),szParentPath);
		if(!Group.Open(szParentPath) ||
			!Group.LoadEntryString(GetFilename(LandscapeShaderPath.getData()),&LandscapeShader) ||
			!Group.Close())
			return;
		// Reinitialize
		InitShaders();
		iLandscapeShaderTime = FileTime(LandscapeShaderPath.getData());
	}
#endif // AUTO_RELOAD_SHADERS

	// Clear error(s?)
	while(glGetError()) {}

	// Activate shader
	glUseProgramObjectARB(hProg);

	// Bind data
	glUniform1fvARB(hMatTexMapUniform, 256, MatTexMap);
	glUniform2fARB(hResolutionUniform, iWidth, iHeight);

	// Bind textures
	int iUnit = 0; int iMaterialUnit = -1;
	if(hScalerUnit >= 0)
	{
		glUniform1iARB(hScalerUnit, iUnit);
		glActiveTexture(GL_TEXTURE0 + iUnit);
		iUnit++;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, fctScaler.Surface->ppTex[0]->texName);
	}
	if(hMaterialUnit >= 0)
	{
		iMaterialUnit = iUnit;
		glUniform1iARB(hMaterialUnit, iUnit);
		glActiveTexture(GL_TEXTURE0 + iUnit);
		iUnit++;
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, hMaterialTexture);
	}
	if(hLandscapeUnit >= 0)
	{
		GLint iLandscapeUnits[C4LR_SurfaceCount];
		for(int i = 0; i < C4LR_SurfaceCount; i++)
		{
			iLandscapeUnits[i] = iUnit;
			glActiveTexture(GL_TEXTURE0 + iUnit);
			iUnit++;
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, Surfaces[i]->ppTex[0]->texName);
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
		glUniform1ivARB(hLandscapeUnit, C4LR_SurfaceCount, iLandscapeUnits);
	}

	// get current blitting offset in texture
	int iBlitX=0;
	int iBlitY=0;

	// set up blit data as rect
	FLOAT_RECT fTexBlt, tTexBlt;
	float fx = float(cgo.TargetX), fy = float(cgo.TargetY);
	fTexBlt.left  = fx;
	fTexBlt.top   = fy;
	fTexBlt.right = fx + float(cgo.Wdt);
	fTexBlt.bottom= fy + float(cgo.Hgt);

	// apply Zoom
	float tx = float(cgo.X), ty = float(cgo.Y);
	pGL->ApplyZoom(tx, ty);
	tTexBlt.left  = tx;
	tTexBlt.top   = ty;
	tTexBlt.right = tx + float(cgo.Wdt) * pGL->Zoom;
	tTexBlt.bottom= ty + float(cgo.Hgt) * pGL->Zoom;

	// blit positions
	CBltVertex Vtx[4];
	Vtx[0].ftx = tTexBlt.left;  Vtx[0].fty = tTexBlt.top;
	Vtx[1].ftx = tTexBlt.right; Vtx[1].fty = tTexBlt.top;
	Vtx[2].ftx = tTexBlt.right; Vtx[2].fty = tTexBlt.bottom;
	Vtx[3].ftx = tTexBlt.left;  Vtx[3].fty = tTexBlt.bottom;
	Vtx[0].tx = fTexBlt.left;  Vtx[0].ty = fTexBlt.top;
	Vtx[1].tx = fTexBlt.right; Vtx[1].ty = fTexBlt.top;
	Vtx[2].tx = fTexBlt.right; Vtx[2].ty = fTexBlt.bottom;
	Vtx[3].tx = fTexBlt.left;  Vtx[3].ty = fTexBlt.bottom;
	for (int i=0; i<4; ++i)
	{
		Vtx[i].tx /= float(iWidth);
		Vtx[i].ty /= float(iHeight);
		Vtx[i].ftz = 0;
		Vtx[i].color[0] = 255;
		Vtx[i].color[1] = 255;
		Vtx[i].color[2] = 255;
		Vtx[i].color[3] = 255;
		//DwTo4UB(RGBA(255, 255, 255, 255), Vtx[i].color);
	}

	// color modulation?
	//DWORD dwModClr = BlitModulated ? BlitModulateClr : 0xffffffff;
	//for (int i=0; i<4; ++i)
	//	DwTo4UB(dwModClr | dwModMask, Vtx[i].color);

	// Blit
	glInterleavedArrays(GL_T2F_C4UB_V3F, sizeof(CBltVertex), Vtx);
	glDrawArrays(GL_QUADS, 0, 4);

	// Remove shader
	glUseProgramObjectARB(0);

	// Unbind textures
	while(iUnit > 0)
	{
		iUnit--;
		glActiveTexture(GL_TEXTURE0 + iUnit);
		glDisable(iUnit == iMaterialUnit ? GL_TEXTURE_3D : GL_TEXTURE_2D);
	}

	// Got an error?
	if(int err = glGetError())
	{
		LogF("GL error: %d", err);
	}
}

#endif // USE_GL