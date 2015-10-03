/*
	Wheat
	Author: Clonkonaut

	Easy crop for farming.
*/

#include Library_Plant
#include Library_Crop

local swing_anim;

private func SeedArea() { return 60; }
private func SeedChance() { return 250; }
private func SeedAmount() { return 4; } // small seed area -> don't allow too many plants
private func SeedOffset() { return 20; }

private func Construction()
{
	StartGrowth(this.growth);
	AddTimer("WaterCheck", 70+Random(10));
	AddTimer("WindCheck", 350);
	swing_anim = PlayAnimation("Swing", 1, Anim_Const(0), Anim_Const(1000));
	return _inherited(...);
}

private func Initialize()
{
	SetMeshMaterial("wheat_material_ripe");
	_inherited(...);
}

// Reverts the mesh material to the unripe green
public func Unripe()
{
	SetMeshMaterial("wheat_material");
}

/* Check the wind to adjust the swinging speed of the stalks */

private func WindCheck()
{
	var speed = 200 - Abs(GetWind());
	SetAnimationPosition(swing_anim, Anim_Linear(GetAnimationPosition(swing_anim), 0, GetAnimationLength("Swing"), speed, ANIM_Loop));
}

local Name = "$Name$";
local Description = "$Description$";
local growth = 3;
local degrowth = -6;
local fastgrowth = 9;