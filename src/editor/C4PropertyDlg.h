/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2006-2007  Armin Burgmeier
 * Copyright (c) 2010  Mortimer
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

/* Console mode dialog for object properties and script interface */

#ifndef INC_C4PropertyDlg
#define INC_C4PropertyDlg

#include "C4ObjectList.h"

#include "C4ConsoleGUI.h"

class C4PropertyDlg
{
	friend class C4ConsoleGUI;
private:
	class State;
	State *state;
public:
	C4PropertyDlg();
	~C4PropertyDlg();
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void UpdateInputCtrl(C4Object *pObj);
	bool Open();
	bool Update();
	bool Update(C4ObjectList &rSelection);
	bool Active;
public: // ToolsDlg fields are public as well...
	C4ID idSelectedDef;
	C4ObjectList Selection;
};

#endif
