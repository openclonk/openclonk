/**
	Time Controller
	Creates time based on the 24-hour time scheme. Time is 
	computed in minutes, which are by default 1/2 a second in
	real life (18 frames). This will make each complete 
	day/night cycle last 12 minutes in real life.
	
	The time controller has an interface which is accessed by:
		Time->HasDayNightCycle(): whether time controller is active.
		Time->SetTime(int to_time): sets the time.
		Time->GetTime(): returns the time.
		Time->IsDay(): whether it is day.
		Time->IsNight(): whether it is night.
		Time->SetCycleSpeed(int seconds_per_tick): set the speed of the day/night cycle.
		Time->GetCycleSpeed(): returns the speed of the day/night cycle.

	@author Ringwall, Maikel
*/

local time_set;
local time; 
local advance_seconds_per_tick;
local daycolour_global;

/*-- Interface --*/

// Creates the time controller object if it does not exist otherwise returns the existing controller.
public func Init()
{
	// Only a definition call if needed.
	if (GetType(this) != C4V_Def)
		return;
	// Create and return time controller if it does not exist.
	var time_controller = FindObject(Find_ID(Time));
	if (!time_controller)
		time_controller = CreateObject(Time);
	return time_controller;
}

// Returns whether the time controller is active.
public func HasDayNightCycle()
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
		return FindObject(Find_ID(Time));
	return;
}

// Sets the current time using a 24*60 minute clock scheme.
public func SetTime(int to_time) 
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			time_controller->SetTime(to_time);
		return;
	}
	// Otherwise normal behavior.
	// Set time.
	time = (to_time * 60) % (24 * 60 * 60);
	// Hide celestials during day.
	if (Inside(time, time_set.sunrise_end, time_set.sunset_start))
		HideCelestials();
	else
		ShowCelestials();
	// Adjust to time.
	AdjustToTime();
	return;
}

// Returns the time in minutes.
public func GetTime()
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			return time_controller->GetTime();
		return;
	}
	// Otherwise normal behavior.
	return time / 60;
}

public func IsDay()
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			return time_controller->IsDay();
		// If there is no time controller active it is day.	
		return true;
	}
	// Otherwise normal behavior.
	var day_start = (time_set.sunrise_start + time_set.sunrise_end) / 2;
	var day_end = (time_set.sunset_start + time_set.sunset_end) / 2;
	if (Inside(time, day_start, day_end))
		return true;
	return false;
}

public func IsNight()
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			return time_controller->IsNight();
		// If there is no time controller active it is not night.
		return false;
	}
	// Otherwise normal behavior.
	var night_start = (time_set.sunset_start + time_set.sunset_end) / 2;
	var night_end = (time_set.sunrise_start + time_set.sunrise_end) / 2;
	if (!Inside(time, night_end, night_start))
		return true;
	return false;
}

// Sets the number of seconds the day will advance each tick (10 frames).
// Setting to 0 will stop day-night cycle. Default is 30 seconds.
public func SetCycleSpeed(int seconds_per_tick)
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			time_controller->SetCycleSpeed(seconds_per_tick);
		return;
	}
	// Otherwise normal behavior.
	advance_seconds_per_tick = seconds_per_tick;
}

// Returns the number of seconds the day advances each tick (10 frames). 
public func GetCycleSpeed()
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			return time_controller->GetCycleSpeed();
		return;
	}
	// Otherwise normal behavior.
	return advance_seconds_per_tick;
}


