/*-- Vine --*/

static const MOSS_MINDIST = 17;
static const MOSS_MAXDIST = 20;
static const MOSS_WATERMAXDIST = 17;
static const MOSS_MINBURIED = 50;


local size;
local maxsize;
local waterpos;
local buriedtime;

func Initialize()
{
	this.Plane=301+Random(230);
	SetPosition(GetX(),GetY()+10);
	var graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	size = 1;
	buriedtime = 0;
	waterpos = [0,0];
	SetObjDrawTransform(10,0,0,0,10);
	maxsize=150+Random(30);
	SetR(Random(360));
	SetClrModulation(RGBa(235+Random(20),235+Random(20),235+Random(20),255-Random(30)));
	AddEffect("MossGrow", this, 100, 20, this, this.ID);

}

func IsProjectileTarget(weapon){return weapon->GetID() == Sword;}
public func CanBeHitByShockwaves() { return true; }
public func Incineration()
{
	Destroy();
	return;
}

public func Damage()
{
	if (GetDamage() > (size/4)) Destroy();
}

private func Destroy()
{
	if(Random(maxsize+35)<size) CreateObject(Moss,0,0,-1);
	for(var r=0; r<(size); r++)
	{
		var i=Random(360);
		var d=Random(size/5);
		CreateParticle("Lichen",Sin(i,d),-Cos(i,d),RandomX(-3,3),RandomX(-3,3),10+Random(20),RGB(200+Random(50),200+Random(50),200+Random(50)),nil,Random(2));
	}
	RemoveObject();
}



protected func Replicate()
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
	for(var i=2+Random(5); i>0; i--)
	if(ExtractLiquid(waterpos[0],waterpos[1]) != Material("Water")) return -1;
	CreateObject(Moss_Lichen,x,y,-1);
	buriedtime = -Random(MOSS_MINBURIED);
 	
}


private func FxMossGrowTimer(target, effect, time)
{
	if(GetMaterial() != Material("Earth") && GetMaterial() != Material("Tunnel"))
	{	
		Destroy();
		return 1;
	}
	if(size < maxsize)
		size++;
	SetObjDrawTransform(10*size+1,0,0,0,10*size+1 );

	if(GetMaterial(waterpos[0],waterpos[1]) != Material("Water"))
	{
		SearchWater();
		buriedtime = Min(buriedtime+1, MOSS_MINBURIED / 3 * 2);
	}
	else
	{
		buriedtime++;
		if(buriedtime>MOSS_MINBURIED)
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

local BlastIncinerate = 1;




