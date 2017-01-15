/**
	Defense Goal
	The players cooperate to defend a base against an enemy for a many attack waves as possible.
	
	@author Maikel
*/


#include Library_Goal


local score;
local completed_waves;
local is_fulfilled;
local fx_wave_control;
local shared_wealth_remainder;
local observer_container;
local plrs_active;
local plrs_bonus;
local plrs_score;


public func Construction()
{
	// Set initial score to zero.
	SetScore(0);
	completed_waves = 0;
	is_fulfilled = false;
	// Wealth is shared and remainder is stored.
	shared_wealth_remainder = 0;
	// Create a container for observers while the round still runs.
	observer_container = CreateObject(RelaunchContainer, AbsX(LandscapeWidth() / 2), AbsY(LandscapeHeight() / 2));
	// Init player tracking.
	plrs_active = [];
	plrs_bonus = [];
	plrs_score = [];
	// Add an effect to control the waves.
	fx_wave_control = CreateEffect(FxWaveControl, 100, nil);
	// Initialize scoreboard.
	Scoreboard->SetTitle(Format("$MsgScoreboard$", GetScore()));
	Scoreboard->Init([
		{key = "bonus", title = Icon_Wealth, sorted = true, desc = true, default = "0", priority = 100}
	]);
	return _inherited(...);
}


/*-- Player Control --*/

public func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	if (GetPlayerType(plr) == C4PT_User)
	{
		// Store active players.
		PushBack(plrs_active, GetPlayerID(plr));
		// Initialize scoreboard.
		Scoreboard->NewPlayerEntry(plr);
		plrs_bonus[plrid] = 0;
		Scoreboard->SetPlayerData(plr, "bonus", plrs_bonus[plrid]);
	}
	return;
}

public func RelaunchPlayer(int plr)
{
	if (GetPlayerType(plr) == C4PT_User)
	{
		var crew = CreateObject(Clonk);
		crew->MakeCrewMember(plr);
		SetCursor(plr, crew);
		crew->Enter(observer_container);
		RemoveArrayValue(plrs_active, GetPlayerID(plr));
		if (GetLength(plrs_active) == 0)
			EndRound();
	}
	return;
}

public func EndRound()
{
	for (var plr in GetPlayers(C4PT_Script))
		EliminatePlayer(plr);
	// Set evaluation data.
	SetRoundEvaluationData();
	// Set league score.
	SetLeaguePerformance(GetScore());
	// Remove wave control effect.
	if (fx_wave_control)
		fx_wave_control->Remove();
	is_fulfilled = true;
	return;
}

public func IsFulfilled() { return is_fulfilled; }


/*-- Score --*/

public func DoScore(int value)
{
	return SetScore(score + value);
}

public func GetScore() { return score; }

public func SetScore(int value)
{
	score = value;
	// Update scoreboard title.
	Scoreboard->SetTitle(Format("$MsgScoreboard$", GetScore()));
	return;
}


/*-- Wave Control --*/

local FxWaveControl = new Effect
{
	Construction = func()
	{
		// Do a timer call every second.
		this.Interval = 36;
		// Init wave number, time passed and the current wave.
		this.wave_nr = 1;
		this.time_passed = 0;
		this.wave = nil;				
		// Launch first wave.
		this->Timer(0);
	},
	
	Timer = func(int time)
	{
		// Start new wave.
		if (!this.wave || this.time_passed >= this.wave.Duration)
		{
			this.wave = GameCall("GetAttackWave", this.wave_nr);
			// Check if this was the last wave.
			if (!this.wave)
			{
				Log("WARNING: wave not specified for wave number %d. Using default wave of defense goal instead.", this.wave_nr);
				this.wave = Target->GetDefaultWave(this.wave_nr);
			}
			// Launch the wave.
			if (this.enemy == nil)
				this.enemy = GameCall("GetEnemyPlayer");
			DefenseWave->LaunchWave(this.wave, this.wave_nr, this.enemy);
			// Track the wave.
			Target->CreateEffect(Target.FxTrackWave, 100, nil, this.wave_nr, this.wave);
			// Reset passed time and increase wave number for next wave.
			this.time_passed = 0;
			this.wave_nr++;
		}	
		// Increase the time passed in this wave.
		this.time_passed++;
		return FX_OK;
	},
	
	GetCurrentWave = func()
	{
		return this.wave;
	},
	
	SetEnemy = func(int plr)
	{
		this.enemy = plr;
	}
};