// Places stars in the indicated rectangle from (0, 0) to (lw, lh).
public func PlaceStars(int lw, int lh)
{
	// Do definition call if needed.
	if (GetType(this) == C4V_Def)
	{
		var time_controller = FindObject(Find_ID(Time));
		if (time_controller)
			return time_controller->PlaceStars(lw, lh);
		return;
	}
	
	// First remove possible old star objects, to prevent too many.
	RemoveAll(Find_ID(Stars));
	
	// Note: The defaults used to be geared just to the (assumed) screen size, because the stars
	// have parallaxity. This did not work well for two reasons:
	// 1. Screens have gained a lot of pixels recently, with 4K-and-larger screens getting common.
	// 2. Even on smaller low-density screens, full map screenshots simulate a way larger viewport.
	// To be resolably future-proof, I've bumped the maximum screen size to 5K. To fix the second
	// issue, always create enough stars to fill the whole landscape and also add a 12% margin for
	// the parallaxity.
	lw = lw ?? Max(5120, LandscapeWidth())  * 112 / 100;
	lh = lh ?? Max(2880, LandscapeHeight()) * 112 / 100;
	
	// Star Creation.
	var maxfailedtries = lw * lh / 40000;
	var failed = 0;

	while (failed != maxfailedtries)
	{
		var pos = [Random(lw), Random(lh)];
		if (!FindObject(Find_ID(Stars), Find_AtPoint(pos[0], pos[1])))
		{
			CreateObjectAbove(Stars, pos[0], pos[1]); 
			continue;
		}
		failed++;
	}	
	return;
}


/*-- Code -- */

protected func Initialize()
{
	// Only one time control object.
	if (ObjectCount(Find_ID(Time)) > 1) 
		return RemoveObject();
	
	// Determine the frame times for day and night events.
	time_set = {
		sunrise_start =  3 * 60 * 60, //  3:00
		sunrise_end   =  9 * 60 * 60, //  9:00
		sunset_start  = 15 * 60 * 60, // 15:00
		sunset_end    = 21 * 60 * 60, // 21:00
	};

	// Create moon and stars if celestial objects are not blocked by the scenario.
	if (!GameCall("HasNoCelestials"))
	{
		PlaceStars();
		CreateObject(Moon);
	}
	
	// Set standard colour of the day
	daycolour_global = [255, 255, 255];
	
	// Set the time to midday (12:00).
	SetTime(12 * 60);
	
	// Add effect that controls time cycle.
	SetCycleSpeed(30);
	AddEffect("IntTimeCycle", this, 100, 10, this);
}

public func Destruction()
{
	// Only if last object.
	if (ObjectCount(Find_ID(Time)) > 1) 
		return;
	// Remove celestial objects.
	RemoveAll(Find_Func("IsCelestial"));
	// Reset sky shading and ambience.
	SetTime(12 * 60);
	DoSkyShade();
	return;
}

// Cycles through day and night.
protected func FxIntTimeCycleTimer(object target, proplist effect)
{
	// Adjust to time.
	AdjustToTime();

	// Advance time.
	time += Max(advance_seconds_per_tick * effect.Interval / 10, 1);
	time %= (24 * 60 * 60);
	
	return FX_OK;
}

private func HideCelestials()
{
	// Hide celestial objects, they will not be drawn during the day.
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
	{
		celestial.Visibility = VIS_None;
		celestial->SetObjAlpha(0);
	}
	return;
}

private func ShowCelestials()
{
	// Show celestial objects.
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
		celestial.Visibility = VIS_All;
	return;
}

private func OnSunriseEnd()
{
	// Next moon phase.
	var moon = FindObject(Find_ID(Moon));
	if (moon)
		moon->NextMoonPhase();
	HideCelestials();
}

private func OnSunsetStart()
{
	ShowCelestials();
}

// Adjusts the sky, celestial and others to the current time. Use SetTime() at runtime, not this.
private func AdjustToTime()
{
	if (time >= time_set.sunrise_end && time < time_set.sunrise_end + advance_seconds_per_tick)
		OnSunriseEnd();
	else if (time >= time_set.sunset_start && time < time_set.sunset_start + advance_seconds_per_tick)
		OnSunsetStart();
	DoSkyShade();
	return;
}

