/**
	Defense Goal
	The players cooperate to defend a base against an enemy for a many attack waves as possible.
	
	Include the Teams.txt in this object in your defense scenario. Also include a ParameterDefs.txt
	if you want to have achievements. See Defense.ocf for an example of the file to add. Relaunching
	must be handled by script, for example the relaunch rule, and RelaunchPlayer must not be called,
	i.e. use OnClonkDeath. The goal can be further controlled by scenario script using the following
	callbacks:
	 * GetAttackWave(int wave_nr): to specify the enemies of this wave.
	 * GetWaveToAchievement(): to specify the amount of waves needed for bronze, silver and gold stars.
	 * OnWaveStarted(int wave_nr): when a wave has started.
	 * OnWaveCompleted(int wave_nr): when a wave has been completed (i.e. all enemies are eliminated).
	
	@author Maikel
*/


#include Library_Goal


local score;
local completed_waves;
local is_fulfilled;
local recovery_time;
local fx_wave_control;
local shared_wealth_remainder;
local observer_container;
local plrs_active;
local plrs_bonus;
local plrs_kills;


public func Construction()
{
	// Set initial score to zero.
	SetScore(0);
	completed_waves = 0;
	is_fulfilled = false;
	// Set default recovery time in seconds.
	recovery_time = 10;
	// Wealth is shared and remainder is stored.
	shared_wealth_remainder = 0;
	// Create a container for observers while the round still runs.
	observer_container = CreateObject(RelaunchContainer, AbsX(LandscapeWidth() / 2), AbsY(LandscapeHeight() / 2));
	// Init player tracking.
	plrs_active = [];
	plrs_bonus = [];
	plrs_kills = [];
	// Initialize scoreboard.
	Scoreboard->SetTitle(Format("$MsgScoreboard$", GetScore()));
	Scoreboard->Init([
		{key = "bonus", title = Icon_Wealth, sorted = true, desc = true, default = "0", priority = 100},
		{key = "kills", title = Scoreboard_Kill, sorted = true, desc = true, default = "0", priority = 50}
	]);
	// Create the enemy script player, the script player should be in the attackers team (id = 2).
	CreateScriptPlayer("$PlayerAttackers$", nil, 2, CSPF_NoEliminationCheck | CSPF_NoScenarioInit | CSPF_NoScenarioSave, GetID());
	return _inherited(...);
}


/*-- Player Control --*/

public func InitializePlayer(int plr)
{
	// The enemy script player is initialized in the function below.
	if (GetPlayerType(plr) == C4PT_Script)
		return;
	// Init the normal players
	var plrid = GetPlayerID(plr);
	// Store active players.
	PushBack(plrs_active, GetPlayerID(plr));
	// Initialize scoreboard.
	Scoreboard->NewPlayerEntry(plr);
	plrs_bonus[plrid] = 0;
	plrs_kills[plrid] = 0;
	Scoreboard->SetPlayerData(plr, "bonus", plrs_bonus[plrid]);
	Scoreboard->SetPlayerData(plr, "kills", plrs_kills[plrid]);
	return;
}

public func InitializeScriptPlayer(int plr)
{
	// Forward to defense goal object.
	if (this == Goal_Defense)
	{
		var goal = FindObject(Find_ID(Goal_Defense));
		if (goal)
			goal->InitializeScriptPlayer(plr);
		return;
	}
	// Add an effect to control the waves and set enemy player.
	fx_wave_control = CreateEffect(FxWaveControl, 100, nil, plr);
	return;
}

public func RelaunchPlayer(int plr)
{
	// The player has not been relaunched by external scripts and is eliminated.
	// Move the player into the observer container and let the player spectate.
	if (GetPlayerType(plr) == C4PT_User)
	{
		var crew = CreateObject(Clonk, 0, 0, plr);
		crew->MakeCrewMember(plr);
		SetCursor(plr, crew);
		crew->Enter(observer_container);
		RemoveArrayValue(plrs_active, GetPlayerID(plr));
		if (GetLength(plrs_active) == 0)
			return EndRound();
		// Update the view of the observing players.
		UpdateOberserverContainer();
	}
	return;
}

