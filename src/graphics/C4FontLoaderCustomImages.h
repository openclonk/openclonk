/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#ifndef C4FontLoaderCustomImages_H
#define C4FontLoaderCustomImages_H

// callback class for CStdFont to allow custom images
class CStdFontCustomImages
{
protected:
	virtual bool DrawFontImage(const char* szImageTag, C4Facet& cgo, C4DrawTransform* transform) = 0;
	virtual float GetFontImageAspect(const char* szImageTag) = 0;

	friend class CStdFont;
public:
	virtual ~CStdFontCustomImages() { }
};

#endif
