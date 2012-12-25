/*
	Standard clonk controls
	Author: Newton
	
	This object provides handling of the clonk controls including item
	management, backpack controls and standard throwing behaviour. It
	should be included into any clonk/crew definition.
	The controls in System.ocg/PlayerControl.c only provide basic movement
	handling, namely the movement left, right, up and down. The rest is
	handled here:
	Grabbing, ungrabbing, shifting and pushing vehicles into buildings;
	entering and exiting buildings; throwing, dropping; backpack control,
	(object) menu control, hotkey controls, usage and it's callbacks and
	forwards to script.
	
	Objects that inherit this object need to return _inherited() in the
	following callbacks (if defined):
		Construction, Collection2, Ejection, RejectCollect, Departure,
		Entrance, AttachTargetLost, CrewSelection, Death,
		Destruction, OnActionChanged
	
	The following callbacks are made to other objects:
		*Stop
		*Left, *Right, *Up, *Down
		*Use, *UseStop, *UseStart, *UseHolding, *UseCancel
	wheras * is 'Contained' if the clonk is contained and otherwise (riding,
	pushing, to self) it is 'Control'. The item in the inventory only gets
	the Use*-calls. If the callback is handled, you should return true.
	Currently, this is explained more in detail here:
	http://forum.openclonk.org/topic_show.pl?tid=337
*/

// make use of other sub-libraries
#include Library_Inventory
#include Library_ClonkInventoryControl
#include Library_ClonkGamepadControl


/* ++++++++++++++++++++++++ Clonk Inventory Control ++++++++++++++++++++++++ */

/*
	used properties
	this.control.hotkeypressed: used to determine if an interaction has already been handled by a hotkey (space + 1-9)
	
	this.control.current_object: object that is being used at the moment
	this.control.using_type: way of usage
	this.control.alt: whether the current object is on mousebutton 2
	this.control.mlastx: last x position of the cursor
	this.control.mlasty: last y position of the cursor
	this.control.noholdingcallbacks: whether to do HoldingUseControl callbacks
*/


/* Item limit */
public func MaxContentsCount() { return 7; }		// Size of the clonks inventory
public func HandObjects() { return 1; } // Amount of hands to select items
public func NoStackedContentMenu() { return true; }	// Contents-Menu shall display each object in a seperate slot


/* ################################################# */

protected func Construction()
{
	if(this.control == nil)
		this.control = {};
	this.control.hotkeypressed = false;
	
	menu = nil;

	this.control.alt = false;
	this.control.current_object = nil;
	this.control.using_type = nil;
	
	return _inherited(...);
}

public func GetUsedObject() { return this.control.current_object; }

// The using-command hast to be canceled if the clonk is entered into
// or exited from a building.

protected func Entrance()         { CancelUse(); return _inherited(...); }
protected func Departure()        { CancelUse(); return _inherited(...); }

// The same for vehicles
protected func AttachTargetLost() { CancelUse(); return _inherited(...); }

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

protected func OnActionChanged(string oldaction)
{
	var old_act = this["ActMap"][oldaction];
	var act = this["ActMap"][GetAction()];
	var old_proc = 0;
	if(old_act) old_proc = old_act["Procedure"];
	var proc = 0;
	if(act) proc = act["Procedure"];
	// if the object's procedure has changed from a non Push/Attach
	// to a Push/Attach action or the other way round, the usage needs
	// to be cancelled
	if (proc != old_proc)
	{
		if (proc == DFA_PUSH || proc == DFA_ATTACH
		 || old_proc == DFA_PUSH || old_proc == DFA_ATTACH)
		{
			CancelUse();
		}
	}
	return _inherited(oldaction,...);
}

/** Returns additional interactions the clonk possesses as an array of function pointers.
	Returned Proplist contains:
		Fn			= Name of the function to call
		Object		= object to call the function in. Will also be displayed on the interaction-button
		Description	= a description of what the interaction does
		IconID		= ID of the definition that contains the icon (like GetInteractionMetaInfo)
		IconName	= Namo of the graphic for teh icon (like GetInteractionMetaInfo)
		[Priority]	= Where to sort in in the interaction-list. 0=front, 1=after script, 2=after vehicles, >=3=at the end, nil equals 3
*/
public func GetExtraInteractions()
{
	var functions = CreateArray();
	
	// flipping construction-preview
	var effect;
	if(effect = GetEffect("ControlConstructionPreview", this))
	{
		if(effect.flipable)
			PushBack(functions, {Fn = "Flip", Description=ConstructionPreviewer->GetFlipDescription(), Object=effect.preview, IconID=ConstructionPreviewer_IconFlip, Priority=0});
	}
		
	return functions;
}

