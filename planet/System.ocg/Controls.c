/*--
		Controls.c
		Authors: boni
		
		Helper functions to find out if a Control fits into a certain category. (Throwing, using, interacting,...)
--*/

/** Control tells the clonk to move */
global func IsMovementControl(int ctrl)
{
	// up, up, down, down, left, right, left, right, B, A
	if(ctrl == CON_Up
	|| ctrl == CON_Down
	|| ctrl == CON_Left
	|| ctrl == CON_Right)
		return true;
	
	return false;
}

/** Control throws selected item */
global func IsThrowControl(int ctrl)
{
	// right mouse button
	if(ctrl == CON_Throw)
		return true;
	
	return false;
}

/** Control drops items from inventory (hotkey or selected items) */
global func IsDropControl(int ctrl)
{
	// selected items
	if(ctrl == CON_Drop
	// hotkeys
	|| ctrl == CON_DropHotkey0
	|| ctrl == CON_DropHotkey1
	|| ctrl == CON_DropHotkey2
	|| ctrl == CON_DropHotkey3
	|| ctrl == CON_DropHotkey4
	|| ctrl == CON_DropHotkey5
	|| ctrl == CON_DropHotkey6
	|| ctrl == CON_DropHotkey7
	|| ctrl == CON_DropHotkey8
	|| ctrl == CON_DropHotkey9)
		return true;
	
	return false;
}

/** Control has the goal of interacting with some other object (Interaction, Grabbing, Entering,...) */
global func IsInteractionControl(int ctrl)
{
	// Interaction itself
	if(ctrl == CON_Interact
	// hotkeys
	|| ctrl == CON_InteractionHotkey0
	|| ctrl == CON_InteractionHotkey1
	|| ctrl == CON_InteractionHotkey2
	|| ctrl == CON_InteractionHotkey3
	|| ctrl == CON_InteractionHotkey4
	|| ctrl == CON_InteractionHotkey5
	|| ctrl == CON_InteractionHotkey6
	|| ctrl == CON_InteractionHotkey7
	|| ctrl == CON_InteractionHotkey8
	|| ctrl == CON_InteractionHotkey9)
		return true;
	
	return false;
}

/** Control has the goal of switching the currently selected crewmember */
global func IsCrewControl(int ctrl)
{
	// next/previous
	if(ctrl == CON_NextCrew
	|| ctrl == CON_PreviousCrew
	// hotkeys
	|| ctrl == CON_PlayerHotkey0
	|| ctrl == CON_PlayerHotkey1
	|| ctrl == CON_PlayerHotkey2
	|| ctrl == CON_PlayerHotkey3
	|| ctrl == CON_PlayerHotkey4
	|| ctrl == CON_PlayerHotkey5
	|| ctrl == CON_PlayerHotkey6
	|| ctrl == CON_PlayerHotkey7
	|| ctrl == CON_PlayerHotkey8
	|| ctrl == CON_PlayerHotkey9
	)
		return true;
	
	return false;
}

/** Control uses selected item */
global func IsUseControl(int ctrl)
{
	if (ctrl == CON_Use || ctrl == CON_UseAlt) return true;
	return false;
}
