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
};

void mape_preferences_from_config(MapePreferences* preferences,
                                  MapeConfigFile* config);
void mape_preferences_to_config(MapePreferences* preferences,
                                MapeConfigFile* config);

#endif /* INC_MAPE_PREFERENCES_H */
