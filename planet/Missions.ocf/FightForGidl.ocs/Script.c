/*
	Fight for Gidl
	Authors: Sven2
	
	Defend the statue against waves of enemies
*/

static g_goal, g_object_fade, g_statue, g_doorleft, g_doorright;
static g_wave; // index of current wave
static g_spawned_enemies;
static g_relaunchs; // array of relaunch counts
static g_scores; // array of player scores
static g_ai; // derived from S2AI; contains changes for this scenario

static const MAX_RELAUNCH = 10;

//======================================================================
/* Initialization */

func Initialize()
{
	// static variable init
	InitWaveData();
}

func InitializePlayer(int plr, int iX, int iY, object pBase, int iTeam)
{
	if (!g_statue) { EliminatePlayer(plr); return; } // no post-elimination join
	if (!g_relaunchs)
	{
		g_relaunchs = [];
		g_scores = [];
		Scoreboard->Init([{key = "relaunchs", title = Rule_Restart, sorted = true, desc = true, default = "", priority = 75},
	                    {key = "score", title = Nugget, sorted = true, desc = true, default = "0", priority = 100}]);
	}
	for (var flagpole in FindObjects(Find_ID(Flagpole), Find_Owner(NO_OWNER))) flagpole->SetOwner(plr);
	for (var stonedoor in FindObjects(Find_ID(StoneDoor), Find_Owner(NO_OWNER))) stonedoor->SetOwner(plr);
	g_relaunchs[plr] = MAX_RELAUNCH;
	g_scores[plr] = 0;
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "relaunchs", g_relaunchs[plr]);
	Scoreboard->SetPlayerData(plr, "score", g_scores[plr]);
	SetFoW(false,plr);
	JoinPlayer(plr);
	if (!g_wave) StartGame();
	return;
}

func RemovePlayer(int plr)
{
	Scoreboard->SetPlayerData(plr, "relaunchs", Icon_Cancel);
	return;
}

func RelaunchPlayer(int plr)
{
	// Relaunch count
	if (!g_relaunchs[plr])
	{
		Log("$MsgOutOfRelaunchs$", GetTaggedPlayerName(plr));
		Scoreboard->SetPlayerData(plr, "relaunchs", Icon_Cancel);
		EliminatePlayer(plr);
		return false;
	}
	// Relaunch count
	--g_relaunchs[plr];
	Scoreboard->SetPlayerData(plr, "relaunchs", g_relaunchs[plr]);
	Log("$MsgRelaunch$", GetTaggedPlayerName(plr));
	JoinPlayer(plr);
	//var gui_arrow = FindObject(Find_ID(GUI_GoalArrow), Find_Owner(plr));
	//gui_arrow->SetAction("Show", GetCursor(plr));
}

func JoinPlayer(plr)
{
	var relaunch_target = FindObject(Find_ID(Flagpole), Sort_Random()),x,y;
	if (relaunch_target)
		{ x = relaunch_target->GetX(); y = relaunch_target->GetY()+20; }
	else
		{ x = LandscapeWidth()/2; y = LandscapeHeight()/2-100; }
	var clonk = GetCrew(plr);
	if (clonk)
	{
		clonk->SetPosition(x,y);
	}
	else
	{
		clonk = CreateObjectAbove(Clonk, x,y+10, plr);
		clonk->MakeCrewMember(plr);
	}
	SetCursor(plr, clonk);
	clonk->DoEnergy(1000);
	// contents
	//clonk.MaxContentsCount = CustomAI.Clonk_MaxContentsCount;
	//clonk.MaxContentsCountVal = 2;
	clonk->CreateContents(Bow);
	var arrow = CreateObjectAbove(Arrow);
	clonk->Collect(arrow);
	arrow->SetInfiniteStackCount();
	//clonk->CreateContents(Musket);
	//clonk->Collect(CreateObjectAbove(LeadShot));
	clonk->~CrewSelection(); // force update HUD
	return;
}

