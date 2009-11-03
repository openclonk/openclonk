/*-- Standard controls --*/

// currently selected
local selected;

local throwAngle;

/* Item limit */

public func MaxContentsCount() { return 3; }

/* Item select access*/

public func Select(int selection)
{
	var item = Contents(selected);
	// de-select previous (if any)
	if(item) item->~Deselection(this);
	
	// select new (if any)
	selected = selection;
	var item = Contents(selected);
	
	// DEBUG
	var bla = "nothing";
	if(item) bla = item->~GetName();
	Message("selected %s (position %d)", this, bla, selected);
	
	if (!item) return;
	if (item->~Selection(this)) return;
	
	Sound("Grab");
}

public func GetSelection()
{
	return selected;
}

protected func Initialize()
{
	selected = 0;
}

/* Main control function */
public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) return false;
	
	// Any control resets a previously given command
	SetCommand("None");

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
	
	if (hot > 0) return this->~ControlHotkey(hot-1);
	
	var proc = GetProcedure();

	// building, vehicle, contents control
	var house = Contained();
	var vehicle = GetActionTarget();
	var contents = Contents(selected);

	if (house)
	{
		if (Control2Script(ctrl, x, y, strength, repeat, release, "Contained", house))
			return true;
	}
	else if (vehicle)
	{
		// control to grabbed vehicle or riding etc.
		if (proc == "PUSH" || proc == "ATTACH")
			if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", vehicle))
				return true;
	}
	else if (contents)
	{
		// out of convencience we call Control2Script, even though it can handle
		// left, right, up and down, too. We don't want that, so this is why we
		// check that ctrl is Use.
		if (ctrl == CON_Use)
			if (Control2Script(ctrl, x, y, strength, repeat, release, "Control", contents))
				return true;
	}
	
	if(!vehicle && !house)
	{
		if(ctrl == CON_Jump)  if(this->~ControlJump(this)) return true;
	}
	
	// everything down from here:
	// standard controls that are called if not overloaded via script
	
	// Movement controls
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
		return ObjectControlMovement(plr, ctrl, strength, release);

	// Push controls
	if (ctrl == CON_Grab || ctrl == CON_Ungrab || ctrl == CON_PushEnter || ctrl == CON_GrabPrevious || ctrl == CON_GrabNext)
		return ObjectControlPush(plr, ctrl);

	// Entrance controls
	if (ctrl == CON_Enter || ctrl == CON_Exit)
		return ObjectControlEntrance(plr,ctrl);

	// Inventory control
	if (ctrl == CON_NextItem)
	{
		var sel = selected;
		sel++;
		if (sel >= MaxContentsCount()) sel = 0;
		
		Select(sel);
		
		return true;
	}
	if (ctrl == CON_PreviousItem)
	{
		var sel = selected;
		sel--;
		if (sel <= 0) sel = MaxContentsCount()-1;
		
		Select(sel);
		
		return true;
	}

	// only if not in house, not grabbing a vehicle and an item selected
	if(!house && !vehicle && contents)
	{
		// throw
		if (ctrl == CON_Throw)
		{
		    if (proc == "SCALE" || proc == "HANGLE")
		      return PlayerObjectCommand(plr, false, "Drop", contents);
		    else
		      return PlayerObjectCommand(plr, false, "Throw", contents, x, y);
		}
		// drop
		if (ctrl == CON_Drop)
		{
			return PlayerObjectCommand(plr, false, "Drop", contents);
		}
	}
	
	// Unhandled control
	return false;
}

public func ObjectCommand(string command, object target, int tx, int ty)
{
	// special control for throw and jump
	// but only with controls, not with general commands
	if (command == "Throw") ControlThrow(this,tx,ty);
	else if (command == "Jump") ControlJump(this);
	// else standard command
	else SetCommand(command,target,tx,ty);
}

// Control redirected to script
private func Control2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, string control, object obj)
{
	// for the use command
	if (ctrl == CON_Use)
	{
		var handled = false;
		
		if(!release && !repeat)
			handled = obj->Call(Format("~%sUse",control),this,x,y);
		else if(release)
			handled = obj->Call(Format("~%sUseStop",control),this,x,y);
		else
			handled = obj->Call(Format("~%sUseHolding",control),this,x,y);
		
		return handled;
	}
	// overloads of movement commandos
	else if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Down || ctrl == CON_Up)
	{
		if (release)
		{
			// if any movement key has been released, ControlStop is called
			if (obj->Call(Format("~%sStop",control),this))  return true;
		}
		else
		{
			// Control*
			if (ctrl == CON_Left)  if (obj->Call(Format("~%sLeft",control),this))  return true;
			if (ctrl == CON_Right) if (obj->Call(Format("~%sRight",control),this)) return true;
			if (ctrl == CON_Up)    if (obj->Call(Format("~%sUp",control),this))    return true;
			if (ctrl == CON_Down)  if (obj->Call(Format("~%sDown",control),this))  return true;
		}
	}
	
	return false;
}

// Handles enter and exit
private func ObjectControlEntrance(int plr, int ctrl)
{
	var proc = GetProcedure();

	// enter
	if (ctrl == CON_Enter)
	{
		// enter only if... one can
		if (proc != "WALK" && proc != "SWIM" && proc != "SCALE" &&
			proc != "HANGLE" && proc != "FLOAT" && proc != "FLIGHT") return false;

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

		PlayerObjectCommand(plr, false, "Ungrab");
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

		PlayerObjectCommand(plr, false, "PushTo", obj);
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

// Throwing
private func Throwing()
{
  // throw selected inventory
  var obj = Contents(selected);
  if(!obj) return;
  
  // parameters...
  var iX, iY, iR, iXDir, iYDir, iRDir;
  iX = 8; if (!GetDir()) iX = -iX;
  iY = Cos(throwAngle,-8);
  iR = Random(360);
  iRDir = RandomX(-10,10);

  var speed = GetPhysical("Throw");

  iXDir = speed * Sin(throwAngle,1000) / 17000;
  iYDir = speed * Cos(throwAngle,-1000) / 17000;
  // throw boost (throws stronger upwards than downwards)
  if(iYDir < 0) iYDir = iYDir * 13/10;
  if(iYDir > 0) iYDir = iYDir * 8/10;
  
  // is riding? add it's velocity
  if (GetActionTarget())
  {
    iXDir += GetActionTarget()->GetXDir() / 10;
    iYDir += GetActionTarget()->GetYDir() / 10;
  }
  // throw
  obj->Exit(iX, iY, iR, 0, 0, iRDir);  
  obj->SetXDir(iXDir,1000);
  obj->SetYDir(iYDir,1000);
}

// custom throw
public func ControlThrow(object me, int x, int y)
{
	// standard throw after all
	if(!x && !y) return false;
	
	if(!Contents(selected)) return false;
	
	throwAngle = Angle(0,0,x,y);
	
	// walking (later with animation: flight, scale, hangle?)
	if(GetProcedure() == "WALK")
	{
		if(throwAngle < 180) SetDir(DIR_Right);
		else SetDir(DIR_Left);
		SetAction("Throw");
		return true;
	}
	// riding
	if(GetAction() == "Ride" || GetAction() == "RideStill")
	{
		SetAction("RideThrow");
		return true;
	}
	return false;
}

public func ControlJump(object me)
{

}