private func DoSkyShade()
{
	// First determine the time phase we are in.
	var sunrise, sunset, night, day;
	sunrise = sunset = night = day = false;
	
	if (Inside(time, time_set.sunrise_start, time_set.sunrise_end))
		sunrise = true;
	else if (Inside(time, time_set.sunrise_end, time_set.sunset_start))
		day = true;
	else if (Inside(time, time_set.sunset_start, time_set.sunset_end))
		sunset = true;
	else
		night = true;
	
	// Specify colors in terms of R, G, B, A arrays.
	var skyshade = [0, 0, 0, 0];
	var nightcolour = [10, 25, 40];
	var daycolour = daycolour_global;
	var sunsetcolour = [140, 45, 10];
	var sunrisecolour = [140, 100, 70];
	var ambient_brightness = 0;
	
	// Darkness of night dependent on the moon-phase.
	if (!day)
	{
		var satellite = FindObject(Find_ID(Moon));
		if (satellite)
		{
			var lightness = satellite->GetMoonLightness();
			nightcolour = [ 6 * lightness / 100, 8 + 25 * lightness / 100, 15 + 60 * lightness / 100 ];
		}
	}

	// Sunrise.
	if (sunrise)
	{
		var time_since_sunrise = time - time_set.sunrise_start;
		// progress in 0..1800
		var progress = time_since_sunrise * 1800 / (time_set.sunrise_end - time_set.sunrise_start);
	
		for (var i = 0; i < 3; ++i)
		{
			var nightfade = Cos(progress / 2, nightcolour[i], 10);
			var dayfade = daycolour[i] - Cos(progress / 2, daycolour[i], 10);
			var sunrisefade = Sin(progress, sunrisecolour[i], 10);
			skyshade[i] = Min(255, dayfade + nightfade + sunrisefade);
		}
		skyshade[3] = Min(255, progress / 2);
		ambient_brightness = 100 * (40 + 215 * progress / 1800) / 255;
	}
	// Day.
	else if (day)
	{
		skyshade = [daycolour[0], daycolour[1], daycolour[2], 255];
		//daycolour_global = [GetRGBaValue(GetSkyAdjust(), RGBA_RED), GetRGBaValue(GetSkyAdjust(), RGBA_GREEN), GetRGBaValue(GetSkyAdjust(), RGBA_BLUE)];
		ambient_brightness = 100;
	}
	// Sunset.
	else if (sunset)
	{
		var time_since_sunset = time - time_set.sunset_start;
		// progress in 0..1800
		var progress = time_since_sunset * 1800 / (time_set.sunset_end - time_set.sunset_start);
		
		for (var i = 0; i < 3; ++i)
		{
			var dayfade = Cos(progress / 2, daycolour[i], 10);
			var nightfade = nightcolour[i] - Cos(progress / 2, nightcolour[i], 10);
			var sunsetfade = Sin(progress, sunsetcolour[i], 10);
			skyshade[i] = Min(255, dayfade + nightfade + sunsetfade);
		}		
		skyshade[3] = Min(255, 900 - progress / 2);
		ambient_brightness = 100 * (255 - 215 * progress / 1800) / 255;
	}
	// Night.
	else if (night)
	{
		skyshade = nightcolour;
		skyshade[3] = 0;
		ambient_brightness = 15;
	}

	// Shade the sky using sky adjust.
	SetSkyAdjust(RGBa(skyshade[0], skyshade[1], skyshade[2], GetRGBaValue(GetSkyAdjust(), RGBA_ALPHA)), GetSkyAdjust(true));

	// Shade the landscape and the general feeling by reducing the ambient light.
	if (GetAmbientBrightness() != ambient_brightness)
		SetAmbientBrightness(ambient_brightness);

	// Adjust celestial objects and clouds.
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
		celestial->SetObjAlpha(255 - skyshade[3]);
	for (var cloud in FindObjects(Find_ID(Cloud)))
		cloud->SetLightingShade(255 - skyshade[3]);
	return;
}


/*-- Scenario saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	// Save time props.
	if (GetTime() != 720) 
		props->AddCall("Time", this, "SetTime", GetTime());
	if (GetCycleSpeed() != 30) 
		props->AddCall("CycleSpeed", this, "SetCycleSpeed", GetCycleSpeed());
	return true;
}


/*-- Editor --*/

public func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.Time = { Name="$Time$", EditorHelp="$TimeHelp$", Type="int", Min = 0, Max = 24*60 - 1, AsyncGet="GetTime", Set="SetTime" };
	def.EditorProps.CycleSpeed = { Name="$CycleSpeed$", EditorHelp="$CycleSpeedHelp$", Type="int", Min = 0, AsyncGet="GetCycleSpeed", Set="SetCycleSpeed" };
}


/*-- Properties --*/

local Name = "Time";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1;
