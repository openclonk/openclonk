/*-- Mushroom --*/

protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	while(Stuck()) SetPosition(GetX(),GetY()-1);
	DoCon(-Random(50));
	return 1;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}