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

/* Drawing tools dialog for landscape editing in console mode */

#ifndef INC_C4ToolsDlg
#define INC_C4ToolsDlg


#include "config/C4Constants.h"
#include "landscape/C4Landscape.h"

const int32_t
	C4TLS_Brush  = 0,
	C4TLS_Line   = 1,
	C4TLS_Rect   = 2,
	C4TLS_Fill   = 3,
	C4TLS_Picker = 4;

const int32_t
	C4TLS_GradeMax     = 50,
	C4TLS_GradeMin     = 1,
	C4TLS_GradeDefault = 5;

#define C4TLS_MatSky "Sky"

enum class LandscapeMode;
class C4ToolsDlg
{
	friend class C4ConsoleGUI;
	private:
	class State;
	State *state;
public:
	C4ToolsDlg();
	~C4ToolsDlg();
public:
	bool Active;
	int32_t Tool, SelectedTool;
	int32_t Grade;
	bool ModeIFT;
	char Material[C4M_MaxName+1];
	char Texture[C4M_MaxName+1];
	bool ModeBack; // If true, IFT is ignored and background material/texture is always used
	char BackMaterial[C4M_MaxName+1];
	char BackTexture[C4M_MaxName+1];
public:
	void Default();
	void Clear();
	bool PopTextures();
	bool PopMaterial();
	bool ChangeGrade(int32_t iChange);
	void NeedPreviewUpdate();
	bool Open();
	bool SetGrade(int32_t iGrade);
	bool SetTool(int32_t iTool, bool fTemp);
	bool ToggleTool() { return !!SetTool((Tool+1)%4, false); }
	bool SetLandscapeMode(LandscapeMode iMode, bool flat_chunk_shapes, bool fThroughControl=false);
	bool SetIFT(bool fIFT);
	bool ToggleIFT() { return !!SetIFT(!ModeIFT); }
	bool SelectTexture(const char *szTexture, bool by_console_gui=false);
	bool SelectMaterial(const char *szMaterial, bool by_console_gui = false);
	bool SelectBackTexture(const char *szTexture, bool by_console_gui = false);
	bool SelectBackMaterial(const char *szMaterial, bool by_console_gui = false);
	void SetAlternateTool();
	void ResetAlternateTool();
	bool IsGradedTool() const { return Tool == C4TLS_Brush || Tool == C4TLS_Line || Tool == C4TLS_Fill; } // return whether Grade measure affects selected tool
protected:
	void AssertValidTexture();
	void AssertValidBackTexture();
	void LoadBitmaps();
	void EnableControls();
	void UpdateIFTControls();
	void InitGradeCtrl();
	void InitMaterialCtrls();
	void UpdateToolCtrls();
	void UpdateTextures();
	void SetColorPattern(const char *szMaterial, const char *szTexture);
public:
	void UpdateLandscapeModeCtrls();
	void SetTexture(const char *szTexture);
	void SetMaterial(const char *szMaterial);
	void SetBackTexture(const char *szTexture);
	void SetBackMaterial(const char *szMaterial);
};

#endif //INC_C4ToolsDlg
