/**
	EnemySpawn
	When activated, spawns one or more AI-controlled enemies that attack the players.
	
	@author Sven2
*/

local Name="$Name$";
local Description="$Description$";
local Visibility = VIS_Editor;

local SPAWNCOUNT_INFINITE = 0x7fffffff; // magic value for spawn_count: Infinite enemy count. (must be a big value)

local active; // Triggered or currently spawning
local waiting_for_script_player; // Set to true if rule was activated but still waiting for 
local enemy = nil; // No enemy defined
local spawn_position = nil; // Where / in which range to spawn
local spawn_count = 1; // Number of enemies to spawn. SPAWNCOUNT_INFINITE for infinite.
local spawn_delay = 0; // Delay after trigger before first spawn
local spawn_interval = 30; // Delay between spawned enemies
local attack_path = nil; // Optional: Array of points along which the spawned enemy moves/attacks
local auto_activate = false; // If true, the object is activated on the first player join
local max_concurrent_enemies = SPAWNCOUNT_INFINITE; // May be set to a smaller value to limit the amount of enemies spawned in parallel. Only works with spawn_delay>0.
local has_been_activated; // Set to true after first activation. Not saved in scenario.
local spawn_action; // UserAction excuted on each spawned enemy

local spawned_count;            // Number of enemies already spawned in current wave
local spawned_enemies;          // Array of spawned enemies. Automatically cleared when clonks die.
local num_enemies_defeated = 0; // Increased for each defeated clonk of this spawner
local num_waves_defeated = 0;   // Increased after each defeated activation

local EnemyDefs; // Proplist of possible enemy spawn definitions; indexed by Type

static g_enemyspawn_player; // player number of attacking player

public func IsEnemySpawn() { return true; }


/* Parameter interface */

public func SetEnemy(proplist new_enemy) { enemy = new_enemy; UpdateEnemyDisplay(); }
public func SetSpawnPosition(proplist new_spawn_position) { spawn_position = new_spawn_position; }
public func SetSpawnCount(int new_spawn_count) { spawn_count = new_spawn_count; }
public func SetSpawnDelay(int new_spawn_delay) { spawn_delay = new_spawn_delay; }
public func SetSpawnInterval(int new_spawn_interval) { spawn_interval = new_spawn_interval; }
public func SetAttackPath(array new_attack_path) { attack_path = new_attack_path; }
public func SetAutoActivate(bool new_auto_activate) { auto_activate = new_auto_activate; }
public func SetMaxConcurrentEnemies(int new_max_concurrent_enemies) { max_concurrent_enemies = new_max_concurrent_enemies; }


/* Initialization */

public func Initialize()
{
	// Default attack path
	var tx = 0, ty = 0;
	if (GetY() < 100 && LandscapeHeight() > 400)
	{
		ty = 100;
	}
	else
	{
		if (GetX() < LandscapeWidth()/2) tx = Min(100, LandscapeWidth()-GetX()-8); else tx = Max(-100, -GetX()+8);
	}
	attack_path = [{X = tx, Y = ty}];
	spawned_enemies = [];
	// Make sure there's an enemy script player
	if (!GetType(g_enemyspawn_player))
	{
		// Look for existing enemy player
		for (var iplr = 0; iplr < GetPlayerCount(C4PT_Script); ++iplr)
		{
			var plr = GetPlayerByIndex(iplr, C4PT_Script);
			if (GetScriptPlayerExtraID(plr) == GetID())
			{
				g_enemyspawn_player = plr;
				break;
			}
		}
		// Otherwise join it
		if (!GetType(g_enemyspawn_player))
		{
			g_enemyspawn_player = NO_OWNER; // Sentinel value: Script player join scheduled but not joined yet
			CreateScriptPlayer("$PlayerAttackers$", nil, 0, CSPF_NoEliminationCheck | CSPF_Invisible | CSPF_FixedAttributes | CSPF_NoScenarioInit | CSPF_NoScenarioSave, GetID());
		}
	}
}

private func IsEnemySpawnPlayerJoined()
{
	return GetType(g_enemyspawn_player) && (g_enemyspawn_player != NO_OWNER);
}


