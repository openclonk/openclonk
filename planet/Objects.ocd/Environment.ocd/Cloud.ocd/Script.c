/**
	Cloud
	Generic cloud, features: rain (water, acid) and thunder.
	The clouds have periods of condensing, idle and raining.
	Different types of rain (water, acid) are hardcoded.
	
	@authors Ringwaul, Maikel
*/

local Plane = -300;

// Cloud modes: idle, raining, condensing.
static const CLOUD_ModeIdle = 0;
static const CLOUD_ModeRaining = 1;
static const CLOUD_ModeCondensing = 2;

static const CLOUD_RainFallOffDistance = 1000;

local mode;
// The time a cloud is in this mode.
local mode_time;

local lightning_chance; // chance of lightning strikes 0-100.
local evap_x; // x coordinate for evaporation

local rain; // Number of liquid pixels the cloud holds.
local rain_mat; // Precipitation type from scenario or other. Material name or nil for no rain.
local rain_inserts_mat; // Should the rain insert actual material pixels on impact?
local rain_amount; // Precipitation amount from scenario or other.
local rain_max; // Max rain the cloud can hold.
local rain_mat_freeze_temp; // Freezing temperature of current rain material.
local rain_mat_frozen; // Material currently frozen to.
local rain_visual_strength; // Amount of rain particles

local rain_sound_dummy; // Helper object that emits the sound

local cloud_shade; // Cloud shade.
local cloud_alpha; // Cloud alpha.
local cloud_color; // Cloud color proplist.

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
	rain_visual_strength = 10;
	rain_inserts_mat = true;
	
	rain_sound_dummy = CreateObject(Dummy, 0, 0, NO_OWNER);
	rain_sound_dummy.Visibility = VIS_All;
	
	// Cloud defaults
	lightning_chance = 0;
	cloud_shade = 0;
	cloud_alpha = 255;
	cloud_color = {r = 255, g = 255, b = 255};
	evap_x = 0;

	DoCon(Random(75));

	SetAction("Fly");
	SetComDir(COMD_None);
	SetPhase(RandomX(1,16));

	// Push low flying clouds up to proper height
	var xoff = 0, yoff = 0;
	while (GetY() + yoff > 0 && !MaterialDepthCheck(xoff, yoff, "Sky", 150))
	{
		yoff--;
	}

	// Failsafe for stupid grounded clouds
	if (GetMaterial(xoff, yoff+30) != Material("Sky")) 
		yoff -= 180;

	SetPosition(GetX()+xoff, GetY()+yoff);

	// Add effect to process all cloud features.
	AddEffect("ProcessCloud", this, 100, 5, this);
	return;
}

