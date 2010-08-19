/*
	Has extra slot
	Author: Newton
	
	For this, an extra object is not required but applicable because the
	HUD needs to be notified on changes of the slot.
	
	Collection2 and Ejection need to call _inherited(...).
*/

local ExtraHUD;

// interface
func HasExtraSlot() { return true; }
func SetHUDObject(object extraslot) { ExtraHUD = extraslot; }

func NotifyHUD() { if (ExtraHUD) ExtraHUD->Update(); }
func Collection2() { NotifyHUD(); return _inherited(...); }
func Ejection() { NotifyHUD(); return _inherited(...); }

func Definition(def) 
{
	SetProperty("Name", "ExtraSlot", def);
}