/* +++++++++++++++++++++++++++ Clonk Control +++++++++++++++++++++++++++ */

/* Main control function */
public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) 
		return false;
	
	// some controls should only do something on release (everything that has to do with interaction)
	if(ctrl == CON_Interact || ctrl == CON_PushEnter || ctrl == CON_Ungrab || ctrl == CON_GrabNext || ctrl == CON_Grab || ctrl == CON_Enter || ctrl == CON_Exit)
	{
		if(!release)
		{
			// this is needed to reset the hotkey-memory
			this.control.hotkeypressed = false;
			return false;
		}
		// if the interaction-command has already been handled by a hotkey (else it'd double-interact)
		else if(this.control.hotkeypressed)
			return false;
		// check if we can handle it by simply accessing the first actionbar item (for consistency)
		else
		{
			if(GetMenu())
				if(!GetMenu()->~Uncloseable())
					return CancelMenu();
			
			if(this->~ControlHotkey(0))
					return true;
		}
	}
	
	// Contents menu
	if (ctrl == CON_Contents)
	{
		// Close any menu if open.
		if (GetMenu())
		{
			// Uncloseable menu?
			if (GetMenu()->~Uncloseable()) return true;

			var is_content = GetMenu()->~IsContentMenu();
			CancelMenu();
			// If contents menu, don't open new one and return.
			if (is_content)
				return true;
		}
		// Open contents menu.
		CancelUse();
		CreateContentsMenus();
		// CreateContentsMenus calls SetMenu(this) in the clonk
		// so after this call menu = the created menu
		if(GetMenu())
			GetMenu()->Show();		
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
	if (this.control.current_object && ctrl == CON_Aim)
	{
		if (this.control.alt) ctrl = CON_UseAlt;
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
		this.control.mlastx = x;
		this.control.mlasty = y;
	}
		
	var proc = GetProcedure();

	// cancel usage
	if (this.control.current_object && ctrl == CON_Ungrab)
	{
		CancelUse();
		return true;
	}

	// Interact controls
	if(ctrl == CON_Interact)
	{
		if(ObjectControlInteract(plr,ctrl))
			return true;
		return _inherited(plr, ctrl, x, y, strength, repeat, release, ...);
	}
	// Push controls
	if (ctrl == CON_Grab || ctrl == CON_Ungrab || ctrl == CON_PushEnter || ctrl == CON_GrabPrevious || ctrl == CON_GrabNext)
		return ObjectControlPush(plr, ctrl);

	// Entrance controls
	if (ctrl == CON_Enter || ctrl == CON_Exit)
		return ObjectControlEntrance(plr,ctrl);
	
	
	// building, vehicle, mount, contents, menu control
	var house = Contained();
	var vehicle = GetActionTarget();
	// the clonk can have an action target even though he lost his action. 
	// That's why the clonk may only interact with a vehicle if in an
	// appropiate procedure:
	if (proc != "ATTACH" && proc != "PUSH")
		vehicle = nil;
	
	// menu
	if (menu)
	{
		return Control2Menu(ctrl, x,y,strength, repeat, release);
	}
	
	var contents = this->GetHandItem(0);
	var contents2 = this->GetHandItem(1);	
	
	// usage
	var use = (ctrl == CON_Use || ctrl == CON_UseDelayed || ctrl == CON_UseAlt || ctrl == CON_UseAltDelayed);
	if (use)
	{
		if (house)
		{
			return ControlUse2Script(ctrl, x, y, strength, repeat, release, house);
		}
		// control to grabbed vehicle
		else if (vehicle && proc == "PUSH")
		{
			return ControlUse2Script(ctrl, x, y, strength, repeat, release, vehicle);
		}
		else if (vehicle && proc == "ATTACH")
		{
			/* objects to which clonks are attached (like horses, mechs,...) have
			   a special handling:
			   Use controls are, forwarded to the
			   horse but if the control is considered unhandled (return false) on
			   the start of the usage, the control is forwarded further to the
			   item. If the item then returns true on the call, that item is
			   regarded as the used item for the subsequent ControlUse* calls.
			   BUT the horse always gets the ControlUse*-calls that'd go to the used
			   item, too and before it so it can decide at any time to cancel its
			   usage via CancelUse().
			  */

			if (ControlUse2Script(ctrl, x, y, strength, repeat, release, vehicle))
				return true;
			else
			{
				// handled if the horse is the used object
				// ("using" is set to the object in StartUse(Delayed)Control - when the
				// object returns true on that callback. Exactly what we want)
				if (this.control.current_object == vehicle) return true;
				// has been cancelled (it is not the start of the usage but no object is used)
				if (!this.control.current_object && (repeat || release)) return true;
			}
		}
		// Release commands are always forwarded even if contents is 0, in case we
		// need to cancel use of an object that left inventory
		if ((contents || (release && this.control.current_object)) && (ctrl == CON_Use || ctrl == CON_UseDelayed))
		{
			if (ControlUse2Script(ctrl, x, y, strength, repeat, release, contents))
				return true;
		}
		else if ((contents2 || (release && this.control.current_object)) && (ctrl == CON_UseAlt || ctrl == CON_UseAltDelayed))
		{
			if (ControlUse2Script(ctrl, x, y, strength, repeat, release, contents2))
				return true;
		}
	}
	
	// Throwing and dropping
	// only if not in house, not grabbing a vehicle and an item selected
	// only act on press, not release
	if (!house && (!vehicle || proc == "ATTACH") && !release)
	{
		if (contents)
		{
			// special treatmant so that we know it's a forced throw
			if(ctrl == CON_ForcedThrow)
			{
				ctrl = CON_Throw;
				this.inventory.forced_ejection = contents; // used in Inventory.ocd
			}
			
			// throw
			if (ctrl == CON_Throw)
			{
				CancelUse();
				
				if (proc == "SCALE" || proc == "HANGLE" || proc == "SWIM")
					return ObjectCommand("Drop", contents);
				else
					return ObjectCommand("Throw", contents, x, y);
			}
			// throw delayed
			if (ctrl == CON_ThrowDelayed)
			{
				CancelUse();
				if (release)
				{
					VirtualCursor()->StopAim();
				
					if (proc == "SCALE" || proc == "HANGLE")
						return ObjectCommand("Drop", contents);
					else
						return ObjectCommand("Throw", contents, this.control.mlastx, this.control.mlasty);
				}
				else
				{
					VirtualCursor()->StartAim(this);
					return true;
				}
			}
			// drop
			if (ctrl == CON_Drop)
			{
				CancelUse();
				return ObjectCommand("Drop", contents);
			}
		}
		// same for contents2 (copypasta)
		if (contents2)
		{
			// special treatmant so that we know it's a forced throw
			if(ctrl == CON_ForcedThrowAlt)
			{
				ctrl = CON_ThrowAlt;
				this.inventory.forced_ejection = contents2; // used in Inventory.ocd
			}
		
			// throw
			if (ctrl == CON_ThrowAlt)
			{
				CancelUse();
				if (proc == "SCALE" || proc == "HANGLE")
					return ObjectCommand("Drop", contents2);
				else
					return ObjectCommand("Throw", contents2, x, y);
			}
			// throw delayed
			if (ctrl == CON_ThrowAltDelayed)
			{
				CancelUse();
				if (release)
				{
					VirtualCursor()->StopAim();
				
					if (proc == "SCALE" || proc == "HANGLE")
						return ObjectCommand("Drop", contents2);
					else
						return ObjectCommand("Throw", contents2, this.control.mlastx, this.control.mlasty);
				}
				else
				{
					CancelUse();
					VirtualCursor()->StartAim(this);
					return true;
				}
			}
			// drop
			if (ctrl == CON_DropAlt)
			{
				return ObjectCommand("Drop", contents2);
			}
		}
	}
	
	// Movement controls (defined in PlayerControl.c, partly overloaded here)
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
	{	
		// forward to script...
		if (house)
		{
			return ControlMovement2Script(ctrl, x, y, strength, repeat, release, house);
		}
		else if (vehicle)
		{
			if (ControlMovement2Script(ctrl, x, y, strength, repeat, release, vehicle)) return true;
		}
	
		return ObjectControlMovement(plr, ctrl, strength, release);
	}
	
	// Unhandled control
	return _inherited(plr, ctrl, x, y, strength, repeat, release, ...);
}

