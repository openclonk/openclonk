/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Grass, 555, 551);
	CreateObject(Grass, 533, 550);
	CreateObject(Grass, 1306, 706);

	CreateObject(Tree_Coniferous, 380, 877);
	CreateObject(Tree_Coniferous, 210, 875);
	CreateObject(Tree_Coniferous, 207, 871);
	CreateObject(Tree_Coniferous, 252, 872);
	CreateObject(Tree_Coniferous, 367, 874);
	CreateObject(Tree_Coniferous, 309, 871);
	CreateObject(Tree_Coniferous, 179, 874);
	CreateObject(Tree_Coniferous, 271, 874);
	CreateObject(Tree_Coniferous, 423, 547);
	CreateObject(Tree_Coniferous, 496, 556);
	CreateObject(Tree_Coniferous, 322, 554);
	CreateObject(Tree_Coniferous, 1339, 367);
	CreateObject(Tree_Coniferous, 1357, 360);
	CreateObject(Tree_Coniferous, 1393, 314);
	CreateObject(Tree_Coniferous, 1304, 387);

	CreateObject(Fern, 1331, 702);
	CreateObject(Fern, 1468, 663);
	CreateObject(Fern, 1583, 696);

	var Lichen0076 = CreateObject(Lichen, 1377, 696);
	Lichen0076->SetAction("Grow");
	Lichen0076->SetPhase(3);
	var Lichen0079 = CreateObject(Lichen, 1514, 695);
	Lichen0079->SetAction("Grow");
	Lichen0079->SetPhase(3);

	var Rank0082 = CreateObject(Rank, 1352, 438);
	Rank0082->SetR(28);
	Rank0082->SetPosition(1352, 435);
	var Rank0083 = CreateObject(Rank, 1428, 446);
	Rank0083->SetR(6);
	Rank0083->SetPosition(1428, 443);
	var Rank0084 = CreateObject(Rank, 1431, 458);
	Rank0084->SetR(-28);
	Rank0084->SetPosition(1431, 455);
	var Rank0085 = CreateObject(Rank, 552, 589);
	Rank0085->SetR(4);
	Rank0085->SetPosition(552, 586);
	var Rank0086 = CreateObject(Rank, 534, 601);
	Rank0086->SetR(-17);
	Rank0086->SetPosition(534, 598);
	var Rank0087 = CreateObject(Rank, 322, 615);
	Rank0087->SetR(23);
	Rank0087->SetPosition(322, 612);
	var Rank0088 = CreateObject(Rank, 404, 762);
	Rank0088->SetR(8);
	Rank0088->SetPosition(404, 759);

	CreateObject(Trunk, 1447, 682);

	CreateObject(SproutBerryBush, 1286, 734);

	CreateObject(Tree_Coniferous, 1297, 388);

	CreateObject(SproutBerryBush, 565, 861);

	CreateObject(Wheat, 919, 553);
	CreateObject(Wheat, 1296, 614);

	var Tree_Coniferous0114 = CreateObject(Tree_Coniferous, 231, 878);
	Tree_Coniferous0114->SetCon(47);

	var Chest0118 = CreateObject(Chest, 264, 1287);

	var WoodenCabin0119 = CreateObject(WoodenCabin, 62, 870);

	CreateObject(Idol, 102, 871);

	var Lorry0121 = CreateObject(Lorry, 76, 1299);
	Lorry0121->SetR(-14);
	Lorry0121->SetPosition(76, 1290);

	var Catapult0123 = CreateObject(Catapult, 445, 547);
	Catapult0123->SetCon(80);
	Catapult0123->SetRDir(1);
	Catapult0123->SetClrModulation(0xff686868);

	CreateObject(Rock, 279, 965);
	CreateObject(Rock, 469, 1214);
	CreateObject(Rock, 225, 1335);
	CreateObject(Rock, 69, 1125);
	CreateObject(Rock, 45, 915);
	CreateObject(Rock, 547, 1015);
	CreateObject(Rock, 1117, 1178);
	CreateObject(Rock, 1176, 1308);
	CreateObject(Rock, 1414, 1075);
	CreateObject(Rock, 1485, 893);

	CreateObject(Coal, 218, 1010);
	CreateObject(Coal, 94, 1046);
	CreateObject(Coal, 98, 1010);

	CreateObject(Ore, 269, 1105);

	CreateObject(Nugget, 40, 1210);
	CreateObject(Nugget, 18, 1216);
	CreateObject(Nugget, 49, 1267);
	CreateObject(Nugget, 439, 1259);
	CreateObject(Nugget, 485, 1154);
	CreateObject(Nugget, 1580, 1139);
	CreateObject(Nugget, 1470, 1080);
	CreateObject(Nugget, 33, 1311);
	CreateObject(Nugget, 134, 1347);
	CreateObject(Nugget, 253, 1317);
	CreateObject(Nugget, 369, 1283);
	CreateObject(Nugget, 373, 1243);

	CreateObject(Metal, 124, 1302);

	CreateObject(Loam, 520, 950);
	CreateObject(Loam, 403, 1010);
	CreateObject(Loam, 339, 1252);
	CreateObject(Loam, 178, 1312);
	CreateObject(Loam, 727, 1274);
	CreateObject(Loam, 884, 1333);
	CreateObject(Loam, 1195, 1300);
	CreateObject(Loam, 1568, 1109);
	CreateObject(Loam, 1565, 880);
	CreateObject(Loam, 1360, 784);
	CreateObject(Loam, 1519, 724);
	CreateObject(Loam, 1348, 721);
	CreateObject(Loam, 1379, 352);

	var Wood0164 = CreateObject(Wood, 518, 1081);
	Wood0164->SetR(35);
	Wood0164->SetPosition(518, 1078);
	CreateObject(Wood, 1302, 904);
	var Wood0166 = CreateObject(Wood, 1335, 904);
	Wood0166->SetClrModulation(0xff302020);

	CreateObject(Moss, 1357, 695);
	CreateObject(Moss, 1269, 905);
	CreateObject(Moss, 336, 558);
	CreateObject(Moss, 479, 554);
	CreateObject(Moss, 523, 847);

	CreateObject(Crate, 155, 1302);
	CreateObject(Crate, 139, 1302);

	CreateObject(Bread, 171, 1301);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);
	Chest0118->CreateContents(Bread);

	WoodenCabin0119->CreateContents(Seeds);
	WoodenCabin0119->CreateContents(Seeds);

	WoodenCabin0119->CreateContents(Sproutberry);

	var Seaweed0191 = CreateObject(Seaweed, 1343, 991);
	Seaweed0191->SetYDir(14);
	Seaweed0191->SetPhase(73);
	var Seaweed0194 = CreateObject(Seaweed, 1430, 918);
	Seaweed0194->SetYDir(14);
	Seaweed0194->SetPhase(14);
	var Seaweed0197 = CreateObject(Seaweed, 1527, 918);
	Seaweed0197->SetYDir(14);
	Seaweed0197->SetPhase(32);

	CreateObject(Firestone, 49, 1316);
	CreateObject(Firestone, 36, 1282);
	Lorry0121->CreateContents(Firestone);
	CreateObject(Firestone, 452, 920);
	CreateObject(Firestone, 50, 949);
	CreateObject(Firestone, 374, 894);
	CreateObject(Firestone, 301, 927);
	CreateObject(Firestone, 38, 1072);
	CreateObject(Firestone, 216, 1085);
	CreateObject(Firestone, 502, 985);
	CreateObject(Firestone, 229, 925);
	CreateObject(Firestone, 413, 1133);
	CreateObject(Firestone, 757, 1233);
	CreateObject(Firestone, 374, 1296);
	CreateObject(Firestone, 345, 1073);
	CreateObject(Firestone, 586, 1250);
	CreateObject(Firestone, 154, 1350);
	CreateObject(Firestone, 381, 1079);
	CreateObject(Firestone, 714, 1253);
	CreateObject(Firestone, 1267, 956);
	CreateObject(Firestone, 1346, 820);
	CreateObject(Firestone, 1393, 916);
	CreateObject(Firestone, 1464, 720);
	CreateObject(Firestone, 1308, 1168);
	CreateObject(Firestone, 80, 1102);
	CreateObject(Firestone, 351, 1230);
	CreateObject(Firestone, 112, 1128);
	return true;
}
