/*-- Item Spark --*/

/*
	global func StartItemSparks(int rate, bool mirror)
		starts the spawning of sparks.
		rate is the average frequency and ranges from 0 to 1000.
		if mirror is true, the sparks are mirrored along LandscapeWidth()/2
	
	global func StopItemSparks()
		self-explanatory
		
	scenario callbacks:
	 func GetSparkItem(int x, int y)
		returns the item ID one spark should spawn.
		called at the creation of the sparks (x=nil, y=nil) for mirrored sparks and on Hit() again, if it returned nil before
	
	 func GetSparkRange()
	 	returns the area where the sparks spawn as an array, standard equals return [0, LandscapeWidth()]
	 
	 func SetupSparkItem(object obj)
	 	called after an item is spawned. Can be used to put arrows into bows, for example


*/

local toSpawn;


global func StartItemSparks(int rate, bool mirror)
{
	AddEffect("GlobalItemSparks", nil, 1, 5, nil, nil, rate, mirror);
}

global func StopItemSparks()
{
	RemoveEffect("GlobalItemSparks");
}

global func FxGlobalItemSparksStart(_, effect, temp, rate, mir)
{
	if(temp) return;
	effect.rate=rate;
	effect.mirror=mir;
}

global func FxGlobalItemSparksTimer(_, effect, time)
{
	if(Random(1000) >= effect.rate) return;
	var range=GameCall("GetSparkRange");
	var max, start;
	if(range != nil)
	{
		start=range[0];
		max=range[1]-start;
		if(effect.mirror) if(max>LandscapeWidth()) max=LandscapeWidth();
	}
	else
	{
		max=LandscapeWidth();
		start=0;
	}

	if(effect.mirror){ max/=2; if(Random(2)) start=LandscapeWidth()/2;}
	
	var x=Random(max)+start;
	var failsafe=0;
	while(failsafe++ < 100 && GBackSolid(x, 1)) x=Random(max)+start;
	if(failsafe >= 100) return;
	
	// mirrors requires free sky
	var what=GameCall("GetSparkItem", nil, nil);
	var count=0;
	if(effect.mirror)
	{
		if(x > LandscapeWidth()/2)
		{
			x-=LandscapeWidth()/2;
			x=LandscapeWidth()/2-x;
		}
		count=2;
		
		var s=CreateObject(ItemSpark, LandscapeWidth()-x, 1, NO_OWNER);
		s.toSpawn=what;
	}
	
	var s=CreateObject(ItemSpark, x, 1, NO_OWNER);
	s.toSpawn=what;
}

protected func Initialize()
{
	AddEffect("Sparkle", this, 1, 2, this);
	AddEffect("CheckStuck", this, 1, 30);
}

func FxCheckStuckTimer(_, effect)
{
	if(!GBackSolid(0, 1)) return 1;
	DoSpawn();
	return -1;
}

func FxSparkleTimer(_, effect)
{
	var x=RandomX(-4, 4);
	//var p="ItemSpark";
	//if(Random(2)) p="ItemSparkA";
	//CreateParticle(p, x, (4-Abs(x))/2, -x/2, -3, 40, RGBa(50,50,150+Random(100), 200), nil); 
	CreateParticle("ItemSpark", 0, 0, 0, GetYDir(), 40, RGBa(50,50,150+Random(100), 255), this, true); 
	CreateParticle("ItemSparkA", 0, 0, 0, GetYDir(), 20, RGBa(50,50,200+Random(50), 255), this, false); 
	
	if(!Random(36))
	for(var i=0;i<3;++i)
	{
		var e=AddEffect("SparkleDeath", nil, 5, 1, nil, GetID());
		e.x=GetX();
		e.y=GetY();
		e.velX=RandomX(-10,10);
		e.velY=RandomX(-2,2);;
		e.size=40+Random(10);
		e.from=this;
		e.vAcc=1;
	}
}

func FxSparkleDeathStart(_, effect, temp, x, y, velX, velY, size)
{
	if(temp) return;
	effect.target=_;
	effect.xT=0;
	effect.yT=0;
}

func FxSparkleDeathTimer(_, effect, effect_time)
{
	if(effect.size <= 5) return -1;
	
	if(GBackSolid(AbsX(effect.x), AbsY(effect.y)))
	{
		effect.velX*=-1;
		effect.velY*=-1;
	}
	CreateParticle("ItemSpark", AbsX(effect.x), AbsY(effect.y), 0, 0, effect.size, RGBa(50,50,150, 175), effect.from, true);
	CreateParticle("ItemSparkA", AbsX(effect.x), AbsY(effect.y), 0, 0, effect.size/2, RGBa(50,50,255, 175), effect.from, false);
	
	effect.xT+=effect.velX;
	effect.yT+=effect.velY;
	effect.velY+=effect.vAcc;
	var f=0;
	while(Abs(effect.xT) > 10){if(effect.xT > 0) f=1; else f=-1; effect.x+=f; effect.xT-=f*10;}
	while(Abs(effect.yT) > 10){if(effect.yT > 0) f=1; else f=-1; effect.y+=f; effect.yT-=f*10;}
	
	effect.size-=2;
}

func DoSpawn()
{
	if(toSpawn == nil)
		toSpawn=GameCall("GetSparkItem", GetX(), GetY());
	var o=CreateObject(toSpawn, 0, 0, NO_OWNER);
	o->SetYDir(-1);
	GameCall("SetupSparkItem", o);
	
	for(var cnt=0;cnt<5;++cnt) 
	{
		var effect=AddEffect("SparkleDeath", nil, 5, 1, nil, GetID());
		effect.x=GetX();
		effect.y=GetY();
		effect.velX=RandomX(-10,10);
		effect.velY=RandomX(-50,-30);
		effect.size=25+Random(10);
		effect.from=this;
		effect.vAcc=5;
	}
	AddEffect("Off", this, 1, 30*4, this);
	RemoveEffect("Sparkle", this);
	this.Plane=501;
}

func FxOffStop(_, effect, reason, temp)
{
	if(temp) return;
	if(this) RemoveObject();
}

func Hit()
{
	if(GetEffect("Off", this)) return SetSpeed();
	DoSpawn();
}