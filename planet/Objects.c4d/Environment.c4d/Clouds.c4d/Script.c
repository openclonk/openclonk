/*-- Cloud Placer --*/

//Great thanks to Maikel for the following function provided 
global func FindPosInMat(string sMat, int iXStart, int iYStart, int iWidth, int iHeight, int iSize)
{
	var iX, iY;
	for(var i = 0; i < 500; i++)
	{
		iX = iXStart+Random(iWidth);
		iY = iYStart+Random(iHeight);
		if(GetMaterial(AbsX(iX),AbsY(iY))==Material(sMat) &&
		   GetMaterial(AbsX(iX+iSize),AbsY(iY+iSize))==Material(sMat) &&
		   GetMaterial(AbsX(iX+iSize),AbsY(iY-iSize))==Material(sMat) &&
		   GetMaterial(AbsX(iX-iSize),AbsY(iY-iSize))==Material(sMat) &&
		   GetMaterial(AbsX(iX-iSize),AbsY(iY+iSize))==Material(sMat)
		) {
			return [iX, iY]; // Location found.
		}
	}
	return 0; // No location found.
}

protected func Initialize()
{
	var iCount = LandscapeWidth()/65; //Determines how many clouds should be on a map

	while(iCount!=0)
	{
		var pos;
		if((pos = FindPosInMat("Sky", 0,0,LandscapeWidth(), LandscapeHeight())) && MaterialDepthCheck(pos[0],pos[1],"Sky",200)) {
			CreateObject(Cloud, pos[0], pos[1], NO_OWNER);
			iCount--;
		}
	}
	AdjustLightningFrequency(GetScenarioVal("Lightning"));
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
