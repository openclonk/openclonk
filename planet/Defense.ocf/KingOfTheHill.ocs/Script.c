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
	GetRelaunchRule()->SetBaseRespawn(true);
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
	
	// Move crew to the island.
	var crew = GetCrew(plr);
	crew->SetPosition(128, 440);
	
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


/*-- Waves Control --*/

public func GetAttackWave(int nr)
{
	// The round starts with a short phase to prepare.
	if (nr == 1) 
		return new DefenseEnemy.BreakWave { Duration = 120 };
	
	// Attack positions.
	var pos_land = {X = LandscapeWidth(), Y = 760, Exact = true};
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

// The attackers should go for flagpoles.
public func GiveRandomAttackTarget(object attacker)
{
	var target = FindObject(Find_Category(C4D_Structure), Find_Func("IsFlagpole"), Find_Hostile(attacker->GetController()), Sort_Random());
	if (target)
		return target;
	target = FindObject(Find_OCF(OCF_CrewMember), Find_Hostile(attacker->GetController()), Sort_Distance());
	if (target)
		return target;
	return;
}

// Give some of the boom attacks a certain path to ensure the inside of the hill is attacked.
public func GetBoomAttackWaypoints(object boompack)
{
	if (!Random(3))
	{
		// Choose a path through the rock or on the side of the main flagpole.
		if (!Random(2))
			return [
				{X = 450 + Random(10), Y = 680 + Random(10)},
				{X = 440 + Random(10), Y = 700 + Random(10)},
				{X = 100 + Random(200), Y = 500 + Random(200)}
			];
		return [
			{X = 70 + Random(30), Y = 460 + Random(10)},
			{X = 70 + Random(30), Y = 490 + Random(10)},
			{X = 100 + Random(200), Y = 500 + Random(200)}
		];
	}
	return nil;
}
