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
	
	Objects that inherit this object need to return _inherited(...) in the
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
	http://forum.openclonk.org/topic_show.pl?tid = 337
*/

// make use of other sub-libraries
#include Library_Inventory
#include Library_ClonkInventoryControl
#include Library_ClonkInteractionControl
#include Library_ClonkMenuControl
#include Library_ClonkUseControl
#include Library_ClonkGamepadControl

// used for interaction with objects
static const ACTIONTYPE_INVENTORY = 0;
static const ACTIONTYPE_VEHICLE = 1;
static const ACTIONTYPE_STRUCTURE = 2;
static const ACTIONTYPE_SCRIPT = 3;
static const ACTIONTYPE_EXTRA = 4;

// elevators within this range (x) can be called
static const ELEVATOR_CALL_DISTANCE = 30;

// default throwing angle used while the Clonk isn't aiming
static const DEFAULT_THROWING_ANGLE = 500;

/* ++++++++++++++++++++++++ Clonk Inventory Control ++++++++++++++++++++++++ */

/*
	used properties
	this.control.hotkeypressed: used to determine if an interaction has already been handled by a hotkey (space + 1-9)
	
	this.control.alt: alternate usage by right mouse button
	this.control.mlastx: last x position of the cursor
	this.control.mlasty: last y position of the cursor
*/


/* Item limit */
local MaxContentsCount = 5; // Size of the clonks inventory
local HandObjects = 1; // Amount of hands to select items
public func NoStackedContentMenu() { return true; }	// Contents-Menu shall display each object in a seperate slot


/* ################################################# */

protected func Construction()
{
	if (this.control == nil)
		this.control = {};
	this.control.hotkeypressed = false;

	this.control.alt = false;
	return _inherited(...);
}

