#strict 2

// Functions to handle player controls (i.e., input keys)

// PlayerControlRelease
// Called by engine whenever a control is issued
// Forwards control to special handler or cursor
// Return whether handled
global func PlayerControl(int plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat, bool release)
{
  //Log("%d, %s, %i, %d, %d, %d, %v, %v", plr, GetPlayerControlName(ctrl), spec_id, x,y,strength, repeat, release);
  // Control handled by definition? Forward
  if (spec_id) return spec_id->PlayerControl(plr, ctrl, x, y, strength, repeat, release);
  // Forward control to cursor
  var cursor = GetCursor(plr);
  if (cursor) 
  {
    // Object controlled by plr
    cursor->SetController(plr);
    // Overload by effect?
    if (cursor->Control2Effect(plr, ctrl, x, y, strength, repeat, release)) return true;
    return cursor->ObjectControl(plr, ctrl, x,y, strength, repeat, release);
  }
  // No cursor? Nothing to handle control then
  return false;
}



/* Object functions */
// To be called in an object context only!

// Control2Effect
// Call control function in all effects that have "Control" in their name
global func Control2Effect(int ctrl, int x, int y, int strength, bool repeat, bool release)
  {
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
  // Any control resets a previously given command
  SetCommand(this, "None");
  // Movement controls
  if (ctrl==CON_Left || ctrl==CON_Right || ctrl==CON_Up || ctrl==CON_Down)
  {
    return ObjectControlMovement(plr, ctrl, strength, release);
  }
  // Unhandled control
  return false;
}

// ObjectControlMovement
// Called when CON_Left/Right/Up/Down controls are issued/released
// Return whether handled
global func ObjectControlMovement(int plr, int ctrl, int strength, bool release)
{
  var proc = GetProcedure();
  // Some specific movement controls
  if (!release)
  {
    // Jump control
    if (ctrl == CON_Up && proc == "WALK")
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
    else if (proc == "FIGHT") // Clonk-to-Clonk-fight. Might want to implement some more sophisticated behaviour here?
    {
      // stop, but don't abort ComDir processing. May want to do Stop while holding a direction to run away?
      if (ctrl == CON_Down) ObjectComStop();
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

// ObjectControlUpdateComdir
// Updates ComDir of object based on current Con_*-directional controls
// Return whether actual, effective direction of movement changed
global func ObjectControlUpdateComdir(int plr)
{
  // Generic movement: Update ComDir based on current control state
  var new_comdir = GetPlayerConDir(plr, CON_Left, CON_Up, CON_Right, CON_Down);
  var old_comdir = GetComDir();
  if (new_comdir != old_comdir)
  {
    // ComDir changed. Update.
    SetComDir(new_comdir);
    //var s = "";
    //if (GetPlayerControlState(plr, CON_Left))  s = Format("%sL", s);
    //if (GetPlayerControlState(plr, CON_Up))    s = Format("%sU", s);
    //if (GetPlayerControlState(plr, CON_Right)) s = Format("%sR", s);
    //if (GetPlayerControlState(plr, CON_Down))  s = Format("%sD", s);
    //s = Format("%s %s", s, ["Stop", "Up", "UpRight", "Right", "DownRight", "Down", "DownLeft", "Left", "UpLeft"][new_comdir]);
    //Message("@%s", this, s);
    // The control is only handled if it had an actual effect on the current movement direction of the Clonk
    var old_cx,old_cy,new_cx,new_cy;
    ComDir2XY(old_comdir, old_cx, old_cy);
    ComDir2XY(new_comdir, new_cx, new_cy);
    var is_handled;
    var proc = GetProcedure();
    if (proc == "WALK" || proc == "HANGLE" || proc == "PUSH" || proc == "PULL")
      // Only horizontal movement changed actual direction
      // Also, enfore clear Left/Right commands without leftover Up/Down
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

// GetPlayerControlName
// Helper function to turn CON_*-constants into strings
global func GetPlayerControlName(int ctrl)
{
  if (ctrl == CON_Left  ) return "Left";
  if (ctrl == CON_Right ) return "Right";
  if (ctrl == CON_Up    ) return "Up";
  if (ctrl == CON_Down  ) return "Down";
  if (ctrl == CON_Throw ) return "Throw";
  if (ctrl == CON_Dig   ) return "Dig";
  return Format("Unknown(%d)", ctrl);
}

// GetPlayerConDir
// Return COMD_*-constant corresponding to current state of passed directional controls
global func GetPlayerConDir(int plr, int con_left, int con_up, int con_right, int con_down)
{
  var x,y;
  if (GetPlayerControlState(plr, con_left))  --x;
  if (GetPlayerControlState(plr, con_up))    --y;
  if (GetPlayerControlState(plr, con_right)) ++x;
  if (GetPlayerControlState(plr, con_down))  ++y;
  // Creating an array here for every keypress/release
  // Would be so cool to have this static const. Guenther?
  var dir_coms = [COMD_UpLeft, COMD_Up, COMD_UpRight, COMD_Left, COMD_None, COMD_Right, COMD_DownLeft, COMD_Down, COMD_DownRight];
  return dir_coms[y*3+x+4];
}

// ComDir2XY
// Returns coordinate directions associated with a COMD_Constant
global func ComDir2XY(int comd, &x, &y)
{
  // Creating an array here for every keypress/release
  // Would be so cool to have this static const. Guenther?
  x = [0,0,1,1,1,0,-1,-1,-1][comd];
  y = [0,-1,-1,0,1,1,1,0,-1][comd];
  return true;
}

// PlayerObjectCommand
// Give a command to all selected Clonks of a player
global func PlayerObjectCommand(int plr, bool exclude_cursor, string command, object target, int tx, int ty, object target2)
{
  for (var i=exclude_cursor; i<GetSelectCount(plr); ++i)
  {
    var follow_clonk = GetCursor(plr, i);
    if (follow_clonk)
    {
      SetCommand(follow_clonk,command,target,tx,ty);
    }
  }
  return true;
}

// ObjectComStop
// Stop action and ComDir
global func ObjectComStop()
{
  SetComDir();
  SetAction("Idle");
  if (!SetAction("Walk")) return false;
  SetXDir(); SetYDir();
  return true;
}

// ObjectComLetGo
// Let go from scaling or hangling
global func ObjectComLetGo(int vx, int vy)
{
  if (!SetAction("Jump")) return false;
  SetXDir(vx); SetYDir(vy);
  return true;
}
