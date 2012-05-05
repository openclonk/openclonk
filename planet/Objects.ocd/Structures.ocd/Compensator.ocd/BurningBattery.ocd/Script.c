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
	CreateParticle("ExploSmokeFastFade", RandomX(-5,5), RandomX(-5,5), 0, 0, Max(10, 300 - time * 5), RGB(100, 100, 100));
}