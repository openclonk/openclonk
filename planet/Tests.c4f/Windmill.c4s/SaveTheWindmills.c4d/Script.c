/*-- Save the windmills --*/

#include Library_Goal

static const SBRD_Rockets = 1;
local score, boss, wave;

func Initialize()
{
	score = [];
	boss = false;
	wave = 1;
	// Set scoreboard caption.
	SetScoreboardData(SBRD_Caption, SBRD_Caption, "$ScoreCaption$", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Rockets, "{{Goal_SaveTheWindmills}}", SBRD_Caption);
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

public func Activate(int byplr)
{
	MessageWindow(GetDesc(), byplr);
	return;
}

public func IncShotScore(int plr)
{
	var plrid = GetPlayerID(plr);
	score[plrid]++;
	if (plr != NO_OWNER)
	{
		SetScoreboardData(plrid, SBRD_Rockets, Format("%d", score[plrid]), score[plrid]);
		SortScoreboard(SBRD_Rockets, true);
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
	// Create scoreboard player entry for this player.
	SetScoreboardData(plrid, SBRD_Caption, GetTaggedPlayerName(plr), SBRD_Caption);
	SetScoreboardData(plrid, SBRD_Rockets, Format("%d", score[plrid]), score[plrid]);
	return _inherited(plr, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	AddEvaluationData(Format("$MsgEval$", score[plrid]), plrid);
	return _inherited(plr, ...);
}

public func DoEvaluationData() 
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plrid = GetPlayerID(GetPlayerByIndex(i));
		AddEvaluationData(Format("$MsgEval$", score[plrid]), plrid);
	}
	return true;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}