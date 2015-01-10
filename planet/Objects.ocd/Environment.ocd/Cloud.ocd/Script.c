/**
	Cloud
	Generic cloud, features: rain (water, acid) and thunder.
	The clouds have periods of condensing, idle and raining.
	Different types of rain (water, acid) are hardcoded.
	
	@authors Ringwaul, Maikel
*/


// Cloud modes: idle, raining, condensing.
static const CLOUD_ModeIdle = 0;
static const CLOUD_ModeRaining = 1;
static const CLOUD_ModeCondensing = 2;
local mode;
// The time a cloud is in this mode.
local mode_time;

local lightning_chance; // chance of lightning strikes 0-100.
local evap_x; // x coordinate for evaporation

local rain; // Number of liquid pixels the cloud holds.
local rain_mat; // Precipitation type from scenario or other. 
local rain_amount; // Precipitation amount from scenario or other.
local rain_max; // Max rain the cloud can hold.

local cloud_shade; // Cloud shade.
local cloud_alpha; // Cloud alpha.

// This is an environment object (e.g., shouldn't be a target for the lift tower)
public func IsEnvironment() { return true; }

protected func Initialize()
{
	// Clouds start idle.
	mode = CLOUD_ModeIdle;
	mode_time = 360 + RandomX(-60, 60);
	
	// Default values for rain.
	rain = 0;
	rain_max = 960;
	
	// Cloud defaults
	lightning_chance = 0;
	cloud_shade = 0;
	cloud_alpha = 255;
	evap_x = 0;

	DoCon(Random(75));

	SetAction("Fly");
	SetComDir(COMD_None);
	SetPhase(RandomX(1,16));

	// Push low flying clouds up to proper height
	while (MaterialDepthCheck(GetX(), GetY(), "Sky", 150) != true)
	{
		SetPosition(GetX(), GetY()-1);
	}

	// Failsafe for stupid grounded clouds
	if (GetMaterial(0, 30) != Material("Sky")) 
		SetPosition(GetX(), GetY() - 180);
	
	// Add effect to process all cloud features.
	AddEffect("ProcessCloud", this, 100, 5, this);
	return;
}

/*-- Definition call interface --*/

// Id call: Creates the indicated number of clouds.
public func Place(int count)
{
	if (this != Cloud)
		return;
	while (count > 0)
	{
		var pos;
		if ((pos = FindPosInMat("Sky", 0, 0, LandscapeWidth(), LandscapeHeight())) && MaterialDepthCheck(pos[0], pos[1], "Sky", 200))
		{
			CreateObjectAbove(Cloud, pos[0], pos[1], NO_OWNER);
			count--;
		}
	}
	return;
}	

// Changes the precipitation type of this cloud.
// Also an id call: Changes all clouds to this settings.
public func SetPrecipitation(string mat, int amount)
{
	// Called to proplist: change all clouds.
	if (this == Cloud)
	{
		for (var cloud in FindObjects(Find_ID(Cloud)))
			cloud->SetPrecipitation(mat, amount);
	}
	else // Otherwise change the clouds precipitation.
	{
		rain_mat = mat;
		rain_amount = amount;
		// Also change rain content.
		rain = BoundBy(amount * rain_max / 100, 0, 960); 
	}
	return;
}

// Changes the lightning frequency type of this cloud.
// Also an id call: Changes all clouds to this settings.
public func SetLightning(int freq)
{
	// Called to proplist: change all clouds.
	if (this == Cloud)
	{
		for (var cloud in FindObjects(Find_ID(Cloud)))
			cloud->SetLightning(freq);
	}
	else // Otherwise change this clouds lightning.
	{
		lightning_chance = freq;	
	}
	return;
}

public func SetRain(int to_rain)
{
	rain = BoundBy(to_rain, 0, rain_max);
	return;
}

