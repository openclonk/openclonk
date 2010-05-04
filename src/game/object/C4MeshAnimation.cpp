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

#include "C4Include.h"
#include "C4MeshAnimation.h"
#include "C4Object.h"
#include "C4ValueList.h"
#include "C4Game.h"

namespace
{
	// Register value providers for serialization
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderConst> C4ValueProviderConstID("const");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderLinear> C4ValueProviderLinearID("linear");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderX> C4ValueProviderXID("x"); 
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderY> C4ValueProviderYID("y");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAbsX> C4ValueProviderAbsXID("absx");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAbsY> C4ValueProviderAbsYID("absy");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderXDir> C4ValueProviderXDirID("xdir"); 
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderYDir> C4ValueProviderYDirID("ydir"); 
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderRDir> C4ValueProviderRDirID("rdir");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderCosR> C4ValueProviderCosRID("cosr");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderSinR> C4ValueProviderSinRID("sinr");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderCosV> C4ValueProviderCosVID("cosv");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderSinV> C4ValueProviderSinVID("sinv");
	const StdMeshInstance::SerializableValueProvider::ID<C4ValueProviderAction> C4ValueProviderActionID("action");
}

StdMeshInstance::ValueProvider* CreateValueProviderFromArray(C4Object* pForObj, C4ValueArray& Data)
{
	int32_t type = Data[0].getInt();
	switch (type)
	{
	case C4AVP_Const:
		return new C4ValueProviderConst(itofix(Data[1].getInt(), 1000));
	case C4AVP_Linear:
		return new C4ValueProviderLinear(itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt(), static_cast<C4AnimationEnding>(Data[5].getInt()));
	case C4AVP_X:
		if (!pForObj) return NULL;
		return new C4ValueProviderX(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_Y:
		if (!pForObj) return NULL;
		return new C4ValueProviderY(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_AbsX:
		if (!pForObj) return NULL;
		return new C4ValueProviderAbsX(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_AbsY:
		if (!pForObj) return NULL;
		return new C4ValueProviderAbsY(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_XDir:
		if (!pForObj) return NULL;
		return new C4ValueProviderXDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_YDir:
		if (!pForObj) return NULL;
		return new C4ValueProviderYDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_RDir:
		if (!pForObj) return NULL;
		return new C4ValueProviderRDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_CosR:
		if (!pForObj) return NULL;
		return new C4ValueProviderCosR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_SinR:
		if (!pForObj) return NULL;
		return new C4ValueProviderSinR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_CosV:
		if (!pForObj) return NULL;
		return new C4ValueProviderCosV(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_SinV:
		if (!pForObj) return NULL;
		return new C4ValueProviderSinV(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_Action:
		if (!pForObj) return NULL;
		return new C4ValueProviderAction(pForObj);
	default:
		return NULL;
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

		{ NULL,     static_cast<C4AnimationEnding>(0)  }
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
}

C4ValueProviderX::C4ValueProviderX(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length), LastX(object->fix_x)
{
	Value = pos;
}

bool C4ValueProviderX::Execute()
{
	//const C4Real obj_x = fixtof(Object->fix_x);
	Value += (End - Begin) * (Object->fix_x - LastX) / Length; // TODO: Use xdir instead?
	LastX = Object->fix_x;

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
	pComp->Separator();
	pComp->Value(LastX);
}

C4ValueProviderY::C4ValueProviderY(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length), LastY(object->fix_y)
{
	Value = pos;
}

bool C4ValueProviderY::Execute()
{
	//const C4Real obj_y = fixtof(Object->fix_y);
	Value += (End - Begin) * (Object->fix_y - LastY) / Length; // TODO: Use ydir instead?
	LastY = Object->fix_y;

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
	pComp->Separator();
	pComp->Value(LastY);
}

C4ValueProviderAbsX::C4ValueProviderAbsX(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length), LastX(object->fix_x)
{
	Value = pos;
}

bool C4ValueProviderAbsX::Execute()
{
	Value += (End - Begin) * Abs(Object->fix_x - LastX) / Length;
	LastX = Object->fix_x;

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
	pComp->Separator();
	pComp->Value(LastX);
}

C4ValueProviderAbsY::C4ValueProviderAbsY(C4Object* object, C4Real pos, C4Real begin, C4Real end, int32_t length):
		Object(object), Begin(begin), End(end), Length(length), LastY(object->fix_y)
{
	Value = pos;
}

bool C4ValueProviderAbsY::Execute()
{
	Value += (End - Begin) * Abs(Object->fix_y - LastY) / Length;
	LastY = Object->fix_y;

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
	pComp->Separator();
	pComp->Value(LastY);
}

C4ValueProviderXDir::C4ValueProviderXDir(C4Object* object, C4Real begin, C4Real end, C4Real max_xdir):
		Object(object), Begin(begin), End(end), MaxXDir(max_xdir)
{
	Execute();
}

bool C4ValueProviderXDir::Execute()
{
	Value = Begin + (End - Begin) * Min<C4Real>(Abs(Object->xdir/MaxXDir), itofix(1));
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
	Value = Begin + (End - Begin) * Min<C4Real>(Abs(Object->ydir/MaxYDir), itofix(1));
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

C4ValueProviderRDir::C4ValueProviderRDir(C4Object* object, C4Real begin, C4Real end, C4Real max_rdir):
		Object(object), Begin(begin), End(end), MaxRDir(max_rdir)
{
	Execute();
}

bool C4ValueProviderRDir::Execute()
{
	Value = Begin + (End - Begin) * Min<C4Real>(Abs(Object->rdir/MaxRDir), itofix(1));
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

C4ValueProviderCosR::C4ValueProviderCosR(C4Object* object, C4Real begin, C4Real end, C4Real offset):
		Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderCosR::Execute()
{
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
