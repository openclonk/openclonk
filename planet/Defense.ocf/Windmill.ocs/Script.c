/*
	Guardians of the Windmills
	Authors: Randrian, Newton, Clonkonaut

	Defend the windmills against waves of enemies
*/


static g_wave; // index of current wave
static g_spawned_enemies;
static g_relaunchs; // array of relaunch counts
static g_scores; // array of player scores
static g_ai; // derived from AI; contains changes for this scenario
static g_lost; // True if all windmills are destroyed
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
	// Wealth shown at all time
	GUI_Controller->ShowWealth();
	// static variable init
	g_homebases = [];
	InitWaveData();
}

func InitializePlayer(int plr, int iX, int iY, object pBase, int iTeam)
{
	if (GetPlayerType(plr) != C4PT_User) return;

	if (g_lost) { EliminatePlayer(plr); return; } // no post-elimination join
	if (!g_relaunchs)
	{
		g_relaunchs = [];
		g_scores = [];
		Scoreboard->Init([{key = "relaunchs", title = Rule_Relaunch, sorted = true, desc = true, default = "", priority = 75},
	                    {key = "score", title = Nugget, sorted = true, desc = true, default = "0", priority = 100}]);
	}
	g_relaunchs[plr] = MAX_RELAUNCH;
	g_scores[plr] = 0;
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "relaunchs", g_relaunchs[plr]);
	Scoreboard->SetPlayerData(plr, "score", g_scores[plr]);

	CreateObject(Homebase, 0,0, plr);

	SetPlayerZoomByViewRange(plr, 1200, 0, PLRZOOM_LimitMax);

	//DoWealth(plr, 10000000);

	JoinPlayer(plr);
	if (!g_wave) StartGame();
}

func RemovePlayer(int plr)
{
	Scoreboard->SetPlayerData(plr, "relaunchs", Icon_Cancel);
	// Split player's wealth among the remaining players
	ScheduleCall(nil, Scenario.DoSharedWealth, 50, 1, GetWealth(plr));
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
	var x=991,y = 970;
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
	for (var obj in [g_windgen1, g_windgen2, g_windgen3, g_windmill])
		if (obj)
			obj->SetOwner(plr);
}

// Enter all buyable things into the homebase
func FillHomebase(object homebase)
{
	// Quick buy items on hotkeys
	homebase->SetQuickbuyItems([WindBag, Bow, Javelin, Blunderbuss, GrenadeLauncher, nil, nil, nil, nil, nil]);

	// Buy menu entries
	homebase->AddCaption("$HomebaseWeapons$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = Bow,                         ammo = Arrow,    desc = "$HomebaseDescBow$" });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = Javelin,         cost = 10,                   desc = "$HomebaseDescJavelin$"                                            , infinite = true});
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = Blunderbuss,          cost = 50,  ammo = LeadBullet, desc = "$HomebaseDescBlunderbuss$",          requirements = ["AdvancedWeapons"] });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = GrenadeLauncher,             ammo = IronBomb, desc = "$HomebaseDescGrenadeLauncher$", requirements = ["MasterWeapons"] });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon     { item = WindBag,         cost = 500,                  desc = "$HomebaseDescWindBag$", requirements = ["MasterWeapons"] });

	homebase->AddCaption("$HomebaseItems$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Consumable { item = Bread,     cost = 5  });
//	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Weapon { item = Hammer,    cost = 1000, desc = "$HomebaseDescHammer$", extra_width = 1 });

	homebase->AddCaption("$HomebaseTechnology$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseAdvancedWeapons$", item = Icon_World,cost = 100, desc="$HomebaseDescAdvancedWeapons$", tech = "AdvancedWeapons" });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseMasterWeapons$", item = Icon_World,cost = 1000, desc = "$HomebaseDescMasterWeapons$", tech = "MasterWeapons", requirements = ["AdvancedWeapons"] });

	homebase->AddCaption("$HomebaseUpgrades$");
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseLoadSpeed$", item = Homebase_Icon, graphics="LoadSpeed%d", costs = [100, 500, 1000], desc = "$HomebaseDescLoadSpeed$", tech = "LoadSpeed", tiers=3 });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseShootingStrength$", item = Homebase_Icon, graphics="ShootingStrength%d", costs = [50, 150, 350], desc = "$HomebaseDescShootingStrength$", tech = "ShootingStrength", tiers=3 });
	homebase->AddHomebaseItem(new Homebase.ITEMTYPE_Technology { name="$HomebaseLife$", item = Homebase_Icon, graphics="Life%d", costs = [10, 50, 100], desc = "$HomebaseDescLife$", tech = "Life", tiers=3 });
	homebase->AddCaption("$HomebaseArtifacts$");
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

