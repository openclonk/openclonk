/*-- 
	Time Controller
	Author:Ringwall
	
	Creates time based on the 24-hour time scheme.
	Time is computed in minutes, which are by default 
	1/2 a second in real life (18 frames). This will
	make each complete day/night cycle last 12 minutes
	in real life.
--*/

local itime;

global func SetTime(int iTime) //Sets the current time using a 1440-minute clock scheme.
{
	var timeobject=FindObject(Find_ID(Environment_Time));

	//clear any existing sunrise/sunset effects
	if(!GetEffect("IntSunrise")) RemoveEffect("IntSunrise");
	if(!GetEffect("IntSunset")) RemoveEffect("IntSunset");

	if(IsDay()==true && IsDay(iTime)==false)
	{
		AddEffect("IntSunset",0,1,1);
	}

	if(IsDay()==false && IsDay(iTime)==true) 
	{
		AddEffect("IntSunrise",0,1,1);
	}

	timeobject->LocalN("itime")=iTime;
	if(timeobject!=nil) return 1;
	else 
		return 0;
}

global func SetTimeSpeed(int iFrames)
{
	//The number of frames per clonk-minute.
	//36 frames would make one clonk minute one real-life second. 18 frames is the default.
	RemoveEffect("IntTimePass");
	AddEffect("IntTimePass",0,1,iFrames);
}

//If clock is true, an integer of the hours/minutes will be output instead of raw minutes. Not for use in calculations!
global func GetTime(bool clock)
{
	if(!FindObject(Find_ID(Environment_Time))) return nil;
	if(clock!=true)	return FindObject(Find_ID(Environment_Time))->LocalN("itime");
	if(clock==true)
	{
		var hour=FindObject(Find_ID(Environment_Time))->LocalN("itime")/60*100;
		var minute=(FindObject(Find_ID(Environment_Time))->LocalN("itime")*100/60-hour)*6/10;
		return hour+minute;
	}
}

//If iTime is not zero, it will return if iTime is at day or night instead of the current time
global func IsDay(int iTime)
{
	var time=GetTime();
	if(iTime!=nil) time=iTime;

	//The day spans from 300(5:00) to 1140(19:00).
	if(time>=300 && time<1140) return true;

	//if false, then it is night
	return false;
}

protected func Initialize()
{
	if(ObjectCount(Find_ID(Environment_Time))>1) RemoveObject();
	AddEffect("IntTimePass",0,1,18);
	itime=720; //Sets the time to midday (12:00)

	if(FindObject(Find_ID(Environment_Celestial)))
	{
		var moon=CreateObject(Moon,LandscapeWidth()/2,LandscapeHeight()/6);
		moon->Resort();
		PlaceStars();
	}
}

protected func PlaceStars()
{
	//Star Creation
	var maxamount=(LandscapeWidth()*LandscapeHeight())/40000;
	var amount=0;

	while(amount!=maxamount)
	{
		var pos;
		if(pos = FindPosInMat("Sky", 0,0,LandscapeWidth(), LandscapeHeight()))
			CreateObject(Star,pos[0],pos[1]); //Places stars around like PlacesObjects should, but that function is broken
		amount=++amount;
	}
}

global func FxIntTimePassTimer(object pTarget,int iNumber,int iTime)
{
	Message("%d",0,GetTime(true));

	if(GetTime()==299) AddEffect("IntSunrise",0,1,1);
	if(GetTime()==1140) AddEffect("IntSunset",0,1,1);

	//time advancement
	if(GetTime()>=1439) SetTime(0);
	if(GetTime()<1439) SetTime(GetTime()+1);
	return 1;
}

global func FxIntSunriseTimer(object pTarget, int iNumber,int iTime)
{
	if(iTime>60)
	{
		var moon=FindObject(Find_ID(Moon));
		if(moon) moon->Phase();
		return -1;
	}

	var skyshade=iTime*425/100;
	SetSkyAdjust(RGB(skyshade,skyshade,skyshade));
	for(var celestial in FindObjects(Find_Func("IsCelestial")))
		celestial->SetObjAlpha(255-skyshade); //makes objects with the IsCelestial func fade away
}

global func FxIntSunsetTimer(object pTarget, int iNumber,int iTime)
{
	if(iTime>60) return -1;
	var skyshade=255-(iTime*425/100);
	SetSkyAdjust(RGB(skyshade,skyshade,skyshade));
	for(var celestial in FindObjects(Find_Func("IsCelestial")))
		celestial->SetObjAlpha(255-skyshade); //makes objects with the IsCelestial func appear
}

func Definition(def) {
	SetProperty("Name","Time",def);
}