public func ObjectCommand(string command, object target, int tx, int ty, object target2)
{
	// special control for throw and jump
	// but only with controls, not with general commands
	if (command == "Throw") return this->~ControlThrow(target,tx,ty);
	else if (command == "Jump") return this->~ControlJump();
	// else standard command
	else return SetCommand(command,target,tx,ty,target2);
	
	// this function might be obsolete: a normal SetCommand does make a callback to
	// script before it is executed: ControlCommand(szCommand, pTarget, iTx, iTy)
}

/* ++++++++++++++++++++++++ Use Controls ++++++++++++++++++++++++ */

public func CancelUse()
{
	if (!this.control.current_object) return;

	// use the saved x,y coordinates for canceling
	CancelUseControl(this.control.mlastx, this.control.mlasty);
}

private func DetermineUsageType(object obj)
{
	if(!obj) return nil;
	// house
	if (obj == Contained())
		return C4D_Structure;
	// object
	if (obj->Contained() == this)
		return C4D_Object;
	// vehicle
	var proc = GetProcedure();
	if (obj == GetActionTarget())
		if (proc == "ATTACH" && proc == "PUSH")
			return C4D_Vehicle;
	// unknown
	return nil;
}

private func GetUseCallString(string action)
{
	// Control... or Contained...
	var control = "Control";
	if (this.control.using_type == C4D_Structure) control = "Contained";
	// ..Use.. or ..UseAlt...
	var estr = "";
	if (this.control.alt && this.control.using_type != C4D_Object) estr = "Alt";
	// Action
	if (!action) action = "";
	
	return Format("~%sUse%s%s",control,estr,action);
}

