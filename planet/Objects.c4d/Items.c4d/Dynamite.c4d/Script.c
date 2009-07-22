#strict 2

// The dynamite is not a weapon but a mining tool

protected func Activate(object clonk)
{
	return Use(clonk);
}

public func Use(object clonk)
{
	// if already activated, nothing (so, throw)
	if(GetAction() == "Fuse")
	{
		return false;
	}

	var x = 0, y = 0;
	
	// hangling
	if(WildcardMatch(clonk->GetAction(),"Hangle*"))
	{
		y = -GetDefHeight(clonk->GetID())/2;
	}
	// climbing
	else if(WildcardMatch(clonk->GetAction(), "Climb*"))
	{
		x = GetDefWidth(clonk->GetID())/2;
		if(clonk->GetDir() == DIR_Left) x = -x;
	}
	// something else
	else
	{
		// no wall -> put into ground
		if(!getWall(x, y, clonk))
		{
			// contact to ground
			if(GetContact(clonk, -1) & CNAT_Bottom)
			{
				y = GetDefHeight(clonk->GetID())/2;
			}
			// probably flying or swimming
			else
			{
				// nothing
				return false;
			}
		}

	}

	// Fuse
	Fuse();
	
	// put into ...
	Sound("Connect");
	Exit(this, x*6/10, y + GetDefHeight(GetID())/2, Angle(x,y,0,0) + RandomX(-25,25));

	return true;
}

public func Fuse()
{
	if(GetAction() != "Fuse")
	{
		Sound("Fuse");
		SetAction("Fuse");
	}
}

// returns true if there is a wall in direction in which "clonk" looks 
// and puts the offset to the wall into "xo, yo" - looking from the clonk
private func getWall(&xo, &yo, object clonk)
{
	var wdt = GetDefWidth(clonk->GetID());
	var dir = clonk->GetDir()*2-1;
	var x = (wdt/2)*dir;
	var s = false;
	if(GBackSolid(x, 0)) { xo = x; yo = 0; return true; }
	if(GBackSolid(x, -5)) { xo = x; yo = -5; return true; }
	if(GBackSolid(x, +5)) { xo = x; yo = +5; return true; }
	return false;
}

protected func Hit() { Sound("WoodHit*"); }

protected func Incineration() { Extinguish(); Fuse(); }

protected func RejectEntrance()
{
  return GetAction() == "Fuse";
}

private func Fusing() {
	// Effekt
	if(GetActTime() < 120)
	{
		var h = GetDefHeight(GetID())/2;
		CastParticles("Spark",1,20,Sin(GetR(),h),-Cos(GetR(),h),15,25,RGB(255,200,0),RGB(255,255,150));
	}
	// Explosion
	else if(GetActTime() > 140)
		Explode(18);
}

protected func Definition(def) {
	SetProperty("ActMap", {
			Fuse = {
				Prototype = Action,
				Name = "Fuse",
				Procedure = DFA_FLOAT,
				NextAction = "Fuse",
				Delay = 1,
				Length = 1,
				FacetBase = 1,
				Sound = "Fusing",
				StartCall = "Fusing"
			}
		}, def);
}