/*-- Cloud processing --*/

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
		mode_time = 480 + RandomX(-90, 90);
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
	// Get wind speed from various locations of the cloud.
	var con = GetCon();
	var wdt = GetDefWidth() * con / 250;
	var hgt = GetDefHeight() * con / 350;
	var wind = (GetWind() + GetWind(wdt, hgt) + GetWind(wdt, -hgt) + GetWind(-wdt, -hgt) + GetWind(-wdt, hgt) + GetWind(nil, nil, true)) / 6;
	
	// Move according to wind.
	if (Abs(wind) < 7)
		SetXDir(0);
	else
		SetXDir(wind * 10, 1000);
		
	// Loop clouds around the map.
	if (GetX() >= LandscapeWidth() - 10) 
		SetPosition(12, GetY());
	if (GetX() <= 10) 
		SetPosition(LandscapeWidth()-12, GetY());
		
	// Some other safety.
	if (GetY() <= 5) 
		SetPosition(0, 6);
	if (GetYDir() != 0) 
		SetYDir(0);
	while (Stuck()) 
		SetPosition(GetX(), GetY() - 5);
	return;
}

private func Precipitation()
{
	if (!rain_mat)
		return;	
	// Precipitaion: water or snow.
	if (rain > 0)
	{
		if (RainDrop(rain_mat));
			rain--;	
	}	
	// If out of liquids, skip mode.
	if (rain == 0)
		mode_time = 0;
	return;
}

// Raindrop somewhere from the cloud.
private func RainDrop(string mat)
{
	// Find Random Position.
	var con = GetCon();
	var wdt = GetDefWidth() * con / 500;
	var hgt = GetDefHeight() * con / 700;
	var x = RandomX(-wdt, wdt);
	var y = RandomX(-hgt, hgt);
	if (!GBackSky(x, y))
		return false;
	// Check if liquid is maybe in frozen form.
	var temp = GetTemperature();
	var melt_temp = GetMaterialVal("BelowTempConvert", "Material", Material(mat));
	if (melt_temp != nil && temp < melt_temp)
		mat = GetMaterialVal("BelowTempConvertTo", "Material", Material(mat));
	// Create rain drop.	
	CastPXS(mat, 1, 1, x, y);
	return true;
}

// Launches possibly one thunder strike from the cloud.
private func ThunderStrike()
{
	// Determine whether to launch a strike.
	if (rain < 100)
		return;
	if (Random(100) >= lightning_chance || Random(80))
		return;
	
	// Find random position in the cloud.
	var con = GetCon();
	var wdt = GetDefWidth() * con / 250;
	var hgt = GetDefHeight() * con / 350;
	var x = GetX() + RandomX(-wdt, wdt);
	var y = GetY() + RandomX(-hgt, hgt);
	
	var pix = 0;
	// Check if there is sky for at least 60 pixels.
	while (GBackSky(x - GetX(), y - GetY() + pix) && pix <= 60)
		pix++;
	if (pix < 60)
		return; 
	
	var str = 2 * con / 3 + RandomX(-15, 15);
	// Launch lightning.
	return LaunchLightning(x, y, str, 0, str / 5, str / 10, str / 10, true);
}

// Tries to evaporate liquids from the surface.
protected func Evaporation() 
{
	var prec = 5;
	
	// Found enough water/acid, skip condensing phase.
	if (rain >= 960)
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
	
	// Try to extract the specified material.
	if (GetMaterial(evap_x, y) == Material(rain_mat))
	{
		ExtractMaterialAmount(evap_x, y, Material("Water"), 3);
		rain += 3;
	}
	
	// Also add some rain by scenario value.
	if (Random(100) < rain_amount)
		rain += 1 + Random(3);
		
	return;
}

//Shades the clouds based on iSize: the water density value of the cloud.
private func ShadeCloud()
{
	var alpha = (cloud_alpha + ((rain + 40) * 255) / 960) / 2;
	var alpha = Min(alpha, 255);
	var shade = BoundBy(cloud_shade, 0, 255);

	SetClrModulation(RGBa(255-shade, 255-shade, 255-shade, alpha));
}

// Utilized by time to make clouds invisible at night
public func SetLightingShade(int darkness)
{
	cloud_shade = darkness;
}

// Utilized by time to make clouds invisible at night
public func SetCloudAlpha(int alpha)
{
	cloud_alpha = BoundBy(alpha, 0, 255);
}


/* Scenartio saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (GetComDir() == COMD_None) props->Remove("ComDir");
	props->Remove("Con");
	props->Remove("ClrModulation");
	if (rain_mat) props->AddCall("Precipitation", this, "SetPrecipitation", Format("%v", rain_mat), rain_amount);
	if (lightning_chance) props->AddCall("Lightning", this, "SetLightning", lightning_chance);
	if (rain) props->AddCall("Rain", this, "SetRain", rain);
	return true;
}


/* Properties */

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
