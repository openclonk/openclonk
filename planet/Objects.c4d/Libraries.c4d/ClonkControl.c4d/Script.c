/*
	Standard clonk controls
	Author: Newton
	
	This object provides handling of the clonk controls including item
	management, backpack controls and standard throwing behaviour. It
	should be included into any clonk definition.
	The controls in System.c4g/PlayerControl.c only provide basic movement
	handling, namely the movement left, right, up and down. The rest is
	handled here:
	Grabbing, ungrabbing, shifting and pushing vehicles into buildings;
	entering and exiting buildings; throwing, dropping; backpack control,
	(object) menu control, hotkey controls, usage and it's callbacks and
	forwards to script. Also handled by this library is the aiming with
	the gamepad conrols.
	
	Objects that inherit this object need to return _inherited() in the
	following callbacks (if defined):
		Construction, Collection2, Ejection, RejectCollect, Departure,
		Entrance, AttachTargetLost, GrabLost, CrewSelection, Death,
		Destruction
	
	The following callbacks are made to other objects:
		*Stop
		*Left, *Right, *Up, *Down
		*Use, *UseStop, *UseStart, *UseHolding, *UseCancel
	wheras * is 'Contained' if the clonk is contained and otherwise (riding,
	pushing, to self) it is 'Control'. 	The item in the inventory only gets
	the Use*-calls. If the callback is handled, you should return true.
	Currently, this is explained more in detail here:
	http://forum.openclonk.org/topic_show.pl?tid=337
	
	The inventory management:
	The objects in the inventory are saved (parallel to Contents()) in the
	array 'inventory'. They are accessed via GetItem(i) and GetItemPos(obj).
	Other functions are MaxContentsCount() (defines the maximum number of
	contents)
*/

/* ++++++++++++++++++++++++ Clonk Inventory Control ++++++++++++++++++++++++ */

local indexed_inventory;
local disableautosort;
local inventory;

/* Item limit */

private func HandObjects() { return 2; }
public func MaxContentsCount() { return 7; }

/* Get the ith item in the inventory */
// the first two are in the hands
public func GetItem(int i)
{
	if (i >= GetLength(inventory))
		return nil;
		
	return inventory[i];
}

/* Search the index of an item */

public func GetItemPos(object item)
{
	if (item)
		if (item->Contained() == this)
		{
			var i = 0;
			for(var obj in inventory)
			{
				if (obj == item) return i;
				++i;
			}
		}
	return nil;
}

/* Switch two items in the clonk's inventory */

public func Switch2Items(int one, int two)
{
	// no valid inventory index: cancel
	if (!Inside(one,0,MaxContentsCount(one)-1)) return;
	if (!Inside(two,0,MaxContentsCount(two)-1)) return;

	// switch them around
	var temp = inventory[one];
	inventory[one] = inventory[two];
	inventory[two] = temp;
	
	// callbacks: cancel use
	if (using == inventory[one] || using == inventory[two])
		CancelUse();
	
	// callbacks: (de)selection
	if (one < HandObjects())
		if (inventory[two]) inventory[two]->~Deselection(this,one);
	if (two < HandObjects())
		if (inventory[one]) inventory[one]->~Deselection(this,two);
		
	if (one < HandObjects())
		if (inventory[one]) inventory[one]->~Selection(this,one);
	if (two < HandObjects())
		if (inventory[two]) inventory[two]->~Selection(this,two);
	
	// callbacks: to self, for HUD
	if (one < HandObjects())
	{
		if (inventory[one])
			this->~OnSlotFull(one);
		else
			this->~OnSlotEmpty(one);
	}
	if (two < HandObjects())
	{
		if (inventory[two])
			this->~OnSlotFull(two);
		else
			this->~OnSlotEmpty(two);
	}
}

/* Overload of Collect function */

public func Collect(object item, bool ignoreOCF, int pos)
{
	if (pos == nil) return _inherited(item,ignoreOCF);
	// fail if the specified slot is full
	if (GetItem(pos) != nil) return false;
	if (!item) return false;
	
	pos = BoundBy(pos,0,MaxContentsCount()-1);
	
	disableautosort = true;
	// collect but do not sort in_
	// Collection2 will be called which attempts to automatically sort in
	// the collected item into the next free inventory slot. Since 'pos'
	// is given as a parameter, we don't want that to happen and sort it
	// in manually afterwards
	var success = _inherited(item);
	disableautosort = false;
	if (success)
	{
		inventory[pos] = item;
		if (pos < HandObjects())
			this->~OnSlotFull(pos);
	}
		
	return success;
}

