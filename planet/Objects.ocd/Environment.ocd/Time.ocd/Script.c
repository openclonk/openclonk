/**--
	Time Controller
	Author:Ringwall
	
	Creates time based on the 24-hour time scheme.
	Time is computed in minutes, which are by default
	1/2 a second in real life (18 frames). This will
	make each complete day/night cycle last 12 minutes
	in real life.
--*/


local time; 
local advance_seconds_per_tick;

/** Sets the current time using a 24*60 minute clock scheme. */
public func SetTime(int to_time) 
{
	// Set time.
	time = (to_time*60) % (24 * 60 * 60);
	// hide celestials during day
	if(Inside(time, time_set["SunriseEnd"], time_set["SunsetStart"]))
		HideCelestials();
	else
		ShowCelestials();
	
	// Adjust to time.
	AdjustToTime();
	return;
}

/** Returns the time in minutes. */
public func GetTime()
{
	return time / 60;
}

/** Sets the number of seconds the day will advance each tick (10 frames).
    Setting to 0 will stop day-night cycle. Default is 30 seconds. */
public func SetCycleSpeed(int seconds_per_tick)
{
	advance_seconds_per_tick = seconds_per_tick;
}

/** Returns the number of seconds the day advances each tick (10 frames). */
public func GetCycleSpeed()
{
	return advance_seconds_per_tick;
}

local time_set;


protected func Initialize()
{
	// Only one time control object.
	if (ObjectCount(Find_ID(Environment_Time)) > 1) 
		return RemoveObject();
		
	time_set = {
		SunriseStart = 10800, // 3:00
		SunriseEnd = 32400, // 9:00
		SunsetStart = 54000, // 15:00
		SunsetEnd = 75600, // 21:00
	};

	// Create moon and stars.
	if (FindObject(Find_ID(Environment_Celestial)))
	{
		PlaceStars();
		CreateObjectAbove(Moon, LandscapeWidth() / 2, LandscapeHeight() / 6);
	}
	
	// Set the time to midday (12:00).
	SetTime(43200); 
	
	// Add effect that controls time cycle.
	SetCycleSpeed(30);
	AddEffect("IntTimeCycle", this, 100, 10, this);
}

public func IsDay()
{
	var day_start = (time_set["SunriseStart"] + time_set["SunriseEnd"]) / 2;
	var day_end = (time_set["SunsetStart"] + time_set["SunsetEnd"]) / 2;
	if (Inside(time, day_start, day_end))
		return true;
	return false;
}

public func IsNight()
{
	var night_start = (time_set["SunsetStart"] + time_set["SunsetEnd"]) / 2;
	var night_end = (time_set["SunriseStart"] + time_set["SunriseEnd"]) / 2;
	if (Inside(time, night_start, night_end))
		return true;
	return false;
}

private func PlaceStars()
{
	// since stars are almost completely parallax (=in screen coordinates), we only need
	// to place stars for max. a reasonable maximum resolution. Lets say 1600x1200
	var lw = Min(LandscapeWidth(), 1600);
	var lh = Min(LandscapeHeight(),1200);
	
	//Star Creation
	var maxfailedtries = lw * lh / 40000;
	var failed = 0;

	while (failed != maxfailedtries)
	{
		var pos = [Random(lw), Random(lh)];
		if(!FindObject(Find_ID(Stars),Find_AtPoint(pos[0],pos[1])))
		{
			CreateObjectAbove(Stars, pos[0], pos[1]); 
			continue;
		}
		failed++;
	}
	
	return;
}

// Cycles through day and night.
protected func FxIntTimeCycleTimer(object target)
{
	// Adjust to time.
	AdjustToTime();

	// Advance time.
	time += advance_seconds_per_tick;
	time %= (24 * 60 * 60);
	
	return 1;
}

private func HideCelestials()
{
	// hide celestial objects, they will not be drawn during the day
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
	{
		celestial.Visibility = VIS_None;
		celestial->SetObjAlpha(0);
	}
}

private func ShowCelestials()
{
	// show celestial objects
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
	{
		celestial.Visibility = VIS_All;
	}
}

private func OnSunriseEnd()
{
	// next moon phase
	var satellite = FindObject(Find_ID(Moon));
	if(satellite)
		satellite->NextMoonPhase();
	
	HideCelestials();
}

private func OnSunsetStart()
{
	ShowCelestials();
}

