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

/* Drawing tools dialog for landscape editing in console mode */

#include <C4Include.h>
#include <C4ToolsDlg.h>
#include <C4Console.h>
#include <C4Texture.h>
#include <C4Landscape.h>
#include <C4GameControl.h>

bool C4ToolsDlg::Open()
{
	// Create dialog window
	Console.ToolsDlgOpen(this);
	Active = true;
	// Update contols
	InitGradeCtrl();
	UpdateLandscapeModeCtrls();
	UpdateToolCtrls();
	UpdateIFTControls();
	InitMaterialCtrls();
	EnableControls();
	return true;
}

bool C4ToolsDlg::SetTool(int32_t iTool, bool fTemp)
{
	Tool=iTool;
	if (!fTemp) SelectedTool = Tool;
	UpdateToolCtrls();
	NeedPreviewUpdate();
	return true;
}

void C4ToolsDlg::InitMaterialCtrls()
{
	// Materials
	Console.ToolsDlgInitMaterialCtrls(this);
	// Textures
	UpdateTextures();
}

void C4ToolsDlg::SetMaterial(const char *szMaterial)
{
	SCopy(szMaterial,Material,C4M_MaxName);
	AssertValidTexture();
	EnableControls();
	if (::Landscape.Mode==C4LSC_Static) UpdateTextures();
	NeedPreviewUpdate();
}

void C4ToolsDlg::SetTexture(const char *szTexture)
{
	// assert valid (for separator selection)
	if (!::TextureMap.GetTexture(szTexture))
	{
		// ensure correct texture is in dlg
		Console.ToolsDlgSetTexture(this, szTexture);
		return;
	}
	SCopy(szTexture,Texture,C4M_MaxName);
	NeedPreviewUpdate();
}

bool C4ToolsDlg::SetIFT(bool fIFT)
{
	if (fIFT) ModeIFT = 1; else ModeIFT=0;
	UpdateIFTControls();
	NeedPreviewUpdate();
	return true;
}

void C4ToolsDlg::SetColorPattern(const char *szMaterial, const char *szTexture)
{
}

bool C4ToolsDlg::SetGrade(int32_t iGrade)
{
	Grade = Clamp(iGrade,C4TLS_GradeMin,C4TLS_GradeMax);
	NeedPreviewUpdate();
	return true;
}

bool C4ToolsDlg::ChangeGrade(int32_t iChange)
{
	Grade = Clamp(Grade+iChange,C4TLS_GradeMin,C4TLS_GradeMax);
	NeedPreviewUpdate();
	InitGradeCtrl();
	return true;
}

bool C4ToolsDlg::SetLandscapeMode(int32_t iMode, bool fThroughControl)
{
	int32_t iLastMode=::Landscape.Mode;
	// Exact to static: confirm data loss warning
	if (iLastMode==C4LSC_Exact)
		if (iMode==C4LSC_Static)
			if (!fThroughControl)
				if (!Console.Message(LoadResStr("IDS_CNS_EXACTTOSTATIC"),true))
					return false;
	// send as control
	if (!fThroughControl)
	{
		::Control.DoInput(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_SetMode, iMode), CDT_Decide);
		return true;
	}
	// Set landscape mode
	::Landscape.SetMode(iMode);
	// Exact to static: redraw landscape from map
	if (iLastMode==C4LSC_Exact)
		if (iMode==C4LSC_Static)
			::Landscape.MapToLandscape();
	// Assert valid tool
	if (iMode!=C4LSC_Exact)
		if (SelectedTool==C4TLS_Fill)
			SetTool(C4TLS_Brush, false);
	// Update controls
	UpdateLandscapeModeCtrls();
	EnableControls();
	UpdateTextures();
	// Success
	return true;
}

void C4ToolsDlg::AssertValidTexture()
{
	// Static map mode only
	if (::Landscape.Mode!=C4LSC_Static) return;
	// Ignore if sky
	if (SEqual(Material,C4TLS_MatSky)) return;
	// Current material-texture valid
	if (::TextureMap.GetIndex(Material,Texture,false)) return;
	// Find valid material-texture
	const char *szTexture;
	for (int32_t iTexture=0; (szTexture=::TextureMap.GetTexture(iTexture)); iTexture++)
	{
		if (::TextureMap.GetIndex(Material,szTexture,false))
			{ SelectTexture(szTexture); return; }
	}
	// No valid texture found
}

bool C4ToolsDlg::SelectTexture(const char *szTexture)
{
	Console.ToolsDlgSelectTexture(this, szTexture);
	SetTexture(szTexture);
	return true;
}

bool C4ToolsDlg::SelectMaterial(const char *szMaterial)
{
	Console.ToolsDlgSetMaterial(this, szMaterial);
	SetMaterial(szMaterial);
	return true;
}

void C4ToolsDlg::SetAlternateTool()
{
	// alternate tool is the picker in any mode
	SetTool(C4TLS_Picker, true);
}

void C4ToolsDlg::ResetAlternateTool()
{
	// reset tool to selected tool in case alternate tool was set
	SetTool(SelectedTool, true);
}
