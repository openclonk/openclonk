/*-- Wooden Barrel --*/

#strict 2

local szLiquid;
local iVolume;
local Closed;
local iDrain;

local debug;

protected func Initialize()
{
	iVolume=0;
	Closed=false;
	iDrain=3; //Vertical offset of liquid intake from barrel center

	debug=1;
}

private func Hit()
{
  Sound("WoodHit");
  if(iVolume>=1 && Closed==false) 
  {
	if(GBackLiquid(0,iDrain) && GetMaterial(0,iDrain)!=szLiquid) return 0;
	EmptyBarrel(GetR());
	Sound(" "); //water splash sound should be added when available -Ringwaul
  }
}

private func Check()
{
	//Fills Barrel with specified liquid from if submerged
	if(GetMaterial(0,iDrain)== Material("Water") && Closed==false) FillBarrel("Water");
	if(GetMaterial(0,iDrain)== Material("Acid") && Closed==false) FillBarrel("Acid");

	if(iVolume==0) SetGraphics() && szLiquid=nil;

	//Debug/Testing Purposes
	if(debug == 1) Message("Volume:|%d|Liquid:|%s", this(), iVolume, szLiquid);
}

private func FillBarrel(string szMat)
{
	var iCapacity=300;
	
	if(iVolume>=1 && szMat!=szLiquid) return 0;
	while(iVolume!=iCapacity && GetMaterial(0,iDrain)== Material(szMat)) 
	{
		ExtractLiquid(0,iDrain);
		iVolume=++iVolume;
	}
	szLiquid=szMat;
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
	if(Closed==true) Closed=false && Sound(" "); //Wood scrape sound

	var AimAngle=Angle(GetX(),GetY(), iX, iY);
	EmptyBarrel(AimAngle,50);

	if(Contained()->GetX() > iX) Contained()->SetDir(0);
	if(Contained()->GetX() < iX) Contained()->SetDir(1);
	return 1;
}

public func IsTool() { return 0; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}