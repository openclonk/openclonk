/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003  Matthes Bender
 * Copyright (c) 2007  Günther Brammer
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

/* Load strings from a primitive memory string table */

#ifndef INC_STD_RES_STR_2_H
#define INC_STD_RES_STR_2_H
const char *LoadResStr(const char *id);
char *LoadResStrNoAmp(const char *id);
char *GetResStr(const char *id, const char *strTable);

void SetResStrTable(char *pTable);
void ClearResStrTable();
bool IsResStrTableLoaded();

#endif // INC_STD_RES_STR_2_H
