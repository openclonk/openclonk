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
				var grass=CreateObject(Grass,AbsX(x),AbsY(y+3));
			}
		}

		x+=9;
	}
}

func Definition(def) {
	SetProperty("Name","Grass",def);
}