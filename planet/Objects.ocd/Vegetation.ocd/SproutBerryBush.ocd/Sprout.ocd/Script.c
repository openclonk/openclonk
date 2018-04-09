#include Library_Crop

local Name = "$Name$";
local Description = "$Description$";
local Plane = 300;

local grow_anim;
local attached_berry;
local attached_flower;
local leaves;

local ActMap = {
Attach = {
	Prototype = Action,
	Name = "Attach",
	Procedure = DFA_ATTACH,
	Directions = 1,
	Length = 1,
	Delay = 0,
	FacetBase=1,
}
};
local BlastIncinerate = 1;
local ContactIncinerate = 2;

// can be harvested?
func IsCrop(){ return true; }

// berries can simply be plucked
private func SickleHarvesting()
{
	return false;
}

public func IsHarvestable()
{
	return !!Contents();
}

public func Harvest(object clonk)
{
	var b = Contents();
	if(!b) return Die();
	b->Exit();
	clonk->Collect(b);
	
	if(attached_berry != -1)
	{
		StopAnimation(attached_berry);
		attached_berry = -1;
	}
	
	Die();
	return true;
}

func Initialize()
{
	attached_berry = -1;
	attached_flower = -1;
	leaves = [];
}

// init without growing animation
func InitGrown(object bush)
{
	SetR(RandomX(-45, 45));
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(2000), Trans_Rotate(RandomX(0,359),0,1,0)));
	
	PlayRandomGrowAnimation(9,10);
	
	AddLeaves(true);
	// the object wants to know when it's fully grown
	ScheduleCall(this, "FullyGrown", 2, 0);
	
	SetAction("Attach", bush);
	return true;
}

func AddLeaves(bool fullyGrown)
{
	// add two leaves
	for(var i = 1; i <= 2; ++i)
	{
		var rtrans = Trans_Rotate(Random(360), 1, 0, 0);
		var matrix = Trans_Mul(Trans_Scale(1, 1, 1), rtrans);
		if(fullyGrown)
			matrix = rtrans;
		leaves[i] = AttachMesh(SproutBerryBush_Leaf, Format("Leaf%d", i), "Main", matrix);
		
		var start_size = 0;
		if(fullyGrown) start_size = 1500;
		AddEffect("GrowLeaf", this, 1, 1, this, nil, i, rtrans, start_size);
	}
}

func PlayRandomGrowAnimation(int start, int max)
{
	var growActions = 2;
	grow_anim = Format("Grow%d", Random(growActions)+1);
	var len = GetAnimationLength(grow_anim);
	PlayAnimation(grow_anim, 1, Anim_Linear(start, 0, len, max,  ANIM_Hold)); 
}

func Init(object bush)
{
	// properties
	var max_grow_time = 36 * RandomX(20, 60);
	
	// random rotation
	SetR(RandomX(-45, 45));
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(2000), Trans_Rotate(RandomX(0,359),0,1,0)));

	PlayRandomGrowAnimation(0, max_grow_time);
	
	// actually grow
	SetObjDrawTransform();
	var steps = 1000 / max_grow_time;
	AddEffect("VirtGrow", this, 1, 1, this, nil, Max(1, steps));
	
	// add two leaves
	AddLeaves(false);
	
	// the object wants to know when it's fully grown
	ScheduleCall(this, "FullyGrown", max_grow_time, 0);
	
	SetAction("Attach", bush);
	return true;
}

func FxVirtGrowStart(target, effect, temp, p1)
{
	if(temp) return;
	effect.step = p1;
	effect.size = 1;
}

func FxVirtGrowTimer(target, effect, time)
{
	effect.size += effect.step;
	if(effect.size >= 1000)
		effect.size = 1000;
		
	SetObjDrawTransform(effect.size, 0, 0, 0, effect.size, (1000-effect.size)/4);
	
	if(effect.size == 1000)
		return -1;
	return 1;
}

func FxGrowLeafStart(target, effect, temp, p1, p2, p3)
{
	if(temp) return;
	effect.leaf = p1;
	effect.rtrans = p2;
	effect.size = p3;
	effect.leaf_size = RandomX(1750, 2000);
}

