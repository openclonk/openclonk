/* Automatically created objects file */

static g_ruin1, g_ruin2, g_ruin3, g_elev1, g_elev2, g_cannon, g_king, g_farmer, g_cannoneer;

func InitializeObjects()
{
	var Grass001 = CreateObject(Grass, 396, 1149);
	Grass001->SetClrModulation(0xffa08060);
	var Grass002 = CreateObject(Grass, 232, 1181);
	Grass002->SetClrModulation(0xffa08060);
	var Grass003 = CreateObject(Grass, 228, 1180);
	Grass003->SetClrModulation(0xffa08060);

	var Rule_BaseRespawn001 = CreateObject(Rule_BaseRespawn, 0, 0);
	Rule_BaseRespawn001->SetInventoryTransfer(true);
	Rule_BaseRespawn001->SetFreeCrew(true);

	var Tree_Coniferous_Burned001 = CreateObject(Tree_Coniferous_Burned, 17, 1141);
	Tree_Coniferous_Burned001->SetR(10);
	Tree_Coniferous_Burned001->SetPosition(17, 1097);
	var Tree_Coniferous_Burned002 = CreateObject(Tree_Coniferous_Burned, 43, 1156);
	Tree_Coniferous_Burned002->SetCon(75);
	Tree_Coniferous_Burned002->SetR(100);
	Tree_Coniferous_Burned002->SetPosition(43, 1150);

	var Tree_Coniferous001 = CreateObject(Tree_Coniferous, 415, 1161);
	Tree_Coniferous001->SetR(10);
	Tree_Coniferous001->SetClrModulation(0xffc08060);
	Tree_Coniferous001->SetPosition(415, 1117);

	var Rank001 = CreateObject(Rank, 241, 1183);
	Rank001->SetR(17);
	Rank001->SetPosition(241, 1180);

	var Fern001 = CreateObject(Fern, 312, 1432);
	Fern001->SetClrModulation(0xffa08060);

	var LargeCaveMushroom001 = CreateObject(LargeCaveMushroom, 1355, 1451);
	LargeCaveMushroom001->SetClrModulation(0xffcddfdf);
	var LargeCaveMushroom002 = CreateObject(LargeCaveMushroom, 1308, 1409);
	LargeCaveMushroom002->SetR(180);
	LargeCaveMushroom002->SetClrModulation(0xffdae7dc);
	LargeCaveMushroom002->SetPosition(1308, 1384);
	var LargeCaveMushroom003 = CreateObject(LargeCaveMushroom, 1411, 1447);
	LargeCaveMushroom003->SetClrModulation(0xffe9d5dd);
	var LargeCaveMushroom004 = CreateObject(LargeCaveMushroom, 1420, 1397);
	LargeCaveMushroom004->SetR(160);
	LargeCaveMushroom004->SetClrModulation(0xffeaedfb);
	LargeCaveMushroom004->SetPosition(1420, 1374);

	var Rank002 = CreateObject(Rank, 1430, 1423);
	Rank002->SetR(-25);
	Rank002->SetPosition(1430, 1420);

	var Lichen001 = CreateObject(Lichen, 1387, 1440);
	Lichen001->SetAction("Grown");
	var Lichen002 = CreateObject(Lichen, 1310, 1456);
	Lichen002->SetAction("Grown");
	var Lichen003 = CreateObject(Lichen, 1466, 1415);
	Lichen003->SetAction("Grown");

	var Trunk001 = CreateObject(Trunk, 217, 1184);
	Trunk001->SetR(-10);
	Trunk001->SetPosition(217, 1159);

	var EnvPack_Bag001 = CreateObject(EnvPack_Bag, 846, 885);
	EnvPack_Bag001->SetClrModulation(0xffa0a0a0);
	CreateObject(EnvPack_Bag, 840, 888);
	CreateObject(EnvPack_Bag, 844, 888);

	CreateObject(EnvPack_BridgeRustic, 1096, 673);

	CreateObject(EnvPack_Candle, 1054, 672);

	CreateObject(EnvPack_Candle, 1054, 575);

	CreateObject(EnvPack_Candle, 1185, 616);

	CreateObject(EnvPack_Candle, 1531, 448);

	CreateObject(EnvPack_Candle, 1362, 432);

	CreateObject(EnvPack_CandleSmall, 1556, 432);

	CreateObject(EnvPack_Crate, 1017, 576);

	CreateObject(EnvPack_FenceRustic, 1111, 728);
	CreateObject(EnvPack_FenceRustic, 1089, 735);

	CreateObject(EnvPack_Guidepost, 315, 1167);

	CreateObject(EnvPack_Lantern, 894, 488);

	CreateObject(EnvPack_Lantern, 1291, 472);

	CreateObject(EnvPack_Painting, 1235, 537);

	CreateObject(EnvPack_Rail, 1121, 672);

	CreateObject(EnvPack_Scarecrow, 204, 1185);

	CreateObject(EnvPack_TreeTrunks, 788, 888);

	CreateObject(EnvPack_WineBarrel, 1438, 552);
	CreateObject(EnvPack_WineBarrel, 1455, 553);

	CreateObject(EnvPack_Candle, 1471, 552);

	g_king = CreateObject(Clonk, 1569, 430);
	g_king->SetCon(200);
	g_king->SetColor(0xff);
	g_king->SetClrModulation(0xffff8000);
	g_king->SetObjectBlitMode(GFX_BLIT_Additive);
	g_king->SetName("$NameHorax$");
	g_king.MaxEnergy = 200000;
	g_king->DoEnergy(150);
	g_king.StaticSaveVar = "g_king";
	S2AI->AddAI(g_king);
	S2AI->SetHome(g_king, 1568, 413, DIR_Left);
	S2AI->SetGuardRange(g_king, 1268, 263, 600, 300);
	S2AI->SetEncounterCB(g_king, "EncounterKing");
	g_king->SetDir(DIR_Left);

	var Goal_Assassination001 = CreateObject(Goal_Assassination, 0, 0);
	Goal_Assassination001->SetVictim(g_king);

	CreateObject(Rule_TeamAccount, 0, 0);

	CreateObject(Rule_NoPowerNeed, 0, 0);

	var LargeCaveMushroom005 = CreateObject(LargeCaveMushroom, 1334, 1459);
	LargeCaveMushroom005->SetClrModulation(0xffd0dbdf);
	var LargeCaveMushroom006 = CreateObject(LargeCaveMushroom, 1396, 1451);
	LargeCaveMushroom006->SetClrModulation(0xffe7e6f0);
	var LargeCaveMushroom007 = CreateObject(LargeCaveMushroom, 1426, 1437);
	LargeCaveMushroom007->SetClrModulation(0xffcfcbe5);

	g_ruin1 = CreateObject(Ruin_WoodenCabin, 97, 1179);
	g_ruin1->SetR(16);
	g_ruin1.StaticSaveVar = "g_ruin1";
	g_ruin1->SetPosition(97, 1150);

	g_ruin2 = CreateObject(Ruin_Windmill, 353, 1145);
	g_ruin2.StaticSaveVar = "g_ruin2";

	g_ruin3 = CreateObject(Ruin_ChemicalLab, 267, 1180);
	g_ruin3.StaticSaveVar = "g_ruin3";

	CreateObject(Foundry, 238, 1287);

	var Chest001 = CreateObject(Chest, 1474, 1414);
	Chest001->SetXDir(3);
	var Chest006 = CreateObject(Chest, 1574, 583);
	var Chest005 = CreateObject(Chest, 823, 887);
	var Chest002 = CreateObject(Chest, 856, 887);
	var Chest003 = CreateObject(Chest, 1032, 575);
	var Chest004 = CreateObject(Chest, 136, 103);

	var StoneDoor001 = CreateObject(StoneDoor, 940, 671);
	StoneDoor001->SetComDir(COMD_Down);
	var StoneDoor002 = CreateObject(StoneDoor, 1348, 527);
	StoneDoor002->SetComDir(COMD_Down);
	var StoneDoor003 = CreateObject(StoneDoor, 1347, 431);
	StoneDoor003->SetComDir(COMD_Down);

	var SpinWheel001 = CreateObject(SpinWheel, 961, 672);
	SpinWheel001->SetStoneDoor(StoneDoor001);
	var SpinWheel002 = CreateObject(SpinWheel, 1367, 527);
	SpinWheel002->SetStoneDoor(StoneDoor002);
	var SpinWheel003 = CreateObject(SpinWheel, 1384, 471);
	SpinWheel003->SetStoneDoor(StoneDoor003);

	CreateObject(Column, 1197, 551);
	CreateObject(Column, 1218, 463);

	CreateObject(Idol, 1080, 575);

	var Flagpole001 = CreateObject(Flagpole, 135, 1182);
	Flagpole001->SetNeutral(true);

	var SteamEngine001 = CreateObject(SteamEngine, 1529, 585);

	g_elev1 = CreateObject(Elevator, 167, 1184);
	g_elev1.StaticSaveVar = "g_elev1";
	g_elev1->SetClrModulation(0xffa08060);
	g_elev1->CreateShaft(95);
	g_elev1->SetCasePosition(1267);
	g_elev2 = CreateObject(Elevator, 1366, 615);
	g_elev2.StaticSaveVar = "g_elev2";
	g_elev2->CreateShaft(481);
	g_elev2->SetCasePosition(1084);

	var Catapult001 = CreateObject(Catapult, 697, 887);
	Catapult001->SetRDir(4);

	var Lorry001 = CreateObject(Lorry, 149, 1323);
	Lorry001->SetR(24);
	Lorry001->SetPosition(149, 1314);
	var Lorry002 = CreateObject(Lorry, 1425, 1254);
	Lorry002->SetR(-35);
	Lorry002->SetPosition(1425, 1244);

	CreateObject(Airship_Burnt, 38, 1152);

	var Cannon001 = CreateObject(Cannon, 788, 679);
	Cannon001->SetR(30);
	Cannon001->SetPosition(788, 669);
	g_cannon = CreateObject(Cannon, 994, 471);
	g_cannon.StaticSaveVar = "g_cannon";
	CreateObject(Cannon, 1336, 336);

	var Clonk001 = CreateObject(Clonk, 673, 887);
	Clonk001->SetColor(0xff);
	Clonk001->SetName("Horst");
	S2AI->AddAI(Clonk001);
	S2AI->SetHome(Clonk001, 670, 878, DIR_Left);
	S2AI->SetGuardRange(Clonk001, 400, 800, 500, 150);
	S2AI->SetAllyAlertRange(Clonk001, 60);
	S2AI->SetEncounterCB(Clonk001, "EncounterOutpost");
	Clonk001->SetDir(DIR_Left);
	var Clonk002 = CreateObject(Clonk, 710, 886);
	Clonk002->SetColor(0xff);
	Clonk002->SetName("Hanniball");
	S2AI->AddAI(Clonk002);
	S2AI->SetHome(Clonk002, 709, 877, DIR_Left);
	S2AI->SetGuardRange(Clonk002, 300, 700, 500, 250);
	S2AI->SetAllyAlertRange(Clonk002, 60);
	Clonk002->SetDir(DIR_Left);
	var Clonk003 = CreateObject(Clonk, 781, 670);
	Clonk003->SetDir(DIR_Right);
	Clonk003->SetColor(0xff);
	Clonk003->SetName("Twonky");
	S2AI->AddAI(Clonk003);
	S2AI->SetHome(Clonk003, 781, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk003, 481, 511, 600, 300);
	var Clonk004 = CreateObject(Clonk, 1010, 670);
	Clonk004->SetDir(DIR_Right);
	Clonk004->SetColor(0xff);
	Clonk004->SetName("Sven");
	S2AI->AddAI(Clonk004);
	S2AI->SetHome(Clonk004, 1010, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk004, 710, 511, 600, 300);
	var Clonk005 = CreateObject(Clonk, 985, 670);
	Clonk005->SetDir(DIR_Right);
	Clonk005->SetColor(0xff);
	Clonk005->SetName("Luki");
	S2AI->AddAI(Clonk005);
	S2AI->SetHome(Clonk005, 985, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk005, 685, 511, 600, 300);
	var Clonk006 = CreateObject(Clonk, 1373, 1245);
	Clonk006->SetColor(0xffff0000);
	Clonk006->SetName("Anna");
	S2AI->AddAI(Clonk006);
	S2AI->SetHome(Clonk006, 1370, 1237, DIR_Left);
	S2AI->SetGuardRange(Clonk006, 1150, 1140, 320, 150);
	S2AI->SetAllyAlertRange(Clonk006, 170);
	Clonk006->SetDir(DIR_Left);
	var Clonk007 = CreateObject(Clonk, 1449, 1246);
	Clonk007->SetColor(0xffff0000);
	Clonk007->SetName("Cindy");
	S2AI->AddAI(Clonk007);
	S2AI->SetHome(Clonk007, 1448, 1237, DIR_Left);
	S2AI->SetGuardRange(Clonk007, 1150, 1140, 320, 150);
	S2AI->SetAllyAlertRange(Clonk007, 170);
	S2AI->SetEncounterCB(Clonk007, "EncounterCave");
	Clonk007->SetDir(DIR_Left);
	g_farmer = CreateObject(Clonk, 307, 1167);
	g_farmer->SetColor(0xff0000);
	g_farmer->SetClrModulation(0xffffa020);
	g_farmer->SetName("Farmer");
	g_farmer.StaticSaveVar = "g_farmer";
	g_farmer->SetDir(DIR_Left);
	var Clonk008 = CreateObject(Clonk, 1197, 550);
	Clonk008->SetDir(DIR_Right);
	Clonk008->SetColor(0xff);
	Clonk008->SetName("Sabrina");
	S2AI->AddAI(Clonk008);
	S2AI->SetHome(Clonk008, 1196, 542, DIR_Right);
	S2AI->SetGuardRange(Clonk008, 896, 392, 600, 300);
	var Clonk009 = CreateObject(Clonk, 1266, 551);
	Clonk009->SetColor(0xff);
	Clonk009->SetName("Laura");
	S2AI->AddAI(Clonk009);
	S2AI->SetHome(Clonk009, 1266, 541, DIR_Left);
	S2AI->SetGuardRange(Clonk009, 966, 391, 600, 300);
	Clonk009->SetDir(DIR_Left);
	var Clonk010 = CreateObject(Clonk, 1287, 471);
	Clonk010->SetDir(DIR_Right);
	Clonk010->SetColor(0xff);
	S2AI->AddAI(Clonk010);
	S2AI->SetHome(Clonk010, 1287, 464, DIR_Right);
	S2AI->SetGuardRange(Clonk010, 987, 312, 600, 300);
	var Clonk011 = CreateObject(Clonk, 1092, 574);
	Clonk011->SetDir(DIR_Right);
	Clonk011->SetColor(0xff);
	Clonk011->SetName("Wolfgang");
	S2AI->AddAI(Clonk011);
	S2AI->SetHome(Clonk011, 1092, 567, DIR_Right);
	S2AI->SetGuardRange(Clonk011, 792, 416, 600, 300);
	var Clonk012 = CreateObject(Clonk, 1070, 574);
	Clonk012->SetColor(0xff);
	Clonk012->SetName("Hans");
	S2AI->AddAI(Clonk012);
	S2AI->SetHome(Clonk012, 1069, 566, DIR_Left);
	S2AI->SetGuardRange(Clonk012, 769, 416, 600, 300);
	Clonk012->SetDir(DIR_Left);
	var Clonk013 = CreateObject(Clonk, 1018, 470);
	Clonk013->SetDir(DIR_Right);
	Clonk013->SetColor(0xff);
	Clonk013->SetName("Joki");
	S2AI->AddAI(Clonk013);
	S2AI->SetHome(Clonk013, 1019, 462, DIR_Right);
	S2AI->SetGuardRange(Clonk013, 719, 312, 600, 300);
	var Clonk014 = CreateObject(Clonk, 285, 1182);
	Clonk014->Kill(Clonk014, true);
	Clonk014->SetDir(DIR_Right);
	Clonk014->SetColor(0xffff0000);
	var Clonk015 = CreateObject(Clonk, 208, 1183);
	Clonk015->Kill(Clonk015, true);
	Clonk015->SetDir(DIR_Right);
	Clonk015->SetColor(0xffff0000);
	g_cannoneer = CreateObject(Clonk, 995, 471);
	g_cannoneer->SetColor(0xff);
	g_cannoneer.StaticSaveVar = "g_cannoneer";
	g_cannoneer->SetDir(DIR_Left);

	CreateObject(Rock, 879, 1003);
	CreateObject(Rock, 262, 1182);
	CreateObject(Rock, 140, 1183);
	CreateObject(Rock, 48, 1151);
	CreateObject(Rock, 154, 1206);
	CreateObject(Rock, 154, 1206);
	CreateObject(Rock, 241, 1287);
	CreateObject(Rock, 338, 1257);
	CreateObject(Rock, 661, 1393);
	CreateObject(Rock, 813, 887);
	CreateObject(Rock, 893, 1291);
	CreateObject(Rock, 1248, 1088);
	CreateObject(Rock, 1334, 1012);
	CreateObject(Rock, 1268, 933);
	CreateObject(Rock, 1296, 795);
	CreateObject(Rock, 1501, 933);
	CreateObject(Rock, 1473, 676);
	CreateObject(Rock, 1367, 655);
	CreateObject(Rock, 1505, 1163);
	CreateObject(Rock, 1482, 1050);
	CreateObject(Rock, 1402, 1448);
	CreateObject(Rock, 1025, 1393);
	CreateObject(Rock, 742, 1522);
	CreateObject(Rock, 712, 1351);
	CreateObject(Rock, 1047, 1207);
	Clonk006->CreateContents(Rock);
	Clonk006->CreateContents(Rock);
	Clonk006->CreateContents(Rock);

	CreateObject(Coal, 59, 1346);
	CreateObject(Coal, 156, 1370);
	CreateObject(Coal, 243, 1555);
	CreateObject(Coal, 61, 1495);
	CreateObject(Coal, 140, 1380);
	SteamEngine001->CreateContents(Coal);
	SteamEngine001->CreateContents(Coal);
	SteamEngine001->CreateContents(Coal);

	CreateObject(Ore, 227, 1366);
	CreateObject(Ore, 64, 1421);
	CreateObject(Ore, 264, 1454);
	CreateObject(Ore, 462, 1479);
	CreateObject(Ore, 77, 1486);
	CreateObject(Ore, 1481, 1449);
	CreateObject(Ore, 1438, 1464);
	CreateObject(Ore, 1566, 1562);

	CreateObject(Nugget, 1079, 1217);
	CreateObject(Nugget, 1244, 1139);
	CreateObject(Nugget, 1156, 1164);
	CreateObject(Nugget, 1127, 1166);

	CreateObject(Wood, 19, 1135);
	CreateObject(Wood, 749, 1056);
	CreateObject(Wood, 168, 1512);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Lorry001->CreateContents(Wood);
	Chest002->CreateContents(Wood);
	Chest002->CreateContents(Wood);
	Chest002->CreateContents(Wood);
	Chest002->CreateContents(Wood);
	Chest002->CreateContents(Wood);
	CreateObject(Wood, 346, 1456);
	CreateObject(Wood, 336, 1456);
	Lorry002->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Chest001->CreateContents(Wood);
	Lorry002->CreateContents(Wood);
	Lorry002->CreateContents(Wood);
	Lorry002->CreateContents(Wood);
	Lorry002->CreateContents(Wood);
	Chest003->CreateContents(Wood);
	Chest003->CreateContents(Wood);
	Chest003->CreateContents(Wood);
	Chest003->CreateContents(Wood);
	Chest003->CreateContents(Wood);
	CreateObject(Wood, 167, 1512);
	CreateObject(Wood, 177, 1512);
	CreateObject(Wood, 511, 1497);

	Lorry001->CreateContents(Loam);
	CreateObject(Loam, 199, 1287);
	CreateObject(Loam, 283, 1431);
	CreateObject(Loam, 372, 1391);
	CreateObject(Loam, 415, 1431);
	CreateObject(Loam, 484, 1487);
	CreateObject(Loam, 511, 1502);
	CreateObject(Loam, 37, 1274);
	CreateObject(Loam, 200, 1583);
	CreateObject(Loam, 356, 1559);
	CreateObject(Loam, 314, 1231);
	CreateObject(Loam, 921, 1287);
	CreateObject(Loam, 1042, 1392);
	CreateObject(Loam, 1180, 1578);
	CreateObject(Loam, 1481, 1415);
	CreateObject(Loam, 1527, 1406);
	CreateObject(Loam, 958, 983);
	CreateObject(Loam, 1267, 896);
	CreateObject(Loam, 892, 828);
	CreateObject(Loam, 1393, 1103);
	CreateObject(Loam, 1462, 1079);
	CreateObject(Loam, 1501, 1415);
	Chest002->CreateContents(Loam);
	Chest002->CreateContents(Loam);
	Chest002->CreateContents(Loam);
	Chest004->CreateContents(Loam);
	Chest004->CreateContents(Loam);
	Chest004->CreateContents(Loam);
	Chest004->CreateContents(Loam);
	Chest004->CreateContents(Loam);

	var GoldBar001 = CreateObject(GoldBar, 1293, 1236);
	GoldBar001->SetR(22);
	GoldBar001->SetPosition(1293, 1235);
	Lorry002->CreateContents(GoldBar);
	Lorry002->CreateContents(GoldBar);

	CreateObject(Airship, 931, 495);

	Clonk001->CreateContents(Sword);
	Clonk007->CreateContents(Sword);
	Clonk004->CreateContents(Sword);
	Clonk005->CreateContents(Sword);
	Clonk012->CreateContents(Sword);
	Clonk010->CreateContents(Sword);
	g_king->CreateContents(Sword);
	Clonk002->CreateContents(Sword);

	var Arrow001 = Clonk001->CreateContents(Arrow);
	Arrow001->SetR(90);
	var Arrow002 = Clonk001->CreateContents(Arrow);
	Arrow002->SetR(90);
	var Arrow003 = Chest005->CreateContents(Arrow);
	Arrow003->SetR(90);
	var Arrow004 = Clonk003->CreateContents(Arrow);
	Arrow004->SetR(90);
	var Arrow005 = Clonk003->CreateContents(Arrow);
	Arrow005->SetR(90);
	var Arrow006 = Clonk013->CreateContents(Arrow);
	Arrow006->SetR(90);
	var Arrow007 = Clonk013->CreateContents(Arrow);
	Arrow007->SetR(90);
	var Arrow008 = g_king->CreateContents(Arrow);
	Arrow008->SetR(90);
	var Arrow009 = g_king->CreateContents(Arrow);
	Arrow009->SetR(90);

	Clonk001->CreateContents(Bow);
	Chest005->CreateContents(Bow);
	Clonk003->CreateContents(Bow);
	Clonk013->CreateContents(Bow);
	g_king->CreateContents(Bow);

	var Boompack001 = CreateObject(Boompack, 135, 1324);
	Boompack001->SetColor(0xff);
	var Boompack002 = CreateObject(Boompack, 1498, 1414);
	Boompack002->SetColor(0xff);
	var Boompack003 = CreateObject(Boompack, 1483, 1414);
	Boompack003->SetColor(0xff);
	var Boompack004 = CreateObject(Boompack, 1491, 1415);
	Boompack004->SetColor(0xff);
	var Boompack005 = Chest001->CreateContents(Boompack);
	Boompack005->AddRestoreMode(Chest001, 1473, 1403);
	Boompack005->SetColor(0xff);
	var Boompack006 = Chest001->CreateContents(Boompack);
	Boompack006->AddRestoreMode(Chest001, 1473, 1403);
	Boompack006->SetColor(0xff);

	Lorry001->CreateContents(DynamiteBox);
	Lorry002->CreateContents(DynamiteBox);
	Chest001->CreateContents(DynamiteBox);
	Chest001->CreateContents(DynamiteBox);
	Chest004->CreateContents(DynamiteBox);

	CreateObject(Dynamite, 1334, 1224);

	Lorry002->CreateContents(Pickaxe);
	Clonk006->CreateContents(Pickaxe);
	Clonk007->CreateContents(Pickaxe);

	Lorry001->CreateContents(Shovel);

	var Barrel001 = CreateObject(Barrel, 167, 1333);
	Barrel001->SetR(-13);
	Barrel001->SetColor(0xff000000);
	Barrel001->SetPosition(167, 1327);

	var Seaweed001 = CreateObject(Seaweed, 169, 1543);
	Seaweed001->SetPhase(5);
	var Seaweed002 = CreateObject(Seaweed, 815, 1342);
	Seaweed002->SetPhase(5);
	var Seaweed003 = CreateObject(Seaweed, 719, 1078);
	Seaweed003->SetPhase(24);
	var Seaweed004 = CreateObject(Seaweed, 772, 1087);
	Seaweed004->SetPhase(42);
	var Seaweed005 = CreateObject(Seaweed, 1258, 1279);
	Seaweed005->SetPhase(31);
	var Seaweed006 = CreateObject(Seaweed, 847, 1367);
	Seaweed006->SetCon(1);
	var Seaweed007 = CreateObject(Seaweed, 793, 1080);
	Seaweed007->SetCon(1);
	var Seaweed008 = CreateObject(Seaweed, 568, 1463);
	Seaweed008->SetCon(1);
	var Seaweed009 = CreateObject(Seaweed, 361, 1558);
	Seaweed009->SetCon(1);
	var Seaweed010 = CreateObject(Seaweed, 438, 1238);
	Seaweed010->SetCon(1);
	var Seaweed011 = CreateObject(Seaweed, 733, 1087);
	Seaweed011->SetCon(1);
	var Seaweed012 = CreateObject(Seaweed, 503, 1325);
	Seaweed012->SetCon(1);
	var Seaweed013 = CreateObject(Seaweed, 568, 1463);
	Seaweed013->SetCon(1);
	var Seaweed014 = CreateObject(Seaweed, 564, 1461);
	Seaweed014->SetCon(1);
	var Seaweed015 = CreateObject(Seaweed, 461, 1247);
	Seaweed015->SetCon(1);
	var Seaweed016 = CreateObject(Seaweed, 568, 1463);
	Seaweed016->SetCon(1);
	var Seaweed017 = CreateObject(Seaweed, 563, 1461);
	Seaweed017->SetCon(1);
	Seaweed017->SetXDir(-37);

	CreateObject(Mushroom, 126, 1320);
	CreateObject(Mushroom, 212, 1288);
	CreateObject(Mushroom, 367, 1392);
	CreateObject(Mushroom, 268, 1432);
	var Mushroom001 = CreateObject(Mushroom, 247, 1292);
	Mushroom001->SetCon(22);
	var Mushroom002 = CreateObject(Mushroom, 384, 1400);
	Mushroom002->SetCon(9);
	var Mushroom003 = CreateObject(Mushroom, 184, 1294);
	Mushroom003->SetCon(8);
	var Mushroom004 = CreateObject(Mushroom, 195, 1293);
	Mushroom004->SetCon(4);
	var Mushroom005 = CreateObject(Mushroom, 215, 1294);
	Mushroom005->SetCon(1);

	Chest006->CreateContents(Musket);

	Chest006->CreateContents(LeadShot);
	Chest006->CreateContents(LeadShot);
	Chest006->CreateContents(LeadShot);

	Clonk009->CreateContents(Javelin);
	Clonk008->CreateContents(Javelin);
	Clonk009->CreateContents(Javelin);
	Clonk008->CreateContents(Javelin);
	Clonk009->CreateContents(Javelin);
	Clonk008->CreateContents(Javelin);

	Clonk012->CreateContents(Shield);
	Clonk010->CreateContents(Shield);
	g_king->CreateContents(Shield);

	Chest003->CreateContents(Bread);
	Chest003->CreateContents(Bread);
	Chest003->CreateContents(Bread);
	Chest004->CreateContents(Bread);
	Chest004->CreateContents(Bread);
	Chest004->CreateContents(Bread);

	CreateObject(EnvPack_ManaAltar, 1052, 471);

	Chest001->CreateContents(Ropeladder);
	Chest001->CreateContents(Ropeladder);

	Catapult001->CreateContents(Firestone);
	Catapult001->CreateContents(Firestone);
	Catapult001->CreateContents(Firestone);
	Catapult001->CreateContents(Firestone);
	Catapult001->CreateContents(Firestone);
	CreateObject(Firestone, 38, 1190);
	CreateObject(Firestone, 101, 1215);
	CreateObject(Firestone, 369, 1282);
	CreateObject(Firestone, 22, 1254);
	CreateObject(Firestone, 376, 1217);
	CreateObject(Firestone, 139, 1347);
	CreateObject(Firestone, 280, 1464);
	CreateObject(Firestone, 451, 1439);
	CreateObject(Firestone, 678, 1365);
	CreateObject(Firestone, 838, 1101);
	CreateObject(Firestone, 880, 1090);
	CreateObject(Firestone, 1022, 1226);
	CreateObject(Firestone, 1338, 1262);
	CreateObject(Firestone, 1144, 1408);
	CreateObject(Firestone, 1051, 1366);
	CreateObject(Firestone, 1328, 1487);
	CreateObject(Firestone, 1467, 1461);
	CreateObject(Firestone, 911, 981);
	CreateObject(Firestone, 1118, 766);
	CreateObject(Firestone, 948, 788);
	CreateObject(Firestone, 781, 911);
	CreateObject(Firestone, 1356, 806);
	CreateObject(Firestone, 1287, 852);
	Clonk006->CreateContents(Firestone);
	Clonk006->CreateContents(Firestone);
	Clonk007->CreateContents(Firestone);
	Chest002->CreateContents(Firestone);
	Chest002->CreateContents(Firestone);
	Chest002->CreateContents(Firestone);
	Clonk011->CreateContents(Firestone);
	Clonk011->CreateContents(Firestone);
	Clonk011->CreateContents(Firestone);
	Clonk011->CreateContents(Firestone);
	Chest003->CreateContents(Firestone);
	Chest003->CreateContents(Firestone);
	Chest003->CreateContents(Firestone);
	g_king->CreateContents(Firestone);
	g_king->CreateContents(Firestone);
	g_king->CreateContents(Firestone);

	return true;
}
