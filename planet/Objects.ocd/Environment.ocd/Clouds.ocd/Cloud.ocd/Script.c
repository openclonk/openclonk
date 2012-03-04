/**
	Cloud
	Generic cloud, features: rain (water, acid) and thunder.
	The clouds have periods of condensing, idle and raining.
	Different types of rain (water, acid) are hardcoded.
	
	TODO: Make dependent on scenario setting.
	TODO: Make for all material types.

	@authors Ringwaul, Maikel
*/


// Cloud modes: idle, raining, condensing.
static const CLOUD_ModeIdle = 0;
static const CLOUD_ModeRaining = 1;
static const CLOUD_ModeCondensing = 2;
local mode;
// The time a cloud is in this mode.
local mode_time;

local water; // number of water pixels the cloud contains.
local acid; // number of acid pixels the cloud contains.
local lightning_chance; // chance of lightning strikes 0-100.
local evap_x; // x coordinate for evaporation


protected func Initialize()
{
	// Clouds start idle.
	mode = CLOUD_ModeIdle;
	mode_time = 360 + RandomX(-60, 60);
	
	// Default values for rain.
	water = RandomX(200, 300);
	acid = 0;
	
	// Cloud defaults
	lightning_chance = 0;
	evap_x = 0;

	DoCon(Random(75));

	SetAction("Fly");
	SetComDir(COMD_None);
	SetPhase(RandomX(1,16));

	//Push low flying clouds up to proper height
	while(MaterialDepthCheck(GetX(),GetY(),"Sky",150)!=true)
	{
		SetPosition(GetX(),GetY()-1);
	}

	//Failsafe for stupid grounded clouds
	if(GetMaterial(0,30)!=Material("Sky")) SetPosition(GetX(), GetY()-180);
	
	// Add effect to process all cloud features.
	AddEffect("ProcessCloud", this, 100, 5, this);
	return;
}

protected func FxProcessCloudStart(object target, proplist effect, int temporary)
{
	// Make sure the effect interval is 5.
	if (!temporary)
		effect.Interval = 5;
	return 1;
}

protected func FxProcessCloudTimer()
{
	// Move clouds.
	MoveCloud();
	// Update mode time.
	mode_time--;
	if (mode_time <= 0)
	{
		// Change mode, reset timer.
		mode = (mode + 1) % 3;
		mode_time = 360 + RandomX(-60, 60);
	}
	// Process modes.
	if (mode == CLOUD_ModeIdle)
	{
		/* Empty */
	}
	else if (mode == CLOUD_ModeRaining)
	{	
		Precipitation();
		ThunderStrike();
	}
	else if (mode == CLOUD_ModeCondensing)
	{
		Evaporation();	
	}
	// Update cloud appearance.	
	ShadeCloud();
	
	return 1;
}

private func MoveCloud()
{
	// Move according to wind.
	var wind = GetWind();
	if (wind >= 7)
		SetXDir(Random(355), 1000);
	else if (wind <= -7)
		SetXDir(-Random(355), 1000);
	else
		SetXDir();
	// Loop clouds around the map.
	if (GetX() >= LandscapeWidth() - 10) 
		SetPosition(12, GetY());
	if (GetX() <= 10) 
		SetPosition(LandscapeWidth()-12, GetY());
	// Some other safety.
	if (GetY() <= 5) 
		SetPosition(0, 6);
	if (GetYDir()!=0) 
		SetYDir(0);
	while (Stuck()) 
		SetPosition(GetX(), GetY() - 5);
	return;
}

private func Precipitation()
{
	// Precipitaion: water or snow.
	if (water > 0)
	{
		if (GetTemperature() > 0)
			RainDrop("Water");
		else
			RainDrop("Snow");
		water--;	
	}
	
	// Precipitation: acid.
	if (acid > 0)
	{
		RainDrop("Acid");
		acid--;	
	}
	// If out of liquids, skip mode.
	if (water == 0 && acid == 0)
		mode_time = 0;
		
	return;
}

// Raindrop somewhere from the cloud.
private func RainDrop(string mat)
{
	var angle = RandomX(0, 359);
	var dist = Random(51);
	CastPXS(mat, 1, 1, Sin(angle,dist),Cos(angle,dist));
}

// Launches possibly one thunder strike from the cloud.
private func ThunderStrike()
{
	// Determine whether to launch a strike.
	if (water < 100)
		return;
	if (Random(100) >= lightning_chance || Random(5))
		return;
	
	// Find random position in the cloud.
	var con = GetCon();
	var wdt = GetDefWidth() * con / 250;
	var hgt = GetDefHeight() * con / 350;
	var x = GetX() + RandomX(-wdt, wdt);
	var y = GetY() + RandomX(-hgt, hgt);
	var str = con + RandomX(-20, 20);
	// Launch lightning.
	return LaunchLightning(x, y, str, 0, str / 5, str / 10, str / 10, true);
}

// Tries to evaporate liquids from the surface.
protected func Evaporation() 
{
	var prec = 5;
	
	// Found enough water/acid, skip condensing phase.
	if (water >= 700 || acid >= 100)
	{
		mode_time = 0;
		return;
	}
	
	// determine new x coordinate from the cloud to test for liquids.
	var wdt = GetDefWidth() * GetCon() * 3 / 4;
	evap_x += prec;
	if (evap_x > wdt)
		evap_x = - wdt;
	
	// Test the line downwards from this coordinate.
	var y = 0;
	while (!GBackSemiSolid(evap_x, y) && y < LandscapeHeight())
		y += prec;
	
	// Try to extract water.
	if(GetMaterial(evap_x, y) == Material("Water"))
	{
		ExtractMaterialAmount(evap_x, y, Material("Water"), 3);
		water += 3;
	}
	
	// Try to extract acid.
	if(GetMaterial(evap_x, y) == Material("Acid"))
	{
		ExtractMaterialAmount(evap_x, y, Material("Acid"), 3);
		acid += 3;
	}
	
	// Also add some rain by scenario value.
	var mat = GetScenarioVal("Precipitation");
	if (Random(100) < GetScenarioVal("Rain"))
	{
		if (mat == "Water")
			water++;
		if (mat == "Acid")
			acid++;	
	}
		
	return;
}

//Shades the clouds based on iSize: the water density value of the cloud.
private func ShadeCloud()
{
	var shade = Min(water*425/1000, 255);
	var shade2 = Min(water-600, 255);
	var shade3 = (acid*255/100)/2;

	if (water <= 600) 
		SetObjAlpha(shade);
	if (water > 600) 
		SetClrModulation(RGBa(255-shade2, 255-shade2, 255-shade2, 255));
	if (acid > 0)
		SetClrModulation(RGBa(255-shade3, 255, 255-shade3, 255-shade));
	return;
}

//For use as scenario setting. Can work after initialize, if you really want to.
global func AdjustLightningFrequency(int freq)
{
	for (var cloud in FindObjects(Find_ID(Cloud)))
		cloud->SetLightningFrequency(freq);
	return;
}

//Routes the global adjust function's variable to the clouds.
public func SetLightningFrequency(int freq)
{
	lightning_chance = freq;
}

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Speed = 20,
		Accel = 16,
		Decel = 16,
		X = 0,
		Y = 0,
		Wdt = 512,
		Hgt = 350,
		Length = 16,
		Delay = 0,
		NextAction = "Fly",
		TurnAction = "Turn",
	},
};
local Name = "Cloud";