private func StartUseControl(int ctrl, int x, int y, object obj)
{
	this.control.current_object = obj;
	this.control.using_type = DetermineUsageType(obj);
	this.control.alt = ctrl != CON_Use;
	
	var hold_enabled = obj->Call("~HoldingEnabled");
	
	if (hold_enabled)
		SetPlayerControlEnabled(GetOwner(), CON_Aim, true);

	// first call UseStart. If unhandled, call Use (mousecontrol)
	var handled = obj->Call(GetUseCallString("Start"),this,x,y);
	if (!handled)
	{
		handled = obj->Call(GetUseCallString(),this,x,y);
		this.control.noholdingcallbacks = handled;
	}
	if (!handled)
	{
		this.control.current_object = nil;
		this.control.using_type = nil;
		if (hold_enabled)
			SetPlayerControlEnabled(GetOwner(), CON_Aim, false);
		return false;
	}
		
	return handled;
}

private func StartUseDelayedControl(int ctrl, object obj)
{
	this.control.current_object = obj;
	this.control.using_type = DetermineUsageType(obj);
	this.control.alt = ctrl != CON_UseDelayed;
				
	VirtualCursor()->StartAim(this);
			
	// call UseStart
	var handled = obj->Call(GetUseCallString("Start"),this,this.control.mlastx,this.control.mlasty);
	this.control.noholdingcallbacks = !handled;
	
	return handled;
}

private func CancelUseControl(int x, int y)
{
	// to horse first (if there is one)
	var horse = GetActionTarget();
	if(horse && GetProcedure() == "ATTACH" && this.control.current_object != horse)
		StopUseControl(x, y, horse, true);

	return StopUseControl(x, y, this.control.current_object, true);
}

