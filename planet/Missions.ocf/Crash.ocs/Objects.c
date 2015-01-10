/* Automatically created objects file */

func InitializeObjects()
{
	CreateObjectAbove(Grass, 555, 551);
	CreateObjectAbove(Grass, 533, 550);
	CreateObjectAbove(Grass, 1306, 706);

	var Rule_BaseRespawn001 = CreateObjectAbove(Rule_BaseRespawn, 0, 0);
	Rule_BaseRespawn001->SetInventoryTransfer(true);
	Rule_BaseRespawn001->SetFreeCrew(true);

	CreateObjectAbove(Tree_Coniferous, 380, 877);
	CreateObjectAbove(Tree_Coniferous, 210, 875);
	CreateObjectAbove(Tree_Coniferous, 207, 871);
	CreateObjectAbove(Tree_Coniferous, 252, 872);
	CreateObjectAbove(Tree_Coniferous, 367, 874);
	CreateObjectAbove(Tree_Coniferous, 309, 871);
	CreateObjectAbove(Tree_Coniferous, 179, 874);
	CreateObjectAbove(Tree_Coniferous, 271, 874);
	CreateObjectAbove(Tree_Coniferous, 423, 547);
	CreateObjectAbove(Tree_Coniferous, 496, 560);
	CreateObjectAbove(Tree_Coniferous, 322, 554);
	CreateObjectAbove(Tree_Coniferous, 1339, 367);
	CreateObjectAbove(Tree_Coniferous, 1357, 360);
	CreateObjectAbove(Tree_Coniferous, 1393, 314);
	CreateObjectAbove(Tree_Coniferous, 1304, 387);

	CreateObjectAbove(Fern, 1331, 704);
	CreateObjectAbove(Fern, 1468, 664);
	CreateObjectAbove(Fern, 1583, 696);

	var Lichen001 = CreateObjectAbove(Lichen, 1377, 696);
	Lichen001->SetAction("Grow");
	Lichen001->SetPhase(3);
	var Lichen002 = CreateObjectAbove(Lichen, 1514, 696);
	Lichen002->SetAction("Grow");
	Lichen002->SetPhase(3);

	var Branch001 = CreateObjectAbove(Branch, 1352, 447);
	Branch001->SetR(170);
	Branch001->SetPosition(1352, 435);
	var Branch002 = CreateObjectAbove(Branch, 1427, 453);
	Branch002->SetR(160);
	Branch002->SetPosition(1427, 442);
	var Branch003 = CreateObjectAbove(Branch, 1430, 460);
	Branch003->SetR(150);
	Branch003->SetPosition(1430, 450);
	var Branch004 = CreateObjectAbove(Branch, 552, 596);
	Branch004->SetR(150);
	Branch004->SetPosition(552, 586);
	var Branch005 = CreateObjectAbove(Branch, 524, 611);
	Branch005->SetR(160);
	Branch005->SetPosition(524, 600);
	var Branch006 = CreateObjectAbove(Branch, 317, 623);
	Branch006->SetR(-170);
	Branch006->SetPosition(317, 611);
	var Branch007 = CreateObjectAbove(Branch, 398, 774);
	Branch007->SetR(-110);
	Branch007->SetPosition(398, 770);

	CreateObjectAbove(Trunk, 1447, 682);

	CreateObjectAbove(SproutBerryBush, 1286, 734);

	CreateObjectAbove(Tree_Coniferous, 1297, 388);

	CreateObjectAbove(SproutBerryBush, 565, 861);

	CreateObjectAbove(Wheat, 1345, 696);

	var Tree_Coniferous001 = CreateObjectAbove(Tree_Coniferous, 231, 878);
	Tree_Coniferous001->SetCon(47);

	var Chest001 = CreateObjectAbove(Chest, 264, 1287);

	var WoodenCabin001 = CreateObjectAbove(WoodenCabin, 62, 870);

	CreateObjectAbove(Idol, 102, 871);

	var Flagpole001 = CreateObjectAbove(Flagpole, 119, 872);
	Flagpole001->SetNeutral(true);

	var Lorry001 = CreateObjectAbove(Lorry, 76, 1299);
	Lorry001->SetR(-14);
	Lorry001->SetPosition(76, 1290);

	var Catapult001 = CreateObjectAbove(Catapult, 445, 547);
	Catapult001->SetCon(80);
	Catapult001->SetRDir(1);
	Catapult001->SetClrModulation(0xff686868);

	CreateObjectAbove(Rock, 279, 965);
	CreateObjectAbove(Rock, 469, 1214);
	CreateObjectAbove(Rock, 225, 1335);
	CreateObjectAbove(Rock, 69, 1125);
	CreateObjectAbove(Rock, 45, 915);
	CreateObjectAbove(Rock, 547, 1015);
	CreateObjectAbove(Rock, 1117, 1178);
	CreateObjectAbove(Rock, 1176, 1308);
	CreateObjectAbove(Rock, 1414, 1075);
	CreateObjectAbove(Rock, 1485, 893);

	CreateObjectAbove(Coal, 218, 1010);
	CreateObjectAbove(Coal, 94, 1046);
	CreateObjectAbove(Coal, 98, 1010);

	CreateObjectAbove(Ore, 269, 1105);

	CreateObjectAbove(Nugget, 40, 1210);
	CreateObjectAbove(Nugget, 18, 1216);
	CreateObjectAbove(Nugget, 49, 1267);
	CreateObjectAbove(Nugget, 439, 1259);
	CreateObjectAbove(Nugget, 485, 1154);
	CreateObjectAbove(Nugget, 1580, 1139);
	CreateObjectAbove(Nugget, 1470, 1080);
	CreateObjectAbove(Nugget, 33, 1311);
	CreateObjectAbove(Nugget, 134, 1347);
	CreateObjectAbove(Nugget, 253, 1317);
	CreateObjectAbove(Nugget, 369, 1283);
	CreateObjectAbove(Nugget, 373, 1243);

	CreateObjectAbove(Metal, 124, 1302);

	CreateObjectAbove(Loam, 520, 950);
	CreateObjectAbove(Loam, 403, 1010);
	CreateObjectAbove(Loam, 339, 1252);
	CreateObjectAbove(Loam, 178, 1312);
	CreateObjectAbove(Loam, 727, 1274);
	CreateObjectAbove(Loam, 884, 1333);
	CreateObjectAbove(Loam, 1195, 1300);
	CreateObjectAbove(Loam, 1568, 1109);
	CreateObjectAbove(Loam, 1565, 880);
	CreateObjectAbove(Loam, 1360, 784);
	CreateObjectAbove(Loam, 1519, 724);
	CreateObjectAbove(Loam, 1348, 721);
	CreateObjectAbove(Loam, 1379, 352);

	var Wood001 = CreateObjectAbove(Wood, 518, 1081);
	Wood001->SetR(35);
	Wood001->SetPosition(518, 1078);
	CreateObjectAbove(Wood, 1302, 904);
	var Wood002 = CreateObjectAbove(Wood, 1335, 904);
	Wood002->SetClrModulation(0xff302020);

	CreateObjectAbove(Moss, 1357, 695);
	CreateObjectAbove(Moss, 1269, 905);
	CreateObjectAbove(Moss, 336, 558);
	CreateObjectAbove(Moss, 479, 554);
	CreateObjectAbove(Moss, 523, 847);

	CreateObjectAbove(Crate, 155, 1302);
	CreateObjectAbove(Crate, 139, 1302);

	CreateObjectAbove(Bread, 171, 1302);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);
	Chest001->CreateContents(Bread);

	WoodenCabin001->CreateContents(Seeds);
	WoodenCabin001->CreateContents(Seeds);

	WoodenCabin001->CreateContents(Sproutberry);

	var Seaweed001 = CreateObjectAbove(Seaweed, 1343, 991);
	Seaweed001->SetPhase(3);
	var Seaweed002 = CreateObjectAbove(Seaweed, 1430, 918);
	Seaweed002->SetPhase(22);
	var Seaweed003 = CreateObjectAbove(Seaweed, 1530, 921);
	Seaweed003->SetPhase(40);

	CreateObjectAbove(Firestone, 49, 1316);
	CreateObjectAbove(Firestone, 36, 1282);
	Lorry001->CreateContents(Firestone);
	CreateObjectAbove(Firestone, 452, 920);
	CreateObjectAbove(Firestone, 50, 949);
	CreateObjectAbove(Firestone, 374, 894);
	CreateObjectAbove(Firestone, 301, 927);
	CreateObjectAbove(Firestone, 38, 1072);
	CreateObjectAbove(Firestone, 216, 1085);
	CreateObjectAbove(Firestone, 502, 985);
	CreateObjectAbove(Firestone, 229, 925);
	CreateObjectAbove(Firestone, 413, 1133);
	CreateObjectAbove(Firestone, 757, 1233);
	CreateObjectAbove(Firestone, 374, 1296);
	CreateObjectAbove(Firestone, 345, 1073);
	CreateObjectAbove(Firestone, 586, 1250);
	CreateObjectAbove(Firestone, 154, 1350);
	CreateObjectAbove(Firestone, 381, 1079);
	CreateObjectAbove(Firestone, 714, 1253);
	CreateObjectAbove(Firestone, 1267, 956);
	CreateObjectAbove(Firestone, 1346, 820);
	CreateObjectAbove(Firestone, 1393, 916);
	CreateObjectAbove(Firestone, 1464, 720);
	CreateObjectAbove(Firestone, 1308, 1168);
	CreateObjectAbove(Firestone, 80, 1102);
	CreateObjectAbove(Firestone, 351, 1230);
	CreateObjectAbove(Firestone, 112, 1128);
	return true;
}
