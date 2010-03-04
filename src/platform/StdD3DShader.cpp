/* Copyright (c) 2009, RedWolf Design GmbH, http://www.clonk.de

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

"Clonk" is a registered trademark of Matthes Bender. */

/* Direct3D shader used for FoW-modulated blitting */

#include "C4Include.h"

#ifdef USE_DIRECTX
#include <StdD3DShader.h>

static const char *szShaderCode =
	     "sampler in_tex;  /* Blitted input texture */"
	"\n" "sampler fow_tex; /* FogOfWar modulation texture */"
	"\n" ""
	"\n" "float4 fow_proj; /* Projection of FogOfWar-texture into screen space: xy=scale, zw=offset */"
	"\n" ""
	"\n" "struct PS_INPUT"
	"\n" "{"
	"\n" "	float4  Color   : COLOR0;    /* Per-vertex modulation color */"
	"\n" "	float2  TexPos  : TEXCOORD0; /* Texture position */"
	"\n" "	float2  ScreenPos : VPOS;    /* Output screen position */"
	"\n" "};"
	"\n" ""
	"\n" "struct PS_OUTPUT"
	"\n" "{"
	"\n" "	float4  Color   : COLOR;     /* Calculated output color */"
	"\n" "};"
	"\n" ""
	"\n" "/* Main shader function */"
	"\n" "PS_OUTPUT MainExec(PS_INPUT In)"
	"\n" "{"
	"\n" "	PS_OUTPUT Out;"
	"\n" "	/* Sample source texture */"
	"\n" "	float4 in_tex_color = tex2D(in_tex,In.TexPos);"
	"\n" "	/* Sample FoW texture */"
	"\n" "	float4 fow_tex_color = tex2D(fow_tex,(In.ScreenPos-fow_proj.zw)*fow_proj.xy);"
	"\n" "#ifdef MOD2"
	"\n" "	/* Apply Mod2-modulation (x2 scale input color and signed add modulation color) */"
	"\n" "	Out.Color.rgb = saturate(in_tex_color.rgb*2 + In.Color.rgb - 0.5);"
	"\n" "#else"
	"\n" "	/* Apply regular modulation */"
	"\n" "	Out.Color.rgb = In.Color.rgb * in_tex_color.rgb;"
	"\n" "#endif"
	"\n" "	/* Apply FoW */"
	"\n" "#ifdef COLORED_FOW"
	"\n" "	/* Colored FoW: Mix source color and FoW color */"
	"\n" "	Out.Color.rgb = Out.Color.rgb * fow_tex_color.a + fow_tex_color.rgb * (1.0 - fow_tex_color.a);"
	"\n" "#else"
	"\n" "	/* Black FoW: Just darken */"
	"\n" "	Out.Color.rgb *= fow_tex_color.a;"
	"\n" "#endif"
	"\n" "	/* Alpha values modulated */"
	"\n" "	Out.Color.a = In.Color.a * in_tex_color.a;"
	"\n" "	return Out;"
	"\n" "}";


CStdD3DShader::CStdD3DShader() : pDevice(NULL), pInterface(NULL), pConstTable(NULL), pCodeBuffer(NULL)
	{
	}

CStdD3DShader::~CStdD3DShader()
	{
	Discard();
	}

void CStdD3DShader::Release()
	{
	// properly release interface
	ReleaseCode();
	if (pInterface) { pInterface->Release(); pInterface = NULL; }
	pDevice = NULL;
	}

void CStdD3DShader::ReleaseCode()
	{
	// release init-time temp values
	if (pConstTable) { pConstTable->Release(); pConstTable = NULL; }
	if (pCodeBuffer) { 	pCodeBuffer->Release(); pCodeBuffer = NULL; }
	}

void CStdD3DShader::Discard()
	{
	// discard interface without deleting
	pCodeBuffer = NULL;
	pConstTable = NULL;
	pInterface = NULL;
	pDevice = NULL;
	}

bool CStdD3DShader::Error(const char *szMsg)
	{
	return lpDDraw->Error(FormatString("Direct3D Shader error: %s", szMsg).getData());
	}

bool CStdD3DShader::Compile(bool fMod2, bool fColoredFoW)
	{
	ID3DXBuffer *pErrMsg=NULL;
	// load and compile shader
	D3DXMACRO defines[3]; int i=0;
	if (fMod2)
		{
		defines[i].Name = "MOD2";
		defines[i].Definition = "1";
		i++;
		}
	if (fColoredFoW)
		{
		defines[i].Name = "COLORED_FOW";
		defines[i].Definition = "1";
		i++;
		}
	defines[i].Name = 0;
	defines[i].Definition = 0;
	HRESULT hr = D3DXCompileShader(szShaderCode, strlen(szShaderCode), i?defines:NULL, NULL, "MainExec", D3DXGetPixelShaderProfile(pDevice), 0, &pCodeBuffer, &pErrMsg, &pConstTable);
	if (hr != D3D_OK)
		{
		StdStrBuf errmsg;
		errmsg.Format("D3DXCompileShaderFromFile error %x", hr);
		if (pErrMsg)
			{
			errmsg.AppendFormat(": %*s", pErrMsg->GetBufferSize(), pErrMsg->GetBufferPointer());
			pErrMsg->Release();
			}
		return Error(errmsg.getData());
		}
	return true;
	}

int CStdD3DShader::GetConstRegister(const char *szName, D3DXREGISTER_SET eRegType)
	{
	// get named register index for shader input
	// errors are not fatal; shader will probably just not need this input
	if (!pConstTable) return 0;
	D3DXHANDLE hConstant = pConstTable->GetConstantByName(NULL, szName);
	if (!hConstant) return 0;
	D3DXCONSTANT_DESC constDesc;
	UINT c=1;
	HRESULT hr = pConstTable->GetConstantDesc(hConstant, &constDesc, &c);
	if (hr != D3D_OK) return 0;
	if (constDesc.RegisterSet != eRegType) return 0;
	return constDesc.RegisterIndex;
	}

bool CStdD3DShader::CreateShader()
	{
	// creating actual DX shader!
	HRESULT hr = pDevice->CreatePixelShader((const DWORD *)pCodeBuffer->GetBufferPointer(), &pInterface);
	if (hr != D3D_OK) return Error(FormatString("CreatePixelShader error %x", hr).getData());
	return true;
	}

bool CStdD3DShader::Init(IDirect3DDevice9 *pDevice, bool fMod2, bool fColoredFoW)
	{
	// re-init?
	if (pInterface) Release();
	// store device for easy acccess
	this->pDevice = pDevice;
	// compile code
	if (!Compile(fMod2, fColoredFoW)) { Release(); return false; }
	// get const registers
	iInTexIndex = GetConstRegister("in_tex", D3DXRS_SAMPLER);
	iFoWTexIndex = GetConstRegister("fow_tex", D3DXRS_SAMPLER);
	iFoWTransformIndex = GetConstRegister("fow_proj", D3DXRS_FLOAT4);
	// create actual shader on device
	if (!CreateShader()) { Release(); return false; }
	// del temp objects
	ReleaseCode();
	// done, success!
	return true;
	}



#endif
