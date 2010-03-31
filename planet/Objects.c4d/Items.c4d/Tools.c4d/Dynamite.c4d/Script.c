/*
	Dynamite
	Author: Newton

	A volatile tool that can be pressed into walls 
	for accurate mining, burning a short fuse before exploding.
*/
	

// The dynamite is not a weapon but a mining tool
public func ControlUse(object clonk, int iX, int iY, bool fBox)
{
	// if already activated, nothing (so, throw)
	if(GetAction() == "Fuse" || fBox)
	{
		var iAngle = Angle(0,0,iX,iY);
		if(!GetWall(iAngle, iX, iY, clonk))
		{
			//CreateParticle ("Blast", iX, iY, 0, 0, 50, RGB(255,200,0), clonk);
			Message("Can't place dynamite here!", clonk);
			if(fBox) return false;
			return true;
		}
		if(fBox) SetReady();
		// put into ...
		Sound("Connect");

		Exit(iX, iY, Angle(iX,iY));
		SetPosition(clonk->GetX()+iX, clonk->GetY()+iY);
	}
	else
	{
		Fuse();
	}
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
private func GetWall(iAngle, &iX, &iY)
{
	var iDist = 12;
	for(var iDist = 12; iDist < 18; iDist++)
	{
		iX = Sin(iAngle, iDist);
	  iY = -Cos(iAngle, iDist);
		if(GBackSolid(iX, iY))
		{
			iX = Sin(iAngle, iDist-5);
			iY = -Cos(iAngle, iDist-5);
			return true;
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

public func DoExplode()
{
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
