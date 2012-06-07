/*-- Sprout Berry Bush --*/



local Name = "$Name$";
local Description = "$Description$";
local Touchable = 0;
local sprout_count;
local max_sprouts;
local grown_sprouts_count;
local saved_water;
local last_checked_y;
local sprout_evolve_counter;

static const SproutBerryBush_average_flower_time = 300; // in seconds
static const SproutBerryBush_water_per_sprout = 50;
static const SproutBerryBush_water_per_berry = 10;
static const SproutBerryBush_max_sprouts = 8;
static const SproutBerryBush_evolve_steps_per_new_sprout = 2;

func Construction()
{
	SetCon(100);
	_inherited(...);
}

private func Initialize()
{
	sprout_evolve_counter = 0;
	sprout_count = 0;
	max_sprouts = 2;
	ScheduleCall(this, "PostInit", 1);
	
	// no instant berries!
	AddEffect("DontFlower", this, 1, 35 * SproutBerryBush_average_flower_time + RandomX(-400,400), this);
	AddTimer("Sprout", 40);
}

public func IsTree() { return false; }

func GrowNormally()
{
	AddEffect("GrowNormally", this, 1, 2, this);
}

// scenario initialization?
func PostInit()
{
	if(GetEffect("GrowNormally", this)) return;
	QuickSprout();
	// some starting water
	saved_water = (SproutBerryBush_water_per_sprout * 3) / 2;
}

func QuickSprout()
{
	for(var i = 0; i < 3; ++i)
	{
		var sprout = CreateObject(SproutBerryBush_Sprout, 0, 15, GetOwner());
		++sprout_count;
		sprout->InitGrown(this);
	}	
}

func Sprout()
{
	// sprouts need water
	// however - could explode when too much water around
	if(saved_water < SproutBerryBush_water_per_sprout*2)
	{
		// get ground position
		var i = 1;
		for(; i < 10; i += 2)
		{
			if(GBackSolid(0, last_checked_y + i)) break;
		}
		last_checked_y += i - 2;
		if(GBackSolid(0, last_checked_y))
		{
			for(var i = 0; i > -10;--i)
			{
				if(!GBackSolid(0, last_checked_y + i))
				{
					last_checked_y += i;
					break;
				}
			}
			last_checked_y = BoundBy(last_checked_y, -15, 15);
		}
		var amnt = ExtractMaterialAmount(0, last_checked_y, Material("Water"), 10);
		saved_water += amnt;
		
		// can't sprout anyway
		if(saved_water < SproutBerryBush_water_per_sprout)
			return;
	}
	
	if(Random(4)) return;
	
	
	// might start flower time
	if(!GetEffect("DontFlower", this))
	{
		if(!Random(4)) StartSeason();
	}
	
	// don't sprout no more?
	if(sprout_count >= max_sprouts) return;
	// one still growing?
	if(grown_sprouts_count != sprout_count) return;
	
	// still owie?
	if(GetEffect("Hurt", this)) return;
	
	// old bush grows stronger!
	if(sprout_evolve_counter >= SproutBerryBush_evolve_steps_per_new_sprout)
	{
		if(max_sprouts < SproutBerryBush_max_sprouts)
			++ max_sprouts;
		sprout_evolve_counter -= SproutBerryBush_evolve_steps_per_new_sprout;
	}
	
	// create new sprout
	var sprout = CreateObject(SproutBerryBush_Sprout, 0, 15, GetOwner());
	++sprout_count;
	sprout->Init(this);
	saved_water -= SproutBerryBush_water_per_sprout;
}

func StartSeason(int time)
{
	if(time == nil)
		time = 35 * 8;
	// blooming time!
	AddEffect("Flower", this, 1, time, this);
	
	// no new season now!
	AddEffect("DontFlower", this, 1, 35 * SproutBerryBush_average_flower_time + RandomX(-200,200), this);
}

func SproutFullyGrown(object which)
{
	++grown_sprouts_count;
}

func IsFlowerTime()
{
	return GetEffect("Flower", this);
}

func LoseSprout(object which)
{
	--sprout_count;
	if(which->IsFullyGrown())
		--grown_sprouts_count;
		
	// that hurt, we should not sprout immediatly again
	AddEffect("Hurt", this, 1, 35*5);
	
	// awwww
	if(sprout_count <= 0)
		RemoveObject();
}


func Die()
{
	RemoveObject();
}