// disable ShiftContents for objects with ClonkControl.c4d

global func ShiftContents()
{
	if (this)
		if (this->~HandObjects() != nil)
			return false;
	return _inherited(...);
}

/* ################################################# */

protected func Construction()
{
	menu = nil;

	// inventory variables
	indexed_inventory = 0;
	inventory = CreateArray();

	// using variables
	alt = false;
	using = nil;

	return _inherited(...);
}

protected func Collection2(object obj)
{
	var sel;

	// See Collect()
	if (disableautosort) return _inherited(obj,...);
	
	var success = false;
	
	// into selected area if empty
	if (!inventory[0])
	{
		inventory[0] = obj;
		success = true;
	}
	// otherwise, next if empty
	else
	{
		for(var i = 1; i < MaxContentsCount(); ++i)
		{
			sel = i % MaxContentsCount();
			if (!inventory[sel])
			{
				indexed_inventory++;
				inventory[sel] = obj;
				success = true;
				break;
			}
		}
	}
	// callbacks
	if (success)
		if (sel < HandObjects())
			this->~OnSlotFull(sel);
	
	if (sel == 0 || sel == 1)
		obj->~Selection(this,sel == 1);

	return _inherited(obj,...);
}

protected func Ejection(object obj)
{
	// if an object leaves this object
	
	// find obj in array and delete (cancel using too)
	var i = 0;
	var success = false;
	for(var item in inventory)
	{
		if (obj == item) 
		{
			inventory[i] = nil;
			indexed_inventory--;
			success = true;
			break;
		}
		++i;
	}
	if (using == obj) CancelUse();

	// callbacks
	if (success)
		if (i < HandObjects())
			this->~OnSlotEmpty(i);
	
	if (i == 0 || i == 1)
		obj->~Deselection(this,i == 1);
	
	// we have over-weight? Put the next unindexed object inside that slot
	// this happens if the clonk is stuffed full with items he can not
	// carry via Enter, CreateContents etc.
	if (ContentsCount() > indexed_inventory && !inventory[i])
	{
		for(var c = 0; c < ContentsCount(); ++c)
		{
			if (GetItemPos(Contents(c)) == nil)
			{
				// found it! Collect it properly
				inventory[i] = Contents(c);
				indexed_inventory++;
				
				if (i < HandObjects())
					this->~OnSlotFull(i);
				
				if (i == 0 || i == 1)
					Contents(c)->~Selection(this,i == 1);
					
				break;
			}
		}
	}
	
	_inherited(obj,...);
}

protected func RejectCollect(id objid, object obj)
{
	// try to stuff obj into an object with an extra slot
	for(var i=0; Contents(i); ++i)
		if (Contents(i)->~HasExtraSlot())
			if (!(Contents(i)->Contents(0)))
				if (Contents(i)->Collect(obj,true))
					return true;
					
	// try to stuff an object in clonk into obj if it has an extra slot
	if (obj->~HasExtraSlot())
		if (!(obj->Contents(0)))
			for(var i=0; Contents(i); ++i)
				if (obj->Collect(Contents(i),true))
					return false;
			

	// check max contents
	if (ContentsCount() >= MaxContentsCount()) return true;

	// check if the two first slots are full. If the overloaded
	// Collect() is called, this check will be skipped
	if (!disableautosort)
		if (GetItem(0) && GetItem(1))
			return true;
	
	return _inherited(objid,obj,...);
}

/* ################################################# */

// The using-command hast to be canceled if the clonk is entered into
// or exited from a building.

protected func Entrance()         { CancelUse(); return _inherited(...); }
protected func Departure()        { CancelUse(); return _inherited(...); }

// The same for vehicles
protected func AttachTargetLost() { CancelUse(); return _inherited(...); }
protected func GrabLost()         { CancelUse(); return _inherited(...); }

// ...aaand the same for when the clonk is deselected
protected func CrewSelection(bool unselect)
{
	if (unselect) 
	{
		// cancel usage on unselect first...
		CancelUse();
		// and if there is still a menu, cancel it too...
		CancelMenu();
	}
	return _inherited(unselect,...);
}

protected func Destruction()
{
	// close open menus, cancel usage...
	CancelUse();
	CancelMenu();
	return _inherited(...);
}

