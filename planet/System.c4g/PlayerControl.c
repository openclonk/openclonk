/*--
		PlayerControl.c
		Authors: Newton
		
		Functions to handle player controls (i.e. input keys)
--*/


static const CON_Gamepad_Deadzone = 60;
static CON_VC_Players;

// PlayerControlRelease
// Called by engine whenever a control is issued
// Forwards control to special handler or cursor
// Return whether handled
global func PlayerControl(int plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat, bool release)
{
	//Log("%d, %s, %i, %d, %d, %d, %v, %v", plr, GetPlayerControlName(ctrl), spec_id, x,y,strength, repeat, release);
	// Control handled by definition? Forward
	if (spec_id) return spec_id->PlayerControl(plr, ctrl, x, y, strength, repeat, release);

	// Forward control to player
	if (Control2Player(plr,ctrl, x, y, strength, repeat, release)) return true;

	// Forward control to cursor
	var cursor = GetCursor(plr);
	if (cursor) 
	{
		// Object controlled by plr
		cursor->SetController(plr);
		
		// local coordinates
		var cursorX = x, cursorY = y;
		if(x != nil || y != nil)
		{
			cursorX -= cursor->GetX();
			cursorY -= cursor->GetY();
		}
		
		// Overload by effect?
		if (cursor->Control2Effect(plr, ctrl, cursorX, cursorY, strength, repeat, release)) return true;

		if (cursor->ObjectControl(plr, ctrl, cursorX, cursorY, strength, repeat, release))
		{
			if (cursor && !release && !repeat)
			{
				cursor->DoNoCollectDelay(-1);
				// non-mouse controls reset view
				if (!x && !y) ResetCursorView(plr);
			}
			return true;
		}

	}
	// No cursor? Nothing to handle control then
	return false;
}

global func InitializePlayerControl(int plr, string controlset_name, bool keyboard, bool mouse, bool gamepad)
{
	// VC = VirtualCursor
	if(!CON_VC_Players)
		CON_VC_Players = CreateArray();
		
	CON_VC_Players[plr] = !mouse;
	
	// for all clonks...
	for(var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
	{
		clonk->~ReinitializeControls();
	}
}

global func PlayerHasVirtualCursor(int plr)
{
	if(!CON_VC_Players)
		return false;
		
	return CON_VC_Players[plr];
}

// Control2Player
// Player-wide controls
global func Control2Player(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	// select previous or next
	if (ctrl == CON_PreviousCrew)
		return ShiftCursor(plr, true);
	if (ctrl == CON_NextCrew)
		return ShiftCursor(plr, false);
		
	// all those hotkeys...
	var hotkey = 0;
	if (ctrl == CON_PlayerHotkey0) hotkey = 10;
	if (ctrl == CON_PlayerHotkey1) hotkey = 1;
	if (ctrl == CON_PlayerHotkey2) hotkey = 2;
	if (ctrl == CON_PlayerHotkey3) hotkey = 3;
	if (ctrl == CON_PlayerHotkey4) hotkey = 4;
	if (ctrl == CON_PlayerHotkey5) hotkey = 5;
	if (ctrl == CON_PlayerHotkey6) hotkey = 6;
	if (ctrl == CON_PlayerHotkey7) hotkey = 7;
	if (ctrl == CON_PlayerHotkey8) hotkey = 8;
	if (ctrl == CON_PlayerHotkey9) hotkey = 9;
	
	if (hotkey > 0)
	{
		// valid crew number?
		var crew = GetCrew(plr,GetCrewCount()-hotkey);
		if (!crew) return false;
		// stop previously selected crew
		StopSelected();
		
		// set cursor if not disabled etc.
		UnselectCrew(plr);
		return SelectCrew(plr,crew, true);
	}
	
	// select the complete crew
	if (ctrl == CON_AllCrew)
	{
		for(var i = 0; i < GetCrewCount(plr); ++i)
		{
			var crew = GetCrew(plr,i);
			SelectCrew(plr,crew, true);
		}
		StopSelected(plr);
	}
	/*
	if (ctrl == CON_Test)
	{
		Message(Format("%d %d (%d %d) %d [%d %d]", plr, ctrl, x, y, strength, repeat, release));
		return true;
	}
	*/
	return false;
}

global func StopSelected(int plr)
{
	var cursor;
	for(var i = 0; cursor = GetCursor(plr,i); ++i)
	{
		cursor->SetCommand("None");
		cursor->SetComDir(COMD_Stop);
	}
}

/* Object functions */
// To be called in an object context only!

// Control2Effect
// Call control function in all effects that have "Control" in their name
global func Control2Effect(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	// x and y are local coordinates
	if (!this) return false;
	
	// Count down from EffectCount, in case effects get deleted
	var i = GetEffectCount("*Control*", this), iEffect;
	var res;
	while (i--)
		{
		iEffect = GetEffect("*Control*", this, i);
		if ( GetEffect(0, this, iEffect, 1) )
			if (EffectCall(this, iEffect, "Control", ctrl, x,y,strength, repeat, release))
				return true;
		}
	// No effect handled the control
	return false;
}

// ObjectControl
// Called from PlayerControl when a control is issued to the cursor
// Return whether handled
// To be overloaded by specific objects to enable additional controls
global func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) return false;
	
	// Any control resets a previously given command
	SetCommand("None");
	
	// Movement controls
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
		return ObjectControlMovement(plr, ctrl, strength, release);
	
	// Unhandled control
	return false;
}