func StartGame()
{
	// Init objects to defend
	var obj;
	for (obj in [g_statue, g_doorleft, g_doorright]) if (obj)
	{
		obj->SetCategory(C4D_Living);
		obj->SetAlive(true);
		obj.MaxEnergy = 800000;
		obj->DoEnergy(obj.MaxEnergy/1000);
		obj->AddEnergyBar();
		obj.FxNoPlayerDamageDamage = Scenario.Object_NoPlayerDamage;
		AddEffect("NoPlayerDamage", obj, 500, 0, obj);
	}
	if (g_statue)
	{
		g_statue.Death = Scenario.Statue_Death;
	}
	// Launch first wave!
	g_wave = 1;
	ScheduleCall(nil, Scenario.LaunchWave, 50, 1, g_wave);
	return true;
}

func Object_NoPlayerDamage(object target, fx, dmg, cause, cause_player)
{
	// players can't damage statue or doors
	if (cause_player>=0) return 0;
	return dmg;
}



//======================================================================
/* Enemy waves */

func LaunchWave(int wave)
{
	// * Schedules spawning of all enemies 
	// * Schedules call to LaunchWaveDone() after last enemy has been spawned
	var wave_data = ENEMY_WAVE_DATA[g_wave];
	g_spawned_enemies = [];
	if (wave_data)
	{
		var wave_spawn_time = 0;
		CustomMessage(Format("$MsgWave$: %s", wave, wave_data.Name));
		Sound("Ding");
		for (var enemy in ForceVal2Array(wave_data.Enemies)) if (enemy)
		{
			if (enemy.Delay)
				ScheduleCall(nil, Scenario.ScheduleLaunchEnemy, enemy.Delay, 1, enemy);
			else
				ScheduleLaunchEnemy(enemy);
			wave_spawn_time = Max(wave_spawn_time, enemy.Delay + enemy.Interval * enemy.Num);
		}
		ScheduleCall(nil, Scenario.LaunchWaveDone, wave_spawn_time+5, 1, wave);
		return true;
	}
	return false;
}

func ScheduleLaunchEnemy(proplist enemy)
{
	// Schedules spawning of enemy definition
	var side = enemy.Side;
	if (!side) side = WAVE_SIDE_LEFT | WAVE_SIDE_RIGHT;
	if (side & WAVE_SIDE_LEFT)  ScheduleCall(nil, CustomAI.LaunchEnemy, Max(enemy.Interval,1), Max(enemy.Num,1), enemy, 10, 529);
	if (side & WAVE_SIDE_RIGHT) ScheduleCall(nil, CustomAI.LaunchEnemy, Max(enemy.Interval,1), Max(enemy.Num,1), enemy, 1190, 509);
	return true;
}

func LaunchWaveDone(int wave)
{
	// All enemies spawned! Now start timer to check whether they are all dead
	ScheduleCall(nil, Scenario.CheckWaveCleared, 20, 9999999, wave);
	return true;
}

func CheckWaveCleared(int wave)
{
	// Check timer to determine if enemy wave has been cleared.
	// Enemies nil themselves when they're dead. So clear out nils and we're done when the list is empty
	var nil_idx;
	while ( (nil_idx=GetIndexOf(g_spawned_enemies))>=0 )
	{
		var l = GetLength(g_spawned_enemies) - 1;
		if (nil_idx<l) g_spawned_enemies[nil_idx] = g_spawned_enemies[l];
		SetLength(g_spawned_enemies, l);
	}
	if (!GetLength(g_spawned_enemies))
	{
		// All enemies dead!
		ClearScheduleCall(nil, Scenario.CheckWaveCleared);
		OnWaveCleared(wave);
	}
}

func OnWaveCleared(int wave)
{
	CustomMessage(Format("$MsgWaveCleared$", wave));
	Sound("Ding");
	// Fade out corpses
	if (g_object_fade) 
		for (var obj in FindObjects(Find_Or(Find_And(Find_ID(Clonk), Find_Not(Find_OCF(OCF_Alive))), Find_ID(Catapult))))
			obj->AddEffect("IntFadeOut", obj, 100, 1, g_object_fade, Rule_ObjectFade);
	// Next wave!
	++g_wave;
	if (ENEMY_WAVE_DATA[g_wave])
		ScheduleCall(nil, Scenario.LaunchWave, 500, 1, g_wave);
	else
	{
		// There is no next wave? Game done D:
		ScheduleCall(nil, Scenario.OnAllWavesCleared, 50, 1);
	}
}

