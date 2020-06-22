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

// Disallow stacking if the extra slots of both objects do not contain items that can stack.
// An empty extra slot does not stack with a filled one either.
public func CanBeStackedWith(object other)
{
	if (Contents())
	{
		if (!other->Contents())
			return false;
		return Contents()->CanBeStackedWith(other->Contents()) && inherited(other, ...);
	}	
	return !other->Contents() && inherited(other, ...);
}

local Name = "ExtraSlot";
