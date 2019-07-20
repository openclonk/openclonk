/*
	Pocketwatch
	Author: Ringwaul

	Tells Time
*/

local watch_anim;

public func GetCarryMode(clonk) { return CARRY_HandBack; }
public func GetCarryTransform(clonk, sec, back)
{
	if (!sec) return Trans_Mul(Trans_Translate(2800, 200, 0), Trans_Rotate(90, 0, 0, 1), Trans_Rotate(180, 0, 1, 0));
	return Trans_Mul(Trans_Translate(2800, 200, 0), Trans_Rotate(90, 0, 0, 1));
}



protected func Construction()
{
	watch_anim = PlayAnimation("time", 5, Anim_Const(1));
}

public func WatchUpdate()
{
	var time = FindObject(Find_ID(Time));

	if (time)
	{
		var watch_time = ((13305 * (time->GetTime() * 2) / 1000) - 1);
		if (watch_time > GetAnimationLength("time")) watch_time = (watch_time - GetAnimationLength("time"));
		SetAnimationPosition(watch_anim, Anim_Const(watch_time));
	}
	return 1;
}

protected func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(15, 1, 0, 0), Trans_Rotate(5, 0, 1, 0), Trans_Rotate(-5, 0, 0, 1), Trans_Translate(500,-400, 0), Trans_Scale(1350)),def);
}

local Name = "$Name$";
local Collectible = true;