func FxGrowLeafTimer(target, effect, time)
{
	effect.size += 3;
	if(effect.size >= effect.leaf_size)
		effect.size = effect.leaf_size;
		
	SetAttachTransform(leaves[effect.leaf], Trans_Mul(Trans_Scale(effect.size, effect.size, effect.size), effect.rtrans));
	if(effect.size == effect.leaf_size)
		return -1;
	return 1;
}

// we now can produce berries!
func FullyGrown()
{
	AddEffect("LifeTimer", this, 1, 30+Random(10), this);
	if(GetActionTarget())
		GetActionTarget()->SproutFullyGrown(this);
}

func FxLifeTimerStart()
{

}

func FxLifeTimerTimer(target, effect, time)
{
	if(!Contents())
	{
		var t = GetActionTarget();
		if(!t) return;
		
		if(!Random(10))
		{
			if(attached_flower != -1)
			{
				if(!GetEffect("FlowerTime", this))
					CreateBerry();
			}
		}
		if((!Random(3)) && t->IsFlowerTime())
		{
			if(attached_flower == -1)
				CreateFlower();
		}
	}
}

func Damage()
{
	// splatter
	if(grow_anim)
		CreateParticle("Dust", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(20, 60), Particles_Material(RGB(50, 50, 255)), 30);
		
	// ouch!
	Die(false);
}

public func IsProjectileTarget(object projectile, object shooter)
{
	if(Random(20))
		return false;
	return !projectile || ObjectDistance(this, projectile) < 15;
}

func AttachTargetLost()
{
	return Die();
}

func Retract()
{
	// die of natural cause
	Die(true);
}

func Incineration()
{
	Extinguish();
	if(!grow_anim) return;
	Die(false);
	SetClrModulation(RGB(50, 50, 50));
}

func Die(bool natural_cause)
{
	if(!grow_anim) return; // already dead
		
	// new property for the final Remove callback
	this.isDead = !natural_cause;	
	
	var time = 20 + Random(20);
	var len = GetAnimationLength(grow_anim);
	PlayAnimation(grow_anim, 6, Anim_Linear(len, len, 0, time,  ANIM_Hold)); 
	AddEffect("QuickFade", this, 1, 1, this, nil, time/2);
	
	if(this.isDead)
	{
		var td = time;
		var tick = SproutBerryBush_water_per_sprout / time;
		if(tick <= 0)
		{
			tick = 1;
			td = SproutBerryBush_water_per_sprout;
		}
		ScheduleCall(this, "Drip", 1, td, tick);
	}
	else
	{
		// give a bit of water back to the bush
		var t = GetActionTarget();
		if(t)
		{
			t.saved_water += SproutBerryBush_water_per_sprout / 3;
		}
	}
	ScheduleCall(this, "Remove", time);
	grow_anim = 0;
	return;
}

func Drip(int tick)
{
	var y = 0;
	while(GBackSolid(0, y) && (y > -5)) --y;
	var water = Material("Water");
	while(tick-- > 0)
		InsertMaterial(water, 0, y, RandomX(-4, 4), RandomX(-4,4));
}

func IsFullyGrown()
{
	return GetEffect("LifeTimer", this);
}

func Remove()
{
	var t = GetActionTarget();
	if (t)
	{
		// not natural cause?
		if(this.isDead)
		{
			// that hurt!
			t.sprout_evolve_counter -= 2;
		}
		else ++t.sprout_evolve_counter;
		
		t->LoseSprout(this);
	}
	if (Contents()) // TODO: Is this needed at all? Contents are removed on destruction.
		t = Contents()->RemoveObject();
	return RemoveObject();
}

