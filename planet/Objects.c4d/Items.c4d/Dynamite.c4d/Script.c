#strict 2

// The dynamite is not a weapon but a mining tool
public func ControlUse(object clonk)
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
		y = -clonk->GetDefHeight()/2;
	}
	// climbing
	else if(WildcardMatch(clonk->GetAction(), "Climb*"))
	{
		x = clonk->GetDefWidth()/2;
		if(clonk->GetDir() == DIR_Left) x = -x;
	}
	// something else
	else
	{
		// no wall -> put into ground
		if(!getWall(x, y, clonk))
		{
			// contact to ground
			if(clonk->GetContact(-1) & CNAT_Bottom)
			{
				y = clonk->GetDefHeight()/2;
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
	Exit(x*6/10, y + GetDefHeight()/2, Angle(x,y,0,0) + RandomX(-25,25));

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
	var wdt = clonk->GetDefWidth();
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
		var h = GetDefHeight()/2;
		CastParticles("Spark",1,20,Sin(GetR(),h),-Cos(GetR(),h),15,25,RGB(255,200,0),RGB(255,255,150));
	}
	// Explosion
	else if(GetActTime() > 140)
		Explode(18);
}

protected func Definition(def) {
	def["Name"] = "Dynamite";
	def["ActMap"] = {
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
		};
}
