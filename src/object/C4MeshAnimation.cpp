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

#include "C4Include.h"
#include "script/C4Aul.h"
#include "object/C4MeshAnimation.h"
#include "object/C4Object.h"
#include "script/C4ValueArray.h"
#include "game/C4Game.h"

namespace
{
	// Register value providers for serialization
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderConst> C4ValueProviderConstID("const");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderLinear> C4ValueProviderLinearID("linear");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderX> C4ValueProviderXID("x"); 
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderY> C4ValueProviderYID("y");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderR> C4ValueProviderRID("r");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAbsX> C4ValueProviderAbsXID("absx");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAbsY> C4ValueProviderAbsYID("absy");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderDist> C4ValueProviderDistID("dist");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderXDir> C4ValueProviderXDirID("xdir"); 
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderYDir> C4ValueProviderYDirID("ydir"); 
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderRDir> C4ValueProviderRDirID("rdir");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAbsRDir> C4ValueProviderAbsRDirID("absrdir");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderCosR> C4ValueProviderCosRID("cosr");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderSinR> C4ValueProviderSinRID("sinr");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderCosV> C4ValueProviderCosVID("cosv");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderSinV> C4ValueProviderSinVID("sinv");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAction> C4ValueProviderActionID("action");
}

StdMeshInstance::ValueProvider* CreateValueProviderFromArray(C4Object* pForObj, C4ValueArray& Data, const StdMeshAnimation* pos_for_animation)
{
	int32_t type = Data[0].getInt();
	switch (type)
	{
	case C4AVP_Const:
		return new C4ValueProviderConst(itofix(Data[1].getInt(), 1000));
	case C4AVP_Linear:
	{
		int32_t end = Data[3].getInt(), len = Data[4].getInt();
		if (len == 0)
			throw C4AulExecError("Length cannot be zero");
		// Sanity check for linear animations that are too long and could cause excessive animation stacks
		if (pos_for_animation)
		{
			int32_t max_end = fixtoi(ftofix(pos_for_animation->Length), 1000);
			if (end < 0 || end > max_end)
				throw C4AulExecError(FormatString("End (%d) not in range of animation '%s' (0-%d).", (int)end, pos_for_animation->Name.getData(), (int)max_end).getData());
		}
		return new C4ValueProviderLinear(itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(end, 1000), len, static_cast<C4AnimationEnding>(Data[5].getInt()));
	}
	case C4AVP_X:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() == 0)
			throw C4AulExecError("Length cannot be zero");

		return new C4ValueProviderX(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_Y:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() == 0)
			throw C4AulExecError("Length cannot be zero");

		return new C4ValueProviderY(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_R:
		if(Data.GetSize() >= 4 && Data[3] != C4VNull)
			pForObj = Data[3].getObj();
		if (!pForObj) return nullptr;
		return new C4ValueProviderR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000));
	case C4AVP_AbsX:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() == 0)
			throw C4AulExecError("Length cannot be zero");
		return new C4ValueProviderAbsX(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_AbsY:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() == 0)
			throw C4AulExecError("Length cannot be zero");
		return new C4ValueProviderAbsY(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_Dist:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() == 0)
			throw C4AulExecError("Length cannot be zero");
		return new C4ValueProviderDist(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_XDir:
		if (!pForObj) return nullptr;
		if (Data[3].getInt() == 0)
			throw C4AulExecError("MaxXDir cannot be zero");
		return new C4ValueProviderXDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_YDir:
		if (!pForObj) return nullptr;
		if (Data[3].getInt() == 0)
			throw C4AulExecError("MaxYDir cannot be zero");
		return new C4ValueProviderYDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_RDir:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() - Data[3].getInt() == 0)
			throw C4AulExecError("MaxRDir - MinRDir cannot be zero");
		return new C4ValueProviderRDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[5].getInt()), itofix(Data[4].getInt(),Data[5].getInt()));

	case C4AVP_AbsRDir:
		if (!pForObj) return nullptr;
		if (Data[4].getInt() - Data[3].getInt() == 0)
			throw C4AulExecError("MaxRDir - MinRDir cannot be zero");
		return new C4ValueProviderAbsRDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[5].getInt()), itofix(Data[4].getInt(),Data[5].getInt()));
	case C4AVP_CosR:
		if (!pForObj) return nullptr;
		return new C4ValueProviderCosR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_SinR:
		if (!pForObj) return nullptr;
		return new C4ValueProviderSinR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_CosV:
		if (!pForObj) return nullptr;
		return new C4ValueProviderCosV(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_SinV:
		if (!pForObj) return nullptr;
		return new C4ValueProviderSinV(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_Action:
		if (!pForObj) return nullptr;
		return new C4ValueProviderAction(pForObj);
	default:
		return nullptr;
	}
}

