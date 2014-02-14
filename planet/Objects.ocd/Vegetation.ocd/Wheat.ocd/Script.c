/*
	Wheat
	Author: Clonkonaut

	Easy crop for farming.
*/

#include Library_Plant
#include Library_Crop

private func SeedArea() { return 50; }
private func SeedChance() {	return 250; }
private func SeedAmount() { return 15; }
private func SeedOffset() { return 10; }

local stalks;

protected func Construction()
{
	StartGrowth(this.growth);
	stalks = [];
	return _inherited(...);
}

protected func Initialize()
{
	if (GetLength(stalks)) return;
	// Create 3-5 stalks
	var num = Random(3);
	for (var i = 0; i < 3+num; i++)
	{
		var x = 12/(3+num) * i - 6;
		var y = GetObjHeight()/2;
		// Search for ground level
		while (GBackSolid(x,y) && y > -13) y--;
		// Skip the stalk
		if (GBackSolid(x,y)) continue;
		while (!GBackSolid(x,y+1) && y < 13) y++;
		// Skip the stalk maybe again!
		if (!GBackSolid(x,y+1)) continue;

		var stalk = CreateObject(WheatStalk, x,y+1, GetOwner());
		stalk->SetCon(GetCon());
		stalk->StartGrowth(this.growth);
		stalk->SetAction("Swing");
		stalk->SetDir(Random(2)*2-1);
		stalk->SetPhase(Random(25));
		stalk->SetMother(this);
		// Be sure that the stalk will adjust its growth in 1 frame
		ScheduleCall(stalk, "AdjustGrowth", 1);
		stalks[GetLength(stalks)] = stalk;
	}
	AddEffect("WaterCheck", this, 2, 70, this);
	AddEffect("WindCheck", this, 3, 350, this);
}

/* Absorb water to grow faster */

protected func FxWaterCheckTimer(object obj, effect)
{
	// Fully grown
	if (GetCon() == 100) return -1;
	// Submerged
	if (InLiquid())
	{
		var degrowth = true;
		// Ignore minimum amount of water if small
		if (GetCon() < 20)
			if (!GBackLiquid(0,-5))
				degrowth = false;
		if (degrowth)
		{
			var grow_effect = GetEffect("IntGrowth", this);
			if (!grow_effect) { grow_effect = StartGrowth(this.degrowth); return; }
			if (grow_effect.growth == this.degrowth) return;
			grow_effect.growth = this.degrowth;
			return;
		}
	}
	// Decrease water amount
	if (effect.water)
		effect.water--;
	// Search for water
	var water = 0;
	for (var i = 0; i < GetObjWidth()+1; i++)
	{
		var y = (GetObjHeight()/2)+1;
		var x = i-(GetObjWidth()/2);
		if (!GBackSolid(x,y)) continue;
		while(GBackSolid(x,y) && y) --y;
		if (!y) continue;
		if (MaterialName(GetMaterial(x,y)) == "Water")
			if (ExtractLiquid(x,y))
				water++;
		if (water == 5) // maximum amount of water extracted in one check
			break;
	}
	// Fasten growth if needed
	effect.water += water;
	if (effect.water)
	{
		var grow_effect = GetEffect("IntGrowth", this);
		if (!grow_effect) { grow_effect = StartGrowth(this.fastgrowth); return; }
		if (grow_effect.growth == this.fastgrowth) return;
		grow_effect.growth = this.fastgrowth;
	}
	else
	{
		var grow_effect = GetEffect("IntGrowth", this);
		if (!grow_effect) { grow_effect = StartGrowth(this.growth); return; }
		if (grow_effect.growth == this.growth) return;
		grow_effect.growth = this.growth;
	}
}

/* Check the wind to adjust the swinging speed of the stalks */

protected func FxWindCheckStart(object obj, effect)
{
	if (Abs(GetWind()) < 25) effect.speed = 0;
	else if (Abs(GetWind()) == 100) effect.speed = 1;
	else effect.speed = 4 - Abs(GetWind())/25;
	for (var stalk in stalks)
		stalk->SetSwingSpeed(effect.speed);
}

protected func FxWindCheckTimer(object obj, effect)
{
	var change = false;
	if (Abs(GetWind()) < 25 && effect.speed != 0)
	{
		effect.speed = 0;
		change = true;
	}
	if (Abs(GetWind()) == 100 && effect.speed != 1)
	{
		effect.speed = 1;
		change = true;
	}
	if (Inside(Abs(GetWind()), 25, 99) && 4 - Abs(GetWind())/25 != effect.speed)
	{
		effect.speed = 4 - Abs(GetWind())/25;
		change = true;
	}
	if (change)
	{
		for (var stalk in stalks)
		{
			if (stalk)
				stalk->SetSwingSpeed(effect.speed);
		}
	}
}

/* Callbacks, engine calls */

// Destroy all stalks!
protected func Destruction()
{
	for (var stalk in stalks)
	{
		if (stalk)
			stalk->RemoveObject();
	}
}

public func IsCrop() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;
local growth = 3;
local degrowth = -6;
local fastgrowth = 9;