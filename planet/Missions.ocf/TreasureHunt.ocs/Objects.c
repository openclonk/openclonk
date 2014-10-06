/* Automatically created objects file */

static npc_dagobert, npc_tarzan, g_golden_shovel, g_flagpole;

func InitializeObjects()
{
	var Rule_BaseRespawn001 = CreateObject(Rule_BaseRespawn, 0, 0);
	Rule_BaseRespawn001->SetInventoryTransfer(true);
	Rule_BaseRespawn001->SetFreeCrew(true);

	CreateObject(Grass, 1627, 396);
	CreateObject(Grass, 1636, 385);
	CreateObject(Grass, 1786, 469);
	CreateObject(Grass, 1574, 493);
	CreateObject(Grass, 1564, 493);
	CreateObject(Grass, 1537, 525);
	CreateObject(Grass, 1585, 486);
	CreateObject(Grass, 1739, 430);

	var Column0010 = CreateObject(Column, 779, 591);
	Column0010->SetClrModulation(0xffffd0d0);
	Column0010->SetMeshMaterial("AncientColumn", 0);
	Column0010.Plane = 50;

	var Chest0009 = CreateObject(Chest, 1002, 313);
	Chest0009.Plane = 50;

	CreateObject(Rule_TeamAccount, 0, 0);

	CreateObject(Rule_NoPowerNeed, 0, 0);

	var LargeCaveMushroom0013 = CreateObject(LargeCaveMushroom, 1308, 1038);
	LargeCaveMushroom0013->SetClrModulation(0xffe4effc);
	var LargeCaveMushroom0017 = CreateObject(LargeCaveMushroom, 1345, 1028);
	LargeCaveMushroom0017->SetClrModulation(0xffe1e3ee);
	LargeCaveMushroom0017->SetMeshMaterial("FlyAmanitaMushroom", 0);
	var LargeCaveMushroom0021 = CreateObject(LargeCaveMushroom, 1399, 1025);
	LargeCaveMushroom0021->SetClrModulation(0xfff3e3e7);
	LargeCaveMushroom0021->SetMeshMaterial("FlyAmanitaMushroom", 0);
	var LargeCaveMushroom0025 = CreateObject(LargeCaveMushroom, 1464, 999);
	LargeCaveMushroom0025->SetClrModulation(0xffe0e6dd);
	var LargeCaveMushroom0029 = CreateObject(LargeCaveMushroom, 1450, 1012);
	LargeCaveMushroom0029->SetClrModulation(0xffe4eae2);
	var LargeCaveMushroom0033 = CreateObject(LargeCaveMushroom, 1523, 993);
	LargeCaveMushroom0033->SetClrModulation(0xffe2deee);
	LargeCaveMushroom0033->SetMeshMaterial("FlyAmanitaMushroom", 0);

	CreateObject(Trunk, 1000, 313);
	CreateObject(Trunk, 1006, 313);

	CreateObject(EnvPack_Painting, 606, 434);

	CreateObject(EnvPack_Guidepost, 81, 743);

	CreateObject(EnvPack_BridgeRustic, 591, 356);

	CreateObject(EnvPack_Bag, 506, 968);

	CreateObject(EnvPack_Lantern, 356, 458);

	CreateObject(EnvPack_TreeTrunks, 601, 408);

	CreateObject(EnvPack_Rail, 564, 354);
	CreateObject(EnvPack_Rail, 616, 356);
	CreateObject(EnvPack_Rail, 633, 354);
	CreateObject(EnvPack_Rail, 554, 353);

	CreateObject(EnvPack_WineBarrel, 627, 455);

	CreateObject(EnvPack_Crate, 494, 385);

	CreateObject(Fern, 1793, 474);
	CreateObject(Fern, 1645, 384);
	CreateObject(Fern, 1525, 535);

	var Rank0066 = CreateObject(Rank, 1320, 326);
	Rank0066->SetR(148);
	Rank0066->SetPosition(1320, 326);
	var Rank0067 = CreateObject(Rank, 1327, 298);
	Rank0067->SetR(165);
	Rank0067->SetPosition(1327, 298);
	var Rank0068 = CreateObject(Rank, 1424, 257);
	Rank0068->SetR(108);
	Rank0068->SetPosition(1424, 257);
	var Rank0069 = CreateObject(Rank, 1430, 248);
	Rank0069->SetR(39);
	Rank0069->SetPosition(1430, 246);
	var Rank0070 = CreateObject(Rank, 1413, 262);
	Rank0070->SetR(128);
	Rank0070->SetPosition(1413, 262);
	var Rank0071 = CreateObject(Rank, 1396, 263);
	Rank0071->SetR(-131);
	Rank0071->SetPosition(1396, 263);

	CreateObject(SproutBerryBush, 1823, 493);

	CreateObject(Trunk, 401, 1147);

	CreateObject(Tree_Coconut, 51, 1142);
	CreateObject(Tree_Coconut, 333, 1150);

	CreateObject(Tree_Coniferous, 1864, 464);
	CreateObject(Tree_Coniferous, 2788, 680);

	var Lichen0093 = CreateObject(Lichen, 2694, 706);
	Lichen0093->SetAction("Grown");

	CreateObject(BigRock, 1301, 500);
	CreateObject(BigRock, 1207, 282);
	CreateObject(BigRock, 1291, 263);

	var Amethyst0099 = CreateObject(Amethyst, 803, 583);
	Amethyst0099.Plane = 190;

	var Chest0101 = CreateObject(Chest, 515, 967);
	Chest0101.tool_spawn = TeleGlove;
	var Chest0102 = CreateObject(Chest, 227, 760);
	Chest0102.tool_spawn = Pickaxe;
	var Chest0103 = CreateObject(Chest, 624, 943);
	Chest0103.tool_spawn = GrappleBow;
	var Chest0104 = CreateObject(Chest, 603, 942);
	Chest0104.tool_spawn = GrappleBow;
	var Chest0105 = CreateObject(Chest, 472, 455);
	var Chest0106 = CreateObject(Chest, 546, 383);
	var Chest0107 = CreateObject(Chest, 840, 47);
	Chest0107.tool_spawn = WindBag;
	var Chest0108 = CreateObject(Chest, 853, 1574);
	var Chest0109 = CreateObject(Chest, 1428, 1542);
	var Chest0110 = CreateObject(Chest, 1765, 1191);
	var Chest0111 = CreateObject(Chest, 1878, 719);
	Chest0111.tool_spawn = Axe;
	var Chest0112 = CreateObject(Chest, 1943, 714);
	var Chest0113 = CreateObject(Chest, 2103, 1119);
	var Chest0114 = CreateObject(Chest, 397, 583);
	var Chest0115 = CreateObject(Chest, 871, 583);
	Chest0115->SetMeshMaterial("GoldenChest", 0);
	var Chest0116 = CreateObject(Chest, 12, 39);
	var Chest0117 = CreateObject(Chest, 2786, 55);
	var Chest0118 = CreateObject(Chest, 1830, 486);
	Chest0118.tool_spawn = Hammer;
	var Chest0119 = CreateObject(Chest, 730, 135);
	var Chest0120 = CreateObject(Chest, 1626, 1591);

	var StoneDoor0121 = CreateObject(StoneDoor, 940, 1151);
	StoneDoor0121->SetComDir(COMD_Down);
	StoneDoor0121->MakeInvincible();
	var StoneDoor0123 = CreateObject(StoneDoor, 1084, 1151);
	StoneDoor0123->SetComDir(COMD_Down);
	StoneDoor0123->MakeInvincible();
	var StoneDoor0125 = CreateObject(StoneDoor, 564, 455);
	StoneDoor0125->SetComDir(COMD_Down);
	StoneDoor0125->MakeInvincible();
	var StoneDoor0127 = CreateObject(StoneDoor, 843, 735);
	StoneDoor0127->SetComDir(COMD_Down);
	StoneDoor0127->MakeInvincible();
	var StoneDoor0129 = CreateObject(StoneDoor, 1058, 719);
	StoneDoor0129->SetComDir(COMD_Down);
	StoneDoor0129->MakeInvincible();
	var StoneDoor0131 = CreateObject(StoneDoor, 1092, 1047);
	StoneDoor0131->SetComDir(COMD_Down);
	StoneDoor0131->MakeInvincible();
	var StoneDoor0133 = CreateObject(StoneDoor, 1892, 951);
	StoneDoor0133->SetComDir(COMD_Down);
	StoneDoor0133->MakeInvincible();
	var StoneDoor0135 = CreateObject(StoneDoor, 813, 735);
	StoneDoor0135->SetComDir(COMD_Down);
	StoneDoor0135->MakeInvincible();
	var StoneDoor0137 = CreateObject(StoneDoor, 781, 735);
	StoneDoor0137->SetComDir(COMD_Down);
	StoneDoor0137->SetClrModulation(0xffa0a0a0);
	var StoneDoor0138 = CreateObject(StoneDoor, 692, 767);
	StoneDoor0138->SetComDir(COMD_Down);
	StoneDoor0138->MakeInvincible();
	var StoneDoor0140 = CreateObject(StoneDoor, 684, 351);
	StoneDoor0140->SetComDir(COMD_Down);
	StoneDoor0140->MakeInvincible();

	var SpinWheel0142 = CreateObject(SpinWheel, 589, 457);
	SpinWheel0142->SetMeshMaterial("SpinWheelGearRed", 0);
	SpinWheel0142->SetStoneDoor(StoneDoor0121);
	var SpinWheel0143 = CreateObject(SpinWheel, 611, 456);
	SpinWheel0143->SetMeshMaterial("SpinWheelGearBlue", 0);
	SpinWheel0143->SetStoneDoor(StoneDoor0123);
	var SpinWheel0144 = CreateObject(SpinWheel, 619, 410);
	SpinWheel0144->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel0144->SetStoneDoor(StoneDoor0125);
	var SpinWheel0145 = CreateObject(SpinWheel, 1223, 1553);
	SpinWheel0145->SetStoneDoor(StoneDoor0129);
	var SpinWheel0146 = CreateObject(SpinWheel, 1117, 1048);
	SpinWheel0146->SetStoneDoor(StoneDoor0131);
	var SpinWheel0147 = CreateObject(SpinWheel, 2761, 690);
	SpinWheel0147->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel0147->SetStoneDoor(StoneDoor0135);
	var SpinWheel0148 = CreateObject(SpinWheel, 1850, 1463);
	SpinWheel0148->SetMeshMaterial("SpinWheelGearRed", 0);
	SpinWheel0148->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel0148->SetStoneDoor(StoneDoor0127);
	var SpinWheel0149 = CreateObject(SpinWheel, 2793, 1521);
	SpinWheel0149->SetMeshMaterial("SpinWheelGearRed", 0);
	SpinWheel0149->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel0149->SetStoneDoor(StoneDoor0133);
	var SpinWheel0150 = CreateObject(SpinWheel, 830, 735);
	SpinWheel0150->SetStoneDoor(StoneDoor0138);
	var SpinWheel0151 = CreateObject(SpinWheel, 703, 352);
	SpinWheel0151->SetMeshMaterial("SpinWheelBaseAlt", 1);
	SpinWheel0151->SetStoneDoor(StoneDoor0140);

	CreateObject(Pump, 1027, 1152);

	CreateObject(Sawmill, 1259, 1047);

	var ToolsWorkshop0156 = CreateObject(ToolsWorkshop, 1169, 903);

	var Column0158 = CreateObject(Column, 779, 488);
	Column0158->SetR(180);
	Column0158->SetClrModulation(0xffffd0d0);
	Column0158->SetMeshMaterial("AncientColumn", 0);
	Column0158->SetPosition(779, 488);
	var Column0159 = CreateObject(Column, 1419, 217);
	Column0159->SetMeshMaterial("AncientColumn", 0);
	CreateObject(Column, 1386, 616);

	CreateObject(Ruin_Windmill, 1678, 375);

	CreateObject(Ruin_WoodenCabin, 1199, 1046);

	var Idol0163 = CreateObject(Idol, 1045, 721);
	Idol0163->SetMeshMaterial("IdolGrayColor", 0);

	g_flagpole = CreateObject(Flagpole, 210, 1185);
	g_flagpole->SetNeutral(true);
	g_flagpole.StaticSaveVar = "g_flagpole";

	var LotsOfCoins0164 = CreateObject(LotsOfCoins, 805, 592);
	LotsOfCoins0164.Plane = 200;

	var Idol0230 = CreateObject(Idol, 824, 583);
	Idol0230->SetR(-4);
	Idol0230.Plane = 220;
	Idol0230->SetPosition(824, 568);

	var Lorry0231 = CreateObject(Lorry, 200, 1183);
	var Lorry0233 = CreateObject(Lorry, 708, 1407);
	Lorry0233->SetMeshMaterial("RuinedLorry", 0);

	var Catapult0235 = CreateObject(Catapult, 1714, 951);
	Catapult0235->SetRDir(2);

	CreateObject(StrawMan, 1924, 439);
	CreateObject(StrawMan, 2642, 705);

	var Clonk0238 = CreateObject(Clonk, 316, 430);
	Clonk0238->SetColor(0xff);
	S2AI->AddAI(Clonk0238);
	S2AI->SetHome(Clonk0238, 315, 422, DIR_Left);
	S2AI->SetGuardRange(Clonk0238, 296, 322, 350, 140);
	S2AI->SetEncounterCB(Clonk0238, "EncounterCastle");
	Clonk0238->SetDir(DIR_Left);
	var Clonk0245 = CreateObject(Clonk, 501, 455);
	Clonk0245->SetDir(DIR_Right);
	Clonk0245->SetColor(0xff);
	S2AI->AddAI(Clonk0245);
	S2AI->SetHome(Clonk0245, 502, 445, DIR_Right);
	S2AI->SetGuardRange(Clonk0245, 460, 300, 200, 160);
	S2AI->SetMaxAggroDistance(Clonk0245, 60);
	var Clonk0252 = CreateObject(Clonk, 534, 454);
	Clonk0252->SetDir(DIR_Right);
	Clonk0252->SetColor(0xff);
	S2AI->AddAI(Clonk0252);
	S2AI->SetHome(Clonk0252, 534, 446, DIR_Right);
	S2AI->SetGuardRange(Clonk0252, 460, 300, 200, 160);
	S2AI->SetMaxAggroDistance(Clonk0252, 60);
	var Clonk0259 = CreateObject(Clonk, 671, 638);
	Clonk0259->SetDir(DIR_Right);
	Clonk0259->SetCon(150);
	Clonk0259->SetColor(0xffffa000);
	S2AI->AddAI(Clonk0259);
	S2AI->SetHome(Clonk0259, 671, 629, DIR_Right);
	S2AI->SetGuardRange(Clonk0259, 580, 480, 320, 175);
	S2AI->SetEncounterCB(Clonk0259, "EncounterFinal");
	npc_dagobert = CreateObject(Clonk, 369, 1143);
	npc_dagobert->SetColor(0xffa000);
	npc_dagobert->SetName("Scrooge");
	npc_dagobert.StaticSaveVar = "npc_dagobert";
	npc_dagobert->MakeInvincible();
	npc_dagobert->SetDir(DIR_Left);
	var Clonk0273 = CreateObject(Clonk, 1720, 375);
	Clonk0273->SetDir(DIR_Right);
	Clonk0273->SetColor(0x808080);
	Clonk0273->SetName("Otto");
	Clonk0273->SetSkin(2);
	var Clonk0280 = CreateObject(Clonk, 1868, 950);
	Clonk0280->SetColor(0xff0000);
	Clonk0280->SetName("Donald");
	Clonk0280->SetDir(DIR_Left);
	var Clonk0286 = CreateObject(Clonk, 676, 942);
	Clonk0286->SetDir(DIR_Right);
	Clonk0286->SetColor(0x802000);
	Clonk0286->SetName("Jane");
	Clonk0286->SetSkin(1);
	npc_tarzan = CreateObject(Clonk, 751, 875);
	npc_tarzan->SetXDir(3);
	npc_tarzan->SetYDir(-1);
	npc_tarzan->SetColor(0x402000);
	npc_tarzan->SetName("Tarzan");
	npc_tarzan.StaticSaveVar = "npc_tarzan";
	npc_tarzan->SetDir(DIR_Left);
	var Clonk0299 = CreateObject(Clonk, 498, 967);
	Clonk0299->SetColor(0x20ffff);
	Clonk0299->SetName("Sophie");
	Clonk0299->SetSkin(3);
	Clonk0299->SetDir(DIR_Left);
	var Clonk0306 = CreateObject(Clonk, 853, 735);
	Clonk0306->SetDir(DIR_Right);
	Clonk0306->SetColor(0x6000);
	Clonk0306->SetName("Riku");
	Clonk0306->SetSkin(2);
	var Clonk0313 = CreateObject(Clonk, 1098, 1150);
	Clonk0313->SetDir(DIR_Right);
	Clonk0313->SetColor(0x800000);
	Clonk0313->SetName("Ann");
	Clonk0313->SetSkin(3);

	npc_dagobert->SetDialogue("Dagobert",true);
	Clonk0273->SetDialogue("Otto",true);

	var GrappleBow0335 = npc_tarzan->CreateContents(GrappleBow);
	var GrappleBow0333 = npc_tarzan->CreateContents(GrappleBow);

	npc_tarzan->SetDialogue("Tarzan",false);
	Clonk0286->SetDialogue("Jane",true);
	Clonk0280->SetDialogue("Donald",true);

	var TeleGlove0325 = Clonk0299->CreateContents(TeleGlove);

	Clonk0299->SetDialogue("Sophie",true);
	Clonk0306->SetDialogue("Riku",true);

	var Pickaxe0320 = Clonk0313->CreateContents(Pickaxe);

	Clonk0313->SetDialogue("Ann",true);

	CreateObject(Skull, 53, 1138);

	var Bone0344 = CreateObject(Bone, 25, 1141);
	Bone0344->SetR(-69);
	Bone0344->SetPosition(25, 1139);
	var Bone0345 = CreateObject(Bone, 48, 1135);
	Bone0345->SetR(-51);
	Bone0345->SetPosition(48, 1135);
	var Bone0346 = CreateObject(Bone, 472, 963);
	Bone0346->SetR(-52);
	Bone0346->SetRDir(-7);
	Bone0346->SetPosition(472, 962);
	var Bone0347 = CreateObject(Bone, 488, 962);
	Bone0347->SetR(-51);
	Bone0347->SetPosition(488, 962);
	var Bone0348 = CreateObject(Bone, 479, 962);
	Bone0348->SetR(-51);
	Bone0348->SetPosition(479, 962);
	var Bone0349 = CreateObject(Bone, 464, 963);
	Bone0349->SetR(-51);
	Bone0349->SetPosition(464, 963);

	Lorry0231->CreateContents(Loam);
	Lorry0231->CreateContents(Loam);
	Lorry0231->CreateContents(Loam);
	Lorry0231->CreateContents(Loam);
	CreateObject(Loam, 153, 1235);
	CreateObject(Loam, 357, 1320);
	CreateObject(Loam, 265, 1454);
	CreateObject(Loam, 528, 1453);
	CreateObject(Loam, 554, 1456);
	CreateObject(Loam, 988, 1287);
	CreateObject(Loam, 1030, 1275);
	CreateObject(Loam, 1065, 1270);
	CreateObject(Loam, 1051, 1261);
	CreateObject(Loam, 1049, 1282);
	CreateObject(Loam, 1081, 1269);
	CreateObject(Loam, 1334, 1052);
	CreateObject(Loam, 1532, 861);
	CreateObject(Loam, 1619, 807);
	CreateObject(Loam, 1642, 442);
	CreateObject(Loam, 1694, 402);
	CreateObject(Loam, 1578, 527);
	CreateObject(Loam, 1746, 459);

	var Metal0372 = CreateObject(Metal, 1922, 978);
	Metal0372->SetR(20);
	Metal0372->SetPosition(1922, 976);

	var Nugget0373 = CreateObject(Nugget, 812, 590);
	Nugget0373->SetClrModulation(0xffffd0a0);
	CreateObject(Nugget, 869, 583);
	var Nugget0375 = CreateObject(Nugget, 853, 584);
	Nugget0375->SetClrModulation(0xffffd0a0);
	var Nugget0376 = CreateObject(Nugget, 823, 584);
	Nugget0376->SetClrModulation(0xffffd0a0);

	Chest0106->CreateContents(GoldBar);
	Chest0110->CreateContents(GoldBar);
	Chest0109->CreateContents(GoldBar);
	Chest0108->CreateContents(GoldBar);
	Lorry0233->CreateContents(GoldBar);
	Chest0107->CreateContents(GoldBar);
	Chest0112->CreateContents(GoldBar);
	Chest0113->CreateContents(GoldBar);
	Chest0114->CreateContents(GoldBar);
	var GoldBar0386 = Chest0009->CreateContents(GoldBar);
	GoldBar0386->SetR(-1);
	Chest0117->CreateContents(GoldBar);
	var GoldBar0388 = Chest0116->CreateContents(GoldBar);
	GoldBar0388->SetR(-97);
	var GoldBar0389 = Chest0119->CreateContents(GoldBar);
	GoldBar0389->SetClrModulation(0xffffd0a0);
	GoldBar0389->SetR(-29);
	var GoldBar0390 = CreateObject(GoldBar, 880, 542);
	GoldBar0390->SetR(-29);
	GoldBar0390->SetClrModulation(0xffffd0a0);
	GoldBar0390->SetPosition(880, 540);
	ToolsWorkshop0156->CreateContents(GoldBar);
	CreateObject(GoldBar, 72, 1463);
	CreateObject(GoldBar, 2746, 736);
	var GoldBar0394 = CreateObject(GoldBar, 2507, 1262);
	GoldBar0394->SetR(-6);
	GoldBar0394->SetPosition(2507, 1262);
	Chest0120->CreateContents(GoldBar);
	var GoldBar0396 = CreateObject(GoldBar, 972, 1280);
	GoldBar0396->SetR(55);
	GoldBar0396->SetPosition(972, 1277);

	CreateObject(Ruby, 864, 585);
	CreateObject(Ruby, 806, 587);
	CreateObject(Ruby, 849, 582);
	CreateObject(Ruby, 856, 588);

	var Amethyst0405 = CreateObject(Amethyst, 793, 588);
	Amethyst0405->SetR(22);
	Amethyst0405->SetPosition(793, 584);
	CreateObject(Amethyst, 885, 561);
	CreateObject(Amethyst, 828, 585);

	Lorry0231->CreateContents(Dynamite);
	Lorry0231->CreateContents(Dynamite);
	Chest0107->CreateContents(Dynamite);
	Chest0107->CreateContents(Dynamite);
	Chest0107->CreateContents(Dynamite);
	CreateObject(Dynamite, 1046, 722);
	Chest0104->CreateContents(Dynamite);
	Chest0103->CreateContents(Dynamite);
	Chest0104->CreateContents(Dynamite);
	Chest0103->CreateContents(Dynamite);
	Chest0101->CreateContents(Dynamite);
	Chest0109->CreateContents(Dynamite);
	Chest0109->CreateContents(Dynamite);
	Chest0109->CreateContents(Dynamite);
	Chest0112->CreateContents(Dynamite);
	Chest0112->CreateContents(Dynamite);
	Chest0112->CreateContents(Dynamite);

	var Bow0434 = Clonk0238->CreateContents(Bow);

	var Arrow0435 = Bow0434->CreateContents(Arrow);
	Arrow0435->SetR(90);
	var Arrow0436 = Clonk0238->CreateContents(Arrow);
	Arrow0436->SetR(90);
	var Arrow0437 = CreateObject(Arrow, 313, 431);
	Arrow0437->SetR(90);
	Arrow0437->SetPosition(313, 431);
	var Arrow0438 = CreateObject(Arrow, 313, 431);
	Arrow0438->SetR(90);
	Arrow0438->SetPosition(313, 431);
	var Arrow0439 = CreateObject(Arrow, 313, 431);
	Arrow0439->SetR(90);
	Arrow0439->SetPosition(313, 431);
	var Arrow0440 = CreateObject(Arrow, 313, 431);
	Arrow0440->SetR(90);
	Arrow0440->SetPosition(313, 431);
	var Arrow0441 = CreateObject(Arrow, 313, 431);
	Arrow0441->SetR(90);
	Arrow0441->SetPosition(313, 431);
	var Arrow0442 = CreateObject(Arrow, 313, 431);
	Arrow0442->SetR(90);
	Arrow0442->SetPosition(313, 431);
	var Arrow0443 = CreateObject(Arrow, 313, 431);
	Arrow0443->SetR(90);
	Arrow0443->SetPosition(313, 431);
	var Arrow0444 = CreateObject(Arrow, 313, 431);
	Arrow0444->SetR(90);
	Arrow0444->SetPosition(313, 431);
	var Arrow0445 = CreateObject(Arrow, 313, 431);
	Arrow0445->SetR(90);
	Arrow0445->SetPosition(313, 431);

	Chest0105->CreateContents(Bread);
	Chest0105->CreateContents(Bread);
	Lorry0233->CreateContents(Bread);
	Lorry0233->CreateContents(Bread);
	Lorry0233->CreateContents(Bread);

	Chest0105->CreateContents(DynamiteBox);
	Chest0105->CreateContents(DynamiteBox);
	Chest0116->CreateContents(DynamiteBox);
	Chest0116->CreateContents(DynamiteBox);
	Chest0117->CreateContents(DynamiteBox);
	Chest0117->CreateContents(DynamiteBox);
	Chest0120->CreateContents(DynamiteBox);
	Chest0113->CreateContents(DynamiteBox);
	Chest0119->CreateContents(DynamiteBox);

	Clonk0252->CreateContents(Sword);
	Clonk0245->CreateContents(Sword);
	Chest0107->CreateContents(Sword);
	Clonk0259->CreateContents(Sword);
	Clonk0259->CreateContents(Sword);

	var Seaweed0465 = CreateObject(Seaweed, 2494, 1263);
	Seaweed0465->SetPhase(41);
	var Seaweed0468 = CreateObject(Seaweed, 2508, 1263);
	Seaweed0468->SetPhase(33);
	var Seaweed0471 = CreateObject(Seaweed, 2520, 1263);
	Seaweed0471->SetPhase(10);
	var Seaweed0474 = CreateObject(Seaweed, 2508, 1263);
	Seaweed0474->SetPhase(10);
	var Seaweed0477 = CreateObject(Seaweed, 2503, 1263);
	Seaweed0477->SetPhase(36);
	var Seaweed0480 = CreateObject(Seaweed, 2526, 1262);
	Seaweed0480->SetPhase(36);
	var Seaweed0483 = CreateObject(Seaweed, 2516, 1263);
	Seaweed0483->SetPhase(36);
	var Seaweed0486 = CreateObject(Seaweed, 2499, 1263);
	Seaweed0486->SetPhase(36);
	var Seaweed0489 = CreateObject(Seaweed, 2663, 1278);
	Seaweed0489->SetPhase(39);
	var Seaweed0492 = CreateObject(Seaweed, 2769, 1272);
	Seaweed0492->SetPhase(39);
	var Seaweed0495 = CreateObject(Seaweed, 2751, 1279);
	Seaweed0495->SetPhase(39);
	var Seaweed0498 = CreateObject(Seaweed, 2762, 1271);
	Seaweed0498->SetPhase(39);
	var Seaweed0501 = CreateObject(Seaweed, 2775, 1279);
	Seaweed0501->SetPhase(39);
	var Seaweed0504 = CreateObject(Seaweed, 2762, 1271);
	Seaweed0504->SetPhase(39);
	var Seaweed0507 = CreateObject(Seaweed, 2659, 1279);
	Seaweed0507->SetPhase(39);
	var Seaweed0510 = CreateObject(Seaweed, 2416, 1245);
	Seaweed0510->SetPhase(77);
	var Seaweed0513 = CreateObject(Seaweed, 2395, 1239);
	Seaweed0513->SetPhase(77);
	var Seaweed0516 = CreateObject(Seaweed, 2396, 1239);
	Seaweed0516->SetPhase(77);
	var Seaweed0519 = CreateObject(Seaweed, 2332, 1145);
	Seaweed0519->SetPhase(77);
	var Seaweed0522 = CreateObject(Seaweed, 2407, 1246);
	Seaweed0522->SetPhase(77);

	CreateObject(Mushroom, 1580, 760);
	CreateObject(Mushroom, 1613, 776);
	CreateObject(Mushroom, 1525, 848);
	CreateObject(Mushroom, 1612, 863);
	CreateObject(Mushroom, 1321, 895);
	CreateObject(Mushroom, 1315, 896);
	CreateObject(Mushroom, 1343, 904);
	CreateObject(Mushroom, 1347, 903);

	CreateObject(Balloon, 491, 383);

	var Barrel0558 = CreateObject(Barrel, 623, 456);
	Barrel0558->SetColor(0xff000000);

	CreateObject(Sproutberry, 1823, 488);
	CreateObject(Sproutberry, 1823, 488);
	CreateObject(Sproutberry, 1823, 488);

	var Boompack0563 = CreateObject(Boompack, 543, 383);
	Boompack0563->SetColor(0xff);
	var Boompack0564 = CreateObject(Boompack, 548, 384);
	Boompack0564->SetColor(0xff);
	var Boompack0565 = CreateObject(Boompack, 1948, 713);
	Boompack0565->SetColor(0xff);
	var Boompack0566 = CreateObject(Boompack, 1944, 487);
	Boompack0566->SetR(135);
	Boompack0566->SetColor(0xff);
	Boompack0566->SetPosition(1944, 483);

	g_golden_shovel = CreateObject(Shovel, 1841, 1011);
	g_golden_shovel->SetMeshMaterial("GoldenShovel", 0);
	g_golden_shovel.StaticSaveVar = "g_golden_shovel";

	CreateObject(Pipe, 1838, 1016);

	var LotsOfCoins0567 = CreateObject(LotsOfCoins, 838, 592);
	LotsOfCoins0567.Plane = 500;

	var GemOfPower0575 = CreateObject(GemOfPower, 825, 572);
	GemOfPower0575->SetCategory(C4D_StaticBack);

	CreateObject(Firestone, 564, 1136);
	CreateObject(Firestone, 552, 1136);
	CreateObject(Firestone, 562, 1136);
	CreateObject(Firestone, 571, 1136);
	CreateObject(Firestone, 567, 1136);
	CreateObject(Firestone, 558, 1136);
	CreateObject(Firestone, 546, 1136);
	CreateObject(Firestone, 560, 1136);
	CreateObject(Firestone, 546, 1136);
	CreateObject(Firestone, 546, 1136);
	CreateObject(Firestone, 555, 1136);
	CreateObject(Firestone, 562, 1136);
	CreateObject(Firestone, 550, 1136);
	CreateObject(Firestone, 552, 1136);
	CreateObject(Firestone, 342, 1225);
	CreateObject(Firestone, 166, 1261);
	CreateObject(Firestone, 234, 1424);
	CreateObject(Firestone, 315, 431);
	CreateObject(Firestone, 1359, 1061);
	CreateObject(Firestone, 1348, 1042);
	CreateObject(Firestone, 1384, 1055);
	CreateObject(Firestone, 1417, 1107);
	CreateObject(Firestone, 1432, 1113);
	CreateObject(Firestone, 1436, 1104);
	CreateObject(Firestone, 1340, 920);
	CreateObject(Firestone, 1476, 876);
	CreateObject(Firestone, 1549, 866);
	CreateObject(Firestone, 1607, 792);
	CreateObject(Firestone, 2053, 852);
	CreateObject(Firestone, 2161, 943);
	CreateObject(Firestone, 2073, 862);
	CreateObject(Firestone, 2064, 852);

	return true;
}
