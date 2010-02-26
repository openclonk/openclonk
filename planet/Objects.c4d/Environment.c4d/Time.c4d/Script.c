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

global func SetTime(int iTime) //Sets the current time using a 24-hour clock scheme.
{
	var timeobject=FindObject(Find_ID(TIME));

	if(IsDay()==true && IsDay(iTime)==false) AddEffect("IntSunset",0,1,GetEffect("IntTimePass",0,0,3));
	if(IsDay()==false && IsDay(iTime)==true) AddEffect("IntSunrise",0,1,GetEffect("IntTimePass",0,0,3));
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
	if(!FindObject(Find_ID(TIME))) return nil;
	if(clock!=true)	return FindObject(Find_ID(TIME))->LocalN("itime");
	if(clock==true)
	{
		var hour=FindObject(Find_ID(TIME))->LocalN("itime")/60*100;
		var minute=(FindObject(Find_ID(TIME))->LocalN("itime")*100/60-hour)*6/10;
		return hour+minute;
	}
}

//If iTime is not zero, it will return if iTime is at day or night instead of the current time
global func IsDay(int iTime)
{
	var time=GetTime();
	if(iTime!=nil) time=iTime;

	//The day spans from 300(5:00) to 1140(19:00).
	if(time>300 && time<1140) return true;

	//if false, then it is night
	return false;
}

protected func Initialize()
{
	if(ObjectCount(Find_ID(TIME))>1) RemoveObject();
	AddEffect("IntTimePass",0,1,18);
	itime=720; //Sets the time to midday (12:00)
	CreateObject(MOON,LandscapeWidth()/2,LandscapeHeight()/6); //Currently stars display in front of the moon for some reason D:
	PlaceStars();
}

protected func PlaceStars()
{
	//Star Creation
	var maxamount=(LandscapeWidth()*LandscapeHeight())/40000;
	var amount=0;

	var iX;
	var iY;
	while(amount!=maxamount)
	{
		if(FindPosInMat(iX, iY, "Sky", 0,0,LandscapeWidth(), LandscapeHeight()))
			CreateObject(STAR,iX,iY); //Places stars around like PlacesObjects should, but that function is broken
		amount=++amount;
	}
}

global func FxIntTimePassTimer(object pTarget,int iNumber,int iTime)
{
	Message("%s",0,GetTime(true));

	if(GetTime()==300) AddEffect("IntSunrise",0,1,GetEffect("IntTimePass",0,0,3));
	if(GetTime()==1140) AddEffect("IntSunset",0,1,GetEffect("IntTimePass",0,0,3));

	//time advancement
	if(GetTime()>=1439) SetTime(0);
	if(GetTime()<1439) SetTime(GetTime()+1);
	return 1;
}

global func FxIntSunriseTimer(object pTarget, int iNumber,int iTime)
{
	var skyshade=iTime*425/100;
	SetSkyAdjust(RGB(skyshade,skyshade,skyshade));
	for(var celestial in FindObjects(Find_Func("IsCelestial")))
		celestial->SetObjAlpha(255-skyshade); //makes objects with the IsCelestial func fade away
	if(skyshade==255)
	{
		FindObject(Find_ID(MOON))->Phase();
		return -1;
	}
}

global func FxIntSunsetTimer(object pTarget, int iNumber,int iTime)
{
	var skyshade=255-(iTime*425/100);
	SetSkyAdjust(RGB(skyshade,skyshade,skyshade));
	for(var celestial in FindObjects(Find_Func("IsCelestial")))
		celestial->SetObjAlpha(255-skyshade); //makes objects with the IsCelestial func appear
	if(skyshade==0) return -1;
}

func Definition(def) {
	SetProperty("Name","Time",def);
}