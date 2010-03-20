/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Armin Burgmeier
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

/* Value providers for mesh animation, cf. StdMeshInstance */

#ifndef INC_C4MeshAnimation
#define INC_C4MeshAnimation

#include "StdMesh.h"

class C4Action;
class C4Object;
class C4ValueArray;

enum C4AnimationValueProviderID
{
	C4AVP_Const,
	C4AVP_Linear,
	C4AVP_X,
	C4AVP_Y,
	C4AVP_AbsX,
	C4AVP_AbsY,
	C4AVP_XDir,
	C4AVP_YDir,
	C4AVP_RDir,
	C4AVP_CosR,
	C4AVP_SinR,
	C4AVP_CosV,
	C4AVP_SinV,
	C4AVP_Action
};

enum C4AnimationEnding
{
	ANIM_Loop,
	ANIM_Hold,
	ANIM_Remove
};

// Keep a constant value
class C4ValueProviderConst: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderConst(FIXED value);
	virtual bool Execute();
};

// Interpolate linearly in time between two values
class C4ValueProviderLinear: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderLinear(FIXED pos, FIXED begin, FIXED end, int32_t length, C4AnimationEnding ending);
	virtual bool Execute();

private:
	const FIXED Begin;
	const FIXED End;
	const int32_t Length;
	const C4AnimationEnding Ending;

	int32_t LastTick;
};

class C4ValueProviderX: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderX(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const int32_t Length;

	FIXED LastX;
};

class C4ValueProviderY: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderY(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const int32_t Length;

	FIXED LastY;
};

class C4ValueProviderAbsX: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderAbsX(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const int32_t Length;

	FIXED LastX;
};

class C4ValueProviderAbsY: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderAbsY(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const int32_t Length;

	FIXED LastY;
};

class C4ValueProviderXDir: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderXDir(const C4Object* object, FIXED begin, FIXED end, FIXED max_xdir);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED MaxXDir;
};

class C4ValueProviderYDir: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderYDir(const C4Object* object, FIXED begin, FIXED end, FIXED max_ydir);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED MaxYDir;
};

class C4ValueProviderRDir: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderRDir(const C4Object* object, FIXED begin, FIXED end, FIXED max_rdir);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED MaxRDir;
};

class C4ValueProviderCosR: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderCosR(const C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED Offset;
};

class C4ValueProviderSinR: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderSinR(const C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED Offset;
};

class C4ValueProviderCosV: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderCosV(const C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED Offset;
};

class C4ValueProviderSinV: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderSinV(const C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const FIXED Begin;
	const FIXED End;
	const FIXED Offset;
};

class C4ValueProviderAction: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderAction(C4Object* object);
	virtual bool Execute();

private:
	C4Object* Object;
};

// Reference another value (which is convertible to FIXED), and optionally scale it
template<typename SourceT>
class C4ValueProviderRef: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderRef(const SourceT& ref, FIXED scale):
		Ref(ref), Scale(scale) {}

	virtual bool Execute()
	{
		Value = Scale * Ref;
		return true;
	}

private:
	const SourceT& Ref;
	FIXED Scale;
};

StdMeshInstance::ValueProvider* CreateValueProviderFromArray(C4Object* pForObj, C4ValueArray& Data);

#endif
