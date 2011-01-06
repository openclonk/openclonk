/*
	Dynamite
	Author: Newton

	A volatile tool that can be pressed into walls
	for accurate mining, burning a short fuse before exploding.
*/
	

public func ControlUse(object clonk, int x, int y, bool box)
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

private func Place(object clonk, int x, int y, bool box)
{
	var angle = Angle(0,0,x,y);
	var pos = GetWall(angle);
	if(pos)
	{
		if(box) SetReady();
		
		// put into ...
		Sound("Connect");
		Exit(pos[0], pos[1], Angle(pos[0],pos[1]));
		SetPosition(clonk->GetX()+pos[0], clonk->GetY()+pos[1]);
		return true;
	}
	return false;
}

public func Fuse()
{
	if(GetAction() != "Fuse")
	{
		if(!FindObject(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this))) Sound("Fuse");
		SetAction("Fuse");
	}
}

// returns true if there is a wall in direction in which "clonk" looks
// and puts the offset to the wall into "xo, yo" - looking from the clonk
private func GetWall(angle)
{
	var dist = 12;
	for(var dist = 12; dist < 18; dist++)
	{
		var x = Sin(angle, dist);
		var y = -Cos(angle, dist);
		if(GBackSolid(x, y))
		{
			return [Sin(angle, dist-5), -Cos(angle, dist-5)];
		}
	}
	return false;
}

protected func Hit() { Sound("WoodHit*"); }

protected func Incineration() { Extinguish(); Fuse(); }

protected func RejectEntrance()
{
	return GetAction() == "Fuse" || GetAction() == "Ready";
}

// Controle of the Dynamite box
public func SetReady()
{
	SetAction("Ready");
}
// Controle of the Dynamite box
public func SetFuse()
{
	SetAction("Fuse");
}

public func Reset()
{
	SetAction("Idle");
}

private func Fusing()
{
	var sin=Sin(180-GetR(),5);
	var cos=Cos(180-GetR(),5);

	if(Contained()!=nil)
	{
		//If the dynamite is held, sparks come from clonk's center.
		sin=0;
		cos=0;
	}

	// Effekt
	if(GetActTime() < 120)
		CastParticles("Spark",1,20,sin,cos,15,25,RGB(255,200,0),RGB(255,255,150));
	// Explosion
	else if(GetActTime() > 140)
		DoExplode();
}

public func OnFuseFinished()
{
	DoExplode();
}

public func DoExplode()
{
	// Activate all fuses
	for(var obj in FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this)))
		obj->~StartFusing(this);
	Explode(18);
}

local ActMap = {
	Fuse = {
		Prototype = Action,
		Name = "Fuse",
		Procedure = DFA_NONE,
		NextAction = "Fuse",
		Delay = 1,
		Length = 1,
		FacetBase = 1,
		Sound = "Fusing",
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
