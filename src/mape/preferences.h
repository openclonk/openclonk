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

#ifndef INC_MAPE_PREFERENCES_H
#define INC_MAPE_PREFERENCES_H

#include <glib/gtypes.h>
#include "forward.h"

struct MapePreferences_ {
	unsigned int tab_width;
	gboolean tab_to_spaces;
	gboolean auto_indentation;
	gboolean text_wrapping;
	gboolean line_numbers;
	gboolean highlight_line;
	gboolean bracket_matching;
	gboolean fixed_seed;
	unsigned int random_seed;
	unsigned int map_width;
	unsigned int map_height;
};

void mape_preferences_from_config(MapePreferences* preferences,
                                  MapeConfigFile* config);
void mape_preferences_to_config(MapePreferences* preferences,
                                MapeConfigFile* config);

#endif /* INC_MAPE_PREFERENCES_H */
