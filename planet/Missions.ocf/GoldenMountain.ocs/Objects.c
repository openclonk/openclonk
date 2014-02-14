/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Grass, 443, 199);
	CreateObject(Grass, 468, 197);
	CreateObject(Grass, 441, 202);

	CreateObject(Tree_Coniferous, 452, 200);
	CreateObject(Tree_Coniferous, 391, 200);
	CreateObject(Tree_Coniferous, 336, 202);
	CreateObject(Tree_Coniferous, 427, 199);
	CreateObject(Tree_Coniferous, 367, 201);

	CreateObject(SproutBerryBush, 59, 214);

	CreateObject(Trunk, 301, 202);

	var Rank0029 = CreateObject(Rank, 41, 169);
	Rank0029->SetR(18);
	Rank0029->SetPosition(41, 166);
	var Rank0030 = CreateObject(Rank, 322, 201);
	Rank0030->SetR(6);
	Rank0030->SetPosition(322, 198);

	CreateObject(SproutBerryBush, 350, 206);

	CreateObject(Rule_TeamAccount, 50, 50);

	CreateObject(Rule_BuyAtFlagpole, 50, 50);

	var Goal_SellGems0037 = CreateObject(Goal_SellGems, 50, 50);
	Goal_SellGems0037->SetTargetAmount(30);

	CreateObject(Rule_BuyAtFlagpole, 50, 50);

	var Goal_SellGems0136 = CreateObject(Goal_SellGems, 50, 50);
	Goal_SellGems0136->SetTargetAmount(30);

	var Chest0039 = CreateObject(Chest, 200, 469);
	var Chest0040 = CreateObject(Chest, 1192, 701);
	var Chest0041 = CreateObject(Chest, 1807, 515);
	var Chest0042 = CreateObject(Chest, 2360, 484);

	CreateObject(Rock, 335, 362);
	CreateObject(Rock, 283, 376);
	CreateObject(Rock, 464, 226);
	CreateObject(Rock, 31, 103);
	CreateObject(Rock, 481, 239);

	Chest0039->CreateContents(Wood);
	Chest0039->CreateContents(Wood);
	Chest0039->CreateContents(Wood);
	Chest0039->CreateContents(Wood);
	Chest0039->CreateContents(Wood);

	Chest0042->CreateContents(Loam);
	Chest0042->CreateContents(Loam);
	Chest0041->CreateContents(Loam);
	Chest0041->CreateContents(Loam);
	Chest0040->CreateContents(Loam);
	Chest0040->CreateContents(Loam);
	Chest0039->CreateContents(Loam);

	Chest0039->CreateContents(Dynamite);
	Chest0039->CreateContents(Dynamite);

	Chest0040->CreateContents(DynamiteBox);
	Chest0042->CreateContents(DynamiteBox);
	Chest0042->CreateContents(DynamiteBox);

	Chest0040->CreateContents(Bucket);
	Chest0041->CreateContents(Bucket);
	Chest0042->CreateContents(Bucket);

	Chest0041->CreateContents(WallKit);
	Chest0041->CreateContents(WallKit);
	Chest0042->CreateContents(WallKit);

	var Barrel0071 = CreateObject(Barrel, 34, 206);
	Barrel0071->SetR(-86);
	Barrel0071->SetPosition(34, 203);

	var Seaweed0073 = CreateObject(Seaweed, 53, 503);
	Seaweed0073->SetPhase(15);
	var Seaweed0076 = CreateObject(Seaweed, 123, 495);
	Seaweed0076->SetPhase(38);

	CreateObject(Mushroom, 74, 201);
	CreateObject(Mushroom, 14, 199);
	CreateObject(Mushroom, 351, 201);
	CreateObject(Mushroom, 388, 199);
	CreateObject(Mushroom, 419, 199);
	CreateObject(Mushroom, 400, 199);
	CreateObject(Mushroom, 291, 199);
	CreateObject(Mushroom, 376, 198);

	Chest0039->CreateContents(Bread);
	Chest0039->CreateContents(Bread);
	Chest0040->CreateContents(Bread);
	Chest0040->CreateContents(Bread);
	Chest0041->CreateContents(Bread);
	Chest0041->CreateContents(Bread);
	Chest0042->CreateContents(Bread);
	Chest0042->CreateContents(Bread);

	CreateObject(Firestone, 51, 85);
	CreateObject(Firestone, 168, 146);
	CreateObject(Firestone, 17, 261);
	CreateObject(Firestone, 394, 253);
	CreateObject(Firestone, 278, 240);
	CreateObject(Firestone, 143, 439);
	CreateObject(Firestone, 618, 225);
	CreateObject(Firestone, 653, 231);
	CreateObject(Firestone, 500, 245);
	CreateObject(Firestone, 633, 236);
	CreateObject(Firestone, 32, 277);
	CreateObject(Firestone, 32, 266);
	CreateObject(Firestone, 17, 238);
	CreateObject(Firestone, 153, 149);
	CreateObject(Firestone, 585, 241);
	return true;
}
