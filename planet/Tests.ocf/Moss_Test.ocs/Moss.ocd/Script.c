/*-- Moss --*/


local wetness;
local graphic;
func Initialize()
{
	graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	else SetGraphics();
	wetness = MOSS_MAXWETNESS;
	AddEffect("MossMoisture",this,100,36,this,this.ID);
}


private func FxMossMoistureTimer(target, effect, time)
{
	if(GetMaterial() == Material("Water"))
	{
		if(wetness < MOSS_MAXWETNESS)
		{	
			wetness = MOSS_MAXWETNESS;
			if(graphic)
			SetGraphics(Format("%d",graphic));
			SetGraphics();
		}
		
	}
	
	else if(!Contained() && !GBackSolid() && !GBackLiquid())
		if(wetness)
		{
			wetness--;
			if(wetness == 0)
			{
				if(graphic)
				SetGraphics(Format("%dDry",graphic));
				else SetGraphics("Dry");
			}
		}		 
}


local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