// Find an object with an entrance in front of this object whose entrance is at
// the right position
global func GetEntranceObject()
{
	if (!this) return nil;

	// object with an entrance on target position
	var obj = FindObject(Find_OCF(OCF_Entrance), Find_AtPoint(0,0), Find_Exclude(this));
	if (!obj) return nil;

	var x = obj->GetDefCoreVal("Entrance","DefCore",0) + obj->GetX();
	var y = obj->GetDefCoreVal("Entrance","DefCore",1) + obj->GetY();
	var wdt = obj->GetDefCoreVal("Entrance","DefCore",2);
	var hgt = obj->GetDefCoreVal("Entrance","DefCore",3);
		
	// entrance is on the vehicle?
	if (!Inside(GetX(), x, x+wdt)) return nil;
	if (!Inside(GetY(), y, y+hgt)) return nil;
	
	return obj;
}

// Called when CON_Left/Right/Up/Down controls are issued/released
// Return whether handled
global func ObjectControlMovement(int plr, int ctrl, int strength, bool release)
{
	if (!this) return false;

	// this is for controlling movement with Analogpad
	if(!release)
		if(strength != nil && strength < CON_Gamepad_Deadzone)
			return true;
	
	var proc = GetProcedure();
	// Some specific movement controls
	if (!release)
	{
		// Jump control
		if (ctrl == CON_Jump)
		{
			return PlayerObjectCommand(plr, false, "Jump");
		}
		if (proc == "SCALE") // Let go from scaling a wall
		{
			if (ctrl == CON_Left && GetDir() == DIR_Right) return ObjectComLetGo(-10);
			if (ctrl == CON_Right && GetDir() == DIR_Left) return ObjectComLetGo(+10);
		}
		else if (proc == "HANGLE") // Let go from hangling the ceiling
		{
			if (ctrl == CON_Down) return ObjectComLetGo(0,0);
		}
		// Make sure other selected Clonks are following
		PlayerObjectCommand(plr, true, "Follow", this, GetX(), GetY());
		// Direct turnaround if object is standing still. Valid for any procedure in OC
		if (!GetXDir())
		{
			if (ctrl == CON_Left) SetDir(DIR_Left);
			else if (ctrl == CON_Right) SetDir(DIR_Right);
		}
	}
	return ObjectControlUpdateComdir(plr);
}

// Updates ComDir of object based on current Con_*-directional controls
// Return whether actual, effective direction of movement changed
global func ObjectControlUpdateComdir(int plr)
{
	if (!this) return false;

	// Generic movement: Update ComDir based on current control state
	var new_comdir = GetPlayerConDir(plr, CON_Left, CON_Up, CON_Right, CON_Down);
	var old_comdir = GetComDir();
	if (new_comdir != old_comdir)
	{
		// ComDir changed. Update.
		SetComDir(new_comdir);
		//var s = "";
		//if (GetPlayerControlState(plr, CON_Left))	s = Format("%sL", s);
		//if (GetPlayerControlState(plr, CON_Up))		s = Format("%sU", s);
		//if (GetPlayerControlState(plr, CON_Right)) s = Format("%sR", s);
		//if (GetPlayerControlState(plr, CON_Down))	s = Format("%sD", s);
		//s = Format("%s %s", s, ["Stop", "Up", "UpRight", "Right", "DownRight", "Down", "DownLeft", "Left", "UpLeft"][new_comdir]);
		//Message("@%s", this, s);
		// The control is only handled if it had an actual effect on the current movement direction of the Clonk
		var old_cx, old_cy, new_cx, new_cy;
		ComDir2XY(old_comdir, old_cx, old_cy);
		ComDir2XY(new_comdir, new_cx, new_cy);
		var is_handled;
		var proc = GetProcedure();
		if (proc == "WALK" || proc == "HANGLE" || proc == "PUSH" || proc == "PULL" || proc == "FLIGHT")
			// Only horizontal movement changed actual direction
			// Also, enforce clear Left/Right commands without leftover Up/Down
			// CON_Down is never handled this way, thus forcing a CON_Stop
			is_handled = (old_cx != new_cx) && !new_cy;
		else if (proc == "SCALE")
			// Only vertical movement changed actual direction
			// Also, enfore clear Up/Down to prevent "Zuppel" in corner
			is_handled = (old_cy != new_cy) && !new_cx;
		else if (proc == "SWIM" || proc == "FLOAT" || proc == "DIG")
			is_handled = (old_cx != new_cx || old_cy != new_cy); // Free 360 degree movement
		else
			is_handled = false;
		return is_handled;
	}
	else
	{
		// ComDir did not change. -> The control was not handled
		//Log("NoChange");
		return false;
	}
}

