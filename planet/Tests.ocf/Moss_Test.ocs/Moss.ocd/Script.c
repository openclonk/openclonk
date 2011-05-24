/*-- Moss --*/

static const MOSS_MAXWETNESS = 30;
static const MOSS_LICHENDELAY = 30;
local wetness;
local graphic;
local lastpos;
local still;
func Initialize()
{
	graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	else SetGraphics();
	wetness = MOSS_MAXWETNESS;
	lastpos = CreateArray();
	AddEffect("MossMoisture",this,100,36,this,this.ID);
	still=0;
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
		if([GetX(),GetY()]==lastpos)
			still++;
		else 
			still=0;
		if(still>MOSS_LICHENDELAY)
			TryToLichen();
		lastpos=[GetX(),GetY()];
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


protected func TryToLichen()
{
	
	var x = RandomX(-MOSS_MAXDIST,MOSS_MAXDIST);
	var y = RandomX(-MOSS_MAXDIST,MOSS_MAXDIST);
	var i = 0;
	var good=false;
	while(i<10)
	{
		if(GetMaterial(x,y)!=Material("Earth") && GetMaterial(x,y)!=Material("Tunnel"))
		{
			i++;
			continue;
		}	
		if(FindObject(Find_ID(Moss_Lichen),Find_Distance(MOSS_MINDIST,x,y)))
		{
			i++;
			continue;
		}
		if(Distance(0,0,x,y)>MOSS_MAXDIST)
		{
			i++;
			continue;
		}
		good = true;
		break;
	}
	if(!good) return ;
	
	CreateObject(Moss_Lichen,x,y,-1);
	still = -MOSS_LICHENDELAY-Random(MOSS_LICHENDELAY);
 	
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

