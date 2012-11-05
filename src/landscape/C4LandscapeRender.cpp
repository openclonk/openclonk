
#include "C4Include.h"
#include "C4LandscapeRender.h"

#include "C4Landscape.h"
#include "C4Texture.h"

#include "C4GroupSet.h"
#include "C4Components.h"

#include "C4DrawGL.h"
#include "StdColors.h"

#ifdef USE_GL

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
const int C4LR_BiasDistanceX = 8;
const int C4LR_BiasDistanceY = 8;

// Workarounds to try if shader fails to compile
const char *C4LR_ShaderWorkarounds[] = {
	"#define NO_TEXTURE_LOD_IN_FRAGMENT\n",
	"#define NO_BROKEN_ARRAYS_WORKAROUND\n",
	"#define SCALER_IN_GPU\n",
};
const int C4LR_ShaderWorkaroundCount = sizeof(C4LR_ShaderWorkarounds) / sizeof(*C4LR_ShaderWorkarounds);

// Name used for the seperator texture
const char *const SEPERATOR_TEXTURE = "--SEP--";

// Map of uniforms to names in shader
static const char *GetUniformName(int iUniform)
{
	switch(iUniform)
	{
	case C4LRU_LandscapeTex: return "landscapeTex";
	case C4LRU_ScalerTex:    return "scalerTex";
	case C4LRU_MaterialTex:  return "materialTex";
	case C4LRU_Resolution:   return "resolution";
	case C4LRU_MatMap:       return "matMap";
	case C4LRU_MatMapTex:    return "matMapTex";
	case C4LRU_MaterialDepth:return "materialDepth";
	case C4LRU_MaterialSize: return "materialSize";
	}
	assert(false);
	return "mysterious";
}

C4LandscapeRenderGL::C4LandscapeRenderGL()
	: iLandscapeShaderTime(0),
	hVert(0), hFrag(0), hProg(0)
{
	ZeroMem(Surfaces, sizeof(Surfaces));
	ZeroMem(hMaterialTexture, sizeof(hMaterialTexture));
	ZeroMem(hUniforms, sizeof(hUniforms));
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
		Surfaces[i] = NULL;
	}
	for (i = 0; i < C4LR_MipMapCount; i++)
	{
		glDeleteObjectARB(hMaterialTexture[i]);
		hMaterialTexture[i] = 0;
	}

	LandscapeShader.Clear();
	LandscapeShaderPath.Clear();
	iLandscapeShaderTime = 0;
}

bool C4LandscapeRenderGL::InitLandscapeTexture()
{

	// Round up to nearest power of two
	int iSfcWdt = 1, iSfcHgt = 1;
	while(iSfcWdt < iWidth) iSfcWdt *= 2;
	while(iSfcHgt < iHeight) iSfcHgt *= 2;

	// One bit more of information to safe more space
	if(iSfcWdt * 3 / 4 >= iWidth) iSfcWdt = iSfcWdt * 3 / 4;
	if(iSfcHgt * 3 / 4 >= iHeight) iSfcHgt = iSfcHgt * 3 / 4;

	// Create our surfaces
	for(int i = 0; i < C4LR_SurfaceCount; i++)
	{
		Surfaces[i] = new C4Surface();
		if(!Surfaces[i]->Create(iSfcWdt, iSfcHgt))
			return false;
	}

	return true;
}

