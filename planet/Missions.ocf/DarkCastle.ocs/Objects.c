/* Automatically created objects file */

static g_shroom1, g_shroom2, g_shroom3, g_shroom4, g_ruin1, g_ruin2, g_ruin3, g_elev1, g_elev2, g_king, g_farmer;

func InitializeObjects()
{
	var Grass01 = CreateObject(Grass, 396, 1149);
	Grass01->SetClrModulation(0xffa08060);
	var Grass02 = CreateObject(Grass, 232, 1181);
	Grass02->SetClrModulation(0xffa08060);
	var Grass03 = CreateObject(Grass, 228, 1180);
	Grass03->SetClrModulation(0xffa08060);

	var Rule_BaseRespawn01 = CreateObject(Rule_BaseRespawn, 0, 0);
	Rule_BaseRespawn01->SetInventoryTransfer(true);
	Rule_BaseRespawn01->SetFreeCrew(true);

	var Tree_Coniferous_Burned01 = CreateObject(Tree_Coniferous_Burned, 17, 1141);
	Tree_Coniferous_Burned01->SetR(10);
	Tree_Coniferous_Burned01->SetPosition(17, 1097);
	var Tree_Coniferous_Burned02 = CreateObject(Tree_Coniferous_Burned, 43, 1156);
	Tree_Coniferous_Burned02->SetCon(75);
	Tree_Coniferous_Burned02->SetR(100);
	Tree_Coniferous_Burned02->SetPosition(43, 1150);

	var Tree_Coniferous01 = CreateObject(Tree_Coniferous, 415, 1161);
	Tree_Coniferous01->SetCon(78);
	Tree_Coniferous01->SetR(10);
	Tree_Coniferous01->SetClrModulation(0xffc08060);
	Tree_Coniferous01->SetPosition(415, 1127);

	var Rank01 = CreateObject(Rank, 241, 1183);
	Rank01->SetR(17);
	Rank01->SetPosition(241, 1180);

	var Fern01 = CreateObject(Fern, 312, 1432);
	Fern01->SetClrModulation(0xffa08060);

	g_shroom1 = CreateObject(LargeCaveMushroom, 1355, 1451);
	g_shroom1->SetClrModulation(0xffcddfdf);
	g_shroom1.StaticSaveVar = "g_shroom1";
	g_shroom2 = CreateObject(LargeCaveMushroom, 1308, 1409);
	g_shroom2->SetR(180);
	g_shroom2->SetClrModulation(0xffdae7dc);
	g_shroom2.StaticSaveVar = "g_shroom2";
	g_shroom2->SetPosition(1308, 1384);
	g_shroom3 = CreateObject(LargeCaveMushroom, 1411, 1447);
	g_shroom3->SetClrModulation(0xffe9d5dd);
	g_shroom3.StaticSaveVar = "g_shroom3";
	g_shroom4 = CreateObject(LargeCaveMushroom, 1420, 1397);
	g_shroom4->SetR(160);
	g_shroom4->SetClrModulation(0xffeaedfb);
	g_shroom4.StaticSaveVar = "g_shroom4";
	g_shroom4->SetPosition(1420, 1374);

	var Rank02 = CreateObject(Rank, 1430, 1423);
	Rank02->SetR(-25);
	Rank02->SetPosition(1430, 1420);

	var Lichen01 = CreateObject(Lichen, 1387, 1440);
	Lichen01->SetAction("Grown");
	var Lichen02 = CreateObject(Lichen, 1310, 1456);
	Lichen02->SetAction("Grown");
	var Lichen03 = CreateObject(Lichen, 1466, 1415);
	Lichen03->SetAction("Grown");

	var Trunk01 = CreateObject(Trunk, 217, 1184);
	Trunk01->SetR(-10);
	Trunk01->SetPosition(217, 1159);

	var EnvPack_Bag01 = CreateObject(EnvPack_Bag, 846, 885);
	EnvPack_Bag01->SetClrModulation(0xffa0a0a0);
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
	g_king->SetName("Horax");
	g_king.MaxEnergy = 200000;
	g_king->DoEnergy(150);
	g_king.StaticSaveVar = "g_king";
	S2AI->AddAI(g_king);
	S2AI->SetHome(g_king, 1568, 413, DIR_Left);
	S2AI->SetGuardRange(g_king, 1268, 263, 600, 300);
	S2AI->SetEncounterCB(g_king, "EncounterKing");
	g_king->SetDir(DIR_Left);

	var Goal_Assassination01 = CreateObject(Goal_Assassination, 0, 0);
	Goal_Assassination01->SetVictim(g_king);

	CreateObject(Rule_TeamAccount, 0, 0);

	CreateObject(Rule_NoPowerNeed, 0, 0);

	g_ruin1 = CreateObject(Ruin_WoodenCabin, 97, 1179);
	g_ruin1->SetR(16);
	g_ruin1.StaticSaveVar = "g_ruin1";
	g_ruin1->SetPosition(97, 1150);

	g_ruin2 = CreateObject(Ruin_Windmill, 353, 1145);
	g_ruin2.StaticSaveVar = "g_ruin2";

	g_ruin3 = CreateObject(Ruin_ChemicalLab, 267, 1180);
	g_ruin3.StaticSaveVar = "g_ruin3";

	CreateObject(Foundry, 238, 1287);

	var Chest02 = CreateObject(Chest, 1473, 1414);
	var Chest06 = CreateObject(Chest, 1574, 583);
	var Chest05 = CreateObject(Chest, 823, 887);
	var Chest01 = CreateObject(Chest, 856, 887);
	var Chest03 = CreateObject(Chest, 1032, 575);
	var Chest04 = CreateObject(Chest, 136, 103);

	var StoneDoor01 = CreateObject(StoneDoor, 940, 671);
	StoneDoor01->SetComDir(COMD_Down);
	var StoneDoor02 = CreateObject(StoneDoor, 1348, 527);
	StoneDoor02->SetComDir(COMD_Down);
	var StoneDoor03 = CreateObject(StoneDoor, 1347, 431);
	StoneDoor03->SetComDir(COMD_Down);

	var SpinWheel01 = CreateObject(SpinWheel, 961, 672);
	SpinWheel01->SetStoneDoor(StoneDoor01);
	var SpinWheel02 = CreateObject(SpinWheel, 1367, 527);
	SpinWheel02->SetStoneDoor(StoneDoor02);
	var SpinWheel03 = CreateObject(SpinWheel, 1384, 471);
	SpinWheel03->SetStoneDoor(StoneDoor03);

	CreateObject(Column, 1197, 551);
	CreateObject(Column, 1218, 463);

	CreateObject(Idol, 1080, 575);

	var Flagpole01 = CreateObject(Flagpole, 135, 1182);
	Flagpole01->SetNeutral(true);

	var SteamEngine01 = CreateObject(SteamEngine, 1529, 585);

	g_elev1 = CreateObject(Elevator, 167, 1184);
	g_elev1.StaticSaveVar = "g_elev1";
	g_elev1->SetClrModulation(0xffa08060);
	g_elev1->CreateShaft(95);
	g_elev1->SetCasePosition(1267);
	g_elev2 = CreateObject(Elevator, 1366, 615);
	g_elev2.StaticSaveVar = "g_elev2";
	g_elev2->CreateShaft(481);
	g_elev2->SetCasePosition(1084);

	var Catapult01 = CreateObject(Catapult, 697, 887);
	Catapult01->SetRDir(4);

	var Lorry01 = CreateObject(Lorry, 149, 1324);
	Lorry01->SetR(24);
	Lorry01->SetPosition(149, 1315);
	var Lorry02 = CreateObject(Lorry, 1425, 1254);
	Lorry02->SetR(-35);
	Lorry02->SetPosition(1425, 1244);

	CreateObject(Airship_Burnt, 38, 1152);

	var Cannon01 = CreateObject(Cannon, 788, 679);
	Cannon01->SetR(30);
	Cannon01->SetPosition(788, 669);
	CreateObject(Cannon, 1004, 471);
	CreateObject(Cannon, 1336, 336);

	var Clonk01 = CreateObject(Clonk, 673, 887);
	Clonk01->SetColor(0xff);
	Clonk01->SetName("Horst");
	S2AI->AddAI(Clonk01);
	S2AI->SetHome(Clonk01, 670, 878, DIR_Left);
	S2AI->SetGuardRange(Clonk01, 400, 800, 500, 150);
	S2AI->SetAllyAlertRange(Clonk01, 60);
	S2AI->SetEncounterCB(Clonk01, "EncounterOutpost");
	Clonk01->SetDir(DIR_Left);
	var Clonk02 = CreateObject(Clonk, 710, 886);
	Clonk02->SetColor(0xff);
	Clonk02->SetName("Hanniball");
	S2AI->AddAI(Clonk02);
	S2AI->SetHome(Clonk02, 709, 877, DIR_Left);
	S2AI->SetGuardRange(Clonk02, 300, 700, 500, 250);
	S2AI->SetAllyAlertRange(Clonk02, 60);
	Clonk02->SetDir(DIR_Left);
	var Clonk03 = CreateObject(Clonk, 781, 670);
	Clonk03->SetDir(DIR_Right);
	Clonk03->SetColor(0xff);
	Clonk03->SetName("Twonky");
	S2AI->AddAI(Clonk03);
	S2AI->SetHome(Clonk03, 781, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk03, 481, 511, 600, 300);
	var Clonk04 = CreateObject(Clonk, 1010, 670);
	Clonk04->SetDir(DIR_Right);
	Clonk04->SetColor(0xff);
	Clonk04->SetName("Sven");
	S2AI->AddAI(Clonk04);
	S2AI->SetHome(Clonk04, 1010, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk04, 710, 511, 600, 300);
	var Clonk05 = CreateObject(Clonk, 985, 670);
	Clonk05->SetDir(DIR_Right);
	Clonk05->SetColor(0xff);
	Clonk05->SetName("Luki");
	S2AI->AddAI(Clonk05);
	S2AI->SetHome(Clonk05, 985, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk05, 685, 511, 600, 300);
	var Clonk06 = CreateObject(Clonk, 1373, 1246);
	Clonk06->SetColor(0xffff0000);
	Clonk06->SetName("Anna");
	S2AI->AddAI(Clonk06);
	S2AI->SetHome(Clonk06, 1370, 1237, DIR_Left);
	S2AI->SetGuardRange(Clonk06, 1150, 1140, 320, 150);
	S2AI->SetAllyAlertRange(Clonk06, 170);
	Clonk06->SetDir(DIR_Left);
	var Clonk07 = CreateObject(Clonk, 1449, 1247);
	Clonk07->SetColor(0xffff0000);
	Clonk07->SetName("Cindy");
	S2AI->AddAI(Clonk07);
	S2AI->SetHome(Clonk07, 1448, 1237, DIR_Left);
	S2AI->SetGuardRange(Clonk07, 1150, 1140, 320, 150);
	S2AI->SetAllyAlertRange(Clonk07, 170);
	S2AI->SetEncounterCB(Clonk07, "EncounterCave");
	Clonk07->SetDir(DIR_Left);
	g_farmer = CreateObject(Clonk, 307, 1167);
	g_farmer->SetColor(0xff0000);
	g_farmer->SetClrModulation(0xffffa020);
	g_farmer->SetName("Farmer");
	g_farmer.StaticSaveVar = "g_farmer";
	g_farmer->SetDir(DIR_Left);
	var Clonk08 = CreateObject(Clonk, 1197, 550);
	Clonk08->SetDir(DIR_Right);
	Clonk08->SetColor(0xff);
	Clonk08->SetName("Sabrina");
	S2AI->AddAI(Clonk08);
	S2AI->SetHome(Clonk08, 1196, 542, DIR_Right);
	S2AI->SetGuardRange(Clonk08, 896, 392, 600, 300);
	var Clonk09 = CreateObject(Clonk, 1266, 551);
	Clonk09->SetColor(0xff);
	Clonk09->SetName("Laura");
	S2AI->AddAI(Clonk09);
	S2AI->SetHome(Clonk09, 1266, 541, DIR_Left);
	S2AI->SetGuardRange(Clonk09, 966, 391, 600, 300);
	Clonk09->SetDir(DIR_Left);
	var Clonk10 = CreateObject(Clonk, 1287, 471);
	Clonk10->SetDir(DIR_Right);
	Clonk10->SetColor(0xff);
	S2AI->AddAI(Clonk10);
	S2AI->SetHome(Clonk10, 1287, 464, DIR_Right);
	S2AI->SetGuardRange(Clonk10, 987, 312, 600, 300);
	var Clonk11 = CreateObject(Clonk, 1092, 574);
	Clonk11->SetDir(DIR_Right);
	Clonk11->SetColor(0xff);
	Clonk11->SetName("Wolfgang");
	S2AI->AddAI(Clonk11);
	S2AI->SetHome(Clonk11, 1092, 567, DIR_Right);
	S2AI->SetGuardRange(Clonk11, 792, 416, 600, 300);
	var Clonk12 = CreateObject(Clonk, 1070, 574);
	Clonk12->SetColor(0xff);
	Clonk12->SetName("Hans");
	S2AI->AddAI(Clonk12);
	S2AI->SetHome(Clonk12, 1069, 566, DIR_Left);
	S2AI->SetGuardRange(Clonk12, 769, 416, 600, 300);
	Clonk12->SetDir(DIR_Left);
	var Clonk13 = CreateObject(Clonk, 1019, 470);
	Clonk13->SetDir(DIR_Right);
	Clonk13->SetColor(0xff);
	Clonk13->SetName("Joki");
	S2AI->AddAI(Clonk13);
	S2AI->SetHome(Clonk13, 1019, 462, DIR_Right);
	S2AI->SetGuardRange(Clonk13, 719, 312, 600, 300);
	var Clonk14 = CreateObject(Clonk, 285, 1182);
	Clonk14->Kill(Clonk14, true);
	Clonk14->SetDir(DIR_Right);
	Clonk14->SetColor(0xffff0000);
	var Clonk16 = CreateObject(Clonk, 208, 1183);
	Clonk16->Kill(Clonk16, true);
	Clonk16->SetDir(DIR_Right);
	Clonk16->SetColor(0xffff0000);

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
	Clonk06->CreateContents(Rock);
	Clonk06->CreateContents(Rock);
	Clonk06->CreateContents(Rock);

	CreateObject(Coal, 59, 1346);
	CreateObject(Coal, 156, 1370);
	CreateObject(Coal, 243, 1555);
	CreateObject(Coal, 61, 1495);
	CreateObject(Coal, 140, 1380);
	SteamEngine01->CreateContents(Coal);
	SteamEngine01->CreateContents(Coal);
	SteamEngine01->CreateContents(Coal);

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
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Lorry01->CreateContents(Wood);
	Chest01->CreateContents(Wood);
	Chest01->CreateContents(Wood);
	Chest01->CreateContents(Wood);
	Chest01->CreateContents(Wood);
	Chest01->CreateContents(Wood);
	CreateObject(Wood, 346, 1456);
	CreateObject(Wood, 336, 1456);
	Lorry02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Chest02->CreateContents(Wood);
	Lorry02->CreateContents(Wood);
	Lorry02->CreateContents(Wood);
	Lorry02->CreateContents(Wood);
	Lorry02->CreateContents(Wood);
	Chest03->CreateContents(Wood);
	Chest03->CreateContents(Wood);
	Chest03->CreateContents(Wood);
	Chest03->CreateContents(Wood);
	Chest03->CreateContents(Wood);
	CreateObject(Wood, 167, 1512);
	CreateObject(Wood, 177, 1512);
	CreateObject(Wood, 511, 1497);

	Lorry01->CreateContents(Loam);
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
	Chest01->CreateContents(Loam);
	Chest01->CreateContents(Loam);
	Chest01->CreateContents(Loam);
	Chest04->CreateContents(Loam);
	Chest04->CreateContents(Loam);
	Chest04->CreateContents(Loam);
	Chest04->CreateContents(Loam);
	Chest04->CreateContents(Loam);

	var GoldBar01 = CreateObject(GoldBar, 1293, 1236);
	GoldBar01->SetR(22);
	GoldBar01->SetPosition(1293, 1235);
	Lorry02->CreateContents(GoldBar);
	Lorry02->CreateContents(GoldBar);

	CreateObject(Airship, 931, 495);

	Clonk01->CreateContents(Sword);
	Clonk07->CreateContents(Sword);
	Clonk04->CreateContents(Sword);
	Clonk05->CreateContents(Sword);
	Clonk12->CreateContents(Sword);
	Clonk10->CreateContents(Sword);
	g_king->CreateContents(Sword);
	Clonk02->CreateContents(Sword);

	var Arrow01 = Clonk01->CreateContents(Arrow);
	Arrow01->SetR(90);
	var Arrow02 = Clonk01->CreateContents(Arrow);
	Arrow02->SetR(90);
	var Arrow03 = Chest05->CreateContents(Arrow);
	Arrow03->SetR(90);
	var Arrow04 = Clonk03->CreateContents(Arrow);
	Arrow04->SetR(90);
	var Arrow05 = Clonk03->CreateContents(Arrow);
	Arrow05->SetR(90);
	var Arrow06 = Clonk13->CreateContents(Arrow);
	Arrow06->SetR(90);
	var Arrow07 = Clonk13->CreateContents(Arrow);
	Arrow07->SetR(90);
	var Arrow08 = g_king->CreateContents(Arrow);
	Arrow08->SetR(90);
	var Arrow09 = g_king->CreateContents(Arrow);
	Arrow09->SetR(90);

	Clonk01->CreateContents(Bow);
	Chest05->CreateContents(Bow);
	Clonk03->CreateContents(Bow);
	Clonk13->CreateContents(Bow);
	g_king->CreateContents(Bow);

	var Boompack01 = CreateObject(Boompack, 135, 1324);
	Boompack01->SetColor(0xff);

	Lorry01->CreateContents(DynamiteBox);
	Lorry02->CreateContents(DynamiteBox);
	Chest02->CreateContents(DynamiteBox);
	Chest02->CreateContents(DynamiteBox);
	Chest04->CreateContents(DynamiteBox);

	CreateObject(Dynamite, 1334, 1224);

	Lorry02->CreateContents(Pickaxe);
	Clonk06->CreateContents(Pickaxe);
	Clonk07->CreateContents(Pickaxe);

	Lorry01->CreateContents(Shovel);

	var Barrel01 = CreateObject(Barrel, 167, 1333);
	Barrel01->SetR(-13);
	Barrel01->SetColor(0xff000000);
	Barrel01->SetPosition(167, 1327);

	var Seaweed01 = CreateObject(Seaweed, 169, 1543);
	Seaweed01->SetPhase(20);
	var Seaweed02 = CreateObject(Seaweed, 815, 1342);
	Seaweed02->SetPhase(20);
	var Seaweed03 = CreateObject(Seaweed, 719, 1078);
	Seaweed03->SetPhase(39);
	var Seaweed04 = CreateObject(Seaweed, 772, 1087);
	Seaweed04->SetPhase(57);
	var Seaweed05 = CreateObject(Seaweed, 1258, 1279);
	Seaweed05->SetPhase(46);
	var Seaweed06 = CreateObject(Seaweed, 847, 1367);
	Seaweed06->SetCon(1);

	CreateObject(Mushroom, 126, 1320);
	CreateObject(Mushroom, 212, 1287);
	CreateObject(Mushroom, 367, 1392);
	CreateObject(Mushroom, 268, 1431);

	Chest06->CreateContents(Musket);

	Chest06->CreateContents(LeadShot);
	Chest06->CreateContents(LeadShot);
	Chest06->CreateContents(LeadShot);

	Clonk09->CreateContents(Javelin);
	Clonk08->CreateContents(Javelin);
	Clonk09->CreateContents(Javelin);
	Clonk08->CreateContents(Javelin);
	Clonk09->CreateContents(Javelin);
	Clonk08->CreateContents(Javelin);

	Clonk12->CreateContents(Shield);
	Clonk10->CreateContents(Shield);
	g_king->CreateContents(Shield);

	Chest03->CreateContents(Bread);
	Chest03->CreateContents(Bread);
	Chest03->CreateContents(Bread);
	Chest04->CreateContents(Bread);
	Chest04->CreateContents(Bread);
	Chest04->CreateContents(Bread);

	CreateObject(EnvPack_ManaAltar, 1052, 471);

	Catapult01->CreateContents(Firestone);
	Catapult01->CreateContents(Firestone);
	Catapult01->CreateContents(Firestone);
	Catapult01->CreateContents(Firestone);
	Catapult01->CreateContents(Firestone);
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
	Clonk06->CreateContents(Firestone);
	Clonk06->CreateContents(Firestone);
	Clonk07->CreateContents(Firestone);
	Chest01->CreateContents(Firestone);
	Chest01->CreateContents(Firestone);
	Chest01->CreateContents(Firestone);
	Clonk11->CreateContents(Firestone);
	Clonk11->CreateContents(Firestone);
	Clonk11->CreateContents(Firestone);
	Clonk11->CreateContents(Firestone);
	Chest03->CreateContents(Firestone);
	Chest03->CreateContents(Firestone);
	Chest03->CreateContents(Firestone);
	g_king->CreateContents(Firestone);
	g_king->CreateContents(Firestone);
	g_king->CreateContents(Firestone);

	return true;
}
