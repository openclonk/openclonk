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

global func SetMinuteLength(int iFrames)
{
	//The number of frames per clonk-minute.
	//36 frames would make one clonk minute one real-life second. 18 frames is the default.
	RemoveEffect("IntTimePass");
	AddEffect("IntTimePass",0,1,iFrames);
}

global func GetMinuteLength()
{
	return GetEffect("IntTimePass",0,0,3);
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

	//The day spans from 360 to 1080.
	if(time >= 360 && time < 1080) return true;

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

	var iX;
	var iY;
	while(amount!=maxamount)
	{
		if(FindPosInMat(iX, iY, "Sky", 0,0,LandscapeWidth(), LandscapeHeight()))
			CreateObject(Star,iX,iY); //Places stars around like PlacesObjects should, but that function is broken
		amount=++amount;
	}
}

global func FxIntTimePassTimer(object pTarget,int iNumber,int iTime)
{
	UpdateTime(true);
}

//bool advance: if true, time will advance on update
global func UpdateTime(bool advance)
{
	//Sunrise begins at 180 and ends at 540
	//Sunset begins at 900 and ends at 1260
	
	//Night Time (1260-180)
	if(GetTime() < 180 || GetTime() > 1260)
	{
		var skyshade = 0;
		ShadeCelestial(255);
	}

	//Sunrise (180-540)
	if(GetTime() >= 180 && GetTime() <=540)
	{
		var skyshade = Sin((GetTime() - 180) / 4,255);
		ShadeCelestial(255-skyshade);
		if(GetTime() == 540) FindObject(Find_ID(Moon))->Phase();
	}

	//Day Time (540-900)
	if(GetTime() > 540 && GetTime() < 900)
	{
		var skyshade = 255;
		ShadeCelestial(0);
	}

	//Sunset (900-120)
	if(GetTime() >= 900 && GetTime() <= 1260)
	{
		var skyshade = 255 - Sin((GetTime() - 900) / 4,255);
		ShadeCelestial(255-skyshade);
	}

	//Shade sky
	SetSkyAdjust(RGB(skyshade,skyshade,skyshade));

	//time advancement
	if(advance != true) 
		return 1;
	if(GetTime()>=1439) SetTime(0);
	if(GetTime()<1439) SetTime(GetTime()+1);
	return 1;
}

global func ShadeCelestial(int shade)
{
	for(var celestial in FindObjects(Find_Func("IsCelestial")))
			celestial->SetObjAlpha(shade);
	return 1;
}

//Makes clonk time follow real time. Just for fun. :]
global func EnableRealTime(bool enable)
{
	if(enable == true)
	{
		FindObject(Find_ID(Environment_Time))->AddEffect("RealTime",0,1,36);
	}
	if(enable == false)
	{
		RemoveEffect("RealTime");
	}
}

global func FxRealTimeTimer(object target, int num, int time)
{
	SetMinuteLength(0);
	SetTime(GetSystemTime(4)*60 + GetSystemTime(5));
	UpdateTime();
}

func Definition(def) {
	SetProperty("Name","Time",def);
}
