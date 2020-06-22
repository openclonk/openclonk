/**
	PlayerControl.c
	Functions to handle player controls (i.e. input keys).
	
	@author Newton
*/

static const CON_Gamepad_Deadzone = 60;
static CON_VC_Players;
static g_player_cursor_pos; // array of [x, y] pos arrays; indexed by player. last cursor pos as sent by CON_CursorPos

// PlayerControlRelease
// Called by engine whenever a control is issued
// Forwards control to special handler or cursor
// Return whether handled
// documented in /docs/sdk/script/fn
global func PlayerControl(int plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat, int status)
{
	var release = status == CONS_Up;
	//Log("%d, %s, %i, %d, %d, %d, %v, %v", plr, GetPlayerControlName(ctrl), spec_id, x, y, strength, repeat, status);
	// Control handled by definition? Forward
	if (spec_id) return spec_id->ForwardedPlayerControl(plr, ctrl, x, y, strength, repeat, status);

	// Forward control to player
	if (Control2Player(plr, ctrl, x, y, strength, repeat, status)) return true;

	// Forward control to cursor
	var cursor = GetCursor(plr);
	if (cursor && cursor->GetCrewEnabled())
	{
		// Object controlled by plr
		cursor->SetController(plr);
		
		// menu controls:
		
		if (cursor->~GetMenu() && status != CONS_Moved)
		{
			// direction keys are always forwarded to the menu
			// (because the clonk shall not move while the menu is open)
			var handled = true;
			if (ctrl == CON_Left)       cursor->GetMenu()->~ControlLeft(release);
			else if (ctrl == CON_Right) cursor->GetMenu()->~ControlRight(release);
			else if (ctrl == CON_Up)    cursor->GetMenu()->~ControlUp(release);
			else if (ctrl == CON_Down)  cursor->GetMenu()->~ControlDown(release);
			else handled = false;
			if (handled) return true;
		
			// cancel menu
			if (ctrl == CON_CancelMenu)
			{
				cursor->TryCancelMenu();
				return true;
			}

			if (ctrl == CON_GUIClick1 || ctrl == CON_GUIClick2 || ctrl == CON_GUICursor)
			{
				var menux = cursor->GetMenu()->~GetX();
				var menuy = cursor->GetMenu()->~GetY();
				
				var dx = x-menux;
				var dy = y-menuy;

				if (ctrl == CON_GUICursor)
				{
					cursor->GetMenu()->~UpdateCursor(dx, dy);
					return true;
				}
				else if (release == true)
				{
					cursor->GetMenu()->~OnMouseClick(dx, dy, ctrl == CON_GUIClick2);
					return false;
				}
			}		
		}
		
		// local coordinates
		var cursorX = x, cursorY = y;
		if (x != nil || y != nil)
		{
			cursorX -= cursor->GetX();
			cursorY -= cursor->GetY();
		}
		
		// Overload by effect?
		if (cursor->Control2Effect(plr, ctrl, cursorX, cursorY, strength, repeat, status)) return true;

		if (cursor->ObjectControl(plr, ctrl, cursorX, cursorY, strength, repeat, status))
		{
			if (cursor && status == CONS_Down && !repeat)
			{
				// non-mouse controls reset view
				if (!x && !y) ResetCursorView(plr);
			}
			return true;
		}
		//else Log("-- not handled");

	}
	
	// Nothing to handle control then
	return false;
}

global func InitializePlayerControl(int plr, string controlset_name, bool keyboard, bool mouse, bool gamepad)
{
	// VC = VirtualCursor
	if (!CON_VC_Players)
		CON_VC_Players = CreateArray();
		
	CON_VC_Players[plr] = !mouse;
	
	// for all clonks...
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
	{
		clonk->~ReinitializeControls();
	}
}

global func PlayerHasVirtualCursor(int plr)
{
	if (!CON_VC_Players)
		return false;
		
	return CON_VC_Players[plr];
}

// Control2Player
// Player-wide controls
global func Control2Player(int plr, int ctrl, int x, int y, int strength, bool repeat, int status)
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
		var crew = GetCrew(plr, hotkey-1);
		if (!crew) return false;
		// stop previously selected crew
		StopSelected();
		
		// set cursor if not disabled etc.
		return SetCursor(plr, crew);
	}
	
	// Modifier keys - do not handle the key. The GetPlayerControlState will still return the correct value when the key is held down.
	if (ctrl == CON_ModifierMenu1) return false;
		
	// cursor pos info - store in player values
	if (ctrl == CON_CursorPos)
	{
		if (!g_player_cursor_pos) g_player_cursor_pos = CreateArray(plr + 1);
		g_player_cursor_pos[plr] = [x, y];
		return true;
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

/* return info of last sent CON_CursorPos packet for that player as [x, y] */
global func GetPlayerCursorPos(int plr)
{
	if (!g_player_cursor_pos) return 0;
	return g_player_cursor_pos[plr];
}

global func StopSelected(int plr)
{
	var cursor = GetCursor(plr);
	if (cursor)
	{
		cursor->SetCommand("None");
		cursor->SetComDir(COMD_Stop);
	}
}

/* Object functions */
// To be called in an object context only!

// Control2Effect
// Call control function in all effects that have "Control" in their name
global func Control2Effect(int plr, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	// x and y are local coordinates
	if (!this) return false;
	
	// Count down from EffectCount, in case effects get deleted
	var i = GetEffectCount("*Control*", this), fx;
	while (i--)
	{
		fx = GetEffect("*Control*", this, i);
		if (fx)
			if (EffectCall(this, fx, "Control", ctrl, x, y, strength, repeat, status))
				return true;
	}
	// No effect handled the control
	return false;
}

// ObjectControl
// Called from PlayerControl when a control is issued to the cursor
// Return whether handled
// To be overloaded by specific objects to enable additional controls
global func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	if (!this) return false;
	
	// Any control resets a previously given command
	SetCommand("None");
	
	// Movement controls
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
		return ObjectControlMovement(plr, ctrl, strength, status, repeat);
	
	// Unhandled control
	return false;
}

