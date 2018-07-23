/*
	Fight for Gidl
	Authors: Sven2
	
	Defend the statue against waves of enemies
*/

static g_wave; // index of current wave
static g_spawned_enemies;
static g_relaunchs; // array of relaunch counts
static g_scores; // array of player scores
static g_ai; // derived from AI; contains changes for this scenario
static const ENEMY = 10; // player number of enemy
static const ALLOW_DEBUG_COMMANDS = true;

static const MAX_RELAUNCH = 10;

static shared_wealth_remainder;

//======================================================================
/* Initialization */

func Initialize()
{
	// dev stuff (we will forget to turn this off for release)
	//AddMsgBoardCmd("waveinfo", "GameCall(\"ShowWaveInfo\")");
	//AddMsgBoardCmd("next", "GameCall(\"SetNextWave\", \"%s\")");
	//AddMsgBoardCmd("nextwait", "GameCall(\"SetNextWave\", \"%s\", true)");
	//AddMsgBoardCmd("scrooge", "GameCall(\"DoWealthForAll\", 1000000000)");
	// Init door dummies
	g_doorleft.dummy_target = g_doorleft->CreateObject(DoorDummy, -6, 6);
	g_doorright.dummy_target = g_doorright->CreateObject(DoorDummy, +6, 6);
	// Wealth shown at all time
	GUI_Controller->ShowWealth();
	// static variable init
	g_homebases = [];
	InitWaveData();
}

func InitializePlayer(int plr, int iX, int iY, object pBase, int iTeam)
{
	if (GetPlayerType(plr) != C4PT_User) return;
	//DoWealth(plr, 10000);
	if (!g_statue) { EliminatePlayer(plr); return; } // no post-elimination join
	if (!g_relaunchs)
	{
		g_relaunchs = [];
		g_scores = [];
		Scoreboard->Init([{key = "relaunchs", title = Rule_Relaunch, sorted = true, desc = true, default = "", priority = 75},
	                    {key = "score", title = Nugget, sorted = true, desc = true, default = "0", priority = 100}]);
	}
	for (var stonedoor in FindObjects(Find_ID(StoneDoor), Find_Owner(NO_OWNER))) stonedoor->SetOwner(plr);
	g_relaunchs[plr] = MAX_RELAUNCH;
	g_scores[plr] = 0;
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "relaunchs", g_relaunchs[plr]);
	Scoreboard->SetPlayerData(plr, "score", g_scores[plr]);
	//SetFoW(false,plr); - need FoW for lights
	CreateObject(Homebase, 0,0, plr);
	JoinPlayer(plr);
	if (!g_wave) StartGame();
	return;
}

func RemovePlayer(int plr)
{
	if (g_homebases[plr]) g_homebases[plr]->RemoveObject();
	Scoreboard->SetPlayerData(plr, "relaunchs", Icon_Cancel);
	// Split player's wealth among the remaining players
	ScheduleCall(nil, Scenario.DoSharedWealth, 50, 1, GetWealth(plr));
	return;
}

private func TransferInventory(object from, object to)
{
	// Drop some items that cannot be transferred (such as connected pipes and dynamite igniters)
	var i = from->ContentsCount();
	while (i--)
	{
		var contents = from->Contents(i);
		if (contents && contents->~IsDroppedOnDeath(from))
			contents->Exit();
	}
	return to->GrabContents(from);
}

func JoinPlayer(plr, prev_clonk)
{
	var spawn_idx = Random(2);
	if (prev_clonk && g_statue) spawn_idx = (prev_clonk->GetX() > g_statue->GetX());
	var x=[494,763][spawn_idx],y = 360;
	var clonk = GetCrew(plr);
	if (clonk)
	{
		clonk->SetPosition(x,y-10);
	}
	else
	{
		clonk = CreateObjectAbove(Clonk, x,y, plr);
		clonk->MakeCrewMember(plr);
	}
	SetCursor(plr, clonk);
	clonk->DoEnergy(1000);
	// contents
	clonk.MaxContentsCount = 1;
	if (prev_clonk) TransferInventory(prev_clonk, clonk);
	if (!clonk->ContentsCount())
	{
		clonk->CreateContents(Bow);
		var arrow = CreateObjectAbove(Arrow);
		clonk->Collect(arrow);
		arrow->SetInfiniteStackCount();
	}
	clonk->~CrewSelection(); // force update HUD
	// Make this work under the friendly fire rule.
	for (var obj in [g_statue, g_doorleft, g_doorright])
		if (obj)
			obj->SetOwner(plr);
	return;
}

