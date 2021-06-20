/**
	Parkour
	
	The goal is to be the first to reach the finish, the team or player to do so wins the round.
	Checkpoints can be added to make the path more interesting and more complex.
	Checkpoints can have different functionalities:
		* Respawn: On/Off - The clonk respawns at the last passed checkpoint.
		* Check: On/Off - The clonk must pass through these checkpoints before being able to finish.
		* Ordered: On/Off - The checkpoints mussed be passed in the order specified.
		* The start and finish are also checkpoints.
	
	@author Maikel
*/


#include Library_Goal

local finished; // Whether the goal has been reached by some player.
local cp_list; // List of checkpoints.
local cp_count; // Number of checkpoints.
local respawn_list; // List of last reached respawn CP per player.
local plr_list; // Number of checkpoints the player completed.
local team_list; // Number of checkpoints the team completed.
local time_store; // String for best time storage in player file.
local no_respawn_handling; // Set to true if this goal should not handle respawn.
local transfer_contents; // Set to true if contents should be transferred on respawn.


/*-- General --*/

protected func Initialize(...)
{
	finished = false;
	no_respawn_handling = false;
	transfer_contents = false;
	cp_list = [];
	cp_count = 0;
	respawn_list = [];
	plr_list = [];
	team_list = [];
	// Best time tracking.
	time_store = Format("Parkour_%s_BestTime", GetScenTitle());
	AddEffect("IntBestTime", this, 100, 1, this);
	// Add a message board command "/resetpb" to reset the pb for this round.
	AddMsgBoardCmd("resetpb", "Goal_Parkour->~ResetPersonalBest(%player%)");
	// Activate restart rule, if there isn't any. But check delayed because it may be created later.
	ScheduleCall(this, this.EnsureRestartRule, 1, 1);
	// Scoreboard.
	InitScoreboard();
	// Assign unassigned checkpoints
	for (var obj in FindObjects(Find_ID(ParkourCheckpoint)))
		if (!obj->GetCPController())
			obj->SetCPController(this);
	return _inherited(...);
}

private func EnsureRestartRule()
{
	var relaunch = GetRelaunchRule();
	relaunch->SetAllowPlayerRestart(true);
	relaunch->SetPerformRestart(false);
	return true;
}

protected func Destruction(...)
{
	// Unassign checkpoints (updates editor help message)
	for (var obj in FindObjects(Find_ID(ParkourCheckpoint)))
		if (obj->GetCPController() == this)
			obj->SetCPController(nil);
	return _inherited(...);
}


/*-- Checkpoint creation --*/

public func SetStartpoint(int x, int y)
{
	// Safety, x and y inside landscape bounds.
	x = BoundBy(x, 0, LandscapeWidth());
	y = BoundBy(y, 0, LandscapeHeight());
	var cp = FindObject(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Start));
	if (!cp)	
		cp = CreateObjectAbove(ParkourCheckpoint, x, y, NO_OWNER);
	cp->SetCPController(this);
	cp->SetPosition(x, y);
	cp->SetCPMode(PARKOUR_CP_Start);
	return cp;
}

public func SetFinishpoint(int x, int y, bool team)
{
	// Safety, x and y inside landscape bounds.
	x = BoundBy(x, 0, LandscapeWidth());
	y = BoundBy(y, 0, LandscapeHeight());
	var cp = FindObject(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Finish));
	if (!cp)	
		cp = CreateObjectAbove(ParkourCheckpoint, x, y, NO_OWNER);
	cp->SetCPController(this);
	cp->SetPosition(x, y);
	var mode = PARKOUR_CP_Finish;
	if (team)
		mode = mode | PARKOUR_CP_Team;
	cp->SetCPMode(mode);
	return cp;
}

public func AddCheckpoint(int x, int y, int mode)
{
	// Safety, x and y inside landscape bounds.
	x = BoundBy(x, 0, LandscapeWidth());
	y = BoundBy(y, 0, LandscapeHeight());
	var cp = CreateObjectAbove(ParkourCheckpoint, x, y, NO_OWNER);
	cp->SetCPController(this);
	cp->SetPosition(x, y);
	cp->SetCPMode(mode);
	return cp;
}

public func DisableRespawnHandling()
{
	// Call this to disable respawn handling by goal. This might be useful if
	// a) you don't want any respawns, or
	// b) the scenario already provides an alternate respawn handling.
	no_respawn_handling = true;
	return true;
}

public func TransferContentsOnRelaunch(bool on)
{
	transfer_contents = on;
	return;
}

