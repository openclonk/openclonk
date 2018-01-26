/*-- Cable reel --*/

protected func Hit()
{
	Sound("Hits::Materials::Rock::RockHit?");
}

public func IsToolProduct() { return true; }

/*-- Line connection --*/

protected func ControlUse(object clonk, int x, int y)
{
	// Is there an object which accepts power lines?
	var obj = FindObject(Find_AtPoint(), Find_Func("IsCableCrossing"));
	// No such object -> message.
	if (!obj)
		return clonk->Message("$TxtNoNewLine$");
	// Is there a cable connected to this wire roll?
	var line = FindObject(Find_CableLine());
	// There already is a cable
	if (line)
	{
		if (obj == line->GetActionTarget(0) || obj == line->GetActionTarget(1)) 
		{
			// Cable is already connected to obj -> remove line.
			line->RemoveObject();
			Sound("Objects::Connect");
			clonk->Message("$TxtLineRemoval$");
			return true;
		}
		else
		{
			// Connect existing power line to obj.
			if(line->GetActionTarget(0) == this)
				line->SetActionTargets(obj, line->GetActionTarget(1));
			else if(line->GetActionTarget(1) == this)
				line->SetActionTargets(line->GetActionTarget(0), obj);
			else
				return;
			Sound("Objects::Connect");
			obj->AddCableConnection(line);
			clonk->Message("$TxtConnect$", obj->GetName());
			//RemoveObject();
			return true;
		}
	}
	else // A new cable needs to be created.
	{
		line = CreateObjectAbove(CableLine, 0, 0, NO_OWNER);
		line->SetActionTargets(this, obj);
		Sound("Objects::Connect");
		clonk->Message("$TxtConnect$", obj->GetName());
		return true;
	}
}

// Finds all power lines connected to obj (can be nil in local calls).
private func Find_CableLine(object obj)
{
	if (!obj)
		obj = this;
	return [C4FO_Func, "IsConnectedTo", obj];
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Metal = 1};