public func RemovePlayer(int plr)
{
	// Check completion if a player is removed, the player could have been the last active one.
	if (GetPlayerType(plr) == C4PT_User)
		if (RemoveArrayValue(plrs_active, GetPlayerID(plr)))
			if (GetLength(plrs_active) == 0)
				return EndRound();
	return;
}

public func EndRound()
{
	// Set evaluation data.
	SetRoundEvaluationData();
	// Remove wave control and wave tracking effects.
	if (fx_wave_control)
		fx_wave_control->Remove();
	var fx, index = 0;
	while (fx = GetEffect("FxTrackWave", this, index++))
		fx->Remove();
	// Eliminate attacker.
	for (var plr in GetPlayers(C4PT_Script))
		EliminatePlayer(plr);	
	is_fulfilled = true;
	return;
}

public func UpdateOberserverContainer()
{
	// Get a possible view cursor.
	var view_cursor;
	if (GetLength(plrs_active) > 0)
	{
		var active_plr = GetPlayerByID(plrs_active[0]);
		view_cursor = GetCursor(active_plr);
	}
	if (view_cursor)
	{
		view_cursor->CreateEffect(FxTrackObservation, 100, 0, this);
		// Set view for existing observers.
		for (var observer in FindObjects(Find_OCF(OCF_CrewMember), Find_Container(observer_container)))
		{
			var observer_plr = observer->GetOwner();
			SetPlrView(observer_plr, view_cursor);
		}
	}
	return;
}

local FxTrackObservation = new Effect
{
	Construction = func(object goal)
	{
		this.goal = goal;	
	},	
	Destruction = func(int reason)
	{
		if (reason == FX_Call_RemoveClear || reason == FX_Call_RemoveDeath)
			this.goal->UpdateOberserverContainer();
	}
};

public func IsFulfilled() { return is_fulfilled; }

// Can be called by a scenario if some other survival condition has failed.
// For example if some object that needed to be defended has been destroyed.
public func NotifyFailedSurvival()
{
	// Relaunch all players so that the goal is correctly fulfilled.
	for (var plr in GetPlayers(C4PT_User))
		RelaunchPlayer(plr);	
	return;
}


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
	// Update the best score for all players.
	for (var plr in GetPlayers(C4PT_User))
		SetBestScore(plr, GetScore());
	return;
}

private func SetBestScore(int plr, int new_score)
{
	var plrid = GetPlayerID(plr);
	// Only set if it increases the player's best score.
	if (new_score > GetBestScore(plr))
		SetPlrExtraData(plr, GetScoreString(), new_score);
	// Also set league score if an improvement is made.
	//if (new_score > GetLeaguePerformance(plrid)) TODO uncomment once available.
	SetLeaguePerformance(new_score, plrid);
	return;
}

private func GetBestScore(int plr)
{
	return GetPlrExtraData(plr, GetScoreString());
}

private func GetScoreString() { return Format("Defense_%s_BestScore", GetScenTitle()); }

private func UpdateBestWave(int new_best_wave)
{
	// Update the best wave for all players.
	for (var plr in GetPlayers(C4PT_User))
		SetBestWave(plr, new_best_wave);
	return;
}

private func SetBestWave(int plr, int new_best_wave)
{
	// Only set if it increases the player's best wave.
	if (new_best_wave > GetBestWave(plr))
		SetPlrExtraData(plr, GetWaveString(), new_best_wave);
	return;
}

private func GetBestWave(int plr)
{
	return GetPlrExtraData(plr, GetWaveString());
}

private func GetWaveString() { return Format("Defense_%s_BestWave", GetScenTitle()); }


/*-- Wave Control --*/