// Clonk death callback
func OnClonkDeath(clonk, killed_by)
{
	// Remove inventory (players and enemies)
	var i = clonk->ContentsCount(), obj;
	while (i--) if (obj=clonk->Contents(i))
		if (!obj->~OnContainerDeath())
			obj->RemoveObject();
	// Clear enemies from list
	i = GetIndexOf(g_spawned_enemies, clonk);
	if (i>=0)
	{
		g_spawned_enemies[i] = nil;
		// Kill bounty
		if (killed_by>=0)
		{
			Scoreboard->SetPlayerData(killed_by, "score", ++g_scores[killed_by]);
			DoWealth(killed_by, clonk.Bounty);
		}
	}
	return;
}



//======================================================================
/* Game end */

func OnAllWavesCleared()
{
	// Success!
	if (g_goal) g_goal.is_fulfilled = true;
	GainScenarioAchievement("Done");
	GameOver();
	return true;
}

func Statue_Death()
{
	// Fail :(
	// Elminiate all players
	var i=GetPlayerCount(C4PT_User);
	while (i--) EliminatePlayer(GetPlayerByIndex(i, C4PT_User));
	// Statue down :(
	CastObjects(Nugget, 5, 10);
	Explode(10);
	return true;
}


//======================================================================
/* Wave and enemy definitions */

static const CSKIN_Default = 0,
             CSKIN_Steampunk = 1,
             CSKIN_Alchemist = 2,
             CSKIN_Farmer = 3,
             CSKIN_Amazon = [CSKIN_Farmer, "farmerClonkAmazon"],
             CSKIN_Ogre = [CSKIN_Alchemist, "alchemistClonkOgre"];

static const WAVE_POS_LEFT = [10, 529];
static const WAVE_POS_RIGHT = [1190, 509];

static const WAVE_SIDE_LEFT = 1,
             WAVE_SIDE_RIGHT = 2;

static ENEMY_WAVE_DATA;

static const g_respawning_weapons = [Firestone, Rock];

