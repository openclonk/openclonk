#strict 2

// The dynamite is not a weapon but a mining tool
public func ControlUse(object clonk, int iX, int iY,)
{
	// if already activated, nothing (so, throw)
	if(GetAction() == "Fuse")
	{
		return false;
	}

	var iAngle = Angle(0,0,iX,iY);
	if(!GetWall(iAngle, iX, iY, clonk))
	{
		//CreateParticle ("Blast", iX, iY, 0, 0, 50, RGB(255,200,0), clonk);
		Message("Can't place dynamite here!", clonk);
		return true;
	}

	// Fuse
	Fuse();

	// put into ...
	Sound("Connect");

	Exit(iX, iY, Angle(iX,iY));
	SetPosition(clonk->GetX()+iX, clonk->GetY()+iY);

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
private func GetWall(iAngle, &iX, &iY, clonk)
{
	var iDist = 8;
	for(var iDist = 8; iDist < 18; iDist++)
	{
		iX = Sin(iAngle, iDist);
	  iY = -Cos(iAngle, iDist);
		Message("%d", clonk, iDist);
		if(GBackSolid(iX, iY)) return true;
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
}