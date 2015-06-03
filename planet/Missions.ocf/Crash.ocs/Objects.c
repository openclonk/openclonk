/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Grass, 555, 550);
	CreateObjectAbove(Grass, 533, 550);
	CreateObjectAbove(Grass, 1306, 706);

	var Rule_BaseRespawn001 = CreateObject(Rule_BaseRespawn);
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

	var Branch001 = CreateObject(Branch, 1352, 435);
	Branch001->SetR(170);
	var Branch002 = CreateObject(Branch, 1427, 442);
	Branch002->SetR(160);
	var Branch003 = CreateObject(Branch, 1430, 450);
	Branch003->SetR(150);
	var Branch004 = CreateObject(Branch, 552, 586);
	Branch004->SetR(150);
	var Branch005 = CreateObject(Branch, 524, 600);
	Branch005->SetR(160);
	var Branch006 = CreateObject(Branch, 317, 611);
	Branch006->SetR(-170);
	var Branch007 = CreateObject(Branch, 398, 770);
	Branch007->SetR(-110);

	CreateObjectAbove(Trunk, 1447, 682);

	CreateObjectAbove(SproutBerryBush, 1286, 734);

	CreateObjectAbove(Tree_Coniferous, 1297, 388);

	CreateObjectAbove(SproutBerryBush, 565, 861);

	CreateObjectAbove(Wheat, 1345, 696);

	var Tree_Coniferous001 = CreateObjectAbove(Tree_Coniferous, 231, 878);
	Tree_Coniferous001->SetCon(48);

	var Chest001 = CreateObjectAbove(Chest, 264, 1287);

	var WoodenCabin001 = CreateObjectAbove(WoodenCabin, 62, 870);
	WoodenCabin001->MakeInvincible();

	CreateObjectAbove(Idol, 102, 871);

	var Flagpole001 = CreateObjectAbove(Flagpole, 119, 872);
	Flagpole001->SetNeutral(true);

	var Lorry001 = CreateObject(Lorry, 76, 1290);
	Lorry001->SetR(-14);

	var Catapult001 = CreateObjectAbove(Catapult, 445, 547);
	Catapult001->SetCon(80);
	Catapult001->SetRDir(1);
	Catapult001->SetClrModulation(0xff686868);

	var Clonk001 = CreateObjectAbove(Clonk, 556, 546);
	Clonk001->SetDir(DIR_Right);
	Clonk001->SetObjectLayer(Clonk001);
	Clonk001->SetName("Gyro");
	Clonk001->SetSkin(2);
	Clonk001->SetDialogue("Gyro", true);
	Clonk001->SetColor(0xa05000);

	CreateObject(Rock, 279, 964);
	CreateObject(Rock, 469, 1213);
	CreateObject(Rock, 225, 1334);
	CreateObject(Rock, 69, 1124);
	CreateObject(Rock, 45, 914);
	CreateObject(Rock, 547, 1014);
	CreateObject(Rock, 1117, 1177);
	CreateObject(Rock, 1176, 1307);
	CreateObject(Rock, 1414, 1074);
	CreateObject(Rock, 1485, 892);

	CreateObject(Coal, 218, 1009);
	CreateObject(Coal, 94, 1045);
	CreateObject(Coal, 98, 1009);

	CreateObject(Ore, 269, 1104);

	CreateObject(Nugget, 40, 1209);
	CreateObject(Nugget, 18, 1215);
	CreateObject(Nugget, 49, 1266);
	CreateObject(Nugget, 439, 1258);
	CreateObject(Nugget, 485, 1153);
	CreateObject(Nugget, 1580, 1138);
	CreateObject(Nugget, 1470, 1079);
	CreateObject(Nugget, 33, 1310);
	CreateObject(Nugget, 134, 1346);
	CreateObject(Nugget, 253, 1316);
	CreateObject(Nugget, 369, 1282);
	CreateObject(Nugget, 373, 1242);

	CreateObject(Metal, 124, 1302);

	CreateObject(Loam, 520, 947);
	CreateObject(Loam, 403, 1007);
	CreateObject(Loam, 339, 1249);
	CreateObject(Loam, 178, 1309);
	CreateObject(Loam, 727, 1271);
	CreateObject(Loam, 884, 1330);
	CreateObject(Loam, 1195, 1297);
	CreateObject(Loam, 1568, 1106);
	CreateObject(Loam, 1565, 877);
	CreateObject(Loam, 1360, 781);
	CreateObject(Loam, 1519, 721);
	CreateObject(Loam, 1348, 718);
	CreateObject(Loam, 1379, 349);

	var Wood001 = CreateObject(Wood, 518, 1078);
	Wood001->SetR(35);
	CreateObjectAbove(Wood, 1302, 904);
	var Wood002 = CreateObjectAbove(Wood, 1335, 904);
	Wood002->SetClrModulation(0xff302020);

	CreateObjectAbove(Moss, 1357, 695);
	CreateObjectAbove(Moss, 1269, 905);
	CreateObject(Moss, 336, 557);
	CreateObject(Moss, 479, 553);
	CreateObject(Moss, 523, 846);

	CreateObjectAbove(Crate, 155, 1302);
	CreateObjectAbove(Crate, 139, 1302);

	CreateObjectAbove(Bread, 171, 1302);
	Chest001->CreateContents(Bread, 8);

	WoodenCabin001->CreateContents(Seeds, 2);

	WoodenCabin001->CreateContents(Sproutberry);

	CreateObjectAbove(Seaweed, 1343, 991);
	CreateObjectAbove(Seaweed, 1430, 918);
	CreateObjectAbove(Seaweed, 1530, 924);

	CreateObject(Firestone, 49, 1315);
	CreateObject(Firestone, 36, 1281);
	Lorry001->CreateContents(Firestone);
	CreateObject(Firestone, 452, 919);
	CreateObject(Firestone, 50, 948);
	CreateObject(Firestone, 374, 893);
	CreateObject(Firestone, 301, 926);
	CreateObject(Firestone, 38, 1071);
	CreateObject(Firestone, 216, 1084);
	CreateObject(Firestone, 502, 984);
	CreateObject(Firestone, 229, 924);
	CreateObject(Firestone, 413, 1132);
	CreateObject(Firestone, 757, 1232);
	CreateObject(Firestone, 374, 1295);
	CreateObject(Firestone, 345, 1072);
	CreateObject(Firestone, 586, 1249);
	CreateObject(Firestone, 154, 1349);
	CreateObject(Firestone, 381, 1078);
	CreateObject(Firestone, 714, 1252);
	CreateObject(Firestone, 1267, 955);
	CreateObject(Firestone, 1346, 819);
	CreateObject(Firestone, 1393, 915);
	CreateObject(Firestone, 1464, 719);
	CreateObject(Firestone, 1308, 1167);
	CreateObject(Firestone, 80, 1101);
	CreateObject(Firestone, 351, 1229);
	CreateObject(Firestone, 112, 1127);
	return true;
}