public func InitializeScriptPlayer(int plr, int team)
{
	// Init the enemy script player: Hostile to all players
	if (g_enemyspawn_player == NO_OWNER)
	{
		// Handle hostility if not done through teams
		for (var iplr = 0; iplr < GetPlayerCount(C4PT_User); ++iplr)
		{
			SetHostility(GetPlayerByIndex(iplr, C4PT_User), plr, true, true, true); // triple true hostility!
		}
		g_enemyspawn_player = plr;
	}
	// Perform delayed activation
	for (var inst in FindObjects(Find_ID(this)))
	{
		if (inst.waiting_for_script_player)
		{
			inst->ActivateSpawn();
		}
	}
}

public func InitializePlayer(int plr, int x, int y, object base, int team, extra_id)
{
	if (GetPlayerType(plr) == C4PT_User && IsEnemySpawnPlayerJoined())
	{
		// Make sure new enemy players are hostile
		SetHostility(g_enemyspawn_player, plr, true, true, true);
	}
}


/* Enemy spawning */

public func StartSpawn()
{
	// Timed spawn or immediate spawn?
	if (spawn_interval > 0 && spawn_count > 1)
	{
		// First enemy comes immediately. Then scheduled.
		Spawn();
		ScheduleCall(this, this.Spawn, spawn_interval, SPAWNCOUNT_INFINITE); // scheduler is removed once desired spawn count has been reached
	}
	else
	{
		// Immediate spawn
		// Can't do inifinite and immediate
		if (spawn_count == SPAWNCOUNT_INFINITE)
		{
			SpawnFinished();
			FatalError("EnemySpawn: Spawning of infinite enmies without timer delay disabled, because it would destroy the universe in a big boom (Hawking, Stephen W. \"Black hole explosions.\" Nature 248.5443 (1974): 30-31.)");
		}
		for (var i = 0; i < spawn_count; ++i)
		{
			Spawn();
		}
	}
}

private func SpawnFinished()
{
	// Stop spawning timer
	ClearScheduleCall(this, this.Spawn);
	// All enemies have been spawned. Mark inactive.
	active = waiting_for_script_player = false;
	SetClrModulation();
	spawned_count = 0;
	// Count finished waves
	if (!GetLength(spawned_enemies)) WaveDefeated();
}

private func GetAttackPath(int off_x, int off_y)
{
	// Get attack path in global coordinates
	if (!attack_path) return [ { X = off_x, Y = off_y } ];
	var global_attack_path = CreateArray(GetLength(attack_path)), i;
	for (var pt in attack_path)
	{
		global_attack_path[i++] = { X = off_x + pt.X, Y = off_y + pt.Y };
	}
	return global_attack_path;
}

private func Spawn()
{
	// Check concurrent enemy limit
	if (GetAliveEnemyCount() >= max_concurrent_enemies) return;
	// Find a spawn location
	var spawn_pos = GetSpawnPosition();
	// Spawn the actual enemy
	if (enemy)
	{
		var enemy_def = EnemyDefs[enemy.Type];
		var spawn_function = enemy_def.SpawnFunction;
		// Attack path may be relative to spawner or relative to the actual object creation position (e.g. for rockets created in a range)
		var use_attack_path;
		if (enemy_def.OffsetAttackPathByPos)
		{
			use_attack_path = GetAttackPath(spawn_pos[0], spawn_pos[1]);
		}
		else
		{
			use_attack_path = GetAttackPath(GetX(), GetY());
		}
		// Do spawn
		var enemies = (enemy_def.SpawnCallTarget ?? GetID())->Call(spawn_function, spawn_pos, enemy, enemy_def, use_attack_path, this);
		// Keep track of enemies. Could be returned as a single enemy or as an array
		var first_enemy;
		if (GetType(enemies) == C4V_Array)
		{
			for (var obj in enemies)
			{
				TrackSpawnedEnemy(obj);
			}
			first_enemy = enemies[0];
		}
		else
		{
			TrackSpawnedEnemy(enemies);
			first_enemy = enemies;
		}
		// Perform spawn action
		if (first_enemy) UserAction->EvaluateAction(spawn_action, this, first_enemy);
		if (!this) return;
	}
	// Keep track of count
	if (++spawned_count == spawn_count)
	{
		SpawnFinished();
	}
}

public func TrackSpawnedEnemy(object enemy)
{
	// Remember any spawned enemies
	if (enemy)
	{
		enemy.EnemySpawn_source = this;
		spawned_enemies[GetLength(spawned_enemies)] = enemy;
	}
}

