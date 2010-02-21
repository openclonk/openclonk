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
	SquishVertices(true);
}

private func HitObject(object pVictim)
{
	Message("Ouch!", pVictim); //Remove when sound works; for debug

	pVictim->DoEnergy(-RandomX(10,15));
	RemoveObject();
}

func UpdatePicture()
{
	var Shots=GetStackCount();
	if(Shots>=MaxStackCount()) SetGraphics(nil);
	if(Shots<MaxStackCount()) SetGraphics(Format("%d",Shots));
	//Realigns vertex points if >1
	if(Shots==1)
	{
		SquishVertices(true);
	}

	if(Shots>1)
	{
		SquishVertices(false);
	}
	_inherited(...);
}

private func SquishVertices(bool squish)
{
	if(squish==true)
	{
		SetVertex(1,VTX_X,0,2);
		SetVertex(1,VTX_Y,0,2);
		SetVertex(2,VTX_X,0,2);
		SetVertex(2,VTX_Y,0,2);
	return 1;
	}

	if(squish!=true)
	{
		SetVertex(1,VTX_X,-3,2);
		SetVertex(1,VTX_Y,1,2);
		SetVertex(2,VTX_X,3,2);
		SetVertex(2,VTX_Y,1,2);
	return 0;
	}

}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}