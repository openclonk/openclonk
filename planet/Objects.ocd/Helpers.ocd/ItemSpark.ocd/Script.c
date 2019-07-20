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
		called at the creation of the sparks (x = nil, y = nil) for mirrored sparks and on Hit() again, if it returned nil before
	
	 func GetSparkRange()
	 	returns the area where the sparks spawn as an array, standard equals return [0, LandscapeWidth()]
	 
	 func SetupSparkItem(object obj)
	 	called after an item is spawned. Can be used to put arrows into bows, for example


*/

local Plane = 300;

local toSpawn;

static ItemSpark_particle;
static ItemSpark_particle_additive;

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
	if (temp) return;
	effect.rate = rate;
	effect.mirror = mir;
}

global func FxGlobalItemSparksTimer(_, effect, time)
{
	if (Random(1000) >= effect.rate) return;
	var range = GameCall("GetSparkRange");
	var max, start;
	if (range != nil)
	{
		start = range[0];
		max = range[1]-start;
		if (effect.mirror) if (max>LandscapeWidth()) max = LandscapeWidth();
	}
	else
	{
		max = LandscapeWidth();
		start = 0;
	}

	if (effect.mirror){ max /= 2; if (Random(2)) start = LandscapeWidth()/2;}
	
	var x = Random(max)+start;
	var failsafe = 0;
	while (failsafe++ < 100 && GBackSolid(x, 1)) x = Random(max)+start;
	if (failsafe >= 100) return;
	
	// mirrors requires free sky
	var what = GameCall("GetSparkItem", nil, nil);
	var count = 0;
	if (effect.mirror)
	{
		if (x > LandscapeWidth()/2)
		{
			x -= LandscapeWidth()/2;
			x = LandscapeWidth()/2-x;
		}
		count = 2;
		
		var s = CreateObjectAbove(ItemSpark, LandscapeWidth()-x, 1, NO_OWNER);
		s.toSpawn = what;
	}
	
	var s = CreateObjectAbove(ItemSpark, x, 1, NO_OWNER);
	s.toSpawn = what;
}

global func FxGlobalItemSparksSaveScen(_, effect, props)
{
	props->Add("Sparks", "StartItemSparks(%d, %v)", effect.rate, effect.mirror);
	return true;
}

protected func Initialize()
{
	if (!ItemSpark_particle)
	{
		ItemSpark_particle =
		{
			Size = PV_Linear(15, 0),
			Rotation = PV_Step(1, PV_Random(0, 90)),
			Alpha = PV_Linear(0, 255),
			R = PV_Random(0, 50),
			G = PV_Random(0, 50),
			B = PV_Random(200, 255),
			Attach = ATTACH_Back,
			CollisionVertex = 500,
			OnCollision = PC_Die()
		};
		
		ItemSpark_particle_additive =
		{
			Prototype = ItemSpark_particle,
			BlitMode = GFX_BLIT_Additive,
			Attach = ATTACH_Front
		};
	}
	AddEffect("Sparkle", this, 1, 2, this);
	AddEffect("CheckStuck", this, 1, 30);
}

func FxCheckStuckTimer(_, effect)
{
	if (!GBackSolid(0, 1)) return 1;
	DoSpawn();
	return -1;
}

func FxSparkleTimer(_, effect)
{
	CreateParticle("ItemSpark", PV_Random(0, GetXDir()/10), PV_Random(0, GetYDir()/10), GetXDir(), GetYDir(), 36, ItemSpark_particle, 5); 
	CreateParticle("ItemSpark", PV_Random(0, GetXDir()/10), PV_Random(0, GetYDir()/10), GetXDir(), GetYDir(), 20, ItemSpark_particle_additive, 5); 
	
	if (!Random(36))
	for (var i = 0;i<3;++i)
	{
		var e = AddEffect("SparkleDeath", nil, 5, 1, nil, GetID());
		e.x = GetX();
		e.y = GetY();
		e.velX = RandomX(-10, 10);
		e.velY = RandomX(-2, 2);;
		e.size = 40 + Random(10);
		e.from = this;
		e.vAcc = 1;
	}
}

func FxSparkleDeathStart(_, effect, temp, x, y, velX, velY, size)
{
	if (temp) return;
	effect.target = _;
	effect.xT = 0;
	effect.yT = 0;
}

func FxSparkleDeathTimer(_, effect, effect_time)
{
	if (effect.size <= 5) return -1;
	
	if (GBackSolid(AbsX(effect.x), AbsY(effect.y)))
	{
		effect.velX*=-1;
		effect.velY*=-1;
	}
	
	effect.from->CreateParticle("ItemSpark", AbsX(effect.x), AbsY(effect.y), 0, 0, 18, ItemSpark_particle, 1); 
	effect.from->CreateParticle("ItemSpark", AbsX(effect.x), AbsY(effect.y), 0, 0, 10, ItemSpark_particle_additive, 1); 
	
	effect.xT += effect.velX;
	effect.yT += effect.velY;
	effect.velY += effect.vAcc;
	var f = 0;
	while (Abs(effect.xT) > 10){if (effect.xT > 0) f = 1; else f=-1; effect.x += f; effect.xT -= f*10;}
	while (Abs(effect.yT) > 10){if (effect.yT > 0) f = 1; else f=-1; effect.y += f; effect.yT -= f*10;}
	
	effect.size -= 2;
}

func DoSpawn()
{
	if (toSpawn == nil)
		toSpawn = GameCall("GetSparkItem", GetX(), GetY());
	var item = CreateObjectAbove(toSpawn, 0, 0, NO_OWNER);
	
	if (item)
	{
		item->SetYDir(-1);
		GameCall("SetupSparkItem", item);
	}
	
	for (var cnt = 0;cnt<5;++cnt) 
	{
		var effect = AddEffect("SparkleDeath", nil, 5, 1, nil, GetID());
		effect.x = GetX();
		effect.y = GetY();
		effect.velX = RandomX(-10, 10);
		effect.velY = RandomX(-50,-30);
		effect.size = 25 + Random(10);
		effect.from = this;
		effect.vAcc = 5;
	}
	AddEffect("Off", this, 1, 30*4, this);
	RemoveEffect("Sparkle", this);
	this.Plane = 501;
}

func FxOffStop(_, effect, reason, temp)
{
	if (temp) return;
	if (this) RemoveObject();
}

func Hit()
{
	if (GetEffect("Off", this)) return SetSpeed();
	DoSpawn();
}

func SaveScenarioObject() { return false; }
