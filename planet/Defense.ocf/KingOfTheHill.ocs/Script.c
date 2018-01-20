/**
	King of the Hill
	Defend the hill from enemy waves.
	
	@author Maikel
*/

static init_defenders;


protected func Initialize()
{
	// Defense goal.
	CreateObject(Goal_Defense);
	
	// Rules.
	CreateObject(Rule_BuyAtFlagpole);
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetBaseRespawn(true);
	relaunch_rule->SetFreeCrew(false);
	relaunch_rule->SetLastClonkRespawn(true);
	relaunch_rule->SetInitialRelaunch(false);
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_NoFriendlyFire);
	CreateObject(Rule_Gravestones)->SetFadeOut(3 * 36);
	return;
}


/*-- Player Control --*/

protected func InitializePlayer(int plr)
{
	if (GetPlayerType(plr) == C4PT_Script)
		return;
	
	// Move players to defenders team.
	if (GetPlayerTeam(plr) != 1)
		SetPlayerTeam(plr, 1);
	
	// Move crew to the initial position.
	var crew = GetCrew(plr);
	crew->SetPosition(120 + Random(16), 440);
	
	// Set zoom ranges.
	SetPlayerZoomByViewRange(plr, 1200, nil, PLRZOOM_LimitMax);
	SetPlayerZoomByViewRange(plr, 450, nil, PLRZOOM_Direct);
	
	// Base material and knowledge.
	GivePlayerBaseMaterial(plr);
	var index = 0, def;
	while (def = GetDefinition(index++))
		SetPlrKnowledge(plr, def);
	
	// Give base and set wealth.
	if (!init_defenders)
	{
		SetWealth(plr, 200);
		for (var obj in FindObjects(Find_Func("IsFlagpole")))
			obj->SetOwner(plr);
		init_defenders = true;
	}
	return;
}


/*-- Scenario Control --*/

public func OnWaveStarted(int wave_nr)
{
	// Fade out enemy ammunition.
	var enemy_plr = Goal_Defense->GetEnemyPlayer();
	for (var obj in FindObjects(Find_Func("IsArrow"), Find_NoContainer()))
		if (obj->GetController() == enemy_plr)
			obj->AddEffect("IntFadeOut", obj, 100, 1, nil, Rule_ObjectFade);
	return;
}


/*-- Waves Control --*/

public func GetAttackWave(int nr)
{
	// The round starts with a short phase to prepare.
	if (nr == 1) 
		return new DefenseEnemy.BreakWave { Duration = 120 };
	
	// Attack positions.
	var pos_land = {X = LandscapeWidth(), Y = 756, Exact = true};
	var pos_sky = {X = LandscapeWidth() - 100, Y = 0};
	var pos_above = {X = 200, Y = 0};
	
	// Automatically build waves that become stronger.
	var wave = new DefenseEnemy.DefaultWave
	{
		Name = "$MsgWave$",
		// Waves last shorter as the number increases.
		Duration = BoundBy(120 - nr / 3, 20, 120),
		Bounty = 50,
		Score = 50,
		Enemies = []
	};
		
	// Add enemy ground troups: swordsman, archer, spearman, grenadier, bomber.
	PushBack(wave.Enemies, new DefenseEnemy.Swordsman {
		Amount = BoundBy((nr + 2) / 5, 0, 20),
		Energy = BoundBy(20 + nr, 30, 100),
		Position = pos_land
	});
	PushBack(wave.Enemies, new DefenseEnemy.Archer {
		Amount = BoundBy((nr + 1) / 5, 0, 20),
		Energy = BoundBy(10 + nr, 20, 50),
		Position = pos_land
	});
	PushBack(wave.Enemies, new DefenseEnemy.Spearman {
		Amount = BoundBy(nr / 5, 0, 20),
		Energy = BoundBy(10 + nr, 20, 50),
		Position = pos_land
	});
	PushBack(wave.Enemies, new DefenseEnemy.Grenadier {
		Amount = BoundBy((nr - 1) / 5, 0, 20),
		Energy = BoundBy(25 + nr, 30, 80),
		Position = pos_land
	});
	PushBack(wave.Enemies, new DefenseEnemy.Bomber {
		Amount = BoundBy((nr - 2) / 5, 0, 20),
		Energy = BoundBy(10 + nr, 20, 50),
		Position = pos_land
	});	
	// Add enemy: boom attack.
	PushBack(wave.Enemies, new DefenseEnemy.BoomAttack {
		Amount = BoundBy(nr / 2 + 1, 1, 20),
		Speed = BoundBy(80 + nr * 5, 100, 250),
		Position = pos_sky
	});
	// Add enemy: rocketeer with bow.
	PushBack(wave.Enemies, new DefenseEnemy.Rocketeer {
		Amount = BoundBy(nr / 2, 1, 20),
		Inventory = [Bow, RandomElement([Arrow, FireArrow, BombArrow])],
		Position = pos_sky
	});
	// Add enemy: rocketeer with blunderbuss.
	if (nr > 4)
	{
		PushBack(wave.Enemies, new DefenseEnemy.Rocketeer {
			Amount = BoundBy(nr / 2 - 2, 1, 20),
			Inventory = [Blunderbuss, LeadBullet],
			Position = pos_sky
		});
	}
	return wave;
}

