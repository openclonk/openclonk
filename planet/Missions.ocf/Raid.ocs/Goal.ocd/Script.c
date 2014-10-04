/*-- Goal --*/

#include Library_Goal

public func IsFulfilled()
{
	return is_fulfilled;
}

public func SetStageNewton()
{
	Description = "$Description2$";
	SetGraphics("Newton");
	NotifyHUD();
	return true;
}

public func SetStagePyrit()
{
	Description = "$Description3$";
	SetGraphics("Pyrit");
	NotifyHUD();
	return true;
}

public func SetStagePlane()
{
	Description = "$Description4$";
	SetGraphics("Plane");
	NotifyHUD();
	return true;
}

public func SetStageDone()
{
	Description = "$Description5$";
	NotifyHUD();
	return true;
}

public func SetFulfilled()
{
	is_fulfilled = true;
	NotifyHUD();
	return true;
}


local is_fulfilled = false;

local Name = "$Name$";
local Description = "$Description1$";
