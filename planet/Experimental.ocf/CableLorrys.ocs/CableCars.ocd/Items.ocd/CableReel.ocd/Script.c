/**
	Cable reel
	Connects cable stations.
	
	@author Clonkonaut	
*/


#include Library_Stackable


public func Hit()
{
	Sound("Hits::Materials::Rock::RockHit?");
}

public func IsToolProduct() { return true; }

public func MaxStackCount() { return 4; }


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
		// Find other connected crossing.
		var other_obj = nil;
		if (line->GetActionTarget(0) == this)
			other_obj = line->GetActionTarget(1);
		else if(line->GetActionTarget(1) == this)
			other_obj = line->GetActionTarget(0);
		else
			return FatalError("Cable reel: line not connected to reel while trying to connect crossings.");
		// Cable is already connected to obj -> remove cable.
		if (obj == line->GetActionTarget(0) || obj == line->GetActionTarget(1)) 
		{
			line->RemoveObject();
			OnCableLengthChange();
			Sound("Objects::Connect");
			clonk->Message("$TxtCableRemoval$");
			return true;
		}
		// Check if this connection does not already exist.
		else if (AreDirectlyConnectedCrossings(obj, other_obj))
		{
			clonk->Message("$TxtDoubleCable$");
		}		
		// Create new connection.
		else
		{
			line->SetActionTargets(obj, other_obj);
			Sound("Objects::Connect");
			obj->AddCableConnection(line);
			OnCableLengthChange();
			clonk->Message("$TxtConnect$", obj->GetName());
			DoStackCount(-1);
			return true;
		}
	}
	// A new cable needs to be created.
	else 
	{
		line = CreateObjectAbove(CableLine, 0, 0, NO_OWNER);
		line->SetActionTargets(this, obj);
		OnCableLengthChange();
		Sound("Objects::Connect");
		clonk->Message("$TxtConnect$", obj->GetName());
		return true;
	}
}

// Returns whether c1 and c2 are already directly connected by a cable.
private func AreDirectlyConnectedCrossings(object c1, object c2)
{
	return !!FindObject(Find_Func("IsConnectedTo", c1), Find_Func("IsConnectedTo", c2));
}

// Finds all cables connected to obj (can be nil in local calls).
private func Find_CableLine(object obj)
{
	if (!obj)
		obj = this;
	return [C4FO_Func, "IsConnectedTo", obj];
}


/*-- Inventory --*/

public func OnCableLineRemoval()
{
	OnCableLengthChange();
	return;
}

public func OnCableLengthChange()
{
	// Update usage bar for a possible carrier (the clonk).
	var carrier = Contained();
	if (carrier)
		carrier->~OnInventoryChange();
	return;
}

// Display the line length bar over the pipe icon.
public func GetInventoryIconOverlay()
{
	var line = FindObject(Find_CableLine());
	if (!line) return;

	var percentage = 100 * line->GetCableLength() / line.CableMaxLength;
	var red = percentage * 255 / 100;
	var green = 255 - red;
	// Overlay a usage bar.
	var overlay = 
	{
		Bottom = "0.75em",
		Margin = ["0.1em", "0.25em"],
		BackgroundColor = RGB(0, 0, 0),
		margin = 
		{
			Margin = "0.05em",
			bar = 
			{
				BackgroundColor = RGB(red, green, 0),
				Right = Format("%d%%", percentage),
			}
		}
	};
	return overlay;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Metal = 1};