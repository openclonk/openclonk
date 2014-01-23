/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, Günther Brammer
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

// To be directly included by platform-specific implementations

C4ConsoleGUI::C4ConsoleGUI()
{
	state = new C4ConsoleGUI::State(this);
}

C4ConsoleGUI::~C4ConsoleGUI() {delete state;}

#define DEFINE_STANDARD_DLG_METHODS(cls)\
cls::cls()\
{\
	state = new cls::State(this);\
	Default();\
}\
\
cls::~cls()\
{\
	Clear();\
	delete state;\
}\

DEFINE_STANDARD_DLG_METHODS(C4ToolsDlg)

void C4ToolsDlg::Clear()
{
	state->Clear();
	Console.ToolsDlgClose();
	Active = false;
}

void C4ToolsDlg::Default()
{
	state->Default();
	Active = false;
	Tool = SelectedTool = C4TLS_Brush;
	Grade = C4TLS_GradeDefault;
	ModeIFT = true;
	SCopy("Earth",Material);
	SCopy("earth",Texture);
}
