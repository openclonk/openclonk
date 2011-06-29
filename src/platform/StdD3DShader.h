/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Sven2
 * Copyright (c) 2009, RedWolf Design GmbH, http://www.clonk.de
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * "Clonk" is a registered trademark of Matthes Bender. */

/* Direct3D shader used for FoW-modulated blitting */

#if defined(USE_DIRECTX) && !defined(INC_STDD3DSHADER)
#define INC_STDD3DSHADER

#include <StdD3D.h>

class CStdD3DShader
{
private:
	IDirect3DDevice9 *pDevice; // DX device parenting shader
	IDirect3DPixelShader9 *pInterface; // DX interface
	ID3DXConstantTable *pConstTable; // offsets of shader inputs
	ID3DXBuffer *pCodeBuffer; // buffer containing shader bytecode

	void ReleaseCode(); // release init-time members only

	bool Compile(bool fMod22, bool fColoredFoW); // create shader byte code
	int GetConstRegister(const char *szName, D3DXREGISTER_SET eRegType); // get named register index for shader input
	bool CreateShader(); // create actual DX pixel shader object

	bool Error(const char *szMsg); // error to ddraw; always return false

public:
	// todo: Make those constants in the shader
	int iInTexIndex, iFoWTexIndex, iFoWTransformIndex; // constant indices used to pass data to the shader

	CStdD3DShader();
	~CStdD3DShader();

	void Release(); // properly release interfaces
	void Discard(); // zero members (e.g. after device has been destroyed)

	bool Init(IDirect3DDevice9 *pDevice, bool fMod2, bool fColoredFoW);

	IDirect3DPixelShader9 *GetInterface() const { return pInterface; }
};

#endif // defined(USE_DIRECTX) && !defined(INC_STDD3DSHADER)
