/*
	Crate
	Used for deliveries.
	
	@author: Ringwaul
*/

#include Library_CarryHeavy

local crateanim;

/*-- Engine Callbacks --*/

func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20, 80),0, 1, 0));
	return _inherited(...);
}

func Hit()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
}

func RejectCollect(id def, object obj)
{
	if (ContentsCount() >= MaxContentsCount)
		return true;
	if (obj->~IsCarryHeavy())
		return true;
	return false;
}

/*-- Interface --*/

public func Open()
{
	PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold));
	Sound("Structures::Chest::Open");
}

public func Close()
{
	crateanim = PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold));
	Sound("Structures::Chest::Close");
}

public func IsContainer() { return true; }

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode()
{
	return CARRY_BothHands;
}

public func GetCarryPhase() { return 800; }

public func GetCarryTransform(object clonk)
{
	if (GetCarrySpecial(clonk))
		return Trans_Translate(3500, 6500, 0);
	
	return Trans_Translate(0, 0, -1500);
}

func Definition(def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-500, -1500, -3000), Trans_Rotate(-30, 1, 0, 0), Trans_Rotate(30, 0, 1, 0));
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ContainBlast = true;
local Components = {Wood = 3};
local MaxContentsCount = 5;