public func OnRocketDeath(object rocket, int killed_by)
{
	return OnClonkDeath(rocket, killed_by);
}

public func OnClonkDeath(object clonk, int killed_by)
{
	if (clonk.EnemySpawn_source == this)
	{
		var idx = GetIndexOf(spawned_enemies, clonk);
		var n = GetLength(spawned_enemies) - 1;
		if (idx >= 0)
		{
			if (idx < n)
			{
				spawned_enemies[idx] = spawned_enemies[n];
			}
			SetLength(spawned_enemies, n);
		}
	}
}

public func HasBeenActivated()
{
	// Has the spawn ever been activated?
	return has_been_activated;
}

public func IsCleared()
{
	// Did the spawn finish spawning and are all enemies killed?
	return has_been_activated && !active && !waiting_for_script_player && !GetAliveEnemyCount();
}

public func GetAliveEnemyCount()
{
	var count = 0;
	for (var aenemy in spawned_enemies) count += !!aenemy;
	return count;
}

private func WaveDefeated()
{
	// Just keep track of the count
	++num_waves_defeated;
}

public func CancelSpawn()
{
	// Stop any spawning timers
	ClearScheduleCall(this, this.StartSpawn);
	ClearScheduleCall(this, this.Spawn);
	// Mark inactive
	SpawnFinished();
}

public func RemoveSpawnedEnemies()
{
	// Remove all spawned enemies
	for (var i = GetLength(spawned_enemies); i>=0; --i)
	{
		var aenemy = spawned_enemies[i];
		if (aenemy)
		{
			aenemy->RemoveObject();
		}
	}
	spawned_enemies = [];
}

public func ActivateSpawn()
{
	// Already triggered or still running?
	if (active) return;
	// Remember activation
	has_been_activated = true;
	// Needs to wait for script player join?
	if (!IsEnemySpawnPlayerJoined())
	{
		waiting_for_script_player = true;
		return;
	}
	// Activate
	waiting_for_script_player = false;
	active = true;
	SetClrModulation(0xffff2020);
	if (spawn_delay)
	{
		ScheduleCall(this, this.StartSpawn, spawn_delay, 1);
	}
	else
	{
		StartSpawn();
	}
}

public func GetSpawnPosition()
{
	// Evaluate spawn position setting
	if (spawn_position)
	{
		var spawn_position_mode = spawn_position.Mode;
		if (spawn_position_mode == "range")
		{
			// Random position in a circle around this position
			var r = Sqrt(Random(spawn_position.Radius * spawn_position.Radius));
			var ang = Random(360);
			return [GetX() + Sin(ang, r), GetY() + Cos(ang, r)];
		}
		else if (spawn_position_mode == "rectangle")	
		{
			var rect = spawn_position.Area;
			return [GetX() + rect[0] + Random(rect[2]), GetY() + rect[1] + Random(rect[3])];
		}
	}
	// Default: Spawn here
	return [GetX(), GetY()];
}

public func InitializePlayers()
{
	// Activate auto-triggered spawns
	if (auto_activate) ActivateSpawn();
}


/* Clonk spawning */

