/* mape - C4 Landscape.txt editor
 * Copyright (C) 2005 Armin Burgmeier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef INC_MAPE_RANDOM_H
#define INC_MAPE_RANDOM_H

#include "mape/forward.h"

/* Simple C-based interface to C4Random */

#ifdef MAPE_COMPILING_CPP
extern "C" {
#endif

void mape_random_seed(unsigned int seed);

#ifdef MAPE_COMPILING_CPP
} /* extern "C" */
#endif

#endif /* INC_MAPE_RANDOM_H */