protected func OnActionChanged(string oldaction)
{
	var old_act = this["ActMap"][oldaction];
	var act = this["ActMap"][GetAction()];
	var old_proc = 0;
	if (old_act) old_proc = old_act["Procedure"];
	var proc = 0;
	if (act) proc = act["Procedure"];
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
		Object		= Object to call the function in. Will also be displayed on the interaction-button
		Description	= A description of what the interaction does
		IconID		= ID of the definition that contains the icon (like GetInteractionMetaInfo)
		IconName	= Name of the graphic for the icon (like GetInteractionMetaInfo)
		Priority	= Where to sort in in the interaction-list. 0 = front, 10 = after script, 20 = after vehicles, 30 = after structures, nil means no preference
*/
public func GetExtraInteractions()
{
	var functions = _inherited(...) ?? [];
	
	// flipping construction-preview
	var fx = GetEffect("ControlConstructionPreview", this);
	if (fx)
	{
		if (fx.flipable)
			PushBack(functions, {Fn = "Flip", Description = ConstructionPreviewer->GetFlipDescription(), Object = fx.preview, IconID = ConstructionPreviewer_IconFlip, Priority = 0});
	}
	// call elevator cases
	var elevators = FindObjects(Find_ID(ElevatorCase), Find_InRect(-ELEVATOR_CALL_DISTANCE, AbsY(0), ELEVATOR_CALL_DISTANCE * 2, GetY() + AbsY(LandscapeHeight())), Find_Func("Ready", this));
	for (var elevator in elevators)
		PushBack(functions, { Fn = "CallCase", Object = elevator, Description = elevator->GetCallDescription(), Priority = 0 });
	return functions;
}

/* +++++++++++++++++++++++++++ Clonk Control +++++++++++++++++++++++++++ */

/* Main control function */
public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	if (!this) 
		return false;
	
	// Contents menu
	if (ctrl == CON_Contents && status == CONS_Down)
	{
		// Close any menu if open.
		if (GetMenu())
		{
			var is_content = GetMenu()->~IsContentMenu();
			// unclosable menu? bad luck
			if (!this->~TryCancelMenu()) return true;
			// If contents menu, don't open new one and return.
			if (is_content)
				return true;
		}
		// Open contents menu.
		CancelUse();
		GUI_ObjectInteractionMenu->CreateFor(this, GUI_OIM_NewStyle);
		// the interaction menu calls SetMenu(this) in the clonk
		// so after this call menu = the created menu
		if (GetMenu())
			GetMenu()->~Show();		
		return true;
	}
	
	/* aiming with mouse:
	   The CON_Aim control is transformed into a use command. Con_Use if
	   repeated does not bear the updated x, y coordinates, that's why this
	   other control needs to be issued and transformed. CON_Aim is a
	   control which is issued on mouse move but disabled when not aiming
	   or when HoldingEnabled() of the used item does not return true.
	   For the rest of the control code, it looks like the x, y coordinates
	   came from CON_Use.
	  */
	if (GetUsedObject() && ctrl == CON_Aim)
	{
		if (this.control.alt) ctrl = CON_UseAlt;
		else     ctrl = CON_Use;
				
		repeat = true;
		status = CONS_Down;
	}
	// controls except a few reset a previously given command
	else if (status != CONS_Moved)
		SetCommand("None");
	
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
	if (ctrl == CON_AimAxisUp || ctrl == CON_AimAxisDown || ctrl == CON_AimAxisLeft || ctrl == CON_AimAxisRight)
	{
		var success = VirtualCursor()->Aim(ctrl, this, strength, repeat, status);
		// in any case, CON_Aim* is called but it is only successful if the virtual cursor is aiming
		return success && VirtualCursor()->IsAiming();
	}
	
	// Simulate a mouse cursor for gamepads.
	if (HasVirtualCursor())
	{
		x = this.control.mlastx;
		y = this.control.mlasty;
	}
		
	// save last mouse position:
	// if the using has to be canceled, no information about the current x, y
	// is available. Thus, the last x, y position needs to be saved
	else if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		this.control.mlastx = x;
		this.control.mlasty = y;
	}

	var proc = GetProcedure();
	
	// building, vehicle, mount, contents, menu control
	var house = Contained();
	var vehicle = GetActionTarget();
	// the clonk can have an action target even though he lost his action. 
	// That's why the clonk may only interact with a vehicle if in an
	// appropiate procedure:
	if (proc != "ATTACH" && proc != "PUSH")
		vehicle = nil;
	
	// menu
	if (this.control.menu)
	{
		return Control2Menu(ctrl, x, y, strength, repeat, status);
	}
	
	var contents = this->GetHandItem(0);
	
	// usage
	var use = (ctrl == CON_Use || ctrl == CON_UseAlt);
	if (use)
	{
		if (house)
		{
			return ControlUse2Script(ctrl, x, y, strength, repeat, status, house);
		}
		// control to grabbed vehicle
		else if (vehicle && proc == "PUSH")
		{
			return ControlUse2Script(ctrl, x, y, strength, repeat, status, vehicle);
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

			if (ControlUse2Script(ctrl, x, y, strength, repeat, status, vehicle))
				return true;
			else
			{
				// handled if the horse is the used object
				// ("using" is set to the object in StartUseControl - when the
				// object returns true on that callback. Exactly what we want)
				if (GetUsedObject() == vehicle) return true;
				// has been cancelled (it is not the start of the usage but no object is used)
//				if (vehicle && !GetUsedObject() && (repeat || status == CONS_Up)) return true;
			}
		}
		// releasing the use-key always cancels shelved commands (in that case no GetUsedObject() exists)
		if (status == CONS_Up) StopShelvedCommand();
		// Release commands are always forwarded even if contents is 0, in case we
		// need to cancel use of an object that left inventory
		if (contents || (status == CONS_Up && GetUsedObject()))
		{
			if (ControlUse2Script(ctrl, x, y, strength, repeat, status, contents))
				return true;
		}
	}
	
	// A click on throw can also just abort usage without having any other effects.
	// todo: figure out if wise.
	var currently_in_use = GetUsedObject() != nil;
	if (ctrl == CON_Throw && currently_in_use && status == CONS_Down)
	{
		CancelUse();
		return true;
	}
	
	// Throwing and dropping
	// only if not in house, not grabbing a vehicle and an item selected
	// only act on press, not release
	if (ctrl == CON_Throw && !house && (!vehicle || proc == "ATTACH" || proc == "PUSH") && status == CONS_Down)
	{		
		if (contents)
		{
			// Object overloaded throw control?
			// Call this before QueryRejectDeparture to allow alternate use of non-droppable objects
			if (contents->~ControlThrow(this, x, y))
				return true;
			
			// The object does not want to be dropped? Still handle command.
			if (contents->~QueryRejectDeparture(this))
				return true;
			
			// Quick-stash into grabbed vehicle?
			if (vehicle && proc == "PUSH" && vehicle->~IsContainer())
			{
				CancelUse();
				vehicle->Collect(contents);
				if (!contents || contents->Contained() != this)
					Sound("Hits::SoftTouch*", false, nil, GetOwner());
				return true;
			}
			
			// just drop in certain situations
			var only_drop = proc == "SCALE" || proc == "HANGLE" || proc == "SWIM";
			// also drop if no throw would be possible anyway
			if (only_drop || Distance(0, 0, x, y) < 10 || (Abs(x) < 10 && y > 10))
				only_drop = true;
			// throw
			CancelUse();

			if (only_drop)
				return ObjectCommand("Drop", contents);
			else
			{
				if (HasVirtualCursor() && !VirtualCursor()->IsActive())
				{
					var angle = DEFAULT_THROWING_ANGLE * (GetDir()*2 - 1);
					x = +Sin(angle, CURSOR_Radius, 10);
					y = -Cos(angle, CURSOR_Radius, 10);
				}
				return ObjectCommand("Throw", contents, x, y);
			}
		}
	}
	
	// Movement controls (defined in PlayerControl.c, partly overloaded here)
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
	{	
		// forward to script...
		if (house)
		{
			return ControlMovement2Script(ctrl, x, y, strength, repeat, status, house);
		}
		else if (vehicle)
		{
			if (ControlMovement2Script(ctrl, x, y, strength, repeat, status, vehicle)) return true;
		}
	
		return ObjectControlMovement(plr, ctrl, strength, status);
	}
	
	// Do a roll on landing or when standing. This means that the CON_Down was not handled previously.
	if (ctrl == CON_Roll && ComDir2XY(GetComDir())[0] != 0)
	{
		if (this->IsWalking())
		{
			if (this->Stuck())
			{
				// Still show some visual feedback for the player.
				this->DoKneel();
			}
			else
			{
				this->DoRoll();
			}
			return true;
		}
	}

	// Fall through half-solid mask
	if (ctrl == CON_FallThrough)
	{
		if (status == CONS_Down)
		{
			if (this->IsWalking())
			{
				HalfVehicleFadeJumpStart();
			}
		}
		else
		{
			HalfVehicleFadeJumpStop();
		}
		return true;
	}
	
	// hotkeys action bar hotkeys
	var hot = 0;
	if (ctrl == CON_InteractionHotkey0) hot = 10;
	if (ctrl == CON_InteractionHotkey1) hot = 1;
	if (ctrl == CON_InteractionHotkey2) hot = 2;
	if (ctrl == CON_InteractionHotkey3) hot = 3;
	if (ctrl == CON_InteractionHotkey4) hot = 4;
	if (ctrl == CON_InteractionHotkey5) hot = 5;
	if (ctrl == CON_InteractionHotkey6) hot = 6;
	if (ctrl == CON_InteractionHotkey7) hot = 7;
	if (ctrl == CON_InteractionHotkey8) hot = 8;
	if (ctrl == CON_InteractionHotkey9) hot = 9;
	
	if (hot > 0)
	{
		this.control.hotkeypressed = true;
		this->~ControlHotkey(hot-1);
		this->~StopInteractionCheck(); // for GUI_Controller_ActionBar
		return true;
	}
	
	// Unhandled control
	return _inherited(plr, ctrl, x, y, strength, repeat, status, ...);
}