C4ValueProviderConst::C4ValueProviderConst(C4Real value)
{
	Value = value;
}

bool C4ValueProviderConst::Execute()
{
	// Keep value we set in ctor
	return true;
}

C4ValueProviderLinear::C4ValueProviderLinear(C4Real pos, C4Real begin, C4Real end, int32_t length, C4AnimationEnding ending):
		Begin(begin), End(end), Length(length), Ending(ending), LastTick(Game.FrameCounter)
{
	Value = pos;
}

bool C4ValueProviderLinear::Execute()
{
	Value += (End - Begin) * itofix(Game.FrameCounter - LastTick) / Length;
	LastTick = Game.FrameCounter;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while ( (End > Begin && Value > End) || (End < Begin && Value < End))
	{
		switch (Ending)
		{
		case ANIM_Loop:
			Value -= (End - Begin);
			return true;
		case ANIM_Hold:
			Value = End;
			return true;
		case ANIM_Remove:
			Value = End;
			return false;
		}
	}

	return true;
}

void C4ValueProviderLinear::CompileFunc(StdCompiler* pComp)
{
	const StdEnumEntry<C4AnimationEnding> Endings[] =
	{
		{ "Loop",   ANIM_Loop                          },
		{ "Hold",   ANIM_Hold                          },
		{ "Remove", ANIM_Remove                        },

		{ nullptr,     static_cast<C4AnimationEnding>(0)  }
	};

	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Length);
	pComp->Separator();
	pComp->Value(mkEnumAdaptT<uint8_t>(Ending, Endings));
	pComp->Separator();
	pComp->Value(LastTick);

	// When a scenario is saved as scenario the FrameCounter will be reset
	// upon scenario start. The LastTick variable is fixed here.
	// TODO: A nicer solution would be to always set LastTick to
	// Game.FrameCounter and to make sure that the Value is always up to
	// date (current frame) when saving by running Execute(). This could
	// even be done in the base class.
	if(pComp->isDeserializer())
		if(LastTick > Game.FrameCounter)
			LastTick = 0;
}

C4ValueProviderX::C4ValueProviderX(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length)
{
	Value = pos;
}

bool C4ValueProviderX::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value += (End - Begin) * (Object->xdir) / Length;

	if (End > Begin)
	{
		while (Value > End)
			Value -= (End - Begin);
		while (Value < Begin)
			Value += (End - Begin);
	}
	else
	{
		while (Value > Begin)
			Value -= (Begin - End);
		while (Value < End)
			Value += (Begin - End);
	}

	return true;
}

void C4ValueProviderX::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Length);
}

C4ValueProviderY::C4ValueProviderY(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length)
{
	Value = pos;
}

bool C4ValueProviderY::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value += (End - Begin) * (Object->ydir) / Length;

	if (End > Begin)
	{
		while (Value > End)
			Value -= (End - Begin);
		while (Value < Begin)
			Value += (End - Begin);
	}
	else
	{
		while (Value > Begin)
			Value -= (Begin - End);
		while (Value < End)
			Value += (Begin - End);
	}

	return true;
}

void C4ValueProviderY::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Length);
}

C4ValueProviderR::C4ValueProviderR(C4Object* object, C4Real begin, C4Real end):
		Object(object), Begin(begin), End(end)
{
	Execute();
}

