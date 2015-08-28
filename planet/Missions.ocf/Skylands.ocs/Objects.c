/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Rule_TeamAccount, 50, 50);

	CreateObject(Rule_BuyAtFlagpole, 50, 50);

	CreateObjectAbove(Tree_Coniferous, 962, 839);
	CreateObjectAbove(Tree_Coniferous2, 774, 951);

	CreateObjectAbove(Tree_Coniferous, 46, 441);
	var Tree_Coniferous0015 = CreateObjectAbove(Tree_Coniferous2, 320, 312);
	Tree_Coniferous0015->SetR(-30);
	Tree_Coniferous0015->SetPosition(320, 293);
	var Tree_Coniferous0019 = CreateObjectAbove(Tree_Coniferous, 422, 887);
	Tree_Coniferous0019->SetClrModulation(0xffb0b080);
	var Tree_Coniferous0023 = CreateObjectAbove(Tree_Coniferous2, 373, 920);
	Tree_Coniferous0023->SetClrModulation(0xff808080);
	var Tree_Coniferous0027 = CreateObjectAbove(Tree_Coniferous, 72, 744);
	Tree_Coniferous0027->SetClrModulation(0xff808080);
	CreateObjectAbove(Tree_Coniferous, 974, 327);
	CreateObjectAbove(Tree_Coniferous2, 1003, 326);
	CreateObjectAbove(Tree_Coniferous2, 550, 107);
	CreateObjectAbove(Tree_Coniferous, 632, 128);
	var Tree_Coniferous0047 = CreateObjectAbove(Tree_Coniferous2, 129, 409);
	Tree_Coniferous0047->SetR(-5);
	Tree_Coniferous0047->SetClrModulation(0xffd49d68);
	Tree_Coniferous0047->SetPosition(129, 387);
	CreateObjectAbove(Tree_Coniferous, 78, 440);
	var Tree_Coniferous0055 = CreateObjectAbove(Tree_Coniferous, 182, 408);
	Tree_Coniferous0055->SetR(-2);
	Tree_Coniferous0055->SetClrModulation(0xffca999c);
	Tree_Coniferous0055->SetPosition(182, 386);
	var Tree_Coniferous0059 = CreateObjectAbove(Tree_Coniferous2, 200, 408);
	Tree_Coniferous0059->SetR(8);
	Tree_Coniferous0059->SetClrModulation(0xfff7e5ee);
	Tree_Coniferous0059->SetPosition(200, 386);
	CreateObjectAbove(Tree_Coniferous, 23, 452);
	var Tree_Coniferous0067 = CreateObjectAbove(Tree_Coniferous, 110, 410);
	Tree_Coniferous0067->SetR(1);
	Tree_Coniferous0067->SetClrModulation(0xffe9e2b0);
	Tree_Coniferous0067->SetPosition(110, 388);
	var Tree_Coniferous0071 = CreateObjectAbove(Tree_Coniferous2, 167, 408);
	Tree_Coniferous0071->SetR(-4);
	Tree_Coniferous0071->SetClrModulation(0xffedfa82);
	Tree_Coniferous0071->SetPosition(167, 386);
	var Tree_Coniferous0075 = CreateObjectAbove(Tree_Coniferous2, 93, 430);
	Tree_Coniferous0075->SetR(-10);
	Tree_Coniferous0075->SetClrModulation(0xffedf4c6);
	Tree_Coniferous0075->SetPosition(93, 408);
	var Tree_Coniferous0079 = CreateObjectAbove(Tree_Coniferous, 36, 446);
	Tree_Coniferous0079->SetR(-2);
	Tree_Coniferous0079->SetClrModulation(0xffca90cd);
	Tree_Coniferous0079->SetPosition(36, 424);
	var Tree_Coniferous0083 = CreateObjectAbove(Tree_Coniferous2, 151, 406);
	Tree_Coniferous0083->SetR(-3);
	Tree_Coniferous0083->SetClrModulation(0xffd8d7e0);
	Tree_Coniferous0083->SetPosition(151, 384);
	var Tree_Coniferous0087 = CreateObjectAbove(Tree_Coniferous, 159, 407);
	Tree_Coniferous0087->SetClrModulation(0xffd1e168);
	var Tree_Coniferous0091 = CreateObjectAbove(Tree_Coniferous2, 60, 440);
	Tree_Coniferous0091->SetR(-10);
	Tree_Coniferous0091->SetClrModulation(0xffd8c49d);
	Tree_Coniferous0091->SetPosition(60, 418);
	var Tree_Coniferous0095 = CreateObjectAbove(Tree_Coniferous, 355, 313);
	Tree_Coniferous0095->SetR(10);
	Tree_Coniferous0095->SetPosition(355, 291);
	CreateObjectAbove(Tree_Coniferous, 132, 726);
	var Tree_Coniferous0103 = CreateObjectAbove(Tree_Coniferous2, 487, 805);
	Tree_Coniferous0103->SetR(-30);
	Tree_Coniferous0103->SetPosition(487, 786);

	var Chest0107 = CreateObjectAbove(Chest, 164, 903);
	Chest0107->SetClrModulation(0xd0ffffff);
	var Chest0108 = CreateObjectAbove(Chest, 503, 808);
	Chest0108->SetClrModulation(0xd0ffffff);
	var Chest0109 = CreateObjectAbove(Chest, 611, 1047);
	Chest0109->SetClrModulation(0xd0ffffff);
	var Chest0110 = CreateObjectAbove(Chest, 258, 134);
	Chest0110->SetClrModulation(0xd0ffffff);
	var Chest0111 = CreateObjectAbove(Chest, 990, 327);
	Chest0111->SetClrModulation(0xd0ffffff);
	var Chest0112 = CreateObjectAbove(Chest, 947, 841);
	Chest0112->SetClrModulation(0xd0ffffff);

	CreateObjectAbove(Plane_Construction, 272, 415);

	var Plane_Wings0115 = CreateObjectAbove(Plane_Wings, 600, 129);
	Plane_Wings0115->SetR(117);
	Plane_Wings0115->SetPosition(600, 118);

	var Plane_Skids0116 = CreateObjectAbove(Plane_Skids, 457, 896);
	Plane_Skids0116->SetR(13);
	Plane_Skids0116->SetPosition(457, 887);

	var Plane_Chassis0117 = CreateObjectAbove(Plane_Chassis, 908, 973);
	Plane_Chassis0117->SetR(-18);
	Plane_Chassis0117->SetPosition(908, 960);

	Chest0110->CreateContents(Rock);
	Chest0110->CreateContents(Rock);
	Chest0110->CreateContents(Rock);
	Chest0110->CreateContents(Rock);
	Chest0108->CreateContents(Rock);
	Chest0108->CreateContents(Rock);
	Chest0108->CreateContents(Rock);
	Chest0108->CreateContents(Rock);
	Chest0112->CreateContents(Rock);
	Chest0112->CreateContents(Rock);

	Chest0107->CreateContents(Nugget);
	Chest0107->CreateContents(Nugget);
	Chest0107->CreateContents(Nugget);
	Chest0111->CreateContents(Nugget);
	Chest0111->CreateContents(Nugget);
	Chest0111->CreateContents(Nugget);

	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0111->CreateContents(Loam);
	Chest0112->CreateContents(Loam);
	Chest0108->CreateContents(Loam);
	Chest0112->CreateContents(Loam);
	Chest0108->CreateContents(Loam);
	Chest0112->CreateContents(Loam);
	Chest0108->CreateContents(Loam);
	Chest0112->CreateContents(Loam);
	Chest0108->CreateContents(Loam);
	Chest0107->CreateContents(Loam);
	Chest0107->CreateContents(Loam);
	Chest0109->CreateContents(Loam);
	Chest0109->CreateContents(Loam);
	Chest0109->CreateContents(Loam);
	Chest0110->CreateContents(Loam);
	Chest0110->CreateContents(Loam);
	Chest0110->CreateContents(Loam);
	CreateObjectAbove(Loam, 116, 755);
	CreateObjectAbove(Loam, 183, 480);
	CreateObjectAbove(Loam, 41, 486);
	CreateObjectAbove(Loam, 158, 432);
	CreateObjectAbove(Loam, 552, 187);
	CreateObjectAbove(Loam, 995, 349);
	CreateObjectAbove(Loam, 1052, 368);
	CreateObjectAbove(Loam, 177, 747);
	Chest0109->CreateContents(Loam);
	Chest0109->CreateContents(Loam);
	Chest0109->CreateContents(Loam);

	Chest0111->CreateContents(Wood);
	Chest0111->CreateContents(Wood);
	Chest0111->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	Chest0112->CreateContents(Wood);
	Chest0112->CreateContents(Wood);

	Chest0108->CreateContents(Metal);
	Chest0108->CreateContents(Metal);
	Chest0112->CreateContents(Metal);
	Chest0107->CreateContents(Metal);
	Chest0109->CreateContents(Metal);
	Chest0108->CreateContents(Metal);
	Chest0108->CreateContents(Metal);

	Chest0110->CreateContents(GoldBar);
	Chest0109->CreateContents(GoldBar);
	Chest0109->CreateContents(GoldBar);
	Chest0109->CreateContents(GoldBar);
	Chest0112->CreateContents(GoldBar);

	var Plane_Propeller0188 = CreateObjectAbove(Plane_Propeller, 1031, 334);
	Plane_Propeller0188->SetR(20);
	Plane_Propeller0188->SetPosition(1031, 329);

	Chest0108->CreateContents(Firestone);
	Chest0108->CreateContents(Firestone);
	Chest0108->CreateContents(Firestone);
	Chest0110->CreateContents(Firestone);
	Chest0110->CreateContents(Firestone);
	Chest0107->CreateContents(Firestone);
	Chest0107->CreateContents(Firestone);
	Chest0107->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	CreateObjectAbove(Firestone, 163, 486);
	CreateObjectAbove(Firestone, 143, 444);
	CreateObjectAbove(Firestone, 37, 467);
	CreateObjectAbove(Firestone, 178, 442);
	CreateObjectAbove(Firestone, 566, 233);
	CreateObjectAbove(Firestone, 539, 176);
	CreateObjectAbove(Firestone, 102, 760);
	CreateObjectAbove(Firestone, 426, 916);
	CreateObjectAbove(Firestone, 479, 932);
	CreateObjectAbove(Firestone, 848, 1000);
	CreateObjectAbove(Firestone, 908, 1012);
	CreateObjectAbove(Firestone, 1063, 354);
	CreateObjectAbove(Firestone, 170, 446);
	CreateObjectAbove(Firestone, 259, 478);
	CreateObjectAbove(Firestone, 310, 477);
	CreateObjectAbove(Firestone, 350, 414);
	CreateObjectAbove(Firestone, 469, 913);
	Chest0109->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	CreateObjectAbove(Firestone, 89, 766);
	CreateObjectAbove(Firestone, 135, 741);
	CreateObjectAbove(Firestone, 159, 748);
	CreateObjectAbove(Firestone, 199, 470);
	CreateObjectAbove(Firestone, 361, 351);
	return true;
}