private func StopUseControl(int x, int y, object obj, bool cancel)
{
	var stop = "Stop";
	if (cancel) stop = "Cancel";
	
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, ContainedUseCancel, etc...
	var handled = obj->Call(GetUseCallString(stop),this,x,y);
	if (obj == this.control.current_object)
	{
		// if ControlUseStop returned -1, the current object is kept as "used object"
		// but no more callbacks except for ControlUseCancel are made. The usage of this
		// object is finally cancelled on ControlUseCancel.
		if(cancel || handled != -1)
		{
			this.control.current_object = nil;
			this.control.using_type = nil;
			this.control.alt = false;
		}
		this.control.noholdingcallbacks = false;
		
		SetPlayerControlEnabled(GetOwner(), CON_Aim, false);

		if (virtual_cursor)
			virtual_cursor->StopAim();
	}
		
	return handled;
}

private func HoldingUseControl(int ctrl, int x, int y, object obj)
{
	var mex = x;
	var mey = y;
	if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		mex = this.control.mlastx;
		mey = this.control.mlasty;
	}
	
	//Message("%d,%d",this,mex,mey);

	// automatic adjustment of the direction
	// --------------------
	// if this is desired for ALL objects is the question, we will find out.
	// For now, this is done for items and vehicles, not for buildings and
	// mounts (naturally). If turning vehicles just like that without issuing
	// a turning animation is the question. But after all, the clonk can turn
	// by setting the dir too.
	
	
	//   not riding and                not in building  not while scaling
	if (GetProcedure() != "ATTACH" && !Contained() &&   GetProcedure() != "SCALE")
	{
		// pushing vehicle: object to turn is the vehicle
		var dir_obj = GetActionTarget();
		if (GetProcedure() != "PUSH") dir_obj = nil;

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
	
	var handled = obj->Call(GetUseCallString("Holding"),this,mex,mey);
			
	return handled;
}

private func StopUseDelayedControl(object obj)
{
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, etc...
	
	var handled = obj->Call(GetUseCallString("Stop"),this,this.control.mlastx,this.control.mlasty);
	if (!handled)
		handled = obj->Call(GetUseCallString(),this,this.control.mlastx,this.control.mlasty);
	
	if (obj == this.control.current_object)
	{
		VirtualCursor()->StopAim();
		// see StopUseControl
		if(handled != -1)
		{
			this.control.current_object = nil;
			this.control.using_type = nil;
			this.control.alt = false;
		}
		this.control.noholdingcallbacks = false;
	}
		
	return handled;
}


// Control use redirected to script
private func ControlUse2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, object obj)
{
	// click on secondary cancels primary and the other way round
	if (this.control.current_object)
	{
		if (ctrl == CON_Use && this.control.alt || ctrl == CON_UseAlt && !this.control.alt
		||  ctrl == CON_UseDelayed && this.control.alt || ctrl == CON_UseAltDelayed && !this.control.alt)
		{
			CancelUseControl(x, y);
			return true;
		}
	}
	
	// standard use
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		if (!release && !repeat)
		{
			return StartUseControl(ctrl,x, y, obj);
		}
		else if (release && obj == this.control.current_object)
		{
			return StopUseControl(x, y, obj);
		}
	}
	// gamepad use
	else if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		if (!release && !repeat)
		{
			return StartUseDelayedControl(ctrl, obj);
		}
		else if (release && obj == this.control.current_object)
		{
			return StopUseDelayedControl(obj);
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
		else if (repeat && !this.control.noholdingcallbacks)
		{
			return HoldingUseControl(ctrl, x, y, obj);
		}
	}
		
	return false;
}

// Control use redirected to script
private func ControlMovement2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, object obj)
{
	// overloads of movement commandos
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Down || ctrl == CON_Up || ctrl == CON_Jump)
	{
		var control = "Control";
		if (Contained() == obj) control = "Contained";
	
		if (release)
		{
			// if any movement key has been released, ControlStop is called
			if (obj->Call(Format("~%sStop",control),this,ctrl))  return true;
		}
		else
		{
			// what about gamepad-deadzone?
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
		
		ObjectCommand("Enter", obj);
		return true;
	}
	
	// exit
	if (ctrl == CON_Exit)
	{
		if (!Contained()) return false;
		
		ObjectCommand("Exit");
		return true;
	}
	
	return false;
}

