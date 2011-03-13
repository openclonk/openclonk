/*-- Cloud Placer --*/

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

local Name = "$Name$";