//======================================================================
/* The Game */

func StartGame()
{
	// Init objects to defend
	var obj;
	for (obj in [g_windgen1, g_windgen2, g_windgen3, g_windmill]) if (obj)
	{
		obj->SetCategory(C4D_Living);
		obj->SetAlive(true);
		obj.MaxEnergy = 800000;
		obj->DoEnergy(obj.MaxEnergy/1000);
		obj->AddEnergyBar();
		GameCallEx("OnCreationRuleNoFF", obj);
	}
	// Launch first wave!
	g_wave = 1;
	ScheduleCall(nil, Scenario.LaunchWave, 50, 1, g_wave);
	return true;
}

public func WindmillDown(object windmill)
{
	if (g_windgen1 == windmill) g_windgen1 = nil;
	if (g_windgen2 == windmill) g_windgen2 = nil;
	if (g_windgen3 == windmill) g_windgen3 = nil;
	if (g_windmill == windmill) g_windmill = nil;

	Sound("Objects::Plane::PlaneCrash", true);

	// Nothing left to defend?
	if (!g_windgen1 && !g_windgen2 && !g_windgen3 && !g_windmill)
	{
		// Fail!
		var i=GetPlayerCount(C4PT_User);
		while (i--) EliminatePlayer(GetPlayerByIndex(i, C4PT_User));
		g_lost = true;
		ScheduleCall(nil, Global.GameOver, 50, 1);
	}
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
		if (wave_data.Chest != nil && g_chest)
		{
			var item = g_chest->CreateContents(wave_data.Chest.Item);
			if (item)
			{
				if (item->GetID() == GoldBar)
				{
					item->SetValue(wave_data.Chest.Value);
					g_chest->SetMeshMaterial("GoldenChest", 0);
				}
			}
		}
		for (var enemy in ForceVal2Array(wave_data.Enemies)) if (enemy)
		{
			if (enemy.Delay)
				ScheduleCall(nil, Scenario.ScheduleLaunchEnemy, enemy.Delay, 1, enemy);
			else
				ScheduleLaunchEnemy(enemy);
			wave_spawn_time = Max(wave_spawn_time, enemy.Delay + enemy.Interval * enemy.Num);
		}
		for (var arrow in ForceVal2Array(wave_data.Arrows))
		{
			CreateArrowForPlayers(arrow.X, arrow.Y);
		}
		ScheduleCall(nil, Scenario.LaunchWaveDone, wave_spawn_time+5, 1, wave);
		return true;
	}
	return false;
}

func CreateArrowForPlayers(int x, int y)
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); i++)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var cursor = GetCursor(plr);
		if (!cursor) continue;
		var arrow = CreateObject(GUI_GoalArrow, cursor->GetX(), cursor->GetY(), plr);
		if (!arrow) continue;
		arrow->SetAction("Show", cursor);
		arrow->SetR(Angle(cursor->GetX(), cursor->GetY(), x, y));
		arrow->SetClrModulation(RGBa(255, 50, 0, 128));
		Schedule(arrow, "RemoveObject()", 100);
	}
}

