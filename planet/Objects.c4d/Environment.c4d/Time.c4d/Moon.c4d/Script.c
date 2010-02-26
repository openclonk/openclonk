/*-- Moon --*/

local phase;

protected func Initialize()
{
	var alpha=0;
	if(GetTime()<300 || GetTime()>1140) alpha=255;
	SetClrModulation(RGBa(255,255,255,alpha));
	phase=0;
	Phase();
	this["Parallaxity"] = [30,100];
}

public func Phase()
{
	if(phase<5) phase=phase+1;
	if(phase>=5) phase=1;

	SetGraphics(Format("%d.2",phase-1));
	if(phase==1) SetPosition(LandscapeWidth()/4,LandscapeHeight()/4);
	if(phase==2) SetPosition(LandscapeWidth()/3,LandscapeHeight()/5);
	if(phase==3) SetPosition(LandscapeWidth()/2,LandscapeHeight()/6);
	if(phase==4) SetPosition(LandscapeWidth()-LandscapeWidth()/3,LandscapeHeight()/5);
	if(phase==5) SetPosition(LandscapeWidth()-LandscapeWidth()/4,LandscapeHeight()/4);
}

global func QueryPhase(int iphase)
{
	var moonphase=FindObject(Find_ID(MOON))->LocalN("phase");
	if(iphase!=nil && iphase<6) SetPhase(iphase);
	return moonphase;
}
//ties global to local func
public func SetPhase(int iphase)
{
	phase=iphase;
}

//only appears during the night
public func IsCelestial() { return true; }

func Definition(def) {
	SetProperty("Name","$Name$",def);
}