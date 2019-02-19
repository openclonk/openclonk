/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Console forward from C4ConsoleGUI to C4ConsoleGUI::State */

#ifndef INC_C4ConsoleQt
#define INC_C4ConsoleQt

#ifdef WITH_QT_EDITOR

// disable OPENGL_ES
// (not necessery if Qt is compiled with -opengl desktop)
//#define QT_OPENGL_ES_2
//#define QT_NO_OPENGL_ES_2
//#define QT_OPENGL_ES
//#define QT_NO_OPENGL_ES
#include <QtWidgets>
#include <qabstractitemmodel.h>
#include <QAbstractTableModel>

// TODO: If we remove the other editors, state and consolegui can be merged and the relevant header go into this file
// For now, just use this to include Qt

#endif // WITH_QT_EDITOR


#endif //INC_C4ConsoleQt
