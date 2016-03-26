/*-- Cable Hoist --*/

#include Library_CableCar

local pickup;

/* Creation */

func Construction()
{
	SetProperty("MeshTransformation",Trans_Rotate(13,0,1,0));
	SetCableSpeed(1);
}

/* Status */

public func IsToolProduct() { return true; }

/* Cable Car Library */

func GetCableOffset(array position, int prec)
{
	if (!prec) prec = 1;
	position[1] += 7 * prec;
}

func Engaged()
{
	SetAction("OnRail");
}

func GetCableCarExtraMenuEntries(array menu_entries, proplist custom_entry, object clonk)
{
	if (IsTravelling()) return;

	if (!pickup)
	{
		// Picking up vehicles
		var vehicles = FindObjects(Find_AtPoint(), Find_Category(C4D_Vehicle), Find_Not(Find_Func("RejectCableHoistPickup", this)), Find_Exclude(this), Find_Func(pickup));
		var i = 0;
		for (var vehicle in vehicles)
		{
			if (GetEffect("CableHoistPickup", vehicle)) continue;

			var pickup = new custom_entry {
				Priority = 2000 + i,
				Tooltip = "$TooltipPickup$",
				OnClick = GuiAction_Call(this, "PickupVehicle", vehicle)
			};
			pickup.image.Symbol = vehicle;
			PushBack(menu_entries, { symbol = vehicle, extra_data = "Pickup", custom = pickup });
			i++;
		}
	} else {
		// Drop the vehicle
		var drop = new custom_entry {
			Priority = 2000,
			Tooltip = "$TooltipDrop$",
			OnClick = GuiAction_Call(this, "DropVehicle")
		};
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
	vehicle->SetPosition(GetX(), GetY());
	vehicle->SetSpeed(0,0);
	vehicle->SetR(GetR());
	vehicle->SetRDir(0);

	pickup = vehicle;
}

func FxCableHoistPickupTimer(object vehicle, proplist effect)
{
	vehicle->SetPosition(GetX(), GetY());
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

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local BorderBound = C4D_Border_Sides;