// selects the next/previous crew member (that is not disabled)
global func ShiftCursor(int plr, bool back)
{
	// get index of currently selected crew
	var index = 0;
	while (index < GetCrewCount(plr))
	{
		if (GetCursor(plr) == GetCrew(plr,index)) break;
		index++;
	}
	
	// a short explanation here:
	// when shifting the cursor to the next crew member, this crew member
	// might be disabled via SetCrewEnabled. That is why we have to go to
	// the crew after the next and perhaps even the crew after the next
	// after the next. Also, we need to stop this skipping after all crew
	// members have been checked in the special case that all crew members
	// are disabled
	var maxcycle = GetCrewCount(plr);
	var cycle = 0;

	do {
		if (back)
		{
			--index;
			if (index < 0) index = GetCrewCount(plr)-1;
		}
		else
		{
			++index;
			if (index >= GetCrewCount(plr)) index = 0;
		}
		++cycle;
	} while (!(GetCrew(plr,index)->GetCrewEnabled()) && cycle < maxcycle);

	StopSelected();

	UnselectCrew(plr);
	return SelectCrew(plr, GetCrew(plr,index), true);
}

// Temporarily used for Debugging!
// Helper function to turn CON_*-constants into strings
global func GetPlayerControlName(int ctrl)
{
	if (ctrl == CON_Aim)		return "Aim";
	if (ctrl == CON_AimLeft)		return "AimLeft";
	if (ctrl == CON_AimRight)		return "AimRight";
	if (ctrl == CON_AimUp)		return "AimUp";
	if (ctrl == CON_AimDown)		return "AimDown";
	if (ctrl == CON_AimAxisLeft)		return "AimAxisLeft";
	if (ctrl == CON_AimAxisRight)		return "AimAxisRight";
	if (ctrl == CON_AimAxisUp)		return "AimAxisUp";
	if (ctrl == CON_AimAxisDown)		return "AimAxisDown";
	if (ctrl == CON_Left)		return "Left";
	if (ctrl == CON_Right)		return "Right";
	if (ctrl == CON_Up)			return "Up";
	if (ctrl == CON_Down)		return "Down";
	if (ctrl == CON_Throw)		return "Throw";
	if (ctrl == CON_ThrowAlt)		return "ThrowAlt";
	if (ctrl == CON_ThrowDelayed)		return "ThrowDelayed";
	if (ctrl == CON_ThrowAltDelayed)		return "ThrowAltDelayed";
	if (ctrl == CON_Jump)		return "Jump";
	if (ctrl == CON_MenuLeft)	return "MenuLeft";
	if (ctrl == CON_MenuRight)	return "MenuRight";
	if (ctrl == CON_MenuOK)		return "MenuOK";
	if (ctrl == CON_MenuUp)		return "MenuUp";
	if (ctrl == CON_MenuDown)	return "MenuDown";
	if (ctrl == CON_NextItem)	return "NextItem";
	if (ctrl == CON_PreviousItem) return "PreviousItem";
	if (ctrl == CON_NextAltItem)	return "NextAltItem";
	if (ctrl == CON_PreviousAltItem) return "PreviousAltItem";
	if (ctrl == CON_Use)		return "Use";
	if (ctrl == CON_Drop)		return "Drop";
	if (ctrl == CON_UseAlt)		return "UseAlt";
	if (ctrl == CON_UseAltDelayed)		return "UseAltDelayed";
	if (ctrl == CON_UseDelayed)		return "UseDelayed";
	if (ctrl == CON_DropAlt)		return "DropAlt";
	if (ctrl == CON_NextCrew)	return "NextCrew";
	if (ctrl == CON_PreviousCrew) return "PreviousCrew";
	if (ctrl == CON_MenuCancel)	return "MenuCancel";
	if (ctrl == CON_PlayerMenu)	return "PlayerMenu";
	if (ctrl == CON_GrabNext)	return "GrabNext";
	if (ctrl == CON_GrabPrevious) return "GrabPrevious";
	if (ctrl == CON_Grab)		return "Grab";
	if (ctrl == CON_Ungrab)		return "Ungrab";
	if (ctrl == CON_AllCrew)	return "AllCrew";
	if (ctrl == CON_PushEnter)	return "PushEnter";
	if (ctrl == CON_Enter)		return "Enter";
	if (ctrl == CON_Exit)		return "Exit";
	if (ctrl == CON_Hotkey0)	return "Hotkey0";
	if (ctrl == CON_Hotkey1)	return "Hotkey1";
	if (ctrl == CON_Hotkey2)	return "Hotkey2";
	if (ctrl == CON_Hotkey3)	return "Hotkey3";
	if (ctrl == CON_Hotkey4)	return "Hotkey4";
	if (ctrl == CON_Hotkey5)	return "Hotkey5";
	if (ctrl == CON_Hotkey6)	return "Hotkey6";
	if (ctrl == CON_Hotkey7)	return "Hotkey7";
	if (ctrl == CON_Hotkey8)	return "Hotkey8";
	if (ctrl == CON_Hotkey9)	return "Hotkey9";
	if (ctrl == CON_PlayerHotkey0)	return "PlayerHotkey0";
	if (ctrl == CON_PlayerHotkey1)	return "PlayerHotkey1";
	if (ctrl == CON_PlayerHotkey2)	return "PlayerHotkey2";
	if (ctrl == CON_PlayerHotkey3)	return "PlayerHotkey3";
	if (ctrl == CON_PlayerHotkey4)	return "PlayerHotkey4";
	if (ctrl == CON_PlayerHotkey5)	return "PlayerHotkey5";
	if (ctrl == CON_PlayerHotkey6)	return "PlayerHotkey6";
	if (ctrl == CON_PlayerHotkey7)	return "PlayerHotkey7";
	if (ctrl == CON_PlayerHotkey8)	return "PlayerHotkey8";
	if (ctrl == CON_PlayerHotkey9)	return "PlayerHotkey9";
	
	return Format("Unknown(%d)", ctrl);
}