protected func Destruction()
{
	if (rain_sound_dummy)
	{
		rain_sound_dummy->RemoveObject();
	}
	_inherited(...);
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
		// Store snow/water conversion
		rain_mat_freeze_temp = GetMaterialVal("BelowTempConvert", "Material", Material(rain_mat));
		// Hack: material val does not return nil for materials that do not freeze, so fix those explicitly.
		if (mat == "Snow" || mat == "Ice")
			rain_mat_freeze_temp = nil;		
		rain_mat_frozen = GetMaterialVal("BelowTempConvertTo", "Material", Material(rain_mat));
		if (rain_mat_frozen == "Ice") rain_mat_frozen = "Snow";
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

// Sets the strength of the visual particle rain.
// Also an id call: Changes all clouds to this settings.
public func SetVisualRainStrength(int to)
{
	// Called to proplist: change all clouds.
	if (this == Cloud)
	{
		for (var cloud in FindObjects(Find_ID(Cloud)))
			cloud->SetVisualRainStrength(to);
	}
	else
	{
		rain_visual_strength = to;
	}
}


// Changes the color of this cloud.
// Also an id call: Changes all clouds to this settings.
public func SetCloudRGB(r, g, b)
{
	// Called to proplist: change all clouds.
	if (this == Cloud)
	{
		for (var cloud in FindObjects(Find_ID(Cloud)))
			cloud->SetCloudRGB(r, g, b);
	}
	else // Otherwise change the clouds color.
	{
		cloud_color.r = r ?? 255;
		cloud_color.g = g ?? 255;
		cloud_color.b = b ?? 255;
	}
	return;
}


// Changes the behavior of this cloud:
// if the parameter is true, the cloud will create particles effects and insert material on impact.
// if the parameter is false, the could will create particle effects, but not insert material 
// Also an id call: Changes all clouds to this settings.
public func SetInsertMaterial(bool should_insert)
{
	// Called to proplist: change all clouds.
	if (this == Cloud)
	{
		for (var cloud in FindObjects(Find_ID(Cloud)))
			cloud->SetInsertMaterial(should_insert);
	}
	else // Otherwise change the clouds precipitation.
	{
		rain_inserts_mat = should_insert;
	}
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

		// Start or stop the rain sound. The object where the sound appears updates position in DropHit()
		if (mode == CLOUD_ModeRaining)
		{
			var mat = RainMat();
			if (rain_sound_dummy && (mat == "Water" || mat == "Acid"))
				rain_sound_dummy->Sound("Liquids::StereoRain", false, 80, nil, +1, CLOUD_RainFallOffDistance);
		}
		else
		{
			if (rain_sound_dummy) rain_sound_dummy->Sound("Liquids::StereoRain",,,, -1);
		}
	}
	// Process modes.
	/*if (mode == CLOUD_ModeIdle)
	{
		// Empty
	}
	else*/ if (mode == CLOUD_ModeRaining)
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
	var wdt = GetDefWidth() * con / 100;
	var hgt = GetDefHeight() * con / 100;
	var xoff = wdt * 10 / 25;
	var yoff = hgt * 10 / 35;
	var wind = (GetWind() + GetWind(xoff, yoff) + GetWind(xoff, -yoff) + GetWind(-xoff, -yoff) + GetWind(-xoff, yoff) + GetWind(nil, nil, true)) / 6;
	
	// Move according to wind.
	if (Abs(wind) < 7)
		SetXDir(0);
	else
		SetXDir(wind * 10, 1000);
		
	var x = GetX(), y = GetY();
	// Loop clouds around the map.
	if (x >= LandscapeWidth() + wdt/2 - 10) 
		x = 12 - wdt/2;
	else if (x <= 10 - wdt/2) 
		x = LandscapeWidth() + wdt/2 - 12;
		
	// Some other safety.
	if (y <= 5) 
		y = 6;
	if (GetYDir() != 0) 
		SetYDir(0);

	if (x != GetX() || y != GetY())
		SetPosition(x, y);

	// We're moving the cloud to y = 6 above, so don't bother moving it higher
	// than that.
	while (Stuck() && GetY() > 10) 
		SetPosition(GetX(), GetY() - 5);
		
	if (rain_sound_dummy)
	{
		rain_sound_dummy->SetPosition(GetX(), GetY());
	}
	return;
}

private func Precipitation()
{
	if (!rain_mat)
		return;
	// Precipitaion: water or snow.
	if (rain > 0)
	{
		if (RainDrop())
			rain--;	
	}	
	// If out of liquids, skip mode.
	if (rain == 0)
		mode_time = 0;
	return;
}

// Check if liquid is maybe in frozen form.
private func RainMat()
{
	if (rain_mat_freeze_temp != nil && GetTemperature() < rain_mat_freeze_temp)
		return rain_mat_frozen;
	else
		return rain_mat;
}

local last_raindrop_color = nil;
local particle_cache;

// Raindrop somewhere from the cloud.
private func RainDrop()
{
	var con = GetCon();
	var wdt = GetDefWidth() * con / 500;
	var hgt = GetDefHeight() * con / 700;

	var mat = RainMat();
	var particle_name = "Raindrop";
	var color = GetMaterialColor(mat);
	if (color != last_raindrop_color)
	{
		particle_cache = {};
		last_raindrop_color = color;
	}

	if (mat == "Lava" || mat == "DuroLava")
		particle_name = "RaindropLava";
	if (mat == "Snow")
		particle_cache.snow = particle_cache.snow ?? Particles_Snow(color);
	else
		particle_cache.rain = particle_cache.rain ?? Particles_Rain(color);

	var count = Max(rain_visual_strength, 1);
	for (var i = 0; i < count; i++)
	{
		var x = RandomX(-wdt, wdt);
		var y = RandomX(-hgt, hgt);
		var xdir = RandomX(GetWind(0,0,1)-5, GetWind(0,0,1)+5)/5;
		var ydir = 30;
		if (!GBackSky(x, y))
			continue;

		if(mat == "Ice")
		{
			// Ice (-> hail) falls faster.
			xdir *= 4;
			ydir *= 4;
		}

		// Snow is special.
		if(mat == "Snow")
		{
			CreateParticle("RaindropSnow", x, y, xdir, 10, PV_Random(2000, 3000), particle_cache.snow, 0);
			if (!i && rain_inserts_mat)
				CastPXS(mat, 1, 0, x, y);
			continue;
		}		

		var particle = new particle_cache.rain {};
		if(Random(2))
			particle.Attach = ATTACH_Back;
		CreateParticle(particle_name, x, y, xdir, ydir, PV_Random(200, 300), particle, 0);

		// Splash.
		if (!i)
		{
			var hit = SimFlight(x, y, xdir, ydir, 25 /* Liquid */, nil, nil, 3);
			var x_final = hit[0], y_final = hit[1], time_passed = hit[4];
			if (time_passed > 0)
			{
					ScheduleCall(this, this.DropHit, time_passed, 0, mat, color, x_final, y_final);
			}
		}
	}
	return true;
}