public func SpawnClonk(array pos, proplist clonk_data, proplist enemy_def, array clonk_attack_path, object spawner)
{
	// Spawn it
	var clonk = CreateObject(Clonk, pos[0], pos[1], g_enemyspawn_player);
	if (!clonk) return [];
	clonk->SetController(g_enemyspawn_player);
	clonk->MakeCrewMember(g_enemyspawn_player);
	// Enemy visuals
	if (clonk_data.Skin)
	{
		clonk->SetSkin(clonk_data.Skin);
	}
	if (!clonk_data.Backpack)
	{
		clonk->~RemoveBackpack();
	}
	if (clonk_data.ScaleX != 100 || clonk_data.ScaleY != 100)
	{
		var scale_z = (clonk_data.ScaleX + clonk_data.ScaleY) / 2;
		clonk->SetMeshTransformation(Trans_Scale(clonk_data.ScaleX, clonk_data.ScaleY, scale_z), CLONK_MESH_TRANSFORM_SLOT_Scale);
	}
	if (clonk_data.Name && GetLength(clonk_data.Name))
	{
		clonk->SetName(clonk_data.Name);
	}
	clonk->SetColor(clonk_data.Color);
	// Physical properties
	clonk.MaxEnergy = clonk_data.Energy * 1000;
	clonk->DoEnergy(10000);
	if (clonk_data.Speed != 100)
	{
		// Speed: Modify Speed in all ActMap entries
		if (clonk.ActMap == clonk.Prototype.ActMap) clonk.ActMap = new clonk.ActMap {};
		for (var action in GetProperties(Clonk.ActMap))
		{
			if (action == "Prototype") continue;
			if (clonk.ActMap[action] == clonk.Prototype.ActMap[action]) clonk.ActMap[action] = new clonk.ActMap[action] {};
			clonk.ActMap[action].Speed = clonk.ActMap[action].Speed * clonk_data.Speed / 100;
		}
		clonk.JumpSpeed = clonk.JumpSpeed * clonk_data.Speed / 100;
		clonk.FlySpeed = clonk.FlySpeed * clonk_data.Speed / 100;
	}
	clonk.MaxContentsCount = 1;
	// Reward for killing enemy
	clonk.Bounty = clonk_data.Bounty;
	// AI
	var fx_ai = AI->AddAI(clonk);
	AI->SetMaxAggroDistance(clonk, Max(LandscapeWidth(), LandscapeHeight()));
	var guard_range = clonk_data.GuardRange;
	if (!guard_range)
	{
		// Automatic guard range around attack path
		var guard_min_x = spawner->GetX();
		var guard_min_y = spawner->GetY();
		var guard_max_x = guard_min_x, guard_max_y = guard_min_y;
		for (var attack_pos in clonk_attack_path)
		{
			guard_min_x = Min(guard_min_x, attack_pos.X);
			guard_min_y = Min(guard_min_y, attack_pos.Y);
			guard_max_x = Max(guard_max_x, attack_pos.X);
			guard_max_y = Max(guard_max_y, attack_pos.Y);
		}
		guard_range = { x = guard_min_x-300, y = guard_min_y-150, wdt = guard_max_x-guard_min_x + 600, hgt = guard_max_y-guard_min_y + 300 };
	}
	AI->SetGuardRange(clonk, guard_range.x, guard_range.y, guard_range.wdt, guard_range.hgt);
	if (clonk_data.AttackMode)
	{
		AI->SetAttackMode(clonk, clonk_data.AttackMode.Identifier);
	}
	AI->SetAttackPath(clonk, clonk_attack_path);
	// Attack speed on weapon
	if (clonk_data.AttackSpeed != 100 && fx_ai && fx_ai.default_weapon)
	{
		fx_ai.default_weapon->~SetSpeedMultiplier(clonk_data.AttackSpeed);
	}
	// Shield
	if (clonk_data.Shield)
	{
		clonk->CreateContents(Shield);
	}
	// Return clonk to be added to spawned enemy list
	return clonk;
}

public func SpawnAICreature(proplist enemy_data, pos, enemy_def, attack_path, spawner)
{
	if (enemy_data && enemy_data.Type == "Clonk")
	{
		// Only Clonks supported for now
		return SpawnClonk(pos, enemy_data.Properties, enemy_def, attack_path, spawner);
	}
}


/* Display */

private func UpdateEnemyDisplay()
{
	// Show enemy type as text above this object
	var msg;
	var error = false;
	if (enemy)
	{
		var enemy_def = EnemyDefs[enemy.Type];
		if (enemy_def == nil)
		{
			msg = Format("Unknown enemy type %v", enemy.Type);
			error = true;
		}
		else if (enemy_def.GetInfoString)
		{
			// Custom dynamic info string
			msg = enemy_def->GetInfoString(enemy);
		}
		else if (enemy_def.InfoString)
		{
			// Custom fixed info string
			msg = enemy_def.InfoString;
		}
		else if (enemy_def.SpawnType)
		{
			// No info string. Fall back to object ID.
			msg = Format("{{%i}}", enemy_def.SpawnType);
		}
		else
		{
			// Can't happen.
			msg = enemy_def.Name ?? "??";
		}
	}
	if (msg) Message("@%s", msg); else Message("");
	if (error)
	{
		FatalError("EnemySpawn: %s", msg); 
	}
}

