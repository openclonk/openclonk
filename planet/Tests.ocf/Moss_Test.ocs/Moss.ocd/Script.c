/*-- Moss --*/

static const MOSS_MINDIST = 17;
static const MOSS_MAXDIST = 20;
static const MOSS_WATERMAXDIST = 15;
static const MOSS_MINBURIED = 30;
static const MOSS_MAXWETNESS = 10;
local wetness;
local buriedtime;
local waterpos;
local vine;
local graphic;
func Initialize()
{
	graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	else SetGraphics();
	wetness = MOSS_MAXWETNESS;
	buriedtime = 0;
	waterpos = [0,0];
	vine = nil;
	AddEffect("MossGrow", this, 100, 36, this, this.ID);
	AddEffect("MossMoisture",this,100,36,this,this.ID);
}

public func Entrance()
{
	if(vine)
		vine->RemoveObject();
}


protected func Replicate()
{
	var x = RandomX(-MOSS_MAXDIST,MOSS_MAXDIST);
	var y = RandomX(-MOSS_MAXDIST,MOSS_MAXDIST);
	var i = 0;
	var good=false;
	while(i<10)
	{
		if(GetMaterial(x,y)!=Material("Earth"))
		{
			i++;
			continue;
		}	
		if(FindObject(Find_ID(Moss),Find_Distance(MOSS_MINDIST,x,y)))
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
	CreateObject(Moss,x,y,-1);
	buriedtime = -Random(MOSS_MINBURIED);
 	
}


func SpreadNoReplication()
{
	for(var m in FindObjects(Find_ID(Moss),Find_Distance(MOSS_WATERMAXDIST)))
		AddEffect("MossReplicationBlock",m,1,MOSS_MINBURIED*10,this,this.ID);
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

private func FxMossGrowTimer(target, effect, time)
{
	if(!wetness) return 1;
	if(GetMaterial() != Material("Earth"))
	{	
		if(buriedtime > (MOSS_MINBURIED/5))
			SpreadNoReplication();
		buriedtime = 0;

		if(vine)
			vine->RemoveObject();
		
		return 1;
	}
	if(buriedtime>2 && !vine && Random(3))
	{
		vine = CreateObject(Moss_Vine,0,20,-1);
		vine.parent=this;
	}
	if(GetMaterial(waterpos[0],waterpos[1]) != Material("Water"))
	{
		SearchWater();
		buriedtime = Min(buriedtime+1, MOSS_MINBURIED / 3 * 2);
	}
	else
	{
		buriedtime++;
		if(buriedtime > MOSS_MINBURIED && !GetEffect("MossReplicationBlock",this))
	 		Replicate();
	}
}

func SearchWater()
{
	for(var i = 0; i < 5; i++)
	{
		waterpos[0] = RandomX(-MOSS_WATERMAXDIST,MOSS_WATERMAXDIST);
		waterpos[1] = RandomX(-MOSS_WATERMAXDIST,MOSS_WATERMAXDIST);
		if(GetMaterial(waterpos[0],waterpos[1]) == Material("Water"))
			break;
	}	
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

