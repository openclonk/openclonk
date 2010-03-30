/*-- Metal Barrel --*/

#strict 2
#include WoodenBarrel

private func Hit()
{
	if(iVolume< 50) Sound(" "); //Hollow clonk sound
	if(iVolume> 51) Sound(" "); //Solid clonk sound
	if(iVolume>=1 && Closed==false) 
	{
		if(GBackLiquid(0,iDrain) && GetMaterial(0,iDrain)!=szLiquid) return 0;
		else EmptyBarrel(GetR());
		Sound(" "); //water splash sound should be added when available -Ringwaul
	}
}

private func Check()
{
	if(GetMaterial(0,iDrain)== Material("Acid") && Closed==false) FillBarrel("Acid");
	if(GetMaterial(0,iDrain)== Material("Lava") && Closed==false) FillBarrel("Lava");
	return (_inherited());
}