public func SetIndexedCP(object cp, int index)
{
	// Called directly from checkpoints after index assignment, resorting, etc.
	// Update internal list
	cp_list[index] = cp;
	if (cp->GetCPMode() & PARKOUR_CP_Finish)
	{
		cp_count = index;
		SetLength(cp_list, cp_count + 1);
	}
	UpdateScoreboardTitle();
	return true;
}


/*-- Checkpoint interaction --*/

// Called from a finish CP to indicate that plr has reached it.
public func PlayerReachedFinishCP(int plr, object cp, bool is_first_clear)
{
	if (finished)
		return;
	var plrid = GetPlayerID(plr);
	var team = GetPlayerTeam(plr);
	plr_list[plrid]++;
	if (team)
		team_list[team]++;
	UpdateScoreboard(plr);
	DoBestTime(plr);
	SetEvalData(plr);
	EliminatePlayers(plr);
	finished = true;
	if (is_first_clear) UserAction->EvaluateAction(on_checkpoint_first_cleared, this, cp, plr);
	UserAction->EvaluateAction(on_checkpoint_cleared, this, cp, plr);
	return;
}

// Called from a respawn CP to indicate that plr has reached it.
public func SetPlayerRespawnCP(int plr, object cp)
{
	if (respawn_list[plr] == cp)
		return;
	respawn_list[plr] = cp;
	cp->PlayerMessage(plr, "$MsgNewRespawn$");
	return;
}

// Called from a check CP to indicate that plr has cleared it.
public func AddPlayerClearedCP(int plr, object cp, bool is_first_clear, bool is_team_auto_clear)
{
	if (finished)
		return;
	var plrid = GetPlayerID(plr);
	plr_list[plrid]++;
	UpdateScoreboard(plr);
	if (!is_team_auto_clear) // No callback if only auto-cleared for other team members after another player cleared it
	{
		if (is_first_clear) UserAction->EvaluateAction(on_checkpoint_first_cleared, this, cp, plr);
		UserAction->EvaluateAction(on_checkpoint_cleared, this, cp, plr);
	}
	return;
}

// Called from a check CP to indicate that plr has cleared it.
public func AddTeamClearedCP(int team, object cp)
{
	if (finished)
		return;
	if (team)
		team_list[team]++;
	return;
}

private func ResetAllClearedCP()
{
	plr_list = [];
	team_list = [];
	respawn_list = [];
	for (var cp in FindObjects(Find_ID(ParkourCheckpoint)))
		cp->ResetCleared();
	return true;
}


/*-- Goal interface --*/

// Eliminates all players apart from the winner and his team.
private func EliminatePlayers(int winner)
{
	var winteam = GetPlayerTeam(winner);
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		var team = GetPlayerTeam(plr);
		if (plr == winner) // The winner self.
			continue;
		if (team && team == winteam) // In the same team as the winner.
			continue;
		EliminatePlayer(plr);
	}
	return;
}

public func IsFulfilled()
{
	return finished;
}

public func GetDescription(int plr)
{
	var team = GetPlayerTeam(plr);
	var msg;
	if (finished)
	{
		if (team)
		{
			if (IsWinner(plr))
				msg = "$MsgParkourWonTeam$";
			else
				msg = "$MsgParkourLostTeam$";
		}
		else
		{
			if (IsWinner(plr))
				msg = "$MsgParkourWon$";
			else
				msg = "$MsgParkourLost$";
		}
	}
	else
		msg = Format("$MsgParkour$", cp_count);

	return msg;
}

public func Activate(int plr)
{
	var team = GetPlayerTeam(plr);
	var msg;
	if (finished)
	{
		if (team)
		{
			if (IsWinner(plr))
				msg = "$MsgParkourWonTeam$";
			else
				msg = "$MsgParkourLostTeam$";
		}
		else
		{
			if (IsWinner(plr))
				msg = "$MsgParkourWon$";
			else
				msg = "$MsgParkourLost$";
		}
	}
	else
		msg = Format("$MsgParkour$", cp_count);
	// Show goal message.
	MessageWindow(msg, plr);
	return;
}

public func GetShortDescription(int plr)
{
	var team = GetPlayerTeam(plr);
	var parkour_length = GetParkourLength();
	if (parkour_length == 0)
		return "";
	var length;
	if (team)
		length = GetTeamPosition(team);
	else
		length = GetPlayerPosition(plr);
	var percentage =  100 * length / parkour_length;
	var red = BoundBy(255 - percentage * 255 / 100, 0, 255);
	var green = BoundBy(percentage * 255 / 100, 0, 255);
	var color = RGB(red, green, 0);
	return Format("<c %x>$MsgShortDesc$</c>", color, percentage, color);
}