protected func Death()
{
	// close open menus, cancel usage...
	CancelUse();
	CancelMenu();
	return _inherited(...);
}

// TODO: what is missing here is a callback for when the clonk StarTs a attach or push
// action.
// So if a clonk e.g. uses a tool and still while using it (holding down the mouse button)
// hits SPACE (grab vehicle), ControlUseStop is not called to the tool. 
// the workaround for now is that the controls do not allow to grab a vehicle while still
// holding down the mouse button. But this does not cover the (seldom?) case that the clonk
// is put into a grabbing/attached action via Script.



/* +++++++++++++++++++++++++++ Clonk Control +++++++++++++++++++++++++++ */

local using;
local alt;
local mlastx, mlasty;
local virtual_cursor;
local noholdingcallbacks;

/* Main control function */
public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) return false;
	
	//Log(Format("%d, %d, %s, strength: %d, repeat: %v, release: %v",  x,y,GetPlayerControlName(ctrl), strength, repeat, release),this);
	
	// open / close backpack
	if (ctrl == CON_Backpack)
	{
		if (GetMenu())
		{
			GetMenu()->Close();
		}
		else if(MaxContentsCount() > 2)
		{
			// Cancel usage
			CancelUse();
			CreateRingMenu(nil,this);
			// CreateRingMenu calls SetMenu(this) in the clonk,
			// so after this call menu = the created menu
			
			// for all contents in the clonks except the first two (hand slots)
			for(var i = 2; i < MaxContentsCount(); ++i)
			{
				// put them in the menu
				var item = GetItem(i);
				GetMenu()->AddItem(item,nil,i);
			}
			// finally, show the menu.
			GetMenu()->Show();
		}
		return true;
	}
	
	/* aiming with mouse:
	   The CON_Aim control is transformed into a use command. Con_Use if
	   repeated does not bear the updated x,y coordinates, that's why this
	   other control needs to be issued and transformed. CON_Aim is a
	   control which is issued on mouse move but disabled when not aiming
	   or when HoldingEnabled() of the used item does not return true.
	   For the rest of the control code, it looks like the x,y coordinates
	   came from CON_Use.
	  */
	if (using && ctrl == CON_Aim)
	{
		if (alt) ctrl = CON_UseAlt;
		else     ctrl = CON_Use;
				
		repeat = true;
		release = false;
	}
	// controls except a few reset a previously given command
	else SetCommand("None");
	
	/* aiming with analog pad or keys:
	   This works completely different. There are CON_AimAxis* and CON_Aim*,
	   both work about the same. A virtual cursor is created which moves in a
	   circle around the clonk and is controlled via these CON_Aim* functions.
	   CON_Aim* is normally on the same buttons as the movement and has a
	   higher priority, thus is called first. The aim is always done, even if
	   the clonk is not aiming. However this returns only true (=handled) if
	   the clonk is really aiming. So if the clonk is not aiming, the virtual
	   cursor aims into the direction in which the clonk is running and e.g.
	   CON_Left is still called afterwards. So if the clonk finally starts to
	   aim, the virtual cursor already aims into the direction in which he ran
	*/
	if (ctrl == CON_AimAxisUp || ctrl == CON_AimAxisDown || ctrl == CON_AimAxisLeft || ctrl == CON_AimAxisRight
	 || ctrl == CON_AimUp     || ctrl == CON_AimDown     || ctrl == CON_AimLeft     || ctrl == CON_AimRight)
	{
		var success = VirtualCursor()->Aim(ctrl,this,strength,repeat,release);
		// in any case, CON_Aim* is called but it is only successful if the virtual cursor is aiming
		return success && VirtualCursor()->IsAiming();
	}
	
	// save last mouse position:
	// if the using has to be canceled, no information about the current x,y
	// is available. Thus, the last x,y position needs to be saved
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		mlastx = x;
		mlasty = y;
	}
	
	// hotkeys (inventory, vehicle and structure control)
	var hot = 0;
	if (ctrl == CON_Hotkey0) hot = 10;
	if (ctrl == CON_Hotkey1) hot = 1;
	if (ctrl == CON_Hotkey2) hot = 2;
	if (ctrl == CON_Hotkey3) hot = 3;
	if (ctrl == CON_Hotkey4) hot = 4;
	if (ctrl == CON_Hotkey5) hot = 5;
	if (ctrl == CON_Hotkey6) hot = 6;
	if (ctrl == CON_Hotkey7) hot = 7;
	if (ctrl == CON_Hotkey8) hot = 8;
	if (ctrl == CON_Hotkey9) hot = 9;
	
	if (hot > 0)
	{
		this->~ControlHotkey(hot-1);
		return true;
	}
	
	var proc = GetProcedure();

	// cancel usage
	if (using && ctrl == CON_CancelUse)
	{
		CancelUse();
		return true;
	}
	
	// building, vehicle, contents menu control
	var house = Contained();
	var vehicle = GetActionTarget();
	var contents = GetItem(0);
	var contents2 = GetItem(1);
	
	if (menu)
	{
		return Control2Menu(ctrl, x,y,strength, repeat, release);
	}
	if (house)
	{
		return Control2Script(ctrl, x, y, strength, repeat, release, "Contained", house);
	}
	if (vehicle && proc == "PUSH")
	{
		// control to grabbed vehicle
		return Control2Script(ctrl, x, y, strength, repeat, release, "Control", vehicle);
	}
	if (vehicle && proc == "ATTACH")
	{
		// control to horse or something
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", vehicle))
			return true;
		// only if the horse does handle the control, it is not forwarded
		// to the items in the clonk
	}
	// out of convencience we call Control2Script, even though it handles
	// left, right, up and down, too. We don't want that, so this is why we
	// check that ctrl is Use.
	// Release commands are always forwarded even if contents is 0, in case we
	// need to cancel use of an object that left inventory
	if ((contents || (release && using)) && (ctrl == CON_Use || ctrl == CON_UseDelayed))
	{
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", contents))
			return true;
	}
	else if ((contents2 || (release && using)) && (ctrl == CON_UseAlt || ctrl == CON_UseAltDelayed))
	{
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", contents2))
			return true;
	}
	
	// everything down from here:
	// standard controls that are called if not overloaded via script
	
	// Movement controls (defined in PlayerControl.c, partly overloaded here)
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
		return ObjectControlMovement(plr, ctrl, strength, release);

	// Push controls
	if (ctrl == CON_Grab || ctrl == CON_Ungrab || ctrl == CON_PushEnter || ctrl == CON_GrabPrevious || ctrl == CON_GrabNext)
		return ObjectControlPush(plr, ctrl);

	// Entrance controls
	if (ctrl == CON_Enter || ctrl == CON_Exit)
		return ObjectControlEntrance(plr,ctrl);

	// only if not in house, not grabbing a vehicle and an item selected
	if (!house && (!vehicle || proc != "PUSH"))
	{
		if (contents)
		{
			// throw
			if (ctrl == CON_Throw)
			{
				if (proc == "SCALE" || proc == "HANGLE")
					return PlayerObjectCommand(plr, false, "Drop", contents);
				else
					return PlayerObjectCommand(plr, false, "Throw", contents, x, y);
			}
			// throw delayed
			if (ctrl == CON_ThrowDelayed)
			{
				if (release)
				{
					VirtualCursor()->StopAim();
				
					if (proc == "SCALE" || proc == "HANGLE")
						return PlayerObjectCommand(plr, false, "Drop", contents);
					else
						return PlayerObjectCommand(plr, false, "Throw", contents, mlastx, mlasty);
				}
				else
				{
					VirtualCursor()->StartAim(this);
				}
			}
			// drop
			if (ctrl == CON_Drop)
			{
				return PlayerObjectCommand(plr, false, "Drop", contents);
			}
		}
		// same for contents2 (copypasta)
		if (contents2)
		{
			// throw
			if (ctrl == CON_ThrowAlt)
			{
			    if (proc == "SCALE" || proc == "HANGLE")
			      return PlayerObjectCommand(plr, false, "Drop", contents2);
			    else
			      return PlayerObjectCommand(plr, false, "Throw", contents2, x, y);
			}
			// throw delayed
			if (ctrl == CON_ThrowAltDelayed)
			{
				if (release)
				{
					VirtualCursor()->StopAim();
				
					if (proc == "SCALE" || proc == "HANGLE")
						return PlayerObjectCommand(plr, false, "Drop", contents2);
					else
						return PlayerObjectCommand(plr, false, "Throw", contents2, mlastx, mlasty);
				}
				else
				{
					VirtualCursor()->StartAim(this);
				}
			}
			// drop
			if (ctrl == CON_DropAlt)
			{
				return PlayerObjectCommand(plr, false, "Drop", contents2);
			}
		}
	}
	
	// Unhandled control
	return false;
}

