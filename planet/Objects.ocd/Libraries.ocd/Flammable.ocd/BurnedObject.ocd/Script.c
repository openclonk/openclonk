/*-- Burned Object --*/

func Incineration()
{
	CreateEffect(BurnDown, 1, 5); // No need for a 1 frame timer anymore

	// Notify the clonk (if held) that this object is now burning
	if (Contained())
		Contained()->~OnInventoryChange();
}

public func Extinguishing()
{
	// Notify the clonk (if held) that this object is no longer burning
	if (Contained())
		Contained()->~OnInventoryChange();
}

public func BurstIntoAshes()
{
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	
	var r = GetR();
	
	for (var cnt = 0; cnt < 5; ++cnt)
	{
		var distance = 3;
		var x = Sin(r, distance);
		var y = -Cos(r, distance);

		for (var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("Ashes", 1, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
}

local BurnDown = new Effect {
	Timer = func (int time) {
		if (this.Target->GetCon() <= 65)
			this.Target->BurstIntoAshes();
	}
};

func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func GetInventoryIconOverlay() // Display a flame in the inventory bar
{
	if (!OnFire()) return;

	var overlay = 
	{
		Symbol = Icon_Flame
	};
	return overlay;
}

public func IsFuel() { return true; }
public func GetFuelAmount()
{
	return GetCon()/2;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 1;
local ContactIncinerate = 1;
local Plane = 390;
