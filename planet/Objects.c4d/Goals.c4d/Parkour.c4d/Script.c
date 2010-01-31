/*-- Parkour --*/

/*
	The goal is to be the first to reach the finish, the team or player to do so wins the round.
	Checkpoints can be added to make the path more interesting and more complex.
	Checkpoints can have different functionalities:
		* Respawn: On/Off - The clonk respawns at the last passed checkpoint.
		* Check: On/Off - The clonk must pass through these checkpoints before being able to finish.
		* Ordered: On/Off - The checkpoints mussed be passed in the order specified.
		* The start and finish are also checkpoints.
	ToDo: 
		* Scoreboard with teams -> commented away, since bugs.
		* Update CP Graphics.
		* Relative distance to next CP in scoreboard -> done.
		* Arrow showing direction to next CP -> partially done.
		* maybe update array access to plrID -> done.
		* Add evaluation data.
*/

#include GOAL

local fFinished; // Whether the goal has been reached by some player.
local aCheckpoints; // List of checkpoints.
local nrCheckpoints; // Number of checkpoints.
local aRespawnCP; // List of last reached respawn CP per player.
local aPlrCP; // Number of checkpoints the player completed.
local aTeamCP; // Number of checkpoints the team completed.
local szBestTimeString; // String for best time storage in player file.

/*-- General --*/

protected func Initialize()
{
	fFinished = false;
	aCheckpoints = [];
	nrCheckpoints = 0;
	aRespawnCP = [];
	aPlrCP = [];
	aTeamCP = [];
	// Best time tracking.
	szBestTimeString = Format("PARK_%s_BestTime", GetScenTitle());
	AddEffect("IntBestTime", this, 100, 1, this);
	// Activate restart rule, if there isn't any.
	if (!FindObject(Find_ID(Core_Rule_Restart)))
		CreateObject(Core_Rule_Restart, 0, 0, NO_OWNER);

	// Scoreboard distance display.
	InitScoreboard();
	AddEffect("IntSBDistance", this, 100, 35, this);
	return _inherited(...);
}

/*-- Checkpoint creation --*/

public func SetStartpoint(int x, int y)
{
	var cp = CreateObject(Core_Goal_Checkpoint, x, y, NO_OWNER);
	cp->SetPosition(x, y);
	cp->SetCPMode(RACE_CP_Start);
	cp->SetCPController(this);
	aCheckpoints[0] = cp;
	return;
}

public func SetFinishpoint(int x, int y)
{
	var cp = CreateObject(Core_Goal_Checkpoint, x, y, NO_OWNER);
	cp->SetPosition(x, y);
	cp->SetCPMode(RACE_CP_Finish);
	cp->SetCPController(this);
	aCheckpoints[nrCheckpoints + 1] = cp;
	return;
}

public func AddCheckpoint(int x, int y, int mode)
{
	var cp = CreateObject(Core_Goal_Checkpoint, x, y, NO_OWNER);
	cp->SetPosition(x, y);
	cp->SetCPMode(mode);
	cp->SetCPController(this);
	if (mode & RACE_CP_Check || mode & RACE_CP_Ordered)
	{
		nrCheckpoints++;
		aCheckpoints[nrCheckpoints + 1] = aCheckpoints[nrCheckpoints]; // Finish 1 place further.
		aCheckpoints[nrCheckpoints] = cp;
	}
	return;
}

/*-- Checkpoint interaction --*/

// Called from a finish CP to indicate that plr has reached it.
public func PlrReachedFinishCP(int plr)
{
	var plrid = GetPlayerID(plr);
	aPlrCP[plrid]++;
	if (GetPlayerTeam(plr))
		aTeamCP[GetPlayerTeam(plr)]++;
	UpdateScoreboard(plr);
	DoBestTime(plr);
	SetEvalData(plr);
	EliminatePlayers(plr);
	fFinished = true;
	return;
}

// Called from a respawn CP to indicate that plr has reached it.
public func SetPlrRespawnCP(int plr, object cp)
{
	if (aRespawnCP[plr] == cp)
		return;
	aRespawnCP[plr] = cp;
	PlrMessage(Translate("MsgNewRespawn"), plr);
	return;
}

// Called from a check CP to indicate that plr has cleared it.
public func AddPlrClearedCP(int plr)
{
	var plrid = GetPlayerID(plr);
	var team = GetPlayerTeam(plr);
	aPlrCP[plrid]++;
	if (team)
		aTeamCP[team]++;
	UpdateScoreboard(plr);
	return;
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
	return fFinished;
}