// A wrapper to SetCommand to catch special behaviour for some actions.
public func ObjectCommand(string command, object target, int tx, int ty, object target2, /*any*/ data)
{
	// special control for throw and jump
	// but only with controls, not with general commands
	if (command == "Throw")
		return this->~ControlThrow(target, tx, ty);
	else if (command == "Jump")
		return this->~ControlJump();
	// else standard command
	else 
	{
		// Make sure to not recollect the item immediately on drops.
		if (command == "Drop")
		{
			// Disable collection for a moment.
			if (target)
				this->OnDropped(target);
		}
		return SetCommand(command, target, tx, ty, target2, data);
	}
	// this function might be obsolete: a normal SetCommand does make a callback to
	// script before it is executed: ControlCommand(szCommand, pTarget, iTx, iTy)
}

/*
	Called by the engine before a command is executed.
	Beware that this is NOT called when SetCommand was called by a script.
	At this point I am not sure whether we need this callback at all.
*/
public func ControlCommand(string command, object target, int tx, int ty)
{
	if (command == "Drop")
	{
		// Disable collection for a moment.
		if (target) this->OnDropped(target);
	}
	return _inherited(command, target, tx, ty, ...);
}

/* ++++++++++++++++++++++++ Movement Controls ++++++++++++++++++++++++ */