private func ObjectControlInteract(int plr, int ctrl)
{
	var interactables = FindObjects(Find_Or(Find_Container(this), Find_AtPoint(0,0)),
						Find_Func("IsInteractable",this), Find_Layer(GetObjectLayer()));
	// if there are several interactable objects, just call the first that returns true
	for (var interactable in interactables)
	{
		if (interactable->~Interact(this))
		{
			return true;
		}
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
		
		// only if there is someting to grab
		var obj = FindObject(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this), Find_Layer(GetObjectLayer()));
		if (!obj) return false;
		
		// grab
		ObjectCommand("Grab", obj);
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

		ObjectCommand("UnGrab");
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

		ObjectCommand("PushTo", GetActionTarget(), 0, 0, obj);
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
	var objs = FindObjects(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this), Find_Layer(GetObjectLayer()));
		
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
	
	ObjectCommand("Grab", objs[index]);
	
	return true;
}



public func IsMounted() { return GetProcedure() == "ATTACH"; }

/* +++++++++++++++++++++++ Menu control +++++++++++++++++++++++ */

local menu;

func HasMenuControl()
{
	return true;
}

func SetMenu(object m)
{
	// already the same
	if ((menu == m) && m)
	{
		return;
	}
	// no multiple menus: close old one
	if (menu && m)
	{
		menu->Close();
	}
	// new one
	menu = m;
	if (menu)
	{
		CancelUse();
		// stop clonk
		SetComDir(COMD_Stop);
	
		if (PlayerHasVirtualCursor(GetOwner()))
			VirtualCursor()->StartAim(this,false,menu);
		else
		{
			if (menu->~CursorUpdatesEnabled()) 
				SetPlayerControlEnabled(GetOwner(), CON_GUICursor, true);

			SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, true);
			SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, true);
		}
	}
	// close menu
	if (!menu)
	{
		RemoveVirtualCursor(); // for gamepads
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
	if (menu)
	{
		menu->Close();
		SetMenu(nil);
		return true;
	}
	
	return false;
}


/* +++++++++++++++  Throwing, jumping +++++++++++++++ */

// Throwing
private func DoThrow(object obj, int angle)
{
	// parameters...
	var iX, iY, iR, iXDir, iYDir, iRDir;
	iX = 4; if (!GetDir()) iX = -iX;
	iY = Cos(angle,-4);
	iR = Random(360);
	iRDir = RandomX(-10,10);

	iXDir = Sin(angle,this.ThrowSpeed);
	iYDir = Cos(angle,-this.ThrowSpeed);
	// throw boost (throws stronger upwards than downwards)
	if (iYDir < 0) iYDir = iYDir * 13/10;
	if (iYDir > 0) iYDir = iYDir * 8/10;
	
	// add own velocity
	iXDir += GetXDir(100)/2;
	iYDir += GetYDir(100)/2;

	// throw
	obj->Exit(iX, iY, iR, 0, 0, iRDir);
	obj->SetXDir(iXDir,100);
	obj->SetYDir(iYDir,100);
	
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
	
	if (GetProcedure() == "WALK")
	{
		ydir = this.JumpSpeed;
	}
	
	if (InLiquid())
	{
		if (!GBackSemiSolid(0,-5))
			ydir = BoundBy(this.JumpSpeed * 3 / 5, 240, 380);
	}

	if (GetProcedure() == "SCALE")
	{
		ydir = this.JumpSpeed/2;
	}
	
	if (ydir && !Stuck())
	{
		SetPosition(GetX(),GetY()-1);

		//Wall kick
		if(GetProcedure() == "SCALE")
		{
			AddEffect("WallKick",this,1);
			SetAction("Jump");

			var xdir;
			if(GetDir() == DIR_Right)
			{
				xdir = -1;
				SetDir(DIR_Left);
			}
			else if(GetDir() == DIR_Left)
			{
				xdir = 1;
				SetDir(DIR_Right);
			}

			SetYDir(-ydir * GetCon(), 100 * 100);
			SetXDir(xdir * 17);
			return true;
		}
		//Normal jump
		else
		{
			SetAction("Jump");
			SetYDir(-ydir * GetCon(), 100 * 100);
			return true;
		}
	}
	return false;
}

func FxIsWallKickStart(object target, int num, bool temp)
{
	return 1;
}