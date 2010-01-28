/*-- Parkour --*/

/*
	The goal is to be the first to reach the finish, the team or player to do so wins the round.
	Checkpoints can be added to make the path more interesting and more complex.
	Checkpoints can have different functionalities:
		* Respawn: On/Off - The clonk respawns at the last passed checkpoint.
		* Check: On/Off - The clonk must pass through these checkpoints before being able to finish.
		* Ordered: On/Off - The checkpoints mussed be passed in the order specified.
		* The start and finish are also checkpoints.
	ToDo: Scoreboard, Teams, Color update CP, relative distance to next CP in scoreboard.
*/

#include GOAL

local fFinished; // Whether the goal has been reached by some player.
local aCheckpoints; // List of checkpoints.
local nrCheckpoints; // Nr. of checkpoints.
local aRespawnCP; // List of last reached respawn CP per player.
local aPlrCP; // Number of checkpoints the player completed.
local aTeamCP; // Number of checkpoints the team completed.

protected func Initialize()
{
	fFinished = false;
	aCheckpoints = [];
	nrCheckpoints = 0;
	aRespawnCP = [];
	aPlrCP = [];
	aTeamCP = [];
	// Activate restart rule, if there isn't any.
	if (!FindObject(Find_ID(Core_Rule_Restart)))
		CreateObject(Core_Rule_Restart, 0, 0, NO_OWNER);

	// Scoreboard distance display.
	AddEffect("IntSBDistance", this, 100, 35, this);
	return _inherited(...);
}

/*-- Checkpoint creation --*/

public func SetStartpoint(int iX, int iY)
{
	var cp = CreateObject(Core_Goal_Checkpoint, iX, iY, NO_OWNER);
	cp->SetPosition(iX, iY);
	cp->SetCPMode(RACE_CP_Start);
	cp->SetCPController(this);
	aCheckpoints[0] = cp;
	return;
}

public func SetFinishpoint(int iX, int iY)
{
	var cp = CreateObject(Core_Goal_Checkpoint, iX, iY, NO_OWNER);
	cp->SetPosition(iX, iY);
	cp->SetCPMode(RACE_CP_Finish);
	cp->SetCPController(this);
	aCheckpoints[nrCheckpoints + 1] = cp;
	return;
}

public func AddCheckpoint(int iX, int iY, int iMode)
{
	var cp = CreateObject(Core_Goal_Checkpoint, iX, iY, NO_OWNER);
	cp->SetPosition(iX, iY);
	cp->SetCPMode(iMode);
	cp->SetCPController(this);
	if (iMode & RACE_CP_Check || iMode & RACE_CP_Ordered)
	{
		nrCheckpoints++;
		aCheckpoints[nrCheckpoints + 1] = aCheckpoints[nrCheckpoints]; // Finish 1 place further.
		aCheckpoints[nrCheckpoints] = cp;
	}
	return;
}

/*-- Checkpoint interaction --*/

// Called from a finish CP to indicate that iPlr has reached it.
public func PlayerHasReachedFinish(int iPlr)
{
	aPlrCP[iPlr]++;
	if (GetPlayerTeam(iPlr))
		aTeamCP[GetPlayerTeam(iPlr)]++;
	UpdateScoreboard(iPlr);
	EliminatePlayers(iPlr);
	fFinished = true;
	return;
}

// Called from a respawn CP to indicate that iPlr has reached it.
public func SetPlrRespawnCP(int iPlr, object cp)
{
	if (aRespawnCP[iPlr] == cp)
		return;
	aRespawnCP[iPlr] = cp;
	PlrMessage(Translate("MsgNewRespawn"), iPlr);
	return;
}

// Called from a check CP to indicate that iPlr has cleared it.
public func AddPlrClearedCP(int iPlr)
{
	aPlrCP[iPlr]++;
	if (GetPlayerTeam(iPlr))
		aTeamCP[GetPlayerTeam(iPlr)]++;
	UpdateScoreboard(iPlr);
	return;
}

/*-- Goal checking --*/

// Eliminate all players apart from iWinner and his team.
private func EliminatePlayers(int iWinner)
{
	var iWinTeam = GetPlayerTeam(iWinner);
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var iPlr = GetPlayerByIndex(i);
		var iTeam = GetPlayerTeam(iPlr);
		if (iPlr == iWinner) // The winner self.
			continue;
		if (iTeam && iTeam == iWinTeam) // In the same team as the winner.
			continue;
		EliminatePlayer(iPlr);
	}
	return;
}

public func IsFulfilled()
{
	return fFinished;
}

public func Activate(int iPlr)
{
	var iTeam = GetPlayerTeam(iPlr);
	var szMessage = "";
	if (fFinished)
	{
		if (iTeam)
		{
			if (IsWinner(iPlr))
				szMessage = Format(Translate("MsgRaceWonTeam"));
			else
				szMessage = Format(Translate("MsgRaceLostTeam"));
		}
		else
		{
			if (IsWinner(iPlr))
				szMessage = Format(Translate("MsgRaceWon"));
			else
				szMessage = Format(Translate("MsgRaceLost"));
		}
	}
	else
		szMessage = Format(Translate("MsgRace"), nrCheckpoints + 1);
	// Show goal message.
	MessageWindow(szMessage, iPlr);
	return;
}

private func IsWinner(int iPlr)
{
	var iTeam = GetPlayerTeam(iPlr);
	var finish = FindObject(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Finish));
	if (!finish)
		return false;
	if (iTeam)
	{
		if (finish->ClearedByTeam(iTeam))
			return true;
	}
	else
	{
		if (finish->ClearedByPlr(iPlr))
			return true;
	}
	return false;
}


/*-- Player section --*/