// Control use redirected to script
func ControlMovement2Script(int ctrl, int x, int y, int strength, bool repeat, int status, object obj)
{
	// overloads of movement commandos
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Down || ctrl == CON_Up || ctrl == CON_Jump)
	{
		var control_string = "Control";
		if (Contained() == obj) 
			control_string = "Contained";
	
		if (status == CONS_Up)
		{
			// if any movement key has been released, ControlStop is called
			if (obj->Call(Format("~%sStop", control_string), this, ctrl))
				return true;
		}
		else
		{
			// what about gamepad-deadzone?
			if (strength != nil && strength < CON_Gamepad_Deadzone)
				return true;
			
			// Control*
			if (ctrl == CON_Left)  if (obj->Call(Format("~%sLeft",control_string),this))  return true;
			if (ctrl == CON_Right) if (obj->Call(Format("~%sRight",control_string),this)) return true;
			if (ctrl == CON_Up)    if (obj->Call(Format("~%sUp",control_string),this))    return true;
			if (ctrl == CON_Down)  if (obj->Call(Format("~%sDown",control_string),this))  return true;
			
			// for attached (e.g. horse: also Jump command
			if (GetProcedure() == "ATTACH")
				if (ctrl == CON_Jump)  if (obj->Call("ControlJump",this)) return true;
		}
	}

}

// Effect to free/unfree hands by disabling/enabling scale and hangle procedures
public func FxIntControlFreeHandsStart(object target, proplist fx, int temp)
{
	// Process on non-temp as well in case scale/handle effects need to stack
	// Stop current action
	var proc = GetProcedure();
	if (proc == "SCALE" || proc == "HANGLE") SetAction("Walk");
	// Make sure ActMap is writable
	if (this.ActMap == this.Prototype.ActMap) this.ActMap = new this.ActMap{};
	// Kill scale/hangle effects
	fx.act_scale = this.ActMap.Scale;
	this.ActMap.Scale = nil;
	fx.act_hangle = this.ActMap.Hangle;
	this.ActMap.Hangle = nil;
	return FX_OK;
}

