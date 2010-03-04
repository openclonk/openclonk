/*-- Grass Placer --*/

protected func Initialize()
{
	if(ObjectCount(Find_ID(Grass))>1) RemoveObject(); //Failsafe to stop creation of more than one grass controller.
	PlaceGrass(85);
}

global func PlaceGrass(int iAmount, int start, int end)
{
	if(!start)
		start = 0;
		
	if(!end)
		end = LandscapeWidth();
		
	var x=start;
	while(x<end)
	{
		var y = MaterialDepthCheck(AbsX(x),0,"Sky");

		if(GetMaterial(AbsX(x),AbsY(y)) == Material("Earth"))
		{
			if(Random(100)>(100-iAmount))
			{
				var rot;
				//Rotates grass to form to terrain.
				if(!GBackSolid(AbsX(x+6),AbsY(y+3))) rot=40;
				if(!GBackSolid(AbsX(x-6),AbsY(y+3))) rot=-40;
				if(!GBackSolid(AbsX(x-6),AbsY(y+3)) && !GBackSolid(AbsX(x+6),AbsY(y+3))) rot=0;
				var grass=CreateObject(Grass,AbsX(x),AbsY(y+3));
				grass->SetR(rot);
				grass->DoCon(Random(50));
			}
		}

		x+=9;
	}
}

func Definition(def) {
	SetProperty("Name","Grass",def);
}