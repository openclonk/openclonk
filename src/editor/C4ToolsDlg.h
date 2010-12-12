/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005, 2007  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2006-2007  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Drawing tools dialog for landscape editing in console mode */

#ifndef INC_C4ToolsDlg
#define INC_C4ToolsDlg

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif

#include "C4Constants.h"
#include "C4ConsoleGUI.h"

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
	bool SetLandscapeMode(int32_t iMode, bool fThroughControl=false);
	bool SetIFT(bool fIFT);
	bool ToggleIFT() { return !!SetIFT(!ModeIFT); }
	bool SelectTexture(const char *szTexture);
	bool SelectMaterial(const char *szMaterial);
	void SetAlternateTool();
	void ResetAlternateTool();
protected:
	void AssertValidTexture();
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
};

#endif //INC_C4ToolsDlg