public func FxIntControlFreeHandsStop(object target, proplist fx, int reason, bool temp)
{
	// Restore scale/hangle effects (engine will handle re-grabbing walls if needed)
	if (fx.act_scale) this.ActMap.Scale = fx.act_scale;
	if (fx.act_hangle) this.ActMap.Hangle = fx.act_hangle;
	return FX_OK;
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

public func IsMounted() { return GetProcedure() == "ATTACH"; }

/*-- Throwing --*/

// Throwing
func DoThrow(object obj, int angle)
{
	// parameters...
	var iX, iY, iR, iXDir, iYDir, iRDir;
	iX = 4; if (!GetDir()) iX = -iX;
	iY = Cos(angle,-4);
	iR = Random(360);
	iRDir = RandomX(-10, 10);

	iXDir = Sin(angle, this.ThrowSpeed);
	iYDir = Cos(angle,-this.ThrowSpeed);
	// throw boost (throws stronger upwards than downwards)
	if (iYDir < 0) iYDir = iYDir * 13/10;
	if (iYDir > 0) iYDir = iYDir * 8/10;
	
	// add own velocity
	iXDir += GetXDir(100)/2;
	iYDir += GetYDir(100)/2;

	// throw
	obj->Exit(iX, iY, iR, 0, 0, iRDir);
	obj->SetXDir(iXDir, 100);
	obj->SetYDir(iYDir, 100);
	
	// Prevent hitting the thrower.
	var block_blow = AddEffect("BlockBlowControl", this, 100, 3, this);
	block_blow.obj = obj;
	return true;
}

// custom throw
// implemented in Clonk.ocd/Animations.ocd
public func ControlThrow() { return _inherited(...); }

// Effect for blocking a blow by an object.
public func FxBlockBlowControlTimer()
{
	return FX_Execute_Kill;
}

public func FxBlockBlowControlQueryCatchBlow(object target, effect fx, object obj)
{
	if (obj == fx.obj)
		return true;
	return false;
}

/*-- Jumping --*/


/*
 Triggers a regular jump, that means that the speed in y direction
 is automatically decided, depending on the action of the clonk.
 
 If you want to execute a jump with a certain speed, use ControlJumpExecute().
 */
public func ControlJump()
{
	var ydir = 0;
	
	if (GetProcedure() == "WALK")
	{
		ydir = this.JumpSpeed;
	}
	
	if (InLiquid() && !GBackSemiSolid(0, -5))
	{
		ydir = BoundBy(this.JumpSpeed * 3 / 5, 240, 380);
	}

	// Jump speed of the wall kick is halved.
	if (GetProcedure() == "SCALE" || GetAction() == "Climb")
	{
		ydir = this.JumpSpeed / 2;
	}
	
	return ControlJumpExecute(ydir);
}


/*
 Additional function for actually triggering a jump directly.
 
 The parameter ydir can be decided directly by the user,
 or you can use the clonk's jump speed by passing this.JumpSpeed
 
 Returns false if the jump was not successful.
 */
public func ControlJumpExecute(int ydir)
{
	if (ydir && !Stuck())
	{
		SetPosition(GetX(), GetY() - 1);

		// Wall kick if scaling or climbing.
		if (GetProcedure() == "SCALE" || GetAction() == "Climb")
		{
			AddEffect("WallKick", this, 1);
			var xdir;
			if (GetDir() == DIR_Right)
			{
				xdir = -1;
				SetDir(DIR_Left);
			}
			else if (GetDir() == DIR_Left)
			{
				xdir = 1;
				SetDir(DIR_Right);
			}

			SetYDir(-ydir * GetCon(), 100 * 100);
			SetXDir(xdir * 17);
			// Set speed first to have proper animations when jump starts.
			SetAction("Jump");
			return true;
		}
		//Normal jump
		else
		{
			SetYDir(-ydir * GetCon(), 100 * 100);
			// Set speed first to have proper animations when jump starts.
			SetAction("Jump");
			return true;
		}
	}
	return false;
}


// Interaction with clonks is special:
// * The clonk opening the menu should always have higher priority so the clonk is predictably selected on the left side even if standing behind e.g. a crate
// * Other clonks should be behind because interaction with them is rare but having your fellow players stand in front of a building is very common
//   (Allies also tend to run in front just when you opened that menu...)
func GetInteractionPriority(object target)
{
	// Self with high priority
	if (target == this) return 100;
	// Dead Clonks are shown (for a death message e.g.) but sorted to the bottom.
	if (!GetAlive()) return -190;
	var owner = NO_OWNER;
	if (target) owner = target->GetOwner();
	// Prefer own clonks for item transfer
	if (owner == GetOwner()) return -100;
	// If no own clonk, prefer friendly
	if (!Hostile(owner, GetOwner())) return -120;
	// Hostile clonks? Lowest priority.
	return -200;
}
