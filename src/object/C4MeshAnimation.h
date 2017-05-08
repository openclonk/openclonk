/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* Value providers for mesh animation, cf. StdMeshInstance */

#ifndef INC_C4MeshAnimation
#define INC_C4MeshAnimation

#include "lib/StdMesh.h"
#include "object/C4ObjectPtr.h"

enum C4AnimationValueProviderID
{
	C4AVP_Const,
	C4AVP_Linear,
	C4AVP_X,
	C4AVP_Y,
	C4AVP_R,
	C4AVP_AbsX,
	C4AVP_AbsY,
	C4AVP_Dist,
	C4AVP_XDir,
	C4AVP_YDir,
	C4AVP_RDir,
	C4AVP_AbsRDir,
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
	C4ValueProviderConst() = default;
	C4ValueProviderConst(C4Real value);
	bool Execute() override;
};

// Interpolate linearly in time between two values
class C4ValueProviderLinear: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderLinear(): Begin(Fix0), End(Fix0) {}
	C4ValueProviderLinear(C4Real pos, C4Real begin, C4Real end, int32_t length, C4AnimationEnding ending);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
private:
	C4Real Begin;
	C4Real End;
	int32_t Length{0};
	C4AnimationEnding Ending{ANIM_Loop};

	int32_t LastTick{0};
};

class C4ValueProviderX: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderX(): Object(nullptr), Begin(Fix0), End(Fix0) {}
	C4ValueProviderX(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	int32_t Length{0};
};

class C4ValueProviderY: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderY(): Object(nullptr), Begin(Fix0), End(Fix0) {}
	C4ValueProviderY(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	int32_t Length{0};
};

class C4ValueProviderR: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderR(): Object(nullptr), Begin(Fix0), End(Fix0) {}
	C4ValueProviderR(C4Object* object, C4Real begin, C4Real end);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
};

class C4ValueProviderAbsX: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAbsX(): Object(nullptr), Begin(Fix0), End(Fix0) {}
	C4ValueProviderAbsX(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	int32_t Length{0};
};

class C4ValueProviderAbsY: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAbsY(): Object(nullptr), Begin(Fix0), End(Fix0) {}
	C4ValueProviderAbsY(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	int32_t Length{0};
};

class C4ValueProviderDist: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderDist(): Object(nullptr), Begin(Fix0), End(Fix0) {}
	C4ValueProviderDist(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	int32_t Length{0};
};

class C4ValueProviderXDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderXDir(): Object(nullptr), Begin(Fix0), End(Fix0), MaxXDir(Fix0) {}
	C4ValueProviderXDir(C4Object* object, C4Real begin, C4Real end, C4Real max_xdir);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real MaxXDir;
};

class C4ValueProviderYDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderYDir(): Object(nullptr), Begin(Fix0), End(Fix0), MaxYDir(Fix0) {}
	C4ValueProviderYDir(C4Object* object, C4Real begin, C4Real end, C4Real max_ydir);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real MaxYDir;
};

class C4ValueProviderRDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderRDir(): Object(nullptr), Begin(Fix0), End(Fix0), MinRDir(Fix0), MaxRDir(Fix0) {}
	C4ValueProviderRDir(C4Object* object, C4Real begin, C4Real end, C4Real min_rdir, C4Real max_rdir);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real MinRDir;
	C4Real MaxRDir;
};

class C4ValueProviderAbsRDir: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAbsRDir(): Object(nullptr), Begin(Fix0), End(Fix0), MinRDir(Fix0), MaxRDir(Fix0) {}
	C4ValueProviderAbsRDir(C4Object* object, C4Real begin, C4Real end, C4Real min_rdir, C4Real max_rdir);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real MinRDir;
	C4Real MaxRDir;
};

class C4ValueProviderCosR: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderCosR(): Object(nullptr), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderCosR(C4Object* object, C4Real begin, C4Real end, C4Real offset);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real Offset;
};

class C4ValueProviderSinR: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderSinR(): Object(nullptr), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderSinR(C4Object* object, C4Real begin, C4Real end, C4Real offset);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real Offset;
};

class C4ValueProviderCosV: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderCosV(): Object(nullptr), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderCosV(C4Object* object, C4Real begin, C4Real end, C4Real offset);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real Offset;
};

class C4ValueProviderSinV: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderSinV(): Object(nullptr), Begin(Fix0), End(Fix0), Offset(Fix0) {}
	C4ValueProviderSinV(C4Object* object, C4Real begin, C4Real end, C4Real offset);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
	C4Real Begin;
	C4Real End;
	C4Real Offset;
};

class C4ValueProviderAction: public StdMeshInstance::SerializableValueProvider
{
public:
	C4ValueProviderAction(): Object(nullptr) {}
	C4ValueProviderAction(C4Object* object);
	bool Execute() override;

	void CompileFunc(StdCompiler* pComp) override;
	void DenumeratePointers() override { Object.DenumeratePointers(); }
	void ClearPointers(C4Object* pObj) override { if(Object == pObj) Object = nullptr; }
private:
	C4ObjectPtr Object;
};

// Reference another value (which is convertible to C4Real), and optionally scale it
template<typename SourceT>
class C4ValueProviderRef: public StdMeshInstance::ValueProvider
{
public:
	C4ValueProviderRef(const SourceT& ref, C4Real scale):
			Ref(ref), Scale(scale) {}

	bool Execute() override
	{
		Value = Scale * Ref;
		return true;
	}

private:
	const SourceT& Ref;
	C4Real Scale;
};

StdMeshInstance::ValueProvider* CreateValueProviderFromArray(C4Object* pForObj, C4ValueArray& Data, const StdMeshAnimation* pos_for_animation = nullptr);

#endif