public func EditorPropertyChanged()
{
	return UpdateEnemyDisplay();
}


/* Editor props and actions */

public func Definition(def)
{
	// EditorActions
	if (!def.EditorActions) def.EditorActions = {};
	def.EditorActions.Activate = { Name="$Activate$", EditorHelp = "$ActivateHelp$", Command="ActivateSpawn()" };
	def.EditorActions.Stop = { Name="$Stop$", EditorHelp = "$StopHelp$", Command="CancelSpawn()" };
	def.EditorActions.RemoveSpawnedEnemies = { Name="$RemoveSpawnedEnemies$", EditorHelp = "$RemoveSpawnedEnemiesHelp$", Command="RemoveSpawnedEnemies()" };
	// UserActions
	UserAction->AddEvaluator("Action", "$Name$", "$ActActivate$", "$ActivateHelp$", "enemy_spawn_set_active", [def, def.EvalAct_Activate], { Target = { Function="action_object" } }, { Type="proplist", EditorProps = {
		Target = UserAction->GetObjectEvaluator("IsEnemySpawn", "$Name$")
		} } );
	UserAction->AddEvaluator("Action", "$Name$", "$ActStop$", "$StopHelp$", "enemy_spawn_stop", [def, def.EvalAct_Stop], { Target = { Function="action_object" } }, { Type="proplist", EditorProps = {
		Target = UserAction->GetObjectEvaluator("IsEnemySpawn", "$Name$")
		} } );
	// UserAction evaluators
	var kill_check_options = { Name="$KillCheckTarget$", Type="enum", Options=[
		{ Name="$AllSpawners$", EditorHelp="$KillCheckAllSpawnersHelp$" },
		{ Name="$ThisSeqeunceSpawners$", EditorHelp="$ThisSequenceSpawnersHelp$", OptionKey="Function", Value={ Function="this_context" } },
		{ Name="$SpecificSpawner$", EditorHelp="$SpecificSpawnerHelp$",
			OptionKey="Function", Value={ Function="specified" }, ValueKey="Object",
			Delegate = UserAction->GetObjectEvaluator("IsEnemySpawn", "$SpecificSpawner$", "$SpecificSpawnerHelp$")
		} ] };
	UserAction->AddEvaluator("Boolean", nil, "$AllEnemiesKilled$", "$AllEnemiesKilledHelp$", "spawned_enemies_killed", [def, def.EvalBool_SpawnedEnemiesKilled], { }, kill_check_options, "Spawner");
	// EditorProps
	if (!def.EditorProps) def.EditorProps = {};
  def.EditorProps.spawn_position = { Name="$SpawnPosition$", Type="enum", OptionKey="Mode", Set="SetSpawnPosition", Save="SpawnPosition", Options = [
		{ Name="$Here$" },
		{ Name="$InRange$", EditorHelp="$SpawnInRangeHelp$", Value={ Mode="range", Radius = 25 }, ValueKey="Radius", Delegate={ Type="circle", Color = 0xff8000, Relative = true } },
		{ Name="$InRect$", EditorHelp="$SpawnInRectHelp$", Value={ Mode="rectangle", Area=[-20, -20, 40, 40] }, ValueKey="Area", Delegate={ Type="rect", Color = 0xff8000, Relative = true } }
		] };
	def.EditorProps.spawn_count = { Name="$SpawnCount$", EditorHelp="$SpawnCountHelp$", Type="enum", Set="SetSpawnCount", Save="SpawnCount", Options = [
	  { Name="$Infinite$", Value = SPAWNCOUNT_INFINITE },
	  { Name="$FixedNumber$", Value = 1, Type = C4V_Int, Delegate={ Type="int", Min = 1, Set="SetSpawnCount", SetRoot = true } }
	  ] };
	def.EditorProps.spawn_delay = { Name="$SpawnDelay$", EditorHelp="$SpawnDelayHelp$", Type="int", Min = 0, Set="SetSpawnDelay", Save="SpawnDelay" };
	def.EditorProps.spawn_interval = { Name="$SpawnInterval$", EditorHelp="$SpawnIntervalHelp$", Type="int", Min = 0, Set="SetSpawnInterval", Save="SpawnInterval" };
	def.EditorProps.attack_path = { Name="$AttackPath$", EditorHelp="$AttackPathHelp$", Type="polyline", StartFromObject = true, DrawArrows = true, Color = 0xdf0000, Relative = true, Save="AttackPath" }; // always saved
	def.EditorProps.auto_activate = { Name="$AutoActivate$", EditorHelp="$AutoActivateHelp$", Type="bool", Set="SetAutoActivate", Save="AutoActivate" };
	def.EditorProps.max_concurrent_enemies = { Name="$MaxConcurrent$", EditorHelp="$MaxConcurrentHelp$", Type="enum", Set="SetMaxConcurrentEnemies", Save="MaxConcurrentEnemies", Options = [
	  { Name="$Unlimited$", Value = SPAWNCOUNT_INFINITE },
	  { Name="$FixedNumber$", Value = 1, Type = C4V_Int, Delegate={ Type="int", Min = 1, Set="SetMaxConcurrentEnemies", SetRoot = true } }
	  ] };
	def.EditorProps.spawn_action = new UserAction.Prop { Name="$SpawnAction$", EditorHelp="$SpawnActionHelp$", Save="SpawnAction" };
	AddEnemyDef("Clonk", { SpawnType = Clonk, SpawnFunction = def.SpawnClonk, GetInfoString = GetAIClonkInfoString }, def->GetAIClonkDefaultPropValues(), def->GetAIClonkEditorProps());
}

