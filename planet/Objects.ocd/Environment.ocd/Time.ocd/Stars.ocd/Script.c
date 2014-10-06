/*-- Stars --*/

protected func Initialize()
{
	var alpha=0;
	if(GetTime()<300 || GetTime()>1140) alpha=255;
	var g = RandomX(1,9);
	if(g > 1) SetGraphics(Format("%d",g));
	SetClrModulation(RGBa(255,255,255,alpha));
	SetObjectBlitMode(GFX_BLIT_Additive);
	var parallax = RandomX(8,12);
	this["Parallaxity"] = [parallax,parallax];
}

public func IsCelestial() { return true; }

// Not stored by itself because it's created by the time environment
// (Also, a million stars in Objects.c would suck)
func SaveScenarioObject() { return false; }

local Name = "Stars";