bool C4LandscapeRenderGL::InitMaterialTexture(C4TextureMap *pTexs)
{

	// Populate our map with all needed textures
	MaterialTextureMap.push_back(StdCopyStrBuf(""));
	AddTexturesFromMap(pTexs);

	// Determine depth to use
	iMaterialTextureDepth = 1;
	while(iMaterialTextureDepth < int32_t(MaterialTextureMap.size()))
		iMaterialTextureDepth <<= 1;

	// Find first (actual) texture
	int iRefTexIx = 0; C4Texture *pRefTex; C4Surface *pRefSfc = NULL;
	for(; pRefTex = pTexs->GetTexture(pTexs->GetTexture(iRefTexIx)); iRefTexIx++)
		if(pRefSfc = pRefTex->Surface32)
			break;
	if(!pRefSfc)
		return false;

	// Get size for our textures. We might be limited by hardware
	int iTexWdt = pRefSfc->Wdt, iTexHgt = pRefSfc->Hgt;
	GLint iMaxTexSize;
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &iMaxTexSize);
	if (iTexWdt > iMaxTexSize || iTexHgt > iMaxTexSize)
	{
		iTexWdt = Min(iTexWdt, iMaxTexSize);
		iTexHgt = Min(iTexHgt, iMaxTexSize);
		LogF("   gl: Material textures too large, GPU only supports %dx%d! Cropping might occur!", iMaxTexSize, iMaxTexSize);
	}
	if(iMaterialTextureDepth >= iMaxTexSize)
	{
		LogF("   gl: Too many material textures! GPU only supports 3D texture depth of %d!", iMaxTexSize);
		return false;
	}
	iMaterialWidth = iTexWdt;
	iMaterialHeight = iTexHgt;

	// Compose together data of all textures
	const int iBytesPP = pRefSfc->byBytesPP;
	const int iTexSize = iTexWdt * iTexHgt * iBytesPP;
	const int iSize = iTexSize * iMaterialTextureDepth;
	BYTE *pData = new BYTE [iSize];
	for(int i = 0; i < iMaterialTextureDepth; i++)
	{
		BYTE *p = pData + i * iTexSize;
		// Get texture at position
		const char *szTexture;
		if(i < int32_t(MaterialTextureMap.size()))
			szTexture = MaterialTextureMap[i].getData();
		else
			szTexture = "";
		// Try to find the texture
		C4Texture *pTex; C4Surface *pSurface;
		if((pTex = pTexs->GetTexture(szTexture)) && (pSurface = pTex->Surface32))
		{
#ifdef DEBUG_SOLID_COLOR_TEXTURES
			// Just write a solid color that depends on the texture index
			DWORD *texdata = reinterpret_cast<DWORD *>(p);
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = RGBA((iTex & 48), (iTex & 3) * 16, (i & 12) * 4, 255);
			continue;
#else
			if(pSurface->iTexX != 1 || pSurface->iTexY != 1)
				Log("   gl: Halp! Material texture is fragmented!");
			else
			{
				// Size recheck
				if(pSurface->Wdt != iTexWdt || pSurface->Hgt != iTexHgt)
					LogF("   gl: texture %s size mismatch (%dx%d vs %dx%d)!", szTexture, pSurface->Wdt, pSurface->Hgt, iTexWdt, iTexHgt);
				// Copy bytes
				DWORD *texdata = reinterpret_cast<DWORD *>(p);
				pSurface->Lock();
				for (int y = 0; y < iTexHgt; ++y)
					for (int x = 0; x < iTexWdt; ++x)
						*texdata++ = pSurface->GetPixDw(x % pSurface->Wdt, y % pSurface->Hgt, false);
				pSurface->Unlock();
				continue;
			}
#endif
		}
		// Seperator texture?
		if(SEqual(szTexture, SEPERATOR_TEXTURE))
		{
			// Make some ugly stripes
			DWORD *texdata = reinterpret_cast<DWORD *>(p);
			for (int y = 0; y < iTexHgt; ++y)
				for (int x = 0; x < iTexWdt; ++x)
					*texdata++ = ((x + y) % 32 < 16 ? RGBA(255, 0, 0, 255) : RGBA(0, 255, 255, 255));
			continue;
		}
		// If we didn't "continue" yet, we haven't written the texture yet. Make it transparent.
		memset(p, 0, iTexSize);
	}

	// Clear error error(s?)
	while(glGetError()) {}
	
	// Alloc 3D textures
	glEnable(GL_TEXTURE_3D);
	glGenTextures(C4LR_MipMapCount, hMaterialTexture);
	
	// Generate textures (mipmaps too!)
	int iSizeSum = 0;
	BYTE *pLastData = new BYTE [iSize / 4];
	for(int iMMLevel = 0; iMMLevel < C4LR_MipMapCount; iMMLevel++)
	{
		
		// Scale the texture down for mip-mapping
		if(iMMLevel) {
			BYTE *pOut = pData;
			BYTE *pIn[4] = { 
				pLastData, pLastData + iBytesPP, 
				pLastData + iBytesPP * iTexWdt, pLastData + iBytesPP * iTexWdt + iBytesPP
			};
			for (int i = 0; i < iMaterialTextureDepth; ++i)
				for (int y = 0; y < iTexHgt / 2; ++y)
				{
					for (int x = 0; x < iTexWdt / 2; ++x)
					{
						for (int j = 0; j < iBytesPP; j++)
						{
							unsigned int s = 0;
							s += *pIn[0]++; s += 3 * *pIn[1]++; s += 3 * *pIn[2]++; s += *pIn[3]++; 
							*pOut++ = BYTE(s / 8);
						}
						pIn[0] += iBytesPP; pIn[1] += iBytesPP; pIn[2] += iBytesPP; pIn[3] += iBytesPP;
					}
					pIn[0] += iBytesPP * iTexWdt; pIn[1] += iBytesPP * iTexWdt;
					pIn[2] += iBytesPP * iTexWdt; pIn[3] += iBytesPP * iTexWdt;
				}
			iTexWdt /= 2; iTexHgt /= 2;
		}

		// Select texture
		glBindTexture(GL_TEXTURE_3D, hMaterialTexture[iMMLevel]);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// We fully expect to tile these
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Make it happen!
		glTexImage3D(GL_TEXTURE_3D, 0, 4, iTexWdt, iTexHgt, iMaterialTextureDepth, 0, GL_BGRA,
					iBytesPP == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV,
					pData);
	   
		// Exchange buffers
		BYTE *tmp = pLastData;
		pLastData = pData;
		pData = tmp;

		// Statistics
		iSizeSum += iTexWdt * iTexHgt * iMaterialTextureDepth * iBytesPP;
	}
	
	// Dispose of data
	delete [] pData;
	delete [] pLastData;
	glDisable(GL_TEXTURE_3D);
	
	// Check whether we were successful
	if(int err = glGetError())
	{
		LogF("   gl: Could not load textures (error %d)", err);
		return false;
	}

	// Announce the good news
	LogF("  gl: Texturing uses %d slots at %dx%d, %d levels (%d MB total)",
		static_cast<int>(MaterialTextureMap.size()),
		iMaterialWidth, iMaterialHeight,
		C4LR_MipMapCount,
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
	C4TexRef *TexRefs[C4LR_SurfaceCount];
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
			int iPix = pSource->_GetPix(To.x+x, To.y+y);
			int iPlac = pSource->_GetPlacement(To.x+x, To.y+y);
			int iMat = pSource->_GetMat(To.x+x, To.y+y);
			int iHBias = Max(0, iPlac * (C4LR_BiasDistanceY-1) - iRight) -
			             Max(0, iPlac * (C4LR_BiasDistanceY-1) - iLeft);
			int iVBias = Max(0, iPlac * (C4LR_BiasDistanceY-1) - pDown[x]) -
			             Max(0, iPlac * (C4LR_BiasDistanceY-1) - pUp[x]);

			// Maximum placement differences that make a difference in the result, 
			// after which we are at the limits of what can be packed into a byte
			const int iMaxPlacDiff = 40;
			int iHBiasScaled = BoundBy(iHBias * 127 / iMaxPlacDiff / C4LR_BiasDistanceX + 128, 0, 255);
			int iVBiasScaled = BoundBy(iVBias * 127 / iMaxPlacDiff / C4LR_BiasDistanceY + 128, 0, 255);

			// Visit our neighbours
			int iNeighbours[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			if(To.y+y > 0) {
				if(To.x+x > 0)
					iNeighbours[0] = pSource->_GetPix(To.x+x-1, To.y+y-1);
				iNeighbours[1] = pSource->_GetPix(To.x+x, To.y+y-1);
				if(To.x+x < iWidth-1)
					iNeighbours[2] = pSource->_GetPix(To.x+x+1, To.y+y-1);
			}
			if(To.x+x > 0)
				iNeighbours[3] = pSource->_GetPix(To.x+x-1, To.y+y);
			if(To.x+x < iWidth-1)
				iNeighbours[4] = pSource->_GetPix(To.x+x+1, To.y+y);
			if(To.y+y+1 < iHeight-1) {
				if(To.x+x > 0)
					iNeighbours[5] = pSource->_GetPix(To.x+x-1, To.y+y+1);
				iNeighbours[6] = pSource->_GetPix(To.x+x, To.y+y+1);
				if(To.x+x < iWidth-1)
					iNeighbours[7] = pSource->_GetPix(To.x+x+1, To.y+y+1);
			}

			// Look for highest-placement material in our surroundings
			int iMaxPix = iPix, iMaxPlace = iPlac, i;
			for(i = 0; i < 8; i++) {
				int iTempPlace = MatPlacement(PixCol2Mat(iNeighbours[i]));
				if(iTempPlace > iMaxPlace || (iTempPlace == iMaxPlace && iNeighbours[i] > iMaxPix) ) {
					iMaxPix = iNeighbours[i]; iMaxPlace = iTempPlace;
				}
			}

			// Scaler calculation depends on whether this is the highest-placement material around
			int iScaler = 0;
			if(iMaxPix == iPix) {

				// If yes, we consider all other materials as "other"
				for(i = 0; i < 8; i++)
					if(iNeighbours[i] == iPix)
						iScaler += (1<<i);

			} else {

				// Otherwise, we *only* consider the highest-placement material as "other"
				for(i = 0; i < 8; i++)
					if(iNeighbours[i] != iMaxPix)
						iScaler += (1<<i);
			}

			// Collect data to save per pixel
			unsigned char data[C4LR_SurfaceCount * 4];
			memset(data, 0, sizeof(data));

			data[C4LR_Material] = iPix;
			data[C4LR_BiasX] = iHBiasScaled;
			data[C4LR_BiasY] = iVBiasScaled;
			data[C4LR_Scaler] = iScaler;

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

void C4LandscapeRenderGL::DumpInfoLog(const char *szWhat, GLhandleARB hShader, int32_t iWorkaround)
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
	pBuf[iActualLength] = '\0';
	LogSilentF("  gl: Compiling %s %d:", szWhat, iWorkaround);
	LogSilent(pBuf);
	delete[] pBuf;
}

int C4LandscapeRenderGL::GetObjectStatus(GLhandleARB hObj, GLenum type)
{
	int iStatus = 0;
	glGetObjectParameterivARB(hObj, type, &iStatus);
	return iStatus;
}

GLhandleARB C4LandscapeRenderGL::CreateShader(GLenum iShaderType, const char *szWhat, const char *szCode, int32_t iWorkaround)
{
	// Create shader
	GLhandleARB hShader = glCreateShaderObjectARB(iShaderType);

	// Find #version
	StdStrBuf Version("");
	const char *szCodeRest = szCode;
	if (const char *szVersion = SSearch(szCode, "#version"))
	{
		while (*szVersion && *szVersion != '\n')
			szVersion++;
		if (*szVersion == '\n')
			szVersion++;
		Version.Copy(szCode, szVersion - szCode);
		szCodeRest = szVersion;
	}

	// Get number of available uniforms from driver
	GLint max_uniforms = 0;
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &max_uniforms);
	Version.AppendFormat("#define MAX_FRAGMENT_UNIFORM_COMPONENTS %d\n", max_uniforms);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &max_uniforms);
	Version.AppendFormat("#define MAX_VERTEX_UNIFORM_COMPONENTS %d\n", max_uniforms);

	// Build code
	const char *szCodes[C4LR_ShaderWorkaroundCount + 2];
	szCodes[0] = Version.getData();
	for(int i = 0; i < C4LR_ShaderWorkaroundCount; i++)
		if(iWorkaround & (1 << i))
			szCodes[i+1] = C4LR_ShaderWorkarounds[i];
		else
			szCodes[i+1] = "";
	szCodes[C4LR_ShaderWorkaroundCount+1] = szCodeRest;

	// Compile
	glShaderSourceARB(hShader, C4LR_ShaderWorkaroundCount + 2, szCodes, 0);
	glCompileShaderARB(hShader);

	// Dump any information to log
	DumpInfoLog(szWhat, hShader, iWorkaround);

	// Success?
	if(GetObjectStatus(hShader, GL_OBJECT_COMPILE_STATUS_ARB) == 1)
		return hShader;

	// Did not work :/
	glDeleteObjectARB(hShader);
	return 0;
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

	// Try all workarounds until one works
	int iWorkaround;
	for(iWorkaround = 0; iWorkaround < (1 << C4LR_ShaderWorkaroundCount); iWorkaround++)
	{
		// Create trivial fragment shader
		const char *szVert = "#version 110\nvoid main() { gl_TexCoord[0] = gl_MultiTexCoord0; gl_Position = ftransform(); } ";
		hVert = CreateShader(GL_VERTEX_SHADER_ARB, "vertex shader", szVert, iWorkaround);
		hFrag = CreateShader(GL_FRAGMENT_SHADER_ARB, "fragment shader", LandscapeShader.getData(), iWorkaround);
		if(!hFrag || !hVert)
			continue;

		// Link program
		hProg = glCreateProgramObjectARB();
		glAttachObjectARB(hProg, hVert);
		glAttachObjectARB(hProg, hFrag);
		glLinkProgramARB(hProg);

		// Link successful?
		DumpInfoLog("shader program", hProg, iWorkaround);
		if(GetObjectStatus(hProg, GL_OBJECT_LINK_STATUS_ARB) == 1)
			break;

		// Clear up
		glDetachObjectARB(hProg, hVert);
		glDetachObjectARB(hProg, hFrag);
		glDeleteObjectARB(hVert);
		glDeleteObjectARB(hFrag);
		glDeleteObjectARB(hProg);
		hProg = hVert = hFrag = 0;
	}
	
	// Did not get it to work?
	if(!hProg)
	{
		Log("  gl: Failed to link shader!");
		return false;
	}
	LogF("  gl: Shader %d linked successfully", iWorkaround);

	// Get uniform locations. Note this is expected to fail for a few of them
	// because the respective uniforms got optimized out!
	for (int i = 0; i < C4LRU_Count; i++)
		hUniforms[i] = glGetUniformLocationARB(hProg, GetUniformName(i));

	// Success?
	if(int err = glGetError())
	{
		LogF("  gl: Error code %d while linking shader!", err);
		return false;
	}
	return true;
}

