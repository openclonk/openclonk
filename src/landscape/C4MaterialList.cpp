/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Sven Eberhardt
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

/* A primitive list to store one amount value per mapped material */

#include <C4Include.h>
#include <C4MaterialList.h>

C4MaterialList::C4MaterialList()
{
	Default();
}

C4MaterialList::~C4MaterialList()
{
	Clear();
}

void C4MaterialList::Default()
{
	Reset();
}

void C4MaterialList::Clear()
{

}

void C4MaterialList::Reset()
{
	for (int cnt=0; cnt<C4MaxMaterial; cnt++)
		Amount[cnt]=0;
}

void C4MaterialList::Set(int32_t iMaterial, int32_t iAmount)
{
	if (!Inside<int32_t>(iMaterial,0,C4MaxMaterial)) return;
	Amount[iMaterial]=iAmount;
}

void C4MaterialList::Add(int32_t iMaterial, int32_t iAmount)
{
	if (!Inside<int32_t>(iMaterial,0,C4MaxMaterial)) return;
	Amount[iMaterial]+=iAmount;
}

int32_t C4MaterialList::Get(int32_t iMaterial)
{
	if (!Inside<int32_t>(iMaterial,0,C4MaxMaterial)) return 0;
	return Amount[iMaterial];
}
