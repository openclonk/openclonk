/*--
	Moss
	Authors: Mimmo, Clonkonaut
--*/

#include Library_Flammable

static const MOSS_MAXWETNESS = 30; // Moisture the moss can achieve. Dries out within 36*MOSS_MAXWETNESS frames
static const MOSS_MAXDIST = 50; // Used for various distance checks
local wetness;
local graphic;
local lastpos;

func Initialize()
{
	graphic = Random(3);
	if (graphic)
		SetGraphics(Format("%d",graphic));

	wetness = MOSS_MAXWETNESS;
	lastpos = CreateArray();
	AddEffect("MossMoisture",this, 100, 36, this);
}

public func ControlUse(object clonk, int x, int y, bool box)
{
	if (!clonk->~IsWalking()) return true;
	// Search for ground
	x = 0; y = 0;
	if (GBackSemiSolid(x, y)) return true;
	if (GetMaterial(x, y) != Material("Tunnel")) return true;
	var i = 0;
	while (!GBackSolid(x, y) && i < 15) { ++y; ++i; }
	if (!GBackSolid(x, y)) return true;
	if (GetMaterialVal("Soil", "Material", GetMaterial(x, y)) == 1)
	{
		// Plant!
		clonk->DoKneel();
		CreateObjectAbove(Lichen, x, y, clonk->GetOwner());
		RemoveObject();
	}
	else
		clonk->Message("$NoSuitableGround$");

	return true;
}

/*-- Reproduction --*/

private func FxMossMoistureTimer(target, effect, time)
{
	if (GetMaterial() == Material("Water"))
	{
		DoWetness(MOSS_MAXWETNESS);
	}
	else if (!Contained() && !GBackSolid() && !GBackLiquid())
		if (wetness)
		{
			DoWetness(-1);
			// Fire nearby -> dry faster
			if (FindObject(Find_Distance(100), Find_OCF(OCF_OnFire))) DoWetness(-1);

			if ([GetX(),GetY()]==lastpos)
			{
				if (FindNearWater())
					TryToLichen();
			}
			else
				lastpos = [GetX(), GetY()];
		}		 
}

private func DoWetness(int change)
{
	var was_wet = wetness > 0;
	wetness = BoundBy(wetness + change, 0, MOSS_MAXWETNESS);
	var is_wet = wetness > 0;
	
	if (was_wet && !is_wet)
	{
		this.Name = Format("$Dry$ %s", this.Name);
		
		if (graphic)
			SetGraphics(Format("%dDry",graphic));
		else
			SetGraphics("Dry");
	}
	else if (!was_wet && is_wet)
	{
		this.Name = this.Prototype.Name;
		
		if (graphic)
			SetGraphics(Format("%d",graphic));
		else
			SetGraphics();
	}
}

public func CanBeStackedWith(object other)
{
	if ((this.wetness > 0) != (other.wetness > 0)) return false;
	return _inherited(other, ...);
}

protected func TryToLichen()
{
	if (GetMaterial() != Material("Tunnel")) return false;
	var y = 0;
	while (!GBackSolid(0, y) && y < 10) y++;
	if (!GBackSolid(0, y)) return false;
	if (!GetMaterialVal("Soil", "Material", GetMaterial(0, y))) return false;
	if (FindObject(Find_ID(Lichen), Find_Distance(MOSS_MAXDIST))) return false;

	CreateObjectAbove(Lichen, 0, y, NO_OWNER);
	RemoveObject();
	return true;
}

// Moss only grows (on itself) if close to water
private func FindNearWater()
{
	// Take a random hit within the target area
	var y = RandomX(-MOSS_MAXDIST, MOSS_MAXDIST);
	for (var i = 0; i < MOSS_MAXDIST; i += 2)
	{
		if (GetMaterial(i, y) == Material("Water")) return true;
		if (GetMaterial(-i, y) == Material("Water")) return true;
	}
	return false;
}

/*-- Status --*/

public func IsFuel() { return !wetness; }
public func GetFuelAmount(int requested_amount) { return 100; }    // disregard the parameter, because only a complete chunk should be removed 

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Placement = 3;
local BlastIncinerate = 1;
local ContactIncinerate = 1;
local Plane = 470;