public func ObjectControlMovement(int plr, int ctrl, int strength, bool release)
{
	// from PlayerControl.c
	var result = inherited(plr,ctrl,strength,release,...);

	// do the following only if strength >= CON_Gamepad_Deadzone
	if(!release)
		if(strength != nil && strength < CON_Gamepad_Deadzone)
			return result;

	
	virtual_cursor = FindObject(Find_ID(GUI_Crosshair),Find_Owner(GetOwner()));
	if(!virtual_cursor) return result;

	// change direction of virtual_cursor
	if(!release)
		virtual_cursor->Direction(ctrl);

	return result;
}

public func ObjectCommand(string command, object target, int tx, int ty, object target2)
{
	// special control for throw and jump
	// but only with controls, not with general commands
	if (command == "Throw") this->~ControlThrow(target,tx,ty);
	else if (command == "Jump") this->~ControlJump();
	// else standard command
	else SetCommand(command,target,tx,ty,target2);
}

/* ++++++++++++++++++++++++ Use Controls ++++++++++++++++++++++++ */

public func CancelUse()
{
	if (!using) return;
	
	var control = "Control";
	if (Contained() == using)
		control = "Contained";
	
	// use the saved x,y coordinates for canceling
	CancelUseControl(control, mlastx, mlasty, using);
}