func ScheduleLaunchEnemy(proplist enemy)
{
	// Schedules spawning of enemy definition
	// Spawn on ground or in air?
	var xmin, xmax, ymin, ymax;
	var def = enemy.Type ?? enemy.Vehicle;
	if (!def) def = Clonk;
	var width = def->GetDefWidth();
	var height = def->GetDefWidth();

	xmin = BoundBy(enemy.PosX - 100, 0 + width/2, LandscapeWidth() - width/2);
	xmax = BoundBy(enemy.PosX + 100, 0 + width/2, LandscapeWidth() - width/2);
	ymin = BoundBy(enemy.PosY - 100, 0 + height/2, LandscapeHeight() - height/2);
	ymax = BoundBy(enemy.PosY + 100, 0 + height/2, LandscapeHeight() - height/2);

	ScheduleCall(nil, CustomAI.LaunchEnemy, Max(enemy.Interval,1), Max(enemy.Num,1), enemy, xmin, xmax - xmin, ymin, ymax - ymin);
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
		bounty = bounty * 4 / BoundBy(GetPlayerCount(C4PT_User), 1, 4); // Carefully tested balancing
		bounty_msg = Format("|<c ffff00>+%d</c>{{Icon_Wealth}}", bounty);
		DoWealthForAll(bounty);
	}
	CustomMessage(Format("$MsgWaveCleared$%s|                                                             ", wave, bounty_msg));
	Sound("UI::NextWave");
	// Fade out stuff
	Airship->AllStop();
	if (g_object_fade)
		for (var obj in FindObjects(Find_Or(Find_And(Find_ID(Clonk), Find_Not(Find_OCF(OCF_Alive))), Find_ID(Catapult), Find_ID(Airship))))
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