void C4LandscapeRenderGL::ClearShaders()
{
	if (!hProg) return;

	// Need to be detached, then deleted
	glDetachObjectARB(hProg, hFrag);
	glDetachObjectARB(hProg, hVert);
	glDeleteObjectARB(hFrag);
	glDeleteObjectARB(hVert);
	glDeleteObjectARB(hProg);
	hFrag = hVert = hProg = 0;

	ZeroMem(hUniforms, sizeof(hUniforms));
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
	if(!fctScaler.Load(*pGroup, C4CFN_LandscapeScaler))
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

void C4LandscapeRenderGL::RefreshShaders()
{
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
		MaterialTextureMap.push_back(StdCopyStrBuf(szFrom));
	// Have one of the textures at the end of the list?
	else if(SEqual(MaterialTextureMap.back().getData(), szFrom))
		MaterialTextureMap.push_back(StdCopyStrBuf(szTo));
	else if(SEqual(MaterialTextureMap.back().getData(), szTo))
		MaterialTextureMap.push_back(StdCopyStrBuf(szFrom));
	else
	{
		// Otherwise add both
		MaterialTextureMap.push_back(StdCopyStrBuf(szFrom));
		MaterialTextureMap.push_back(StdCopyStrBuf(szTo));
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
	for(int32_t i = 0; pEntry = pMap->GetEntry(i); i++)
		// ToDo: Properly handle jumping back
		AddTextureAnim(pEntry->GetTextureName());

}

void C4LandscapeRenderGL::BuildMatMap(GLfloat *pFMap, GLubyte *pIMap)
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
			if(pFMap) pFMap[pix] = 0.5 / iMaterialTextureDepth;
			if(pIMap) pIMap[pix] = 0;
			continue;
		}

		// Got animation?
		int iPhases = 1; const char *p = pEntry->GetTextureName();
		while(p = strchr(p, '-')) { p++; iPhases++; }
		// Hard-coded hack. Fix me!
		const int iPhaseLength = 300;
		float phase = (iPhases == 1 ? 0 : float(GetTime() % (iPhases * iPhaseLength)) / iPhaseLength);

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
		if(pFMap) pFMap[pix] = (gTexCoo + 0.5) / iMaterialTextureDepth;
		if(pIMap) pIMap[pix] = int((gTexCoo * 256.0 / iMaterialTextureDepth) + 0.5);
	}
}

