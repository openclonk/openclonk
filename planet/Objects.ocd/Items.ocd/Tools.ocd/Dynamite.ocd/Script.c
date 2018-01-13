/**
	Dynamite
	A volatile tool that can be pressed into wallsfor accurate mining, burning a short fuse before exploding.
	
	@author: Newton
*/

/*-- Engine Callbacks --*/

// time in frames until explosion
local FuseTime = 140;

func Hit()
{
	Sound("Hits::GeneralHit?");
}

func Incineration(int caused_by)
{
	Extinguish();
	Fuse();
	SetController(caused_by);
}

public func Entrance(object new_container)
{
	// Collection back into dynamite box: Kill any connected fuses
	if (new_container && new_container->~IsDynamiteBox())
	{
		var fuses = FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this));
		if (GetLength(fuses) >= 2)
		{
			// Two fuses? Then bridge over this stick
			var t1 = fuses[0]->GetConnectedItem(this);
			var t2 = fuses[1]->GetConnectedItem(this);
			fuses[0]->Connect(t1, t2);
			fuses[0] = nil;
		}
		for (var fuse in fuses) if (fuse) fuse->RemoveObject();
	}
}

/*-- Callbacks --*/

public func OnCannonShot(object cannon)
{
	Fuse();
}

// Drop fusing dynamite on death to prevent explosion directly after respawn
public func IsDroppedOnDeath(object clonk)
{
	return (GetAction() == "Fuse");
}

public func IsFusing()
{
	return GetAction() == "Fuse";
}

// Called by the Dynamite box
public func SetReady()
{
	SetAction("Ready");
}
// Called by the Dynamite box
public func SetFuse()
{
	SetAction("Fuse");
	// Object can't be collected anymore when it fuses.
	this.Collectible = false;
}

public func Reset()
{
	SetAction("Idle");
	// Object can be collected again.
	this.Collectible = true;
}

public func OnFuseFinished(object fuse)
{
	SetController(fuse->GetController());
	DoExplode();
}

public func IsInfiniteStackCount()
{
	return false;
}

public func IsGrenadeLauncherAmmo() { return true; }

/*-- Usage --*/

public func ControlUse(object clonk, int x, int y)
{
	return ControlPlace(clonk, x, y, GetLength(FindFuses()));
}

private func FindFuses()
{
	return FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this));
}

public func ControlPlace(object clonk, int x, int y, bool box)
{
	// if already activated, nothing (so, throw)
	if(GetAction() == "Fuse" || box)
	{
		if(Place(clonk,x,y,box))
			return true;
		// if placed with the box, we are more tolerant where the
		// user clicks and search for other positions too
		else if(box)
		{

			// get rough direction (left, right, up down)
			var angle = (Angle(0,0,x,y)+45)/90*90;

			var plusminus = -1;
			// first check if it is possible to place the dynamite
			// in roughly the same direction as he clicked, then
			// in each left or right of that direction and then
			// in the opposite direction.
			for(var i=0; i<=3; ++i)
			{

				angle += plusminus * i * 90;
				x = Sin(angle, 300);
				y = -Cos(angle, 300);

				if(Place(clonk,x,y,box))
					return true;

				plusminus *= -1;
			}
			
		}
	}
	else
	{
		Fuse();
		return true;
	}
	return false;
}

public func Fuse()
{
	if (GetAction() != "Fuse")
	{
		if (!GetLength(FindFuses())) 
			Sound("Fire::Fuse");
		SetAction("Fuse");
		// Object can't be collected anymore when it fuses.
		this.Collectible = false;	
	}
}

func Place(object clonk, int x, int y, bool box)
{
	var angle = Angle(0,0,x,y);
	var pos = GetWall(angle);
	if(pos)
	{
		if(box) SetReady();
		
		// put into ...
		Sound("Objects::Connect");
		Exit(pos[0], pos[1], Angle(pos[0],pos[1]));
		SetPosition(clonk->GetX()+pos[0], clonk->GetY()+pos[1]);
		return true;
	}
	return false;
}

// returns true if there is a wall in direction in which "clonk" looks
// and puts the offset to the wall into "xo, yo" - looking from the clonk
func GetWall(int angle)
{
	var dist = 12;
	for (var dist = 12; dist < 18; dist++)
	{
		var x = Sin(angle, dist);
		var y = -Cos(angle, dist);
		if (GBackSolid(x, y))
			return [Sin(angle, dist-5), -Cos(angle, dist-5)];
	}
	return false;
}

func Fusing()
{
	var x = Sin(GetR(), 5);
	var y = -Cos(GetR(), 5);

	if (Contained()!=nil)
	{
		//If the dynamite is held, sparks come from clonk's center.
		x = y = 0;
	}

	// Effect: fire particles.
	if (GetActTime() < FuseTime - 20)
		CreateParticle("Fire", x, y, PV_Random(x - 5, x + 5), PV_Random(y - 15, y + 5), PV_Random(10, 40), Particles_Glimmer(), 3);
	// Explosion: after fusetime is over.
	else if (GetActTime() > FuseTime)
		DoExplode();
	return;
}


public func DoExplode()
{
	// Activate all fuses.
	for (var obj in FindFuses())
		obj->~StartFusing(this);
	Explode(26);
}

public func IsExplosive() { return true; }

/*-- Scenario saving --*/
// Do not save within dynamite box - will be handled by box

public func SaveScenarioObject(props, ...)
{
	var c = Contained();
	if (c && c->~IsDynamiteBox()) return false;
	return inherited(props, ...);
}

/*-- Production --*/

public func IsChemicalProduct() { return true; }

/*-- Properties --*/

local ActMap = {
	Fuse = {
		Prototype = Action,
		Name = "Fuse",
		Procedure = DFA_NONE,
		NextAction = "Fuse",
		Delay = 1,
		Length = 1,
		FacetBase = 1,
		Sound = "Fire::FuseLoop",
		StartCall = "Fusing"
	},
	Ready = {
		Prototype = Action,
		Name = "Ready",
		Procedure = DFA_NONE,
		NextAction = "Ready",
		Delay = 1,
		Length = 1,
		FacetBase = 1,
	}
};
local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local BlastIncinerate = 1;
local ContactIncinerate = 1;
local Components = {Coal = 1, Firestone = 1};