/* Automatically created objects file */

func InitializeObjects()
{
	CreateObjectAbove(Grass, 572, 486);
	CreateObjectAbove(Grass, 566, 484);
	CreateObjectAbove(Grass, 553, 484);
	CreateObjectAbove(Grass, 583, 485);
	CreateObjectAbove(Grass, 588, 486);
	CreateObjectAbove(Grass, 789, 469);
	CreateObjectAbove(Grass, 754, 437);
	CreateObject(Grass, 795, 474);
	CreateObjectAbove(Grass, 855, 454);
	CreateObjectAbove(Grass, 868, 454);
	CreateObject(Grass, 362, 405);
	CreateObjectAbove(Grass, 257, 436);
	CreateObjectAbove(Grass, 253, 441);

	var Torch001 = CreateObject(Torch, 366, 544);
	Torch001->AttachToWall(false);
	Torch001->SetR(-20);

	var Branch001 = CreateObject(Branch, 490, 459);
	Branch001->SetR(140);

	CreateObjectAbove(Fern, 663, 487);
	CreateObjectAbove(Fern, 522, 488);
	CreateObjectAbove(Fern, 484, 488);
	CreateObjectAbove(Fern, 670, 496);

	CreateObjectAbove(Flower, 666, 497);
	var Flower002 = CreateObject(Flower, 529, 484);
	Flower002->SetR(30);
	CreateObjectAbove(Flower, 594, 494);
	CreateObjectAbove(Flower, 561, 494);
	CreateObjectAbove(Flower, 794, 480);

	CreateObjectAbove(Tree_Coniferous2, 814, 542);

	var Tree_Coniferous3001 = CreateObject(Tree_Coniferous3, 719, 424);
	Tree_Coniferous3001->SetR(-10);

	var Trunk001 = CreateObject(Trunk, 482, 476);
	Trunk001->SetCon(60);
	Trunk001->SetR(160);

	var Branch002 = CreateObject(Branch, 496, 454);
	Branch002->SetR(110);

	var Trunk002 = CreateObject(Trunk, 423, 515);
	Trunk002->SetCon(60);
	Trunk002->SetR(70);

	CreateObjectAbove(Tree_Coniferous3, 807, 516);

	CreateObjectAbove(Flower, 862, 463);

	CreateObjectAbove(Fern, 911, 449);

	CreateObjectAbove(Tree_Coniferous3, 223, 516);

	CreateObject(Fern, 265, 422);
	CreateObjectAbove(Zaphive, 822, 502);

	Zap->Place(1, 6, Shape->Rectangle(770, 445, 50, 50));

	CreateObjectAbove(Chest, 757, 687);

	var Clonk001 = CreateObjectAbove(Clonk, 351, 549, 0);
	Clonk001->SetDir(DIR_Left);

	var Wipf001 = CreateObjectAbove(Wipf, 270, 606);
	Wipf001->SetDir(DIR_Right);

	CreateObjectAbove(Fish, 840, 572);
	CreateObjectAbove(Fish, 919, 564);
	CreateObjectAbove(Fish, 942, 586);
	CreateObjectAbove(Fish, 909, 588);
	CreateObjectAbove(Fish, 112, 595);
	CreateObjectAbove(Fish, 165, 551);
	CreateObjectAbove(Fish, 180, 571);
	var Mosquito001 = CreateObjectAbove(Mosquito, 866, 519);
	Mosquito001->SetComDir(COMD_None);
	var Mosquito002 = CreateObjectAbove(Mosquito, 867, 519);
	Mosquito002->SetComDir(COMD_None);
	var Mosquito003 = CreateObjectAbove(Mosquito, 878, 512);
	Mosquito003->SetComDir(COMD_None);
	var Mosquito004 = CreateObjectAbove(Mosquito, 875, 520);
	Mosquito004->SetComDir(COMD_None);
	var Mosquito005 = CreateObjectAbove(Mosquito, 867, 523);
	Mosquito005->SetComDir(COMD_None);
	var Mosquito006 = CreateObjectAbove(Mosquito, 870, 515);
	Mosquito006->SetComDir(COMD_None);
	var Mosquito007 = CreateObjectAbove(Mosquito, 859, 522);
	Mosquito007->SetComDir(COMD_None);
	var Mosquito008 = CreateObjectAbove(Mosquito, 858, 522);
	Mosquito008->SetComDir(COMD_None);
	var Mosquito009 = CreateObjectAbove(Mosquito, 864, 525);
	Mosquito009->SetComDir(COMD_None);
	var Mosquito010 = CreateObjectAbove(Mosquito, 855, 514);
	Mosquito010->SetComDir(COMD_None);
	var Mosquito011 = CreateObjectAbove(Mosquito, 861, 507);
	Mosquito011->SetComDir(COMD_None);
	var Mosquito012 = CreateObjectAbove(Mosquito, 865, 514);
	Mosquito012->SetComDir(COMD_None);
	var Mosquito013 = CreateObjectAbove(Mosquito, 841, 525);
	Mosquito013->SetComDir(COMD_None);
	var Mosquito014 = CreateObjectAbove(Mosquito, 842, 525);
	Mosquito014->SetComDir(COMD_None);
	var Mosquito015 = CreateObjectAbove(Mosquito, 853, 518);
	Mosquito015->SetComDir(COMD_None);
	var Mosquito016 = CreateObjectAbove(Mosquito, 850, 526);
	Mosquito016->SetComDir(COMD_None);
	var Mosquito017 = CreateObjectAbove(Mosquito, 872, 532);
	Mosquito017->SetComDir(COMD_None);
	var Mosquito018 = CreateObjectAbove(Mosquito, 861, 515);
	Mosquito018->SetComDir(COMD_None);
	var Mosquito019 = CreateObjectAbove(Mosquito, 850, 522);
	Mosquito019->SetComDir(COMD_None);
	var Mosquito020 = CreateObjectAbove(Mosquito, 860, 528);
	Mosquito020->SetComDir(COMD_None);

	CreateObjectAbove(Piranha, 878, 578);
	CreateObjectAbove(Piranha, 885, 552);

	var Butterfly001 = CreateObjectAbove(Butterfly, 856, 450);
	Butterfly001->SetComDir(COMD_None);
	Butterfly001->SetCommand("Call", Butterfly001, nil, 0, nil, "MissionComplete");
	Butterfly001->SetAction("Fly");
	var Butterfly002 = CreateObjectAbove(Butterfly, 909, 442);
	Butterfly002->SetComDir(COMD_None);
	Butterfly002->SetCommand("Call", Butterfly002, nil, 0, nil, "MissionComplete");
	Butterfly002->SetAction("Fly");
	var Butterfly003 = CreateObjectAbove(Butterfly, 586, 482);
	Butterfly003->SetComDir(COMD_None);
	Butterfly003->SetCommand("Call", Butterfly003, nil, 0, nil, "MissionComplete");
	Butterfly003->SetAction("Fly");

	CreateObjectAbove(Mushroom, 414, 471);
	CreateObjectAbove(Mushroom, 409, 471);
	CreateObjectAbove(Mushroom, 471, 487);
	CreateObjectAbove(Mushroom, 474, 486);
	CreateObjectAbove(Mushroom, 347, 479);

	var Seaweed001 = CreateObjectAbove(Seaweed, 689, 528);
	Seaweed001->SetYDir(16);
	var Seaweed002 = CreateObjectAbove(Seaweed, 680, 537);
	Seaweed002->SetYDir(16);
	var Seaweed003 = CreateObjectAbove(Seaweed, 719, 536);
	Seaweed003->SetYDir(16);
	var Seaweed004 = CreateObjectAbove(Seaweed, 687, 529);
	Seaweed004->SetYDir(16);
	var Seaweed005 = CreateObjectAbove(Seaweed, 778, 531);
	Seaweed005->SetYDir(16);
	var Seaweed006 = CreateObjectAbove(Seaweed, 782, 529);
	Seaweed006->SetYDir(16);
	var Seaweed007 = CreateObjectAbove(Seaweed, 836, 606);
	Seaweed007->SetYDir(16);
	var Seaweed008 = CreateObjectAbove(Seaweed, 819, 581);
	Seaweed008->SetYDir(16);
	return true;
}