// The attackers should go for flagpoles, then crewmembers, and then hostile structures.
public func GiveRandomAttackTarget(object attacker)
{
	var controller = attacker->GetController();
	var target = FindObject(Find_ID(Flagpole), Find_Hostile(controller), Sort_Random());
	if (target)
		return target;
	target = FindObject(Find_OCF(OCF_CrewMember), Find_Hostile(controller), Sort_Distance());
	if (target)
		return target;
	var target = FindObject(Find_Category(C4D_Structure), Find_Hostile(controller), Sort_Random());
	if (target)
		return target;
	return;
}

// Returns what boom attacks should go for while on their respective paths.
public func GiveAttackTargetOnWaypointPath(object boomattack)
{
	var controller = boomattack->GetController();
	var target = boomattack->FindObject(Find_ID(Flagpole), Find_Hostile(controller), Find_Distance(100), boomattack->Find_PathFree(), Sort_Distance());
	if (target)
		return target;
	target = boomattack->FindObject(Find_OCF(OCF_CrewMember), Find_Hostile(controller), Find_Distance(100), boomattack->Find_PathFree(), Sort_Distance());
	if (target)
		return target;
	var target = boomattack->FindObject(Find_Category(C4D_Structure), Find_Hostile(controller), Find_Distance(100), boomattack->Find_PathFree(), Sort_Distance());
	if (target)
		return target;		
	return;
}

// Give some of the boom attacks a certain path to ensure the inside of the hill is attacked.
public func GetBoomAttackWaypoints(object boompack)
{
	// Construct three different paths which all pass through the inside of the hill and cover a large area.
	// Upper path through the upper entrance and then straight down and finally into the ruby mine.
	if (!Random(3))
		return [
			{X = 360 + Random(40), Y = 80 + Random(280)},
			{X = 75 + Random(20), Y = 440 + Random(20)},
			{X = 60 + Random(100), Y = 720 + Random(120)},
			{X = 395 + Random(5), Y = 845 + Random(5)},			
			{X = 465 + Random(5), Y = 885 + Random(5)},			
			{X = 500 + Random(180), Y = 850 + Random(100)}
		];
	// Middle path through the upper entrance and then diagonally down into the ruby mine.
	if (!Random(2))
		return [
			{X = 500 + Random(100), Y = 350 + Random(100)},
			{X = 200 + Random(50), Y = 400 + Random(20)},
			{X = 75 + Random(20), Y = 440 + Random(20)},
			{X = 280 + Random(40), Y = 680 + Random(60)},
			{X = 395 + Random(5), Y = 845 + Random(5)},			
			{X = 465 + Random(5), Y = 885 + Random(5)},			
			{X = 500 + Random(180), Y = 850 + Random(100)}
		];
	// Lower path throught the lower entrance and then into the ruby mine.
	return [
		{X = 660 + Random(240), Y = 580 + Random(80)},
		{X = 480 + Random(10), Y = 650 + Random(10)},
		{X = 455 + Random(5), Y = 680 + Random(10)},
		{X = 440 + Random(10), Y = 700 + Random(10)},
		{X = 150 + Random(100), Y = 640 + Random(120)},
		{X = 395 + Random(5), Y = 845 + Random(5)},			
		{X = 465 + Random(5), Y = 885 + Random(5)},			
		{X = 500 + Random(180), Y = 850 + Random(100)}
	];
}
