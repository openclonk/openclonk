/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2010  Martin Plicht
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