public func GetAIClonkEditorProps()
{
	// Return an editor props delegate for AI clonks
	if (!this.AIClonkEditorProps)
	{
		var props = {};
		props.AttackMode = new AI->GetControlEffect().EditorProps.attack_mode { Set = nil, Priority = 100 };
		props.GuardRange = { Name="$AttackRange$", EditorHelp="$AttackRangeHelp$", Type="enum", Options = [
			{ Name="$Automatic$", EditorHelp="$AutomaticGuardRangeHelp$"},
			{ Name="$Custom$", Type = C4V_PropList, Value={}, DefaultValueFunction = this.GetDefaultAIRect, Delegate = AI->GetControlEffect().EditorProps.guard_range }
			] };
		props.Color = { Name="$Color$", Type="color" };
		props.Bounty = { Name="$Bounty$", EditorHelp="$BountyHelp$", Type="int", Min = 0, Max = 100000 };
		props.ScaleX = { Name="$ScaleX$", EditorHelp="$ScaleXHelp$", Type="int", Min = 50, Max = 1000 };
		props.ScaleY = { Name="$ScaleY$", EditorHelp="$ScaleYHelp$", Type="int", Min = 50, Max = 1000 };
		props.Speed = { Name="$Speed$", EditorHelp="$SpeedHelp$", Type="int", Min = 5 };
		props.Energy = { Name="$Energy$", EditorHelp="$EnergyHelp$", Type="int", Min = 1, Max = 100000 };
		props.Backpack = { Name="$Backpack$", EditorHelp="$BackpackHelp$", Type="bool" };
		props.Shield = { Name="$Shield$", EditorHelp="$ShieldHelp$", Type="bool" };
		props.AttackSpeed = { Name="$AttackSpeed$", EditorHelp="$AttackSpeedHelp$", Type="int", Min = 1, Max = 20000 };
		this.AIClonkEditorProps = { Type="proplist", Name = Clonk->GetName(), EditorProps = props, Priority = 100 };
	}
	return this.AIClonkEditorProps;
}

public func GetAIClonkDefaultPropValues(string attack_mode)
{
	// Default settings for AI enemy clonks
	return {
		AttackMode = { Identifier = attack_mode ?? "Sword" },
		Color = 0xff0000,
		Bounty = 0,
		ScaleX = 100,
		ScaleY = 100,
		Speed = 100,
		Energy = 50,
		Backpack = false,
		Shield = false,
		AttackSpeed = 100,
		};
}

public func GetAICreatureEditorProps(proplist default_clonk_properties, string none_help)
{
	// TODO: Allow registration of more creature types
	var props = {
		Type="enum", ValueKey="Properties", OptionKey="Type", Options=[
				{ Name="$None$", EditorHelp = none_help },
				{ Name = Clonk->GetName(), Value={
						Type="Clonk",
						Properties = default_clonk_properties ?? EnemySpawn->GetAIClonkDefaultPropValues()
						},
					Delegate = EnemySpawn->GetAIClonkEditorProps() }
				] };
	return props;
}

