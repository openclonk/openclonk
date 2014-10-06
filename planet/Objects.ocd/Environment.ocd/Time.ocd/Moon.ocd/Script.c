/*-- Moon --*/

local phase;

protected func Initialize()
{
	var alpha=0;
	if(GetTime() < 300 || GetTime() > 1140) alpha=255;
	SetClrModulation(RGBa(255,255,255,alpha));
	SetAction("Be");
	Update();
	this["Parallaxity"] = [30,30];
}

public func NextMoonPhase()
{
	SetMoonPhase(phase+1);
}

/** @return values from 0..100, depending on the full-ness of the moon */
public func GetMoonLightness()
{
	return 100 - Abs(100 * phase / this.ActMap.Be.Length - 50);
}

public func GetMoonPhase()
{
	return phase;
}
public func SetMoonPhase(int iphase)
{
	phase = iphase % this.ActMap.Be.Length;
	Update();
}

private func Update() {
	SetPhase(phase);
	
	var phases = this.ActMap.Be.Length;
	
	var x = phase - phases/2;
	var height = LandscapeHeight() / (6 - (x*x)/phases);
	var width = 100 + phase * (LandscapeWidth()-200) / phases;
	
	SetPosition(width,height);
}

// only appears during the night
public func IsCelestial() { return true; }

// Not stored by itself because it's created by the time environment
func SaveScenarioObject() { return false; }


local ActMap = {

Be = {
	Prototype = Action,
	Name = "Be",
	Procedure = DFA_FLOAT,
	Length = 8,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 128,
	Hgt = 128,
	NextAction = "Hold"
}
};
