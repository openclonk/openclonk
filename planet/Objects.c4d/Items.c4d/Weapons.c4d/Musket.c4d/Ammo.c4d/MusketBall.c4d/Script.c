/*-- Musket Ball --*/

#strict 2
#include L_ST

public func MaxStackCount() { return 8; }

public func IsMusketAmmo() { return 1; }


protected func Hit()
{
	RemoveEffect("HitCheck",this);
	SetVelocity(Random(359)); //stops object from careening over the terrain, ricochets would be better :p
}

public func AffectShot(object shooter,int ix,int iy)
{
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);	
	//Smush vertexes into one point
	SetVertexXY(1,0,0);
	SetVertexXY(2,0,0);

}

private func HitObject(object pVictim)
{
	Message("Ouch!", pVictim); //Remove when sound works; for debug

	Punch(pVictim,RandomX(20,30));
	RemoveObject();
}

func UpdatePicture()
{
	var Shots=GetStackCount();
	if(Shots>=MaxStackCount()) SetGraphics(nil);
	if(Shots<MaxStackCount()) SetGraphics(Format("%d",Shots));
	_inherited(...);
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}