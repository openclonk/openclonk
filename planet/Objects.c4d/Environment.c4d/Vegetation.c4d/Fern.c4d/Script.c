/*-- Fern --*/

protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	while(Stuck()) SetPosition(GetX(),GetY()-1);
	DoCon(-Random(50));
	return 1;
}

public func Incineration()
{
	CastParticles("Grass",10,35,0,0,30,50,RGB(255,255,255),RGB(255,255,255));
	RemoveObject();
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}