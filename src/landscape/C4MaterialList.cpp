/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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