local FxWaveControl = new Effect
{
	Construction = func(int enemy_plr)
	{
		// Check if enemy player is correct.
		if (enemy_plr == nil || enemy_plr == NO_OWNER)
			return FX_Execute_Kill;		
		// Do a timer call every second.
		this.Interval = 36;
		// Init wave number, time passed and the current wave.
		this.wave_nr = 0;
		this.time_passed = 0;
		this.pause_time = 0;
		this.wave = nil;
		// Store enemy player.
		this.enemy = enemy_plr;	
		// Launch first wave.
		this->Timer(0);
	},
	
	Timer = func(int time)
	{
		// Start new wave if duration of previous wave has passed.
		if ((!this.wave && this.pause_time <= 0) || (this.wave && this.wave.Duration != nil && this.time_passed >= this.wave.Duration))
		{
			this.wave_nr++;
			this.wave = GameCall("GetAttackWave", this.wave_nr);
			// Check if this was the last wave.
			if (!this.wave)
			{
				Log("WARNING: wave not specified for wave number %d. Using default wave of defense goal instead.", this.wave_nr);
				this.wave = Target->GetDefaultWave(this.wave_nr);
			}
			DefenseEnemy->LaunchWave(this.wave, this.wave_nr, this.enemy);
			// Track the wave.
			Target->CreateEffect(Target.FxTrackWave, 100, nil, this.wave_nr, this.wave);
			// Reset passed time and increase wave number for next wave.
			this.time_passed = 0;
			// Do a game call for scenario specific scripts.
			GameCall("OnWaveStarted", this.wave_nr);
		}	
		// Increase the time passed in this wave and decrease the pause time.
		this.time_passed++;
		this.pause_time--;
		return FX_OK;
	},
	
	Destruction = func()
	{
		// Remove clock, since not needed any more.
		GUI_Clock->RemoveCountdown();
	},
	
	OnWaveCompleted = func(int wave_nr)
	{
		// Set current wave to nil if it has been completed and has no duration in order to start new one.
		if (wave_nr == this.wave_nr && this.wave.Duration == nil)
		{
			this.wave = nil;
			var rtime = Target->GetRecoveryTime();
			if (rtime > 0)
			{
				this.pause_time = rtime;
				GUI_Clock->CreateCountdown(rtime);
			}
		}
		// Do a game call for scenario specific scripts.
		GameCall("OnWaveCompleted", this.wave_nr);
		return;
	},
	
	GetCurrentWave = func()
	{
		return this.wave;
	},

	GetNextWave = func()
	{
		return GameCall("GetAttackWave", this.wave_nr + 1);
	},
	
	GetCurrentWaveNumber = func()
	{
		return this.wave_nr;
	},
	
	GetEnemy = func()
	{
		return this.enemy;
	}
};

public func GetDefaultWave(int wave_nr)
{
	// Just launch some rockets as the default wave and increase the amount and their speed with every wave.
	var wave = new DefenseEnemy.DefaultWave
	{
		Name = "$WaveFallBack$",
		Duration = 2 * 60,
		Bounty = 25,	
		Score = 25,
		Enemies = [new DefenseEnemy.BoomAttack {Amount = Max(10, 2 * wave_nr), Speed = 100 + wave_nr * 10}]
	};
	return wave;
}

public func SetRecoveryTime(int to_time)
{
	recovery_time = to_time;
	return;
}

public func GetRecoveryTime() { return recovery_time; }

