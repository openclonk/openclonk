//Hatch Graphic

local hatch_anim;
local parent;

protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(0,-10000), Trans_Rotate(-10,0,1,0), Trans_Rotate(-8,1,0,0)));
}

public func SetHatchParent(object hatch)
{
	parent = hatch;
}

func Anim(string anim_name)
{
	var animstart = 0;
	if(hatch_anim)
	{
		if(GetAnimationPosition(hatch_anim) != GetAnimationLength(anim_name))
		{
			animstart = GetAnimationLength(anim_name) - GetAnimationPosition(hatch_anim);
		}
	}

	StopAnimation(hatch_anim);
	hatch_anim = PlayAnimation(anim_name, 5, Anim_Linear(animstart, 0, GetAnimationLength(anim_name), 14, ANIM_Hold), Anim_Const(1000));
}

local ActMap = {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 26,
			Hgt = 26,
			NextAction = "Attach",
		},
};

local Plane = 100;