func CreateBerry()
{
	if(attached_berry != -1) return;
	var berry = CreateContents(Sproutberry);
	attached_berry = AttachMesh(berry, "Head", "Main", nil, nil);
	SetAttachTransform(attached_berry, Trans_Scale(1, 1, 1));
	AddEffect("GrowAttachedBerry", this, 1, 2, this);
	
	// don't need the flower anymore
	if(attached_flower != -1)
		AddEffect("ShrinkAttachedFlower", this, 1, 2, this);
	
	// if not plucked, will eventually remove the berry
	AddEffect("RetractBerry", this, 1, 35 * 15 * ((4 * 3) + Random(12)), this); 
}

func FxRetractBerryTimer(target, effect, time)
{
	var t = GetActionTarget();
	if(!t) return;
	
	// do not remove the bush with this action!
	if(t.sprout_count == 1)
	{
		effect.Interval = 35 * 20;
		return;
	}
	Retract();
	return -1;
}

func CreateFlower()
{
	if(attached_flower != -1) return;
	
	// needs some water for this
	var t = GetActionTarget();
	if(!t) return;
	if(t.saved_water < SproutBerryBush_water_per_berry) return;
	t.saved_water -= SproutBerryBush_water_per_berry;
	
	// send the happy message
	for(var obj in FindObjects(Find_Distance(200), Find_ID(SproutBerryBush), Find_Exclude(t)))
		obj->StartSeason(35 * 3);
	
	attached_flower = AttachMesh(SproutBerryBush_Flower, "Head", "Main", nil, nil);
	SetAttachTransform(attached_flower, Trans_Scale(1, 1, 1));
	AddEffect("GrowAttachedFlower", this, 1, 2, this);
	AddEffect("FlowerTime", this, 1, 35 * (50 + Random(10)), this);
}

func FxGrowAttachedBerryStart(target, effect, temp)
{
	if(temp) return;
	effect.size = 1;
}

func FxGrowAttachedBerryTimer(target, effect, time)
{
	if(attached_berry == -1) return -1;
	
	effect.size += 20;
	if(effect.size >= 750)
		effect.size = 750;
		
	SetAttachTransform(attached_berry, Trans_Scale(effect.size, effect.size, effect.size));
	
	if(effect.size == 750)
		return -1;
	return 1;
}

static const SproutBerryBush_Flower_BlossomSize = 1000;

func FxGrowAttachedFlowerStart(target, effect, temp)
{
	if(temp) return;
	effect.size = 1;
}

func FxGrowAttachedFlowerTimer(target, effect, time)
{
	if(attached_flower == -1) return -1;
	
	effect.size += 20;
	if(effect.size >= SproutBerryBush_Flower_BlossomSize)
		effect.size = SproutBerryBush_Flower_BlossomSize;
		
	SetAttachTransform(attached_flower, Trans_Scale(effect.size, effect.size, effect.size));
	
	if(effect.size == SproutBerryBush_Flower_BlossomSize)
		return -1;
	return 1;
}

func FxShrinkAttachedFlowerStart(target, effect, temp)
{
	if(temp) return;
	effect.size = SproutBerryBush_Flower_BlossomSize;
}

func FxShrinkAttachedFlowerTimer(target, effect, time)
{
	if(attached_flower == -1) return -1;
	
	effect.size -= 20;
	if(effect.size <= 0)
		effect.size = 0;
		
	SetAttachTransform(attached_flower, Trans_Scale(effect.size, effect.size, effect.size));
	
	if(effect.size == 0)
	{
		DetachMesh(attached_flower);
		attached_flower = -1;
		return -1;
	}
	return 1;
}

func Ejection(obj)
{
	if(attached_berry != -1)
	{
		DetachMesh(attached_berry);
		attached_berry = -1;
	}
}

func FxQuickFadeStart(target, effect, temp, p1)
{
	if(temp) return;
	effect.t = p1;
	effect.alpha = 255;
	effect.step = 255 / effect.t;
}

func FxQuickFadeTimer(target, effect, time)
{
	if(time < effect.t) return;
	effect.alpha -= effect.step;
	SetClrModulation(RGBa(255, 255, 255, effect.alpha));
	if(effect.alpha <= 0) return -1;
	return 1;
}

// Only save main bush object
func SaveScenarioObject() { return false; }