bool C4ValueProviderR::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	C4Real r = Object->fix_r;
	if(r < 0) r += 360;

	Value = Begin + (End - Begin) * r / 360;
	return true;
}

void C4ValueProviderR::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
}

C4ValueProviderAbsX::C4ValueProviderAbsX(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length)
{
	Value = pos;
}

bool C4ValueProviderAbsX::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	C4Real dist;
	if(Object->xdir > itofix(100) || Object->ydir > itofix(100))
		dist = itofix(Distance(0, 0, fixtoi(Object->xdir), fixtoi(Object->ydir)));
	else if(Object->xdir > itofix(1) || Object->ydir > itofix(1))
		dist = itofix(Distance(0, 0, fixtoi(Object->xdir, 256), fixtoi(Object->ydir, 256)), 16);
	else
		dist = itofix(Distance(0, 0, fixtoi(Object->xdir, 16384), fixtoi(Object->ydir, 16384)), 128);

	Value += (End - Begin) * Abs(Object->xdir) / Length;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while ( (End > Begin && Value > End) || (End < Begin && Value < End))
		Value -= (End - Begin);

	return true;
}

void C4ValueProviderAbsX::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Length);
}

C4ValueProviderAbsY::C4ValueProviderAbsY(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length)
{
	Value = pos;
}

bool C4ValueProviderAbsY::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value += (End - Begin) * Abs(Object->ydir) / Length;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while ( (End > Begin && Value > End) || (End < Begin && Value < End))
		Value -= (End - Begin);

	return true;
}

void C4ValueProviderAbsY::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Length);
}

C4ValueProviderDist::C4ValueProviderDist(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length)
{
	Value = pos;
}

bool C4ValueProviderDist::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	// The following computes sqrt(xdir**2 + ydir**2), and it attempts to
	// do so without involving floating point numbers, and at the same
	// time cover a large range of xdir and ydir.
	C4Real dist;
	if(Object->xdir > itofix(256) || Object->ydir > itofix(256))
		dist = itofix(Distance(0, 0, fixtoi(Object->xdir), fixtoi(Object->ydir)));
	else if(Object->xdir > itofix(1) || Object->ydir > itofix(1))
		dist = itofix(Distance(0, 0, fixtoi(Object->xdir, 256), fixtoi(Object->ydir, 256)), 256);
	else
		dist = itofix(Distance(0, 0, fixtoi(Object->xdir, 65536), fixtoi(Object->ydir, 65536)), 65536);

	Value += (End - Begin) * dist / Length;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while ( (End > Begin && Value > End) || (End < Begin && Value < End))
		Value -= (End - Begin);

	return true;
}

void C4ValueProviderDist::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Length);
}

C4ValueProviderXDir::C4ValueProviderXDir(C4Object* object, C4Real begin, C4Real end, C4Real max_xdir):
		Object(object), Begin(begin), End(end), MaxXDir(max_xdir)
{
	Execute();
}

bool C4ValueProviderXDir::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value = Begin + (End - Begin) * std::min<C4Real>(Abs(Object->xdir/MaxXDir), itofix(1));
	return true;
}

void C4ValueProviderXDir::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(MaxXDir);
}

C4ValueProviderYDir::C4ValueProviderYDir(C4Object* object, C4Real begin, C4Real end, C4Real max_ydir):
		Object(object), Begin(begin), End(end), MaxYDir(max_ydir)
{
	Execute();
}

bool C4ValueProviderYDir::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value = Begin + (End - Begin) * std::min<C4Real>(Abs(Object->ydir/MaxYDir), itofix(1));
	return true;
}

void C4ValueProviderYDir::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(MaxYDir);
}

C4ValueProviderRDir::C4ValueProviderRDir(C4Object* object, C4Real begin, C4Real end, C4Real min_rdir, C4Real max_rdir):
		Object(object), Begin(begin), End(end), MinRDir(min_rdir), MaxRDir(max_rdir)
{
	Execute();
}

