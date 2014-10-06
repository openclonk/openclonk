/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Grass, 792, 656);
	CreateObject(Grass, 647, 614);
	CreateObject(Grass, 656, 618);
	CreateObject(Grass, 176, 533);
	CreateObject(Grass, 86, 502);
	CreateObject(Grass, 164, 529);
	CreateObject(Grass, 248, 571);
	CreateObject(Grass, 254, 575);
	CreateObject(Grass, 207, 548);
	CreateObject(Grass, 241, 568);
	CreateObject(Grass, 224, 558);
	CreateObject(Grass, 169, 533);
	CreateObject(Grass, 199, 541);
	CreateObject(Grass, 215, 550);
	CreateObject(Grass, 222, 554);
	CreateObject(Grass, 10, 498);
	CreateObject(Grass, 3, 501);

	CreateObject(Trunk, 1117, 546);
	var Trunk0019 = CreateObject(Trunk, 1100, 555);
	Trunk0019->SetCon(80);
	var Trunk0020 = CreateObject(Trunk, 1363, 543);
	Trunk0020->SetCon(65);
	var Trunk0021 = CreateObject(Trunk, 1236, 561);
	Trunk0021->SetCon(40);
	CreateObject(Trunk, 983, 593);
	CreateObject(Trunk, 1050, 574);

	var Rank0024 = CreateObject(Rank, 1105, 503);
	Rank0024->SetR(-140);
	Rank0024->SetPosition(1105, 503);
	var Rank0025 = CreateObject(Rank, 1415, 280);
	Rank0025->SetR(176);
	Rank0025->SetPosition(1415, 280);
	var Rank0026 = CreateObject(Rank, 1509, 289);
	Rank0026->SetR(-165);
	Rank0026->SetPosition(1509, 289);
	var Rank0027 = CreateObject(Rank, 1557, 314);
	Rank0027->SetR(-159);
	Rank0027->SetPosition(1557, 314);
	var Rank0028 = CreateObject(Rank, 1315, 293);
	Rank0028->SetR(-173);
	Rank0028->SetPosition(1315, 293);
	var Rank0029 = CreateObject(Rank, 1583, 452);
	Rank0029->SetR(-8);
	Rank0029->SetPosition(1583, 449);
	var Rank0030 = CreateObject(Rank, 1491, 479);
	Rank0030->SetR(-26);
	Rank0030->SetPosition(1491, 476);

	var Trunk0031 = CreateObject(Trunk, 1388, 302);
	Trunk0031->SetCon(55);
	Trunk0031->SetR(180);
	Trunk0031->SetPosition(1388, 290);
	var Trunk0032 = CreateObject(Trunk, 1316, 218);
	Trunk0032->SetCon(66);

	CreateObject(Tree_Coniferous, 531, 574);

	CreateObject(Fern, 596, 595);

	CreateObject(Tree_Coniferous, 633, 607);
	CreateObject(Tree_Coniferous, 716, 632);
	CreateObject(Tree_Coniferous, 764, 631);
	CreateObject(Tree_Coniferous, 581, 590);
	CreateObject(Tree_Coniferous, 413, 702);
	CreateObject(Tree_Coniferous, 789, 628);
	CreateObject(Tree_Coniferous, 934, 615);
	CreateObject(Tree_Coniferous, 1134, 533);
	CreateObject(Tree_Coniferous, 122, 507);
	CreateObject(Tree_Coniferous, 137, 518);
	CreateObject(Tree_Coniferous, 131, 515);
	CreateObject(Tree_Coniferous, 152, 525);
	CreateObject(Tree_Coniferous, 189, 543);
	CreateObject(Tree_Coniferous, 231, 565);
	CreateObject(Tree_Coniferous, 1290, 227);

	CreateObject(Fern, 241, 1104);
	CreateObject(Fern, 493, 1160);
	CreateObject(Fern, 1008, 736);
	CreateObject(Fern, 1530, 464);
	CreateObject(Fern, 820, 1248);
	CreateObject(Fern, 1125, 1207);

	CreateObject(Tree_Coniferous, 264, 1111);
	CreateObject(Tree_Coniferous, 691, 626);

	CreateObject(Fern, 1100, 1353);
	CreateObject(Fern, 1074, 1354);

	var Trunk0141 = CreateObject(Trunk, 211, 1090);
	Trunk0141->SetCon(80);
	var Trunk0142 = CreateObject(Trunk, 438, 1155);
	Trunk0142->SetCon(70);
	var Trunk0143 = CreateObject(Trunk, 538, 1103);
	Trunk0143->SetR(-165);
	Trunk0143->SetPosition(538, 1080);

	var Tree_Coniferous0144 = CreateObject(Tree_Coniferous, 624, 612);
	Tree_Coniferous0144->SetCon(24);

	var Rank0148 = CreateObject(Rank, 1634, 363);
	Rank0148->SetR(-158);
	Rank0148->SetPosition(1634, 363);

	var Trunk0149 = CreateObject(Trunk, 1457, 528);
	Trunk0149->SetCon(30);

	CreateObject(Fern, 75, 1102);

	CreateObject(Rule_NoPowerNeed, 0, 0);

	var Chest0156 = CreateObject(Chest, 55, 1308);
	CreateObject(Chest, 1381, 534);
	var Chest0158 = CreateObject(Chest, 620, 879);
	var Chest0159 = CreateObject(Chest, 625, 748);
	var Chest0160 = CreateObject(Chest, 1553, 279);
	CreateObject(Chest, 1647, 1380);

	CreateObject(Armory, 1326, 523);

	var Flagpole0204 = CreateObject(Flagpole, 120, 71);
	Flagpole0204->SetCategory(C4D_StaticBack);
	Flagpole0204->SetColor(0xffc85a12);

	var WindGenerator0170 = CreateObject(WindGenerator, 1262, 537);
	WindGenerator0170->SetCon(48);
	WindGenerator0170->SetCategory(C4D_StaticBack);
	WindGenerator0170->SetClrModulation(0xff7a6e65);

	var WoodenCabin0155 = CreateObject(WoodenCabin, 56, 505);
	WoodenCabin0155->SetCategory(C4D_Structure|C4D_Background);

	var Elevator0162 = CreateObject(Elevator, 1170, 544);
	Elevator0162->SetCon(40);
	Elevator0162->SetClrModulation(0xff58362c);

	var Clonk0289 = CreateObject(Clonk, 155, 997);
	Clonk0289->Kill(Clonk0289, true);
	Clonk0289->SetYDir(16);
	Clonk0289->SetColor(0xffdd3420);
	Clonk0289->SetName("Hans-Georg");
	Clonk0289->SetDir(DIR_Left);

	CreateObject(Rock, 109, 659);
	CreateObject(Rock, 86, 860);
	CreateObject(Rock, 31, 883);
	CreateObject(Rock, 172, 601);
	CreateObject(Rock, 128, 1045);
	CreateObject(Rock, 37, 1031);
	CreateObject(Rock, 240, 1204);
	CreateObject(Rock, 453, 1176);
	CreateObject(Rock, 514, 1233);
	CreateObject(Rock, 493, 1292);
	CreateObject(Rock, 669, 1190);
	CreateObject(Rock, 794, 1262);
	CreateObject(Rock, 983, 1371);
	CreateObject(Rock, 1135, 1283);
	CreateObject(Rock, 1193, 1261);
	CreateObject(Rock, 1036, 1128);
	CreateObject(Rock, 876, 1126);
	CreateObject(Rock, 940, 976);
	CreateObject(Rock, 668, 926);
	CreateObject(Rock, 1152, 1064);
	CreateObject(Rock, 1044, 809);
	CreateObject(Rock, 1199, 740);
	CreateObject(Rock, 1305, 605);
	CreateObject(Rock, 1315, 837);
	CreateObject(Rock, 1448, 637);
	CreateObject(Rock, 1579, 854);
	CreateObject(Rock, 1455, 244);
	CreateObject(Rock, 667, 711);
	CreateObject(Rock, 475, 634);
	Chest0159->CreateContents(Rock);
	CreateObject(Rock, 948, 721);
	Chest0156->CreateContents(Rock);
	CreateObject(Rock, 380, 332);
	CreateObject(Rock, 302, 300);
	CreateObject(Rock, 1519, 1011);

	CreateObject(Coal, 160, 666);
	CreateObject(Coal, 133, 1112);
	CreateObject(Coal, 741, 708);
	CreateObject(Coal, 1081, 642);
	var Coal0464 = CreateObject(Coal, 1482, 1211);
	Coal0464->SetCon(98);
	CreateObject(Coal, 609, 1179);

	CreateObject(Sulphur, 663, 947);
	CreateObject(Sulphur, 897, 958);
	CreateObject(Sulphur, 973, 784);
	CreateObject(Sulphur, 993, 673);
	CreateObject(Sulphur, 1324, 692);
	CreateObject(Sulphur, 1554, 749);
	CreateObject(Sulphur, 1393, 640);

	Chest0158->CreateContents(Loam);
	CreateObject(Loam, 28, 587);
	Chest0160->CreateContents(Loam);
	CreateObject(Loam, 154, 183);

	CreateObject(Nugget, 679, 894);
	CreateObject(Nugget, 660, 887);
	CreateObject(Nugget, 637, 878);
	CreateObject(Nugget, 683, 845);
	CreateObject(Nugget, 91, 584);
	CreateObject(Nugget, 718, 856);
	CreateObject(Nugget, 802, 1101);
	CreateObject(Nugget, 677, 1225);
	Chest0158->CreateContents(Nugget);
	Chest0158->CreateContents(Nugget);
	CreateObject(Nugget, 1100, 1229);
	CreateObject(Nugget, 1114, 974);
	CreateObject(Nugget, 1484, 586);
	CreateObject(Nugget, 358, 407);

	Chest0156->CreateContents(Metal);

	CreateObject(Wood, 643, 656);
	var Wood0494 = CreateObject(Wood, 672, 760);
	Wood0494->SetR(22);
	Wood0494->SetRDir(-3);
	Wood0494->SetPosition(672, 758);

	var Axe0315 = CreateObject(Axe, 1537, 1292);
	Axe0315->SetR(99);
	Axe0315->SetPosition(1537, 1290);

	var Shovel0316 = CreateObject(Shovel, 138, 982);
	Shovel0316->SetR(123);
	Shovel0316->SetPosition(138, 978);

	Chest0159->CreateContents(Ropeladder);

	CreateObject(Mushroom, 1503, 472);
	CreateObject(Mushroom, 979, 590);
	CreateObject(Mushroom, 1225, 544);
	CreateObject(Mushroom, 1082, 551);
	CreateObject(Mushroom, 1438, 529);
	CreateObject(Mushroom, 601, 880);
	CreateObject(Mushroom, 199, 1071);
	CreateObject(Mushroom, 180, 1047);
	CreateObject(Mushroom, 116, 941);
	CreateObject(Mushroom, 1113, 726);
	CreateObject(Mushroom, 1076, 726);
	CreateObject(Mushroom, 1532, 825);
	CreateObject(Mushroom, 1552, 1309);
	CreateObject(Mushroom, 1577, 1317);
	CreateObject(Mushroom, 674, 897);
	CreateObject(Mushroom, 662, 1161);
	CreateObject(Mushroom, 799, 1240);
	CreateObject(Mushroom, 1616, 463);
	CreateObject(Mushroom, 1659, 507);
	Chest0159->CreateContents(Mushroom);
	CreateObject(Mushroom, 659, 757);
	CreateObject(Mushroom, 1615, 1389);

	Chest0158->CreateContents(Dynamite);
	Chest0158->CreateContents(Dynamite);
	Chest0160->CreateContents(Dynamite);
	Chest0160->CreateContents(Dynamite);

	var Seaweed0473 = CreateObject(Seaweed, 189, 1167);
	Seaweed0473->SetYDir(16);
	Seaweed0473->SetPhase(11);
	var Seaweed0476 = CreateObject(Seaweed, 142, 1169);
	Seaweed0476->SetYDir(16);
	Seaweed0476->SetPhase(11);
	var Seaweed0479 = CreateObject(Seaweed, 522, 658);
	Seaweed0479->SetYDir(16);
	Seaweed0479->SetPhase(11);
	var Seaweed0482 = CreateObject(Seaweed, 610, 705);
	Seaweed0482->SetYDir(16);
	Seaweed0482->SetPhase(11);
	var Seaweed0485 = CreateObject(Seaweed, 697, 682);
	Seaweed0485->SetYDir(16);
	Seaweed0485->SetPhase(11);
	var Seaweed0488 = CreateObject(Seaweed, 766, 697);
	Seaweed0488->SetYDir(16);
	Seaweed0488->SetPhase(11);

	CreateObject(GrappleBow, 1601, 326);

	Chest0160->CreateContents(Bow);

	var Arrow0496 = Chest0160->CreateContents(Arrow);
	Arrow0496->SetR(90);

	CreateObject(Firestone, 84, 1018);
	CreateObject(Firestone, 95, 909);
	CreateObject(Firestone, 695, 1002);
	CreateObject(Firestone, 864, 1279);
	CreateObject(Firestone, 901, 710);
	CreateObject(Firestone, 1298, 951);
	CreateObject(Firestone, 1448, 742);
	CreateObject(Firestone, 1644, 318);
	Chest0159->CreateContents(Firestone);
	Chest0159->CreateContents(Firestone);
	CreateObject(Firestone, 193, 1172);
	Chest0156->CreateContents(Firestone);
	CreateObject(Firestone, 573, 616);
	CreateObject(Firestone, 1154, 674);
	CreateObject(Firestone, 534, 698);
	CreateObject(Firestone, 1520, 953);

	return true;
}
