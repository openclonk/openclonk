/**
	Sproutberry
	Fresh from nature's garden.
*/

#include Library_Edible

/*-- Engine Callbacks --*/

public func Construction()
{
	this.MeshTransformation = Trans_Scale(1500, 1500, 1500);
}

func Hit()
{
	Sound("Hits::SoftHit1");
}

// sproutberries are extremely unstable and likely to grow a new plant if not carried
func Departure(object what)
{
	if(!GetEffect("SproutCheck", this))
		AddEffect("SproutCheck", this, 1, 10, this);
}

func SaveScenarioObject(props, ...)
{
	// Do not save berries that are still attached to bushes
	if (Contained())
		if (Contained()->GetID() == SproutBerryBush_Sprout)
			return false;
	return inherited(props, ...);
}

/*-- Sprouting --*/

func FxSproutCheckTimer(target, effect, time)
{
	var c = Contained();
	if(c)
	{
		if(c->GetCategory() & C4D_Living)
		{
			return -1;
		}
	}
	
	// can only create a new bush after some time
	// or faster if burried!
	if(time < 35 * 30) return;
	if(!GBackSolid(0, -1))
		if(time < 35 * 60) return;
	
	// okay, create a bush or just remove
	this.Collectible = 0;
	
	// graphical effect
	AddEffect("Shrink", this, 1, 2, this);
	
	var y = -10;
	for(;y < 10; ++y)
	{
		if(GBackSolid(0, y+1)) break;
	}
	
	// no fitting ground found :/
	if((!GBackSolid(0, y+1)) || GBackSolid(0, y))
	{
		return -1;
	}
	
	// already a bush here?
	var b = FindObject(Find_Distance(20 + Random(10)), Find_ID(SproutBerryBush));
	if (b)
	{
		// extra fertilizer
		b.saved_water += SproutBerryBush_water_per_berry * 2;
		return -1;
	}
	
	// create a bush!
	var bush = CreateObjectAbove(SproutBerryBush, 0, y + 8, NO_OWNER);
	
	// no insta-bush please..
	bush->GrowNormally();
	
	// no water is lost
	bush.saved_water += SproutBerryBush_water_per_berry;
	
	// but make sure the bush is removed if it can not grow sprouts after some time
	AddEffect("BushSuitable", bush, 1, 35 * 60 * 5, nil, GetID());
}

func FxBushSuitableTimer(target, effect, time)
{
	// bush could not grow yet :(
	if(target.sprout_count == 0)
	{
		target->Die();
	}
	
	// everything seems alright!
	return -1;
}

func FxShrinkStart(target, effect, temp)
{
	if(temp) return;
	effect.size = 1000;
	effect.color_sub = 0;
}

func FxShrinkTimer(target, effect, time)
{
	effect.size -= 10;
	
	// if contained we do not need a visual effect before removing..
	if((effect.size <= 0) || Contained())
	{
		RemoveObject();
		return -1;
	}
	
	SetObjDrawTransform(1000, 0, 0, 0, effect.size, 1000 - effect.size);
	
	if(effect.color_sub < 255)
	{
		effect.color_sub = Min(effect.color_sub + 5, 255);
		SetClrModulation(RGB(255 - effect.color_sub / 3, 255 - effect.color_sub / 2, 255 - effect.color_sub));
	}
}

/*-- Display --*/

public func GetCarryMode()
{
	return CARRY_Hand;
}

public func GetCarryBone()
{
	return "Main";
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;