/* Automatically created objects file */

static g_chemical, g_cabin, g_sawmill, g_workshop, g_flagpole, g_windmill, npc_newton, npc_lara, npc_lisa, npc_woody, npc_rocky, npc_mave, npc_pyrit, npc_clonko, npc_matthi, npc_dora;

func InitializeObjects()
{
	var Rule_BaseRespawn01 = CreateObject(Rule_BaseRespawn, 0, 0);
	Rule_BaseRespawn01->SetInventoryTransfer(true);
	Rule_BaseRespawn01->SetFreeCrew(true);
	
	CreateObjectAbove(Grass, 1121, 429);
	CreateObjectAbove(Grass, 1185, 444);

	CreateObject(Rule_NoPowerNeed, 0, 0);

	CreateObject(Rule_TeamAccount, 0, 0);

	CreateObjectAbove(EnvPack_Scarecrow, 1218, 440);

	CreateObjectAbove(EnvPack_Guidepost, 835, 369);

	CreateObjectAbove(EnvPack_TreeTrunks, 808, 368);

	CreateObjectAbove(SproutBerryBush, 1154, 445);

	var Branch0012 = CreateObjectAbove(Branch, 1509, 664);
	Branch0012->SetR(-26);
	Branch0012->SetPosition(1509, 657);

	CreateObjectAbove(Trunk, 1194, 454);

	CreateObjectAbove(Tree_Coconut, 1487, 669);

	var Tree_Coniferous0018 = CreateObjectAbove(Tree_Coniferous, 1068, 408);
	Tree_Coniferous0018->SetR(6);
	Tree_Coniferous0018->SetPosition(1068, 363);
	var Tree_Coniferous0022 = CreateObjectAbove(Tree_Coniferous, 1032, 403);
	Tree_Coniferous0022->SetR(-8);
	Tree_Coniferous0022->SetPosition(1032, 358);
	CreateObjectAbove(Tree_Coniferous, 1371, 576);
	CreateObjectAbove(Tree_Coniferous, 1258, 470);
	CreateObjectAbove(Tree_Coniferous, 1085, 429);
	CreateObjectAbove(Tree_Coniferous, 1155, 441);
	var Tree_Coniferous0042 = CreateObjectAbove(Tree_Coniferous, 1302, 503);
	Tree_Coniferous0042->SetR(30);
	Tree_Coniferous0042->SetPosition(1302, 464);
	var Tree_Coniferous0046 = CreateObjectAbove(Tree_Coniferous, 1328, 541);
	Tree_Coniferous0046->SetR(20);
	Tree_Coniferous0046->SetPosition(1328, 499);
	CreateObjectAbove(Tree_Coniferous, 1219, 441);

	CreateObjectAbove(EnvPack_Guidepost, 2054, 521);

	CreateObjectAbove(Tree_Coniferous, 2107, 520);
	CreateObjectAbove(Tree_Coniferous, 2421, 612);
	CreateObjectAbove(Tree_Coniferous, 2569, 576);

	CreateObjectAbove(Trunk, 2631, 589);

	CreateObjectAbove(SproutBerryBush, 2599, 590);
	CreateObjectAbove(SproutBerryBush, 2521, 582);
	CreateObjectAbove(SproutBerryBush, 3332, 653);
	CreateObjectAbove(SproutBerryBush, 2674, 593);

	var Branch0084 = CreateObjectAbove(Branch, 2335, 607);
	Branch0084->SetR(21);
	Branch0084->SetPosition(2335, 600);

	CreateObjectAbove(BigRock, 3273, 609);

	var LargeCaveMushroom0086 = CreateObjectAbove(LargeCaveMushroom, 2877, 1342);
	LargeCaveMushroom0086->SetClrModulation(0xffe1dde0);
	var LargeCaveMushroom0090 = CreateObjectAbove(LargeCaveMushroom, 3101, 1371);
	LargeCaveMushroom0090->SetClrModulation(0xffdde4da);
	var LargeCaveMushroom0094 = CreateObjectAbove(LargeCaveMushroom, 2971, 1339);
	LargeCaveMushroom0094->SetClrModulation(0xffe0eef5);
	var LargeCaveMushroom0098 = CreateObjectAbove(LargeCaveMushroom, 2793, 1261);
	LargeCaveMushroom0098->SetClrModulation(0xffdcd2ed);

	CreateObjectAbove(Tree_Coconut, 1822, 679);

	var Basement0123 = CreateObjectAbove(Basement, 2720, 615);
	Basement0123->SetCategory(C4D_StaticBack);
	var Basement0122 = CreateObjectAbove(Basement, 2897, 603);
	Basement0122->SetCategory(C4D_StaticBack);
	var Basement0121 = CreateObjectAbove(Basement, 2935, 604);
	Basement0121->SetCategory(C4D_StaticBack);
	var Basement0120 = CreateObjectAbove(Basement, 2967, 604);
	Basement0120->SetCategory(C4D_StaticBack);
	var Basement0119 = CreateObjectAbove(Basement, 3019, 622);
	Basement0119->SetCategory(C4D_StaticBack);
	var Basement0118 = CreateObjectAbove(Basement, 3061, 624);
	Basement0118->SetCategory(C4D_StaticBack);
	var Basement0117 = CreateObjectAbove(Basement, 3171, 607);
	Basement0117->SetCategory(C4D_StaticBack);
	var Basement0116 = CreateObjectAbove(Basement, 3205, 608);
	Basement0116->SetCategory(C4D_StaticBack);
	var Basement0115 = CreateObjectAbove(Basement, 3230, 609);
	Basement0115->SetCategory(C4D_StaticBack);
	var Basement0114 = CreateObjectAbove(Basement, 2837, 616);
	Basement0114->SetCategory(C4D_StaticBack);
	var Basement0113 = CreateObjectAbove(Basement, 2796, 617);
	Basement0113->SetCategory(C4D_StaticBack);
	var Basement0112 = CreateObjectAbove(Basement, 2758, 616);
	Basement0112->SetCategory(C4D_StaticBack);
	var Basement0111 = CreateObjectAbove(Basement, 581, 374);
	Basement0111->SetCategory(C4D_StaticBack);
	var Basement0110 = CreateObjectAbove(Basement, 733, 373);
	Basement0110->SetCategory(C4D_StaticBack);
	var Basement0109 = CreateObjectAbove(Basement, 618, 374);
	Basement0109->SetCategory(C4D_StaticBack);
	var Basement0108 = CreateObjectAbove(Basement, 93, 395);
	Basement0108->SetCategory(C4D_StaticBack);
	var Basement0107 = CreateObjectAbove(Basement, 464, 372);
	Basement0107->SetCategory(C4D_StaticBack);
	var Basement0106 = CreateObjectAbove(Basement, 758, 374);
	Basement0106->SetCategory(C4D_StaticBack);

	var Foundry0124 = CreateObjectAbove(Foundry, 944, 782);
	var Foundry0127 = CreateObjectAbove(Foundry, 2958, 596);

	g_chemical = CreateObjectAbove(ChemicalLab, 734, 365);
	g_chemical.StaticSaveVar = "g_chemical";

	g_cabin = CreateObjectAbove(WoodenCabin, 546, 367);
	g_cabin.StaticSaveVar = "g_cabin";

	g_sawmill = CreateObjectAbove(Sawmill, 782, 366);
	g_sawmill.StaticSaveVar = "g_sawmill";

	CreateObjectAbove(Pump, 466, 363);

	g_workshop = CreateObjectAbove(ToolsWorkshop, 609, 365);
	g_workshop.StaticSaveVar = "g_workshop";
	var ToolsWorkshop0139 = CreateObjectAbove(ToolsWorkshop, 2905, 595);

	CreateObjectAbove(Castle_ConstructionSite, 281, 343);

	CreateObjectAbove(Kitchen, 3030, 615);

	CreateObjectAbove(InventorsLab, 3212, 599);

	var Shipyard0146 = CreateObjectAbove(Shipyard, 2763, 608);

	CreateObjectAbove(Loom, 3080, 616);

	var StoneDoor0150 = CreateObjectAbove(StoneDoor, 540, 1263);
	StoneDoor0150->SetComDir(COMD_Down);

	var SpinWheel0151 = CreateObjectAbove(SpinWheel, 571, 1263);
	SpinWheel0151->SetStoneDoor(StoneDoor0150);

	var WindGenerator0269 = CreateObjectAbove(WindGenerator, 3163, 599);
	WindGenerator0269->SetCategory(C4D_StaticBack);

	g_flagpole = CreateObjectAbove(Flagpole, 502, 369);
	g_flagpole->SetNeutral(true);
	g_flagpole.StaticSaveVar = "g_flagpole";

	g_windmill = CreateObjectAbove(Windmill, 665, 351);
	g_windmill->SetCategory(C4D_StaticBack);
	g_windmill.StaticSaveVar = "g_windmill";

	var Elevator0361 = CreateObjectAbove(Elevator, 76, 387);
	Elevator0361->SetDir(DIR_Right);
	Elevator0361->CreateShaft(530);
	Elevator0361->SetCasePosition(905);

	var Lorry0369 = CreateObjectAbove(Lorry, 25, 779);
	Lorry0369->SetR(11);
	Lorry0369->SetPosition(25, 771);
	var Lorry0371 = CreateObjectAbove(Lorry, 3188, 598);

	var Catapult0373 = CreateObjectAbove(Catapult, 2795, 608);
	Catapult0373->SetRDir(4);
	Catapult0373->SetObjectLayer(Catapult0373);

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

	CreateObjectAbove(Fish, 1923, 729);
	CreateObjectAbove(Fish, 1924, 746);

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

	CreateObjectAbove(Skull, 1566, 703);
	CreateObjectAbove(Skull, 3124, 1378);

	CreateObjectAbove(Rock, 685, 594);
	CreateObjectAbove(Rock, 793, 488);
	CreateObjectAbove(Rock, 1244, 515);
	CreateObjectAbove(Rock, 1480, 735);
	CreateObjectAbove(Rock, 794, 895);
	CreateObjectAbove(Rock, 311, 447);
	CreateObjectAbove(Rock, 1243, 792);
	CreateObjectAbove(Rock, 1231, 737);
	CreateObjectAbove(Rock, 563, 922);
	CreateObjectAbove(Rock, 1219, 1088);
	CreateObjectAbove(Rock, 428, 1319);
	CreateObjectAbove(Rock, 2100, 951);
	CreateObjectAbove(Rock, 1922, 1161);
	CreateObjectAbove(Rock, 2277, 968);
	CreateObjectAbove(Rock, 2405, 696);
	CreateObjectAbove(Rock, 2893, 989);
	CreateObjectAbove(Rock, 3131, 689);
	CreateObjectAbove(Rock, 3266, 1379);

	CreateObjectAbove(Ore, 2226, 943);
	Foundry0127->CreateContents(Ore);
	Foundry0127->CreateContents(Ore);
	Foundry0127->CreateContents(Ore);

	CreateObjectAbove(Loam, 1030, 449);
	CreateObjectAbove(Loam, 1122, 920);
	CreateObjectAbove(Loam, 1492, 804);
	CreateObjectAbove(Loam, 935, 1132);
	CreateObjectAbove(Loam, 456, 1004);
	CreateObjectAbove(Loam, 2315, 632);
	CreateObjectAbove(Loam, 2582, 790);
	CreateObjectAbove(Loam, 3056, 725);
	CreateObjectAbove(Loam, 3235, 792);
	CreateObjectAbove(Loam, 3167, 949);
	CreateObjectAbove(Loam, 2630, 1052);
	CreateObjectAbove(Loam, 3238, 1150);
	CreateObjectAbove(Loam, 2734, 1245);
	CreateObjectAbove(Loam, 3003, 1345);
	Foundry0124->CreateContents(Loam);
	Foundry0124->CreateContents(Loam);
	CreateObjectAbove(Loam, 951, 1333);
	CreateObjectAbove(Loam, 970, 1355);
	CreateObjectAbove(Loam, 952, 1364);
	CreateObjectAbove(Loam, 808, 1336);
	CreateObjectAbove(Loam, 737, 1322);
	CreateObjectAbove(Loam, 652, 1280);
	CreateObjectAbove(Loam, 797, 1391);
	CreateObjectAbove(Loam, 1021, 1339);
	CreateObjectAbove(Loam, 492, 1263);
	CreateObjectAbove(Loam, 504, 1263);
	CreateObjectAbove(Loam, 500, 1263);

	CreateObjectAbove(Metal, 2217, 942);
	ToolsWorkshop0139->CreateContents(Metal);
	ToolsWorkshop0139->CreateContents(Metal);
	ToolsWorkshop0139->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Shipyard0146->CreateContents(Metal);
	Lorry0371->CreateContents(Metal);
	Lorry0371->CreateContents(Metal);

	CreateObjectAbove(Moss, 1529, 681);

	ToolsWorkshop0139->CreateContents(Wood);
	ToolsWorkshop0139->CreateContents(Wood);
	ToolsWorkshop0139->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);
	Shipyard0146->CreateContents(Wood);

	var Crate0548 = CreateObjectAbove(Crate, 2836, 607);

	var Hammer0549 = Crate0548->CreateContents(Hammer);
	Hammer0549->SetR(50);
	ToolsWorkshop0139->CreateContents(Hammer);
	ToolsWorkshop0139->CreateContents(Hammer);

	Foundry0124->CreateContents(Bucket);
	Foundry0124->CreateContents(Bucket);
	Foundry0124->CreateContents(Bucket);
	CreateObjectAbove(Bucket, 435, 1271);

	Lorry0369->CreateContents(Axe);
	Lorry0369->CreateContents(Axe);
	Lorry0369->CreateContents(Axe);

	ToolsWorkshop0139->CreateContents(Pickaxe);
	ToolsWorkshop0139->CreateContents(Pickaxe);

	CreateObjectAbove(Crate, 2849, 607);
	CreateObjectAbove(Crate, 444, 1271);
	CreateObjectAbove(Crate, 473, 1263);
	CreateObjectAbove(Crate, 403, 1271);

	var Barrel0560 = Foundry0124->CreateContents(Barrel);
	Barrel0560->SetColor(0xff000000);
	Barrel0560->AddRestoreMode(Foundry0124);
	var Barrel0562 = Foundry0124->CreateContents(Barrel);
	Barrel0562->SetColor(0xff000000);
	Barrel0562->AddRestoreMode(Foundry0124);
	var Barrel0564 = CreateObjectAbove(Barrel, 484, 367);
	Barrel0564->SetR(23);
	Barrel0564->SetColor(0xff000000);
	Barrel0564->SetObjectLayer(Barrel0564);
	Barrel0564->SetPosition(484, 361);
	var Barrel0566 = CreateObjectAbove(Barrel, 648, 351);
	Barrel0566->SetR(-22);
	Barrel0566->SetColor(0xff000000);
	Barrel0566->SetObjectLayer(Barrel0566);
	Barrel0566->SetPosition(648, 345);
	var Barrel0568 = CreateObjectAbove(Barrel, 244, 321);
	Barrel0568->SetColor(0xff000000);
	Barrel0568->SetObjectLayer(Barrel0568);
	var Barrel0570 = CreateObjectAbove(Barrel, 396, 343);
	Barrel0570->SetColor(0xff000000);
	Barrel0570->SetObjectLayer(Barrel0570);

	CreateObjectAbove(Mushroom, 1192, 448);
	CreateObjectAbove(Mushroom, 1170, 440);
	CreateObjectAbove(Mushroom, 1492, 663);
	CreateObjectAbove(Mushroom, 1131, 436);
	CreateObjectAbove(Mushroom, 1523, 675);
	CreateObjectAbove(Mushroom, 1163, 440);
	CreateObjectAbove(Mushroom, 1070, 414);
	CreateObjectAbove(Mushroom, 1010, 399);
	CreateObjectAbove(Mushroom, 960, 400);
	CreateObjectAbove(Mushroom, 1175, 440);
	CreateObjectAbove(Mushroom, 1120, 432);
	CreateObjectAbove(Mushroom, 989, 400);
	CreateObjectAbove(Mushroom, 968, 400);
	CreateObjectAbove(Mushroom, 1013, 400);
	CreateObjectAbove(Mushroom, 1496, 664);
	CreateObjectAbove(Mushroom, 1088, 424);
	CreateObjectAbove(Mushroom, 1545, 696);
	CreateObjectAbove(Mushroom, 1223, 440);
	CreateObjectAbove(Mushroom, 943, 399);
	CreateObjectAbove(Mushroom, 1006, 400);

	var Seaweed0652 = CreateObjectAbove(Seaweed, 1952, 903);
	Seaweed0652->SetPhase(26);
	var Seaweed0655 = CreateObjectAbove(Seaweed, 2013, 911);
	Seaweed0655->SetPhase(1);
	var Seaweed0658 = CreateObjectAbove(Seaweed, 1903, 887);
	Seaweed0658->SetPhase(57);
	var Seaweed0661 = CreateObjectAbove(Seaweed, 1983, 911);
	Seaweed0661->SetPhase(35);
	var Seaweed0664 = CreateObjectAbove(Seaweed, 2207, 942);
	Seaweed0664->SetPhase(29);
	var Seaweed0667 = CreateObjectAbove(Seaweed, 2127, 895);
	Seaweed0667->SetPhase(44);
	var Seaweed0670 = CreateObjectAbove(Seaweed, 2227, 943);
	Seaweed0670->SetPhase(29);
	var Seaweed0673 = CreateObjectAbove(Seaweed, 2191, 927);
	Seaweed0673->SetPhase(29);
	var Seaweed0676 = CreateObjectAbove(Seaweed, 2232, 943);
	Seaweed0676->SetPhase(66);
	var Seaweed0679 = CreateObjectAbove(Seaweed, 2269, 927);
	Seaweed0679->SetPhase(66);
	var Seaweed0682 = CreateObjectAbove(Seaweed, 2249, 935);
	Seaweed0682->SetPhase(66);

	CreateObjectAbove(DynamiteBox, 2844, 607);
	CreateObjectAbove(DynamiteBox, 452, 1271);
	var DynamiteBox0687 = CreateObjectAbove(DynamiteBox, 430, 1271);
	DynamiteBox0687->SetR(10);
	DynamiteBox0687->SetPosition(430, 1269);

	var MetalBarrel0688 = CreateObjectAbove(MetalBarrel, 395, 1271);
	MetalBarrel0688->SetColor(0xff000000);
	var MetalBarrel0690 = CreateObjectAbove(MetalBarrel, 421, 1271);
	MetalBarrel0690->SetR(-104);
	MetalBarrel0690->SetColor(0xff000000);
	MetalBarrel0690->SetPosition(421, 1268);
	var MetalBarrel0692 = CreateObjectAbove(MetalBarrel, 411, 1271);
	MetalBarrel0692->SetColor(0xff000000);
	var MetalBarrel0694 = CreateObjectAbove(MetalBarrel, 385, 1271);
	MetalBarrel0694->SetColor(0xff000000);

	var PowderKeg0696 = CreateObjectAbove(PowderKeg, 378, 1271);
	PowderKeg0696->SetR(99);
	PowderKeg0696->SetPosition(378, 1268);

	var WindBag0698 = CreateObjectAbove(WindBag, 382, 1271);
	WindBag0698->SetR(-29);
	WindBag0698->SetPosition(382, 1268);

	CreateObjectAbove(Firestone, 1272, 962);
	CreateObjectAbove(Firestone, 1763, 901);
	CreateObjectAbove(Firestone, 1415, 709);
	CreateObjectAbove(Firestone, 772, 622);
	CreateObjectAbove(Firestone, 1196, 494);
	CreateObjectAbove(Firestone, 345, 693);
	Lorry0371->CreateContents(Firestone);
	Lorry0371->CreateContents(Firestone);
	CreateObjectAbove(Firestone, 2460, 1367);
	CreateObjectAbove(Firestone, 2893, 672);
	CreateObjectAbove(Firestone, 2998, 960);
	CreateObjectAbove(Firestone, 3266, 1173);
	CreateObjectAbove(Firestone, 2653, 1130);
	CreateObjectAbove(Firestone, 2410, 1166);
	CreateObjectAbove(Firestone, 2853, 1379);

	return true;
}
