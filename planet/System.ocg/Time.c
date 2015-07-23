/*--
		Time.c
		Authors: Clonkonaut

		Wrapper for local functions of Environment_Time (Objects.ocd\Environment.ocd\Time.ocd).
		See there for detailed function descriptions.
--*/

// Returns the time object if there is any
global func HasDayNightCycle()
{
	return FindObject(Find_ID(Environment_Time));
}

global func SetTime(int time)
{
	var e_time = HasDayNightCycle();
	if (!e_time) return;
	return e_time->SetTime(time);
}

global func GetTime()
{
	var e_time = HasDayNightCycle();
	if (!e_time) return;
	return e_time->GetTime();
}

global func IsDay()
{
	var e_time = HasDayNightCycle();
	if (!e_time) return;
	return e_time->IsDay();
}

global func IsNight()
{
	var e_time = HasDayNightCycle();
	if (!e_time) return;
	return e_time->IsNight();
}