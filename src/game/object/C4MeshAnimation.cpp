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

StdMeshInstance::ValueProvider* CreateValueProviderFromArray(C4Object* pForObj, C4ValueArray& Data)
{
	int32_t type = Data[0].getInt();
	switch(type)
	{
	case C4AVP_Const:
		return new C4ValueProviderConst(itofix(Data[1].getInt(), 1000));
	case C4AVP_Linear:
		return new C4ValueProviderLinear(itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt(), static_cast<C4AnimationEnding>(Data[5].getInt()));
	case C4AVP_X:
		if(!pForObj) return NULL;
		return new C4ValueProviderX(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_Y:
		if(!pForObj) return NULL;
		return new C4ValueProviderY(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_AbsX:
		if(!pForObj) return NULL;
		return new C4ValueProviderAbsX(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_AbsY:
		if(!pForObj) return NULL;
		return new C4ValueProviderAbsY(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(), 1000), Data[4].getInt());
	case C4AVP_XDir:
		if(!pForObj) return NULL;
		return new C4ValueProviderXDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_YDir:
		if(!pForObj) return NULL;
		return new C4ValueProviderYDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_RDir:
		if(!pForObj) return NULL;
		return new C4ValueProviderRDir(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_CosR:
		if(!pForObj) return NULL;
		return new C4ValueProviderCosR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_SinR:
		if(!pForObj) return NULL;
		return new C4ValueProviderSinR(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_CosV:
		if(!pForObj) return NULL;
		return new C4ValueProviderCosV(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_SinV:
		if(!pForObj) return NULL;
		return new C4ValueProviderSinV(pForObj, itofix(Data[1].getInt(), 1000), itofix(Data[2].getInt(), 1000), itofix(Data[3].getInt(),Data[4].getInt()));
	case C4AVP_Action:
		if(!pForObj) return NULL;
		return new C4ValueProviderAction(pForObj);
	default:
		return NULL;
	}
}

C4ValueProviderConst::C4ValueProviderConst(FIXED value)
{
	Value = value;
}

bool C4ValueProviderConst::Execute()
{
	// Keep value we set in ctor
	return true;
}

C4ValueProviderLinear::C4ValueProviderLinear(FIXED pos, FIXED begin, FIXED end, int32_t length, C4AnimationEnding ending):
	Begin(begin), End(end), Length(length), Ending(ending), LastTick(Game.FrameCounter)
{
	Value = pos;
}

bool C4ValueProviderLinear::Execute()
{
	Value += (End - Begin) * itofix(Game.FrameCounter - LastTick) / Length;
	LastTick = Game.FrameCounter;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while( (End > Begin && Value > End) || (End < Begin && Value < End))
	{
		switch(Ending)
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

C4ValueProviderX::C4ValueProviderX(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length):
	Object(object), Begin(begin), End(end), Length(length), LastX(object->fix_x)
{
	Value = pos;
}

bool C4ValueProviderX::Execute()
{
	//const FIXED obj_x = fixtof(Object->fix_x);
	Value += (End - Begin) * (Object->fix_x - LastX) / Length; // TODO: Use xdir instead?
	LastX = Object->fix_x;

	if(End > Begin)
	{
		while(Value > End)
			Value -= (End - Begin);
		while(Value < Begin)
			Value += (End - Begin);
	}
	else
	{
		while(Value > Begin)
			Value -= (Begin - End);
		while(Value < End)
			Value += (Begin - End);
	}

	return true;
}

C4ValueProviderY::C4ValueProviderY(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length):
	Object(object), Begin(begin), End(end), Length(length), LastY(object->fix_y)
{
	Value = pos;
}

bool C4ValueProviderY::Execute()
{
	//const FIXED obj_y = fixtof(Object->fix_y);
	Value += (End - Begin) * (Object->fix_y - LastY) / Length; // TODO: Use ydir instead?
	LastY = Object->fix_y;

	if(End > Begin)
	{
		while(Value > End)
			Value -= (End - Begin);
		while(Value < Begin)
			Value += (End - Begin);
	}
	else
	{
		while(Value > Begin)
			Value -= (Begin - End);
		while(Value < End)
			Value += (Begin - End);
	}

	return true;
}

C4ValueProviderAbsX::C4ValueProviderAbsX(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length):
	Object(object), Begin(begin), End(end), Length(length), LastX(object->fix_x)
{
	Value = pos;
}

bool C4ValueProviderAbsX::Execute()
{
	Value += (End - Begin) * Abs(Object->fix_x - LastX) / Length;
	LastX = Object->fix_x;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while( (End > Begin && Value > End) || (End < Begin && Value < End))
		Value -= (End - Begin);

	return true;
}

C4ValueProviderAbsY::C4ValueProviderAbsY(const C4Object* object, FIXED pos, FIXED begin, FIXED end, int32_t length):
	Object(object), Begin(begin), End(end), Length(length), LastY(object->fix_y)
{
	Value = pos;
}

bool C4ValueProviderAbsY::Execute()
{
	Value += (End - Begin) * Abs(Object->fix_y - LastY) / Length;
	LastY = Object->fix_y;

	assert( (End >= Begin && Value >= Begin) || (End <= Begin && Value <= Begin));
	while( (End > Begin && Value > End) || (End < Begin && Value < End))
		Value -= (End - Begin);

	return true;
}

C4ValueProviderXDir::C4ValueProviderXDir(const C4Object* object, FIXED begin, FIXED end, FIXED max_xdir):
	Object(object), Begin(begin), End(end), MaxXDir(max_xdir)
{
	Execute();
}


bool C4ValueProviderXDir::Execute()
{
	Value = Begin + (End - Begin) * Min<FIXED>(Abs(Object->xdir/MaxXDir), itofix(1));
	return true;
}

C4ValueProviderYDir::C4ValueProviderYDir(const C4Object* object, FIXED begin, FIXED end, FIXED max_ydir):
	Object(object), Begin(begin), End(end), MaxYDir(max_ydir)
{
	Execute();
}

bool C4ValueProviderYDir::Execute()
{
	Value = Begin + (End - Begin) * Min<FIXED>(Abs(Object->ydir/MaxYDir), itofix(1));
	return true;
}

C4ValueProviderRDir::C4ValueProviderRDir(const C4Object* object, FIXED begin, FIXED end, FIXED max_rdir):
	Object(object), Begin(begin), End(end), MaxRDir(max_rdir)
{
	Execute();
}

bool C4ValueProviderRDir::Execute()
{
	Value = Begin + (End - Begin) * Min<FIXED>(Abs(Object->rdir/MaxRDir), itofix(1));
	return true;
}

C4ValueProviderCosR::C4ValueProviderCosR(const C4Object* object, FIXED begin, FIXED end, FIXED offset):
	Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderCosR::Execute()
{
	Value = Begin + (End - Begin) * Cos(Object->fix_r + Offset);
	return true;
}

C4ValueProviderSinR::C4ValueProviderSinR(const C4Object* object, FIXED begin, FIXED end, FIXED offset):
	Object(object), Begin(begin), End(end), Offset(offset)
{
	Execute();
}

bool C4ValueProviderSinR::Execute()
{
	Value = Begin + (End - Begin) * Sin(Object->fix_r + Offset);
	return true;
}

C4ValueProviderCosV::C4ValueProviderCosV(const C4Object* object, FIXED begin, FIXED end, FIXED offset):
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

C4ValueProviderSinV::C4ValueProviderSinV(const C4Object* object, FIXED begin, FIXED end, FIXED offset):
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

C4ValueProviderAction::C4ValueProviderAction(const C4Object* object):
	Action(object->Action)
{
}

bool C4ValueProviderAction::Execute()
{
	// TODO: We could cache these...
	const StdMeshAnimation* animation = Action.Animation->GetAnimation();
	const int32_t length = Action.pActionDef->GetPropertyInt(P_Length);
	const int32_t delay = Action.pActionDef->GetPropertyInt(P_Delay);

	if(delay)
		Value = itofix(Action.Phase * delay + Action.PhaseDelay) / (delay * length) * ftofix(animation->Length);
	else
		Value = itofix(Action.Phase) / length * ftofix(animation->Length);

	return true;
}
