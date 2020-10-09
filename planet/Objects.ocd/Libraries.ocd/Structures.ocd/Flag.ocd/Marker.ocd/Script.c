/**
	Flag Library: Marker
	A small marker object which indicates the ownership area of a flag. 
	This is part of the flag library.
	
	@author Zapper, Maikel
*/


// Keep track of the fade.
local fade;

// The marker should not be affect by wind and other forces.
public func IsEnvironment() { return true; }

protected func Initialize()
{
	fade = 0;
	// Make the marker float.
	SetAction("Float");
	SetComDir(COMD_None);
	// Reset the color.
	ResetColor();
	return;
}

public func ResetColor()
{
	// Set color and alpha.
	SetClrModulation(GetPlayerColor(GetOwner()));
	SetObjAlpha(fade);
	return;
}


/*-- Movement --*/

public func MoveTo(int x, int y, int r)
{
	if (GetEffect("MoveTo", this)) 
		RemoveEffect("MoveTo", this);
	AddEffect("MoveTo", this, 1, 1, this, nil, x, y, r);
	return;
}

protected func FxMoveToStart(object target, proplist effect, int temp, int x, int y, int r)
{
	if (temp)
		return FX_OK;
	
	effect.x = x;
	effect.y = y;
	effect.r = r;
	
	// Determine distance and x and y stepping.
	effect.distance = Distance(GetX(), GetY(), x, y);
	effect.x_step = (effect.x - GetX());
	effect.y_step = (effect.y - GetY());
	SetXDir(effect.x_step, 100);
	SetYDir(effect.y_step, 100);
	
	// Determine rotational distance and r stepping.	
	var r_diff = GetTurnDirection(GetR(), r);
	if (r_diff)
	{
		effect.r_step = Max(1, Abs(effect.distance / r_diff));
		effect.r_step *= BoundBy(r_diff, -1, 1);
	} 
	else 
		effect.r_step = 0;

	return FX_OK;
}

protected func FxMoveToTimer(object target, proplist effect, int time)
{
	if ((Abs(GetX() - effect.x) < 2) && (Abs(GetY() - effect.y) < 2))
	{
		SetPosition(effect.x, effect.y);
		SetR(effect.r);
		SetSpeed(0, 0);
		return -1;
	} 
	if (Abs(GetR() - effect.r) >= 2) 
		SetR(GetR() + effect.r_step);	
	return FX_OK;
}

// Flagpoles do not work across landscape borders, therefore remove
// the marker when it crosses a border as if it would move out of
// the landscape. The border rule moves objects across the border,
// so remove in the callback from this rule.
public func OnBorderCrossed()
{
	return RemoveObject();
}


/*-- Fading --*/

public func FadeIn()
{
	if (GetEffect("Fade*", this)) 
		RemoveEffect("Fade*", this);
	AddEffect("FadeIn", this, 1, 1, this);
	return;
}

public func FadeOut()
{
	if (GetEffect("Fade*", this))
		RemoveEffect("Fade*", this);
	AddEffect("FadeOut", this, 1, 1, this);
	return;
}

protected func FxFadeInTimer(object target, proplist effect, int time)
{
	if (fade == 255) 
		return -1;
	fade = BoundBy(fade + 3, 0, 255);
	SetObjAlpha(fade);
	return FX_OK;
}

protected func FxFadeOutTimer(object target, proplist effect, int time)
{
	if (fade == 0) 
		return -1;
	fade = BoundBy(fade - 3, 0, 255);
	SetObjAlpha(fade);
	return FX_OK;
}


/*-- Saving --*/

// The UI is not saved.
public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
	Float = {
		Prototype = Action,
		Name = "Float",
		Procedure = DFA_FLOAT,
		Directions = 1,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
	}
};

local Plane = 1545;
