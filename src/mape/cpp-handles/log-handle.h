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

#ifndef INC_MAPE_C4_LOG_HANDLE_H
#define INC_MAPE_C4_LOG_HANDLE_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _C4MapgenHandle C4MapgenHandle;

void c4_log_handle_clear();
const char* c4_log_handle_get_first_log_message();

G_END_DECLS

#endif /* INC_MAPE_C4_LOG_HANDLE_H */
