/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009 Armin Burgmeier
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
