/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
 // startup screen

#ifndef INC_C4LoaderScreen
#define INC_C4LoaderScreen

#include "graphics/C4FacetEx.h"


class C4LoaderScreen
{
public:
	enum Flag: int {
		BLACK = 0x00,
		BACKGROUND = 0x01,
		TITLE = 0x02,
		PROGRESS = 0x04,
		LOG = 0x08,
		PROCESS = 0x0f,

		// All of the above
		ALL = 0xFF
	};

protected:
	C4FacetSurface fctBackground; // background image
	char *szInfo;      // info text to be drawn on loader screen
	bool fBlackScreen; // if set, a black screen is drawn instead of a loader

	std::map<C4Group*, const std::string> loaders;
	void SeekLoaderScreens(C4Group &rFromGrp, const std::string &wildcard);

public:
	C4LoaderScreen();
	~C4LoaderScreen();

	bool Init(std::string szLoaderSpec); // inits and loads from global C4Game-class
	void SetBlackScreen(bool fIsBlack);  // enabled/disables drawing of loader screen
	// draw loader screen (does not page flip!)
	void Draw(C4Facet &cgo, Flag options = Flag::ALL, int iProgress = 0, class C4LogBuffer *pLog = nullptr, int Process = 0);
};

#endif //INC_C4LoaderScreen
