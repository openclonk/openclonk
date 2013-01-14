/*-- Rank --*/

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	SetR(RandomX(-30,30));
}

func Damage()
{	
	if (GetDamage() > 15)
	{
		CastObjects(Wood, 1, 25);
		RemoveObject();
	}
	return;
}

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;