private func StartUseControl(int ctrl, control, int x, int y, object obj)
{
	using = obj;
	var hold_enabled = obj->Call("~HoldingEnabled");
	
	if (hold_enabled)
		SetPlayerControlEnabled(GetOwner(), CON_Aim, true);
		
	if (ctrl == CON_Use) alt = false;
	else alt = true;
	
	var estr = "";
	if (alt && !(obj->Contained())) estr = "Alt";
	
	// first call UseStart. If unhandled, call Use (mousecontrol)
	var handled = obj->Call(Format("~%sUseStart%s",control,estr),this,x,y);
	if (!handled)
	{
		handled = obj->Call(Format("~%sUse%s",control,estr),this,x,y);
		noholdingcallbacks = handled;
	}
	if (!handled)
	{
		using = nil;
		if (hold_enabled)
			SetPlayerControlEnabled(GetOwner(), CON_Aim, false);
		return false;
	}
		
	return handled;
}

private func StartUseDelayedControl(int ctrl, control, object obj)
{
	using = obj;
	var hold_enabled = obj->Call("~HoldingEnabled");
				
	if (ctrl == CON_UseDelayed) alt = false;
	else alt = true;
			
	var estr = "";
	if (alt && !(obj->Contained())) estr = "Alt";
			
	VirtualCursor()->StartAim(this);
			
	// call UseStart
	var handled = obj->Call(Format("~%sUseStart%s",control,estr),this,mlastx,mlasty);
	if (!handled) noholdingcallbacks = true;
	else noholdingcallbacks = false;
	
	return handled;
}

private func CancelUseControl(control, int x, int y, object obj)
{
	return StopUseControl(control, x, y, obj, true);
}

private func StopUseControl(control, int x, int y, object obj, bool cancel)
{
	var estr = "";
	if (alt && !(obj->Contained())) estr = "Alt";
	
	var holding_enabled = obj->Call("~HoldingEnabled");
	
	var stop = "Stop";
	if (cancel) stop = "Cancel";
	
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, ContainedUseCancel, etc...
	var handled = obj->Call(Format("~%sUse%s%s",control,estr,stop),this,x,y);
	using = nil;
	alt = false;
	noholdingcallbacks = false;
			
	SetPlayerControlEnabled(GetOwner(), CON_Aim, false);

	if (virtual_cursor)
		virtual_cursor->StopAim();
		
	return handled;
}

