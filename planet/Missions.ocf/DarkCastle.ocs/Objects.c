/* Automatically created objects file */

static g_ruin1, g_ruin2, g_ruin3, g_elev2, g_elev1, g_cannon, g_king, g_farmer, g_cannoneer;

func InitializeObjects()
{
	var Grass001 = CreateObjectAbove(Grass, 396, 1149);
	Grass001->SetClrModulation(0xffa08060);
	var Grass002 = CreateObjectAbove(Grass, 232, 1181);
	Grass002->SetClrModulation(0xffa08060);
	var Grass003 = CreateObjectAbove(Grass, 228, 1180);
	Grass003->SetClrModulation(0xffa08060);


	var Tree_Coniferous_Burned001 = CreateObject(Tree_Coniferous_Burned, 17, 1097);
	Tree_Coniferous_Burned001->SetR(10);

	var Tree_Coniferous001 = CreateObject(Tree_Coniferous, 415, 1117);
	Tree_Coniferous001->SetR(10);
	Tree_Coniferous001->SetClrModulation(0xffc08060);

	var Branch001 = CreateObject(Branch, 241, 1176);
	Branch001->SetR(17);

	var Fern001 = CreateObjectAbove(Fern, 312, 1432);
	Fern001->SetClrModulation(0xffa08060);

	var LargeCaveMushroom001 = CreateObjectAbove(LargeCaveMushroom, 1355, 1451);
	LargeCaveMushroom001->SetClrModulation(0xffcddfdf);
	var LargeCaveMushroom002 = CreateObject(LargeCaveMushroom, 1308, 1384);
	LargeCaveMushroom002->SetR(180);
	LargeCaveMushroom002->SetClrModulation(0xffdae7dc);
	var LargeCaveMushroom003 = CreateObjectAbove(LargeCaveMushroom, 1411, 1447);
	LargeCaveMushroom003->SetClrModulation(0xffe9d5dd);
	var LargeCaveMushroom004 = CreateObject(LargeCaveMushroom, 1420, 1374);
	LargeCaveMushroom004->SetR(160);
	LargeCaveMushroom004->SetClrModulation(0xffeaedfb);

	var Branch002 = CreateObject(Branch, 1430, 1417);
	Branch002->SetR(-25);

	var Lichen001 = CreateObjectAbove(Lichen, 1387, 1440);
	Lichen001->SetAction("Grown");
	var Lichen002 = CreateObjectAbove(Lichen, 1310, 1456);
	Lichen002->SetAction("Grown");
	var Lichen003 = CreateObjectAbove(Lichen, 1466, 1415);
	Lichen003->SetAction("Grown");

	var Trunk001 = CreateObject(Trunk, 217, 1159);
	Trunk001->SetR(-10);

	var EnvPack_Bag001 = CreateObjectAbove(EnvPack_Bag, 846, 885);
	EnvPack_Bag001->SetClrModulation(0xffa0a0a0);
	CreateObjectAbove(EnvPack_Bag, 840, 888);
	CreateObjectAbove(EnvPack_Bag, 844, 888);

	CreateObjectAbove(EnvPack_BridgeRustic, 1096, 673);
	CreateObjectAbove(EnvPack_Candle, 1054, 672);
	CreateObjectAbove(EnvPack_Candle, 1054, 575);
	CreateObjectAbove(EnvPack_Candle, 1185, 616);
	CreateObjectAbove(EnvPack_Candle, 1531, 448);
	CreateObjectAbove(EnvPack_Candle, 1362, 432);
	CreateObjectAbove(EnvPack_CandleSmall, 1556, 432);
	CreateObjectAbove(EnvPack_Crate, 1017, 576);

	CreateObjectAbove(EnvPack_FenceRustic, 1111, 728);
	CreateObjectAbove(EnvPack_FenceRustic, 1089, 735);

	CreateObjectAbove(EnvPack_Guidepost, 315, 1167);

	CreateObjectAbove(EnvPack_Lantern, 894, 488);
	CreateObjectAbove(EnvPack_Lantern, 1291, 472);
	CreateObjectAbove(EnvPack_Painting, 1235, 537);

	CreateObjectAbove(EnvPack_Rail, 1121, 672);

	CreateObjectAbove(EnvPack_Scarecrow, 204, 1185);

	CreateObject(EnvPack_TreeTrunks, 788, 888);

	CreateObjectAbove(EnvPack_WineBarrel, 1438, 552);
	CreateObjectAbove(EnvPack_WineBarrel, 1455, 553);

	CreateObjectAbove(EnvPack_Candle, 1471, 552);
	g_king = CreateObjectAbove(Clonk, 1568, 431);
	g_king->SetDir(DIR_Right);
	g_king->SetCon(200);
	g_king->SetClrModulation(0xffff8000);
	g_king->SetObjectBlitMode(GFX_BLIT_Additive);
	g_king->SetName("Hörx");
	g_king.MaxEnergy = 200000;
	g_king->DoEnergy(150);
	g_king.StaticSaveVar = "g_king";
	AI->AddAI(g_king);
	AI->SetGuardRange(g_king, 1185, 342, 415, 136);
	AI->SetEncounterCB(g_king, "EncounterKing");

	var Goal_Assassination001 = CreateObject(Goal_Assassination);
	Goal_Assassination001->SetVictim(g_king);

	CreateObject(Rule_TeamAccount);

	CreateObject(Rule_NoPowerNeed);

	var LargeCaveMushroom005 = CreateObjectAbove(LargeCaveMushroom, 1334, 1459);
	LargeCaveMushroom005->SetClrModulation(0xffd0dbdf);
	var LargeCaveMushroom006 = CreateObjectAbove(LargeCaveMushroom, 1396, 1451);
	LargeCaveMushroom006->SetClrModulation(0xffe7e6f0);
	var LargeCaveMushroom007 = CreateObjectAbove(LargeCaveMushroom, 1426, 1437);
	LargeCaveMushroom007->SetClrModulation(0xffcfcbe5);

	var Fern002 = CreateObject(Fern, 276, 1442);
	Fern002->SetCon(22);

	g_ruin1 = CreateObject(Ruin_WoodenCabin, 97, 1150);
	g_ruin1->SetR(16);
	g_ruin1.StaticSaveVar = "g_ruin1";

	g_ruin2 = CreateObjectAbove(Ruin_Windmill, 353, 1145);
	g_ruin2.StaticSaveVar = "g_ruin2";

	g_ruin3 = CreateObjectAbove(Ruin_ChemicalLab, 267, 1180);
	g_ruin3.StaticSaveVar = "g_ruin3";

	CreateObjectAbove(Foundry, 238, 1287);

	var Chest002 = CreateObjectAbove(Chest, 1475, 1415);
	var Chest006 = CreateObjectAbove(Chest, 1574, 583);
	var Chest005 = CreateObjectAbove(Chest, 823, 887);
	var Chest001 = CreateObjectAbove(Chest, 856, 887);
	var Chest003 = CreateObjectAbove(Chest, 1032, 575);
	var Chest004 = CreateObjectAbove(Chest, 136, 103);

	var StoneDoor001 = CreateObject(StoneDoor, 940, 652);
	StoneDoor001->SetComDir(COMD_Down);
	var StoneDoor002 = CreateObject(StoneDoor, 1348, 508);
	StoneDoor002->SetComDir(COMD_Down);
	StoneDoor002->DoDamage(170);
	var StoneDoor003 = CreateObject(StoneDoor, 1347, 412);
	StoneDoor003->SetComDir(COMD_Down);
	StoneDoor003->DoDamage(170);

	var SpinWheel001 = CreateObjectAbove(SpinWheel, 961, 672);
	SpinWheel001->SetSwitchTarget(StoneDoor001);
	var SpinWheel002 = CreateObjectAbove(SpinWheel, 1367, 527);
	SpinWheel002->SetSwitchTarget(StoneDoor002);
	var SpinWheel003 = CreateObjectAbove(SpinWheel, 1384, 471);
	SpinWheel003->SetSwitchTarget(StoneDoor003);

	CreateObjectAbove(Column, 1197, 551);
	CreateObjectAbove(Column, 1218, 463);

	CreateObjectAbove(Idol, 1080, 575);

	var SteamEngine001 = CreateObjectAbove(SteamEngine, 1529, 585);

	var Flagpole001 = CreateObjectAbove(Flagpole, 135, 1182);
	Flagpole001->SetNeutral(true);

	g_elev2 = CreateObjectAbove(Elevator, 1366, 614);
	g_elev2.StaticSaveVar = "g_elev2";
	g_elev2->CreateShaft(477);
	g_elev2->SetCasePosition(1079);
	g_elev1 = CreateObjectAbove(Elevator, 167, 1184);
	g_elev1.StaticSaveVar = "g_elev1";
	g_elev1->SetClrModulation(0xffa08060);
	g_elev1->CreateShaft(95);
	g_elev1->SetCasePosition(1267);
	var Catapult001 = CreateObjectAbove(Catapult, 697, 887);
	Catapult001->SetRDir(-7);

	var Lorry001 = CreateObject(Lorry, 149, 1314);
	Lorry001->SetR(24);
	var Lorry002 = CreateObject(Lorry, 1425, 1244);
	Lorry002->SetR(-36);

	CreateObjectAbove(Airship_Burnt, 38, 1152);

	var Cannon001 = CreateObject(Cannon, 788, 669);
	Cannon001->SetR(30);
	g_cannon = CreateObjectAbove(Cannon, 994, 471);
	g_cannon.StaticSaveVar = "g_cannon";
	CreateObjectAbove(Cannon, 1336, 336);

	var Clonk001 = CreateObjectAbove(Clonk, 673, 887);
	Clonk001->SetName("Horst");
	AI->AddAI(Clonk001);
	AI->SetHome(Clonk001, 670, 878, DIR_Left);
	AI->SetGuardRange(Clonk001, 400, 800, 500, 150);
	AI->SetAllyAlertRange(Clonk001, 60);
	AI->SetEncounterCB(Clonk001, "EncounterOutpost");
	Clonk001->SetDir(DIR_Left);
	var Clonk002 = CreateObjectAbove(Clonk, 710, 886);
	Clonk002->SetName("Hanniball");
	AI->AddAI(Clonk002);
	AI->SetHome(Clonk002, 709, 877, DIR_Left);
	AI->SetGuardRange(Clonk002, 634, 809, 233, 104);
	AI->SetAllyAlertRange(Clonk002, 60);
	Clonk002->SetDir(DIR_Left);
	var Clonk003 = CreateObjectAbove(Clonk, 781, 670);
	Clonk003->SetDir(DIR_Right);
	Clonk003->SetName("Twonky");
	AI->AddAI(Clonk003);
	AI->SetHome(Clonk003, 781, 663, DIR_Right);
	AI->SetGuardRange(Clonk003, 481, 511, 600, 300);
	var Clonk004 = CreateObjectAbove(Clonk, 1010, 670);
	Clonk004->SetDir(DIR_Right);
	Clonk004->SetName("Sven");
	AI->AddAI(Clonk004);
	AI->SetHome(Clonk004, 1010, 663, DIR_Right);
	AI->SetGuardRange(Clonk004, 773, 619, 501, 78);
	var Clonk005 = CreateObjectAbove(Clonk, 984, 671);
	Clonk005->SetDir(DIR_Right);
	Clonk005->SetName("Luki");
	AI->AddAI(Clonk005);
	AI->SetHome(Clonk005, 985, 663, DIR_Right);
	AI->SetGuardRange(Clonk005, 773, 621, 596, 146);
	var Clonk006 = CreateObjectAbove(Clonk, 1372, 1244);
	Clonk006->SetColor(0xffff0000);
	Clonk006->SetName("Anna");
	Clonk006->SetSkin(1);
	AI->AddAI(Clonk006);
	AI->SetHome(Clonk006, 1370, 1237, DIR_Left);
	AI->SetGuardRange(Clonk006, 1150, 1140, 320, 150);
	AI->SetAllyAlertRange(Clonk006, 170);
	Clonk006->SetDir(DIR_Left);
	var Clonk007 = CreateObjectAbove(Clonk, 1448, 1246);
	Clonk007->SetColor(0xffff0000);
	Clonk007->SetName("Cindy");
	Clonk007->SetSkin(1);
	AI->AddAI(Clonk007);
	AI->SetGuardRange(Clonk007, 1150, 1140, 320, 150);
	AI->SetAllyAlertRange(Clonk007, 170);
	AI->SetEncounterCB(Clonk007, "EncounterCave");
	Clonk007->SetDir(DIR_Left);
	g_farmer = CreateObjectAbove(Clonk, 307, 1167);
	g_farmer->SetColor(0xff0000);
	g_farmer->SetClrModulation(0xffffa020);
	g_farmer->SetName("Farmer");
	g_farmer.StaticSaveVar = "g_farmer";
	g_farmer->SetDir(DIR_Left);
	var Clonk008 = CreateObjectAbove(Clonk, 1197, 550);
	Clonk008->SetDir(DIR_Right);
	Clonk008->SetName("Sabrina");
	Clonk008->SetSkin(1);
	AI->AddAI(Clonk008);
	AI->SetHome(Clonk008, 1196, 542, DIR_Right);
	AI->SetGuardRange(Clonk008, 955, 472, 415, 299);
	var Clonk009 = CreateObjectAbove(Clonk, 1265, 551);
	Clonk009->SetName("Laura");
	Clonk009->SetSkin(1);
	AI->AddAI(Clonk009);
	AI->SetHome(Clonk009, 1266, 541, DIR_Left);
	AI->SetGuardRange(Clonk009, 966, 391, 383, 283);
	Clonk009->SetDir(DIR_Left);
	var Clonk010 = CreateObjectAbove(Clonk, 1286, 470);
	Clonk010->SetDir(DIR_Right);
	AI->AddAI(Clonk010);
	AI->SetHome(Clonk010, 1287, 464, DIR_Right);
	AI->SetGuardRange(Clonk010, 987, 312, 367, 313);
	var Clonk011 = CreateObjectAbove(Clonk, 1092, 574);
	Clonk011->SetDir(DIR_Right);
	Clonk011->SetName("Wolfgang");
	AI->AddAI(Clonk011);
	AI->SetHome(Clonk011, 1092, 567, DIR_Right);
	AI->SetGuardRange(Clonk011, 792, 416, 600, 300);
	var Clonk012 = CreateObjectAbove(Clonk, 1069, 574);
	Clonk012->SetDir(DIR_Right);
	Clonk012->SetName("Hans");
	AI->AddAI(Clonk012);
	AI->SetHome(Clonk012, 1069, 566, DIR_Right);
	AI->SetGuardRange(Clonk012, 769, 416, 600, 300);
	var Clonk013 = CreateObjectAbove(Clonk, 1018, 470);
	Clonk013->SetDir(DIR_Right);
	Clonk013->SetName("Joki");
	AI->AddAI(Clonk013);
	AI->SetHome(Clonk013, 1019, 462, DIR_Right);
	AI->SetGuardRange(Clonk013, 848, 293, 498, 319);
	var Clonk014 = CreateObjectAbove(Clonk, 285, 1182);
	Clonk014->Kill(Clonk014, true);
	Clonk014->SetDir(DIR_Right);
	Clonk014->SetColor(0xffff0000);
	var Clonk015 = CreateObjectAbove(Clonk, 208, 1183);
	Clonk015->Kill(Clonk015, true);
	Clonk015->SetDir(DIR_Right);
	Clonk015->SetColor(0xffff0000);
	g_cannoneer = CreateObjectAbove(Clonk, 1000, 471);
	g_cannoneer.StaticSaveVar = "g_cannoneer";
	g_cannoneer->SetDir(DIR_Left);
	g_cannoneer->SetName("Brutus");
	g_cannoneer->SetAlternativeSkin("Guard");
	var Clonk016 = CreateObjectAbove(Clonk, 1317, 334);
	Clonk016->SetName("Archibald");
	AI->AddAI(Clonk016);
	AI->SetGuardRange(Clonk016, 978, 231, 391, 244);
	Clonk016->SetDir(DIR_Left);
	CreateObject(Rock, 879, 1002);
	CreateObjectAbove(Rock, 262, 1182);
	CreateObjectAbove(Rock, 140, 1183);
	CreateObjectAbove(Rock, 48, 1151);
	CreateObject(Rock, 154, 1205);
	CreateObject(Rock, 154, 1205);
	CreateObjectAbove(Rock, 241, 1287);
	CreateObject(Rock, 338, 1256);
	CreateObject(Rock, 661, 1392);
	CreateObjectAbove(Rock, 813, 887);
	CreateObject(Rock, 893, 1290);
	CreateObject(Rock, 1248, 1087);
	CreateObject(Rock, 1334, 1011);
	CreateObject(Rock, 1268, 932);
	CreateObject(Rock, 1298, 795);
	CreateObject(Rock, 1501, 932);
	CreateObject(Rock, 1473, 675);
	CreateObject(Rock, 1367, 654);
	CreateObject(Rock, 1505, 1162);
	CreateObject(Rock, 1482, 1049);
	CreateObject(Rock, 1402, 1447);
	CreateObject(Rock, 1025, 1392);
	CreateObject(Rock, 742, 1521);
	CreateObject(Rock, 712, 1350);
	CreateObject(Rock, 1047, 1206);
	Clonk006->CreateContents(Rock, 3);

	CreateObject(Coal, 59, 1345);
	CreateObject(Coal, 156, 1369);
	CreateObject(Coal, 243, 1554);
	CreateObject(Coal, 61, 1494);
	CreateObject(Coal, 140, 1379);
	SteamEngine001->CreateContents(Coal, 3);

	CreateObject(Ore, 227, 1365);
	CreateObjectAbove(Ore, 64, 1421);
	CreateObject(Ore, 264, 1453);
	CreateObject(Ore, 462, 1478);
	CreateObject(Ore, 77, 1485);
	CreateObject(Ore, 1481, 1448);
	CreateObject(Ore, 1438, 1463);
	CreateObject(Ore, 1566, 1561);

	CreateObject(Nugget, 1079, 1216);
	CreateObject(Nugget, 1244, 1138);
	CreateObject(Nugget, 1156, 1163);
	CreateObject(Nugget, 1127, 1165);

	CreateObjectAbove(Wood, 19, 1135);
	CreateObjectAbove(Wood, 749, 1056);
	CreateObjectAbove(Wood, 168, 1512);
	Lorry001->CreateContents(Wood, 30);
	Chest001->CreateContents(Wood, 5);
	CreateObjectAbove(Wood, 346, 1456);
	CreateObjectAbove(Wood, 336, 1456);
	Lorry002->CreateContents(Wood, 5);
	Chest002->CreateContents(Wood, 7);
	Chest003->CreateContents(Wood, 5);
	CreateObjectAbove(Wood, 167, 1512);
	CreateObjectAbove(Wood, 177, 1512);
	CreateObjectAbove(Wood, 511, 1497);

	Lorry001->CreateContents(Loam);
	CreateObjectAbove(Loam, 199, 1287);
	CreateObjectAbove(Loam, 283, 1431);
	CreateObjectAbove(Loam, 372, 1391);
	CreateObjectAbove(Loam, 415, 1431);
	CreateObjectAbove(Loam, 484, 1487);
	CreateObjectAbove(Loam, 511, 1502);
	CreateObject(Loam, 37, 1271);
	CreateObjectAbove(Loam, 200, 1583);
	CreateObjectAbove(Loam, 356, 1559);
	CreateObject(Loam, 314, 1228);
	CreateObject(Loam, 921, 1284);
	CreateObject(Loam, 1042, 1389);
	CreateObject(Loam, 1180, 1575);
	CreateObjectAbove(Loam, 1481, 1415);
	CreateObjectAbove(Loam, 1527, 1406);
	CreateObject(Loam, 958, 980);
	CreateObject(Loam, 1267, 893);
	CreateObject(Loam, 892, 825);
	CreateObjectAbove(Loam, 1393, 1103);
	CreateObject(Loam, 1462, 1076);
	CreateObjectAbove(Loam, 1501, 1415);
	Chest001->CreateContents(Loam, 3);
	Chest004->CreateContents(Loam, 5);

	var GoldBar001 = CreateObject(GoldBar, 1293, 1235);
	GoldBar001->SetR(22);
	Lorry002->CreateContents(GoldBar, 2);

	CreateObjectAbove(Airship, 931, 495);

	Clonk001->CreateContents(Sword);
	Clonk007->CreateContents(Sword);
	Clonk004->CreateContents(Sword);
	Clonk005->CreateContents(Sword);
	Clonk012->CreateContents(Sword);
	Clonk010->CreateContents(Sword);
	g_king->CreateContents(Sword);
	Clonk002->CreateContents(Sword);

	Clonk001->CreateContents(Arrow, 2);
	Chest005->CreateContents(Arrow);
	Clonk013->CreateContents(Arrow, 2);
	var Arrow006 = Clonk003->CreateContents(Arrow);
	Arrow006->SetInfiniteStackCount();

	Clonk001->CreateContents(Bow);
	Chest005->CreateContents(Bow);
	Clonk003->CreateContents(Bow);
	Clonk013->CreateContents(Bow);
	g_king->CreateContents(Bow);

	CreateObjectAbove(Boompack, 135, 1324);
	CreateObjectAbove(Boompack, 1498, 1414);
	CreateObjectAbove(Boompack, 1483, 1414);
	CreateObjectAbove(Boompack, 1491, 1415);
	var Boompack001 = Chest002->CreateContents(Boompack);
	Boompack001->AddRestoreMode(Chest002, 1473, 1403);
	var Boompack002 = Chest002->CreateContents(Boompack);
	Boompack002->AddRestoreMode(Chest002, 1473, 1403);

	Lorry001->CreateContents(DynamiteBox);
	Lorry002->CreateContents(DynamiteBox);
	Chest002->CreateContents(DynamiteBox, 2);
	Chest004->CreateContents(DynamiteBox);

	CreateObjectAbove(Dynamite, 1334, 1224);

	Lorry002->CreateContents(Pickaxe);
	Clonk006->CreateContents(Pickaxe);
	Clonk007->CreateContents(Pickaxe);

	Lorry001->CreateContents(Shovel);

	var Barrel001 = CreateObject(Barrel, 167, 1327);
	Barrel001->SetR(-13);
	Barrel001->SetColor(0xff000000);
	var Barrel002 = Chest002->CreateContents(Barrel);
	Barrel002->SetColor(0xff000000);

	CreateObjectAbove(Seaweed, 169, 1543);
	CreateObjectAbove(Seaweed, 815, 1342);
	CreateObjectAbove(Seaweed, 719, 1078);
	CreateObjectAbove(Seaweed, 772, 1087);
	CreateObjectAbove(Seaweed, 1258, 1279);
	var Seaweed001 = CreateObject(Seaweed, 592, 1425);
	Seaweed001->SetCon(1);
	var Seaweed002 = CreateObject(Seaweed, 652, 1304);
	Seaweed002->SetCon(1);
	var Seaweed003 = CreateObject(Seaweed, 182, 1575);
	Seaweed003->SetCon(1);
	var Seaweed004 = CreateObjectAbove(Seaweed, 353, 1558);
	Seaweed004->SetCon(1);
	var Seaweed005 = CreateObject(Seaweed, 435, 1239);
	Seaweed005->SetCon(1);
	var Seaweed006 = CreateObject(Seaweed, 461, 1252);
	Seaweed006->SetCon(1);
	var Seaweed007 = CreateObject(Seaweed, 490, 1303);
	Seaweed007->SetCon(1);
	var Seaweed008 = CreateObject(Seaweed, 515, 1365);
	Seaweed008->SetCon(1);

	CreateObjectAbove(Mushroom, 126, 1320);
	CreateObjectAbove(Mushroom, 212, 1288);
	CreateObjectAbove(Mushroom, 367, 1392);
	CreateObjectAbove(Mushroom, 268, 1432);
	CreateObject(Mushroom, 247, 1303);
	CreateObject(Mushroom, 384, 1419);
	var Mushroom001 = CreateObject(Mushroom, 184, 1314);
	Mushroom001->SetCon(98);
	var Mushroom002 = CreateObject(Mushroom, 195, 1314);
	Mushroom002->SetCon(95);
	var Mushroom003 = CreateObject(Mushroom, 215, 1315);
	Mushroom003->SetCon(92);
	var Mushroom004 = CreateObject(Mushroom, 205, 1296);
	Mushroom004->SetCon(46);
	var Mushroom005 = CreateObject(Mushroom, 409, 1436);
	Mushroom005->SetCon(33);
	var Mushroom006 = CreateObject(Mushroom, 396, 1410);
	Mushroom006->SetCon(13);

	Chest006->CreateContents(Blunderbuss);

	Chest006->CreateContents(LeadBullet, 3);

	Clonk009->CreateContents(Javelin, 3);
	Clonk008->CreateContents(Javelin, 3);

	Clonk012->CreateContents(Shield);
	Clonk010->CreateContents(Shield);
	g_king->CreateContents(Shield);
	Clonk016->CreateContents(Shield);
	Chest006->CreateContents(Shield);

	Chest003->CreateContents(Bread, 3);
	Chest004->CreateContents(Bread, 3);
	Chest006->CreateContents(Bread, 2);

	CreateObjectAbove(EnvPack_ManaAltar, 1052, 471);

	Chest002->CreateContents(Ropeladder, 2);

	g_king->CreateContents(BombArrow);

	Chest002->CreateContents(WallKit, 2);

	Chest002->CreateContents(Bucket, 2);

	Clonk016->CreateContents(GrenadeLauncher);

	Clonk016->CreateContents(IronBomb, 20);
	Catapult001->CreateContents(Firestone, 5);
	CreateObject(Firestone, 38, 1189);
	CreateObject(Firestone, 101, 1214);
	CreateObject(Firestone, 369, 1281);
	CreateObject(Firestone, 22, 1253);
	CreateObject(Firestone, 376, 1216);
	CreateObject(Firestone, 139, 1346);
	CreateObject(Firestone, 280, 1463);
	CreateObject(Firestone, 451, 1438);
	CreateObject(Firestone, 678, 1364);
	CreateObject(Firestone, 838, 1100);
	CreateObject(Firestone, 880, 1089);
	CreateObject(Firestone, 1022, 1225);
	CreateObject(Firestone, 1338, 1261);
	CreateObject(Firestone, 1144, 1407);
	CreateObject(Firestone, 1051, 1365);
	CreateObject(Firestone, 1328, 1486);
	CreateObject(Firestone, 1467, 1460);
	CreateObject(Firestone, 911, 980);
	CreateObject(Firestone, 1118, 765);
	CreateObject(Firestone, 948, 787);
	CreateObject(Firestone, 781, 910);
	CreateObject(Firestone, 1356, 805);
	CreateObject(Firestone, 1287, 851);
	Clonk006->CreateContents(Firestone, 2);
	Clonk007->CreateContents(Firestone);
	Chest001->CreateContents(Firestone, 3);
	Clonk011->CreateContents(Firestone, 4);
	Chest003->CreateContents(Firestone, 3);
	g_king->CreateContents(Firestone, 3);

	CreateObject(Rule_Gravestones);

	return true;
}
