/**
	BurningBattery
	A part of a compensator!
*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;
local time;

func Initialize()
{
	SetRDir(RandomX(-200, 200));
	AddTimer("DoSmoke", 2);
}

func Hit()
{
	Explode(20);
}

func DoSmoke()
{
	++time;
	Smoke(RandomX(-5, 5), RandomX(-5, 5), 2);
}