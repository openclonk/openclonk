/**
	Sequence
	
	Cutscene to be watched by all players.
	Start calling global func StartSequence, stop using StopSequence
*/

local seq_name;
local seq_progress;
local started;


/* Start and stop */

func Start(string name, int progress, ...)
{
	if (started) Stop();
	SetPosition(0,0); // force global coordinates
	this.seq_name = name;
	this.seq_progress = progress;
	// call init function of this scene - difference to start function is that it is called before any player joins
	var fn_init = Format("~%s_Init", seq_name);
	if (!Call(fn_init, ...))
		GameCall(fn_init, this, ...);
	// Disable crew of all players
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		JoinPlayer(plr);
	}
	started = true;
	// effect
	Sound("Ding", true);
	// call start function of this scene
	var fn_start = Format("%s_Start", seq_name);
	if (!Call(fn_start, ...))
		GameCall(fn_start, this, ...);
	return true;
}

public func IntializePlayer(int plr)
{
	JoinPlayer(plr);
	return true;
}

public func RemovePlayer(int plr)
{
	// call by sequence if it ends and by engine if player leaves
	var fn_remove = Format("~%s_RemovePlayer", seq_name);
	if (!Call(fn_remove, plr))
		GameCall(fn_remove, this, plr);
	return true;
}

public func JoinPlayer(int plr)
{
	var j=0, crew;
	while (crew = GetCrew(plr, j++))
	{
		//if (crew == GetCursor(plr)) crew.Sequence_was_cursor = true; else crew.Sequence_was_cursor = nil;
		crew->SetCrewEnabled(false);
		crew->CancelUse();
		if(crew->GetMenu()) if(!crew->GetMenu()->~Uncloseable()) crew->CancelMenu();
		crew->MakeInvincible();
		crew->SetCommand("None");
		crew->SetComDir(COMD_Stop);
	}
	// per-player sequence callback
	var fn_join = Format("~%s_JoinPlayer", seq_name);
	if (!Call(fn_join, plr))
		GameCall(fn_join, this, plr);
	return true;
}

public func Stop(bool no_remove)
{
	if (started)
	{
		SetViewTarget(nil);
		// Reenable crew and reset cursor
		for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User);
			var j=0, crew;
			while (crew = GetCrew(plr, j++))
			{
				crew->SetCrewEnabled(true);
				crew->ClearInvincible();
				//if (crew.Sequence_was_cursor) SetCursor(plr, crew);
			}
			// ensure proper cursor
			if (!GetCursor(plr)) SetCursor(plr, GetCrew(plr));
			if (crew = GetCursor(plr)) SetPlrView(plr, crew);
			// per-player sequence callback
			RemovePlayer(plr);
		}
		Sound("Ding", true);
		started = false;
		// call stop function of this scene
		var fn_init = Format("~%s_Stop", seq_name);
		if (!Call(fn_init))
			GameCall(fn_init, this);
	}
	if (!no_remove) RemoveObject();
	return true;
}

func Destruction()
{
	Stop(true);
}


/* Sequence callbacks */

func ScheduleNext(int delay, int skip)
{
	return ScheduleCall(this, this.CallNext, delay, 1, skip);
}

func ScheduleSame(int delay) { return ScheduleNext(delay, -1); }

func CallNext(int skip)
{
	// Start conversation context.
	// Update dialogue progress first.
	seq_progress += skip+1;
	// Then call relevant functions.
	var fn_progress = Format("%s_%d", seq_name, seq_progress);
	if (!Call(fn_progress))
		GameCall(fn_progress, this);
	return true;
}



/* Force view on target */

// Force all player views on given target
public func SetViewTarget(object view_target)
{
	ClearScheduleCall(this, this.UpdateViewTarget);
	if (view_target)
	{
		UpdateViewTarget(view_target);
		ScheduleCall(this, this.UpdateViewTarget, 30, 999999999, view_target);
	}
	return true;
}

private func UpdateViewTarget(object view_target)
{
	// Force view of all players on target
	if (!view_target) return;
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		SetPlrView(plr, view_target);
	}
}



/* Status */

// No scenario saving
func SaveScenarioObject(props) { return false; }


/* Message function forwards */

public func MessageBoxAll(string message, object talker, bool as_message, ...)
{
	return Dialogue->MessageBoxAll(message, talker, as_message, ...);
}

private func MessageBox(string message, object clonk, object talker, int for_player, bool as_message, ...)
{
	return Dialogue->MessageBox(message, clonk, talker, for_player, as_message, ...);
}




/* Global helper functions */

global func StartSequence(string name, int progress, ...)
{
	var seq = CreateObject(Sequence, 0,0, NO_OWNER);
	seq->Start(name, progress, ...);
	return seq;
}

global func StopSequence()
{
	var seq = FindObject(Find_ID(Sequence));
	if (!seq) return false;
	return seq->Stop();
}
