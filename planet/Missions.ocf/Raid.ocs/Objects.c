/* Automatically created objects file */

static g_chemical, g_cabin, g_sawmill, g_workshop, g_windmill, g_flagpole, npc_newton, npc_lara, npc_lisa, npc_woody, npc_rocky, npc_mave, npc_pyrit, npc_clonko, npc_matthi, npc_dora, g_guidepost1, g_guidepost2;

func InitializeObjects()
{
	var Time001 = CreateObject(Time);
	Time001->SetTime(600);
	Time001->SetCycleSpeed(20);

	CreateObject(Rule_NoPowerNeed);

	CreateObject(Rule_TeamAccount);

	CreateObjectAbove(EnvPack_Scarecrow, 1218, 440);

	g_guidepost1 = CreateObjectAbove(EnvPack_Guidepost, 835, 369);

	CreateObject(EnvPack_TreeTrunks, 808, 368);

	CreateObjectAbove(SproutBerryBush, 1154, 445);

	var Branch001 = CreateObject(Branch, 1509, 657);
	Branch001->SetR(-26);

	CreateObjectAbove(Trunk, 1194, 454);

	CreateObjectAbove(Tree_Coconut, 1487, 669);

	var Tree_Coniferous001 = CreateObject(Tree_Coniferous, 1068, 363);
	Tree_Coniferous001->SetR(6);
	CreateObjectAbove(Tree_Coniferous, 1371, 576);
	CreateObjectAbove(Tree_Coniferous, 1258, 470);
	var Tree_Coniferous002 = CreateObject(Tree_Coniferous, 1302, 464);
	Tree_Coniferous002->SetR(30);
	CreateObjectAbove(Tree_Coniferous, 1219, 441);

	g_guidepost2 = CreateObjectAbove(EnvPack_Guidepost, 2054, 521);

	CreateObjectAbove(Trunk, 2631, 589);

	CreateObjectAbove(SproutBerryBush, 2599, 590);
	CreateObjectAbove(SproutBerryBush, 2521, 582);
	CreateObject(SproutBerryBush, 3332, 645);
	CreateObjectAbove(SproutBerryBush, 2674, 593);

	var Branch002 = CreateObject(Branch, 2335, 600);
	Branch002->SetR(21);

	CreateObjectAbove(BigRock, 3273, 609);

	var LargeCaveMushroom001 = CreateObjectAbove(LargeCaveMushroom, 2877, 1342);
	LargeCaveMushroom001->SetClrModulation(0xffe1dde0);
	var LargeCaveMushroom002 = CreateObjectAbove(LargeCaveMushroom, 3101, 1371);
	LargeCaveMushroom002->SetClrModulation(0xffdde4da);
	var LargeCaveMushroom003 = CreateObjectAbove(LargeCaveMushroom, 2971, 1339);
	LargeCaveMushroom003->SetClrModulation(0xffe0eef5);
	var LargeCaveMushroom004 = CreateObjectAbove(LargeCaveMushroom, 2793, 1261);
	LargeCaveMushroom004->SetClrModulation(0xffdcd2ed);

	CreateObjectAbove(Tree_Coconut, 1822, 679);
	CreateObjectAbove(Tree_Coniferous2, 2567, 583);
	CreateObjectAbove(Tree_Coniferous2, 2107, 528);
	var Tree_Coniferous2001 = CreateObject(Tree_Coniferous2, 1409, 544);
	Tree_Coniferous2001->SetR(20);
	CreateObjectAbove(Tree_Coniferous2, 1157, 449);

	CreateObjectAbove(Tree_Coniferous, 1102, 432);

	var Tree_Coniferous2002 = CreateObject(Tree_Coniferous2, 1032, 358);
	Tree_Coniferous2002->SetR(-8);
	CreateObjectAbove(Tree_Coniferous3, 2359, 624);
	CreateObjectAbove(Tree_Coniferous3, 2424, 609);

	CreateObjectAbove(Flower, 907, 399);
	CreateObjectAbove(Flower, 997, 408);
	CreateObjectAbove(Flower, 938, 407);
	CreateObjectAbove(Flower, 1404, 599);
	var Flower005 = CreateObject(Flower, 1322, 533);
	Flower005->SetR(50);
	var Flower006 = CreateObject(Flower, 1328, 541);
	Flower006->SetR(50);
	var Flower007 = CreateObject(Flower, 1311, 523);
	Flower007->SetR(50);
	var Flower008 = CreateObjectAbove(Flower, 2600, 592);
	Flower008->SetSkin(2);
	var Flower009 = CreateObject(Flower, 2578, 574);
	Flower009->SetR(20);
	Flower009->SetSkin(2);

	CreateObject(Basement, 758, 370);
	CreateObject(Basement, 464, 368);
	CreateObject(Basement, 93, 391);
	CreateObject(Basement, 618, 370);
	CreateObject(Basement, 733, 369);
	CreateObject(Basement, 581, 370);
	CreateObject(Basement, 2758, 612);
	CreateObject(Basement, 2796, 613);
	CreateObject(Basement, 2837, 612);
	CreateObject(Basement, 3233, 603);
	CreateObject(Basement, 3205, 604);
	CreateObject(Basement, 3171, 603);
	CreateObject(Basement, 3061, 620);
	CreateObject(Basement, 3019, 618);
	CreateObject(Basement, 2967, 600);
	CreateObject(Basement, 2935, 600);
	CreateObject(Basement, 2897, 599);
	CreateObject(Basement, 2720, 611);

	var Foundry002 = CreateObjectAbove(Foundry, 944, 782);
	var Foundry001 = CreateObjectAbove(Foundry, 2958, 596);

	g_chemical = CreateObjectAbove(ChemicalLab, 734, 365);
	g_chemical.StaticSaveVar = "g_chemical";

	g_cabin = CreateObjectAbove(WoodenCabin, 546, 367);
	g_cabin.StaticSaveVar = "g_cabin";

	g_sawmill = CreateObjectAbove(Sawmill, 782, 366);
	g_sawmill.StaticSaveVar = "g_sawmill";

	CreateObjectAbove(Pump, 466, 363);

	g_workshop = CreateObjectAbove(ToolsWorkshop, 609, 365);
	g_workshop.StaticSaveVar = "g_workshop";
	var ToolsWorkshop001 = CreateObjectAbove(ToolsWorkshop, 2905, 595);

	CreateObjectAbove(Castle_ConstructionSite, 281, 343);

	CreateObjectAbove(Kitchen, 3030, 615);

	CreateObjectAbove(InventorsLab, 3212, 599);

	var Shipyard001 = CreateObjectAbove(Shipyard, 2763, 608);

	CreateObjectAbove(Loom, 3080, 616);

	var StoneDoor001 = CreateObject(StoneDoor, 540, 1244);
	StoneDoor001->SetComDir(COMD_Down);

	var SpinWheel001 = CreateObjectAbove(SpinWheel, 571, 1263);
	SpinWheel001->SetSwitchTarget(StoneDoor001);

	g_windmill = CreateObjectAbove(Windmill, 665, 351);
	g_windmill->SetCategory(C4D_StaticBack);
	g_windmill.StaticSaveVar = "g_windmill";

	g_flagpole = CreateObjectAbove(Flagpole, 502, 369);
	g_flagpole.StaticSaveVar = "g_flagpole";
	g_flagpole->SetNeutral(true);

	CreateObjectAbove(WindGenerator, 3163, 599);

	var Elevator001 = CreateObjectAbove(Elevator, 76, 387);
	Elevator001->SetDir(DIR_Right);
	Elevator001->CreateShaft(530);
	Elevator001->SetCasePosition(905);
	var Elevator002 = CreateObjectAbove(Elevator, 985, 782);
	Elevator002->CreateShaft(30);
	Elevator002->SetCasePosition(800);
	var Lorry001 = CreateObject(Lorry, 25, 771);
	Lorry001->SetR(11);
	var Lorry002 = CreateObject(Lorry, 3188, 591);
	Lorry002->SetR(6);
	Lorry002->SetRDir(7);

	npc_newton = CreateObjectAbove(Clonk, 226, 321);
	npc_newton->SetColor(0xffff);
	npc_newton->SetName("Newton");
	npc_newton->SetObjectLayer(npc_newton);
	npc_newton.StaticSaveVar = "npc_newton";
	npc_newton->SetDir(DIR_Left);
	npc_lara = CreateObjectAbove(Clonk, 300, 337);
	npc_lara->SetColor(0xff0000);
	npc_lara.StaticSaveVar = "npc_lara";
	npc_lara->SetName("Lara");
	npc_lara->SetObjectLayer(npc_lara);
	npc_lara->SetSkin(1);
	npc_lara->SetDir(DIR_Left);
	npc_lisa = CreateObjectAbove(Clonk, 496, 367);
	npc_lisa->SetColor(0xff00);
	npc_lisa.StaticSaveVar = "npc_lisa";
	npc_lisa->SetName("Lisa");
	npc_lisa->SetObjectLayer(npc_lisa);
	npc_lisa->SetSkin(3);
	npc_lisa->SetDir(DIR_Left);
	npc_woody = CreateObjectAbove(Clonk, 782, 367);
	npc_woody->SetColor(0x808000);
	npc_woody.StaticSaveVar = "npc_woody";
	npc_woody->SetName("Woody");
	npc_woody->SetObjectLayer(npc_woody);
	npc_woody->SetSkin(2);
	npc_woody->SetDir(DIR_Left);
	npc_rocky = CreateObjectAbove(Clonk, 98, 774);
	npc_rocky->SetDir(DIR_Right);
	npc_rocky->SetColor(0x808080);
	npc_rocky.StaticSaveVar = "npc_rocky";
	npc_rocky->SetName("Rocky");
	npc_rocky->SetObjectLayer(npc_rocky);
	npc_rocky->SetSkin(2);
	npc_mave = CreateObjectAbove(Clonk, 973, 783);
	npc_mave->SetDir(DIR_Right);
	npc_mave->SetColor(0xff8000);
	npc_mave->SetName("Mave");
	npc_mave->SetObjectLayer(npc_mave);
	npc_mave.StaticSaveVar = "npc_mave";
	npc_pyrit = CreateObjectAbove(Clonk, 2816, 607);
	npc_pyrit->SetColor(0xff0000);
	npc_pyrit.StaticSaveVar = "npc_pyrit";
	npc_pyrit->SetName("Pyrit");
	npc_pyrit->SetObjectLayer(npc_pyrit);
	npc_pyrit->SetSkin(2);
	npc_pyrit->SetDir(DIR_Left);
	npc_clonko = CreateObjectAbove(Clonk, 2934, 595);
	npc_clonko->SetDir(DIR_Right);
	npc_clonko->SetColor(0xff8000);
	npc_clonko->SetName("Clonko");
	npc_clonko->SetObjectLayer(npc_clonko);
	npc_clonko.StaticSaveVar = "npc_clonko";
	npc_matthi = CreateObjectAbove(Clonk, 3002, 613);
	npc_matthi->SetColor(0x80ff00);
	npc_matthi->SetName("Matthi");
	npc_matthi->SetObjectLayer(npc_matthi);
	npc_matthi.StaticSaveVar = "npc_matthi";
	npc_matthi->SetDir(DIR_Left);
	npc_dora = CreateObjectAbove(Clonk, 3178, 1367);
	npc_dora->SetDir(DIR_Right);
	npc_dora->SetColor(0xffff20);
	npc_dora.StaticSaveVar = "npc_dora";
	npc_dora->SetName("Dora");
	npc_dora->SetObjectLayer(npc_dora);
	npc_dora->SetSkin(3);

	var Catapult001 = CreateObjectAbove(Catapult, 2795, 608);
	Catapult001->SetRDir(-5);
	Catapult001->SetObjectLayer(npc_pyrit);

	CreateObjectAbove(Fish, 1889, 728);
	CreateObjectAbove(Fish, 1879, 726);
	
	CreateObjectAbove(Squid, 2100, 800);

	var Mosquito001 = CreateObjectAbove(Mosquito, 2292, 514);
	Mosquito001->SetXDir(-6);
	Mosquito001->SetCommand("Call", Mosquito001, nil, 0, nil, "MissionComplete");

	var Sickle001 = npc_matthi->CreateContents(Sickle);

	npc_matthi->SetDialogue("Matthi",true);

	var Hammer001 = npc_clonko->CreateContents(Hammer);

	npc_clonko->SetDialogue("Clonko",true);

	var Hammer002 = npc_pyrit->CreateContents(Hammer);
	var Hammer003 = npc_pyrit->CreateContents(Hammer);

	npc_pyrit->SetDialogue("Pyrit",true);

	var Bucket001 = npc_mave->CreateContents(Bucket);

	npc_mave->SetDialogue("Mave",false);

	var Hammer004 = npc_newton->CreateContents(Hammer);

	npc_newton->SetDialogue("Newton",true);
	npc_lara->SetDialogue("Lara",true);
	npc_lisa->SetDialogue("Lisa",true);

	var Axe001 = npc_woody->CreateContents(Axe);

	npc_woody->SetDialogue("Woody",true);

	var Pickaxe001 = npc_rocky->CreateContents(Pickaxe);

	npc_rocky->SetDialogue("Rocky",true);
	npc_dora->SetDialogue("Dora",true);

	CreateObjectAbove(Skull, 1566, 703);
	CreateObjectAbove(Skull, 3124, 1378);

	CreateObject(Rock, 685, 593);
	CreateObject(Rock, 793, 487);
	CreateObject(Rock, 1244, 514);
	CreateObject(Rock, 1480, 734);
	CreateObject(Rock, 794, 894);
	CreateObject(Rock, 311, 446);
	CreateObject(Rock, 1243, 791);
	CreateObject(Rock, 1231, 736);
	CreateObject(Rock, 563, 921);
	CreateObject(Rock, 1219, 1087);
	CreateObject(Rock, 428, 1318);
	CreateObject(Rock, 2100, 950);
	CreateObject(Rock, 1922, 1160);
	CreateObject(Rock, 2277, 967);
	CreateObject(Rock, 2405, 695);
	CreateObject(Rock, 2893, 988);
	CreateObject(Rock, 3131, 688);
	CreateObject(Rock, 3266, 1378);

	CreateObjectAbove(Ore, 2226, 943);
	Foundry001->CreateContents(Ore, 3);

	CreateObject(Loam, 1030, 446);
	CreateObject(Loam, 1122, 917);
	CreateObject(Loam, 1492, 801);
	CreateObjectAbove(Loam, 926, 1166);
	CreateObject(Loam, 456, 1001);
	CreateObject(Loam, 2315, 629);
	CreateObject(Loam, 2582, 787);
	CreateObject(Loam, 3056, 722);
	CreateObject(Loam, 3235, 789);
	CreateObject(Loam, 3167, 946);
	CreateObject(Loam, 2630, 1049);
	CreateObject(Loam, 3238, 1147);
	CreateObject(Loam, 2734, 1242);
	CreateObject(Loam, 3003, 1342);
	Foundry002->CreateContents(Loam, 2);
	CreateObject(Loam, 951, 1330);
	CreateObjectAbove(Loam, 970, 1355);
	CreateObjectAbove(Loam, 952, 1399);
	CreateObject(Loam, 808, 1333);
	CreateObject(Loam, 737, 1319);
	CreateObject(Loam, 652, 1277);
	CreateObject(Loam, 797, 1388);
	CreateObjectAbove(Loam, 1012, 1391);
	CreateObjectAbove(Loam, 492, 1263);
	CreateObjectAbove(Loam, 504, 1263);
	CreateObjectAbove(Loam, 500, 1263);

	CreateObjectAbove(Metal, 2217, 942);
	ToolsWorkshop001->CreateContents(Metal, 3);
	Shipyard001->CreateContents(Metal, 4);
	Lorry002->CreateContents(Metal, 2);

	CreateObject(Moss, 1529, 680);

	ToolsWorkshop001->CreateContents(Wood, 3);
	Shipyard001->CreateContents(Wood, 4);

	ToolsWorkshop001->CreateContents(Pickaxe, 2);

	Lorry001->CreateContents(Axe, 3);

	var Crate001 = CreateObjectAbove(Crate, 2836, 607);

	Crate001->CreateContents(Hammer);
	ToolsWorkshop001->CreateContents(Hammer, 2);

	CreateObjectAbove(Bucket, 435, 1271);
	CreateObjectAbove(Bucket, 943, 775);
	CreateObjectAbove(Bucket, 944, 775);
	CreateObjectAbove(Bucket, 946, 775);

	CreateObjectAbove(Crate, 2849, 607);
	CreateObjectAbove(Crate, 444, 1271);
	CreateObjectAbove(Crate, 473, 1263);
	CreateObjectAbove(Crate, 403, 1271);

	var Barrel001 = Foundry002->CreateContents(Barrel);
	Barrel001->AddRestoreMode(Foundry002, 944, 757);
	Barrel001->SetColor(0xff000000);
	var Barrel002 = Foundry002->CreateContents(Barrel);
	Barrel002->AddRestoreMode(Foundry002, 944, 757);
	Barrel002->SetColor(0xff000000);
	var Barrel003 = CreateObject(Barrel, 484, 361);
	Barrel003->SetR(23);
	Barrel003->SetColor(0xff000000);
	Barrel003->SetObjectLayer(Barrel003);
	var Barrel004 = CreateObject(Barrel, 648, 345);
	Barrel004->SetR(-22);
	Barrel004->SetColor(0xff000000);
	Barrel004->SetObjectLayer(Barrel004);
	var Barrel005 = CreateObjectAbove(Barrel, 244, 321);
	Barrel005->SetColor(0xff000000);
	Barrel005->SetObjectLayer(Barrel005);
	var Barrel006 = CreateObjectAbove(Barrel, 396, 343);
	Barrel006->SetColor(0xff000000);
	Barrel006->SetObjectLayer(Barrel006);

	CreateObjectAbove(Mushroom, 1192, 448);
	CreateObjectAbove(Mushroom, 1170, 439);
	CreateObjectAbove(Mushroom, 1492, 663);
	CreateObjectAbove(Mushroom, 1131, 434);
	CreateObjectAbove(Mushroom, 1523, 674);
	CreateObjectAbove(Mushroom, 1163, 438);
	CreateObjectAbove(Mushroom, 1070, 414);
	CreateObjectAbove(Mushroom, 1010, 399);
	CreateObjectAbove(Mushroom, 960, 400);
	CreateObjectAbove(Mushroom, 1175, 440);
	CreateObjectAbove(Mushroom, 1120, 432);
	CreateObjectAbove(Mushroom, 989, 398);
	CreateObjectAbove(Mushroom, 968, 398);
	CreateObjectAbove(Mushroom, 1013, 399);
	CreateObjectAbove(Mushroom, 1496, 662);
	CreateObjectAbove(Mushroom, 1088, 423);
	CreateObjectAbove(Mushroom, 1545, 696);
	CreateObjectAbove(Mushroom, 1223, 438);
	CreateObjectAbove(Mushroom, 943, 399);
	CreateObjectAbove(Mushroom, 1006, 399);

	CreateObjectAbove(Seaweed, 1952, 903);
	CreateObjectAbove(Seaweed, 2013, 911);
	CreateObjectAbove(Seaweed, 1903, 887);
	CreateObjectAbove(Seaweed, 1983, 911);
	CreateObjectAbove(Seaweed, 2207, 942);
	CreateObjectAbove(Seaweed, 2127, 895);
	CreateObjectAbove(Seaweed, 2227, 943);
	CreateObjectAbove(Seaweed, 2191, 927);
	CreateObjectAbove(Seaweed, 2232, 943);
	CreateObjectAbove(Seaweed, 2269, 927);
	CreateObjectAbove(Seaweed, 2249, 935);

	CreateObjectAbove(DynamiteBox, 2844, 607);
	CreateObjectAbove(DynamiteBox, 452, 1271);
	var DynamiteBox001 = CreateObject(DynamiteBox, 430, 1269);
	DynamiteBox001->SetR(10);

	var MetalBarrel001 = CreateObjectAbove(MetalBarrel, 395, 1271);
	MetalBarrel001->SetColor(0xff000000);
	MetalBarrel001->PutLiquid("Oil");
	var MetalBarrel002 = CreateObject(MetalBarrel, 421, 1268);
	MetalBarrel002->SetR(-104);
	MetalBarrel002->SetColor(0xff000000);
	MetalBarrel002->PutLiquid("Oil");
	var MetalBarrel003 = CreateObjectAbove(MetalBarrel, 411, 1271);
	MetalBarrel003->SetColor(0xff000000);
	MetalBarrel003->PutLiquid("Oil");
	var MetalBarrel004 = CreateObjectAbove(MetalBarrel, 385, 1271);
	MetalBarrel004->SetColor(0xff000000);
	MetalBarrel004->PutLiquid("Oil");

	var PowderKeg001 = CreateObject(PowderKeg, 378, 1268);
	PowderKeg001->SetR(99);

	var WindBag001 = CreateObject(WindBag, 382, 1268);
	WindBag001->SetR(-29);
	CreateObject(Firestone, 1272, 961);
	CreateObject(Firestone, 1763, 900);
	CreateObject(Firestone, 1415, 708);
	CreateObject(Firestone, 772, 621);
	CreateObject(Firestone, 1196, 493);
	CreateObject(Firestone, 345, 692);
	Lorry002->CreateContents(Firestone, 2);
	CreateObject(Firestone, 2460, 1366);
	CreateObject(Firestone, 2893, 671);
	CreateObject(Firestone, 2998, 959);
	CreateObject(Firestone, 3266, 1172);
	CreateObject(Firestone, 2653, 1129);
	CreateObject(Firestone, 2410, 1165);
	CreateObject(Firestone, 2853, 1378);
	return true;
}
