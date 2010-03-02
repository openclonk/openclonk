/*-- Grass Placer --*/

protected func Initialize()
{
	if(ObjectCount(Find_ID(Grass))>1) RemoveObject(); //Failsafe to stop creation of more than one grass controller.
	SetGrassDensity(85);
}

global func SetGrassDensity(int iAmount) //Allows a scenario to change the percentage of earth covered in grass on Initialize().
{
	for(var allgrass in FindObjects(Find_ID(Grass)))
		allgrass->RemoveObject();

	FindObject(Find_ID(Environment_Grass))->PlaceGrass(iAmount);
}

public func PlaceGrass(int iAmount)
{
	SetPosition();

	var x=0;
	while(x<LandscapeWidth())
	{
    var y = MaterialDepthCheck(x,0,"Sky");

	if(GetMaterial(x,y) == Material("Earth"))
	{
	if(Random(100)>(100-iAmount)) GrowGrass(x,y);
	}

	x+=9;
	}
	return 1;
}

public func GrowGrass(int x, int y)
{
	var rot;
	//Rotates grass to form to terrain.
	if(!GBackSolid(x+6,y+3)) rot=40;
	if(!GBackSolid(x-6,y+3)) rot=-40;
	if(!GBackSolid(x-6,y+3) && !GBackSolid(x+6,y+3)) rot=0;
	var grass=CreateObject(Grass,x,y+3);
	grass->SetR(rot);
	grass->DoCon(Random(50));
}

func Definition(def) {
	SetProperty("Name","Grass",def);
}