private func HoldingUseControl(int ctrl, control, int x, int y, object obj)
{
	var estr = "";
	if (alt && !(obj->Contained())) estr = "Alt";
	
	var mex = x;
	var mey = y;
	if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		mex = mlastx;
		mey = mlasty;
	}
	
	//Message("%d,%d",this,mex,mey);

	// automatic adjustment of the direction
	// --------------------
	// if this is desired for ALL objects is the question, we will find out.
	// For now, this is done for items and vehicles, not for buildings and
	// mounts (naturally). If turning vehicles just liket hat without issuing
	// a turning animation is the question. But after all, the clonk can turn
	// by setting the dir too.
	
	
	//   not riding and                not in building  not while scaling
	if (GetProcedure() != "ATTACH" && !Contained() &&   GetProcedure() != "SCALE")
	{
		// pushing vehicle: object to turn is the vehicle
		var dir_obj = GetActionTarget();

		// otherwise, turn the clonk
		if (!dir_obj) dir_obj = this;
	
		if ((dir_obj->GetComDir() == COMD_Stop && dir_obj->GetXDir() == 0) || dir_obj->GetProcedure() == "FLIGHT")
		{
			if (dir_obj->GetDir() == DIR_Left)
			{
				if (mex > 0)
					dir_obj->SetDir(DIR_Right);
			}
			else
			{
				if (mex < 0)
					dir_obj->SetDir(DIR_Left);
			}
		}
	}
	
	var handled = obj->Call(Format("~%sUse%sHolding",control,estr),this,mex,mey);
			
	return handled;
}

private func StopUseDelayedControl(control, object obj)
{
	var estr = "";
	if (alt && !(obj->Contained())) estr = "Alt";
	
	var holding_enabled = obj->Call("~HoldingEnabled");
	
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, etc...
	var handled = obj->Call(Format("~%sUse%sStop",control,estr),this,mlastx,mlasty);
	if (!handled)
		handled = obj->Call(Format("~%sUse%s",control,estr),this,mlastx,mlasty);

	//Log("called %sUse%sStop(this,%d,%d)",control,estr,mlastx,mlasty);
	
	VirtualCursor()->StopAim();
	using = nil;
	alt = false;
	noholdingcallbacks = false;
		
	return handled;
}

/* Control to menu */

private func Control2Menu(int ctrl, int x, int y, int strength, bool repeat, bool release)
{

	/* all this stuff is already done on a higher layer - in playercontrol.c
	   now this is just the same for gamepad control */
	   
	if (!PlayerHasVirtualCursor(GetOwner()))
		return false;


	// fix pos of x and y
	var mex = mlastx+GetX()-GetMenu()->GetX();
	var mey = mlasty+GetY()-GetMenu()->GetY();
	
	// update angle for visual effect on the menu
	if (repeat)
	{	
		if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
			this->GetMenu()->~UpdateCursor(mex,mey);
	}
	// click on menu
	if (release)
	{
		// select
		if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
			this->GetMenu()->Select(mex,mey, ctrl == CON_UseAltDelayed);
	}
	
	return true;
}

// Control redirected to script
private func Control2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, string control, object obj)
{
	
	// click on secondary cancels primary and the other way round
	if (using)
	{
		if (ctrl == CON_Use && alt || ctrl == CON_UseAlt && !alt
		||  ctrl == CON_UseDelayed && alt || ctrl == CON_UseAltDelayed && !alt)
		{
			CancelUseControl(control, x, y, using);
			return true;
		}
	}
	
	// standard use
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		if (!release && !repeat)
		{
			return StartUseControl(ctrl, control, x, y, obj);
		}
		else if (release)
		{
			return StopUseControl(control, x, y, obj);
		}
	}
	// gamepad use
	else if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		if (!release && !repeat)
		{
			return StartUseDelayedControl(ctrl, control,obj);
		}
		else if (release)
		{
			return StopUseDelayedControl(control,obj);
		}
	}
	
	// more use (holding)
	if (ctrl == CON_Use || ctrl == CON_UseAlt || ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		if (release)
		{
			// leftover use release
			CancelUse();
			return true;
		}
		else if (repeat && !noholdingcallbacks)
		{
			return HoldingUseControl(ctrl, control, x, y, obj);
		}
	}
	
	// overloads of movement commandos
	else if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Down || ctrl == CON_Up || ctrl == CON_Jump)
	{
		if (release)
		{
			// if any movement key has been released, ControlStop is called
			if (obj->Call(Format("~%sStop",control),this,ctrl))  return true;
		}
		else
		{
			// what about deadzone?
			if (strength != nil && strength < CON_Gamepad_Deadzone)
				return true;
			
			// Control*
			if (ctrl == CON_Left)  if (obj->Call(Format("~%sLeft",control),this))  return true;
			if (ctrl == CON_Right) if (obj->Call(Format("~%sRight",control),this)) return true;
			if (ctrl == CON_Up)    if (obj->Call(Format("~%sUp",control),this))    return true;
			if (ctrl == CON_Down)  if (obj->Call(Format("~%sDown",control),this))  return true;
			
			// for attached (e.g. horse: also Jump command
			if (GetProcedure() == "ATTACH")
				if (ctrl == CON_Jump)  if (obj->Call("ControlJump",this)) return true;
		}
	}
	
	return false;
}

