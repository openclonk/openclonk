/**
	Tunnel support
	Purely for decoration

	@authors: Clonkonaut, Ringwaul (Graphics)
*/

local extension = 0;

func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(75, 105), 0,1,0));
}

// Stretch the support beams
// 0 equals standard height (~30 pixels)
// 100 is about 85 pixels
public func Extend(int percentage)
{
	percentage = BoundBy(percentage, 0, 100);

	extension = percentage;

	var height = 30 + (55 * percentage / 100);
	percentage = 2500 * percentage / 100;
	PlayAnimation("extend", 1, Anim_Const(percentage));

	SetShape(-15, -15 + (30 - height), 30, height);
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;

	props->AddCall("Extension", this, "Extend", extension);
	return true;
}

/*-- Properties --*/

local Name = "$Name$";
local Plane = 701;