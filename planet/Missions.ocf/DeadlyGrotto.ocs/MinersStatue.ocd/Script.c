/*--
	Miner's statue
	Author: Sven2

	Impressive piece of rock carving.
--*/

local is_broken;

protected func Hit(x, y)
{
	StonyObjectHit(x,y);
	return true;
}

func SetBroken()
{
	SetGraphics("Broken");
	is_broken = true;
	ScheduleCall(this, MinersStatue.Check4Head, 20, 99999999);
	return true;
}

func SetIntact()
{
	SetGraphics();
	is_broken = false;
	ClearScheduleCall(this, MinersStatue.Check4Head);
	return true;
}

func Check4Head()
{
	var head = FindObject(Find_InRect(-23,-40,46,80), Find_ID(MinersStatue_Head));
	if (head)
	{
		SetIntact();
		head->RemoveObject();
	}
	return true;
}

func IsBroken() { return is_broken; }

public func Definition(proplist def)
{
}

local Collectible = false;
local Name = "$Name$";
local Description = "$Description$";
local Touchable = 0;
local Plane = 220;
