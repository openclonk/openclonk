
/*

	I am Rich
	
	@author: K-Pone
	some parts reused from Maikel

*/


static g_timeover;
static g_goal;

local COL_Score = 1;

local updatescoreboardfx = new Effect
{
	Timer = func()
	{
		GameCall("UpdateScoreboard");
	},
};

func Initialize()
{
	Tree_Coniferous->Place(12);
	Tree_Coniferous2->Place(8);
	Tree_Coniferous3->Place(5);
	Tree_Coniferous4->Place(3);
	Tree_Deciduous->Place(4);
	Grass->Place(80);
	Flower->Place(20);
	Cotton->Place(20);
	Wheat->Place(50);
	
	PlaceObjects(Rock, 75 + 10 * 2 + Random(10),"Earth");
	PlaceObjects(Firestone, 60 + 10 * 2 + Random(5), "Earth");
	PlaceObjects(Loam, 90 + 10 * 2 + Random(5), "Earth");
	PlaceObjects(Ore, 60 + 10 * 2 + Random(5), "Earth");
	PlaceObjects(Wood, 40 + 10 * 2 + Random(5), "Earth");
	PlaceObjects(Coal, 70 + 10 * 2 + Random(5), "Earth");
	PlaceObjects(Metal, 35 + 10 * 2 + Random(5), "Earth");
	
	g_timeover = false;
	
	g_goal = CreateObject(Goal_BeRich);
	
	CreateObject(Rule_BuyAtFlagpole);
	GetRelaunchRule()
		->SetBaseRespawn(true)
		->SetLastClonkRespawn(true);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_TeamAccount);
	
	if (SCENPAR_NoEnergy == 1) CreateObject(Rule_NoPowerNeed);
	
	CreateEffect(updatescoreboardfx, 1, 35);
	
	CreateObject(PlayerStart)->SetStartingCrew([{id = Clonk, count = 2}])
	                         ->SetStartingBaseMaterial([])
	                         ->SetStartingMaterial([])
                             ->SetStartingKnowledge();
}

func InitializePlayer(int plr)
{
	var crew = GetCrew(plr);
	crew->CreateContents(Shovel);
	crew->CreateContents(Hammer);
	crew->CreateContents(Axe);
	
	var flagpole = CreateFlagpole(crew);
	if (flagpole && ObjectDistance(crew, flagpole) > 100)
	{
		var x = flagpole->GetX();
		var y = flagpole->GetY();
		for (var i = 0; i < GetCrewCount(plr); ++i)
		{
			GetCrew(plr, i)->SetPosition(x, y);
		}
	}

	GivePlayerAllKnowledge(plr);
	BaseMats(plr);
	
	DoWealth(plr, SCENPAR_StartGold);
	
	SetScoreboardData(SBRD_Caption, SBRD_Caption,  "Player", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, COL_Score,     "{{Nugget}}");
	
	SetScoreboardData(plr,     SBRD_Caption,  GetTaggedPlayerName(plr));
	SetScoreboardData(plr,     COL_Score,     "0", 0);
	
	UpdateScoreboard();
}

func UpdateScoreboard()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var playerid = GetPlayerByIndex(i);
		var pscore = GetWealth(GetPlayerByIndex(i));
		SetScoreboardData(playerid, COL_Score, Format("%d", pscore), pscore);
	}	
}

