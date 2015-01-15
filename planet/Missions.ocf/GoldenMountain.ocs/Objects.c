/* Automatically created objects file */

func InitializeObjects()
{
	CreateObjectAbove(Grass, 443, 199);
	CreateObjectAbove(Grass, 468, 197);
	CreateObjectAbove(Grass, 441, 202);

	CreateObjectAbove(Tree_Coniferous, 452, 200);
	CreateObjectAbove(Tree_Coniferous, 391, 200);
	CreateObjectAbove(Tree_Coniferous, 336, 202);
	CreateObjectAbove(Tree_Coniferous, 427, 199);
	CreateObjectAbove(Tree_Coniferous, 367, 201);

	CreateObjectAbove(SproutBerryBush, 59, 214);

	CreateObjectAbove(Trunk, 301, 202);

	var Branch0029 = CreateObjectAbove(Branch, 41, 169);
	Branch0029->SetR(188);
	Branch0029->SetPosition(41, 166);
	var Branch0030 = CreateObjectAbove(Branch, 322, 195);
	Branch0030->SetR(6);
	Branch0030->SetPosition(322, 192);

	CreateObjectAbove(SproutBerryBush, 350, 206);

	var Chest0039 = CreateObjectAbove(Chest, 200, 469);
	var Chest0040 = CreateObjectAbove(Chest, 1192, 701);
	var Chest0041 = CreateObjectAbove(Chest, 1807, 515);
	var Chest0042 = CreateObjectAbove(Chest, 2360, 484);

	CreateObjectAbove(Rock, 335, 362);
	CreateObjectAbove(Rock, 283, 376);
	CreateObjectAbove(Rock, 464, 226);
	CreateObjectAbove(Rock, 31, 103);
	CreateObjectAbove(Rock, 481, 239);

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

	var Barrel0071 = CreateObjectAbove(Barrel, 34, 206);
	Barrel0071->SetR(-86);
	Barrel0071->SetPosition(34, 203);

	var Seaweed0073 = CreateObjectAbove(Seaweed, 53, 503);
	Seaweed0073->SetPhase(15);
	var Seaweed0076 = CreateObjectAbove(Seaweed, 123, 495);
	Seaweed0076->SetPhase(38);

	CreateObjectAbove(Mushroom, 74, 201);
	CreateObjectAbove(Mushroom, 14, 199);
	CreateObjectAbove(Mushroom, 351, 201);
	CreateObjectAbove(Mushroom, 388, 199);
	CreateObjectAbove(Mushroom, 419, 199);
	CreateObjectAbove(Mushroom, 400, 199);
	CreateObjectAbove(Mushroom, 291, 199);
	CreateObjectAbove(Mushroom, 376, 198);

	Chest0039->CreateContents(Bread);
	Chest0039->CreateContents(Bread);
	Chest0040->CreateContents(Bread);
	Chest0040->CreateContents(Bread);
	Chest0041->CreateContents(Bread);
	Chest0041->CreateContents(Bread);
	Chest0042->CreateContents(Bread);
	Chest0042->CreateContents(Bread);

	CreateObjectAbove(Firestone, 51, 85);
	CreateObjectAbove(Firestone, 168, 146);
	CreateObjectAbove(Firestone, 17, 261);
	CreateObjectAbove(Firestone, 394, 253);
	CreateObjectAbove(Firestone, 278, 240);
	CreateObjectAbove(Firestone, 143, 439);
	CreateObjectAbove(Firestone, 618, 225);
	CreateObjectAbove(Firestone, 653, 231);
	CreateObjectAbove(Firestone, 500, 245);
	CreateObjectAbove(Firestone, 633, 236);
	CreateObjectAbove(Firestone, 32, 277);
	CreateObjectAbove(Firestone, 32, 266);
	CreateObjectAbove(Firestone, 17, 238);
	CreateObjectAbove(Firestone, 153, 149);
	CreateObjectAbove(Firestone, 585, 241);
	return true;
}
