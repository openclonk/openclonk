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
	
	// Add effect that controls time cycle.
	advance_seconds_per_tick = 30;
	AddEffect("IntTimeCycle", this, 100, 10, this);
	
	// Set the time to midday (12:00).
	time = 43200; 

	// Create moon and stars.
	if (FindObject(Find_ID(Environment_Celestial)))
	{
		CreateObject(Moon, LandscapeWidth() / 2, LandscapeHeight() / 6);
		PlaceStars();
	}
	return;
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
	//Star Creation
	var maxamount = LandscapeWidth() * LandscapeHeight() / 40000;
	var amount = 0;

	while (amount != maxamount)
	{
		var pos;
		if (pos = FindPosInMat("Sky", 0, 0, LandscapeWidth(), LandscapeHeight()))
			CreateObject(Star, pos[0], pos[1]); 
		amount++;
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

// Adjusts the sky, celestial and others to the current time. Use SetTime() at runtime, not this.
private func AdjustToTime()
{
	var skyshade = [0,0,0,0]; //R,G,B,A
	
	var nightcolour = [10,25,40]; // default darkest-night colour
	var daycolour = [255,255,255];
	var sunsetcolour = [140,45,10];
	var sunrisecolour = [140,100,70];
	
	// Darkness of night dependent on the moon-phase
	var satellite = FindObject(Find_ID(Moon));
	if(satellite){
		var lightness = satellite->GetMoonLightness();
		nightcolour = [ 6 * lightness / 100, 8 + 25 * lightness / 100, 15 + 60 * lightness / 100 ];
		
		if (Abs(time - time_set["SunriseEnd"]) <= advance_seconds_per_tick)
			satellite->NextMoonPhase();
	}
		
	// Sunrise 
	if (Inside(time, time_set["SunriseStart"], time_set["SunriseEnd"]))
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
		
		skyshade[3] = Min(255,progress);
	}
	// Day
	else if (Inside(time, time_set["SunriseEnd"], time_set["SunsetStart"]))
	{
		skyshade[0] = 255;
		skyshade[1] = 255;
		skyshade[2] = 255;
		
		skyshade[3] = 255;
	}
	// Sunset
	else if (Inside(time, time_set["SunsetStart"], time_set["SunsetEnd"]))
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
		
		skyshade[3] = Min(255,1800-progress);
	}
	// Night
	else if (time > time_set["SunsetEnd"] || time < time_set["SunriseStart"])
	{
		skyshade[0] = nightcolour[0];
		skyshade[1] = nightcolour[1];
		skyshade[2] = nightcolour[2];
		
		skyshade[3] = 0;
	}
	
	// Shade sky.
	SetSkyAdjust(RGB(skyshade[0], skyshade[1], skyshade[2]));
	
	// Adjust celestial objects.
	for (var celestial in FindObjects(Find_Func("IsCelestial")))
			celestial->SetObjAlpha(255 - skyshade[3]);
			
	// Adjust clouds
	for(var cloud in FindObjects(Find_ID(Cloud))){
		cloud->SetLightingShade(255 - skyshade[2]);
	}
	
	return;
}

local Name = "Time";