public func Activate(int plr)
{
	var team = GetPlayerTeam(plr);
	var msg;
	if (fFinished)
	{
		if (team)
		{
			if (IsWinner(plr))
				msg = Format(Translate("MsgRaceWonTeam"));
			else
				msg = Format(Translate("MsgRaceLostTeam"));
		}
		else
		{
			if (IsWinner(plr))
				msg = Format(Translate("MsgRaceWon"));
			else
				msg = Format(Translate("MsgRaceLost"));
		}
	}
	else
		msg = Format(Translate("MsgRace"), nrCheckpoints + 1);
	// Show goal message.
	MessageWindow(msg, plr);
	return;
}

public func GetShortDescription(int plr)
{
	return Translate("MsgDesc");
}

private func IsWinner(int plr)
{
	var team = GetPlayerTeam(plr);
	var finish = FindObject(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Finish));
	if (!finish)
		return false;
	if (team)
	{
		if (finish->ClearedByTeam(team))
			return true;
	}
	else
	{
		if (finish->ClearedByPlr(plr))
			return true;
	}
	return false;
}

/*-- Player section --*/

protected func InitializePlayer(int plr, int x, int y, object base, int team)
{
	// If the race is already finished, then immediately eliminate player.
	if (fFinished)
		return EliminatePlayer(plr);
	// Remove all hostilities.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		SetHostility(plr, GetPlayerByIndex(i), false, true);
		SetHostility(GetPlayerByIndex(i), plr, false, true);
	}
	// Init Respawn CP to start CP.
	var plrid = GetPlayerID(plr);
	aRespawnCP[plr] = aCheckpoints[0];
	aPlrCP[plrid] = 0;
	if (team)
		if (!aTeamCP[team])
			aTeamCP[team] = 0;
	// Scoreboard.
	InitScoreboard();
	UpdateScoreboard(plr);
	DoScoreboardShow(1, plr + 1);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("PlrHasRespawned", plr, aRespawnCP[plr]);
	return;
}

protected func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(CLNK, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	SelectCrew(plr, clonk, true);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("PlrHasRespawned", plr, aRespawnCP[plr]);
	// Log message.
	Log(RndRespawnMsg(), GetPlayerName(plr));
	return;
}

private func RndRespawnMsg()
{
  return Translate(Format("MsgRespawn%d", Random(4)));
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x, y;
	FindRespawnPos(plr, x, y);
	clonk->SetPosition(x, y);
	AddEffect("IntDirNextCP", clonk, 100, 10, this);
	return;
}

private func FindRespawnPos(int plr, int &x, int &y)
{
	x = aRespawnCP[plr]->GetX();
	y = aRespawnCP[plr]->GetY();
	return;
}

protected func RemovePlayer(int plr)
{
	// TODO

	if (!fFinished)
		AddEvalData(plr);
	return;
}

/*-- Scoreboard --*/

protected func FxIntSBDistanceTimer()
{
	for (var i = 0; i < GetPlayerCount(); i++)
		UpdateScoreboard(GetPlayerByIndex(i));
	return FX_OK;
}

static const SBRD_Checkpoints = 0;
static const SBRD_Distance = 1;
static const SBRD_BestTime = 2;

private func InitScoreboard()
{
	if (nrCheckpoints > 0)
		var caption = Format(Translate("MsgCaptionX"), nrCheckpoints + 1);
	else
		var caption = Translate("MsgCaptionNone");
	// The above row.
	SetScoreboardData(SBRD_Caption, SBRD_Caption, caption, SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Checkpoints, Format("{{%i}}", Core_Goal_Checkpoint), SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Distance, Format("->{{%i}}", Core_Goal_Checkpoint), SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_BestTime, "T", SBRD_Caption);
	return;
}

private func UpdateScoreboard(int plr)
{
	var plrid = GetPlayerID(plr);
	// The player name.
	SetScoreboardData(plrid, SBRD_Caption, GetTaggedPlayerName(plr), SBRD_Caption);
	// The player scores.
	SetScoreboardData(plrid, SBRD_Checkpoints, Format("%d", aPlrCP[plrid]), aPlrCP[plrid]);
	SetScoreboardData(plrid, SBRD_Distance, Format("%d%", GetDistToCP(plr)), GetDistToCP(plr));
	SetScoreboardData(plrid, SBRD_BestTime, TimeToString(GetPlrExtraData(plr, szBestTimeString)), SBRD_Caption);
	// Sort.
	SortScoreboard(SBRD_Distance, false);
	SortScoreboard(SBRD_Checkpoints, true);
	return;
}

/*-- Direction & distance indication --*/

