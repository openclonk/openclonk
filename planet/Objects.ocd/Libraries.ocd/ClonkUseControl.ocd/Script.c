/**
	Clonk use controls
	Author: Newton

	Objects that inherit this object need to return _inherited(...) in the
	following callbacks (if defined):
		Construction, Departure,
		Entrance, AttachTargetLost, CrewSelection, Death,
		Destruction
	
	The following callbacks are made to other objects:
		*Use, *UseStop, *UseStart, *UseHolding, *UseCancel

	Used properties
	
	this.control.current_object: object that is being used at the moment
	this.control.using_type: way of usage
	this.control.mlastx: last x position of the cursor
	this.control.mlasty: last y position of the cursor
	this.control.noholdingcallbacks: whether to do HoldingUseControl callbacks
	this.control.shelved_command: command (function) with condition that will be executed when the condition is met
		used for example to re-call *Use/Throw commands when the Clonk finished scaling


	wheras * is 'Contained' if the clonk is contained and otherwise (riding,
	pushing, to self) it is 'Control'. The item in the inventory only gets
	the Use*-calls. If the callback is handled, you should return true.
	Currently, this is explained more in detail here:
	http://forum.openclonk.org/topic_show.pl?tid = 337
*/

/* ++++++++++++++++++++++++ Callbacks ++++++++++++++++++++++++ */

protected func Construction()
{
	if (this.control == nil)
		this.control = {};

	this.control.current_object = nil;
	this.control.using_type = nil;
	this.control.shelved_command = nil;

	return _inherited(...);
}

public func GetUsedObject()
{
	return this.control.current_object;
}

// The using-command hast to be canceled if the clonk is entered into
// or exited from a building.
protected func Entrance()
{
	CancelUse();
	return _inherited(...);
}


protected func Departure()
{
	CancelUse();
	return _inherited(...);
}

// The same for vehicles
protected func AttachTargetLost()
{
	CancelUse();
	return _inherited(...);
}

// ...aaand the same for when the clonk is deselected
protected func CrewSelection(bool unselect)
{
	if (unselect)
	{
		// cancel usage on unselect first...
		CancelUse();
	}

	return _inherited(unselect, ...);
}

protected func Destruction()
{
	// cancel usage...
	CancelUse();
	return _inherited(...);
}

protected func Death()
{
	// cancel usage...
	CancelUse();
	return _inherited(...);
}


/* ++++++++++++++++++++++++ Shelved command ++++++++++++++++++++++++ */

public func ShelveCommand(object condition_obj, string condition, object callback_obj, string callback, proplist data)
{
	this.control.shelved_command =
	{
		cond = condition,
		cond_obj = condition_obj,
		callback = callback,
		callback_obj = callback_obj,
		data = data
	};
	AddEffect("ShelvedCommand", this, 1, 5, this);
}

public func StopShelvedCommand()
{
	this.control.shelved_command = nil;
	if (GetEffect("ShelvedCommand", this))
		RemoveEffect("ShelvedCommand", this);
}

func FxShelvedCommandTimer(_, effect, time)
{
	if (!this.control.shelved_command) return -1;
	if (!this.control.shelved_command.callback_obj) return -1;
	if (!this.control.shelved_command.cond_obj) return -1;
	if (!this.control.shelved_command.cond_obj->Call(this.control.shelved_command.cond, this.control.shelved_command.data)) return 1;
	this.control.shelved_command.callback_obj->Call(this.control.shelved_command.callback, this.control.shelved_command.data);
	return -1;
}

func FxShelvedCommandStop(target, effect, reason, temp)
{
	if (temp)
		return;
	this.control.shelved_command = nil;
}


/* ++++++++++++++++++++++++ Use Controls ++++++++++++++++++++++++ */

public func CancelUse()
{
	if (!GetUsedObject())
	{
		// just forget any possibly stored actions
		StopShelvedCommand();
		return;
	}

	// use the saved x, y coordinates for canceling
	CancelUseControl(this.control.mlastx, this.control.mlasty);
}

