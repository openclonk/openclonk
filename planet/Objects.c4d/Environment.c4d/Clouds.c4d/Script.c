/*-- Cloud Placer --*/

#strict

//Great thanks to Maikel for the following function provided 
protected func FindPosInMat(int &iToX, int &iToY, string sMat, int iXStart, int iYStart, int iWidth, int iHeight, int iSize)
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
		if(FindPosInMat(iX, iY, "Sky", 0,0,LandscapeWidth(), LandscapeHeight()) && 
			MaterialDepthCheck(iX,iY,"Sky",200)==true) CreateObject(CLOD, iX, iY, NO_OWNER) && (iCount=--iCount);
	}
}

global func MaterialDepthCheck(int iX,int iY,string szMaterial,int iDepth)
{
	var iTravelled;
	var iXval = iX;
	var iYval = iY;

	//If iDepth is equal to zero, the function will always measure the depth of the material.
	//If iDepth is not equal to zero, the function will return true if the material is as deep or deeper than iDepth (in pixels).
	if(iDepth==0) iDepth=LandscapeHeight();

	while(iTravelled!=iDepth) 
	{
		if(GetMaterial(iXval,iYval)==Material(szMaterial)) (iTravelled=++iTravelled) && (iYval=++iYval);
		if(GetMaterial(iXval,iYval)!=Material(szMaterial)) return(iTravelled);//returns depth of material
	}
	if(iTravelled==iDepth) return(true);
}
func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