// Returns the length the player has completed.
private func GetPlayerPosition(int plr)
{
	var plrid = GetPlayerID(plr);
	var cleared = plr_list[plrid];
	var length = 0;
	// Add length of cleared checkpoints.
	for (var i = 0; i < cleared; i++)
		length += ObjectDistance(cp_list[i], cp_list[i + 1]);
	// Add length of current checkpoint.
	var add_length = 0;
	if (cleared < cp_count)
	{
		var path_length = ObjectDistance(cp_list[cleared], cp_list[cleared + 1]);
		add_length = Max(path_length - ObjectDistance(cp_list[cleared + 1], GetCursor(plr)), 0);
	}
	return length + add_length;
}

// Returns the length the team has completed.
private func GetTeamPosition(int team)
{
	var cleared = team_list[team];
	var length = 0;
	// Add length of cleared checkpoints.
	for (var i = 0; i < cleared; i++)
		length += ObjectDistance(cp_list[i], cp_list[i + 1]);
	// Add length of current checkpoint.
	var add_length = 0;
	if (cleared < cp_count)
	{
		for (var i = 0; i < GetPlayerCount(); i++)
		{
			var plr = GetPlayerByIndex(i);
			if (GetPlayerTeam(plr) == team)
			{
				var path_length = ObjectDistance(cp_list[cleared], cp_list[cleared + 1]);
				var test_length = Max(path_length - ObjectDistance(cp_list[cleared + 1], GetCursor(plr)), 0);
				if (test_length > add_length)
					add_length = test_length;
			}			
		}
	}
	return length + add_length;
}

// Returns the length of this parkour.
private func GetParkourLength()
{
	var length = 0;
	for (var i = 0; i < cp_count; i++)
		length += ObjectDistance(cp_list[i], cp_list[i + 1]);
	return length;
}	

// Returns the number of checkpoints cleared by the player.
public func GetPlayerClearedCheckpoints(int plr)
{
	var plrid = GetPlayerID(plr);
	return plr_list[plrid];
}

public func GetLeaderClearedCheckpoints()
{
	return Max(plr_list);
}

private func IsWinner(int plr)
{
	var team = GetPlayerTeam(plr);
	var finish = cp_list[cp_count];
	if (!finish)
		return false;
	if (team)
	{
		if (finish->ClearedByTeam(team))
			return true;
	}
	else
	{
		if (finish->ClearedByPlayer(plr))
			return true;
	}
	return false;
}

/*-- Player section --*/

protected func InitializePlayer(int plr, int x, int y, object base, int team)
{
	// If the parkour is already finished, then immediately eliminate player.
	if (finished)
		return EliminatePlayer(plr);
	// Remove all hostilities.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		SetHostility(plr, GetPlayerByIndex(i), false, true);
		SetHostility(GetPlayerByIndex(i), plr, false, true);
	}
	// Init Respawn CP to start CP.
	var plrid = GetPlayerID(plr);
	respawn_list[plr] = cp_list[0];
	plr_list[plrid] = 0;
	if (team)
		if (!team_list[team])
			team_list[team] = 0;
	// Scoreboard.
	Scoreboard->NewPlayerEntry(plr);
	UpdateScoreboard(plr);
	DoScoreboardShow(1, plr + 1);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlayerRespawn", plr, FindRespawnCP(plr));
	return;
}

protected func OnClonkDeath(object clonk, int killed_by)
{
	var plr = clonk->GetOwner();
	// Only respawn if required and if the player still exists.
	if (no_respawn_handling || !GetPlayerName(plr) || GetCrewCount(plr)) 
		return;
	var new_clonk = CreateObjectAbove(Clonk, 0, 0, plr);
	new_clonk->MakeCrewMember(plr);
	SetCursor(plr, new_clonk);
	JoinPlayer(plr);
	// Transfer contents if active.
	if (transfer_contents)
		GetRelaunchRule()->TransferInventory(clonk, new_clonk);
	// Scenario script callback.
	GameCall("OnPlayerRespawn", plr, FindRespawnCP(plr));
	// Log message.
	Log(RndRespawnMsg(), GetPlayerName(plr));
	// Respawn actions
	var cp = FindRespawnCP(plr);
	UserAction->EvaluateAction(on_respawn, this, clonk, plr);
	if (cp)
		cp->OnPlayerRespawn(new_clonk, plr);
	return;
}

private func RndRespawnMsg()
{
	return Translate(Format("MsgRespawn%d", Random(4)));
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(clonk.MaxEnergy / 1000);
	var pos = FindRespawnPos(plr);
	clonk->SetPosition(pos[0], pos[1]);
	AddEffect("IntDirNextCP", clonk, 100, 1, this);
	return;
}

