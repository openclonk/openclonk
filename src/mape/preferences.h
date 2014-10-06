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

#ifndef INC_MAPE_PREFERENCES_H
#define INC_MAPE_PREFERENCES_H

#include <glib.h>
#include "mape/forward.h"

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
	double map_zoom;
};

void mape_preferences_from_config(MapePreferences* preferences,
                                  MapeConfigFile* config);
void mape_preferences_to_config(MapePreferences* preferences,
                                MapeConfigFile* config);

#endif /* INC_MAPE_PREFERENCES_H */
