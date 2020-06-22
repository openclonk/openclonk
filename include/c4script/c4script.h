/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
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

#ifndef C4SCRIPTSTANDALONE_H
#define C4SCRIPTSTANDALONE_H

#ifdef __cplusplus
extern "C" {
#endif

int c4s_runfile(const char *filename);
int c4s_runstring(const char *script);

int c4s_checkfile(const char *filename);
int c4s_checkstring(const char *script);

#ifdef __cplusplus
}
#endif

#endif // C4SCRIPTSTANDALONE_H