// You always respawn at the last completed checkpoint you passed by.
// More complicated behavior should be set by the scenario. 
private func FindRespawnCP(int plr)
{
	var respawn_cp = respawn_list[plr];
	if (!respawn_cp)
		respawn_cp = respawn_list[plr] = cp_list[0];
	return respawn_cp;
}

private func FindRespawnPos(int plr)
{
	var cp = FindRespawnCP(plr);
	if (!cp) cp = this; // Have to start somewhere
	return [cp->GetX(), cp->GetY()];
}

protected func RemovePlayer(int plr)
{
	respawn_list[plr] = nil;
	if (!finished)
		AddEvalData(plr);
	return;
}


/*-- Scenario saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	props->AddCall("Goal", this, "EnsureRestartRule");
	if (no_respawn_handling)
		props->AddCall("Goal", this, "DisableRespawnHandling");
	if (transfer_contents)
		props->AddCall("Goal", this, "TransferContentsOnRelaunch", true);
	return true;
}


/*-- Scoreboard --*/

static const SBRD_Checkpoints = 0;
static const SBRD_BestTime = 1;

private func UpdateScoreboardTitle()
{
	var caption;
	if (cp_count > 0)
		caption = Format("$MsgCaptionX$", cp_count);
	else
		caption = "$MsgCaptionNone$";
	return Scoreboard->SetTitle(caption);
}

private func InitScoreboard()
{
	Scoreboard->Init(
		[
		{key = "checkpoints", title = "#", sorted = true, desc = true, default = 0, priority = 80},
		{key = "besttime", title = GUI_Clock, sorted = true, desc = true, default = 0, priority = 70}
		]
		);
	UpdateScoreboardTitle();
	return;
}

private func UpdateScoreboard(int plr)
{
	if (finished)
		return;
	var plrid = GetPlayerID(plr);
	Scoreboard->SetPlayerData(plr, "checkpoints", plr_list[plrid]);
	var bt = GetPlrExtraData(plr, time_store);
	Scoreboard->SetPlayerData(plr, "besttime", TimeToString(bt), bt);
	return;
}


/*-- Direction indication --*/

// Effect for direction indication for the clonk.
protected func FxIntDirNextCPStart(object target, effect fx)
{
	var arrow = CreateObjectAbove(GUI_GoalArrow, 0, 0, target->GetOwner());
	arrow->SetAction("Show", target);
	fx.arrow = arrow;
	return FX_OK;
}

protected func FxIntDirNextCPTimer(object target, effect fx)
{
	var plr = target->GetOwner();
	var team = GetPlayerTeam(plr);
	var arrow = fx.arrow;
	// Find nearest CP.
	var nextcp;
	for (var cp in FindObjects(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Check | PARKOUR_CP_Finish), Sort_Distance(target->GetX() - GetX(), target->GetY() - GetY())))
		if (!cp->ClearedByPlayer(plr) && (cp->IsActiveForPlayer(plr) || cp->IsActiveForTeam(team)))
		{
			nextcp = cp;
			break;
		}
	if (!nextcp)
		return arrow->SetClrModulation(RGBa(0, 0, 0, 0));
	// Calculate parameters.
	var angle = Angle(target->GetX(), target->GetY(), nextcp->GetX(), nextcp->GetY());
	var dist = Min(510 * ObjectDistance(GetCrew(plr), nextcp) / 400, 510);
	var red = BoundBy(dist, 0, 255);
	var green = BoundBy(510 - dist, 0, 255);
	var blue = 0;
	// Arrow is colored a little different for the finish.
	if (nextcp->GetCPMode() & PARKOUR_CP_Finish)
		blue = 128;
	var color = RGBa(red, green, blue, 128);
	// Draw arrow.
	arrow->SetR(angle);
	arrow->SetClrModulation(color);
	// Check if clonk is contained in a vehicle, if so attach arrow to vehicle.
	var container = target->Contained();
	if (container && container->GetCategory() & C4D_Vehicle)
	{
		if (arrow->GetActionTarget() != container)
		{
			arrow->SetActionTargets(container);
			arrow->SetCategory(C4D_Vehicle);
		}
	} 
	else
	{
		if (arrow->GetActionTarget() != target)
		{
			arrow->SetActionTargets(target);
			arrow->SetCategory(C4D_StaticBack);
		}
	}
	return FX_OK;
}

protected func FxIntDirNextCPStop(object target, effect fx)
{
	fx.arrow->RemoveObject();
	return;
}


/*-- Time tracker --*/

