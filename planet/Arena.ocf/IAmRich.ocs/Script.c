
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
}

func InitializePlayer(int plr)
{
	var crew = GetCrew(plr);
	crew->CreateContents(Shovel);
	crew->CreateContents(Hammer);
	crew->CreateContents(Axe);
	
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
		SetScoreboardData(playerid, COL_Score, Format("%i", pscore), pscore);
	}	
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
		
		for (var mat in materials)
		{
			SetBaseMaterial(plr, mat[0], mat[1]);
			SetBaseProduction(plr, mat[0], mat[2]);
		}
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
		
		for (var mat in materials)
		{
			SetBaseMaterial(plr, mat[0], mat[1]);
			SetBaseProduction(plr, mat[0], mat[2]);
		}
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
		
		for (var mat in materials)
		{
			SetBaseMaterial(plr, mat[0], mat[1]);
			SetBaseProduction(plr, mat[0], mat[2]);
		}
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
		
		for (var mat in materials)
		{
			SetBaseMaterial(plr, mat[0], mat[1]);
			SetBaseProduction(plr, mat[0], mat[2]);
		}
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
		
		for (var mat in materials)
		{
			SetBaseMaterial(plr, mat[0], mat[1]);
			SetBaseProduction(plr, mat[0], mat[2]);
		}
	}
}