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
	var Tree_Coniferous_Burned002 = CreateObject(Tree_Coniferous_Burned, 43, 1246);
	Tree_Coniferous_Burned002->SetCon(75);
	Tree_Coniferous_Burned002->SetR(100);

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

	CreateObjectAbove(Tree_Coniferous, 408, 1167);
	var Tree_Coniferous002 = CreateObject(Tree_Coniferous, 408, 1191);
	Tree_Coniferous002->SetCon(47);
	var Tree_Coniferous003 = CreateObjectAbove(Tree_Coniferous, 217, 1191);
	Tree_Coniferous003->SetCon(39);
	var Tree_Coniferous004 = CreateObject(Tree_Coniferous, 392, 1148);
	Tree_Coniferous004->SetCon(27);
	var Tree_Coniferous005 = CreateObject(Tree_Coniferous, 410, 1168);
	Tree_Coniferous005->SetCon(3);

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
	var StoneDoor003 = CreateObject(StoneDoor, 1347, 412);
	StoneDoor003->SetComDir(COMD_Down);

	var SpinWheel001 = CreateObjectAbove(SpinWheel, 961, 672);
	SpinWheel001->SetSwitchTarget(StoneDoor001);
	var SpinWheel002 = CreateObjectAbove(SpinWheel, 1367, 527);
	SpinWheel002->SetSwitchTarget(StoneDoor002);
	var SpinWheel003 = CreateObjectAbove(SpinWheel, 1384, 471);
	SpinWheel003->SetSwitchTarget(StoneDoor003);

	CreateObject(Column, 1197, 551);
	CreateObject(Column, 1218, 463);

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

	var GoldBar001 = CreateObject(GoldBar, 1293, 1235);
	GoldBar001->SetR(22);

	CreateObjectAbove(Airship, 931, 495);

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
	
	// lava
	var light01 = CreateObject(Rock, 520, 1210);
	light01->SetCategory(C4D_StaticBack);
	light01->SetLightRange(100, 100);
	light01->SetLightColor(RGB(255,144,16));
	light01.Visibility = VIS_None;

	// candles
	for (var candle_shine in FindObjects(Find_ID(EnvPack_Candle_Shine)))
	{
		candle_shine->SetLightRange(30, 20);
		candle_shine->SetLightColor(RGB(255,163,58));
	}
	
	// large cave mushroom
	LargeCaveMushroom003->SetLightRange(80,50);
	LargeCaveMushroom003->SetLightColor(RGB(0,100,0));
	Object(17)->SetLightRange(80,50);
	Object(17)->SetLightColor(RGB(0,100,0));
	
	// small cave mushrooms
	for (var mushroom in FindObjects(Find_ID(Mushroom), Find_InRect(160, 1240, 200, 60)))
	{
		mushroom->SetLightColor(RGB(100,0,0));
		mushroom->SetLightRange(50,30);
	}

	Object(463)->SetLightRange(50,10);
	Object(463)->SetLightColor(RGB(100,0,200));
	Object(13)->SetLightRange(50,10);
    Object(13)->SetLightColor(RGB(100,200,0));

	return true;
}
