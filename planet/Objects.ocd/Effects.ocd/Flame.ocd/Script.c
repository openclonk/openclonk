/** 
	Flame
	Spreads fire.
	
	@author Maikel
*/


protected func Initialize()
{
	Incinerate(100, GetController());
	AddTimer("Burning", RandomX(24, 26));
	return;
}

public func Burning()
{
	if (!OnFire()) return RemoveObject();
	// Consume inflammable material and make the flame a little bigger.
	if (FlameConsumeMaterial() && GetCon() <= 80)
	{
		DoCon(6);	
		SetXDir(RandomX(-8, 8));
	}
	// Split the flame if it is large enough and not too many flames are nearby.
	var amount = ObjectCount(Find_ID(GetID()), Find_Distance(10));	
	if (amount < 5 && GetCon() > 50 && !Random(4))
	{
		var x = Random(15);
		var new_flame = CreateObjectAbove(GetID());
		new_flame->SetSpeed(x, -7);
		new_flame->SetCon(GetCon() / 2);
		SetSpeed(-x, -7);
		SetCon(GetCon() / 2);	
	}
	return;
}

// Don't incinerate twice in saved scenarios.
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Fire");
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 500;