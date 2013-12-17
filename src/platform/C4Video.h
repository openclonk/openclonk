/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Captures uncompressed AVI from game view */

#ifndef INC_C4Video
#define INC_C4Video

#include <C4Surface.h>

// avoid pulling in vfw.h in every file
struct IAVIFile;
struct IAVIStream;

class C4TargetFacet;

class C4Video
{
public:
	C4Video();
	~C4Video();
protected:
	bool Active;
	IAVIFile *  pAviFile;
	IAVIStream * pAviStream;
	int AviFrame;
	double AspectRatio;
	int X,Y,Width,Height;
	BYTE *Buffer;
	int BufferSize;
	int InfoSize;
	bool Recording;
	C4Surface * Surface;
	int ShowFlash;
public:
	void Draw();
	void Draw(C4TargetFacet &cgo);
	void Resize(int iChange);
	bool Start(const char *szFilename);
	void Default();
	void Init(C4Surface * sfcSource, int iWidth=768, int iHeight=576);
	void Clear();
	bool Start();
	bool Stop();
	bool Toggle();
	void Execute();
	bool Reduce();
	bool Enlarge();
protected:
	bool RecordFrame();
	bool AdjustPosition();
};

#endif