// Enter all buyable things into the homebase
func FillHomebase(object homebase)
{
	// Quick buy items on hotkeys
	homebase->SetQuickbuyItems([/*Hammer*/ nil, Bow, Sword, Blunderbuss, GrenadeLauncher, nil, Firestone, IronBomb, nil, nil]);

	// Buy menu entries
	homebase->AddCaption("$HomebaseWeapons$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = Bow,                  ammo = Arrow, desc = "$HomebaseDescBow$" });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = Sword,     cost = 25 });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Consumable { item = Firestone, cost = 5});
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = Blunderbuss,    cost = 50, ammo = LeadBullet, desc = "$HomebaseDescBlunderbuss$",     requirements = ["AdvancedWeapons"] });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Consumable { item = IronBomb,  cost = 15,                                             requirements = ["AdvancedWeapons"] });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Consumable { item = Lantern,cost = 15,                                            requirements = ["AdvancedWeapons"] });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = GrenadeLauncher, ammo = IronBomb, desc = "$HomebaseDescGrenadeLauncher$", requirements = ["MasterWeapons"] });

	homebase->AddCaption("$HomebaseItems$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Consumable { item = Bread,     cost = 5  });
	//homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon { item = Hammer,    cost = 1000, desc = "$HomebaseDescHammer$", extra_width = 1 });

	homebase->AddCaption("$HomebaseTechnology$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseAdvancedWeapons$", item = Icon_World,cost = 100, desc="$HomebaseDescAdvancedWeapons$", tech = "AdvancedWeapons" });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseMasterWeapons$", item = Icon_World,cost = 1000, desc = "$HomebaseDescMasterWeapons$", tech = "MasterWeapons", requirements = ["AdvancedWeapons"] });

	homebase->AddCaption("$HomebaseUpgrades$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseLoadSpeed$", item = Homebase_Icon, graphics="LoadSpeed%d", costs = [100, 500, 1000], desc = "$HomebaseDescLoadSpeed$", tech = "LoadSpeed", tiers=3 });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseLife$", item = Homebase_Icon, graphics="Life%d", costs = [10, 50, 100], desc = "$HomebaseDescLife$", tech = "Life", tiers=3 });
	homebase->AddCaption("$HomebaseArtifacts$");
}

func StartGame()
{
	// Init objects to defend
	for (var obj in [g_statue, g_doorleft, g_doorright]) if (obj)
	{
		obj->SetCategory(C4D_Living);
		obj->SetAlive(true);
		obj.MaxEnergy = 800000;
		obj->DoEnergy(obj.MaxEnergy/1000);
		obj->AddEnergyBar();
		GameCallEx("OnCreationRuleNoFF", obj);
	}
	if (g_statue)
	{
		g_statue->SetCategory(C4D_Living | C4D_StaticBack);
		g_statue.Death = Scenario.Statue_Death;
	}
	// Launch first wave!
	g_wave = 1;
	ScheduleCall(nil, Scenario.LaunchWave, 50, 1, g_wave);
	return true;
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
		Sound("UI::Ding");
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
	// Spawn on ground or in air?
	var xmin, xmax, y;
	if (enemy.Type == DefenseBoomAttack)
	{
		// Air spawn
		xmin = 0;
		xmax = 550;
		y = 5;
	}
	else
	{
		xmin = xmax = 0;
		y = 509;
	}
	// Spawn either only enemy or mirrored enemies on both sides
	var side = enemy.Side;
	if (!side) side = WAVE_SIDE_LEFT | WAVE_SIDE_RIGHT;
	if (side & WAVE_SIDE_LEFT)  ScheduleCall(nil, CustomAI.LaunchEnemy, Max(enemy.Interval,1), Max(enemy.Num,1), enemy, 10 + xmin, xmax - xmin, y);
	if (side & WAVE_SIDE_RIGHT) ScheduleCall(nil, CustomAI.LaunchEnemy, Max(enemy.Interval,1), Max(enemy.Num,1), enemy, 1190 - xmax, xmax - xmin, y);
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
	var bounty = ENEMY_WAVE_DATA[g_wave].Bounty, bounty_msg = "";
	if (bounty)
	{
		bounty_msg = Format("|<c ffff00>+%d</c>{{Icon_Wealth}}", bounty);
		DoWealthForAll(bounty);
	}
	CustomMessage(Format("$MsgWaveCleared$%s|                                                             ", wave, bounty_msg));
	Sound("UI::Ding");
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
	// Player died?
	if (!clonk) return;
	var plr = clonk->GetOwner();
	if (GetPlayerType(plr) == C4PT_User)
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
		JoinPlayer(plr, clonk);
		//var gui_arrow = FindObject(Find_ID(GUI_GoalArrow), Find_Owner(plr));
		//gui_arrow->SetAction("Show", GetCursor(plr));
	}
	else
	{
		// Enemy clonk death
		// Remove inventory
		var i = clonk->ContentsCount();
		while (i--)
		{
			var obj = clonk->Contents(i);
			if (obj && !obj->~OnContainerDeath())
				obj->RemoveObject();
		}
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
			else
			{
				// Killer could not be determined. Just give gold to everyone.
				DoSharedWealth(clonk.Bounty);
			}
		}
	}
	return;
}

