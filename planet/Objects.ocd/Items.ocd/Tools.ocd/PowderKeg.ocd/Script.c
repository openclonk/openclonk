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
	CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-20, 10), PV_Random(10, 40), Particles_Glimmer(), 6);
	if (timer > TimeToExplode)
		Explode(GetExplosionStrength());
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
local NoBurnDecay = 1;
local ContactIncinerate = 2;
local Components = {Barrel = 1, Coal = 1};
