/* Automatically created objects file */

static g_chemical, g_cabin, g_sawmill, g_workshop, g_flagpole, g_windmill, npc_newton, npc_lara, npc_lisa, npc_woody, npc_rocky, npc_mave, npc_pyrit, npc_clonko, npc_matthi, npc_dora;

func InitializeObjects()
{
	var Rule_BaseRespawn01 = CreateObject(Rule_BaseRespawn, 0, 0);
	Rule_BaseRespawn01->SetInventoryTransfer(true);
	Rule_BaseRespawn01->SetFreeCrew(true);
	
	CreateObject(Grass, 1121, 429);
	CreateObject(Grass, 1185, 444);

	CreateObject(Rule_NoPowerNeed, 0, 0);

	CreateObject(Rule_TeamAccount, 0, 0);

	CreateObject(EnvPack_Scarecrow, 1218, 440);

	CreateObject(EnvPack_Guidepost, 835, 369);

	CreateObject(EnvPack_TreeTrunks, 808, 368);

	CreateObject(SproutBerryBush, 1154, 445);

	var Rank0012 = CreateObject(Rank, 1509, 664);
	Rank0012->SetR(-26);
	Rank0012->SetPosition(1509, 661);

	CreateObject(Trunk, 1194, 454);

	CreateObject(Tree_Coconut, 1487, 669);

	var Tree_Coniferous0018 = CreateObject(Tree_Coniferous, 1068, 408);
	Tree_Coniferous0018->SetR(6);
	Tree_Coniferous0018->SetPosition(1068, 363);
	var Tree_Coniferous0022 = CreateObject(Tree_Coniferous, 1032, 403);
	Tree_Coniferous0022->SetR(-8);
	Tree_Coniferous0022->SetPosition(1032, 358);
	CreateObject(Tree_Coniferous, 1371, 576);
	CreateObject(Tree_Coniferous, 1258, 470);
	CreateObject(Tree_Coniferous, 1085, 429);
	CreateObject(Tree_Coniferous, 1155, 441);
	var Tree_Coniferous0042 = CreateObject(Tree_Coniferous, 1302, 503);
	Tree_Coniferous0042->SetR(30);
	Tree_Coniferous0042->SetPosition(1302, 464);
	var Tree_Coniferous0046 = CreateObject(Tree_Coniferous, 1328, 541);
	Tree_Coniferous0046->SetR(20);
	Tree_Coniferous0046->SetPosition(1328, 499);
	CreateObject(Tree_Coniferous, 1219, 441);

	CreateObject(EnvPack_Guidepost, 2054, 521);

	CreateObject(Tree_Coniferous, 2107, 520);
	CreateObject(Tree_Coniferous, 2421, 612);
	CreateObject(Tree_Coniferous, 2569, 576);

	CreateObject(Trunk, 2631, 589);

	CreateObject(SproutBerryBush, 2599, 590);
	CreateObject(SproutBerryBush, 2521, 582);
	CreateObject(SproutBerryBush, 3332, 653);
	CreateObject(SproutBerryBush, 2674, 593);

	var Rank0084 = CreateObject(Rank, 2335, 607);
	Rank0084->SetR(21);
	Rank0084->SetPosition(2335, 604);

	CreateObject(BigRock, 3273, 609);

	var LargeCaveMushroom0086 = CreateObject(LargeCaveMushroom, 2877, 1342);
	LargeCaveMushroom0086->SetClrModulation(0xffe1dde0);
	var LargeCaveMushroom0090 = CreateObject(LargeCaveMushroom, 3101, 1371);
	LargeCaveMushroom0090->SetClrModulation(0xffdde4da);
	var LargeCaveMushroom0094 = CreateObject(LargeCaveMushroom, 2971, 1339);
	LargeCaveMushroom0094->SetClrModulation(0xffe0eef5);
	var LargeCaveMushroom0098 = CreateObject(LargeCaveMushroom, 2793, 1261);
	LargeCaveMushroom0098->SetClrModulation(0xffdcd2ed);

	CreateObject(Tree_Coconut, 1822, 679);

	var Basement0123 = CreateObject(Basement, 2720, 615);
	Basement0123->SetCategory(C4D_StaticBack);
	var Basement0122 = CreateObject(Basement, 2897, 603);
	Basement0122->SetCategory(C4D_StaticBack);
	var Basement0121 = CreateObject(Basement, 2935, 604);
	Basement0121->SetCategory(C4D_StaticBack);
	var Basement0120 = CreateObject(Basement, 2967, 604);
	Basement0120->SetCategory(C4D_StaticBack);
	var Basement0119 = CreateObject(Basement, 3019, 622);
	Basement0119->SetCategory(C4D_StaticBack);
	var Basement0118 = CreateObject(Basement, 3061, 624);
	Basement0118->SetCategory(C4D_StaticBack);
	var Basement0117 = CreateObject(Basement, 3171, 607);
	Basement0117->SetCategory(C4D_StaticBack);
	var Basement0116 = CreateObject(Basement, 3205, 608);
	Basement0116->SetCategory(C4D_StaticBack);
	var Basement0115 = CreateObject(Basement, 3230, 609);
	Basement0115->SetCategory(C4D_StaticBack);
	var Basement0114 = CreateObject(Basement, 2837, 616);
	Basement0114->SetCategory(C4D_StaticBack);
	var Basement0113 = CreateObject(Basement, 2796, 617);
	Basement0113->SetCategory(C4D_StaticBack);
	var Basement0112 = CreateObject(Basement, 2758, 616);
	Basement0112->SetCategory(C4D_StaticBack);
	var Basement0111 = CreateObject(Basement, 581, 374);
	Basement0111->SetCategory(C4D_StaticBack);
	var Basement0110 = CreateObject(Basement, 733, 373);
	Basement0110->SetCategory(C4D_StaticBack);
	var Basement0109 = CreateObject(Basement, 618, 374);
	Basement0109->SetCategory(C4D_StaticBack);
	var Basement0108 = CreateObject(Basement, 93, 395);
	Basement0108->SetCategory(C4D_StaticBack);
	var Basement0107 = CreateObject(Basement, 464, 372);
	Basement0107->SetCategory(C4D_StaticBack);
	var Basement0106 = CreateObject(Basement, 758, 374);
	Basement0106->SetCategory(C4D_StaticBack);

	var Foundry0124 = CreateObject(Foundry, 944, 782);
	var Foundry0127 = CreateObject(Foundry, 2958, 596);

	g_chemical = CreateObject(ChemicalLab, 734, 365);
	g_chemical.StaticSaveVar = "g_chemical";

	g_cabin = CreateObject(WoodenCabin, 546, 367);
	g_cabin.StaticSaveVar = "g_cabin";

	g_sawmill = CreateObject(Sawmill, 782, 366);
	g_sawmill.StaticSaveVar = "g_sawmill";

	CreateObject(Pump, 466, 363);

	g_workshop = CreateObject(ToolsWorkshop, 609, 365);
	g_workshop.StaticSaveVar = "g_workshop";
	var ToolsWorkshop0139 = CreateObject(ToolsWorkshop, 2905, 595);

	CreateObject(Castle_ConstructionSite, 281, 343);

	CreateObject(Kitchen, 3030, 615);

	CreateObject(InventorsLab, 3212, 599);

	var Shipyard0146 = CreateObject(Shipyard, 2763, 608);

	CreateObject(Loom, 3080, 616);

	var StoneDoor0150 = CreateObject(StoneDoor, 540, 1263);
	StoneDoor0150->SetComDir(COMD_Down);

	var SpinWheel0151 = CreateObject(SpinWheel, 571, 1263);
	SpinWheel0151->SetStoneDoor(StoneDoor0150);

	var WindGenerator0269 = CreateObject(WindGenerator, 3163, 599);
	WindGenerator0269->SetCategory(C4D_StaticBack);

	g_flagpole = CreateObject(Flagpole, 502, 369);
	g_flagpole->SetNeutral(true);
	g_flagpole.StaticSaveVar = "g_flagpole";

	g_windmill = CreateObject(Windmill, 665, 351);
	g_windmill->SetCategory(C4D_StaticBack);
	g_windmill.StaticSaveVar = "g_windmill";

	var Elevator0361 = CreateObject(Elevator, 76, 387);
	Elevator0361->SetDir(DIR_Right);
	Elevator0361->CreateShaft(530);
	Elevator0361->SetCasePosition(905);

	var Lorry0369 = CreateObject(Lorry, 25, 779);
	Lorry0369->SetR(11);
	Lorry0369->SetPosition(25, 771);
	var Lorry0371 = CreateObject(Lorry, 3188, 598);

	var Catapult0373 = CreateObject(Catapult, 2795, 608);
	Catapult0373->SetRDir(4);
	Catapult0373->SetObjectLayer(Catapult0373);

	npc_newton = CreateObject(Clonk, 226, 321);
	npc_newton->SetColor(0xffff);
	npc_newton->SetName("Newton");
	npc_newton->SetObjectLayer(npc_newton);
	npc_newton.StaticSaveVar = "npc_newton";
	npc_newton->SetDir(DIR_Left);
	npc_lara = CreateObject(Clonk, 300, 337);
	npc_lara->SetColor(0xff0000);
	npc_lara.StaticSaveVar = "npc_lara";
	npc_lara->SetName("Lara");
	npc_lara->SetObjectLayer(npc_lara);
	npc_lara->SetSkin(1);
	npc_lara->SetDir(DIR_Left);
	npc_lisa = CreateObject(Clonk, 496, 367);
	npc_lisa->SetColor(0xff00);
	npc_lisa.StaticSaveVar = "npc_lisa";
	npc_lisa->SetName("Lisa");
	npc_lisa->SetObjectLayer(npc_lisa);
	npc_lisa->SetSkin(3);
	npc_lisa->SetDir(DIR_Left);
	npc_woody = CreateObject(Clonk, 782, 367);
	npc_woody->SetColor(0x808000);
	npc_woody.StaticSaveVar = "npc_woody";
	npc_woody->SetName("Woody");
	npc_woody->SetObjectLayer(npc_woody);
	npc_woody->SetSkin(2);
	npc_woody->SetDir(DIR_Left);
	npc_rocky = CreateObject(Clonk, 98, 774);
	npc_rocky->SetDir(DIR_Right);
	npc_rocky->SetColor(0x808080);
	npc_rocky.StaticSaveVar = "npc_rocky";
	npc_rocky->SetName("Rocky");
	npc_rocky->SetObjectLayer(npc_rocky);
	npc_rocky->SetSkin(2);
	npc_mave = CreateObject(Clonk, 973, 783);
	npc_mave->SetDir(DIR_Right);
	npc_mave->SetColor(0xff8000);
	npc_mave->SetName("Mave");
	npc_mave->SetObjectLayer(npc_mave);
	npc_mave.StaticSaveVar = "npc_mave";
	npc_pyrit = CreateObject(Clonk, 2816, 607);
	npc_pyrit->SetColor(0xff0000);
	npc_pyrit.StaticSaveVar = "npc_pyrit";
	npc_pyrit->SetName("Pyrit");
	npc_pyrit->SetObjectLayer(npc_pyrit);
	npc_pyrit->SetSkin(2);
	npc_pyrit->SetDir(DIR_Left);
	npc_clonko = CreateObject(Clonk, 2934, 595);
	npc_clonko->SetDir(DIR_Right);
	npc_clonko->SetColor(0xff8000);
	npc_clonko->SetName("Clonko");
	npc_clonko->SetObjectLayer(npc_clonko);
	npc_clonko.StaticSaveVar = "npc_clonko";
	npc_matthi = CreateObject(Clonk, 3002, 613);
	npc_matthi->SetColor(0x80ff00);
	npc_matthi->SetName("Matthi");
	npc_matthi->SetObjectLayer(npc_matthi);
	npc_matthi.StaticSaveVar = "npc_matthi";
	npc_matthi->SetDir(DIR_Left);
	npc_dora = CreateObject(Clonk, 3178, 1367);
	npc_dora->SetDir(DIR_Right);
	npc_dora->SetColor(0xffff20);
	npc_dora.StaticSaveVar = "npc_dora";
	npc_dora->SetName("Dora");
	npc_dora->SetObjectLayer(npc_dora);
	npc_dora->SetSkin(3);

	CreateObject(Fish, 1923, 729);
	CreateObject(Fish, 1924, 746);

	npc_dora->SetDialogue("Dora",true);

	var Pickaxe0478 = npc_rocky->CreateContents(Pickaxe);

	npc_rocky->SetDialogue("Rocky",true);

	var Axe0473 = npc_woody->CreateContents(Axe);

	npc_woody->SetDialogue("Woody",true);
	npc_lisa->SetDialogue("Lisa",true);
	npc_lara->SetDialogue("Lara",true);

	var Hammer0462 = npc_newton->CreateContents(Hammer);

	npc_newton->SetDialogue("Newton",true);

	var Bucket0460 = npc_mave->CreateContents(Bucket);

	npc_mave->SetDialogue("Mave",false);

	var Hammer0455 = npc_pyrit->CreateContents(Hammer);
	var Hammer0454 = npc_pyrit->CreateContents(Hammer);

	npc_pyrit->SetDialogue("Pyrit",true);

	var Hammer0451 = npc_clonko->CreateContents(Hammer);

	npc_clonko->SetDialogue("Clonko",true);

	var Sickle0448 = npc_matthi->CreateContents(Sickle);

	npc_matthi->SetDialogue("Matthi",true);

	CreateObject(Skull, 1566, 703);
	CreateObject(Skull, 3124, 1378);

	CreateObject(Rock, 685, 594);
	CreateObject(Rock, 793, 488);
	CreateObject(Rock, 1244, 515);
	CreateObject(Rock, 1480, 735);
	CreateObject(Rock, 794, 895);
	CreateObject(Rock, 311, 447);
	CreateObject(Rock, 1243, 792);
	CreateObject(Rock, 1231, 737);
	CreateObject(Rock, 563, 922);
	CreateObject(Rock, 1219, 1088);
	CreateObject(Rock, 428, 1319);
	CreateObject(Rock, 2100, 951);
	CreateObject(Rock, 1922, 1161);
	CreateObject(Rock, 2277, 968);
	CreateObject(Rock, 2405, 696);
	CreateObject(Rock, 2893, 989);
	CreateObject(Rock, 3131, 689);
	CreateObject(Rock, 3266, 1379);

	CreateObject(Ore, 2226, 943);
	Foundry0127->CreateContents(Ore);
	Foundry0127->CreateContents(Ore);
	Foundry0127->CreateContents(Ore);

	CreateObject(Loam, 1030, 449);
	CreateObject(Loam, 1122, 920);
	CreateObject(Loam, 1492, 804);
	CreateObject(Loam, 935, 1132);
	CreateObject(Loam, 456, 1004);
	CreateObject(Loam, 2315, 632);
	CreateObject(Loam, 2582, 790);
	CreateObject(Loam, 3056, 725);
	CreateObject(Loam, 3235, 792);
	CreateObject(Loam, 3167, 949);
	CreateObject(Loam, 2630, 1052);
	CreateObject(Loam, 3238, 1150);
	CreateObject(Loam, 2734, 1245);
	CreateObject(Loam, 3003, 1345);
	Foundry0124->CreateContents(Loam);
	Foundry0124->CreateContents(Loam);
	CreateObject(Loam, 951, 1333);
	CreateObject(Loam, 970, 1355);
	CreateObject(Loam, 952, 1364);
	CreateObject(Loam, 808, 1336);
	CreateObject(Loam, 737, 1322);
	CreateObject(Loam, 652, 1280);
	CreateObject(Loam, 797, 1391);
	CreateObject(Loam, 1021, 1339);
	CreateObject(Loam, 492, 1263);
	CreateObject(Loam, 504, 1263);
	CreateObject(Loam, 500, 1263);

	CreateObject(Metal, 2217, 942);
	ToolsWorkshop0139->CreateContents(Metal);
	ToolsWorkshop0139->CreateContents(Metal);
	ToolsWorkshop0139->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Lorry0371->CreateContents(Metal);
	Lorry0371->CreateContents(Metal);

	CreateObject(Moss, 1529, 681);

	ToolsWorkshop0139->CreateContents(Wood);
	ToolsWorkshop0139->CreateContents(Wood);
	ToolsWorkshop0139->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);

	var Crate0548 = CreateObject(Crate, 2836, 607);

	var Hammer0549 = Crate0548->CreateContents(Hammer);
	Hammer0549->SetR(50);
	ToolsWorkshop0139->CreateContents(Hammer);
	ToolsWorkshop0139->CreateContents(Hammer);

	Foundry0124->CreateContents(Bucket);
	Foundry0124->CreateContents(Bucket);
	Foundry0124->CreateContents(Bucket);
	CreateObject(Bucket, 435, 1271);

	Lorry0369->CreateContents(Axe);
	Lorry0369->CreateContents(Axe);
	Lorry0369->CreateContents(Axe);

	ToolsWorkshop0139->CreateContents(Pickaxe);
	ToolsWorkshop0139->CreateContents(Pickaxe);

	CreateObject(Crate, 2849, 607);
	CreateObject(Crate, 444, 1271);
	CreateObject(Crate, 473, 1263);
	CreateObject(Crate, 403, 1271);

	var Barrel0560 = Foundry0124->CreateContents(Barrel);
	Barrel0560->SetColor(0xff000000);
	Barrel0560->AddRestoreMode(Foundry0124);
	var Barrel0562 = Foundry0124->CreateContents(Barrel);
	Barrel0562->SetColor(0xff000000);
	Barrel0562->AddRestoreMode(Foundry0124);
	var Barrel0564 = CreateObject(Barrel, 484, 367);
	Barrel0564->SetR(23);
	Barrel0564->SetColor(0xff000000);
	Barrel0564->SetObjectLayer(Barrel0564);
	Barrel0564->SetPosition(484, 361);
	var Barrel0566 = CreateObject(Barrel, 648, 351);
	Barrel0566->SetR(-22);
	Barrel0566->SetColor(0xff000000);
	Barrel0566->SetObjectLayer(Barrel0566);
	Barrel0566->SetPosition(648, 345);
	var Barrel0568 = CreateObject(Barrel, 244, 321);
	Barrel0568->SetColor(0xff000000);
	Barrel0568->SetObjectLayer(Barrel0568);
	var Barrel0570 = CreateObject(Barrel, 396, 343);
	Barrel0570->SetColor(0xff000000);
	Barrel0570->SetObjectLayer(Barrel0570);

	CreateObject(Mushroom, 1192, 448);
	CreateObject(Mushroom, 1170, 440);
	CreateObject(Mushroom, 1492, 663);
	CreateObject(Mushroom, 1131, 436);
	CreateObject(Mushroom, 1523, 675);
	CreateObject(Mushroom, 1163, 440);
	CreateObject(Mushroom, 1070, 414);
	CreateObject(Mushroom, 1010, 399);
	CreateObject(Mushroom, 960, 400);
	CreateObject(Mushroom, 1175, 440);
	CreateObject(Mushroom, 1120, 432);
	CreateObject(Mushroom, 989, 400);
	CreateObject(Mushroom, 968, 400);
	CreateObject(Mushroom, 1013, 400);
	CreateObject(Mushroom, 1496, 664);
	CreateObject(Mushroom, 1088, 424);
	CreateObject(Mushroom, 1545, 696);
	CreateObject(Mushroom, 1223, 440);
	CreateObject(Mushroom, 943, 399);
	CreateObject(Mushroom, 1006, 400);

	var Seaweed0652 = CreateObject(Seaweed, 1952, 903);
	Seaweed0652->SetPhase(26);
	var Seaweed0655 = CreateObject(Seaweed, 2013, 911);
	Seaweed0655->SetPhase(1);
	var Seaweed0658 = CreateObject(Seaweed, 1903, 887);
	Seaweed0658->SetPhase(57);
	var Seaweed0661 = CreateObject(Seaweed, 1983, 911);
	Seaweed0661->SetPhase(35);
	var Seaweed0664 = CreateObject(Seaweed, 2207, 942);
	Seaweed0664->SetPhase(29);
	var Seaweed0667 = CreateObject(Seaweed, 2127, 895);
	Seaweed0667->SetPhase(44);
	var Seaweed0670 = CreateObject(Seaweed, 2227, 943);
	Seaweed0670->SetPhase(29);
	var Seaweed0673 = CreateObject(Seaweed, 2191, 927);
	Seaweed0673->SetPhase(29);
	var Seaweed0676 = CreateObject(Seaweed, 2232, 943);
	Seaweed0676->SetPhase(66);
	var Seaweed0679 = CreateObject(Seaweed, 2269, 927);
	Seaweed0679->SetPhase(66);
	var Seaweed0682 = CreateObject(Seaweed, 2249, 935);
	Seaweed0682->SetPhase(66);

	CreateObject(DynamiteBox, 2844, 607);
	CreateObject(DynamiteBox, 452, 1271);
	var DynamiteBox0687 = CreateObject(DynamiteBox, 430, 1271);
	DynamiteBox0687->SetR(10);
	DynamiteBox0687->SetPosition(430, 1269);

	var MetalBarrel0688 = CreateObject(MetalBarrel, 395, 1271);
	MetalBarrel0688->SetColor(0xff000000);
	var MetalBarrel0690 = CreateObject(MetalBarrel, 421, 1271);
	MetalBarrel0690->SetR(-104);
	MetalBarrel0690->SetColor(0xff000000);
	MetalBarrel0690->SetPosition(421, 1268);
	var MetalBarrel0692 = CreateObject(MetalBarrel, 411, 1271);
	MetalBarrel0692->SetColor(0xff000000);
	var MetalBarrel0694 = CreateObject(MetalBarrel, 385, 1271);
	MetalBarrel0694->SetColor(0xff000000);

	var PowderKeg0696 = CreateObject(PowderKeg, 378, 1271);
	PowderKeg0696->SetR(99);
	PowderKeg0696->SetPosition(378, 1268);

	var WindBag0698 = CreateObject(WindBag, 382, 1271);
	WindBag0698->SetR(-29);
	WindBag0698->SetPosition(382, 1268);

	CreateObject(Firestone, 1272, 962);
	CreateObject(Firestone, 1763, 901);
	CreateObject(Firestone, 1415, 709);
	CreateObject(Firestone, 772, 622);
	CreateObject(Firestone, 1196, 494);
	CreateObject(Firestone, 345, 693);
	Lorry0371->CreateContents(Firestone);
	Lorry0371->CreateContents(Firestone);
	CreateObject(Firestone, 2460, 1367);
	CreateObject(Firestone, 2893, 672);
	CreateObject(Firestone, 2998, 960);
	CreateObject(Firestone, 3266, 1173);
	CreateObject(Firestone, 2653, 1130);
	CreateObject(Firestone, 2410, 1166);
	CreateObject(Firestone, 2853, 1379);

	return true;
}
