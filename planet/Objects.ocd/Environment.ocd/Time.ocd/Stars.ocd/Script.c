/** 
	Stars
	Stars which are shown in the night sky.	
*/


protected func Initialize()
{
	var g = RandomX(1, 9);
	if (g > 1) 
		SetGraphics(Format("%d", g));
	
	var alpha = 0;
	if (Time->IsNight())
		alpha = 255;
	SetClrModulation(RGBa(255, 255, 255, alpha));
	SetObjectBlitMode(GFX_BLIT_Additive);
	
	var parallax = RandomX(8, 12);
	this.Parallaxity = [parallax, parallax];
	return;
}

// Only appears during the night.
public func IsCelestial() { return true; }

// Not stored by itself because it's created by the time environment.
public func SaveScenarioObject() { return false; }