// init ENEMY_WAVE_DATA - would like to make it const, but arrays in static const proplists don't work properly
func InitWaveData()
{
	// Define weapon types
	var bigsword   = { InvType=Sword,     Scale=2000, Material="LaserSword"};
	var ogresword  = { InvType=Sword,     Scale=1800, Material="OgreSword"};
	var bigclub    = { InvType=Club,      Scale=2000};
	var nukekeg    = { InvType=PowderKeg, Scale=1400, Material="NukePowderKeg", Strength=80};
	// Define different enemy types
	var newbie     = { Name="$EnemyNewbie$",    Inventory=Rock,        Energy=  1, Bounty=  1, Color=0xff8000ff,                                  };
	var flintstone = { Name="$EnemyFlintstone$",Inventory=Firestone,   Energy= 10, Bounty=  3, Color=0xff8080ff,                                  };
	var bowman     = { Name="$EnemyBow$",       Inventory=[Bow, Arrow],Energy= 10, Bounty=  5, Color=0xff00ff00, Skin=CSKIN_Farmer                };
	var amazon     = { Name="$EnemyAmazon$",    Inventory=Javelin,     Energy= 10, Bounty=  5,                   Skin=CSKIN_Amazon,    Backpack=0 };
	var suicide    = { Name="$EnemySuicide$",   Inventory=PowderKeg,   Energy= 20, Bounty= 15, Color=0xffff0000, Skin=CSKIN_Alchemist, Backpack=0,                         Speed=80, Siege=true };
	var runner     = { Name="$EnemyRunner$",    Inventory=Rock,        Energy=  1, Bounty= 10, Color=0xffff0000,                       Backpack=0,                         Speed=250           };
	var artillery  = { Name="$EnemyArtillery$", Inventory=Firestone,   Energy= 30, Bounty= 20, Color=0xffffff00, Skin=CSKIN_Steampunk,             Vehicle=Catapult };
	var swordman   = { Name="$EnemySwordman$",  Inventory=Sword,       Energy= 30, Bounty= 30, Color=0xff0000ff,                                  };
	var bigswordman= { Name="$EnemySwordman2$", Inventory=bigsword,    Energy= 60, Bounty= 60, Color=0xff00ffff, Skin=CSKIN_Steampunk, Backpack=0 };
	var ogre       = { Name="$EnemyOgre$",      Inventory=bigclub,     Energy= 90, Bounty=100, Color=0xff00ffff, Skin=CSKIN_Ogre,      Backpack=0, Scale=[1400,1200,1200], Speed=50 };
	var swordogre  = { Name="$EnemyOgre$",      Inventory=ogresword,   Energy= 90, Bounty=100, Color=0xff805000, Skin=CSKIN_Ogre,      Backpack=0, Scale=[1400,1200,1200], Speed=50 };
	var nukeogre   = { Name="$EnemyOgre$",      Inventory=nukekeg,     Energy=120, Bounty=100, Color=0xffff0000, Skin=CSKIN_Ogre,      Backpack=0, Scale=[1400,1200,1200], Speed=40, Siege=true };
	//newbie = runner;
	//newbie = runner;

	// Define composition of waves
	ENEMY_WAVE_DATA = [nil,
	    { Name = "$WaveNewbies$", Enemies = [
			new newbie      {            Num= 1, Interval=10, Side = WAVE_SIDE_LEFT }
	]}, { Name = "$WaveBows$", Enemies = [
			new newbie      {            Num= 2, Interval=10 },
			new bowman      { Delay= 30, Num= 3, Interval=10, Side = WAVE_SIDE_RIGHT },
			new amazon      { Delay= 30, Num= 3, Interval=10, Side = WAVE_SIDE_LEFT }
	]}, { Name = "Explosive", Enemies = [
			new flintstone  {            Num=10, Interval=20 }
	]}, { Name = "Suicidal", Enemies = [
			new suicide     {            Num= 2, Interval= 5 },
			new flintstone  { Delay= 15, Num= 5, Interval= 5 },
			new suicide     { Delay= 50, Num= 1 }
	]}, { Name = "Swordsmen", Enemies = [
			new swordman    {            Num=10, Interval=20 },
			new bigswordman { Delay=210, Num= 1, Interval=10 }
	]}, { Name = "Oh Shrek!", Enemies = [
			new ogre        {            Num= 2, Interval=20 },
			new swordogre   { Delay= 40, Num= 1, Interval=20 },
			new bowman      { Delay= 60, Num= 3, Interval= 5 }
	]}, { Name = "Heavy artillery incoming", Enemies = [
			new artillery   {            Num= 1              },
			new ogre        {            Num= 2, Interval=20 },
			new flintstone  { Delay= 15, Num= 5, Interval= 5 },
			new swordogre   { Delay= 60, Num= 1, Interval=20 }
	]}, { Name = "Stop the big ones", Enemies = [
			new flintstone  {            Num=20, Interval=15 },
			new nukeogre    {            Num= 2, Interval=99 },
			new amazon      { Delay= 50, Num= 6, Interval=10 }
	]}, { Name = "Two of each kind", Enemies = [
			new newbie      { Delay=  0                      },
			new ogre        { Delay=  0                      },
			new swordman    { Delay= 10                      },
			new amazon      { Delay= 12                      },
			new nukeogre    { Delay= 14                      },
			new swordogre   { Delay= 20                      },
			new artillery   { Delay= 21                      },
			new flintstone  { Delay= 22                      },
			new bigswordman { Delay= 40                      },
			new bowman      { Delay= 45                      },
			new suicide     { Delay= 30                      }
	]}, { Name = "Finale!", Enemies = [
			new artillery   {            Num= 2, Interval= 5 },
			new newbie      {            Num=10, Interval=15 },
			new swordman    { Delay=  3, Num= 5, Interval=30 },
			new nukeogre    { Delay= 60, Num= 2, Interval=50 },
			new bigswordman { Delay=103, Num= 3, Interval=30 },
			new amazon      { Delay= 10, Num= 5, Interval=10 },
			new flintstone  {            Num=20, Interval=10 },
			new bowman      { Delay=  8, Num= 5, Interval=40 },
			new suicide     { Delay= 25, Num=10, Interval=20 },
			new ogre        { Delay= 30, Num= 3, Interval=20 },
			new swordogre   { Delay=  4, Num= 4, Interval=99 }
	]} ];
	return true;
}
