/**
	Ropebridge Segment

	@author Randrian	
*/

local master;
local is_fragile;
local has_plank;
local double;


/*-- State --*/

public func SetMaster(object new_master)
{
	master = new_master;
}

public func SetFragile(bool fragile)
{
	is_fragile = fragile;
}

public func SetPlank(bool plank)
{
	has_plank = plank;
}

public func HasPlank() { return has_plank; }


public func CreateDouble()
{
	if (!double)
	{
		double = CreateObjectAbove(GetID());
		double.Plane = 600;
	}
}

public func GetDouble() { return double; }


/*-- Events --*/

protected func Damage(int amount)
{
	if (GetDamage() > 18 && has_plank)
		LoosePlank();
	return;
}

public func LoosePlank()
{
	var loose_plank = CreateObject(BridgePlank);
	loose_plank->SetR(GetR());
	loose_plank->SetPosition(GetX(100) + Cos(GetR(), -400) + Sin(GetR(), 200), GetY(100) + Sin(GetR(), -400) + Cos(GetR(), 200), 0, 100);
	has_plank = false;
	SetSolidMask();
	SetGraphics(nil, nil, 6);
}

public func GetLoadWeight()
{
	if (!has_plank)
		return 10;
	var weight = 50;
	for (var obj in FindObjects(Find_AtRect(-3, -10, 6, 10), Find_Exclude(this), Find_NoContainer()))
		if (obj->GetID() != Ropebridge_Segment && obj->GetID() != Ropebridge_Post && obj->GetID() != BridgePlank)
			if (obj->GetContact(-1, 8))
				weight += obj->GetMass();

	if (is_fragile && weight > 60 && Random(10))
	{
		ScheduleCall(this, "LoosePlank", 10);
		is_fragile = 0;
	}
	return weight;
}

public func Destruction()
{
	if (double)
		double->RemoveObject();
	return;
}

// Main bridge object is saved
func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH
	}
};