// returns true if the clonk is able to enter a building (procedurewise)
public func CanEnter()
{
	var proc = GetProcedure();
	if (proc != "WALK" && proc != "SWIM" && proc != "SCALE" &&
		proc != "HANGLE" && proc != "FLOAT" && proc != "FLIGHT" &&
		proc != "PUSH") return false;
	return true;
}

// Handles enter and exit
private func ObjectControlEntrance(int plr, int ctrl)
{
	var proc = GetProcedure();

	// enter
	if (ctrl == CON_Enter)
	{
		// contained
		if (Contained()) return false;
		// enter only if... one can
		if (!CanEnter()) return false;

		// a building with an entrance at right position is there?
		var obj = GetEntranceObject();
		if (!obj) return false;
		
		PlayerObjectCommand(plr, false, "Enter", obj);
		return true;
	}
	
	// exit
	if (ctrl == CON_Exit)
	{
		if (!Contained()) return false;
		
		PlayerObjectCommand(plr, false, "Exit");
		return true;
	}
	
	return false;
}

// Handles push controls
private func ObjectControlPush(int plr, int ctrl)
{
	if (!this) return false;
	
	var proc = GetProcedure();
	
	// grabbing
	if (ctrl == CON_Grab)
	{
		// grab only if he walks
		if (proc != "WALK") return false;
		
		// disallow if the clonk is still using something
		if (using) return false;
		
		// only if there is someting to grab
		var obj = FindObject(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this));
		if (!obj) return false;
		
		// grab
		PlayerObjectCommand(plr, false, "Grab", obj);
		return true;
	}
	
	// grab next/previous
	if (ctrl == CON_GrabNext)
		return ShiftVehicle(plr, false);
	if (ctrl == CON_GrabPrevious)
		return ShiftVehicle(plr, true);
	
	// ungrabbing
	if (ctrl == CON_Ungrab)
	{
		// ungrab only if he pushes
		if (proc != "PUSH") return false;

		PlayerObjectCommand(plr, false, "UnGrab");
		return true;
	}
	
	// push into building
	if (ctrl == CON_PushEnter)
	{
		if (proc != "PUSH") return false;
		
		// respect no push enter
		if (GetActionTarget()->GetDefCoreVal("NoPushEnter","DefCore")) return false;
		
		// a building with an entrance at right position is there?
		var obj = GetActionTarget()->GetEntranceObject();
		if (!obj) return false;

		PlayerObjectCommand(plr, false, "PushTo", GetActionTarget(), 0, 0, obj);
		return true;
	}
	
}

// grabs the next/previous vehicle (if there is any)
private func ShiftVehicle(int plr, bool back)
{
	if (!this) return false;
	
	if (GetProcedure() != "PUSH") return false;

	var lorry = GetActionTarget();
	// get all grabbable objects
	var objs = FindObjects(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this));
		
	// nothing to switch to (there is no other grabbable object)
	if (GetLength(objs) <= 1) return false;
		
	// find out at what index of the array objs the vehicle is located
	var index = 0;
	for(var obj in objs)
	{
		if (obj == lorry) break;
		index++;
	}
		
	// get the next/previous vehicle
	if (back)
	{
		--index;
		if (index < 0) index = GetLength(objs)-1;
	}
	else
	{
		++index;
		if (index >= GetLength(objs)) index = 0;
	}
	
	PlayerObjectCommand(plr, false, "Grab", objs[index]);
	
	return true;
} 

/* Virtual cursor stuff */

// get virtual cursor, if noone is there, create it
private func VirtualCursor()
{
	if (!virtual_cursor)
	{
		virtual_cursor = FindObject(Find_ID(GUI_Crosshair),Find_Owner(GetOwner()));
	}
	if (!virtual_cursor)
	{
		virtual_cursor = CreateObject(GUI_Crosshair,0,0,GetOwner());
	}
	
	return virtual_cursor;
}

// virtual cursor is visible
private func VirtualCursorAiming()
{
	if (!virtual_cursor) return false;
	return virtual_cursor->IsAiming();
}

// store pos of virtual cursor into mlastx, mlasty
public func UpdateVirtualCursorPos()
{
	mlastx = VirtualCursor()->GetX()-GetX();
	mlasty = VirtualCursor()->GetY()-GetY();
}