// Return COMD_*-constant corresponding to current state of passed directional controls
global func GetPlayerConDir(int plr, int con_left, int con_up, int con_right, int con_down)
{
	var x,y;
	if (GetPlayerControlState(plr, con_left))	--x;
	if (GetPlayerControlState(plr, con_up))		--y;
	if (GetPlayerControlState(plr, con_right)) ++x;
	if (GetPlayerControlState(plr, con_down))	++y;
	// Creating an array here for every keypress/release
	// Would be so cool to have this static const. Guenther?
	var dir_coms = [COMD_UpLeft, COMD_Up, COMD_UpRight, COMD_Left, COMD_None, COMD_Right, COMD_DownLeft, COMD_Down, COMD_DownRight];
	return dir_coms[y*3+x+4];
}

// Returns coordinate directions associated with a COMD_Constant
global func ComDir2XY(int comd, &x, &y)
{
	// Creating an array here for every keypress/release
	// Would be so cool to have this static const. Guenther?
	x = [0,0,1,1,1,0,-1,-1,-1][comd];
	y = [0,-1,-1,0,1,1,1,0,-1][comd];
	return true;
}

// Give a command to all selected Clonks of a player
global func PlayerObjectCommand(int plr, bool exclude_cursor, string command, object target, int tx, int ty, object target2)
{
	for (var i=exclude_cursor; i<GetSelectCount(plr); ++i)
	{
		var follow_clonk = GetCursor(plr, i);
		if (follow_clonk)
		{
			follow_clonk->ObjectCommand(command,target,tx,ty,target2);
		}
	}
	return true;
}

global func ObjectCommand(string command, object target, int tx, int ty, object target2)
{
	if(!this) return;
	this->SetCommand(command,target,tx,ty, target2);
}

// Let go from scaling or hangling
global func ObjectComLetGo(int vx, int vy)
{
	if (!SetAction("Jump")) return false;
	SetXDir(vx); SetYDir(vy);
	return true;
}


/* Drag & Drop */

global func MouseDragDrop(int plr, object source, object target)
{
	//Log("MouseDragDrop(%d, %v, %v)", plr, source, target);
	var src_drag = source->MouseDrag(plr);
	if (!src_drag) return false;
	if (target)
	{
		if (!target->MouseDrop(plr, src_drag)) return false;
	}
	if (source) source->MouseDragDone(src_drag, target);
	return true;
}
