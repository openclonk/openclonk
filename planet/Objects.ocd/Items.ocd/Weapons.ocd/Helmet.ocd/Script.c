/**
	Helmet
	Protective head armor.
	
	@author: pluto, Clonkonaut
*/

#include Library_Wearable

/*-- Engine Callbacks --*/

func Hit()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}

/*-- Usage --*/

public func ControlUse(object clonk)
{
	if (IsWorn())
		TakeOff();
	else
		PutOn(clonk);

	return true;
}

// Helmet effect: 20% less damage
func OnDamage(int damage, int cause, int by_player)
{
	// Do nothing on energy gained
	if (damage > 0) return damage;
	// Doesn't protect against all damage
	if (cause == FX_Call_EngBlast || cause == FX_Call_EngFire || cause == FX_Call_EngAsphyxiation || cause == FX_Call_EngCorrosion)
		return damage;

	return damage - (damage*20/100);
}

/*-- Production --*/

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }

/*-- Display --*/

public func GetWearPlace()
{
	return WEARABLE_Head;
}

public func GetWearBone()
{
	return "Main";
}

public func GetWearTransform()
{
	return Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Translate(0, -300));
}

public func GetCarryMode(object clonk, bool secondary)
{
	if (IsWorn() || display_disabled)
		return CARRY_None;
	return CARRY_BothHands;
}

public func GetCarryPhase(object clonk)
{
	return 550;
}

public func GetCarryBone(object clonk, bool secondary)
{
	return "Main";
}

public func GetCarryTransform(object clonk, bool secondary, bool no_hand, bool on_back)
{
	return Trans_Mul(Trans_Rotate(80, 0, 0, 1), Trans_Rotate(-90, 0, 1), Trans_Rotate(-45, 0, 0, 1), Trans_Translate(-1000, 4000));
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(45, 0, 1), Trans_Rotate(10, 0, 0, 1)),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 1, Metal = 1};