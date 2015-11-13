/*-- Save the windmills --*/

#include Library_Goal

static const SBRD_Rockets = 1;
local score, boss, wave;

func Initialize()
{
	score = [];
	boss = false;
	wave = 1;
	// init scoreboard
	Scoreboard->Init(
		[{key = "windmills", title = Goal_SaveTheWindmills, sorted = true, desc = true, default = 0, priority = 80}]
		);
	Scoreboard->SetTitle("$ScoreCaption$");
	// Remove settlement eval data.
	HideSettlementScoreInEvaluation(true);
	inherited(...);
}

public func IsFulfilled()
{
	if(!FindObject(Find_ID(WindGenerator)))
	{
		for(var i = 0; i<GetPlayerCount(); ++i)
		{
			EliminatePlayer(GetPlayerByIndex(i));
			return false;
		}
	}
	else if(boss)
	{
		if(!FindObject(Find_ID(BigBoomattack)))
			return true;
	}
	return false;
}

public func GetDescription(int plr)
{
	return this.Description;
}

public func Activate(int byplr)
{
	MessageWindow(this.Description, byplr);
	return;
}

public func IncShotScore(int plr)
{
	var plrid = GetPlayerID(plr);
	score[plrid]++;
	if (plr != NO_OWNER)
	{
		Scoreboard->SetPlayerData(plr, "windmills", score[plrid]);
	}
	NotifyHUD();
}

public func SetWave(int num)
{
	wave = num;
}

public func BossAttacks()
{
	boss = true;
}

public func GetShortDescription(int plr)
{
	//var allscore = 0;
	//for(var i=0; i<GetLength(score); ++i)
	//	allscore += score[i];
	//return Format("$ShotScore$",allscore);
	if (wave < 13)
		return Format("$WaveCount$", wave);
	else if (wave == 13)
		return "$WaveBoss$";
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	score[plrid] = 0;
	// Create scoreboard player entry for this player
	Scoreboard->NewPlayerEntry(plr);
	return _inherited(plr, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	AddEvaluationData(Format("$MsgEval$", score[plrid]), plrid);
	return _inherited(plr, ...);
}

public func OnGameOver()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plrid = GetPlayerID(GetPlayerByIndex(i));
		AddEvaluationData(Format("$MsgEval$", score[plrid]), plrid);
	}
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
