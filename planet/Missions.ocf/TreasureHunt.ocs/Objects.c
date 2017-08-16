/* Automatically created objects file */

static g_flagpole, g_golden_idol, npc_dagobert, npc_tarzan, g_golden_shovel, g_last_stone_door;

func InitializeObjects()
{
	CreateObjectAbove(Grass, 1627, 396);
	CreateObjectAbove(Grass, 1636, 385);
	CreateObjectAbove(Grass, 1786, 469);
	CreateObjectAbove(Grass, 1574, 493);
	CreateObjectAbove(Grass, 1564, 493);
	CreateObjectAbove(Grass, 1537, 525);
	CreateObjectAbove(Grass, 1585, 486);
	CreateObjectAbove(Grass, 1739, 430);

	var Torch001 = CreateObjectAbove(Torch, 1201, 1549);
	Torch001->AttachToWall(true);
	var Torch002 = CreateObjectAbove(Torch, 1109, 1146);
	Torch002->AttachToWall(true);
	var Torch003 = CreateObjectAbove(Torch, 923, 1144);
	Torch003->AttachToWall(true);
	var Torch004 = CreateObjectAbove(Torch, 1869, 1454);
	Torch004->AttachToWall(true);
	var Torch005 = CreateObjectAbove(Torch, 562, 1126);
	Torch005->AttachToWall(true);

	var Chest001 = CreateObject(Chest, 1002, 302);
	Chest001.Plane = 50;

	var Column001 = CreateObjectAbove(Column, 779, 591);
	Column001->SetClrModulation(0xffffd0d0);
	Column001->SetMeshMaterial("AncientColumn", 0);
	Column001.Plane = 50;

	CreateObject(Rule_TeamAccount);

	CreateObject(Rule_NoPowerNeed);

	var LargeCaveMushroom001 = CreateObjectAbove(LargeCaveMushroom, 1308, 1038);
	LargeCaveMushroom001->SetClrModulation(0xffe4effc);
	var LargeCaveMushroom002 = CreateObjectAbove(LargeCaveMushroom, 1345, 1028);
	LargeCaveMushroom002->SetClrModulation(0xffe1e3ee);
	LargeCaveMushroom002->SetMeshMaterial("FlyAmanitaMushroom", 0);
	var LargeCaveMushroom003 = CreateObjectAbove(LargeCaveMushroom, 1399, 1025);
	LargeCaveMushroom003->SetClrModulation(0xfff3e3e7);
	LargeCaveMushroom003->SetMeshMaterial("FlyAmanitaMushroom", 0);
	var LargeCaveMushroom004 = CreateObjectAbove(LargeCaveMushroom, 1464, 999);
	LargeCaveMushroom004->SetClrModulation(0xffe0e6dd);
	var LargeCaveMushroom005 = CreateObjectAbove(LargeCaveMushroom, 1450, 1012);
	LargeCaveMushroom005->SetClrModulation(0xffe4eae2);
	var LargeCaveMushroom006 = CreateObjectAbove(LargeCaveMushroom, 1523, 993);
	LargeCaveMushroom006->SetClrModulation(0xffe2deee);
	LargeCaveMushroom006->SetMeshMaterial("FlyAmanitaMushroom", 0);

	CreateObjectAbove(Trunk, 1000, 313);
	CreateObjectAbove(Trunk, 1006, 313);

	CreateObjectAbove(EnvPack_Painting, 606, 434);

	CreateObjectAbove(EnvPack_Guidepost, 81, 743);

	CreateObjectAbove(EnvPack_BridgeRustic, 591, 356);
	CreateObjectAbove(EnvPack_Bag, 506, 968);

	CreateObjectAbove(EnvPack_Lantern, 356, 458);
	CreateObject(EnvPack_TreeTrunks, 601, 408);

	CreateObjectAbove(EnvPack_Rail, 564, 354);
	CreateObjectAbove(EnvPack_Rail, 616, 356);
	CreateObjectAbove(EnvPack_Rail, 633, 354);
	CreateObjectAbove(EnvPack_Rail, 554, 353);

	CreateObjectAbove(EnvPack_WineBarrel, 627, 455);

	CreateObjectAbove(EnvPack_Crate, 494, 385);

	CreateObjectAbove(Fern, 1793, 474);
	CreateObjectAbove(Fern, 1645, 384);
	CreateObjectAbove(Fern, 1525, 535);

	var Branch001 = CreateObject(Branch, 1320, 329);
	Branch001->SetR(148);
	var Branch002 = CreateObject(Branch, 1327, 291);
	Branch002->SetR(165);
	var Branch003 = CreateObject(Branch, 1424, 260);
	Branch003->SetR(108);
	var Branch004 = CreateObject(Branch, 1430, 240);
	Branch004->SetR(39);
	var Branch005 = CreateObject(Branch, 1413, 265);
	Branch005->SetR(128);
	var Branch006 = CreateObject(Branch, 1396, 266);
	Branch006->SetR(-131);

	CreateObjectAbove(SproutBerryBush, 1823, 493);

	CreateObjectAbove(Trunk, 401, 1147);

	CreateObjectAbove(Tree_Coconut, 51, 1142);
	CreateObjectAbove(Tree_Coconut, 333, 1150);

	CreateObjectAbove(Tree_Coniferous, 1864, 464);
	CreateObjectAbove(Tree_Coniferous, 2788, 680);

	var Lichen001 = CreateObjectAbove(Lichen, 2694, 706);
	Lichen001->SetAction("Grown");

	CreateObject(BigRock, 1301, 497);
	CreateObjectAbove(BigRock, 1207, 282);
	CreateObject(BigRock, 1291, 260);
	var Amethyst001 = CreateObject(Amethyst, 803, 579);
	Amethyst001.Plane = 190;

	var Chest002 = CreateObjectAbove(Chest, 515, 967);
	Chest002.tool_spawn = TeleGlove;
	var Chest003 = CreateObjectAbove(Chest, 227, 760);
	Chest003.tool_spawn = Pickaxe;
	var Chest004 = CreateObjectAbove(Chest, 624, 943);
	Chest004.tool_spawn = GrappleBow;
	var Chest005 = CreateObjectAbove(Chest, 603, 942);
	Chest005.tool_spawn = GrappleBow;
	var Chest021 = CreateObjectAbove(Chest, 472, 455);
	var Chest010 = CreateObjectAbove(Chest, 546, 383);
	var Chest006 = CreateObjectAbove(Chest, 840, 47);
	Chest006.tool_spawn = WindBag;
	var Chest013 = CreateObjectAbove(Chest, 853, 1574);
	var Chest012 = CreateObjectAbove(Chest, 1428, 1542);
	var Chest011 = CreateObjectAbove(Chest, 1765, 1191);
	var Chest007 = CreateObjectAbove(Chest, 1878, 719);
	Chest007.tool_spawn = Axe;
	var Chest014 = CreateObjectAbove(Chest, 1943, 714);
	var Chest015 = CreateObjectAbove(Chest, 2103, 1119);
	var Chest016 = CreateObjectAbove(Chest, 397, 583);
	var Chest008 = CreateObjectAbove(Chest, 871, 583);
	Chest008->SetMeshMaterial("GoldenChest", 0);
	var Chest018 = CreateObjectAbove(Chest, 2662, 1357);
	var Chest017 = CreateObjectAbove(Chest, 720, 352);
	var Chest009 = CreateObjectAbove(Chest, 1830, 486);
	Chest009.tool_spawn = Hammer;
	var Chest019 = CreateObjectAbove(Chest, 730, 135);
	var Chest020 = CreateObjectAbove(Chest, 1626, 1591);

	CreateObjectAbove(Pump, 1027, 1152);

	CreateObjectAbove(Sawmill, 1259, 1047);

	var ToolsWorkshop001 = CreateObjectAbove(ToolsWorkshop, 1169, 903);

	CreateObjectAbove(Ruin_Windmill, 1678, 375);

	CreateObjectAbove(Ruin_WoodenCabin, 1199, 1046);

	var Idol001 = CreateObjectAbove(Idol, 1045, 721);
	Idol001->SetMeshMaterial("IdolGrayColor", 0);

	g_flagpole = CreateObject(Flagpole, 210, 1151);
	g_flagpole.StaticSaveVar = "g_flagpole";
	g_flagpole->SetName("Respawn");
	g_flagpole->SetNeutral(true);

	var LotsOfCoins001 = CreateObject(LotsOfCoins, 805, 583);
	LotsOfCoins001.Plane = 200;

	g_golden_idol = CreateObject(Idol, 824, 568);
	g_golden_idol->SetR(-4);
	g_golden_idol.Plane = 220;
	g_golden_idol.StaticSaveVar = "g_golden_idol";

	var Lorry002 = CreateObjectAbove(Lorry, 200, 1183);
	var Lorry001 = CreateObjectAbove(Lorry, 708, 1407);
	Lorry001->SetMeshMaterial("RuinedLorry", 0);

	var Catapult001 = CreateObjectAbove(Catapult, 1714, 951);
	Catapult001->SetRDir(-7);

	CreateObjectAbove(StrawMan, 1924, 439);
	CreateObjectAbove(StrawMan, 2642, 705);
	var Clonk001 = CreateObjectAbove(Clonk, 316, 431);
	Clonk001->SetName("$Guard$");
	AI->AddAI(Clonk001);
	AI->SetHome(Clonk001, 315, 422, DIR_Left);
	AI->SetGuardRange(Clonk001, 296, 322, 350, 140);
	AI->SetEncounterCB(Clonk001, "EncounterCastle");
	Clonk001->SetDir(DIR_Left);
	
	var Clonk002 = CreateObjectAbove(Clonk, 501, 454);
	Clonk002->SetDir(DIR_Right);
	Clonk002->SetName("$Guard$");
	AI->AddAI(Clonk002);
	AI->SetHome(Clonk002, 502, 445, DIR_Right);
	AI->SetGuardRange(Clonk002, 460, 300, 200, 160);
	AI->SetMaxAggroDistance(Clonk002, 60);
	
	var Clonk003 = CreateObjectAbove(Clonk, 534, 455);
	Clonk003->SetName("$Guard$");
	Clonk003->SetDir(DIR_Right);
	AI->AddAI(Clonk003);
	AI->SetGuardRange(Clonk003, 460, 300, 200, 160);
	AI->SetMaxAggroDistance(Clonk003, 60);
	
	var Clonk004 = CreateObjectAbove(Clonk, 671, 638);
	Clonk004->SetName("$TreasureHunter$");
	Clonk004->SetDir(DIR_Right);
	Clonk004->SetCon(150);
	Clonk004->SetColor(0xffffa000);
	AI->AddAI(Clonk004);
	AI->SetHome(Clonk004, 671, 629, DIR_Right);
	AI->SetGuardRange(Clonk004, 580, 480, 320, 175);
	AI->SetEncounterCB(Clonk004, "EncounterFinal");
	
	npc_dagobert = CreateObjectAbove(Clonk, 369, 1142);
	npc_dagobert->SetColor(0xffa000);
	npc_dagobert->SetName("Dagobert");
	npc_dagobert.StaticSaveVar = "npc_dagobert";
	npc_dagobert->MakeInvincible();
	npc_dagobert->SetDir(DIR_Left);
	var Clonk005 = CreateObjectAbove(Clonk, 1720, 375);
	Clonk005->SetDir(DIR_Right);
	Clonk005->SetColor(0x808080);
	Clonk005->SetName("Otto");
	Clonk005->MakeInvincible();
	Clonk005->SetSkin(2);
	var Clonk006 = CreateObjectAbove(Clonk, 1868, 951);
	Clonk006->SetColor(0xff0000);
	Clonk006->SetName("Donald");
	Clonk006->MakeInvincible();
	Clonk006->SetDir(DIR_Left);
	var Clonk007 = CreateObjectAbove(Clonk, 676, 943);
	Clonk007->SetDir(DIR_Right);
	Clonk007->SetColor(0x802000);
	Clonk007->SetName("Jane");
	Clonk007->SetSkin(1);
	Clonk007->MakeInvincible();
	npc_tarzan = CreateObjectAbove(Clonk, 750, 859);
	npc_tarzan->SetXDir(3);
	npc_tarzan->SetYDir(27);
	npc_tarzan->SetColor(0x402000);
	npc_tarzan->SetName("Tarzan");
	npc_tarzan.StaticSaveVar = "npc_tarzan";
	npc_tarzan->SetDir(DIR_Left);
	var Clonk008 = CreateObjectAbove(Clonk, 498, 966);
	Clonk008->SetColor(0x20ffff);
	Clonk008->SetName("Sophie");
	Clonk008->SetSkin(3);
	Clonk008->MakeInvincible();
	Clonk008->SetDir(DIR_Left);
	var Clonk009 = CreateObjectAbove(Clonk, 853, 734);
	Clonk009->SetDir(DIR_Right);
	Clonk009->SetColor(0x6000);
	Clonk009->SetName("Riku");
	Clonk009->SetSkin(2);
	var Clonk010 = CreateObjectAbove(Clonk, 1098, 1151);
	Clonk010->SetDir(DIR_Right);
	Clonk010->SetColor(0x800000);
	Clonk010->SetName("Ann");
	Clonk010->SetSkin(3);
	Clonk010->MakeInvincible();
	var Pickaxe001 = Clonk010->CreateContents(Pickaxe);

	Clonk010->SetDialogue("Ann",true);
	Clonk009->SetDialogue("Riku",true);

	var TeleGlove001 = Clonk008->CreateContents(TeleGlove);

	Clonk008->SetDialogue("Sophie",true);
	Clonk006->SetDialogue("Donald",true);
	Clonk007->SetDialogue("Jane",true);

	var GrappleBow001 = npc_tarzan->CreateContents(GrappleBow);
	var GrappleBow002 = npc_tarzan->CreateContents(GrappleBow);

	npc_tarzan->SetDialogue("Tarzan",false);
	Clonk005->SetDialogue("Otto",true);
	npc_dagobert->SetDialogue("Dagobert",true);

	CreateObjectAbove(Skull, 53, 1138);

	var Bone001 = CreateObject(Bone, 25, 1139);
	Bone001->SetR(-65);
	var Bone002 = CreateObject(Bone, 48, 1135);
	Bone002->SetR(-51);
	var Bone003 = CreateObject(Bone, 472, 966);
	Bone003->SetR(-41);
	Bone003->SetRDir(11);
	var Bone004 = CreateObject(Bone, 488, 964);
	Bone004->SetR(-51);
	Bone004->SetYDir(8);
	var Bone005 = CreateObject(Bone, 479, 964);
	Bone005->SetR(-51);
	var Bone006 = CreateObject(Bone, 464, 965);
	Bone006->SetR(-51);

	Lorry002->CreateContents(Loam, 4);
	CreateObject(Loam, 153, 1232);
	CreateObject(Loam, 357, 1317);
	CreateObject(Loam, 265, 1451);
	CreateObject(Loam, 528, 1450);
	CreateObject(Loam, 554, 1453);
	CreateObject(Loam, 988, 1284);
	CreateObject(Loam, 1030, 1272);
	CreateObject(Loam, 1065, 1267);
	CreateObject(Loam, 1051, 1258);
	CreateObject(Loam, 1049, 1279);
	CreateObject(Loam, 1081, 1266);
	CreateObject(Loam, 1334, 1049);
	CreateObject(Loam, 1532, 858);
	CreateObject(Loam, 1619, 804);
	CreateObject(Loam, 1642, 439);
	CreateObject(Loam, 1694, 399);
	CreateObject(Loam, 1578, 524);
	CreateObject(Loam, 1746, 456);

	var Metal001 = CreateObject(Metal, 1922, 976);
	Metal001->SetR(20);

	var Nugget001 = CreateObject(Nugget, 812, 589);
	Nugget001->SetClrModulation(0xffffd0a0);
	CreateObject(Nugget, 869, 582);
	var Nugget002 = CreateObject(Nugget, 853, 583);
	Nugget002->SetClrModulation(0xffffd0a0);
	var Nugget003 = CreateObject(Nugget, 823, 583);
	Nugget003->SetClrModulation(0xffffd0a0);

	Chest010->CreateContents(GoldBar);
	Chest011->CreateContents(GoldBar);
	Chest012->CreateContents(GoldBar);
	Chest013->CreateContents(GoldBar);
	Lorry001->CreateContents(GoldBar);
	Chest006->CreateContents(GoldBar);
	Chest014->CreateContents(GoldBar);
	Chest015->CreateContents(GoldBar);
	Chest016->CreateContents(GoldBar);
	Chest001->CreateContents(GoldBar);
	Chest017->CreateContents(GoldBar);
	Chest018->CreateContents(GoldBar);
	var GoldBar001 = Chest019->CreateContents(GoldBar);
	GoldBar001->SetClrModulation(0xffffd0a0);
	var GoldBar002 = CreateObject(GoldBar, 880, 540);
	GoldBar002->SetR(-29);
	GoldBar002->SetClrModulation(0xffffd0a0);
	ToolsWorkshop001->CreateContents(GoldBar);
	CreateObject(GoldBar, 72, 1463);
	CreateObject(GoldBar, 2746, 736);
	CreateObjectAbove(GoldBar, 2507, 1262);
	Chest020->CreateContents(GoldBar);
	var GoldBar003 = CreateObject(GoldBar, 972, 1277);
	GoldBar003->SetR(55);

	CreateObject(Ruby, 864, 581);
	CreateObject(Ruby, 806, 583);
	CreateObject(Ruby, 849, 578);
	CreateObject(Ruby, 856, 584);

	var Amethyst002 = CreateObject(Amethyst, 793, 584);
	Amethyst002->SetR(22);
	CreateObject(Amethyst, 885, 557);
	CreateObjectAbove(Amethyst, 828, 585);

	Lorry002->CreateContents(Dynamite, 2);
	Chest006->CreateContents(Dynamite, 3);
	CreateObjectAbove(Dynamite, 790, 736);
	Chest005->CreateContents(Dynamite, 2);
	Chest004->CreateContents(Dynamite, 2);
	Chest002->CreateContents(Dynamite);
	Chest012->CreateContents(Dynamite, 3);
	Chest014->CreateContents(Dynamite, 3);

	var Bow001 = Clonk001->CreateContents(Bow);
	Chest003->CreateContents(Bow);

	Bow001->CreateContents(Arrow);
	Clonk001->CreateContents(Arrow);
	var Arrow001 = CreateObject(Arrow, 313, 431);
	Arrow001->SetR(90);
	var Arrow002 = CreateObject(Arrow, 313, 431);
	Arrow002->SetR(90);
	var Arrow003 = CreateObject(Arrow, 313, 431);
	Arrow003->SetR(90);
	var Arrow004 = CreateObject(Arrow, 313, 431);
	Arrow004->SetR(90);
	var Arrow005 = CreateObject(Arrow, 313, 431);
	Arrow005->SetR(90);
	var Arrow006 = CreateObject(Arrow, 313, 431);
	Arrow006->SetR(90);
	var Arrow007 = CreateObject(Arrow, 313, 431);
	Arrow007->SetR(90);
	var Arrow008 = CreateObject(Arrow, 313, 431);
	Arrow008->SetR(90);
	var Arrow009 = CreateObject(Arrow, 313, 431);
	Arrow009->SetR(90);
	Chest003->CreateContents(Arrow);

	Chest021->CreateContents(Bread, 2);
	Lorry001->CreateContents(Bread, 3);
	Chest004->CreateContents(Bread);
	Chest017->CreateContents(Bread);
	Chest009->CreateContents(Bread);
	ToolsWorkshop001->CreateContents(Bread);
	Chest011->CreateContents(Bread);
	Chest012->CreateContents(Bread);
	Chest018->CreateContents(Bread);
	Chest007->CreateContents(Bread);
	Chest002->CreateContents(Bread);

	Chest021->CreateContents(DynamiteBox, 2);
	Chest017->CreateContents(DynamiteBox, 2);
	Chest020->CreateContents(DynamiteBox);
	Chest015->CreateContents(DynamiteBox);
	Chest019->CreateContents(DynamiteBox);

	Clonk003->CreateContents(Sword);
	Clonk002->CreateContents(Sword);
	Chest006->CreateContents(Sword);
	Clonk004->CreateContents(Sword, 2);
	Lorry001->CreateContents(Sword);
	Chest003->CreateContents(Sword);

	CreateObjectAbove(Seaweed, 2494, 1263);
	CreateObjectAbove(Seaweed, 2508, 1263);
	CreateObjectAbove(Seaweed, 2520, 1263);
	CreateObjectAbove(Seaweed, 2508, 1263);
	CreateObjectAbove(Seaweed, 2503, 1263);
	CreateObjectAbove(Seaweed, 2526, 1262);
	CreateObjectAbove(Seaweed, 2516, 1263);
	CreateObjectAbove(Seaweed, 2499, 1263);
	CreateObjectAbove(Seaweed, 2663, 1278);
	CreateObjectAbove(Seaweed, 2769, 1272);
	CreateObjectAbove(Seaweed, 2751, 1279);
	CreateObjectAbove(Seaweed, 2762, 1271);
	CreateObjectAbove(Seaweed, 2775, 1279);
	CreateObjectAbove(Seaweed, 2762, 1271);
	CreateObjectAbove(Seaweed, 2659, 1279);
	CreateObjectAbove(Seaweed, 2416, 1245);
	CreateObjectAbove(Seaweed, 2395, 1239);
	CreateObjectAbove(Seaweed, 2396, 1239);
	CreateObjectAbove(Seaweed, 2332, 1145);
	CreateObject(Seaweed, 2407, 1239);

	CreateObjectAbove(Mushroom, 1580, 758);
	CreateObjectAbove(Mushroom, 1613, 776);
	CreateObjectAbove(Mushroom, 1525, 847);
	CreateObjectAbove(Mushroom, 1612, 864);
	CreateObjectAbove(Mushroom, 1321, 895);
	CreateObjectAbove(Mushroom, 1315, 895);
	CreateObjectAbove(Mushroom, 1343, 902);
	CreateObjectAbove(Mushroom, 1347, 903);

	CreateObjectAbove(Balloon, 491, 383);

	var Barrel001 = CreateObjectAbove(Barrel, 623, 456);
	Barrel001->SetColor(0xff000000);

	CreateObjectAbove(Sproutberry, 1823, 488);
	CreateObjectAbove(Sproutberry, 1823, 488);
	CreateObjectAbove(Sproutberry, 1823, 488);

	CreateObjectAbove(Boompack, 543, 383);
	CreateObjectAbove(Boompack, 548, 384);
	CreateObjectAbove(Boompack, 1948, 713);
	var Boompack001 = CreateObject(Boompack, 1944, 483);
	Boompack001->SetR(135);

	g_golden_shovel = CreateObjectAbove(Shovel, 1841, 1011);
	g_golden_shovel->SetMeshMaterial("GoldenShovel", 0);
	g_golden_shovel.StaticSaveVar = "g_golden_shovel";

	CreateObject(Pipe, 1838, 1013);

	var LotsOfCoins002 = CreateObject(LotsOfCoins, 838, 583);
	LotsOfCoins002.Plane = 500;

	var GemOfPower001 = CreateObjectAbove(GemOfPower, 825, 572);
	GemOfPower001->SetCategory(C4D_StaticBack);

	CreateObjectAbove(Firestone, 564, 1136);
	CreateObjectAbove(Firestone, 552, 1136);
	CreateObjectAbove(Firestone, 562, 1136);
	CreateObjectAbove(Firestone, 571, 1136);
	CreateObjectAbove(Firestone, 567, 1136);
	CreateObject(Firestone, 558, 1135);
	CreateObject(Firestone, 546, 1135);
	CreateObjectAbove(Firestone, 560, 1136);
	CreateObject(Firestone, 546, 1135);
	CreateObject(Firestone, 546, 1135);
	CreateObject(Firestone, 555, 1135);
	CreateObjectAbove(Firestone, 562, 1136);
	CreateObject(Firestone, 550, 1135);
	CreateObjectAbove(Firestone, 552, 1136);
	CreateObject(Firestone, 342, 1224);
	CreateObject(Firestone, 166, 1260);
	CreateObject(Firestone, 234, 1423);
	CreateObjectAbove(Firestone, 315, 431);
	CreateObject(Firestone, 1359, 1060);
	CreateObject(Firestone, 1348, 1041);
	CreateObject(Firestone, 1384, 1054);
	CreateObject(Firestone, 1417, 1106);
	CreateObject(Firestone, 1432, 1112);
	CreateObject(Firestone, 1436, 1103);
	CreateObject(Firestone, 1340, 919);
	CreateObject(Firestone, 1476, 875);
	CreateObject(Firestone, 1549, 865);
	CreateObject(Firestone, 1607, 791);
	CreateObject(Firestone, 2053, 851);
	CreateObject(Firestone, 2161, 942);
	CreateObject(Firestone, 2073, 861);
	CreateObject(Firestone, 2064, 851);

	var StoneDoor001 = CreateObject(StoneDoor, 940, 1132);
	StoneDoor001->SetComDir(COMD_Down);
	StoneDoor001->MakeInvincible();
	var StoneDoor002 = CreateObject(StoneDoor, 1084, 1132);
	StoneDoor002->SetComDir(COMD_Down);
	StoneDoor002->MakeInvincible();
	var StoneDoor003 = CreateObject(StoneDoor, 564, 436);
	StoneDoor003->SetComDir(COMD_Down);
	StoneDoor003->MakeInvincible();
	var StoneDoor004 = CreateObject(StoneDoor, 843, 716);
	StoneDoor004->SetComDir(COMD_Down);
	StoneDoor004->MakeInvincible();
	var StoneDoor005 = CreateObject(StoneDoor, 1058, 700);
	StoneDoor005->SetComDir(COMD_Down);
	StoneDoor005->MakeInvincible();
	var StoneDoor006 = CreateObject(StoneDoor, 1092, 1028);
	StoneDoor006->SetComDir(COMD_Down);
	StoneDoor006->MakeInvincible();
	var StoneDoor007 = CreateObject(StoneDoor, 1892, 932);
	StoneDoor007->SetComDir(COMD_Down);
	StoneDoor007->MakeInvincible();
	var StoneDoor008 = CreateObject(StoneDoor, 813, 716);
	StoneDoor008->SetComDir(COMD_Down);
	StoneDoor008->MakeInvincible();
	g_last_stone_door = CreateObject(StoneDoor, 781, 716);
	g_last_stone_door->SetComDir(COMD_Down);
	g_last_stone_door->SetClrModulation(0xffa0a0a0);
	g_last_stone_door.StaticSaveVar = "g_last_stone_door";
	var StoneDoor009 = CreateObject(StoneDoor, 692, 748);
	StoneDoor009->SetComDir(COMD_Down);
	StoneDoor009->MakeInvincible();
	var StoneDoor010 = CreateObject(StoneDoor, 684, 332);
	StoneDoor010->SetComDir(COMD_Down);
	StoneDoor010->MakeInvincible();

	var SpinWheel001 = CreateObjectAbove(SpinWheel, 589, 457);
	SpinWheel001->SetMeshMaterial("SpinWheelGearRed", 0);
	SpinWheel001->SetStoneDoor(StoneDoor001);
	var SpinWheel002 = CreateObjectAbove(SpinWheel, 611, 456);
	SpinWheel002->SetMeshMaterial("SpinWheelGearBlue", 0);
	SpinWheel002->SetStoneDoor(StoneDoor002);
	var SpinWheel003 = CreateObjectAbove(SpinWheel, 619, 410);
	SpinWheel003->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel003->SetStoneDoor(StoneDoor003);
	var SpinWheel004 = CreateObject(SpinWheel, 1223, 1545);
	SpinWheel004->SetStoneDoor(StoneDoor005);
	var SpinWheel005 = CreateObjectAbove(SpinWheel, 1117, 1048);
	SpinWheel005->SetStoneDoor(StoneDoor006);
	var SpinWheel006 = CreateObjectAbove(SpinWheel, 2761, 690);
	SpinWheel006->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel006->SetStoneDoor(StoneDoor008);
	var SpinWheel007 = CreateObjectAbove(SpinWheel, 1850, 1463);
	SpinWheel007->SetMeshMaterial("SpinWheelGearRed", 0);
	SpinWheel007->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel007->SetStoneDoor(StoneDoor004);
	var SpinWheel008 = CreateObjectAbove(SpinWheel, 2793, 1521);
	SpinWheel008->SetMeshMaterial("SpinWheelGearRed", 0);
	SpinWheel008->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel008->SetStoneDoor(StoneDoor007);
	var SpinWheel009 = CreateObjectAbove(SpinWheel, 830, 735);
	SpinWheel009->SetStoneDoor(StoneDoor009);
	var SpinWheel010 = CreateObjectAbove(SpinWheel, 703, 352);
	SpinWheel010->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel010->SetStoneDoor(StoneDoor010);

	var Column002 = CreateObject(Column, 779, 488);
	Column002->SetR(180);
	Column002->SetClrModulation(0xffffd0d0);
	Column002->SetMeshMaterial("AncientColumn", 0);
	var Column003 = CreateObject(Column, 1419, 217);
	Column003->SetMeshMaterial("AncientColumn", 0);
	CreateObject(Column, 1386, 616);
	return true;
}
