/*-- Race --*/

/*
	The goal is to be the first to reach the finish, the team or player to do so wins the round.
	Checkpoints can be added to make the path more interesting and complex.
	Checkpoints can have different functionalities:
		* Respawn: On/Off - The clonk respawns at the last passed checkpoint.
		* Check: On/Off - The clonk must pass through these checkpoints before being able to finish.
		* Ordered: The checkpoints mussed be passed in the order specified.
		* The start and finish are also checkpoints.
	ToDo: Scoreboard, Teams, Color update CP.
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
	return _inherited(...);
}

/*-- Checkpoint creation --*/

public func SetStartpoint(int iX, int iY)
{
	var cp = CreateObject(CKPT, iX, iY, NO_OWNER);
	cp->SetPosition(iX, iY);
	cp->SetCPMode(RACE_CP_Start);
	cp->SetCPController(this);
	aCheckpoints[0] = cp;
	return;
}

public func SetFinishpoint(int iX, int iY)
{
	var cp = CreateObject(CKPT, iX, iY, NO_OWNER);
	cp->SetPosition(iX, iY);
	cp->SetCPMode(RACE_CP_Finish);
	cp->SetCPController(this);
	aCheckpoints[nrCheckpoints + 1] = cp;
	return;
}

public func AddCheckpoint(int iX, int iY, int iMode)
{
	var cp = CreateObject(CKPT, iX, iY, NO_OWNER);
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
public func SetPlrRespawnCP(int iPlr, object pCP)
{
	aRespawnCP[iPlr] = pCP;
	return;
}

// Called from a check CP to indicate that iPlr has reached it.
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


	}
	else
	{
		szMessage = Format(Translate("MsgRace"), nrCheckpoints + 1);
	}
	MessageWindow(szMessage, iPlr);
	return;
}

private func GetPlrPos(int iPlr)
{
	var cp;


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
	return;
}

protected func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(CLNK, 0, 0, iPlr);
	clonk->MakeCrewMember(iPlr);
	SetCursor(iPlr,clonk);
	SelectCrew(iPlr, clonk, true);
	Log(RndRelaunchMsg(), GetPlayerName(iPlr));
	JoinPlayer(iPlr);
	return;
}

private func RndRelaunchMsg()
{
  return Translate(Format("MsgRelaunch%d", Random(4)));
}

protected func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	var x, y;
	FindRespawnPos(iPlr, x, y);
	clonk->SetPosition(x, y);
	// Give scenario defined objects.
	if (GameCall("RACE_GiveContents"))
		for(var idObj in GameCall("RACE_GiveContents"))
			clonk->CreateContents(idObj);
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



}

/*-- Scoreboard --*/

static const SBRD_Checkpoints = 0;
static const SBRD_Distance = 1;

private func UpdateScoreboard(int iPlr)
{
	//if (GetPlayerTeam(iPlr))
	var szCaption = Format("Race over %d checkpoints", nrCheckpoints + 1);
	SetScoreboardData(SBRD_Caption,	SBRD_Caption,		szCaption, 					SBRD_Caption);
	SetScoreboardData(SBRD_Caption,	SBRD_Checkpoints,	Format("{{%i}}", CKPT), 	SBRD_Caption);
	//SetScoreboardData(SBRD_Caption,	SBRD_Distance,		"D", 						SBRD_Caption);
	SetScoreboardData(iPlr,			SBRD_Caption,		GetTaggedPlayerName(iPlr),	SBRD_Caption);
	SetScoreboardData(iPlr,			SBRD_Checkpoints,	Format("%d", aPlrCP[iPlr]),	aPlrCP[iPlr]);
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


func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