func CreateFlagpole(object near_clonk)
{
	var xy = FindConstructionSite(Flagpole, near_clonk->GetX(), near_clonk->GetY());
	var flagpole;
	if (xy)
	{
		flagpole = CreateConstruction(Flagpole, xy[0], xy[1], near_clonk->GetOwner(), 100, true);
	}
	if (!flagpole)
	{
		var type = Flagpole;
		var area = Loc_InRect(near_clonk->GetX() - 50, near_clonk->GetY() - 50, 100, 100);
		var left = type->GetDefOffset(0);
		var space_top = Loc_Space(type->GetDefHeight(), CNAT_Top);
		var space_l = Loc_Space(Abs(left), CNAT_Left);
		var space_r = Loc_Space(left + type->GetDefWidth(), CNAT_Right);
		var on_surface = Loc_Wall(CNAT_Bottom, Loc_Not(Loc_Liquid()));
		
		// Find object near the desired area first, with less restrictions
		var spot = FindLocation(on_surface, space_l, space_r, space_top, area);
		spot = spot ?? FindLocation(on_surface, space_top, area);
		spot = spot ?? FindLocation(on_surface, area);
		
		// Same again, but on the whole landscape
		if (area != nil)
		{
			spot = FindLocation(on_surface, space_l, space_r, space_top);
			spot = spot ?? FindLocation(on_surface, space_top);
			spot = spot ?? FindLocation(on_surface);
		}
	
		if (spot)
		{
			flagpole = CreateConstruction(Flagpole, spot.x, spot.y, near_clonk->GetOwner(), 100, true);
		}
	}
	return flagpole;
}

func OnClonkDeath(object clonk, int killed_by)
{
	var w1 = GetWealth(clonk->GetOwner()) / 10;
	DoWealth(clonk->GetOwner(), w1 * -1);
	DoWealth(killed_by, w1);
}

func OnCountdownFinished()
{
	g_goal->OnlyRichSurvives();
	g_timeover = true;
}

func BaseMats(int plr)
{
	var materials;
	
	if (SCENPAR_BO_Clonks == 1)
	{
		materials = 
		[
			[Clonk,       10,   2]
		];
		
		GivePlayerBaseMaterial(plr, materials);
	}
	
	if (SCENPAR_BO_BuildingMaterial == 1)
	{
		materials = 
		[
			[Wood,       25,   2],
			[Metal,      20,   2],
			[Rock,       25,   2],
			[Loam,       10,   2]
		];
		
		GivePlayerBaseMaterial(plr, materials);
	}
	
	if (SCENPAR_BO_Tools == 1)
	{
		materials = 
		[
			[Shovel,      2,   2],
			[Hammer,      2,   2],
			[Pickaxe,     2,   2],
			[Sickle,      2,   2],
			[Axe,         2,   2],
			[WindBag,     2,   2],
			[GrappleBow,  2,   2],
			[TeleGlove,   2,   2],
			[WallKit,     2,   2],
			[Ropeladder,  2,   2],
			[Bucket,      2,   2],
			[Barrel,      2,   2]
		];
		
		GivePlayerBaseMaterial(plr, materials);
	}
	
	if (SCENPAR_BO_Weapons == 1)
	{
		materials = 
		[
			[Sword,       2,   2],
			[Club,        2,   2],
			[Axe,         2,   2],
			[Bow,         2,   2],
			[Arrow,       5,   2],
			[FireArrow,   2,   2],
			[BombArrow,   2,   2],
			[Blunderbuss, 2,   2],
			[LeadBullet,  2,   2],
			[GrenadeLauncher, 2, 2],
			[IronBomb,    5,   2],
			[Dynamite,   10,   2],
			[DynamiteBox, 2,   2],
			[Javelin,     2,   2],
			[SmokeBomb,   2,   2],
			[Lantern,     2,   2],
			[Shield,      2,   2],
			[Helmet,      2,   2],
			[Cannon,      2,   2],
			[Catapult,    2,   2],
			[Boompack,    5,   2],
			[PowderKeg,   5,   2]
		];
		
		GivePlayerBaseMaterial(plr, materials);
	}
	
	if (SCENPAR_BO_Food == 1)
	{
		materials = 
		[
			[Bread,      10,   2],
			//[Mushroom,   20,   2], // Take out mushrooms because they have a value of 0. They can be bought for nothing.
			//[CookedMushroom, 20, 2],
			[Flour,      10,   2],
			[Sproutberry, 10,   2]
		];
		
		GivePlayerBaseMaterial(plr, materials);
	}
}