public func TriggerHoldingControl()
{
	// using has been commented because it must be possible to use the virtual
	// cursor aim also without a used object - for menus
	// However, I think the check for 'using' here is just an unecessary safeguard
	// since there is always a using-object if the clonk is aiming for a throw
	// or a use. If the clonk uses it, there will be callbacks that cancel the
	// callbacks to the virtual cursor
	// - Newton
	if (/*using && */!noholdingcallbacks)
	{
		var ctrl;
		if (alt) ctrl = CON_UseAltDelayed;
		else     ctrl = CON_UseDelayed;
				
		ObjectControl(GetOwner(), ctrl, 0, 0, 0, true, false);
	}

}

/* +++++++++++++++++++++++ Menu control +++++++++++++++++++++++ */

local menu;

func HasMenuControl()
{
	return true;
}

func SetMenu(object m)
{
	// already the same
	if (menu == m)
	{
		return;
	}
	// multiple menus are not supported
	if (menu && m)
	{
		menu->Close();
	}
	// new one
	menu = m;
	if (menu)
	{	
		// stop clonk
		SetComDir(COMD_Stop);
	
		if (PlayerHasVirtualCursor(GetOwner()))
			VirtualCursor()->StartAim(this,false,menu);
		else
		{
			SetPlayerControlEnabled(GetOwner(), CON_GUICursor, true);
			SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, true);
			SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, true);
		}
	}
	// close menu
	if (!menu)
	{
		if (virtual_cursor)
			virtual_cursor->StopAim();
		
		SetPlayerControlEnabled(GetOwner(), CON_GUICursor, false);
		SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, false);
		SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, false);
	}
}

func MenuClosed()
{
	SetMenu(nil);
}

func GetMenu()
{
	return menu;
}

func CancelMenu()
{
	if (menu) menu->Close();
}

func ReinitializeControls()
{
	if(PlayerHasVirtualCursor(GetOwner()))
	{
		// if is aiming or in menu and no virtual cursor is there? Create one
		if (!virtual_cursor)
			if (menu || using)
				VirtualCursor()->StartAim(this,false,menu);
	}
	else
	{
		// remove any virtual cursor
		if (virtual_cursor)
			virtual_cursor->RemoveObject();
	}
}

/* Backpack control */

func Selected(object mnu, object mnu_item, bool alt)
{
	var backpack_index = mnu_item->GetExtraData();
	var hands_index = 0;
	if (alt) hands_index = 1;
	// swap index with backpack index
	Switch2Items(hands_index, backpack_index);
	return true;
}

/* +++++++++++++++  Throwing, jumping +++++++++++++++ */

// Throwing
private func DoThrow(object obj, int angle)
{
	// parameters...
	var iX, iY, iR, iXDir, iYDir, iRDir;
	iX = 8; if (!GetDir()) iX = -iX;
	iY = Cos(angle,-8);
	iR = Random(360);
	iRDir = RandomX(-10,10);

	var speed = GetPhysical("Throw");

	iXDir = speed * Sin(angle,1000) / 17000;
	iYDir = speed * Cos(angle,-1000) / 17000;
	// throw boost (throws stronger upwards than downwards)
	if (iYDir < 0) iYDir = iYDir * 13/10;
	if (iYDir > 0) iYDir = iYDir * 8/10;
	
	// add own velocity
	iXDir += GetXDir(1000)/2;
	iYDir += GetYDir(1000)/2;

	// throw
	obj->Exit(iX, iY, iR, 0, 0, iRDir);	
	obj->SetXDir(iXDir,1000);
	obj->SetYDir(iYDir,1000);
	
	return true;
}

// custom throw
public func ControlThrow(object target, int x, int y)
{
	// standard throw after all
	if (!x && !y) return false;
	if (!target) return false;
	
	return false;
}

public func ControlJump()
{
	var ydir = 0;
	var xdir = 0;
	
	if (GetProcedure() == "WALK")
	{
		ydir = GetPhysical("Jump")/1000;
	}
	else if (InLiquid())
	{
		if (!GBackSemiSolid(0,-5))
			ydir = BoundBy(GetPhysical("Swim")/2500,24,38);
	}		
	
	if (ydir)
	{
		SetPosition(GetX(),GetY()-1);
		SetAction("Jump");
		SetXDir(GetXDir()+(GetDir()*2-1)*xdir*GetCon()/100);
		SetYDir(-ydir*GetCon()/100);
		return true;
	}
	return false;
}