public func GetDefaultWave(int wave_nr)
{
	// Just launch some rockets as the default wave and increase the amount and their speed with every wave.
	var wave = new DefaultWave
	{
		Name = "$WaveFallBack$",
		Duration = 2 * 60,
		Bounty = 25,	
		Score = 25,
		Enemies = [new DefenseEnemy.BoomAttack {Amount = Max(10, 2 * wave_nr), Speed = 100 + wave_nr * 10}]
	};
	return wave;
}

public func SetEnemyPlayer(int plr)
{
	if (fx_wave_control)
		fx_wave_control->SetEnemy(plr);
	return;
}


/*-- Wave Tracking --*/

local FxTrackWave = new Effect
{
	Construction = func(int wave_nr, proplist wave)
	{
		this.wave_nr = wave_nr;
		this.wave = wave;
		this.enemies = [];	
	},
	OnEnemyInit = func(object enemy)
	{
		PushBack(this.enemies, enemy);	
	},
	OnEnemyEliminated = func(object enemy)
	{
		RemoveArrayValue(this.enemies, enemy);
		if (GetLength(this.enemies) == 0)
		{
			// Completed this wave: log, give bounty and remove effect.
			if (this.wave.Bounty != nil)
			{
				Target->DoWealthForAll(this.wave.Bounty);
				CustomMessage(Format("$MsgWaveCleared$", this.wave_nr, this.wave.Bounty));
			}
			if (this.wave.Score != nil)
				Target->DoScore(this.wave.Score);
			Target->IncreaseCompletedWaves();
			Sound("UI::NextWave");
			this->Remove();
		}
	}
};

public func OnEnemyCreation(object enemy, int wave_nr)
{
	var index = 0, fx;
	while (fx = GetEffect("FxTrackWave", this, index++))
		if (fx.wave_nr == wave_nr)
			fx->OnEnemyInit(enemy);
	return;
}

public func OnEnemyElimination(object enemy)
{
	var index = 0, fx;
	while (fx = GetEffect("FxTrackWave", this, index++))
		fx->OnEnemyEliminated(enemy);
	return;
}

public func IncreaseCompletedWaves()
{
	completed_waves++;
	return;
}


/*-- Reward --*/

public func OnClonkDeath(object clonk, int killed_by)
{
	var plrid = GetPlayerID(killed_by);
	if (clonk.Bounty)
	{
		if (killed_by != NO_OWNER)
		{
			DoWealth(killed_by, clonk.Bounty);
			plrs_bonus[plrid] += clonk.Bounty;
			Scoreboard->SetPlayerData(killed_by, "bonus", plrs_bonus[plrid]);
		}
		else
		{		
			DoSharedWealth(clonk.Bounty);
		}
	}
	if (clonk.Score)
		DoScore(clonk.Score);
	// Notify about elimination of enemy.
	OnEnemyElimination(clonk);
	return;
}

public func DoSharedWealth(int amount)
{
	// Split gold among all players. Keep track of remainder and use it next time.
	shared_wealth_remainder += amount;
	var cnt = GetPlayerCount(C4PT_User);
	if (cnt)
	{
		var wealth_add = shared_wealth_remainder / cnt;
		if (wealth_add)
		{
			shared_wealth_remainder -= wealth_add * cnt;
			DoWealthForAll(wealth_add);
		}
	}
	return true;
}

// Add wealth to all players.
public func DoWealthForAll(int amount)
{
	for (var plr in GetPlayers(C4PT_User))
		DoWealth(plr, amount);
	return;
}


/*-- Evaluation Data --*/

public func SetRoundEvaluationData()
{
	// Obviously get rid of settlement score.
	HideSettlementScoreInEvaluation(true);
	// Display the number of completed waves.
	AddEvaluationData(Format("$MsgEvaluationData$", completed_waves, GetScore()), 0);
	return;
}


/*-- Description --*/

public func GetDescription(int plr)
{
	var wave_msg = "$MsgCurrentWave$";
	// Add enemies of current wave.
	if (fx_wave_control)
	{
		var enemies = fx_wave_control->GetCurrentWave().Enemies;
		if (enemies)
			for (var enemy in enemies)
				wave_msg = Format("%s%dx %s\n", wave_msg, enemy.Amount, enemy.Name);
	}
	// Add score.
	wave_msg = Format("%s\n%s", wave_msg, Format("$MsgCurrentScore$", GetScore(), 0/*replace with plr high score*/));
	return Format("%s\n\n%s", "$Description$", wave_msg);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1;