public func GiveRandomAttackTarget(object attacker)
{
	return GetRandomWindmill();
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

//======================================================================
/* Wave and enemy definitions */

static ENEMY_WAVE_DATA;

static const g_respawning_weapons = [Firestone, Rock];

func InitWaveData()
{
	// Define different enemy types
	var pilot       = { Name="$EnemyPilot$",     Inventory=Rock,          Energy=30, Bounty=20, Color=0xff0000ff, Skin=CSKIN_Alchemist, Backpack=0, Vehicle=Airship };
	var swordman    = { Name="$EnemyCrewman$",   Inventory=Sword,         Energy=50, Bounty=15, Color=0xffff0000, Skin=CSKIN_Default,   Backpack=0,                    IsCrew=true };
	var defender    = { Name="$EnemyDefender$",  Inventory=[Shield, Axe], Energy=50, Bounty=10, Color=0xff00ff00, Skin=CSKIN_Farmer,    Backpack=0,                    IsCrew=true };
	var bowman      = { Name="$EnemyBow$",       Inventory=[Bow, Arrow],  Energy=30, Bounty=10, Color=0xff80ff80, Skin=CSKIN_Steampunk, Backpack=0,                    IsCrew=true };
	var artillery   = { Name="$EnemyArtillery$", Inventory=Firestone,     Energy=10, Bounty=25, Color=0xffffff80, Skin=CSKIN_Steampunk, Backpack=0, Vehicle=Catapult,  IsCrew=true };
	var ballooner   = { Name="$EnemyBalloon$",   Inventory=Sword,         Energy=30, Bounty=15, Color=0xff008000, Skin=CSKIN_Default,               Vehicle=Balloon };
	var rocketeer   = { Name="$EnemyRocket$",    Inventory=[Bow, Arrow],  Energy=15, Bounty=15, Color=0xffffffff, Skin=CSKIN_Steampunk,             Vehicle=DefenseBoomAttack };
	var boomattack  = { Type=DefenseBoomAttack, Bounty=2 };
	var boomattackf = { Type=DefenseBoomAttack, Bounty=15, Speed=300 };

	// Define composition of waves
	ENEMY_WAVE_DATA = [nil,
		{ Name = "$WaveFirst$", Bounty = 1, Enemies = 
			new boomattack   {  Num= 1, Interval=10, PosX = 0, PosY = 500 },
			Arrows = { X = 0, Y = 500 },
			Chest = { Item = GoldBar, Value = 25 }
		}, { Name = "$WaveSecond$", Bounty = 30, Enemies = 
			[new boomattack  {  Num= 3, Interval=10, PosX = 0,    PosY = 500 },
			new boomattack   {  Num= 3, Interval=10, PosX = 2000, PosY = 500 },],
			Arrows = [{ X = 0, Y = 500 },{ X = 2000, Y = 500 }]
		}, { Name = "$WaveThird$", Bounty = 10, Enemies = 
			new rocketeer    {  Num= 8, PosX = 0, PosY = 500 },
			Arrows = { X = 0, Y = 500 }
		}, { Name = "$WaveFourth$", Bounty = 15, Enemies = 
			[new rocketeer   {  Num= 8, Interval=10, PosX = 0,    PosY = 500 },
			new rocketeer    {  Num= 8, Interval=10, PosX = 2000, PosY = 500 },],
			Arrows = [{ X = 0, Y = 500 },{ X = 2000, Y = 500 }]
		}, { Name = "$WaveFifth$", Bounty = 20, Enemies = 
			[new boomattack  {  Num= 10,           PosX = 1000, PosY = 2000 },
			new pilot        {  Num= 1, Delay = 1, PosX = 2000, PosY = 750  },
			new defender     {  Num= 1, Delay = 2, PosX = 2000, PosY = 750  },
			new pilot        {  Num= 1, Delay = 3, PosX = 0,    PosY = 750  },
			new defender     {  Num= 1, Delay = 4, PosX = 0,    PosY = 750  },],
			Arrows = [{ X = 0, Y = 750 },{ X = 2000, Y = 750 },{ X = 1000, Y = 2000 }]
		}, { Name = "$WaveSixth$", Bounty = 20, Enemies = 
			[new pilot       {  Num= 1, Delay = 1, PosX = 2000, PosY = 1250 },
			new defender     {  Num= 2, Delay = 2, PosX = 2000, PosY = 1250 },
			new bowman       {  Num= 2, Delay = 2, PosX = 2000, PosY = 1250 },
			new swordman     {  Num= 1, Delay = 2, PosX = 2000, PosY = 1250 },
			new pilot        {  Num= 1, Delay = 3, PosX = 0,    PosY = 1250 },
			new defender     {  Num= 2, Delay = 4, PosX = 0,    PosY = 1250 },
			new bowman       {  Num= 2, Delay = 4, PosX = 0,    PosY = 1250 },
			new swordman     {  Num= 1, Delay = 4, PosX = 0,    PosY = 1250 },],
			Arrows = [{ X = 0, Y = 1250 },{ X = 2000, Y = 1250 }],
			Chest = { Item = GoldBar, Value = 100 }
		}, { Name = "$WaveSeventh$", Bounty = 50, Enemies = 
			new ballooner    {  Num= 10, PosX = 1000, PosY = 0 },
			Arrows = { X = 1000, Y = 0 }
		}, { Name = "$WaveEighth$", Bounty = 50, Enemies = 
			[new boomattack  {  Num= 15, Interval =  5,             PosX = 500,  PosY =    0 },
			new pilot        {  Num=  1,                Delay = 80, PosX = 0,    PosY = 1250 },
			new defender     {  Num=  3,                Delay = 81, PosX = 0,    PosY = 1250 },
			new bowman       {  Num=  3,                Delay = 81, PosX = 0,    PosY = 1250 },
			new pilot        {  Num=  1,                Delay = 82, PosX = 2000, PosY = 1250 },
			new defender     {  Num=  3,                Delay = 83, PosX = 2000, PosY = 1250 },
			new bowman       {  Num=  3,                Delay = 83, PosX = 2000, PosY = 1250 },
			new pilot        {  Num=  1,                Delay = 84, PosX = 200,  PosY = 2000 },
			new defender     {  Num=  3,                Delay = 85, PosX = 200,  PosY = 2000 },
			new swordman     {  Num=  3,                Delay = 85, PosX = 200,  PosY = 2000 },
			new pilot        {  Num=  1,                Delay = 86, PosX = 1800, PosY = 2000 },
			new defender     {  Num=  3,                Delay = 87, PosX = 1800, PosY = 2000 },
			new swordman     {  Num=  3,                Delay = 87, PosX = 1800, PosY = 2000 },],
			Arrows = [{ X = 500, Y = 0 },{ X = 0, Y = 1250 },{ X = 2000, Y = 1250 },{ X = 200, Y = 2000 },{ X = 1800, Y = 2000 }]
		}, { Name = "$WaveNinth$", Bounty = 100, Enemies = 
			[new ballooner   {  Num= 10, Interval = 10, Delay = 350, PosX = 1000, PosY =   0 },
			new boomattackf  {  Num= 8,  Interval =  1,              PosX = 0,    PosY = 300 },
			new boomattackf  {  Num= 8,  Interval =  1,              PosX = 2000, PosY = 300 },],
			Arrows = [{ X = 1000, Y = 0 },{ X = 0, Y = 300 },{ X = 2000, Y = 300 }]
		}, { Name = "$WaveTenth$", Bounty = 1000, Enemies = 
			[new boomattack  {  Num=  7, Interval =  1,              PosX = 0,    PosY =    0 },
			new boomattack   {  Num=  7, Interval =  1,              PosX = 2000, PosY =    0 },
			new rocketeer    {  Num=  4, Interval = 10,              PosX = 0,    PosY =  300 },
			new rocketeer    {  Num=  4, Interval = 10,              PosX = 2000, PosY =  300 },
			new ballooner    {  Num= 10, Interval =  5, Delay = 100, PosX = 1000, PosY =    0 },
			new pilot        {  Num=  1,                Delay =  80, PosX = 0,    PosY = 1250 },
			new defender     {  Num=  3,                Delay =  81, PosX = 0,    PosY = 1250 },
			new bowman       {  Num=  4,                Delay =  81, PosX = 0,    PosY = 1250 },
			new pilot        {  Num=  1,                Delay =  82, PosX = 2000, PosY = 1250 },
			new defender     {  Num=  3,                Delay =  83, PosX = 2000, PosY = 1250 },
			new bowman       {  Num=  4,                Delay =  83, PosX = 2000, PosY = 1250 },
			new pilot        {  Num=  1,                Delay =  84, PosX = 200,  PosY = 2000 },
			new defender     {  Num=  3,                Delay =  85, PosX = 200,  PosY = 2000 },
			new swordman     {  Num=  4,                Delay =  85, PosX = 200,  PosY = 2000 },
			new pilot        {  Num=  1,                Delay =  86, PosX = 1800, PosY = 2000 },
			new defender     {  Num=  3,                Delay =  87, PosX = 1800, PosY = 2000 },
			new swordman     {  Num=  4,                Delay =  87, PosX = 1800, PosY = 2000 },
			new pilot        {  Num=  1,                Delay =  88, PosX = 880,  PosY = 2000 },
			new bowman       {  Num=  3,                Delay =  89, PosX = 880,  PosY = 2000 },
			new swordman     {  Num=  3,                Delay =  89, PosX = 880,  PosY = 2000 },
			new pilot        {  Num=  1,                Delay =  88, PosX = 1120, PosY = 2000 },
			new bowman       {  Num=  3,                Delay =  89, PosX = 1120, PosY = 2000 },
			new swordman     {  Num=  3,                Delay =  89, PosX = 1120, PosY = 2000 },],
			Arrows = [{ X = 0, Y = 0 },{ X = 2000, Y = 0 },{ X = 0, Y = 300 },{ X = 2000, Y = 300 },{ X = 1000, Y = 0 },{ X = 0, Y = 1250 },{ X = 2000, Y = 1250 },{ X = 200, Y = 2000 },{ X = 1800, Y = 2000 },{ X = 880, Y = 2000 },{ X = 1120, Y = 2000 }]
		}];
	return true;
}
