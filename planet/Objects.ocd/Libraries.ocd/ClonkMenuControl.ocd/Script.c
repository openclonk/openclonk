/**
	Clonk menu controls
	Author: Newton

	This object provides handling of the clonk controls including item
	management, backpack controls and standard throwing behaviour. It
	should be included into any clonk/crew definition.
	The controls in System.ocg/PlayerControl.c only provide basic movement
	handling, namely the movement left, right, up and down. The rest is
	handled here:
	(object) menu control and it's callbacks and
	forwards to script.

	Objects that inherit this object need to return _inherited(...) in the
	following callbacks (if defined):
		Construction, Collection2, Ejection, RejectCollect, Departure,
		Entrance, AttachTargetLost, CrewSelection, Death,
		Destruction, OnActionChanged	

	Used properties
	this.control.menu: the menu that is currently assigned to the Clonk. Use the methods SetMenu/GetMenu/etc to access it.

*/

/* ++++++++++++++++++++++++ Callbacks ++++++++++++++++++++++++ */

protected func Construction()
{
	if (this.control == nil)
		this.control = {};

	this.control.menu = nil;
	return _inherited(...);
}


// ...aaand the same for when the clonk is deselected
protected func CrewSelection(bool unselect)
{
	if (unselect)
	{
		// if there is still a menu, cancel it too...
		CancelMenu();
	}
	return _inherited(unselect, ...);
}

protected func Destruction()
{
	// close open menus, ...
	CancelMenu();
	return _inherited(...);
}

protected func Death()
{
	// close open menus, ...
	CancelMenu();
	return _inherited(...);
}

/* +++++++++++++++++++++++ Menu control +++++++++++++++++++++++ */

func HasMenuControl()
{
	return true;
}

// helper function that can be attached to a proplist to set callbacks on-the-fly
func GetTrue()
{
	return true;
}

/*
Sets the menu this Clonk currently has focus of. Old menus that have been opened via SetMenu will be closed, making sure that only one menu is open at a time.
Additionally, the Clonk's control is disabled while a menu is open.
The menu parameter can either be an object that closes its menu via a Close() callback or it can be a menu ID as returned by GuiOpen. When /menu/ is such an ID,
the menu will be closed via GuiClose when a new menu is opened. If you need to do cleaning up, you will have to use the OnClose callback of the menu.
When you call SetMenu with a menu ID, you should also call clonk->MenuClosed(), once your menu is closed.
*/
func SetMenu(new_menu, bool unclosable)
{
	unclosable = unclosable ?? false;
	var current_menu = this.control.menu;
	
	// no news?
	if (new_menu) // if new_menu == nil, it is important that we still do the cleaning-up below even if we didn't have a menu before (see MenuClosed())
		if (current_menu == new_menu)
			return;
	
	// close old one!
	if (current_menu != nil)
	{
		if (GetType(current_menu) == C4V_C4Object)
			current_menu->Close();
		else if (GetType(current_menu) == C4V_PropList)
			GuiClose(current_menu.ID);
		else
			FatalError("Library_ClonkControl::SetMenu() was called with invalid parameter.");
	}
	else
	{
		// we have a new menu but didn't have another one before? Enable menu controls!
		if (new_menu)
		{
			this->~CancelUse();
			// stop clonk
			SetComDir(COMD_Stop);

			if (this->~HasVirtualCursor())
			{
				this->~VirtualCursor()->StartAim(this, 0, new_menu);
			}
			else
			{
				if (GetType(new_menu) == C4V_C4Object && new_menu->~CursorUpdatesEnabled())
					SetPlayerControlEnabled(GetOwner(), CON_GUICursor, true);

				SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, true);
				SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, true);
			}
		}
	}
	
	if (new_menu)
	{
		if (GetType(new_menu) == C4V_C4Object)
		{
			this.control.menu = new_menu;
		}
		else if (GetType(new_menu) == C4V_Int)
		{
			// add a proplist, so that it is always safe to call functions on clonk->GetMenu()
			this.control.menu =
			{
				ID = new_menu
			};
		}
		else
			FatalError("Library_ClonkControl::SetMenu called with invalid parameter!");

		// make sure the menu is unclosable even if it is just a GUI ID
		if (unclosable)
		{
			this.control.menu.Unclosable = Library_ClonkControl.GetTrue;
		}
	}
	else
	{
		// always disable cursors, even if no old menu existed, because it can happen that a menu removes itself and thus the Clonk never knows whether the cursors are active or not
		if (this->~HasVirtualCursor())
		{
			this->~RemoveVirtualCursor(); // for gamepads
		}
		SetPlayerControlEnabled(GetOwner(), CON_GUICursor, false);
		SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, false);
		SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, false);

		this.control.menu = nil;
	}
	return this.control.menu;
}

func MenuClosed()
{
	// make sure not to clean up the menu again
	this.control.menu = nil;
	// and remove cursors etc.
	SetMenu(nil);
}

/*
Returns the current menu or nil. If a menu is returned, it is always a proplist (but not necessarily an object).
Stuff like if (clonk->GetMenu()) clonk->GetMenu()->~IsClosable(); is always safe.
If you want to remove the menu, the suggested method is clonk->TryCancelMenu() to handle unclosable menus correctly.
*/
func GetMenu()
{
	// No new-style menu set? Return the classic menu ID. This is deprecated and should be removed in some future.
	// This function must return a proplist, but clashes with the engine-defined "GetMenu".
	// This workaround here at least allows developers to reach the Clonk's menu ID.
	if (this.control.menu == nil)
	{
		var menu_id = inherited(...);
		if (menu_id) return {ID = menu_id};
	}
	return this.control.menu;
}

// Returns true when an existing menu was closed
func CancelMenu()
{
	if (this.control.menu)
	{
		SetMenu(nil);
		return true;
	}
	
	return false;
}

// Tries to cancel a non-unclosable menu. Returns true when there is no menu left after this call (even if there never was one).
func TryCancelMenu()
{
	if (!this.control.menu)
		return true;
	if (this.control.menu->~Unclosable())
		return false;
	CancelMenu();
	return true;
}

public func RejectShiftCursor()
{
	if (this.control.menu && this.control.menu->~Unclosable())
		return true;
	return _inherited(...);
}

public func OnShiftCursor()
{
	TryCancelMenu();
	return _inherited(...);
}

