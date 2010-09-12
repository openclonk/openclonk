/*-- Moon --*/

local phase;

protected func Initialize()
{
	var alpha=0;
	if(GetTime()<300 || GetTime()>1140) alpha=255;
	SetClrModulation(RGBa(255,255,255,alpha));
	phase=1;
	Phase(true);
	this["Parallaxity"] = [40,100];
}

public func Phase(bool noAdvance)
{
	if(noAdvance!=true)
	{	
	if(phase<=5) phase=phase+1;
	if(phase>=6) phase=1;
	}

	if(phase==1) SetGraphics();
	if(phase>1) SetGraphics(Format("%d",LocalN("phase")-1));
	if(phase==1) SetPosition(LandscapeWidth()/4,LandscapeHeight()/4);
	if(phase==2) SetPosition(LandscapeWidth()/3,LandscapeHeight()/5);
	if(phase==3) SetPosition(LandscapeWidth()/2,LandscapeHeight()/6);
	if(phase==4) SetPosition(LandscapeWidth()-LandscapeWidth()/3,LandscapeHeight()/5);
	if(phase==5) SetPosition(LandscapeWidth()-LandscapeWidth()/4,LandscapeHeight()/4);
}

//Get phase can also be used to modify the phase... ie: QueryPhase(3) would set
//the phase of the moon to phase 3. QueryPhase() will simply return the current moon phase.
global func GetPhase(int iphase)
{
	var moonphase=FindObject(Find_ID(Moon))->LocalN("phase");
	if(iphase!=nil && iphase<6) FindObject(Find_ID(Moon))->SetPhase(iphase);
	return moonphase;
}
//ties global to local func
public func SetPhase(int iphase)
{
	phase=iphase;
	Phase(true);
}

//only appears during the night
public func IsCelestial() { return true; }

local Name = "$Name$";
