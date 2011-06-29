/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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
	SURFACE Surface;
	int ShowFlash;
public:
	void Draw();
	void Draw(C4TargetFacet &cgo);
	void Resize(int iChange);
	bool Start(const char *szFilename);
	void Default();
	void Init(SURFACE sfcSource, int iWidth=768, int iHeight=576);
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