// Checks whether the given material might smoke when in contact with water.
private func SmokeyMaterial(string material_name)
{
	var mat = Material(material_name);
	return GetMaterialVal("Corrosive", "Material", mat) || GetMaterialVal("Incendiary", "Material", mat);
}

private func DropHit(string material_name, int color, int x_orig, int y_orig)
{
	// Adjust position so that it's in the air.
	var x = AbsX(x_orig), y = AbsY(y_orig);
	while (y > 0 && GBackSemiSolid(x, y - 1)) y--;

	// Add material at impact
	if (rain_inserts_mat)
	{
		InsertMaterial(Material(material_name), x, y - 1);
	}
	
	// Sound?
	if (rain_sound_dummy && Distance(x_orig, y_orig, rain_sound_dummy->GetX(), rain_sound_dummy->GetY()) > CLOUD_RainFallOffDistance)
	{
		rain_sound_dummy->SetPosition(x_orig, y_orig);
	}

	// Some materials cast smoke when hitting water.
	if (GetMaterial(x,y) == Material("Water") && SmokeyMaterial(material_name))
	{
		Smoke(x, y, 3, RGB(150,160,150));
	}
	// Liquid? liquid splash!
	else if(GBackLiquid(x,y))
	{
		particle_cache.splash_water = particle_cache.splash_water ?? Particles_SplashWater(color);
		CreateParticle("RaindropSplashLiquid", x, y - 3, 0, 0, 20, particle_cache.splash_water);
	}
	// Solid? normal splash!
	else
	{
		if( (material_name == "Acid" && GetMaterial(x,y) == Material("Earth")) || material_name == "Lava" || material_name == "DuroLava")
			Smoke(x, y, 3, RGB(150,160,150));
		CreateParticle("RaindropSplash", x, y, 0, 0, 5, Particles_Splash(color), 0);
		if(material_name == "Ice")
		{
			particle_cache.hail = particle_cache.hail ?? Particles_Hail(color);
			CreateParticle("Hail", x, y - 1, RandomX(-2,2), -Random(10), PV_Random(300, 300), particle_cache.hail, 0);
		}
		else
		{
			particle_cache.small_rain = particle_cache.small_rain ?? Particles_RainSmall(color);
			CreateParticle("SphereSpark", x, y - 1, PV_Random(-10, 10), PV_Random(-30, -10), PV_Random(200, 300), particle_cache.small_rain, 5);
		}
	}
}

private func GetMaterialColor(string name)
{
	// A Material's color is actually defined by its texture.
	var texture = GetMaterialVal("TextureOverlay", "Material", Material(name));
	return GetAverageTextureColor(texture);
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
	return LaunchLightning(x, y, str, 0, str / 5, str / 10, str / 10);
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
		// No idea why the value was originally increased by +3 regardless
		// of the return value of ExtractMaterialAmount; just left it that way
		if (rain_inserts_mat)
		{
			ExtractMaterialAmount(evap_x, y, Material("Water"), 3);
		}
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
	var factor = 255 - shade;

	SetClrModulation(RGBa(factor * cloud_color.r / 255, factor * cloud_color.g / 255, factor * cloud_color.b / 255, alpha));
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
	if (rain_mat != nil) props->AddCall("Precipitation", this, "SetPrecipitation", Format("%v", rain_mat), rain_amount);
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
