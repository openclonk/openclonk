/*
	Crate
	Author: Ringwaul

	Used for deliveries.
*/

#include Library_CarryHeavy

local crateanim;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

public func GetCarryTransform(clonk)
{
	if(GetCarrySpecial(clonk))
		return Trans_Translate(3500, 6500, 0);
	
	return Trans_Translate(0, 0, -1500);
}

protected func Construction()
{
	crateanim = PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20,80),0,1,0));
	return _inherited(...);
}

/*-- Contents --*/

local MaxContentsCount = 5;

protected func RejectCollect(id def, object obj)
{
	if (ContentsCount() >= MaxContentsCount)
		return true;
	if (obj->~IsCarryHeavy())
		return true;
	return false;
}

private func Open()
{
	StopAnimation(crateanim);
	crateanim = PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold), Anim_Const(1000));
	Sound("Structures::Chest::Open");
}

private func Close()
{
	StopAnimation(crateanim);
	crateanim = PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold), Anim_Const(1000));
	Sound("Structures::Chest::Close");
}

protected func Definition(def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-500, -1500, -3000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0));
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }
public func IsContainer() { return true; }

func Hit()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ContainBlast = true;
