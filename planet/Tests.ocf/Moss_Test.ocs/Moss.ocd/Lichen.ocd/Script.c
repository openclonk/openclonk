/*-- Vine --*/

static const MOSS_MINDIST = 17;
static const MOSS_MAXDIST = 20;
static const MOSS_WATERMAXDIST = 15;
static const MOSS_MINBURIED = 30;
static const MOSS_MAXWETNESS = 10;

local size;
local maxsize;
local waterpos;
local buriedtime;

func Initialize()
{
	CreateObject(Rock,0,0);
//	var graphic = Random(3);
//	if(graphic)
//		SetGraphics(Format("%d",graphic));
//	size = 1;
//	buriedtime = 0;
//	waterpos = [0,0];
	//SetObjDrawTransform(10,0,0,0,10);
//	maxsize=75+Random(10);
	//SetR(Random(360));
	
//	AddEffect("MossGrow", this, 100, 36, this, this.ID);

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
	buriedtime = -Random(MOSS_MINBURIED);
 	
}


func SpreadNoReplication()
{
	for(var m in FindObjects(Find_ID(Moss),Find_Distance(MOSS_WATERMAXDIST)))
		AddEffect("MossReplicationBlock",m,1,MOSS_MINBURIED*10,this,this.ID);
}

func Destroy() { RemoveObject(); }


private func FxMossGrowTimer(target, effect, time)
{
	if(GetMaterial() != Material("Earth") && GetMaterial() != Material("Tunnel"))
	{	
		Destroy();
		return 1;
	}
	if(size < maxsize)
		size++;
	SetObjDrawTransform(10*size,0,0,0,10*size );

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