public func GetEnemyPlayer(int plr)
{
	// Forward to defense goal object.
	if (this == Goal_Defense)
	{
		var goal = FindObject(Find_ID(Goal_Defense));
		if (goal)
			return goal->GetEnemyPlayer(plr);
		return;
	}
	if (fx_wave_control)
		return fx_wave_control->GetEnemy();
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
			if (this.wave.Score != nil && !Target->IsFulfilled())
				Target->DoScore(this.wave.Score);
			Target->OnWaveCompleted(this.wave_nr);
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

public func OnWaveCompleted(int wave_nr)
{
	// Increase number of completed waves and update achievements.
	completed_waves++;
	CheckAchievement();
	UpdateBestWave(completed_waves);
	// Let wave control effect know a wave has been completed.
	if (fx_wave_control)
		fx_wave_control->OnWaveCompleted(wave_nr);
	return;
}


/*-- Reward --*/

public func OnRocketDeath(object rocket, int killed_by)
{
	return OnClonkDeath(rocket, killed_by);
}

public func OnClonkDeath(object clonk, int killed_by)
{
	var plrid = GetPlayerID(killed_by);
	if (clonk.Bounty)
	{
		if (killed_by != NO_OWNER)
		{
			DoWealth(killed_by, clonk.Bounty);
			plrs_bonus[plrid] += clonk.Bounty;
			plrs_kills[plrid] += 1;
			Scoreboard->SetPlayerData(killed_by, "bonus", plrs_bonus[plrid]);
			Scoreboard->SetPlayerData(killed_by, "kills", plrs_kills[plrid]);			
		}
		else
		{		
			DoSharedWealth(clonk.Bounty);
		}
	}
	if (clonk.Score && !IsFulfilled())
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


/*-- Achievements --*/

public func CheckAchievement()
{
	var achievement = ConvertWaveToAchievement(completed_waves);
	// Give the players their achievement.
	if (achievement > 0)
		GainScenarioAchievement("Done", achievement);
	return;
}

public func GetWaveToAchievement()
{
	// Get the number of waves needed for achieving stars.
	var achievement_data = GameCall("GetWaveToAchievement");
	// By default the stars are awarded for 5, 10 and 25 completed waves.
	if (!achievement_data || GetLength(achievement_data) != 3)
		achievement_data = [5, 10, 25];
	SortArray(achievement_data);	
	return achievement_data;
}

public func ConvertWaveToAchievement(int wave_nr)
{
	var achievement_data = GetWaveToAchievement();
	for (var index = GetLength(achievement_data); index >= 1; index--)
		if (wave_nr >= achievement_data[index - 1])
			return index;
	return 0;
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
	var wave_msg = "";
	if (fx_wave_control)
	{
		// Add enemies of current wave.
		var current_wave = fx_wave_control->GetCurrentWave();
		if (current_wave)
		{
			wave_msg = Format("$MsgCurrentWave$", fx_wave_control->GetCurrentWaveNumber());
			var enemies = current_wave.Enemies;
			if (enemies)
			{
				for (var enemy in enemies)
					if (enemy.Amount > 0)
						wave_msg = Format("%s%dx %s\n", wave_msg, enemy.Amount, enemy.Name);
			}
			else
			{
				wave_msg = Format("%s$MsgNoEnemies$\n", wave_msg);
			}
		}
		// Show enemies of next wave.
		var next_enemies = fx_wave_control->GetNextWave().Enemies;
		if (next_enemies)
		{
			wave_msg = Format("%s\n$MsgNextWave$", wave_msg);
			for (var enemy in next_enemies)
				if (enemy.Amount > 0)
					wave_msg = Format("%s%dx %s\n", wave_msg, enemy.Amount, enemy.Name);		
		}
	}
	// Add wave and score.
	wave_msg = Format("%s\n%s", wave_msg, Format("$MsgFinishedWave$", completed_waves, GetBestWave(plr)));
	wave_msg = Format("%s\n%s", wave_msg, Format("$MsgCurrentScore$", GetScore(), GetBestScore(plr)));
	// Get basic description and achievement information.
	var achievement_data = GetWaveToAchievement();
	var desc_msg = Format("$Description$", achievement_data[0], achievement_data[1], achievement_data[2]);
	return Format("%s\n\n%s", desc_msg, wave_msg);
}


/*-- Debugging Control --*/

public func TestWave(int nr)
{
	var wave = GameCall("GetAttackWave", nr);
	if (!wave || !fx_wave_control)
		return;
	DefenseEnemy->LaunchWave(wave, nr, fx_wave_control->GetEnemy());
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local EditorPlacementLimit = 1;