private func DoSkyShade()
{
	// first determine the time phase we are in
	var sunrise, sunset, night, day;
	sunrise = sunset = night = day = false;
	
	if (Inside(time, time_set["SunriseStart"], time_set["SunriseEnd"]))
		sunrise = true;
	else if(Inside(time, time_set["SunriseEnd"], time_set["SunsetStart"]))
		day = true;
	else if(Inside(time, time_set["SunsetStart"], time_set["SunsetEnd"]))
		sunset = true;
	else
		night = true;
	
	var skyshade = [0,0,0,0]; //R,G,B,A
	var nightcolour = [10,25,40]; // default darkest-night colour
	var daycolour = [255,255,255];
	var sunsetcolour = [140,45,10];
	var sunrisecolour = [140,100,70];
	
	if (!day)
	{
		// Darkness of night dependent on the moon-phase
		var satellite = FindObject(Find_ID(Moon));
		if(satellite)
		{
			var lightness = satellite->GetMoonLightness();
			nightcolour = [ 6 * lightness / 100, 8 + 25 * lightness / 100, 15 + 60 * lightness / 100 ];
		}
	}
		
	// Sunrise 
	if (sunrise)
	{
		var time_since_sunrise = time - time_set["SunriseStart"];
		// progress in 0..1800
		var progress = time_since_sunrise * 1800 / (time_set["SunriseEnd"] - time_set["SunriseStart"]);
	
		for(var i=0; i<3; ++i)
		{
			var nightfade = Cos(progress/2, nightcolour[i],10);
			var dayfade = daycolour[i] - Cos(progress/2, daycolour[i],10);
			var sunrisefade = Sin(progress, sunrisecolour[i],10);
			
			skyshade[i] = Min(255,dayfade + nightfade + sunrisefade);
		}
		
		skyshade[3] = Min(255,progress/2);
	}
	// Day
	else if (day)
	{
		skyshade[0] = 255;
		skyshade[1] = 255;
		skyshade[2] = 255;
		
		skyshade[3] = 255;
	}
	// Sunset
	else if (sunset)
	{
		var time_since_sunset = time - time_set["SunsetStart"];
		// progress in 0..1800
		var progress = time_since_sunset * 1800 / (time_set["SunsetEnd"] - time_set["SunsetStart"]);
		
		for(var i=0; i<3; ++i)
		{
			var dayfade = Cos(progress/2, daycolour[i],10);
			var nightfade = nightcolour[i] - Cos(progress/2, nightcolour[i],10);
			var sunsetfade = Sin(progress, sunsetcolour[i],10);
			
			skyshade[i] = Min(255,dayfade + nightfade + sunsetfade);
		}
		
		skyshade[3] = Min(255,900-progress/2);
	}
	// Night
	else if (night)
	{
		skyshade[0] = nightcolour[0];
		skyshade[1] = nightcolour[1];
		skyshade[2] = nightcolour[2];
		
		skyshade[3] = 0;
	}
	
	// Shade sky.
	SetSkyAdjust(RGB(skyshade[0], skyshade[1], skyshade[2]));
	
	// Shade landscape.
	var gamma = [0,0,0];
	var min_gamma = [30,75,120]; 
	gamma[0] = BoundBy(skyshade[0], min_gamma[0], 128);
	gamma[1] = BoundBy(skyshade[1], min_gamma[1], 128);
	gamma[2] = BoundBy(skyshade[2], min_gamma[2], 128);
	
	//SetGamma(0, RGB(gamma[0], gamma[1], gamma[2]), RGB(127+gamma[0], 127+gamma[1], 127+gamma[2]), 3);
	
	if(!day && !night)
	{
		// Adjust celestial objects.
		for (var celestial in FindObjects(Find_Func("IsCelestial")))
			celestial->SetObjAlpha(255 - skyshade[3]);
				
		// Adjust clouds
		for(var cloud in FindObjects(Find_ID(Cloud))){
			cloud->SetLightingShade(255 - skyshade[2]);
		}
	}
}

// Adjusts the sky, celestial and others to the current time. Use SetTime() at runtime, not this.
private func AdjustToTime()
{
	if (Abs(time - time_set["SunriseEnd"]) <= advance_seconds_per_tick)
		OnSunriseEnd();
	else if (Abs(time - time_set["SunsetStart"]) <= advance_seconds_per_tick)
		OnSunsetStart();

	DoSkyShade();
}


/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	// Initialize function depends on this object implicitely
	// So make sure it's created before this
	var celestial_env = FindObject(Find_ID(Environment_Celestial));
	if (celestial_env) celestial_env->MakeScenarioSaveName();
	// Save time props
	if (GetTime() != 43200) props->AddCall("Time", this, "SetTime", GetTime());
	if (GetCycleSpeed() != 30) props->AddCall("CycleSpeed", this, "SetCycleSpeed", GetCycleSpeed());
	return true;
}


/* Properties */

local Name = "Time";
