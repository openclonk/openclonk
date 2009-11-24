/*-- Wooden Barrel --*/

#strict 2

local szLiquid;
local iVolume;
local Closed;

local debug;

protected func Initialize()
{
	iVolume=0;
	Closed=false;
	debug=1;
}

private func Hit()
{
  Sound("WoodHit");
  if(iVolume>=1 && Closed==false) 
  {
	EmptyBarrel(GetR());
	Sound(" "); //water splash sound should be added when available -Ringwaul

	//Debug/Testing Purposes
	if(debug == 1) Message("Volume|%d|Liquid|%s", this(), iVolume, szLiquid);
  }
}

private func Check()
{
	if(GetMaterial(0,2)== Material("Water"))
	{
		FillBarrel("Water");
	}
	if(iVolume==0) SetGraphics() && szLiquid=nil;
}

private func FillBarrel(string szMat)
{
	var iCapacity=300;

	if(iVolume>=1 && szMat!=szLiquid) return 0;
	while(iVolume!=iCapacity && GetMaterial(0,2)== Material(szMat)) ExtractLiquid(0,2) && (iVolume=++iVolume);
	szLiquid=szMat;

	//Debug/Testing Purposes
	if(debug == 1) Message("Volume|%d|Liquid|%s", this(), iVolume, szLiquid);
}

private func EmptyBarrel(int iAngle, int iStrength)
{
	if(!iAngle) iAngle=0;
	if(!iStrength) iStrength=30;
	CastPXS(szLiquid, iVolume,iStrength,0,0,iAngle,30);
	iVolume=0;
}

public func ControlUse(object pByClonk, int iX, int iY)
{
	//var AimAngle=Angle(GetX(),GetY(), iX, iY);
	//EmptyBarrel(AimAngle);

	if(Contained()->GetDir() == 0) EmptyBarrel(45, 40);
	if(Contained()->GetDir() == 1) EmptyBarrel(-45, 40);
}

public func IsTool() { return 0; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}