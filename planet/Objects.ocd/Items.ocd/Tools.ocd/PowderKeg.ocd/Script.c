/*--
	Powder Keg
	A barrel filled with black powder.
	
	@author Ringwaul
*/

#include Library_CarryHeavy

local count;
local TimeToExplode = 90; // Number of frames to pass until keg explodes

public func GetCarryTransform(clonk)
{
	if(GetCarrySpecial(clonk))
		return Trans_Translate(1000, -6500, 0);
		
	return Trans_Mul(Trans_Translate(1500, 0, -1500),Trans_Rotate(180,1,0,0));
}
public func GetCarryPhase() { return 900; }

protected func Initialize()
{
	UpdatePicture();
}

protected func Construction()
{
	count = 12;
	var effect = AddEffect("Update",this,1,1,this);
	effect.oldcount = count;
}

local MaxContentsCount = 12;

public func GetPowderCount()
{
	return count;
}

public func SetPowderCount(newcount)
{
	count = newcount;
	return;
}

public func DoPowderCount(int change)
{
	if (count == nil)
		return;
	return SetPowderCount(GetPowderCount() + change);
}

public func FxUpdateTimer(object target, effect, int timer)
{
	if(count != effect.oldcount)
		UpdatePicture();
	if(count == 0)
	{
		ChangeDef(Barrel);
		return -1;
	}
	effect.oldcount = count;
	return 1;
}

public func UpdatePicture()
{
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;
	
	if (count == nil)
	{
		SetGraphics(nil, nil, 11);
		SetGraphics("Inf", Icon_Number, 12, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
		return;
	}
	
	var one = count % 10;
	var ten = (count / 10) % 10;
	if (ten > 0)
	{
		SetGraphics(Format("%d", ten), Icon_Number, 11, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - spacing, 0, s, yoffs, 11);
	}
	else
		SetGraphics(nil, nil, 11);
		
	SetGraphics(Format("%d", one), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
}

public func Incineration(int caused_by)
{
	SetController(caused_by);
	AddEffect("Fuse", this, 1, 1, this);
}

public func FxFuseTimer(object target, effect, int timer)
{
	// Particle effect
	var lifetime = PV_Random(10, 40);
	var amount = 6;
	if (Contained() && Contained()->~IsClonk())
	{
		var prec = 10;
		// Position, is always the same regardless of rotation
		var x = 2, y = -6;
		// Needs a loop for now, because CreateParticleAtBone seems to not use the PV_Random values in the dir array
		for (var i = 0; i < amount; ++i)
		{
			// Positions for array: [-y, 0, -x]
			Contained()->CreateParticleAtBone("Fire", "pos_tool1", [2 - y, 0, -1 - x], [prec * RandomX(-10, 20), 0, prec * RandomX(-10, 10)], lifetime, Particles_Glimmer(), 1);
		}
	}
	else
	{
		var x, y;
		
		if (Contained())
		{
			// Display at the center if contained, because the fuse vertex might not be visible
			x = 0; y = 0; 
		}
		else
		{
			// Display at a vertex, because this is easier than calculating the correct rotated position
			var fuse_vertex = 1;
			x = GetVertex(fuse_vertex, 0);
			y = GetVertex(fuse_vertex, 1);
		}
		CreateParticle("Fire", x, y, PV_Random(-10, 10), PV_Random(-20, 10), lifetime, Particles_Glimmer(), amount);
	}
	// Explosion after timeout
	if (timer > TimeToExplode)
	{
		Explode(GetExplosionStrength());
	}
}

// Powderkeg explosion strength ranges from 17-32.
public func GetExplosionStrength() { return Sqrt(64 * (4 + count)); }

public func IsProjectileTarget()
{
	return true;
}

public func Damage(int change, int cause, int by_player)
{
	Incinerate(100, by_player);
}

public func OnProjectileHit(object projectile)
{
	Incinerate(100, projectile->GetController());
}

func Hit()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	var v = GetPowderCount();
	if (v != 12) props->AddCall("Powder", this, "SetPowderCount", v);
	return true;
}

func IsChemicalProduct() { return true; }
func AlchemyProcessTime() { return 100; }
public func IsExplosive() { return true; }


/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 1;
local NoBurnDecay = true;
local ContactIncinerate = 2;
local Components = {Barrel = 1, Coal = 1};
