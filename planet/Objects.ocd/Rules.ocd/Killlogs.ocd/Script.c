/*-- Kill logs --*/

func Initialize()
{
	// Only one at a time.
	if (ObjectCount(Find_ID(GetID())) > 1) 
		return RemoveObject();
}

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}

func OnClonkDeath(object clonk, int killed_by)
{
	var plr = clonk->GetOwner();
	// Only log for existing players and clonks.
	if (plr == NO_OWNER || !GetPlayerName(plr) || !clonk) 
		return;
	ScheduleCall(this, "OnClonkDeathEx", 1, 0, clonk, plr, killed_by);
	return _inherited(clonk, killed_by, ...);
}

// parameters: clonk, owner, killed_by
global func GetAdditionalPlayerRelaunchString(){return _inherited(...);} // dummy

public func OnClonkDeathEx(object clonk, int plr, int killed_by)
{
	if (!GetPlayerName(plr))
		return;
	var name = "Clonk";
	if (clonk)
		name = clonk.Prototype->GetName();
	// Assert there are three StringTbl entries for each.
	var which_one = Random(3) + 1;
	var log = "";
	if (!GetPlayerName(killed_by))
 		 log = Format(Translate(Format("KilledByGaia%d", which_one)), GetTaggedPlayerName(plr), name);
 	else if (plr == killed_by)
		log = Format(Translate(Format("Selfkill%d", which_one)), GetTaggedPlayerName(plr), name);
 	else if (!Hostile(plr,killed_by))
  		log = Format(Translate(Format("Teamkill%d", which_one)), GetTaggedPlayerName(plr), name, GetTaggedPlayerName(killed_by));
	else
		log = Format(Translate(Format("KilledByPlayer%d", which_one)), GetTaggedPlayerName(plr), name, GetTaggedPlayerName(killed_by));
	
	if (IsActiveRelaunchRule())
	{
		var relaunches = GetRelaunchRule()->GetPlayerRelaunchCount(plr);
		if (relaunches != nil)
		{
			var msg = "";
			if (relaunches < 0) // Player eliminated.
				msg = Format("$MsgFail$", GetTaggedPlayerName(plr));
			else if (relaunches == 0) // Last relaunch.
				msg = Format("$MsgRelaunch0$", GetTaggedPlayerName(plr));
			else if (relaunches == 1) // One relaunch remaining.
				msg = Format("$MsgRelaunch1$", GetTaggedPlayerName(plr));
			else // Multiple relaunches remaining.
				msg = Format("$MsgRelaunchX$", GetTaggedPlayerName(plr), relaunches);
			log = Format("%s %s", log, msg);
		}
	}
	
	// This is also not a global function, but that is okay. So..
	// get additional strings from goals/rules ("%s is now king!!")
	for (var goal in FindObjects(Find_Or(Find_Category(C4D_Goal), Find_Category(C4D_Rule))))
	{
		var other = goal->~GetAdditionalPlayerRelaunchString(clonk, plr, killed_by);
		if (other)
			log = Format("%s %s", log, other);
	}
	
	// Get additional stuff from scenario.
	var other = GameCall("GetAdditionalPlayerRelaunchString", clonk, plr, killed_by);
	if (other)
		log = Format("%s %s", log, other);
	
	// Also allow global callback function to add to death messages.
	other = GetAdditionalPlayerRelaunchString(clonk, plr, killed_by);
	if (other)
		log = Format("%s, %s", log, other);
		
	Log(log);
	return;
}

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
