//Airship Graphic

local propanim;
local parent;

protected func Initialize()
{
	propanim = PlayAnimation("Flight", 5, Anim_Const(0), Anim_Const(1000));
}

//Moves the propeller 1 tick per call
func AnimationForward()
{
	var i = 50;
	//Loop animation
	if(GetAnimationPosition(propanim) + i > GetAnimationLength("Flight"))
	{
		SetAnimationPosition(propanim, Anim_Const(GetAnimationPosition(propanim) + i - GetAnimationLength("Flight")));
		return 1;
	}

	//advance animation
	else
	{
		SetAnimationPosition(propanim, Anim_Const(GetAnimationPosition(propanim) + i));
		return 1;
	}
	//SoundEffect?
}

public func SetAirshipParent(object airship)
{
	parent = airship;
}

local ActMap = {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 64,
			Hgt = 54,
			NextAction = "Attach",
		},
};
