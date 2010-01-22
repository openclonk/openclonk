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
	C4ValueProviderConst(float value);
	virtual bool Execute();
};

// Interpolate linearly in time between two values
class C4ValueProviderLinear: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderLinear(float pos, float begin, float end, unsigned int length, C4AnimationEnding ending);
	virtual bool Execute();

private:
	const float Begin;
	const float End;
	const unsigned int Length;
	const C4AnimationEnding Ending;

	int32_t LastTick;
};

class C4ValueProviderX: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderX(const C4Object* object, float pos, float begin, float end, unsigned int length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const unsigned int Length;

	float LastX;
};

class C4ValueProviderY: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderY(const C4Object* object, float pos, float begin, float end, unsigned int length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const unsigned int Length;

	float LastY;
};

class C4ValueProviderAbsX: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderAbsX(const C4Object* object, float pos, float begin, float end, unsigned int length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const unsigned int Length;

	float LastX;
};

class C4ValueProviderAbsY: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderAbsY(const C4Object* object, float pos, float begin, float end, unsigned int length);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const unsigned int Length;

	float LastY;
};

class C4ValueProviderXDir: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderXDir(const C4Object* object, float begin, float end, FIXED max_xdir);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED MaxXDir;
};

class C4ValueProviderYDir: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderYDir(const C4Object* object, float begin, float end, FIXED max_ydir);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED MaxYDir;
};

class C4ValueProviderRDir: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderRDir(const C4Object* object, float begin, float end, FIXED max_rdir);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED MaxRDir;
};

class C4ValueProviderCosR: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderCosR(const C4Object* object, float begin, float end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED Offset;
};

class C4ValueProviderSinR: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderSinR(const C4Object* object, float begin, float end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED Offset;
};

class C4ValueProviderCosV: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderCosV(const C4Object* object, float begin, float end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED Offset;
};

class C4ValueProviderSinV: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderSinV(const C4Object* object, float begin, float end, FIXED offset);
	virtual bool Execute();

private:
	const C4Object* const Object;
	const float Begin;
	const float End;
	const FIXED Offset;
};

class C4ValueProviderAction: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderAction(const C4Object* object);
	virtual bool Execute();

private:
	const C4Action& Action;
};

// Reference another value (which is convertible to float), and optionally scale it
template<typename SourceT>
class C4ValueProviderRef: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderRef(const SourceT& ref, float scale):
		Ref(ref), Scale(scale) {}

	virtual bool Execute()
	{
		Value = static_cast<float>(Ref) * Scale;
		return true;
	}

private:
	const SourceT& Ref;
	float Scale;
};

StdMeshInstance::ValueProvider* CreateValueProviderFromArray(C4Object* pForObj, C4ValueArray& Data);

#endif
