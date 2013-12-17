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

/**
 * SECTION:mape-random
 * @title: MapeRandom
 * @short_description: Interface to the random number generator
 * @include: mape/random.h
 * @stability: Unstable
 *
 * These functions provide a simple interface to the random number generator
 * used for map generation in the Clonk engine.
 **/

#include "mape/cpp-handles/random-handle.h"
#include "mape/random.h"

/*
 * Public API.
 */

/**
 * mape_random_seed:
 * @seed: Value to seed the random number generator with.
 *
 * Sets the random seed for the Clonk random number generator that is used
 * when generating a map with C4MapCreatorS2.
 */
void
mape_random_seed(unsigned int seed)
{
  c4_random_handle_seed(seed);
}

/* vim:set et sw=2 ts=2: */
