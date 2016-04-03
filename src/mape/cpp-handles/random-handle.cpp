/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#include "C4Include.h"
#include "lib/C4Random.h"
#include "mape/cpp-handles/random-handle.h"

extern "C" {

void c4_random_handle_seed(unsigned int seed)
{
  FixedRandom(seed);
}

} // extern "C"
