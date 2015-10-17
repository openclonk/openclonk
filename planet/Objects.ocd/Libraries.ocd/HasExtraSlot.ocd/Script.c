/*
	Has extra slot
	Author: Newton
	
	For this, an extra object is not required but applicable because the
	HUD needs to be notified on changes of the slot.
	
	Collection2 and Ejection need to call _inherited(...).
*/

// interface
func HasExtraSlot() { return true; }

func Collection2() { NotifyHUD(); return _inherited(...); }
func Ejection() { NotifyHUD(); return _inherited(...); }

// HUDs will attach effects to this object that take care of the rest.
func NotifyHUD()
{
	var i = 0, e = nil;
	while (e = GetEffect("*", this, i++))
		EffectCall(this, e, "Update");
}

// This object is a container that can be inspected further in the interaction menu.
func IsContainer() { return true; }

// Disallow stacking if this object is not empty.
public func CanBeStackedWith(object other)
{
	return !ContentsCount() && inherited(other, ...);
}

local Name = "ExtraSlot";
