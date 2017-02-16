/**
	ClonkGamepadControl
	Handles the control of the Clonk using a gamepad.
	
	Is included by ClonkControl.ocd
*/

/*
	used properties:
	this library uses the 'this.control' property with properties declared in ClonkControl.ocd
	i.e.:
	this.control.mlastx
	this.control.mlasty
	this.control.current_object
	this.control.noholdingcallbacks
*/

local virtual_cursor;

/* This part of gamepad control handles only object-style menus.
Fullscreen menus are handled differently. */
func Control2Menu(int ctrl, int x, int y, int strength, bool repeat, int status)
{
	/* all this stuff is already done on a higher layer - in playercontrol.c
	   now this is just the same for gamepad control */
	if (!HasVirtualCursor())
		return true;

	if (!this->GetMenu())
		return false;

	// fix pos of x and y
	var mex = this.control.mlastx + GetX() - GetMenu()->GetX();
	var mey = this.control.mlasty + GetY() - GetMenu()->GetY();

	// update angle for visual effect on the menu
	if (repeat)
	{
		if (ctrl == CON_Use || ctrl == CON_UseAlt)
			this->GetMenu()->~UpdateCursor(mex, mey);
	}
	// click on menu
	if (status == CONS_Up)
	{
		// select
		if (ctrl == CON_Use)
			this->GetMenu()->~OnMouseClick(mex, mey);
	}

	return true;
}

public func ObjectControlMovement(int plr, int ctrl, int strength, int status)
{
	// from PlayerControl.c
	var result = inherited(plr, ctrl, strength, status, ...);

	// do the following only if strength >= CON_Gamepad_Deadzone
	if (status == CONS_Down)
		if (strength != nil && strength < CON_Gamepad_Deadzone)
			return result;
	

	if (!virtual_cursor)
		virtual_cursor = FindObject(Find_ID(GUI_Crosshair), Find_Owner(GetOwner()));
	if (!virtual_cursor)
		return result;

	// change direction of virtual_cursor
	if (status == CONS_Down)
		virtual_cursor->Direction(ctrl);

	return result;
}

func ReinitializeControls()
{
	if (PlayerHasVirtualCursor(GetOwner()))
	{
		// if is aiming or in menu and no virtual cursor is there, create one
		if (!virtual_cursor)
			if (this.menu || this.control.current_object) // properties declared in ClonkControl.ocd
				VirtualCursor()->StartAim(this, 0, this.menu);
	}
	else
	{
		// remove any virtual cursor
		if (virtual_cursor)
			virtual_cursor->RemoveObject();
	}
}

/* Virtual cursor stuff */

// Helper function.
private func HasVirtualCursor()
{
	return PlayerHasVirtualCursor(GetOwner());
}

// get virtual cursor, if noone is there, create it
private func VirtualCursor()
{
	if (!virtual_cursor)
	{
		virtual_cursor = FindObject(Find_ID(GUI_Crosshair), Find_Owner(GetOwner()));
	}
	if (!virtual_cursor)
	{
		virtual_cursor = CreateObject(GUI_Crosshair, 0, 0, GetOwner());
	}
	
	return virtual_cursor;
}

// virtual cursor is visible
private func VirtualCursorAiming()
{
	if (!virtual_cursor)
		return false;
	return virtual_cursor->IsAiming();
}

// store pos of virtual cursor into mlastx, mlasty
public func UpdateVirtualCursorPos()
{
	this.control.mlastx = VirtualCursor()->GetX() - GetX();
	this.control.mlasty = VirtualCursor()->GetY() - GetY();
}

public func TriggerHoldingControl()
{
	if (this.control.current_object && !this.control.noholdingcallbacks)
	{
		var ctrl = CON_Use;
		if (this.control.alt)
			ctrl = CON_UseAlt;
		ObjectControl(GetOwner(), ctrl, 0, 0, 0, true, CONS_Down);
	}
}

func RemoveVirtualCursor()
{
	if (virtual_cursor)
		virtual_cursor->StopAim();
}