void C4LandscapeRenderGL::Draw(const C4TargetFacet &cgo)
{
	// Must have GL and be initialized
	if(!pGL && !hProg) return;
	
	// prepare rendering to surface
	C4Surface *sfcTarget = cgo.Surface;
	if (!pGL->PrepareRendering(sfcTarget)) return;

#ifdef AUTO_RELOAD_SHADERS
	RefreshShaders();
#endif // AUTO_RELOAD_SHADERS

	// Clear error(s?)
	while(glGetError()) {}

	// Activate shader
	glUseProgramObjectARB(hProg);

	// Bind data
	if (hUniforms[C4LRU_Resolution] != GLhandleARB(-1))
		glUniform2fARB(hUniforms[C4LRU_Resolution], Surfaces[0]->Wdt, Surfaces[0]->Hgt);
	if (hUniforms[C4LRU_MatMap] != GLhandleARB(-1))
	{
		GLfloat MatMap[256];
		BuildMatMap(MatMap, NULL);
		glUniform1fvARB(hUniforms[C4LRU_MatMap], 256, MatMap);
	}
	if (hUniforms[C4LRU_MaterialDepth] != GLhandleARB(-1))
		glUniform1iARB(hUniforms[C4LRU_MaterialDepth], iMaterialTextureDepth);
	if (hUniforms[C4LRU_MaterialSize] != GLhandleARB(-1))
		glUniform2fARB(hUniforms[C4LRU_MaterialSize], float(iMaterialWidth) / ::Game.C4S.Landscape.MaterialZoom,
		                                              float(iMaterialHeight) / ::Game.C4S.Landscape.MaterialZoom);
		
	// Setup facilities for texture unit allocation (gimme local functions...)
	int iUnit = 0; int iUnitMap[32]; ZeroMem(iUnitMap, sizeof(iUnitMap));
	#define ALLOC_UNIT(hUniform, iType) do { \
		if(hUniform != GLhandleARB(-1)) glUniform1iARB(hUniform, iUnit); \
		glActiveTexture(GL_TEXTURE0 + iUnit); \
		iUnitMap[iUnit] = iType; \
		glEnable(iType); \
		iUnit++; \
		assert(iUnit < 32); \
	} while(false)

	// Start binding textures
	if(hUniforms[C4LRU_ScalerTex] != GLhandleARB(-1))
	{
		ALLOC_UNIT(hUniforms[C4LRU_ScalerTex], GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, fctScaler.Surface->ppTex[0]->texName);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if(hUniforms[C4LRU_MaterialTex] != GLhandleARB(-1))
	{
		ALLOC_UNIT(hUniforms[C4LRU_MaterialTex], GL_TEXTURE_3D);

		// Decide which mip-map level to use
		double z = 0.5; int iMM = 0;
		while(pGL->Zoom < z * ::Game.C4S.Landscape.MaterialZoom && iMM + 1 <C4LR_MipMapCount)
			{ z /= 2; iMM++; }
		glBindTexture(GL_TEXTURE_3D, hMaterialTexture[iMM]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if(hUniforms[C4LRU_LandscapeTex] != GLhandleARB(-1))
	{
		GLint iLandscapeUnits[C4LR_SurfaceCount];
		for(int i = 0; i < C4LR_SurfaceCount; i++)
		{
			iLandscapeUnits[i] = iUnit;
			ALLOC_UNIT(GLhandleARB(-1), GL_TEXTURE_2D);
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
		glUniform1ivARB(hUniforms[C4LRU_LandscapeTex], C4LR_SurfaceCount, iLandscapeUnits);
	}
	if(hUniforms[C4LRU_MatMapTex] != GLhandleARB(-1))
	{
		ALLOC_UNIT(hUniforms[C4LRU_MatMapTex], GL_TEXTURE_1D);
		GLubyte MatMap[256];
		BuildMatMap(NULL, MatMap);
		glTexImage1D(GL_TEXTURE_1D, 0, 1, 256, 0, GL_RED, GL_UNSIGNED_BYTE, MatMap);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

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
	C4BltVertex Vtx[4];
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
		Vtx[i].tx /= float(Surfaces[0]->Wdt);
		Vtx[i].ty /= float(Surfaces[0]->Hgt);
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

	// Blend it
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Blit
	glInterleavedArrays(GL_T2F_C4UB_V3F, sizeof(C4BltVertex), Vtx);
	glDrawArrays(GL_QUADS, 0, 4);

	// Remove shader
	glUseProgramObjectARB(0);

	// Unbind textures
	while(iUnit > 0)
	{
		iUnit--;
		glActiveTexture(GL_TEXTURE0 + iUnit);
		glDisable(iUnitMap[iUnit]);
	}

	// Got an error?
	if(int err = glGetError())
	{
		LogF("GL error: %d", err /*, gluErrorString(err)*/);
	}
}

#endif // USE_GL
