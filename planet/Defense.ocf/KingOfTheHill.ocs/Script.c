/**
	King of the Hill
	Defend the hill from enemy waves.
	
	@author Maikel
*/

static init_defenders;
static g_enemy_plr;

protected func Initialize()
{
	CreateScriptPlayer("$PlayerAttackers$", nil, 2, CSPF_NoEliminationCheck);
	CreateObject(Goal_Defense);
	
	CreateObject(Rule_BuyAtFlagpole);
	CreateObject(Rule_BaseRespawn);
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_NoFriendlyFire);
	return;
}


/*-- Player Control --*/

protected func InitializePlayer(int plr)
{
	// Do script player.
	if (GetPlayerType(plr) != C4PT_User)
	{
		g_enemy_plr = plr;
		GetCrew(plr)->RemoveObject();
		return;
	}
	
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

public func GetEnemyPlayer() { return g_enemy_plr; }


/*-- Waves Control --*/

public func GetAttackWave(int nr)
{
	// The round starts with a short phase to prepare.
	if (nr == 1) return new DefenseWave.Break { Duration = 120 };
	
	// Attack positions.
	var pos_land = {X = LandscapeWidth(), Y = 740, Exact = true};
	var pos_sky = {X = LandscapeWidth() - 100, Y = 0};
	var pos_above = {X = 200, Y = 0};
	
	// Automatically build waves that become stronger.
	var wave = 
	{
		Name = "$MsgWave$",
		// Waves last shorter as the number increases.
		Duration = BoundBy(120 - nr / 3, 20, 120),
		Bounty = 100,
		Score = 100,
		Enemies = []
	};
	
	// Add enemy ground troups: swordsman, archer or spearman.
	if (nr % 3 == 0)
	{
		PushBack(wave.Enemies, new DefenseEnemy.Swordsman {
			Amount = BoundBy(nr - 1, 1, 20),
			Energy = BoundBy(20 + nr, 30, 100),
			Position = pos_land
		});
	}
	if (nr % 3 == 1)
	{
		PushBack(wave.Enemies, new DefenseEnemy.Archer {
			Amount = BoundBy(nr - 1, 1, 20),
			Energy = BoundBy(10 + nr, 20, 50),
			Position = pos_land
		});
	}
	if (nr % 3 == 2)
	{
		PushBack(wave.Enemies, new DefenseEnemy.Spearman {
			Amount = BoundBy(nr - 1, 1, 20),
			Energy = BoundBy(10 + nr, 20, 50),
			Position = pos_land
		});
	}
	// Add enemy: boom attack.
	PushBack(wave.Enemies, new DefenseEnemy.BoomAttack {
		Amount = BoundBy(nr - 1, 1, 20),
		Speed = BoundBy(80 + nr * 5, 100, 250),
		Position = pos_sky
	});
	// Add enemy: rocketeer.
	PushBack(wave.Enemies, new DefenseEnemy.Rocketeer {
		Amount = BoundBy(nr - 1, 1, 20),
		Inventory = [Bow, RandomElement([Arrow, FireArrow, BombArrow])],
		Position = pos_sky
	});
	return wave;
}

// The attackers should go for flagpoles.
public func GiveRandomAttackTarget(object attacker)
{
	return FindObject(Find_Category(C4D_Structure), Find_Func("IsFlagpole"), Find_Hostile(attacker->GetController()), Sort_Random());
}
