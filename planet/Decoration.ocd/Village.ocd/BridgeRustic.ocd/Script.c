/**
	@author Dustin Neß (dness.de)
*/

local graphicFront, graphicBack;

protected func Construction()
{
	graphicBack = CreateObjectAbove(EnvPack_BridgeRustic_Back,-20, 0, nil);
	graphicBack->SetAction("Attach", this);


	graphicFront = CreateObjectAbove(EnvPack_BridgeRustic_Front, 0, 0, nil);
	graphicFront->SetAction("Attach", this);
	
	
	
	SetClrModulation(RGBa(0, 0, 0, 0)); // Set SolidMask graphic invisible

	return true;
}
