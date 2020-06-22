/*
 * OpenClonk, http://www.openclonk.org
 *
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
// color calculation routines

#ifndef INC_StdColors
#define INC_StdColors

// helper function
inline uint32_t RGBA(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	return ((a & 255) << 24) | ((r & 255) << 16) | ((g & 255) << 8) | (b & 255);
}
#define C4RGB(r, g, b) (((DWORD)(0xff)<<24)|(((DWORD)(r)&0xff)<<16)|(((DWORD)(g)&0xff)<<8)|((b)&0xff))
#define GetBlueValue(rgb) ((unsigned char)(rgb))
#define GetGreenValue(rgb) ((unsigned char)(((unsigned short)(rgb)) >> 8))
#define GetRedValue(rgb) ((unsigned char)((rgb)>>16))

inline void BltAlpha(DWORD &dwDst, DWORD dwSrc)
{
	// blit one color value w/alpha on another
	if (dwDst>>24 == 0x00) { dwDst=dwSrc; return; }
	BYTE byAlphaSrc=BYTE(dwSrc>>24); BYTE byAlphaDst=255-byAlphaSrc;
	dwDst = std::min<uint32_t>(((dwDst    & 0xff ) * byAlphaDst + (dwSrc & 0xff    ) * byAlphaSrc) >>8,            0xff)      | // blue
	        std::min<uint32_t>(((dwDst   & 0xff00) * byAlphaDst + (dwSrc & 0xff00  ) * byAlphaSrc) >>8 & 0xff00,   0xff00)    | // green
	        std::min<uint32_t>(((dwDst & 0xff0000) * byAlphaDst + (dwSrc & 0xff0000) * byAlphaSrc) >>8 & 0xff0000, 0xff0000)  | // red
	        std::min<uint32_t>( (dwDst >> 24) + byAlphaSrc, 255) << 24; // alpha
}

inline void BltAlphaAdd(DWORD &dwDst, DWORD dwSrc)
{
	// blit one color value w/alpha on another in additive mode
	if (dwDst>>24 == 0x00) { dwDst=dwSrc; return; }
	BYTE byAlphaSrc=BYTE(dwSrc>>24);
	dwDst = std::min<uint32_t>((dwDst    & 0xff ) + ((int(dwSrc    & 0xff  ) * byAlphaSrc) >>8)       , 0xff)      | // blue
	        std::min<uint32_t>(((dwDst   & 0xff00) + (int(dwSrc>>8 & 0xff  ) * byAlphaSrc)) & 0x00ffff00, 0xff00)    | // green
	        std::min<uint32_t>(((dwDst & 0xff0000) + (int(dwSrc>>8 & 0xff00) * byAlphaSrc)) & 0xffff0000, 0xff0000)  | // red
	        std::min<uint32_t>( (dwDst >> 24) + byAlphaSrc, 255) << 24; // alpha
}

inline void ModulateClr(DWORD &dwDst, DWORD dwMod) // modulate two color values
{
	// modulate two color values
	// get alpha
	int iA1=dwDst>>24, iA2=dwMod>>24;
	// modulate color values; mod alpha upwards
	dwDst = ((dwDst     & 0xff) * (dwMod     & 0xff) / 0xff)      | // blue
	        ((dwDst>> 8 & 0xff) * (dwMod>> 8 & 0xff) / 0xff) << 8 | // green
	        ((dwDst>>16 & 0xff) * (dwMod>>16 & 0xff) / 0xff) << 16| // red
	        std::min(iA1*iA2/0xff, 255)                           << 24; // alpha (TODO: We don't need std::min() here, do we?)
}

inline void ModulateClrA(DWORD &dwDst, DWORD dwMod) // modulate two color values and add alpha value
{
	// modulate two color values and add alpha value
	dwDst = ((dwDst     & 0xff) * (dwMod     & 0xff) / 0xff)      | // B
	        ((dwDst>> 8 & 0xff) * (dwMod>> 8 & 0xff) / 0xff) << 8 | // G
	        ((dwDst>>16 & 0xff) * (dwMod>>16 & 0xff) / 0xff) << 16| // R
	        (std::max<uint32_t>((dwDst>>24)+(dwMod>>24), 0xff) - 0xff)<<24;
}
inline void ModulateClrMOD2(DWORD &dwDst, DWORD dwMod) // clr1+clr2-0.5
{
	// signed color addition
	dwDst = (Clamp<int>(((int)(dwDst&0xff)+(dwMod&0xff)-0x7f)<<1,0,0xff)&0xff) |  // B
	        (Clamp<int>(((int)(dwDst&0xff00)+(dwMod&0xff00)-0x7f00)<<1,0,0xff00)&0xff00) | // G
	        (Clamp<int>(((int)(dwDst&0xff0000)+(dwMod&0xff0000)-0x7f0000)<<1,0,0xff0000)&0xff0000) | // R
	        (std::max<uint32_t>((dwDst>>24)+(dwMod>>24), 0xff) - 0xff)<<24;
}

inline DWORD LightenClrBy(DWORD &dwDst, int iBy) // enlight a color
{
	// enlight a color
	// quite a desaturating method...
	dwDst = std::min<int>((dwDst     & 0xff) + iBy, 255)       | // blue
	        std::min<int>((dwDst>> 8 & 0xff) + iBy, 255) <<  8 | // green
	        std::min<int>((dwDst>>16 & 0xff) + iBy, 255) << 16 | // red
	        (dwDst & 0xff000000);                     // alpha
	return dwDst;
}

inline DWORD DarkenClrBy(DWORD &dwDst, int iBy) // darken a color
{
	// darken a color
	// quite a desaturating method...
	dwDst = std::max<int>(int(dwDst     & 0xff) - iBy, 0)       | // blue
	        std::max<int>(int(dwDst>> 8 & 0xff) - iBy, 0) <<  8 | // green
	        std::max<int>(int(dwDst>>16 & 0xff) - iBy, 0) << 16 | // red
	        (dwDst & 0xff000000);                     // alpha
	return dwDst;
}

inline DWORD PlrClr2TxtClr(DWORD dwClr)
{
	// convert player color to text color, lightening up when necessary
	int lgt=std::max(std::max(GetRedValue(dwClr), GetGreenValue(dwClr)), GetBlueValue(dwClr));
	if (lgt<0x8f) LightenClrBy(dwClr, 0x8f-lgt);
	return dwClr|0xff000000;
}

inline DWORD GetClrModulation(DWORD dwSrcClr, DWORD dwDstClr, DWORD &dwBack)
{
	// get modulation that is necessary to transform dwSrcClr to dwDstClr
	// does not support alpha values in dwSrcClr and dwDstClr
	// get source color
	BYTE sB=BYTE(dwSrcClr); dwSrcClr=dwSrcClr>>8;
	BYTE sG=BYTE(dwSrcClr); dwSrcClr=dwSrcClr>>8;
	BYTE sR=BYTE(dwSrcClr); dwSrcClr=dwSrcClr>>8;
	// get dest color
	BYTE dB=BYTE(dwDstClr); dwDstClr=dwDstClr>>8;
	BYTE dG=BYTE(dwDstClr); dwDstClr=dwDstClr>>8;
	BYTE dR=BYTE(dwDstClr); dwDstClr=dwDstClr>>8;
	// get difference
	int cR=(int) dR-sR;
	int cG=(int) dG-sG;
	int cB=(int) dB-sB;
	// get max enlightment
	int diffN=0;
	if (cR>0) diffN=cR;
	if (cG>0) diffN=std::max(diffN, cG);
	if (cB>0) diffN=std::max(diffN, cB);
	// is dest > src?
	if (diffN)
	{
		// so a back mask must be used
		int bR=sR+(cR*255)/diffN;
		int bG=sG+(cG*255)/diffN;
		int bB=sB+(cB*255)/diffN;
		dwBack=RGBA(bR, bG, bB, 255);
	}
	if (!sR) sR=1;
	if (!sG) sG=1;
	if (!sB) sB=1;
	return RGBA(std::min((int)dR*256/sR, 255), std::min((int)dG*256/sG, 255), std::min((int)dB*256/sB, 255), 255-diffN);
}

inline bool rgb2xyY(double r, double g, double b, double *px, double *py, double *pY) // linear rgb to CIE xyY
{
	double X = 0.412453*r + 0.357580*g + 0.180423*b;
	double Y = 0.212671*r + 0.715160*g + 0.072169*b;
	double Z = 0.019334*r + 0.119193*g + 0.950227*b;
	double XYZ=X+Y+Z;
	if (!XYZ)
	{
		*px=*py=0.3; // assume grey cromaticity for black
	}
	else
	{
		*px = X/XYZ; *py = Y/XYZ;
	}
	*pY = Y;
	return true;
}

inline bool xy2upvp(double x, double y, double *pu, double *pv) // CIE xy to u'v'
{
	double n = -2.0*x+12.0*y+3.0;
	if (!n) return false;
	*pu = 4.0*x / n;
	*pv = 9.0*y / n;
	return true;
}

inline bool RGB2rgb(int R, int G, int B, double *pr, double *pg, double *pb, double gamma=2.2) // monitor RGB (0 to 255) to linear rgb (0.0 to 1.0) assuming default gamma 2.2
{
	*pr = pow((double) R / 255.0, 1.0/gamma);
	*pg = pow((double) G / 255.0, 1.0/gamma);
	*pb = pow((double) B / 255.0, 1.0/gamma);
	return true;
}

// a standard pal
struct CStdPalette
{
	DWORD Colors[256];

	DWORD GetClr(BYTE byCol)
	{ return Colors[byCol]; }
};

#endif