public func GetAIClonkInfoString(proplist enemy_props)
{
	// Spawn info string shown above enemy spawns
	var msg = "{{Clonk}}";
	var attack_mode = AI.AttackModes[enemy_props.AttackMode.Identifier];
	var str = attack_mode->~GetInfoString(enemy_props.AttackMode);
	if (str) msg = Format("%s%s", msg, str);
	return msg;
}

public func GetAICreatureInfoString(proplist enemy_props)
{
	// Spawn info string shown above enemy spawns
	if (enemy_props)
	{
		if (enemy_props.Type == "Clonk") return GetAIClonkInfoString(enemy_props.Properties);
	}
	return "";
}

private func SetAIClonkAttackMode(proplist attack_mode)
{
	this.AttackMode.Identifier = attack_mode.Identifier;
}

private func GetDefaultAIRect(object target_object, proplist props)
{
	// Default attack rectangle around spawner
	var r = {};
	if (target_object)
	{
		r.x = target_object->GetX()-300;
		r.y = target_object->GetY()-150;
		r.wdt = 600;
		r.hgt = 300;
	}
	return r;
}

public func AddEnemyDef(identifier, enemy_def, default_value, parameter_delegate)
{
	// First-time setup of enemy selection in editor
	if (!this.EditorProps) this.EditorProps = {};
	if (!this.EditorProps.enemy) this.EditorProps.enemy = {
		Name = "$Enemy$",
		EditorHelp = "$EnemyHelp$",
		Type = "enum",
		OptionKey = "Type",
		Set = "SetEnemy",
		Save = "Enemy",
		Sorted = true,
		Priority = 100,
		Options = [ { Name="$None$", Priority = 100 } ] };
	if (!this.EnemyDefs) this.EnemyDefs = {};
	// Remember definition
	this.EnemyDefs[identifier] = enemy_def;
	// Add editor selection
	if (!default_value) default_value = {};
	default_value.Type = identifier;
	var enemy_opt = {
		Name = enemy_def.Name ?? enemy_def.SpawnType->GetName(),
		EditorHelp = enemy_def.EditorHelp,
		Delegate = parameter_delegate,
		Value = default_value
		};
	var opts = this.EditorProps.enemy.Options;
	opts[GetLength(opts)] = enemy_opt;
}

private func EvalAct_Activate(proplist props, proplist context)
{
	// User action: Activate spawner.
	var spawner = UserAction->EvaluateValue("Object", props.Target, context);
	if (!spawner || !spawner->IsEnemySpawn())
	{
		return;
	}
	// Remember all activated spawners
	if (!context.activated_enemy_spawns)
		context.activated_enemy_spawns = [spawner];
	else if (GetIndexOf(context.activated_enemy_spawns, spawner) == -1)
		context.activated_enemy_spawns[GetLength(context.activated_enemy_spawns)] = spawner;
	// Activate!
	spawner->ActivateSpawn();
}

private func EvalAct_Stop(proplist props, proplist context)
{
	// User action: Cancel spawner activation.
	// Keep in activated spawner list because enemies may already have been spawned
	var spawner = UserAction->EvaluateValue("Object", props.Target, context);
	if (!spawner || !spawner->IsEnemySpawn())
	{
		return;
	}
	spawner->CancelSpawn();
}

private func EvalBool_SpawnedEnemiesKilled(proplist props, proplist context)
{
	// User action evaluator: Enemy spawn cleared
	var spawner = props.Spawner;
	if (!spawner)
	{
		// Check if there's an activated, un-cleared spawner anywhere on the map
		return !FindObject(Find_Func("IsEnemySpawn"), Find_Func("HasBeenActivated"), Find_Not(Find_Func("IsCleared")));
	}
	if (spawner.Function == "this_context")
	{
		// Check if all spawns activated in this context have been cleared
		if (context.activated_enemy_spawns)
			for (var spawn in context.activated_enemy_spawns)
				if (!spawn->IsCleared())
					return false;
		return true;
	}
	// Must be specified spawner now
	// Specified spawner must exist and be both activated and cleared
	var obj = UserAction->EvaluateValue("Object", spawner.Object, context);
	return obj && obj->~IsEnemySpawn() && obj->IsCleared();
}
