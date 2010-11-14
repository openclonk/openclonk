/*--- Cloud ---*/

local szMat, iSize, iCondensing;
local iSearchY;
local iWaitTime;
local iAcidity;
local iStrikeChance;

protected func Initialize()
{
	//Cloud defaults and modifiers
	iCondensing = 0;
	iSize = RandomX(300,500);
	SetClrModulation(RGB(255,255,255));
	iSearchY = 0;
	iAcidity=0;
	iWaitTime = RandomX(130,190);
	var iGraphics = RandomX(1,3);

	DoCon(Random(75));

	SetAction("Fly");
	SetPhase(RandomX(1,16));

	//Push low flying clouds up to proper height
	while(MaterialDepthCheck(GetX(),GetY(),"Sky",150)!=true)
	{
		SetPosition(GetX(),GetY()-1);
	}

	//Failsafe for stupid grounded clouds
	if(GetMaterial(0,30)!=Material("Sky")) SetPosition(GetX(), GetY()-180);

}

public func Precipitation()
{
	var iLaunch;

	if (GetTemperature() < 0 && iAcidity == 0) szMat = "Snow";
	if (GetTemperature() >= 1 && iAcidity == 0) szMat = "Water";
	if (iAcidity >= 1) szMat="Acid";

	//Reroute function to Evaporation if cloud is growing
	if(iCondensing == 1) return(Evaporation());
	if(iSize <= 50 && iAcidity==0) iCondensing = 1;

	//water-snow precipitation
	if(iWaitTime == 0 && szMat != "Acid")
	{
		RainDrop();
		iSize = --iSize;
	}

	//acid precipitation
	if(iWaitTime == 0 && szMat == "Acid")
	{
		RainDrop();
		iAcidity = --iAcidity;
	}
	//Lightning Strike; only during rain
	if(iWaitTime <= 0 && iSize >= 650 && Random(100) >= 100-(iStrikeChance/16) && szMat=="Water") LaunchLightning(GetX(), GetY(), RandomX(60, 100), 0, 100, 100, 10, true);
}

public func TimedEvents()
{
	var iRight = LandscapeWidth() - 10;

	if(iWaitTime != 0) (iWaitTime = --iWaitTime);
	if(iWaitTime == 0) Precipitation();
	WindDirection();
	CloudShade();
	//Makes clouds loop around map;
	if(GetX() >= iRight) SetPosition(12, GetY());
	if(GetX() <= 10) SetPosition(LandscapeWidth()-12, GetY());
	if(GetY() <= 5) SetPosition(0,6);
	if(GetYDir()!=0) SetYDir(0);

	while(Stuck()) SetPosition(GetX(),GetY()-5);
}

protected func Evaporation() //Creates a search line every x-amount(currently five) of pixels to check for water beneath the cloud
{
	var iSearchX = GetX();
	var iPrecision = 5;
	
	if(iSize >= 700 || iAcidity >= 100)
	{
		iCondensing = 0;
		iSearchY = 0;
		iWaitTime = RandomX(130,190);
	}
	//line below prevents clouds evaporating through solids
	if(GetMaterial(0, iSearchY) != Material("Water") && GetMaterial(0, iSearchY) != Material("Acid") && GetMaterial(0, iSearchY) != Material("Sky")) return(iSearchY=0);
	if(GetMaterial(0, iSearchY) == Material("Water"))
	{
		ExtractMaterialAmount(0, iSearchY,Material("Water"), 3);
		iSize = iSize+3;
	}
	if(ObjectCount(Find_ID(Environment_AcidRain))>=1 && GetMaterial(0, iSearchY) == Material("Acid")) {
		ExtractMaterialAmount(0, iSearchY,Material("Acid"), 3);
		iAcidity = iAcidity+3;
	}
	//advance search point
	if(GetMaterial(0, iSearchY) != Material("Water") && GetMaterial(0, iSearchY) != Material("Acid"))
		iSearchY += iPrecision;

	if(iSearchY + GetY() >= LandscapeHeight()) (iSearchY = 0);
}

private func CloudShade()
{
//Shades the clouds based on iSize: the water density value of the cloud.
	var iShade = iSize*425/1000;
	var iShade2 = iSize-600;
	var iShade3 = (iAcidity*255/100)/2;

	if(iSize <= 600) SetObjAlpha(iShade);
	if(iSize > 600) SetObjAlpha(255);
	if(iSize > 600 && szMat=="Water") SetClrModulation(RGBa(255-iShade2,255-iShade2,255-iShade2, 255));
	if(iAcidity >= 1) SetClrModulation(RGBa(255-iShade3,255,255-iShade3, 255-iShade));
}

public func RainDrop()
{
	var angle = RandomX(0,359);
	var dist = Random(51);
	CastPXS(szMat, 1, 1, Sin(angle,dist),Cos(angle,dist));
}

//For use as scenario setting. Can work after initialize, if you really want to.
global func AdjustLightningFrequency(int iFreq)
{
	for(var Cloud in FindObjects(Find_ID(Cloud)))
		Cloud->SetLightningFrequency(iFreq);
	return(iFreq);
}

//Routes the global adjust function's variable to the clouds.
public func SetLightningFrequency(int iFreq)
{
	iStrikeChance=iFreq;
}

private func WindDirection()
{
	var iWind = GetWind();

	if(iWind >= 7) SetXDir(Random(355),1000);
	if(iWind <= -7) SetXDir(-Random(355),1000);
	if(iWind < 6 && iWind > -6) SetXDir();
}

func Definition(def) {
	SetProperty("ActMap", {
Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	X = 0,
	Y = 0,
	Wdt = 512,
	Hgt = 350,
	Length = 16,
	Delay = 0,
	NextAction = "Fly",
	TurnAction = "Turn",
},
}, def);
}
local Name = "Cloud";
