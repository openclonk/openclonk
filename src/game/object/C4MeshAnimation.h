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
#include "C4ObjectPtr.h"

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
class C4ValueProviderConst: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderConst() {}
	C4ValueProviderConst(FIXED value);
	virtual bool Execute();
};

// Interpolate linearly in time between two values
class C4ValueProviderLinear: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderLinear(): Begin(Fix0), End(Fix0), Length(0), Ending(ANIM_Loop), LastTick(0) {}
	C4ValueProviderLinear(FIXED pos, FIXED begin, FIXED end, int32_t length, C4AnimationEnding ending);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
private:
	FIXED Begin;
	FIXED End;
	int32_t Length;
	C4AnimationEnding Ending;

	int32_t LastTick;
};

class C4ValueProviderX: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderX(): Object(NULL), Begin(Fix0), End(Fix0), Length(0), LastX(Fix0) {}
	C4ValueProviderX(C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	int32_t Length;

	FIXED LastX;
};

class C4ValueProviderY: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderY(): Object(NULL), Begin(Fix0), End(Fix0), Length(0), LastY(Fix0) {}
	C4ValueProviderY(C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	int32_t Length;

	FIXED LastY;
};

class C4ValueProviderAbsX: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAbsX(): Object(NULL), Begin(Fix0), End(Fix0), Length(0), LastX(Fix0) {}
	C4ValueProviderAbsX(C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	int32_t Length;

	FIXED LastX;
};

class C4ValueProviderAbsY: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAbsY(): Object(NULL), Begin(Fix0), End(Fix0), Length(0), LastY(Fix0) {}
	C4ValueProviderAbsY(C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	int32_t Length;

	FIXED LastY;
};

class C4ValueProviderXDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderXDir(): Object(NULL), Begin(Fix0), End(Fix0), MaxXDir(Fix0) {}
	C4ValueProviderXDir(C4Object* object, FIXED begin, FIXED end, FIXED max_xdir);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED MaxXDir;
};

class C4ValueProviderYDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderYDir(): Object(NULL), Begin(Fix0), End(Fix0), MaxYDir(Fix0) {}
	C4ValueProviderYDir(C4Object* object, FIXED begin, FIXED end, FIXED max_ydir);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED MaxYDir;
};

class C4ValueProviderRDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderRDir(): Object(NULL), Begin(Fix0), End(Fix0), MaxRDir(Fix0) {}
	C4ValueProviderRDir(C4Object* object, FIXED begin, FIXED end, FIXED max_rdir);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED MaxRDir;
};

class C4ValueProviderCosR: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderCosR(): Object(NULL), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderCosR(C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED Offset;
};

class C4ValueProviderSinR: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderSinR(): Object(NULL), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderSinR(C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED Offset;
};

class C4ValueProviderCosV: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderCosV(): Object(NULL), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderCosV(C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED Offset;
};

class C4ValueProviderSinV: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderSinV(): Object(NULL), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderSinV(C4Object* object, FIXED begin, FIXED end, FIXED offset);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
	FIXED Begin;
	FIXED End;
	FIXED Offset;
};

class C4ValueProviderAction: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAction(): Object(NULL) {}
	C4ValueProviderAction(C4Object* object);
	virtual bool Execute();

	virtual void CompileFunc(StdCompiler* pComp);
	virtual void DenumeratePointers() { Object.DenumeratePointers(); }
	virtual void EnumeratePointers() { Object.EnumeratePointers(); }
private:
	C4ObjectPtr Object;
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
