/*-- Cloud Placer --*/

protected func Initialize()
{
	var iX, iY;
	var iCount = LandscapeWidth()/65; //Determines how many clouds should be on a map

	while(iCount!=0)
	{
		if(FindPosInMat(iX, iY, "Sky", 0,0,LandscapeWidth(), LandscapeHeight()) && MaterialDepthCheck(iX,iY,"Sky",200)==true) {
			CreateObject(Cloud, iX, iY, NO_OWNER);
			iCount--;
		}
	}
	AdjustLightningFrequency(GetScenarioVal("Lightning"));
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
