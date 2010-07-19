/*-- Tree --*/

protected func Initialize()
{
	SetCon(RandomX(30,100));
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	return 1;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}