bool C4ValueProviderRDir::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	C4Real val = (Object->rdir - MinRDir) / (MaxRDir - MinRDir);

	Value = Begin + (End - Begin) * Clamp<C4Real>(val, itofix(0), itofix(1));
	return true;
}

void C4ValueProviderRDir::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(MaxRDir);
}

C4ValueProviderAbsRDir::C4ValueProviderAbsRDir(C4Object* object, C4Real begin, C4Real end, C4Real min_rdir, C4Real max_rdir):
		Object(object), Begin(begin), End(end), MinRDir(min_rdir), MaxRDir(max_rdir)
{
	Execute();
}

bool C4ValueProviderAbsRDir::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	C4Real val = (Abs(Object->rdir) - MinRDir) / (MaxRDir - MinRDir);

	Value = Begin + (End - Begin) * Clamp<C4Real>(val, itofix(0), itofix(1));
	return true;
}

void C4ValueProviderAbsRDir::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(MaxRDir);
}

C4ValueProviderCosR::C4ValueProviderCosR(C4Object* object, C4Real begin, C4Real end, C4Real offset):
		Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderCosR::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value = Begin + (End - Begin) * Cos(Object->fix_r + Offset);
	return true;
}

void C4ValueProviderCosR::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Offset);
}

C4ValueProviderSinR::C4ValueProviderSinR(C4Object* object, C4Real begin, C4Real end, C4Real offset):
		Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderSinR::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	Value = Begin + (End - Begin) * Sin(Object->fix_r + Offset);
	return true;
}

void C4ValueProviderSinR::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Offset);
}

C4ValueProviderCosV::C4ValueProviderCosV(C4Object* object, C4Real begin, C4Real end, C4Real offset):
		Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderCosV::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	// TODO: Maybe we can optimize this by using cos(r) = x/sqrt(x*x+y*y), sin(r)=y/sqrt(x*x+y*y)
	// plus addition theorems for sin or cos.

	int angle = Angle(0, 0, fixtoi(Object->xdir, 256), fixtoi(Object->ydir, 256));
	Value = Begin + (End - Begin) * Cos(itofix(angle) + Offset);
	return true;
}

void C4ValueProviderCosV::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Offset);
}

C4ValueProviderSinV::C4ValueProviderSinV(C4Object* object, C4Real begin, C4Real end, C4Real offset):
		Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderSinV::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	// TODO: Maybe we can optimize this by using cos(r) = x/sqrt(x*x+y*y), sin(r)=y/sqrt(x*x+y*y),
	// plus addition theorems for sin or cos.

	int angle = Angle(0, 0, fixtoi(Object->xdir, 256), fixtoi(Object->ydir, 256));
	Value = Begin + (End - Begin) * Sin(itofix(angle) + Offset);
	return true;
}

void C4ValueProviderSinV::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
	pComp->Separator();
	pComp->Value(Begin);
	pComp->Separator();
	pComp->Value(End);
	pComp->Separator();
	pComp->Value(Offset);
}

C4ValueProviderAction::C4ValueProviderAction(C4Object* object):
		Object(object)
{
}

bool C4ValueProviderAction::Execute()
{
	// Object might have been removed
	if(!Object) return false;

	const C4Action& Action = Object->Action;
	C4PropList* pActionDef = Object->GetAction();

	// TODO: We could cache these...
	const StdMeshAnimation* animation = Action.Animation->GetAnimation();
	const int32_t length = pActionDef->GetPropertyInt(P_Length);
	const int32_t delay = pActionDef->GetPropertyInt(P_Delay);

	if (delay)
		Value = itofix(Action.Phase * delay + Action.PhaseDelay) * ftofix(animation->Length) / (delay * length);
	else
		Value = itofix(Action.Phase) * ftofix(animation->Length) / length;

	return true;
}

void C4ValueProviderAction::CompileFunc(StdCompiler* pComp)
{
	SerializableValueProvider::CompileFunc(pComp);
	pComp->Separator();
	pComp->Value(Object);
}