// Find an object with an entrance in front of this object whose entrance is at
// the right position
global func GetEntranceObject()
{
	if (!this) return nil;

	// object with an entrance on target position
	var obj = FindObject(Find_OCF(OCF_Entrance), Find_Layer(GetObjectLayer()), 
	                     Find_AtPoint(0, 0), Find_Exclude(this));
	if (!obj) return nil;

	var x = obj->GetDefCoreVal("Entrance","DefCore",0) + obj->GetX();
	var y = obj->GetDefCoreVal("Entrance","DefCore",1) + obj->GetY();
	var wdt = obj->GetDefCoreVal("Entrance","DefCore",2);
	var hgt = obj->GetDefCoreVal("Entrance","DefCore",3);
		
	// entrance is on the vehicle?
	if (!Inside(GetX(), x, x + wdt)) return nil;
	if (!Inside(GetY(), y, y + hgt)) return nil;
	
	return obj;
}
global func NameComDir(comdir)
{
	if (comdir == COMD_Stop) return "COMD_Stop";
	if (comdir == COMD_Up) return "COMD_Up";
	if (comdir == COMD_UpRight) return "COMD_UpRight";
	if (comdir == COMD_UpLeft) return "COMD_UpLeft";
	if (comdir == COMD_Right) return "COMD_Right";
	if (comdir == COMD_Left) return "COMD_Left";
	if (comdir == COMD_Down) return "COMD_Down";
	if (comdir == COMD_DownRight) return "COMD_DownRight";
	if (comdir == COMD_DownLeft) return "COMD_DownLeft";
	if (comdir == COMD_None) return "COMD_None";
}
// Called when CON_Left/Right/Up/Down controls are issued/released
// Return whether handled
global func ObjectControlMovement(int plr, int ctrl, int strength, int status, bool repeat)
{
	if (!this) return false;
	
	// movement is only possible when not contained
	if (Contained()) return false;

	// this is for controlling movement with Analogpad
	if (status == CONS_Down)
		if (strength != nil && strength < CON_Gamepad_Deadzone)
			return true;
	
	var proc = GetProcedure();
	// Some specific movement controls
	if (status == CONS_Down)
	{
		// Jump control
		if (ctrl == CON_Jump)
		{
			if (proc == "WALK" && GetComDir() == COMD_Up)
				SetComDir(COMD_Stop);
			if (proc == "WALK")
			{
				this->ObjectCommand("Jump");
				return true;
			}
		}
		if (proc == "SWIM" && !GBackSemiSolid(0,-5)) //Water jump
		{
			if (ctrl == CON_Up) return false;
			else if (ctrl == CON_Jump) this->ObjectCommand("Jump");
		}
		if (proc == "SCALE") // Let go from scaling a wall
		{
			//Wall kick
			if (ctrl == CON_Left && GetDir() == DIR_Right && GetComDir() == COMD_Up)
				return this->ObjectCommand("Jump");
			if (ctrl == CON_Right && GetDir() == DIR_Left && GetComDir() == COMD_Up)
				return this->ObjectCommand("Jump");

			//Let go of wall
			if (ctrl == CON_Left && GetDir() == DIR_Right) return this->ObjectComLetGo(-10);
			if (ctrl == CON_Right && GetDir() == DIR_Left) return this->ObjectComLetGo(+10);
		}
		else if (proc == "HANGLE") // Let go from hangling the ceiling
		{
			if (ctrl == CON_Down) return this->ObjectComLetGo(0, 0);
		}
		// Direct turnaround if object is standing still. Valid for any procedure in OC
		if (!GetXDir())
		{
			if (ctrl == CON_Left) SetDir(DIR_Left);
			else if (ctrl == CON_Right) SetDir(DIR_Right);
		}
	}
	else // release
	{
		// If rolling, allow to instantly switch to walking again.
		if (GetAction() == "Roll")
		{
			if (ctrl == CON_Left && GetDir() == DIR_Left || ctrl == CON_Right && GetDir() == DIR_Right)
				SetAction("Walk");
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
		var old = ComDir2XY(old_comdir);
		var new = ComDir2XY(new_comdir);
		var old_cx = old[0], old_cy = old[1], new_cx = new[0], new_cy = new[1];
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
global func ShiftCursor(int plr, bool back, bool force)
{
	// Is the selected Clonk busy at the moment? E.g. uncloseable menu open..
	if (!force)
	{
		var cursor = GetCursor(plr);
		if (cursor && cursor->~RejectShiftCursor()) return false;
	}
	
	// get index of currently selected crew
	var index = 0;
	while (index < GetCrewCount(plr))
	{
		if (GetCursor(plr) == GetCrew(plr, index)) break;
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
	} while (cycle < maxcycle && !(GetCrew(plr, index)->GetCrewEnabled()));
	
	// Changing the cursor closes all menus that are associated with the old cursor.
	// However, if a menu is not closable, then it requires the attention of the player and switching the cursor is disabled..
	var current_cursor = GetCursor(plr);
	var new_cursor = GetCrew(plr, index);
	if (current_cursor == new_cursor) return false;
	
	StopSelected(plr);
	if (current_cursor)
		current_cursor->~OnShiftCursor(new_cursor);
		
	return SetCursor(plr, new_cursor);
}

// Temporarily used for Debugging!
// Helper function to turn CON_*-constants into strings
global func GetPlayerControlName(int ctrl)
{
	var con_name = GetConstantNameByValue(ctrl, "CON_");
	if (!con_name) con_name = Format("Unknown(%d)", ctrl);
	return con_name;
}

// Return COMD_*-constant corresponding to current state of passed directional controls
global func GetPlayerConDir(int plr, int con_left, int con_up, int con_right, int con_down)
{
	var x, y;
	if (GetPlayerControlState(plr, con_left))	--x;
	if (GetPlayerControlState(plr, con_up))		--y;
	if (GetPlayerControlState(plr, con_right)) ++x;
	if (GetPlayerControlState(plr, con_down))	++y;
	// Creating an array here for every keypress/release
	// Would be so cool to have this static const. Guenther?
	var dir_coms = [COMD_UpLeft, COMD_Up, COMD_UpRight, COMD_Left, COMD_Stop, COMD_Right, COMD_DownLeft, COMD_Down, COMD_DownRight];
	return dir_coms[y*3 + x+4];
}

// Returns coordinate directions associated with a COMD_Constant
global func ComDir2XY(int comd)
{
	// Creating an array here for every keypress/release
	// Would be so cool to have this static const. Guenther?
	return [[0, 0, 1, 1, 1, 0,-1,-1,-1][comd], [0,-1,-1, 0, 1, 1, 1, 0,-1][comd]];
}

global func ObjectCommand(string command, object target, int tx, int ty, object target2, /*any*/ data)
{
	// this function exists to be overloadable by ClonkControl.c4d
	if (!this)
		return;
	this->SetCommand(command, target, tx, ty, target2, data);
}

// Let go from scaling or hangling
global func ObjectComLetGo(int vx, int vy)
{
	if (!SetAction("Jump")) return false;
	SetXDir(vx); SetYDir(vy);
	return true;
}

/* Mouse Hovering */

// Engine callback when the mouse hovers over an object (entering) or stops hovering over an object (leaving).
// Either leaving, entering or both are set. They can be nil, but never both at the same time.
// dragged is an object being drag(&not dropped yet)
global func MouseHover(int player, object leaving, object entering, object dragged)
{
	// Leaving the hovering zone should be processed first.
	if (leaving)
		leaving->~OnMouseOut(player, dragged);
	// Then process entering a new hovering zone. 
	if (entering)
		entering->~OnMouseOver(player, dragged);
	return true;
}

/* Drag & Drop */

// Engine callback on drag & drop: gives the player, the dragged obect and the object which is being dropped(can be nil).
global func MouseDragDrop(int plr, object source, object target)
{
	//Log("MouseDragDrop(%d, %s, %s)", plr, source->GetName(), target->GetName());
	if (!source) return false; // can happen if source got deleted after control was queued
	var src_drag = source->~OnMouseDrag(plr);
	if (!src_drag) 
		return false;
	
	// If there is drop target, check whether it accepts the drop.
	if (target)
		if (!target->~OnMouseDrop(plr, src_drag))
			return false;

	// Notify the drop object it succeeded.
	if (source)
		source->~OnMouseDragDone(src_drag, target);
	return true;
}

/* Debug */

// uncomment this to get log messages for any player control issued
/*global func PlayerControl(int plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat, bool release)
{
	var r = inherited(plr, ctrl, spec_id, x, y, strength, repeat, release, ...), rs;
	if (r) rs = ""; else rs = "!";
	Log("%s%d, %s, %i, %d, %d, %d, %v, %v", rs, plr, GetPlayerControlName(ctrl), spec_id, x, y, strength, repeat, release);
	return r;
}*/

/*
This is used by Library_ClonkInventoryControl and needs to be a global function (in a non-appendto).
This function returns the priority of an object when selecting an object to pick up.
*/
global func Library_ClonkInventoryControl_Sort_Priority(int x_position)
{
	// Objects are sorted by position, preferring the direction of the key press.
	var priority_x = GetX() - x_position;
	if (priority_x < 0) priority_x += 1000;
	return priority_x; 
}
