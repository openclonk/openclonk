/*-- Cloud Placer --*/

#strict 2

//Great thanks to Maikel for the following function provided 
global func FindPosInMat(int &iToX, int &iToY, string sMat, int iXStart, int iYStart, int iWidth, int iHeight, int iSize)
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
       GetMaterial(AbsX(iX-iSize),AbsY(iY+iSize))==Material(sMat))
                {
      iToX = iX; iToY = iY;
      return true; // Location found.
    }
  }
  return false; // No location found.
}
protected func Initialize()
{
	var iX, iY;
	var iCount = LandscapeWidth()/65; //Determines how many clouds should be on a map

	while(iCount!=0)
	{
		if(FindPosInMat(iX, iY, "Sky", 0,0,LandscapeWidth(), LandscapeHeight()) && MaterialDepthCheck(iX,iY,"Sky",200)==true) CreateObject(CLOD, iX, iY, NO_OWNER) && (iCount=--iCount);
	}
	AdjustLightningFrequency(GetScenarioVal("Lightning"));
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