// Store the best time in the player file, same for teammembers.
private func DoBestTime(int plr)
{
	var effect = GetEffect("IntBestTime", this);
	var time = effect.besttime;
	var winteam = GetPlayerTeam(plr);
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var check_plr = GetPlayerByIndex(i);
		if (winteam == 0 && check_plr != plr)
			continue;
		if (winteam != GetPlayerTeam(check_plr))
			continue;
		// Store best time for all players in the winning team.
		var rectime = GetPlrExtraData(check_plr, time_store);
		if (time != 0 && (!rectime || time < rectime))
		{
			SetPlrExtraData(check_plr, time_store, time);
			Log(Format("$MsgBestTime$", GetPlayerName(check_plr), TimeToString(time)));
		}
	}
	return;
}

// Starts at goal initialization, should be equivalent to gamestart.
protected func FxIntBestTimeTimer(object target, effect, time)
{
	effect.besttime = time;
	return FX_OK;
}

// Returns a best time string.
private func TimeToString(int time)
{
	if (!time) // No time.
		return "N/A";
	if (time > 36 * 60 * 60) // Longer than an hour.
		return Format("%d:%.2d:%.2d.%.1d", (time / 60 / 60 / 36) % 24, (time / 60 / 36) % 60, (time / 36) % 60, (10 * time / 36) % 10);
	if (time > 36 * 60) // Longer than a minute.
		return Format("%d:%.2d.%.1d", (time / 60 / 36) % 60, (time / 36) % 60, (10 * time / 36) % 10);
	else // Only seconds.
		return Format("%d.%.1d", (time / 36) % 60, (10 * time / 36) % 10);
}

// Resets the personal best (call from message board).
public func ResetPersonalBest(int plr)
{
	if (!GetPlayerName(plr))
		return;
	// Forward call to actual goal.
	if (this == Goal_Parkour)
	{
		var goal = FindObject(Find_ID(Goal_Parkour));
		if (goal)
			goal->ResetPersonalBest(plr);
	}
	SetPlrExtraData(plr, time_store, nil);
	// Also update the scoreboard.
	UpdateScoreboard(plr);
	return;
}


/*-- Evaluation data --*/

private func SetEvalData(int winner)
{
	var winteam = GetPlayerTeam(winner);
	var effect = GetEffect("IntBestTime", this);
	var time = effect.besttime;
	var msg;
	// General data.
	if (winteam)
		msg = Format("$MsgEvalTeamWon$", GetTeamName(winteam), TimeToString(time));
	else
		msg = Format("$MsgEvalPlrWon$", GetPlayerName(winner), TimeToString(time));
	AddEvaluationData(msg, 0);
	// Individual data.
	for (var i = 0; i < GetPlayerCount(); i++)
		AddEvalData(GetPlayerByIndex(i));
	// Obviously get rid of settlement score.
	HideSettlementScoreInEvaluation(true);
	return;
}

private func AddEvalData(int plr)
{
	if (finished)
		return;
	var plrid = GetPlayerID(plr);
	var cps = plr_list[plrid];
	var msg;
	if (cps == cp_count)
		msg = "$MsgEvalPlayerAll$";
	else
		msg = Format("$MsgEvalPlayerX$", cps, cp_count);
	AddEvaluationData(msg, plrid);
	return;
}


/* Editor */

local on_checkpoint_cleared, on_checkpoint_first_cleared, on_respawn;

public func SetOnCheckpointCleared(v) { on_checkpoint_cleared = v; return true; }
public func SetOnCheckpointFirstCleared(v) { on_checkpoint_first_cleared = v; return true; }
public func SetOnRespawn(v) { on_respawn = v; return true; }

public func Definition(def)
{
	_inherited(def);
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.on_checkpoint_cleared = new UserAction.Prop { Name="$OnCleared$", EditorHelp="$OnClearedHelp$", Set="SetOnCheckpointCleared", Save="Checkpoint" };
	def.EditorProps.on_checkpoint_first_cleared = new UserAction.Prop { Name="$OnFirstCleared$", EditorHelp="$OnFirstClearedHelp$", Set="SetOnCheckpointFirstCleared", Save="Checkpoint" };
	def.EditorProps.on_respawn = new UserAction.Prop { Name="$OnRespawn$", EditorHelp="$OnRespawnHelp$", Set="SetOnRespawn", Save = "Checkpoint" };
	if (!def.EditorActions) def.EditorActions = {};
	def.EditorActions.reset_all_cleared = { Name="$ResetAllCleared$", EditorHelp="$ResetAllClearedHelp$", Command="ResetAllClearedCP()" };
}


/*-- Proplist --*/

local Name = "$Name$";