protected func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	// If the race is already finished, then immediately eliminate player.
	if (fFinished)
		return EliminatePlayer(iPlr);
	// Remove all hostilities.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		SetHostility(iPlr, GetPlayerByIndex(i), 0, 1);
		SetHostility(GetPlayerByIndex(i), iPlr, 0, 1);
	}
	// Init Respawn CP to start CP.
	aRespawnCP[iPlr] = aCheckpoints[0];
	aPlrCP[iPlr] = 0;
	if (iTeam)
		if (!aPlrCP[iTeam])
			aPlrCP[iTeam] = 0;
	UpdateScoreboard(iPlr);
	DoScoreboardShow(1, iPlr + 1);
	JoinPlayer(iPlr);
	// Scenario script callback.
	GameCall("PlrHasRespawned", iPlr, aRespawnCP[iPlr]);
	return;
}

protected func OnClonkDeath(object clonk)
{
	var iPlr = clonk->GetOwner();
	var nclonk = CreateObject(CLNK, 0, 0, iPlr);
	nclonk->MakeCrewMember(iPlr);
	nclonk->GrabObjectInfo(clonk);
	SetCursor(iPlr, nclonk);
	SelectCrew(iPlr, nclonk, true);
	JoinPlayer(iPlr);
	// Scenario script callback.
	GameCall("PlrHasRespawned", iPlr, aRespawnCP[iPlr]);
	// Log message.
	Log(RndRespawnMsg(), GetPlayerName(iPlr));
}

/*
protected func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(CLNK, 0, 0, iPlr);
	clonk->MakeCrewMember(iPlr);
	SetCursor(iPlr, clonk);
	SelectCrew(iPlr, clonk, true);
	JoinPlayer(iPlr);
	// Scenario script callback.
	GameCall("PlrHasRespawned", iPlr, aRespawnCP[iPlr]);
	// Log message.
	Log(RndRespawnMsg(), GetPlayerName(iPlr));
	return;
}
*/

private func RndRespawnMsg()
{
  return Translate(Format("MsgRespawn%d", Random(4)));
}

protected func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	var x, y;
	FindRespawnPos(iPlr, x, y);
	clonk->SetPosition(x, y);
	AddEffect("IntDirNextCP", clonk, 100, 10, this);
	return;
}

private func FindRespawnPos(int iPlr, int &iX, int &iY)
{
	iX = aRespawnCP[iPlr]->GetX();
	iY = aRespawnCP[iPlr]->GetY();
	return;
}

protected func RemovePlayer(int iPlr)
{
	// TODO

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

private func UpdateScoreboard(int iPlr)
{
	// TODO
	var szCaption = Format("Race over %d checkpoints", nrCheckpoints + 1);
	SetScoreboardData(SBRD_Caption,	SBRD_Caption,		szCaption, 					SBRD_Caption);
	SetScoreboardData(SBRD_Caption,	SBRD_Checkpoints,	Format("{{%i}}", Core_Goal_Checkpoint), 	SBRD_Caption);
	SetScoreboardData(SBRD_Caption,	SBRD_Distance,		Format("->{{%i}}", Core_Goal_Checkpoint),	SBRD_Caption);
	SetScoreboardData(iPlr,			SBRD_Caption,		GetTaggedPlayerName(iPlr),	SBRD_Caption);
	SetScoreboardData(iPlr,			SBRD_Checkpoints,	Format("%d", aPlrCP[iPlr]),	aPlrCP[iPlr]);
	SetScoreboardData(iPlr,	SBRD_Distance,		Format("%d%", GetDistToCP(iPlr)), 	GetDistToCP(iPlr));
	return;
}

// Returns the number of players in a team
private func GetTeamPlrCount(int iTeam)
{
	var cnt = 0;
	for(var i = 0; i < GetPlayerCount(); i++) 
		if(GetPlayerTeam(GetPlayerByIndex(i)) == iTeam)
			cnt++;
	return cnt;
}

// Only works for ordered CPs
private func GetDistToCP(int iPlr)
{
	// Get last finished ordered CP.
	var lastcp;
	for (var cp in FindObjects(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Ordered), Find_Func("ClearedByPlr", iPlr)))
		if (!lastcp || lastcp->~GetCPNumber() < cp->~GetCPNumber())
			lastcp = cp;
	if (!lastcp)
		lastcp = FindObject(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Start));
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
	var dist = ObjectDistance(GetCrew(iPlr), nextcp); 
	dist = (100 * dist) / ObjectDistance(lastcp, nextcp); 
	dist = BoundBy(dist, 0, 100);
	return dist;
}

/*-- Direction indication --*/

protected func FxIntDirNextCPTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	var plr = pTarget->GetOwner();
	// Find nearest CP.
	var nextcp;
	for (var cp in FindObjects(Find_ID(Core_Goal_Checkpoint), Find_Func("FindCPMode", RACE_CP_Check | RACE_CP_Finish), Sort_Distance(pTarget->GetX()-GetX(), pTarget->GetY()-GetY())))
		if (!cp->ClearedByPlr(plr) && (cp->IsActiveForPlr(plr) || cp->IsActiveForPlr(GetPlayerTeam(plr))))
		{
			nextcp = cp;
			break;
		}	
	if (!nextcp)
		return;
	// Calculate angle.
	var angle = Angle(pTarget->GetX(), pTarget->GetY(), nextcp->GetX(), nextcp->GetY());
	// Effect.
	CreateParticle("DebugReticle", pTarget->GetX()+Sin(angle, 20)-GetX(), pTarget->GetY()-Cos(angle,20)-GetY(), 0, 0, 30, RGB(255,0,0));
	return FX_OK;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
