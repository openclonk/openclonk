/*
 * OpenClonk, http://www.openclonk.org
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

// To be directly included by platform-specific implementations

C4ConsoleGUI::C4ConsoleGUI()
{
	state = new C4ConsoleGUI::State(this);
}

C4ConsoleGUI::~C4ConsoleGUI() {delete state;}

void C4ConsoleGUI::SetCaption(const char *caption)
{
	if (!Active) return;
#ifdef _WIN32
	// Sorry, the window caption needs to be constant so
	// the window can be found using FindWindow
	SetTitle(C4ENGINECAPTION);
#else
	SetTitle(caption);
#endif
}

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
	Console.ClearDlg(this);
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

DEFINE_STANDARD_DLG_METHODS(C4PropertyDlg)

void C4PropertyDlg::Default()
{
	state->Default();
	Active = false;
	//idSelectedDef=C4ID::None;
	Selection.Default();
}

void C4PropertyDlg::Clear()
{
	state->Clear();
	Selection.Clear();
	Console.ClearDlg(this);
	Active = false;
}

const char *C4ConsoleGUI::LIST_DIVIDER = "divid0r";
