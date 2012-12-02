/*-- Cable reel --*/

protected func Hit()
{
	Sound("RockHit?");
}

public func IsToolProduct() { return true; }

/*-- Line connection --*/

// Use will connect power line to building at the clonk's position.
protected func ControlUse(object clonk, int x, int y)
{
	// Is there an object which accepts power lines?
	var obj = FindObject(Find_AtPoint(), Find_Func("IsCableCrossing"));
	// No such object -> message.
	if (!obj)
		return clonk->Message("$TxtNoNewLine$");
	// Is there a power line connected to this wire roll?
	var line = FindObject(Find_CableLine());
	// There already is a power line.
	if (line)
	{
		if (obj == line->GetActionTarget(0) || obj == line->GetActionTarget(1)) 
		{
			// Power line is already connected to obj -> remove line.
			line->RemoveObject();
			Sound("Connect");
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
			Sound("Connect");
			line->SetAction("Wait");
			line->UpdateDraw();
			obj->AddCableConnection(line);
			clonk->Message("$TxtConnect$", obj->GetName());
			//RemoveObject();
			return true;
		}
	}
	else // A new power line needs to be created.
	{
		line = CreateObject(CableLine, 0, 0, NO_OWNER);
		line->SetActionTargets(this, obj);
		Sound("Connect");
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
local Rebuy = true;