public func DoSharedWealth(int amount)
{
	// Split gold among all players. Keep track of remainder and use it next time
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

public func DoWealthForAll(int amount)
{
	// Add wealth to all players
	for (var iplr = 0; iplr < GetPlayerCount(C4PT_User); ++iplr)
		DoWealth(GetPlayerByIndex(iplr, C4PT_User), amount);
	return true;
}



//======================================================================
/* Game end */

func OnAllWavesCleared()
{
	// Success!
	if (g_goal) g_goal.is_fulfilled = true;
	if (GetPlayerType(ENEMY) == C4PT_Script) EliminatePlayer(ENEMY);
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
	ScheduleCall(nil, Global.GameOver, 50, 1);
	return Explode(10);
}

/* Developer commands */

public func ShowWaveInfo()
{
	// Debug summary to balance waves
	var total_reward = 0, total_bonus = 0;
	for (var i = 1; i<GetLength(ENEMY_WAVE_DATA); ++i)
	{
		var wave = ENEMY_WAVE_DATA[i];
		var reward = wave.Bounty ?? 0;
		var bonus = 0;
		for (var enemy in wave.Enemies)
		{
			var numsides = 2;
			if (enemy.Side) numsides = 1;
			bonus += enemy.Bounty  * enemy.Num * numsides;
		}
		total_reward += reward;
		total_bonus += bonus;
		Log("[%02d] %04d + %04d  (= %05d + %05d = %05d) %s", i, reward, bonus, total_reward, total_bonus, total_reward + total_bonus, wave.Name);
	}
	return true;
}

private func GetWaveByName(string wave_name)
{
	var i = 0, imax = GetLength(ENEMY_WAVE_DATA);
	while (++i < imax) if (WildcardMatch(ENEMY_WAVE_DATA[i].Name, wave_name)) break;
	if (i == imax)
	{
		Log("No match for wave mask: %s", wave_name);
		i = 0;
	}
	return i;
}

public func SetNextWave(string wave_name, bool wait)
{
	// Find wave by wildcard
	var i_wave;
	if (!GetLength(wave_name))
		i_wave = (g_wave + 1) % GetLength(ENEMY_WAVE_DATA);
	else
		i_wave = GetWaveByName(wave_name);
	if (!i_wave) return false;
	// Clear any previous
	ClearScheduleCall(nil, Scenario.CheckWaveCleared);
	if (g_spawned_enemies)
		for (var enemy in g_spawned_enemies)
			if (enemy) enemy->RemoveObject();
	// Give gold for skipped waves
	var total_reward = 0, total_bonus = 0;
	for (var i=g_wave; i<i_wave; ++i)
	{
		var wave = ENEMY_WAVE_DATA[i];
		total_reward += wave.Bounty ?? 0;
		for (var enemy in wave.Enemies)
		{
			var numsides = 2;
			if (enemy.Side) numsides = 1;
			total_bonus += enemy.Bounty  * enemy.Num * numsides;
		}
	}
	DoWealthForAll(total_reward);
	DoSharedWealth(total_bonus);
	// Schedule next wave
	Log("Next wave: %s", ENEMY_WAVE_DATA[i_wave].Name);
	g_wave = i_wave;
	ScheduleCall(nil, Scenario.LaunchWave, 500 + wait * 2000, 1, g_wave);
}

public func GiveRandomAttackTarget(object attacker)
{
	return g_statue;
}

//======================================================================
/* Wave and enemy definitions */

static const CSKIN_Amazon = [CSKIN_Farmer, "Clonk_Amazon"],
             CSKIN_Ogre = [CSKIN_Alchemist, "Clonk_Ogre"];

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
	var chippie    = { Type=Chippie, Bounty=30 };
	var boomattack = { Type=DefenseBoomAttack, Bounty=10 };
	var boomattackf= { Type=DefenseBoomAttack, Bounty=25, Speed=300 };
	//newbie = runner;
	//newbie = runner;

	// Define composition of waves
	ENEMY_WAVE_DATA = [nil,
			{ Name = "$WaveNewbies$", Bounty = 10, Enemies = [
			new newbie   {            Num= 1, Interval=10, Side = WAVE_SIDE_LEFT }
	]}, { Name = "$WaveBows$", Bounty = 15, Enemies = [
			new newbie      {            Num= 2, Interval=10 },
			new bowman      { Delay= 30, Num= 3, Interval=10, Side = WAVE_SIDE_RIGHT },
			new amazon      { Delay= 30, Num= 3, Interval=10, Side = WAVE_SIDE_LEFT }
	]}, { Name = "Explosive", Bounty = 20, Enemies = [
			new flintstone  {            Num=10, Interval=20 }
	]}, { Name = "Boomattack", Bounty = 20, Enemies = [
			new boomattack  {            Num=10, Interval=70 }
	]}, { Name = "Suicidal", Bounty = 20, Enemies = [
			new suicide     {            Num= 2, Interval= 5 },
			new flintstone  { Delay= 15, Num= 5, Interval= 5 },
			new suicide     { Delay= 50, Num= 1 }
	]}, { Name = "Swordsmen", Bounty = 30, Enemies = [
			new swordman    {            Num=10, Interval=20 },
			new bigswordman { Delay=210, Num= 1, Interval=10 }
	]}, { Name = "Oh Shrek!", Bounty = 50, Enemies = [
			new ogre        {            Num= 2, Interval=20 },
			new swordogre   { Delay= 40, Num= 1, Interval=20 },
			new bowman      { Delay= 60, Num= 3, Interval= 5 }
	]}, { Name = "Heavy artillery incoming", Bounty = 50, Enemies = [
			new artillery   {            Num= 1              },
			new ogre        {            Num= 2, Interval=20 },
			new flintstone  { Delay= 15, Num= 5, Interval= 5 },
			new swordogre   { Delay= 60, Num= 1, Interval=20 }
	]}, { Name = "Fast rockets", Bounty = 50, Enemies = [
			new boomattackf {            Num=6, Interval=30 }
	]}, { Name = "Stop the big ones", Bounty = 75, Enemies = [
			new flintstone  {            Num=20, Interval=15 },
			new nukeogre    {            Num= 2, Interval=99 },
			new amazon      { Delay= 50, Num= 6, Interval=10 }
	]}, { Name = "Supreme Boomattack", Bounty = 100, Enemies = [
			new boomattack  {            Num=30, Interval=10 },
			new boomattackf {            Num=5, Interval=40 }
	]}, { Name = "Alien invasion", Bounty = 100, Enemies = [
			new bowman      { Delay=260, Num= 3, Interval= 5 },
			new chippie     {            Num=10, Interval=10 },
			new amazon      { Delay=250, Num= 6, Interval=10 }
	]}, { Name = "Two of each kind", Bounty = 250, Enemies = [
			new newbie      { Delay=  0                      },
			new ogre        { Delay=  0                      },
			new swordman    { Delay= 10                      },
			new amazon      { Delay= 12                      },
			new nukeogre    { Delay= 14                      },
			new boomattack  { Delay= 15                      },
			new swordogre   { Delay= 20                      },
			new artillery   { Delay= 21                      },
			new flintstone  { Delay= 22                      },
			new bigswordman { Delay= 40                      },
			new bowman      { Delay= 45                      },
			new suicide     { Delay= 30                      },
			new boomattackf { Delay= 50                      }
	]}, { Name = "Finale!", Bounty = 1000, Enemies = [
			new artillery   {            Num= 2, Interval= 5 },
			new newbie      {            Num=10, Interval=15 },
			new swordman    { Delay=  3, Num= 5, Interval=30 },
			new nukeogre    { Delay= 60, Num= 2, Interval=50 },
			new bigswordman { Delay=103, Num= 3, Interval=30 },
			new amazon      { Delay= 10, Num= 5, Interval=10 },
			new boomattack  { Delay= 40, Num=20, Interval=20 },
			new flintstone  {            Num=20, Interval=10 },
			new bowman      { Delay=  8, Num= 5, Interval=40 },
			new suicide     { Delay= 25, Num=10, Interval=20 },
			new ogre        { Delay= 30, Num= 3, Interval=20 },
			new swordogre   { Delay=  4, Num= 4, Interval=99 }
	]} ];
	return true;
}
