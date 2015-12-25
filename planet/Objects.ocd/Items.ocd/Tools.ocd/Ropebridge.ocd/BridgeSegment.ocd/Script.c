/**
	Ropebridge Segment

	@author Randrian	
*/

local master;
local Plank;
local fragile;
local Double;

public func SetMaster(newmaster)
{
	master = newmaster;
}

/*-- Events --*/

protected func Damage(int amount)
{
	if (GetDamage() > 18 && Plank)
		LoosePlank();
}

public func LoosePlank()
{
	var loose_plank = CreateObject(BridgePlank);
	loose_plank->SetR(GetR());
	loose_plank->SetPosition(GetX(100) + Cos(GetR(), -400) + Sin(GetR(), 200), GetY(100) + Sin(GetR(), -400) + Cos(GetR(), 200), 0, 100);
	Plank = 0;
	SetSolidMask();
	SetGraphics(nil, nil, 6);
}

public func GetLoadWeight()
{
	if (!Plank)
		return 10;
	var weight = 50;
	var arr = [0, 0, 0, 0, 0, 0, 0];
	var i = 0;
	for (obj in FindObjects(Find_AtRect(-3, -10, 6, 10), Find_Exclude(this), Find_NoContainer()))
		if (obj->GetID() != Ropebridge_Segment && obj->GetID() != Ropebridge_Post && obj->GetID() != BridgePlank)
			if (obj->GetContact(-1, 8))
			{
				arr[i++] = obj->GetName();
				weight += obj->GetMass();
			}
	if (fragile && weight > 60 && Random(10))
	{
		ScheduleCall(this, "LoosePlank", 10);
		fragile = 0;
	}
	return weight;
}

public func CreateDouble()
{
	if (!Double)
	{
		Double = CreateObjectAbove(GetID());
		Double.Plane = 600;
	}
}

// Main bridge object is saved
func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH
	},
	Fall = {
		Prototype = Action,
		Name = "Fall",
		Procedure = DFA_NONE,
		Length = 20,
		Delay = 1,
		EndCall = "LoosePlank",
		NextAction = "Idle",
	},
};
