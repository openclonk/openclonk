/* Automatically created objects file */

static g_shroom1, g_shroom2, g_shroom3, g_shroom4, g_ruin1, g_ruin2, g_ruin3, g_elev1, g_elev2, g_farmer, g_king;

func InitializeObjects()
{
	var Grass0001 = CreateObject(Grass, 396, 1149);
	Grass0001->SetClrModulation(0xffa08060);
	var Grass0002 = CreateObject(Grass, 232, 1181);
	Grass0002->SetClrModulation(0xffa08060);
	var Grass0003 = CreateObject(Grass, 228, 1180);
	Grass0003->SetClrModulation(0xffa08060);

	var Tree_Coniferous_Burned0004 = CreateObject(Tree_Coniferous_Burned, 17, 1141);
	Tree_Coniferous_Burned0004->SetR(10);
	Tree_Coniferous_Burned0004->SetPosition(17, 1097);
	var Tree_Coniferous_Burned0005 = CreateObject(Tree_Coniferous_Burned, 43, 1156);
	Tree_Coniferous_Burned0005->SetCon(75);
	Tree_Coniferous_Burned0005->SetR(100);
	Tree_Coniferous_Burned0005->SetPosition(43, 1150);

	var Tree_Coniferous0006 = CreateObject(Tree_Coniferous, 415, 1161);
	Tree_Coniferous0006->SetCon(75);
	Tree_Coniferous0006->SetR(10);
	Tree_Coniferous0006->SetClrModulation(0xffc08060);
	Tree_Coniferous0006->SetPosition(415, 1129);

	var Rank0010 = CreateObject(Rank, 241, 1183);
	Rank0010->SetR(17);
	Rank0010->SetPosition(241, 1180);

	var Fern0011 = CreateObject(Fern, 312, 1432);
	Fern0011->SetClrModulation(0xffa08060);

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

	var Rank0031 = CreateObject(Rank, 1430, 1423);
	Rank0031->SetR(-25);
	Rank0031->SetPosition(1430, 1420);

	var Lichen0032 = CreateObject(Lichen, 1387, 1439);
	Lichen0032->SetAction("Grown");
	var Lichen0035 = CreateObject(Lichen, 1310, 1455);
	Lichen0035->SetAction("Grown");
	var Lichen0038 = CreateObject(Lichen, 1466, 1415);
	Lichen0038->SetAction("Grown");

	var Trunk0041 = CreateObject(Trunk, 217, 1184);
	Trunk0041->SetR(-10);
	Trunk0041->SetPosition(217, 1159);

	var EnvPack_Bag0042 = CreateObject(EnvPack_Bag, 846, 885);
	EnvPack_Bag0042->SetClrModulation(0xffa0a0a0);
	CreateObject(EnvPack_Bag, 840, 888);
	CreateObject(EnvPack_Bag, 844, 888);

	CreateObject(EnvPack_BridgeRustic, 1096, 673);

	CreateObject(EnvPack_Candle, 1054, 672);

	var EnvPack_Candle_Shine0049 = CreateObject(EnvPack_Candle_Shine, 1054, 672);
	EnvPack_Candle_Shine0049->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0049->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0050 = CreateObject(EnvPack_Candle_Shine, 1054, 672);
	EnvPack_Candle_Shine0050->SetClrModulation(0xbfffffff);
	EnvPack_Candle_Shine0050->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0051 = CreateObject(EnvPack_Candle_Shine, 1054, 672);
	EnvPack_Candle_Shine0051->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0051->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0052 = CreateObject(EnvPack_Candle_Shine, 1054, 671);
	EnvPack_Candle_Shine0052->SetClrModulation(0xc9ffffff);
	EnvPack_Candle_Shine0052->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Candle, 1054, 575);

	var EnvPack_Candle_Shine0054 = CreateObject(EnvPack_Candle_Shine, 1054, 575);
	EnvPack_Candle_Shine0054->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0054->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0055 = CreateObject(EnvPack_Candle_Shine, 1054, 575);
	EnvPack_Candle_Shine0055->SetClrModulation(0xd9ffffff);
	EnvPack_Candle_Shine0055->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0056 = CreateObject(EnvPack_Candle_Shine, 1054, 575);
	EnvPack_Candle_Shine0056->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0056->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0057 = CreateObject(EnvPack_Candle_Shine, 1054, 575);
	EnvPack_Candle_Shine0057->SetClrModulation(0xd8ffffff);
	EnvPack_Candle_Shine0057->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Candle, 1185, 616);

	var EnvPack_Candle_Shine0059 = CreateObject(EnvPack_Candle_Shine, 1185, 616);
	EnvPack_Candle_Shine0059->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0059->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0060 = CreateObject(EnvPack_Candle_Shine, 1185, 616);
	EnvPack_Candle_Shine0060->SetClrModulation(0xc6ffffff);
	EnvPack_Candle_Shine0060->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0061 = CreateObject(EnvPack_Candle_Shine, 1185, 616);
	EnvPack_Candle_Shine0061->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0061->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0062 = CreateObject(EnvPack_Candle_Shine, 1185, 616);
	EnvPack_Candle_Shine0062->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0062->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Candle, 1531, 448);

	var EnvPack_Candle_Shine0064 = CreateObject(EnvPack_Candle_Shine, 1531, 448);
	EnvPack_Candle_Shine0064->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0064->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0065 = CreateObject(EnvPack_Candle_Shine, 1531, 448);
	EnvPack_Candle_Shine0065->SetClrModulation(0xd4ffffff);
	EnvPack_Candle_Shine0065->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0066 = CreateObject(EnvPack_Candle_Shine, 1531, 448);
	EnvPack_Candle_Shine0066->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0066->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0067 = CreateObject(EnvPack_Candle_Shine, 1531, 447);
	EnvPack_Candle_Shine0067->SetClrModulation(0xddffffff);
	EnvPack_Candle_Shine0067->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Candle, 1362, 432);

	var EnvPack_Candle_Shine0069 = CreateObject(EnvPack_Candle_Shine, 1362, 432);
	EnvPack_Candle_Shine0069->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0069->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0070 = CreateObject(EnvPack_Candle_Shine, 1362, 432);
	EnvPack_Candle_Shine0070->SetClrModulation(0xc7ffffff);
	EnvPack_Candle_Shine0070->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0071 = CreateObject(EnvPack_Candle_Shine, 1362, 432);
	EnvPack_Candle_Shine0071->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0071->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0072 = CreateObject(EnvPack_Candle_Shine, 1361, 430);
	EnvPack_Candle_Shine0072->SetClrModulation(0xcbffffff);
	EnvPack_Candle_Shine0072->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_CandleSmall, 1556, 432);

	var EnvPack_CandleSmall_Shine0074 = CreateObject(EnvPack_CandleSmall_Shine, 1556, 435);
	EnvPack_CandleSmall_Shine0074->SetCon(40);
	EnvPack_CandleSmall_Shine0074->SetClrModulation(0x46ffffff);
	EnvPack_CandleSmall_Shine0074->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_CandleSmall_Shine0075 = CreateObject(EnvPack_CandleSmall_Shine, 1556, 423);
	EnvPack_CandleSmall_Shine0075->SetCon(40);
	EnvPack_CandleSmall_Shine0075->SetClrModulation(0x43ffffff);
	EnvPack_CandleSmall_Shine0075->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_CandleSmall_Shine0076 = CreateObject(EnvPack_CandleSmall_Shine, 1556, 429);
	EnvPack_CandleSmall_Shine0076->SetCon(40);
	EnvPack_CandleSmall_Shine0076->SetClrModulation(0x46ffffff);
	EnvPack_CandleSmall_Shine0076->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_CandleSmall_Shine0077 = CreateObject(EnvPack_CandleSmall_Shine, 1555, 432);
	EnvPack_CandleSmall_Shine0077->SetCon(40);
	EnvPack_CandleSmall_Shine0077->SetClrModulation(0x41ffffff);
	EnvPack_CandleSmall_Shine0077->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Crate, 1017, 576);

	CreateObject(EnvPack_FenceRustic, 1111, 728);
	CreateObject(EnvPack_FenceRustic, 1089, 735);

	CreateObject(EnvPack_Guidepost, 315, 1167);

	CreateObject(EnvPack_Lantern, 894, 488);

	var EnvPack_Lantern_Shine0083 = CreateObject(EnvPack_Lantern_Shine, 894, 477);
	EnvPack_Lantern_Shine0083->SetClrModulation(0xe4ffffff);
	EnvPack_Lantern_Shine0083->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Lantern_Shine0084 = CreateObject(EnvPack_Lantern_Shine, 894, 477);
	EnvPack_Lantern_Shine0084->SetClrModulation(0xd3ffffff);
	EnvPack_Lantern_Shine0084->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Lantern_Shine0085 = CreateObject(EnvPack_Lantern_Shine, 894, 477);
	EnvPack_Lantern_Shine0085->SetClrModulation(0xe4ffffff);
	EnvPack_Lantern_Shine0085->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Lantern_Shine0086 = CreateObject(EnvPack_Lantern_Shine, 894, 477);
	EnvPack_Lantern_Shine0086->SetClrModulation(0xc6ffffff);
	EnvPack_Lantern_Shine0086->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Lantern, 1291, 472);

	var EnvPack_Lantern_Shine0088 = CreateObject(EnvPack_Lantern_Shine, 1291, 461);
	EnvPack_Lantern_Shine0088->SetClrModulation(0xe4ffffff);
	EnvPack_Lantern_Shine0088->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Lantern_Shine0089 = CreateObject(EnvPack_Lantern_Shine, 1291, 461);
	EnvPack_Lantern_Shine0089->SetClrModulation(0xd5ffffff);
	EnvPack_Lantern_Shine0089->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Lantern_Shine0090 = CreateObject(EnvPack_Lantern_Shine, 1291, 461);
	EnvPack_Lantern_Shine0090->SetClrModulation(0xe4ffffff);
	EnvPack_Lantern_Shine0090->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Lantern_Shine0091 = CreateObject(EnvPack_Lantern_Shine, 1291, 461);
	EnvPack_Lantern_Shine0091->SetClrModulation(0xccffffff);
	EnvPack_Lantern_Shine0091->SetObjectBlitMode(GFX_BLIT_Additive);

	CreateObject(EnvPack_Painting, 1235, 537);

	CreateObject(EnvPack_Rail, 1121, 672);

	CreateObject(EnvPack_Scarecrow, 204, 1185);

	CreateObject(EnvPack_TreeTrunks, 788, 888);

	CreateObject(EnvPack_WineBarrel, 1438, 552);
	CreateObject(EnvPack_WineBarrel, 1455, 553);

	CreateObject(EnvPack_Candle, 1471, 552);

	var EnvPack_Candle_Shine0099 = CreateObject(EnvPack_Candle_Shine, 1471, 552);
	EnvPack_Candle_Shine0099->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0099->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0100 = CreateObject(EnvPack_Candle_Shine, 1471, 552);
	EnvPack_Candle_Shine0100->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0100->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0101 = CreateObject(EnvPack_Candle_Shine, 1471, 552);
	EnvPack_Candle_Shine0101->SetClrModulation(0xe4ffffff);
	EnvPack_Candle_Shine0101->SetObjectBlitMode(GFX_BLIT_Additive);
	var EnvPack_Candle_Shine0102 = CreateObject(EnvPack_Candle_Shine, 1472, 552);
	EnvPack_Candle_Shine0102->SetClrModulation(0xddffffff);
	EnvPack_Candle_Shine0102->SetObjectBlitMode(GFX_BLIT_Additive);

	g_ruin1 = CreateObject(Ruin1, 97, 1179);
	g_ruin1->SetR(16);
	g_ruin1.StaticSaveVar = "g_ruin1";
	g_ruin1->SetPosition(97, 1150);

	g_ruin2 = CreateObject(Ruin2, 353, 1145);
	g_ruin2.StaticSaveVar = "g_ruin2";

	g_ruin3 = CreateObject(Ruin3, 267, 1180);
	g_ruin3.StaticSaveVar = "g_ruin3";

	CreateObject(Foundry, 238, 1287);

	var Chest0109 = CreateObject(Chest, 1473, 1414);
	var Chest0110 = CreateObject(Chest, 1574, 583);
	var Chest0111 = CreateObject(Chest, 823, 887);
	var Chest0112 = CreateObject(Chest, 856, 887);
	var Chest0113 = CreateObject(Chest, 1032, 575);
	var Chest0114 = CreateObject(Chest, 136, 103);

	var StoneDoor0115 = CreateObject(StoneDoor, 940, 671);
	StoneDoor0115->SetComDir(COMD_Down);
	var StoneDoor0116 = CreateObject(StoneDoor, 1348, 527);
	StoneDoor0116->SetComDir(COMD_Down);
	var StoneDoor0117 = CreateObject(StoneDoor, 1347, 431);
	StoneDoor0117->SetComDir(COMD_Down);

	var SpinWheel0118 = CreateObject(SpinWheel, 961, 672);
	SpinWheel0118->SetStoneDoor(StoneDoor0115);
	var SpinWheel0119 = CreateObject(SpinWheel, 1367, 527);
	SpinWheel0119->SetStoneDoor(StoneDoor0116);
	var SpinWheel0120 = CreateObject(SpinWheel, 1384, 471);
	SpinWheel0120->SetStoneDoor(StoneDoor0117);

	CreateObject(Column, 1197, 551);
	CreateObject(Column, 1218, 463);

	CreateObject(Idol, 1080, 575);

	var SteamEngine0124 = CreateObject(SteamEngine, 1529, 585);
	SteamEngine0124->SetCategory(C4D_StaticBack);

	g_elev1 = CreateObject(Elevator, 167, 1184);
	g_elev1.StaticSaveVar = "g_elev1";
	g_elev1->SetClrModulation(0xffa08060);
	g_elev1->CreateShaft(4);
	g_elev1->SetCasePosition(1176);
	g_elev2 = CreateObject(Elevator, 1366, 615);
	g_elev2.StaticSaveVar = "g_elev2";
	g_elev2->CreateShaft(4);
	g_elev2->SetCasePosition(607);

	CreateObject(Airship, 931, 495);

	var Catapult0211 = CreateObject(Catapult, 697, 887);
	Catapult0211->SetRDir(2);

	var Lorry0212 = CreateObject(Lorry, 149, 1324);
	Lorry0212->SetR(24);
	Lorry0212->SetPosition(149, 1315);
	var Lorry0214 = CreateObject(Lorry, 1428, 1253);
	Lorry0214->SetR(-30);
	Lorry0214->SetPosition(1428, 1243);

	CreateObject(Airship_Burnt, 38, 1152);

	var Cannon0217 = CreateObject(Cannon, 788, 679);
	Cannon0217->SetR(30);
	Cannon0217->SetPosition(788, 669);
	CreateObject(Cannon, 1004, 471);
	CreateObject(Cannon, 1336, 336);

	var Clonk0220 = CreateObject(Clonk, 673, 886);
	Clonk0220->SetColor(0xff);
	Clonk0220->SetName("Horst");
	S2AI->AddAI(Clonk0220);
	S2AI->SetHome(Clonk0220, 670, 878, DIR_Left);
	S2AI->SetGuardRange(Clonk0220, 400, 800, 500, 150);
	S2AI->SetAllyAlertRange(Clonk0220, 60);
	S2AI->SetEncounterCB(Clonk0220, "EncounterOutpost");
	Clonk0220->SetDir(DIR_Left);
	var Clonk0227 = CreateObject(Clonk, 710, 887);
	Clonk0227->SetColor(0xff);
	Clonk0227->SetName("Hanniball");
	S2AI->AddAI(Clonk0227);
	S2AI->SetHome(Clonk0227, 709, 877, DIR_Left);
	S2AI->SetGuardRange(Clonk0227, 300, 700, 500, 250);
	S2AI->SetAllyAlertRange(Clonk0227, 60);
	Clonk0227->SetDir(DIR_Left);
	var Clonk0234 = CreateObject(Clonk, 781, 671);
	Clonk0234->SetDir(DIR_Right);
	Clonk0234->SetColor(0xff);
	Clonk0234->SetName("Twonky");
	S2AI->AddAI(Clonk0234);
	S2AI->SetHome(Clonk0234, 781, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk0234, 481, 511, 600, 300);
	var Clonk0241 = CreateObject(Clonk, 1010, 671);
	Clonk0241->SetDir(DIR_Right);
	Clonk0241->SetColor(0xff);
	Clonk0241->SetName("Sven");
	S2AI->AddAI(Clonk0241);
	S2AI->SetHome(Clonk0241, 1010, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk0241, 710, 511, 600, 300);
	var Clonk0248 = CreateObject(Clonk, 985, 671);
	Clonk0248->SetDir(DIR_Right);
	Clonk0248->SetColor(0xff);
	Clonk0248->SetName("Luki");
	S2AI->AddAI(Clonk0248);
	S2AI->SetHome(Clonk0248, 985, 663, DIR_Right);
	S2AI->SetGuardRange(Clonk0248, 685, 511, 600, 300);
	var Clonk0255 = CreateObject(Clonk, 1373, 1246);
	Clonk0255->SetColor(0xffff0000);
	Clonk0255->SetName("Anna");
	S2AI->AddAI(Clonk0255);
	S2AI->SetHome(Clonk0255, 1370, 1237, DIR_Left);
	S2AI->SetGuardRange(Clonk0255, 1150, 1140, 320, 150);
	S2AI->SetAllyAlertRange(Clonk0255, 170);
	Clonk0255->SetDir(DIR_Left);
	var Clonk0262 = CreateObject(Clonk, 1449, 1246);
	Clonk0262->SetColor(0xffff0000);
	Clonk0262->SetName("Cindy");
	S2AI->AddAI(Clonk0262);
	S2AI->SetHome(Clonk0262, 1448, 1237, DIR_Left);
	S2AI->SetGuardRange(Clonk0262, 1150, 1140, 320, 150);
	S2AI->SetAllyAlertRange(Clonk0262, 170);
	S2AI->SetEncounterCB(Clonk0262, "EncounterCave");
	Clonk0262->SetDir(DIR_Left);
	g_farmer = CreateObject(Clonk, 307, 1166);
	g_farmer->SetDir(DIR_Left);
	g_farmer->SetColor(0xff0000);
	g_farmer->SetClrModulation(0xffffa020);
	g_farmer->SetName("Farmer");
	g_farmer.StaticSaveVar = "g_farmer";
	var Clonk0275 = CreateObject(Clonk, 1197, 551);
	Clonk0275->SetDir(DIR_Right);
	Clonk0275->SetColor(0xff);
	Clonk0275->SetName("Sabrina");
	S2AI->AddAI(Clonk0275);
	S2AI->SetHome(Clonk0275, 1196, 542, DIR_Right);
	S2AI->SetGuardRange(Clonk0275, 896, 392, 600, 300);
	var Clonk0282 = CreateObject(Clonk, 1266, 550);
	Clonk0282->SetColor(0xff);
	Clonk0282->SetName("Laura");
	S2AI->AddAI(Clonk0282);
	S2AI->SetGuardRange(Clonk0282, 966, 391, 600, 300);
	Clonk0282->SetDir(DIR_Left);
	var Clonk0289 = CreateObject(Clonk, 1287, 471);
	Clonk0289->SetDir(DIR_Right);
	Clonk0289->SetColor(0xff);
	S2AI->AddAI(Clonk0289);
	S2AI->SetHome(Clonk0289, 1287, 464, DIR_Right);
	S2AI->SetGuardRange(Clonk0289, 987, 312, 600, 300);
	var Clonk0296 = CreateObject(Clonk, 1092, 575);
	Clonk0296->SetDir(DIR_Right);
	Clonk0296->SetColor(0xff);
	Clonk0296->SetName("Wolfgang");
	S2AI->AddAI(Clonk0296);
	S2AI->SetHome(Clonk0296, 1092, 567, DIR_Right);
	S2AI->SetGuardRange(Clonk0296, 792, 416, 600, 300);
	g_king = CreateObject(Clonk, 1569, 431);
	g_king->SetDir(DIR_Left);
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
	var Clonk0310 = CreateObject(Clonk, 1070, 575);
	Clonk0310->SetDir(DIR_Right);
	Clonk0310->SetColor(0xff);
	Clonk0310->SetName("Hans");
	S2AI->AddAI(Clonk0310);
	S2AI->SetHome(Clonk0310, 1069, 566, DIR_Left);
	S2AI->SetGuardRange(Clonk0310, 769, 416, 600, 300);
	var Clonk0317 = CreateObject(Clonk, 1019, 471);
	Clonk0317->SetColor(0xff);
	Clonk0317->SetName("Joki");
	S2AI->AddAI(Clonk0317);
	S2AI->SetGuardRange(Clonk0317, 719, 312, 600, 300);
	Clonk0317->SetDir(DIR_Left);
	var Clonk0324 = CreateObject(Clonk, 285, 1182);
	Clonk0324->Kill(Clonk0324, true);
	Clonk0324->SetDir(DIR_Right);
	Clonk0324->SetColor(0xffff0000);
	var Clonk0330 = CreateObject(Clonk, 208, 1183);
	Clonk0330->Kill(Clonk0330, true);
	Clonk0330->SetDir(DIR_Right);
	Clonk0330->SetColor(0xffff0000);

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
	Clonk0255->CreateContents(Rock);
	Clonk0255->CreateContents(Rock);
	Clonk0255->CreateContents(Rock);

	CreateObject(Coal, 59, 1346);
	CreateObject(Coal, 156, 1370);
	CreateObject(Coal, 243, 1555);
	CreateObject(Coal, 61, 1495);
	CreateObject(Coal, 140, 1380);
	SteamEngine0124->CreateContents(Coal);
	SteamEngine0124->CreateContents(Coal);
	SteamEngine0124->CreateContents(Coal);

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
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Lorry0212->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	CreateObject(Wood, 346, 1456);
	CreateObject(Wood, 336, 1456);
	Lorry0214->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Chest0109->CreateContents(Wood);
	Lorry0214->CreateContents(Wood);
	Lorry0214->CreateContents(Wood);
	Lorry0214->CreateContents(Wood);
	Lorry0214->CreateContents(Wood);
	Chest0113->CreateContents(Wood);
	Chest0113->CreateContents(Wood);
	Chest0113->CreateContents(Wood);
	Chest0113->CreateContents(Wood);
	Chest0113->CreateContents(Wood);
	CreateObject(Wood, 167, 1512);
	CreateObject(Wood, 177, 1512);
	CreateObject(Wood, 511, 1497);

	Lorry0212->CreateContents(Loam);
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
	CreateObject(Loam, 1393, 644);
	CreateObject(Loam, 1462, 1079);
	CreateObject(Loam, 1501, 1415);
	Chest0112->CreateContents(Loam);
	Chest0112->CreateContents(Loam);
	Chest0112->CreateContents(Loam);
	Chest0114->CreateContents(Loam);
	Chest0114->CreateContents(Loam);
	Chest0114->CreateContents(Loam);
	Chest0114->CreateContents(Loam);
	Chest0114->CreateContents(Loam);

	var GoldBar0474 = CreateObject(GoldBar, 1293, 1236);
	GoldBar0474->SetR(22);
	GoldBar0474->SetPosition(1293, 1235);
	Lorry0214->CreateContents(GoldBar);
	Lorry0214->CreateContents(GoldBar);

	Clonk0220->CreateContents(Sword);
	Clonk0262->CreateContents(Sword);
	Clonk0241->CreateContents(Sword);
	Clonk0248->CreateContents(Sword);
	Clonk0310->CreateContents(Sword);
	Clonk0289->CreateContents(Sword);
	g_king->CreateContents(Sword);
	Clonk0227->CreateContents(Sword);

	var Arrow0485 = Clonk0220->CreateContents(Arrow);
	Arrow0485->SetR(90);
	var Arrow0486 = Clonk0220->CreateContents(Arrow);
	Arrow0486->SetR(90);
	var Arrow0487 = Chest0111->CreateContents(Arrow);
	Arrow0487->SetR(90);
	var Arrow0488 = Clonk0234->CreateContents(Arrow);
	Arrow0488->SetR(90);
	var Arrow0489 = Clonk0234->CreateContents(Arrow);
	Arrow0489->SetR(90);
	var Arrow0490 = Clonk0317->CreateContents(Arrow);
	Arrow0490->SetR(90);
	var Arrow0491 = Clonk0317->CreateContents(Arrow);
	Arrow0491->SetR(90);
	var Arrow0492 = g_king->CreateContents(Arrow);
	Arrow0492->SetR(90);
	var Arrow0493 = g_king->CreateContents(Arrow);
	Arrow0493->SetR(90);

	Clonk0220->CreateContents(Bow);
	Chest0111->CreateContents(Bow);
	Clonk0234->CreateContents(Bow);
	Clonk0317->CreateContents(Bow);
	g_king->CreateContents(Bow);

	var Boompack0499 = CreateObject(Boompack, 135, 1324);
	Boompack0499->SetColor(0xff);

	Lorry0212->CreateContents(DynamiteBox);
	Lorry0214->CreateContents(DynamiteBox);
	Chest0109->CreateContents(DynamiteBox);
	Chest0109->CreateContents(DynamiteBox);
	Chest0114->CreateContents(DynamiteBox);

	CreateObject(Dynamite, 1334, 1224);

	Lorry0214->CreateContents(Pickaxe);
	Clonk0255->CreateContents(Pickaxe);
	Clonk0262->CreateContents(Pickaxe);

	Lorry0212->CreateContents(Shovel);

	var Barrel0510 = CreateObject(Barrel, 167, 1333);
	Barrel0510->SetR(-13);
	Barrel0510->SetPosition(167, 1327);

	var Seaweed0512 = CreateObject(Seaweed, 169, 1543);
	Seaweed0512->SetPhase(49);
	var Seaweed0515 = CreateObject(Seaweed, 815, 1342);
	Seaweed0515->SetPhase(49);
	var Seaweed0518 = CreateObject(Seaweed, 719, 1078);
	Seaweed0518->SetPhase(68);
	var Seaweed0521 = CreateObject(Seaweed, 772, 1087);
	Seaweed0521->SetPhase(8);
	var Seaweed0524 = CreateObject(Seaweed, 1258, 1279);
	Seaweed0524->SetPhase(75);
	var Seaweed0527 = CreateObject(Seaweed, 847, 1366);
	Seaweed0527->SetCon(1);

	CreateObject(Mushroom, 126, 1320);
	CreateObject(Mushroom, 212, 1287);
	CreateObject(Mushroom, 367, 1392);
	CreateObject(Mushroom, 268, 1431);

	Chest0110->CreateContents(Musket);

	Chest0110->CreateContents(LeadShot);
	Chest0110->CreateContents(LeadShot);
	Chest0110->CreateContents(LeadShot);

	Clonk0282->CreateContents(Javelin);
	Clonk0275->CreateContents(Javelin);
	Clonk0282->CreateContents(Javelin);
	Clonk0275->CreateContents(Javelin);
	Clonk0282->CreateContents(Javelin);
	Clonk0275->CreateContents(Javelin);

	Clonk0310->CreateContents(Shield);
	Clonk0289->CreateContents(Shield);
	g_king->CreateContents(Shield);

	Chest0113->CreateContents(Bread);
	Chest0113->CreateContents(Bread);
	Chest0113->CreateContents(Bread);
	Chest0114->CreateContents(Bread);
	Chest0114->CreateContents(Bread);
	Chest0114->CreateContents(Bread);

	CreateObject(EnvPack_ManaAltar, 1052, 471);

	Catapult0211->CreateContents(Firestone);
	Catapult0211->CreateContents(Firestone);
	Catapult0211->CreateContents(Firestone);
	Catapult0211->CreateContents(Firestone);
	Catapult0211->CreateContents(Firestone);
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
	Clonk0255->CreateContents(Firestone);
	Clonk0255->CreateContents(Firestone);
	Clonk0262->CreateContents(Firestone);
	Chest0112->CreateContents(Firestone);
	Chest0112->CreateContents(Firestone);
	Chest0112->CreateContents(Firestone);
	Clonk0296->CreateContents(Firestone);
	Clonk0296->CreateContents(Firestone);
	Clonk0296->CreateContents(Firestone);
	Clonk0296->CreateContents(Firestone);
	Chest0113->CreateContents(Firestone);
	Chest0113->CreateContents(Firestone);
	Chest0113->CreateContents(Firestone);
	g_king->CreateContents(Firestone);
	g_king->CreateContents(Firestone);
	g_king->CreateContents(Firestone);

	return true;
}
