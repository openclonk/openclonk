/*-- Save the windmills --*/

#include Library_Goal

local score, boss;

func Initialize()
{
	score = CreateArray();
	boss = false;
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
	score[GetPlayerID(plr)]++;
	NotifyHUD();
}

public func BossAttacks()
{
	boss = true;
}

public func GetShortDescription(int plr)
{
	var allscore = 0;
	for(var i=0; i<GetLength(score); ++i)
		allscore += score[i];
		
	return Format("$ShotScore$",allscore);
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}