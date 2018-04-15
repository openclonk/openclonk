/**
	Cable Hoist 

	@author Clonkonaut
*/

#include Library_CableCar

local pickup;


/*-- Creation --*/

public func Construction()
{
	this.MeshTransformation = Trans_Rotate(13, 0, 1, 0);
	SetCableSpeed(1);
	return _inherited(...);
}


/*-- Engine Callbacks --*/

public func Damage(int change, int cause)
{
	if (cause == FX_Call_DmgBlast)
	{
		// Explosions knock the hoist off the rail
		if (GetAction() == "OnRail")
			DisengageRail();
		if (pickup)
			DropVehicle();
	}
}


/*-- Status --*/

public func IsToolProduct() { return true; }


/*-- Cable Car Library --*/

public func GetCableOffset(array position, int prec)
{
	if (!prec)
		prec = 1;
	position[1] += 5 * prec;
}

public func Engaged()
{
	this.Touchable = 0;
	SetAction("OnRail");
}

public func Disengaged()
{
	this.Touchable = 1;
	SetAction("Idle");
	if (pickup)
		DropVehicle();
}

public func GetCableCarExtraMenuEntries(array menu_entries, proplist custom_entry, object clonk)
{
	if (IsTravelling())
		return;

	if (!pickup && GetRailTarget())
	{
		// Picking up vehicles
		var vehicles = FindObjects(Find_AtPoint(), Find_Category(C4D_Vehicle), Find_Not(Find_Func("RejectCableCarPickup", this)), Find_Exclude(this));
		var i = 0;
		for (var vehicle in vehicles)
		{
			if (GetEffect("FxCableHoistPickup", vehicle)) continue;

			var to_pickup = new custom_entry {
				Priority = 2000 + i,
				Tooltip = "$TooltipPickup$",
				OnClick = GuiAction_Call(this, "PickupVehicle", vehicle),
				image = { Prototype = custom_entry.image, Symbol = vehicle },
				icon = { Prototype = custom_entry.icon, Symbol = Icon_LibraryCableCar, GraphicsName = "Engage" }
			};
			PushBack(menu_entries, { symbol = vehicle, extra_data = "Pickup", custom = to_pickup });
			i++;
		}
	}
	else if (pickup && GetRailTarget())
	{
		// Drop the vehicle
		var drop = new custom_entry {
			Priority = 2000,
			Tooltip = "$TooltipDrop$",
			OnClick = GuiAction_Call(this, "DropVehicle"),
			image = { Prototype = custom_entry.image, Symbol = pickup },
			icon = { Prototype = custom_entry.icon, Symbol = Icon_LibraryCableCar, GraphicsName = "Disengage" }
		};
		PushBack(menu_entries, { symbol = pickup, extra_data = "Drop", custom = drop });
	}
}

public func OnCableCarHover(symbol, extra_data, desc_menu_target, menu_id)
{
	if (symbol && extra_data == "Pickup")
	{
		GuiUpdate({ Text = Format("$DescPickup$", symbol->GetName()) }, menu_id, 1, desc_menu_target);
		return;
	}
	if (symbol && extra_data == "Drop")
	{
		GuiUpdate({ Text = Format("$DescDrop$", symbol->GetName()) }, menu_id, 1, desc_menu_target);
		return;
	}
	return _inherited(symbol, extra_data, desc_menu_target, menu_id, ...);
}


/*-- Picking up vehicles --*/

public func PickupVehicle(object vehicle, bool no_rotation_copy)
{
	if (!vehicle) return;
	if (GetEffect("FxCableHoistPickup", vehicle)) return;
	var width = vehicle->GetObjWidth() / 2;
	var height = vehicle->GetObjHeight() / 2;
	if (!Inside(GetX(), vehicle->GetX() - width, vehicle->GetX() + width))
		if (!Inside(GetY(), vehicle->GetY() - height, vehicle->GetY() + height))
			return;
	vehicle->CreateEffect(FxCableHoistPickup, 1, 1, this, no_rotation_copy);
	pickup = vehicle;
	UpdateInteractionMenus(this.GetCableCarMenuEntries);
}

public func DropVehicle()
{
	if (!pickup) return;
	RemoveEffect("FxCableHoistPickup", pickup);
	pickup = nil;
	UpdateInteractionMenus(this.GetCableCarMenuEntries);
}

local FxCableHoistPickup = new Effect
{
	Construction = func(object hoist, bool no_rotation_copy)
	{
		this.hoist = hoist;
		this.vehicle_touchable = Target.Touchable;
		Target.Touchable = 0;
		// Follow motion of hoist.
		this.movement_prec = 100;
		Target->SetPosition(this.hoist->GetX(this.movement_prec), this.hoist->GetY(this.movement_prec) + 4, false, this.movement_prec);
		Target->SetSpeed(0, 0);
		if (!no_rotation_copy)
		{
			Target->SetR(this.hoist->GetR());
			Target->SetRDir(0);
		}
	},
	
	Timer = func(int time)
	{
		// Follow motion of hoist.
		Target->SetPosition(this.hoist->GetX(this.movement_prec), this.hoist->GetY(this.movement_prec) + 4, false, this.movement_prec);
		Target->SetSpeed(0, 0);
	},
	
	Destruction = func()
	{
		Target.Touchable = this.vehicle_touchable;	
	}
};


/*-- Actions --*/

public func OnRail()
{
	DoMovement();
}

local ActMap = {
		OnRail = {
			Prototype = Action,
			Name = "OnRail",
			Procedure = DFA_FLOAT,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 1,
			X = 0,
			Y = 0,
			Wdt = 26,
			Hgt = 18,
			EndCall = "OnRail",
			NextAction = "OnRail",
		},
};


/*-- Callbacks --*/

public func GetAttachedVehicle()
{
	return pickup;
}

// Calls from the stations will mostly be forwarded to the attached vehicle

public func DropContents(object station)
{
	if (pickup)
		pickup->DropContents(station);
}

// Check for available contents
public func IsAvailable(proplist order)
{
	// So far only do something if a lorry is connected, all other vehicles are considered off-limits
	if (pickup && pickup->~IsLorry())
		if (pickup->ContentsCount(order.type) >= order.min_amount)
			return true;
	return false;
}

// Called when a station has asked to make a delivery
public func IsReadyForDelivery(proplist order, object requesting_station)
{
	// Is already on a delivery.
	if (lib_ccar_delivery)
		return false;
	// Only if a lorry is connected
	if (pickup && pickup->~IsLorry())
	{
		// Lorry must have enough space left...
		if (pickup->ContentsCount() + order.min_amount <= pickup.MaxContentsCount)
			return true;
		// ...or contain the requested objects
		if (pickup->ContentsCount(order.type) >= order.min_amount)
			return true;
	}
	return false;
}

// Called when searching for a better option to deliver something
public func OverridePriority(proplist order, object requesting_station, object best_car)
{
	// Check if the connected vehicle holds the requested objects and if yes, override the selection
	if (pickup && pickup->ContentsCount(order.type) >= order.min_amount)
		return true;
	return false;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		 return false;
	if (pickup) 
		props->AddCall("Pickup", this, "PickupVehicle", pickup);
	return true;
}


/*-- Definition --*/

public func Definition(def)
{
	def.PictureTransformation = Trans_Mul(Trans_Rotate(-25, 1, 0, 0), Trans_Rotate(40, 0, 1, 0));
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local BorderBound = C4D_Border_Sides;
local Components = {Metal = 2};
