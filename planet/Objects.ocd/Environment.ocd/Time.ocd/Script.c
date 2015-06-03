/**
	Time Controller
	Creates time based on the 24-hour time scheme.
	Time is computed in minutes, which are by default
	1/2 a second in real life (18 frames). This will
	make each complete day/night cycle last 12 minutes
	in real life.

	@author Ringwall, Maikel
*/

local time_set;
local time; 
local advance_seconds_per_tick;


/*-- Interface --*/

// Sets the current time using a 24*60 minute clock scheme. 
public func SetTime(int to_time) 
{
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
	return time / 60;
}

// Sets the number of seconds the day will advance each tick (10 frames).
// Setting to 0 will stop day-night cycle. Default is 30 seconds.
public func SetCycleSpeed(int seconds_per_tick)
{
	advance_seconds_per_tick = seconds_per_tick;
}

// Returns the number of seconds the day advances each tick (10 frames). 
public func GetCycleSpeed()
{
	return advance_seconds_per_tick;
}


/*-- Code -- */

protected func Initialize()
{
	// Only one time control object.
	if (ObjectCount(Find_ID(Environment_Time)) > 1) 
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
		CreateObjectAbove(Moon, LandscapeWidth() / 2, LandscapeHeight() / 6);
	}
	
	// Set the time to midday (12:00).
	SetTime(43200); 
	
	// Add effect that controls time cycle.
	SetCycleSpeed(30);
	AddEffect("IntTimeCycle", this, 100, 10, this);
	return;
}

public func IsDay()
{
	var day_start = (time_set.sunrise_start + time_set.sunrise_end) / 2;
	var day_end = (time_set.sunset_start + time_set.sunset_end) / 2;
	if (Inside(time, day_start, day_end))
		return true;
	return false;
}

public func IsNight()
{
	var night_start = (time_set.sunset_start + time_set.sunset_end) / 2;
	var night_end = (time_set.sunrise_start + time_set.sunrise_end) / 2;
	if (Inside(time, night_start, night_end))
		return true;
	return false;
}

private func PlaceStars()
{
	// Since stars are almost completely parallax (=in screen coordinates), we only need
	// to place stars for max. a reasonable maximum resolution, let's say 1920x1200.
	var lw = Min(LandscapeWidth(), 1920);
	var lh = Min(LandscapeHeight(), 1200);
	
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
	else if(Inside(time, time_set.sunrise_end, time_set.sunset_start))
		day = true;
	else if(Inside(time, time_set.sunset_start, time_set.sunset_end))
		sunset = true;
	else
		night = true;
	
	// Specify colors in terms of R, G, B, A arrays.
	var skyshade = [0, 0, 0, 0];
	var nightcolour = [10, 25, 40];
	var daycolour = [255, 255, 255];
	var sunsetcolour = [140, 45, 10];
	var sunrisecolour = [140, 100, 70];
	
	// Darkness of night dependent on the moon-phase.
	if (!day)
	{
		var satellite = FindObject(Find_ID(Moon));
		if(satellite)
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
	}
	// Day.
	else if (day)
	{
		skyshade = [255, 255, 255, 255];
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
	}
	// Night.
	else if (night)
	{
		skyshade = nightcolour;
		skyshade[3] = 0;
	}
	
	// Shade the sky using sky adjust.
	SetSkyAdjust(RGB(skyshade[0], skyshade[1], skyshade[2]));
	
	// Shade the landscape and the general feeling by reducing the ambient light.
	var new_ambient = 100 * skyshade[2] / 255;
	if (GetAmbientBrightness() != new_ambient)
		SetAmbientBrightness(new_ambient);
	
	// Adjust celestial objects and clouds.
	if (!day && !night)
	{
		for (var celestial in FindObjects(Find_Func("IsCelestial")))
			celestial->SetObjAlpha(255 - skyshade[3]);
		for (var cloud in FindObjects(Find_ID(Cloud)))
			cloud->SetLightingShade(255 - skyshade[2]);
	}
	return;
}


/*-- Scenario saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	// Save time props.
	if (GetTime() != 43200) 
		props->AddCall("Time", this, "SetTime", GetTime());
	if (GetCycleSpeed() != 30) 
		props->AddCall("CycleSpeed", this, "SetCycleSpeed", GetCycleSpeed());
	return true;
}


/*-- Properties --*/

local Name = "Time";