// Returns the last cleared CP for plr.
private func FindLastCP(int plr)
{
	var lastcp;
	for (var cp in FindObjects(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Ordered), Find_Func("ClearedByPlr", plr)))
		if (!lastcp || lastcp->~GetCPNumber() < cp->~GetCPNumber())
			lastcp = cp;
	if (!lastcp)
		lastcp = FindObject(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Start));
	return lastcp;
}

// Returns the next CP for plr.
private func FindNextCP(int plr)
{
	var nextcp;

	return;
}

// Only works for ordered CPs
private func GetDistToCP(int plr)
{
	// Get last finished ordered CP.
	var lastcp = FindLastCP(plr);
	if (!lastcp)
		return 0;
	// Get next ordered CP.
	var nextcp;
	for (var cp in FindObjects(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Ordered)))
		if (cp->GetCPNumber() == lastcp->GetCPNumber() + 1)
			nextcp = cp;
	if (!nextcp)
		nextcp = FindObject(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Finish));
	if (!nextcp)
		return 0;
	var dist = ObjectDistance(GetCrew(plr), nextcp); 
	dist = (100 * dist) / ObjectDistance(lastcp, nextcp); 
	dist = BoundBy(dist, 0, 100);
	return dist;
}

// Effect for direction indication for the clonk.
protected func FxIntDirNextCPTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	var plr = pTarget->GetOwner();
	// Find nearest CP.
	var nextcp;
	for (var cp in FindObjects(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Check | RACE_CP_Finish), Sort_Distance(pTarget->GetX()-GetX(), pTarget->GetY()-GetY())))
		if (!cp->ClearedByPlr(plr) && (cp->IsActiveForPlr(plr) || cp->IsActiveForTeam(GetPlayerTeam(plr))))
		{
			nextcp = cp;
			break;
		}	
	if (!nextcp)
		return;
	// Calculate angle.
	var angle = Angle(pTarget->GetX(), pTarget->GetY(), nextcp->GetX(), nextcp->GetY());
	// Effect, should be replaced with an arrow.
	CreateParticle("DebugReticle", pTarget->GetX()+Sin(angle, 20)-GetX(), pTarget->GetY()-Cos(angle,20)-GetY(), 0, 0, 30, RGB(255,0,0));
	return FX_OK;
}

/*-- Time tracker --*/

private func DoBestTime(int plr)
{
	var effect = GetEffect("IntBestTime", this);
	var time = EffectVar(0, this, effect);
	var rectime = GetPlrExtraData(plr, szBestTimeString);
	if (time != 0 && (!rectime || time < rectime))
	{
		SetPlrExtraData(plr, szBestTimeString, time);
		Log(Format(Translate("MsgBestTime"), GetPlayerName(plr), TimeToString(time)));
	}
	return;
}

// Starts at goal initialization, should be equivalent to gamestart.
protected func FxIntBestTimeTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	EffectVar(0, pTarget, iEffectNumber) = iEffectTime;
	return FX_OK;
}

// Returns a best time string.
private func TimeToString(int time)
{
	if (!time) // No time.
		return "N/A";
	if (time > 36*60*60) // Longer than an hour.
		return Format("%d:%.2d:%.2d.%.1d", (time/60/60/36)%24, (time/60/36)%60, (time/36)%60, (10*time/36)%10);
	if (time > 36*60) // Longer than a minute.
		return Format("%d:%.2d.%.1d", (time/60/36)%60, (time/36)%60, (10*time/36)%10);
	else // Only seconds.
		return Format("%d.%.1d", (time/36)%60, (10*time/36)%10);
}

/*-- Evaluation data --*/

private func SetEvalData(int winner)
{
	var winteam = GetPlayerTeam(winner);
	var effect = GetEffect("IntBestTime", this);
	var time = EffectVar(0, this, effect);
	var msg;
	// General data.
	if (winteam)
		msg = Format(Translate("MsgEvalTeamWon"), GetTeamName(winteam), TimeToString(time));
	else
		msg = Format(Translate("MsgEvalPlrWon"), GetPlayerName(winner), TimeToString(time));
	AddEvaluationData(msg, 0);
	// Individual data.
	for (var i = 0; i < GetPlayerCount(); i++)
		AddEvalData(GetPlayerByIndex(i));
	// Obviously get rid of settlement score.*/
	HideSettlementScoreInEvaluation(true); 
	return;
}

private func AddEvalData(int plr)
{
	var plrid = GetPlayerID(plr);
	var cps = aPlrCP[plrid];
	var msg;
	if (cps == nrCheckpoints)
		msg = Translate("MsgEvalPlayerAll");
	else
		msg = Format(Translate("MsgEvalPlayerX"), cps);
	AddEvaluationData(msg, plrid);
	return;
}

/*-- Proplist --*/

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}