/*
	ObjectFade Rule
	Author: Caesar

	Removes unused objects. Like Hazard-Arena.
	
	TODO:
	Use ChangeEffect properly to save calls
*/

local fade_time = 18;

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}

public func DoFadeTime(int to_add)
{
	fade_time += to_add;
	return;
}

protected func Initialize() 
{
	if(ObjectCount(Find_ID(Rule_ObjectFade), Find_Exclude(this))) {
		FindObject(Find_ID(Rule_ObjectFade), Find_Exclude(this))->DoFadeTime(36);
		return RemoveObject();
	}
	AddTimer("Timer");
}

protected func Timer() 
{
	for (var fade in FindObjects(Find_Category(C4D_Object), Find_NoContainer(), Find_Not(Find_OCF(OCF_HitSpeed1)))) 
	{
		if (!CheckFadeConditions(fade)) continue;
		if (GetEffect("IntFadeOut*", fade)) continue;
		AddEffect("IntFadeOutCandidate", fade, 1, 36, this, Rule_ObjectFade);
	}
}

public func FxIntFadeOutCandidateTimer(object target, effect, int time) 
{
	// Re-check condition a few times
	if (!CheckFadeConditions(target)) return FX_Execute_Kill;
	if (time > 300)
	{
		// OK - now fade out
		AddEffect("IntFadeOut", target, 100, 1, this, Rule_ObjectFade);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func FadeOutObject(object target)
{
	// Definition or hooked call: Fade out an object (even if rule is not active)
	return AddEffect("IntFadeOut", target, 100, 1, nil, Rule_ObjectFade);
}

func CheckFadeConditions(object fade)
{
	// Moving objects should not.
	// Allow small movement because engine physics bug sometimes cause objects to wiggle forever
	if (Distance(fade->GetXDir(), fade->GetYDir()) > 4) return false;
	// No InEarth objects
	if (fade->GBackSolid()) return false;
	// No objects that explicitely want to stay
	if (fade->~HasNoFadeOut()) return false;
	// Not if picked up
	if (fade->Contained()) return false;
	// Fade out OK
	return true;
}


public func FxIntFadeOutStart(object target, effect) 
{
	effect.x = target->GetX();
	effect.y = target->GetY();
	effect.color = target->GetClrModulation() & 0x00ffffff; //Safe pure rgb
	effect.alpha = target->GetClrModulation() >> 24 & 255; //Safe alpha
}

public func FxIntFadeOutTimer(object target, effect, int time) 
{
	if (time < fade_time * 2/3) 
		return;

	if (!(target->Contained()) && effect.x == target->GetX() && effect.y == target->GetY())
	{
		if(time >= fade_time) 
		{
			target->RemoveObject();
			return -1;
		}
	}
	else 
	{
		target->SetClrModulation(effect.alpha << 24 | effect.color);
		return -1;
	}

	target->SetClrModulation(((fade_time - time) * effect.alpha / (fade_time/3)) << 24 | effect.color);
	return 1;
}

public func FxIntFadeOutTimerEffect(string new_effect_name) 
{
	if (new_effect_name == "IntFadeOut")
		return -1;
}

local Name = "Object Fade";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