// to be called during usage of an object to re-start usage as soon as possible
func PauseUse(object obj, string custom_condition, proplist data)
{
	// cancel use first, since it removes old shelved commands
	if (this.control.started_use)
	{
		CancelUse();
		this.control.started_use = false;
	}
	
	var callback_obj = this;
	
	if (custom_condition != nil)
	{
		callback_obj = obj;
	}
	else
	{
		custom_condition = "CanReIssueCommand";
	}

	data = data ?? {};
	data.obj = obj;
	data.ctrl = CON_Use;
	ShelveCommand(callback_obj, custom_condition, this, "ReIssueCommand", data);
}

func DetermineUsageType(object obj)
{
	if (!obj)
		return nil;
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

func GetUseCallString(string action)
{
	// Control... or Contained...
	var control_string = "Control";
	if (this.control.using_type == C4D_Structure)
		control_string = "Contained";
	// ..Use.. or ..UseAlt...
	var estr = "";
	if (this.control.alt && this.control.using_type != C4D_Object)
		estr = "Alt";
	// Action
	if (!action)
		action = "";
	return Format("~%sUse%s%s", control_string, estr, action);
}

func CanReIssueCommand(proplist data)
{
	if (!data.obj)
		return false;
	
	if (data.ctrl == CON_Use)
		return !data.obj->~RejectUse(this);
}

func ReIssueCommand(proplist data)
{
	if (data.ctrl == CON_Use)
		return StartUseControl(data.ctrl, this.control.mlastx, this.control.mlasty, data.obj);
}

func StartUseControl(int ctrl, int x, int y, object obj)
{
	this.control.started_use = false;
	
	if (obj->~RejectUse(this))
	{
		// remember for later:
		PauseUse(obj);
		// but still catch command
		return true;
	}

	// Disable climb/hangle actions for the duration of this use
	if (obj.ForceFreeHands && !GetEffect("IntControlFreeHands", this))
		AddEffect("IntControlFreeHands", this, 130, 0, this);
	
	obj->SetController(GetController());
	this.control.current_object = obj;
	this.control.using_type = DetermineUsageType(obj);
	this.control.alt = ctrl != CON_Use;
	
	if (this->~HasVirtualCursor())
	{
		var cursor = this->~VirtualCursor(), angle;
		if (!cursor->IsActive() && (angle = obj->~DefaultCrosshairAngle(this, GetDir() * 2 - 1)))
		{
			x = +Sin(angle, CURSOR_Radius, 10);
			y = -Cos(angle, CURSOR_Radius, 10);
		}
		cursor->StartAim(this, angle);
	}

	var hold_enabled = obj->Call("~HoldingEnabled");

	if (hold_enabled)
		SetPlayerControlEnabled(GetOwner(), CON_Aim, true);

	// first call UseStart. If unhandled, call Use (mousecontrol)
	var handled = obj->Call(GetUseCallString("Start"), this, x, y);
	if (!handled)
	{
		handled = obj->Call(GetUseCallString(), this, x, y);
		this.control.noholdingcallbacks = handled;
	}
	else
	{
		// *Start was handled. So clean up possible old noholdingcallbacks-values.
		this.control.noholdingcallbacks = false;
	}
	
	if (!handled)
	{
		this.control.current_object = nil;
		this.control.using_type = nil;
		if (hold_enabled)
			SetPlayerControlEnabled(GetOwner(), CON_Aim, false);
		return false;
	}
	else
	{
		this.control.started_use = true;
		// add helper effect that prevents errors when objects are suddenly deleted by quickly cancelling their use beforehand
		AddEffect("ItemRemovalCheck", GetUsedObject(), 1, 100, this, nil); // the slow timer is arbitrary and will just clean up the effect if necessary
	}

	return handled;
}

func CancelUseControl(int x, int y)
{
	// forget possibly stored commands
	StopShelvedCommand();
	
	// to horse first (if there is one)
	var horse = GetActionTarget();
	if (horse && GetProcedure() == "ATTACH" && GetUsedObject() != horse)
		StopUseControl(x, y, horse, true);
	
	return StopUseControl(x, y, GetUsedObject(), true);
}

func StopUseControl(int x, int y, object obj, bool cancel)
{
	var stop = "Stop";
	if (cancel)
		stop = "Cancel";
	
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, ContainedUseCancel, etc...
	var handled = false;
	if (obj)
	{
		handled = obj->Call(GetUseCallString(stop), this, x, y);
	}
	
	if (obj == GetUsedObject())
	{
		// if ControlUseStop returned -1, the current object is kept as "used object"
		// but no more callbacks except for ControlUseCancel are made. The usage of this
		// object is finally cancelled on ControlUseCancel.
		if (cancel || handled != -1)
		{
			// look for correct removal helper effect and remove it
			var effect_index = 0;
			var removal_helper = nil;
			do
			{
				removal_helper = GetEffect("ItemRemovalCheck", GetUsedObject(), effect_index++);
				if (!removal_helper)
					break;
				if (removal_helper.CommandTarget != this)
					continue;
				break;
			} while (true);
			
			RemoveEffect("IntControlFreeHands", this); // make sure we can climb again

			this.control.current_object = nil;
			this.control.using_type = nil;
			this.control.alt = false;
			
			if (removal_helper)
			{
				RemoveEffect(nil, nil, removal_helper, true);
			}
		}
		this.control.noholdingcallbacks = false;
		
		SetPlayerControlEnabled(GetOwner(), CON_Aim, false);

		if (this->~HasVirtualCursor())
			this->~VirtualCursor()->StopAim();
	}

	return handled;
}

func HoldingUseControl(int ctrl, int x, int y, object obj)
{
	var mex = x;
	var mey = y;

	//Message("%d,%d",this, mex, mey);

	// automatic adjustment of the direction
	// --------------------
	// if this is desired for ALL objects is the question, we will find out.
	// For now, this is done for items and vehicles, not for buildings and
	// mounts (naturally). If turning vehicles just like that without issuing
	// a turning animation is the question. But after all, the clonk can turn
	// by setting the dir too.
	

	//   not riding and                not in building  not while scaling
	if (GetProcedure() != "ATTACH" && !Contained() && GetProcedure() != "SCALE")
	{
		// pushing vehicle: object to turn is the vehicle
		var dir_obj = GetActionTarget();
		if (GetProcedure() != "PUSH")
			dir_obj = nil;
		
		// otherwise, turn the clonk
		if (!dir_obj)
			dir_obj = this;
		
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
	
	var handled = obj->Call(GetUseCallString("Holding"), this, mex, mey);
	
	return handled;
}

// very infrequent timer to prevent dangling effects, this is not necessary for correct functioning
func FxItemRemovalCheckTimer(object target, proplist effect, int time)
{
	if (!effect.CommandTarget)
		return -1;
	if (effect.CommandTarget->~GetUsedObject() != target)
		return -1;
	return 1;
}

// this will be called when an inventory object (that is in use) is suddenly removed
func FxItemRemovalCheckStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return;
	// only trigger when the object has been removed
	if (reason != FX_Call_RemoveClear)
		return;
	// safety
	if (!effect.CommandTarget)
		return;
	if (effect.CommandTarget->~GetUsedObject() != target)
		return;
	// quickly cancel use in a clean way while the object is still available
	effect.CommandTarget->CancelUse();
	return;
}


// Control use redirected to script
func ControlUse2Script(int ctrl, int x, int y, int strength, bool repeat, int status, object obj)
{
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		// cancel usage if a menu pops up
		if (this->~GetMenu())
		{
			CancelUse();
			return true;
		}
	
		// standard use
		if (status == CONS_Down && !repeat)
		{
			return StartUseControl(ctrl, x, y, obj);
		}
		else if (status == CONS_Up && (obj == GetUsedObject() || obj == GetActionTarget()))
		{
			return StopUseControl(x, y, obj);
		}

		// more use (holding)
		if (status == CONS_Up)
		{
			// leftover use release
			CancelUse();
			return true;
		}
		else if (status == CONS_Down && repeat && !this.control.noholdingcallbacks)
		{
			return HoldingUseControl(ctrl, x, y, obj);
		}
	}
	
	return false;
}
