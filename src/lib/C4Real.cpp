/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Sven Eberhardt
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

#include "C4Include.h"
#include "C4Real.h"

#include "StdCompiler.h"

void CompileFunc(C4Real &rValue, StdCompiler *pComp)
{
	// Read value (as float)
	float value;
	if (pComp->isDecompiler())
		value = static_cast<float>(rValue);
	pComp->Value(value);
	if (pComp->isCompiler())
		rValue = value;
	}
