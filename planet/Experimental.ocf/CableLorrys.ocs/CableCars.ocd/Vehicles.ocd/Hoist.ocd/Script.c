/*-- Cable Hoist --*/

#include Library_CableCar

local pickup;

/* Creation */

func Construction()
{
	SetProperty("MeshTransformation",Trans_Rotate(13,0,1,0));
	SetCableSpeed(1);
}

/* Engine Callbacks */

func Damage(int change, int cause)
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

/* Status */

public func IsToolProduct() { return true; }

/* Cable Car Library */

func GetCableOffset(array position, int prec)
{
	if (!prec) prec = 1;
	position[1] += 5 * prec;
}

func Engaged()
{
	SetAction("OnRail");
}

func Disengaged()
{
	SetAction("Idle");
	if (pickup)
		DropVehicle();
}

func GetCableCarExtraMenuEntries(array menu_entries, proplist custom_entry, object clonk)
{
	if (IsTravelling()) return;

	if (!pickup && GetRailTarget())
	{
		// Picking up vehicles
		var vehicles = FindObjects(Find_AtPoint(), Find_Category(C4D_Vehicle), Find_Not(Find_Func("RejectCableHoistPickup", this)), Find_Exclude(this), Find_Func(pickup));
		var i = 0;
		for (var vehicle in vehicles)
		{
			if (GetEffect("CableHoistPickup", vehicle)) continue;

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
	} else if (pickup && GetRailTarget()) {
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

/* Picking up vehicles */

public func PickupVehicle(object vehicle)
{
	if (!vehicle) return;
	if (GetEffect("CableHoistPickup", vehicle)) return;
	var width = vehicle->GetObjWidth() / 2;
	var height = vehicle->GetObjHeight() / 2;
	if (!Inside(GetX(), vehicle->GetX() - width, vehicle->GetX() + width))
		if (!Inside(GetY(), vehicle->GetY() - height, vehicle->GetY() + height))
			return;

	AddEffect("CableHoistPickup", vehicle, 1, 1, this);
	UpdateInteractionMenus(this.GetCableCarMenuEntries);
}

public func DropVehicle()
{
	if (!pickup) return;
	RemoveEffect("CableHoistPickup", pickup);
	UpdateInteractionMenus(this.GetCableCarMenuEntries);
}

func FxCableHoistPickupStart(object vehicle, proplist effect)
{
	vehicle->SetPosition(GetX(), GetY()+4);
	vehicle->SetSpeed(0,0);
	vehicle->SetR(GetR());
	vehicle->SetRDir(0);

	pickup = vehicle;
}

func FxCableHoistPickupTimer(object vehicle, proplist effect)
{
	vehicle->SetPosition(GetX(), GetY()+4);
	vehicle->SetSpeed(0,0);
}

func FxCableHoistPickupStop(object vehicle, proplist effect)
{
	pickup = nil;
}

/* Actions */

func OnRail()
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

/* Callbacks */

public func GetAttachedVehicle()
{
	return pickup;
}

// Calls from the stations will mostly be forwarded to the attached vehicle

public func DropContents(proplist station)
{
	if (pickup)
		pickup->DropContents(station);
}

// Check for available contents
public func IsAvailable(proplist requested, int amount)
{
	// So far only do something if a lorry is connected, all other vehicles are considered off-limits
	if (pickup && pickup->~IsLorry())
		if (pickup->ContentsCount(requested) >= amount)
			return true;
	return false;
}

// Called when a station has asked to make a delivery
public func IsReadyForDelivery(proplist requested, int amount, proplist requesting_station)
{
	// Only if a lorry is connected
	if (pickup && pickup->~IsLorry())
	{
		// Lorry must have enough space left...
		if (pickup->ContentsCount() + amount <= pickup.MaxContentsCount)
			return true;
		// ...or contain the requested objects
		if (pickup->ContentsCount(requested) >= amount)
			return true;
	}
	return false;
}

// Called when searching for a better option to deliver something
public func OverridePriority(proplist requested, int amount, proplist requesting_station, proplist best)
{
	// Check if the connected vehicle holds the requested objects and if yes, override the selection
	if (pickup && pickup->ContentsCount(requested) >= amount)
		return true;

	return false;
}

/* Definition */

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local BorderBound = C4D_Border_Sides;