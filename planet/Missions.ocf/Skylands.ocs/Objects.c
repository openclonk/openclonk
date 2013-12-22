/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Rule_TeamAccount, 50, 50);

	CreateObject(Rule_BuyAtFlagpole, 50, 50);

	CreateObject(Tree_Coniferous, 962, 839);
	CreateObject(Tree_Coniferous, 774, 951);

	CreateObject(Tree_SmallConiferous, 46, 441);
	var Tree_SmallConiferous0015 = CreateObject(Tree_SmallConiferous, 320, 312);
	Tree_SmallConiferous0015->SetR(-30);
	Tree_SmallConiferous0015->SetPosition(320, 293);
	var Tree_SmallConiferous0019 = CreateObject(Tree_SmallConiferous, 422, 887);
	Tree_SmallConiferous0019->SetClrModulation(0xffb0b080);
	var Tree_SmallConiferous0023 = CreateObject(Tree_SmallConiferous, 373, 920);
	Tree_SmallConiferous0023->SetClrModulation(0xff808080);
	var Tree_SmallConiferous0027 = CreateObject(Tree_SmallConiferous, 72, 744);
	Tree_SmallConiferous0027->SetClrModulation(0xff808080);
	CreateObject(Tree_SmallConiferous, 974, 327);
	CreateObject(Tree_SmallConiferous, 1003, 326);
	CreateObject(Tree_SmallConiferous, 550, 107);
	CreateObject(Tree_SmallConiferous, 632, 128);
	var Tree_SmallConiferous0047 = CreateObject(Tree_SmallConiferous, 129, 409);
	Tree_SmallConiferous0047->SetR(-5);
	Tree_SmallConiferous0047->SetClrModulation(0xffd49d68);
	Tree_SmallConiferous0047->SetPosition(129, 387);
	CreateObject(Tree_SmallConiferous, 78, 440);
	var Tree_SmallConiferous0055 = CreateObject(Tree_SmallConiferous, 182, 408);
	Tree_SmallConiferous0055->SetR(-2);
	Tree_SmallConiferous0055->SetClrModulation(0xffca999c);
	Tree_SmallConiferous0055->SetPosition(182, 386);
	var Tree_SmallConiferous0059 = CreateObject(Tree_SmallConiferous, 200, 408);
	Tree_SmallConiferous0059->SetR(8);
	Tree_SmallConiferous0059->SetClrModulation(0xfff7e5ee);
	Tree_SmallConiferous0059->SetPosition(200, 386);
	CreateObject(Tree_SmallConiferous, 23, 452);
	var Tree_SmallConiferous0067 = CreateObject(Tree_SmallConiferous, 110, 410);
	Tree_SmallConiferous0067->SetR(1);
	Tree_SmallConiferous0067->SetClrModulation(0xffe9e2b0);
	Tree_SmallConiferous0067->SetPosition(110, 388);
	var Tree_SmallConiferous0071 = CreateObject(Tree_SmallConiferous, 167, 408);
	Tree_SmallConiferous0071->SetR(-4);
	Tree_SmallConiferous0071->SetClrModulation(0xffedfa82);
	Tree_SmallConiferous0071->SetPosition(167, 386);
	var Tree_SmallConiferous0075 = CreateObject(Tree_SmallConiferous, 93, 430);
	Tree_SmallConiferous0075->SetR(-10);
	Tree_SmallConiferous0075->SetClrModulation(0xffedf4c6);
	Tree_SmallConiferous0075->SetPosition(93, 408);
	var Tree_SmallConiferous0079 = CreateObject(Tree_SmallConiferous, 36, 446);
	Tree_SmallConiferous0079->SetR(-2);
	Tree_SmallConiferous0079->SetClrModulation(0xffca90cd);
	Tree_SmallConiferous0079->SetPosition(36, 424);
	var Tree_SmallConiferous0083 = CreateObject(Tree_SmallConiferous, 151, 406);
	Tree_SmallConiferous0083->SetR(-3);
	Tree_SmallConiferous0083->SetClrModulation(0xffd8d7e0);
	Tree_SmallConiferous0083->SetPosition(151, 384);
	var Tree_SmallConiferous0087 = CreateObject(Tree_SmallConiferous, 159, 407);
	Tree_SmallConiferous0087->SetClrModulation(0xffd1e168);
	var Tree_SmallConiferous0091 = CreateObject(Tree_SmallConiferous, 60, 440);
	Tree_SmallConiferous0091->SetR(-10);
	Tree_SmallConiferous0091->SetClrModulation(0xffd8c49d);
	Tree_SmallConiferous0091->SetPosition(60, 418);
	var Tree_SmallConiferous0095 = CreateObject(Tree_SmallConiferous, 355, 313);
	Tree_SmallConiferous0095->SetR(10);
	Tree_SmallConiferous0095->SetPosition(355, 291);
	CreateObject(Tree_SmallConiferous, 132, 726);
	var Tree_SmallConiferous0103 = CreateObject(Tree_SmallConiferous, 487, 805);
	Tree_SmallConiferous0103->SetR(-30);
	Tree_SmallConiferous0103->SetPosition(487, 786);

	var Chest0107 = CreateObject(Chest, 164, 903);
	Chest0107->SetClrModulation(0xd0ffffff);
	var Chest0108 = CreateObject(Chest, 503, 808);
	Chest0108->SetClrModulation(0xd0ffffff);
	var Chest0109 = CreateObject(Chest, 611, 1047);
	Chest0109->SetClrModulation(0xd0ffffff);
	var Chest0110 = CreateObject(Chest, 258, 134);
	Chest0110->SetClrModulation(0xd0ffffff);
	var Chest0111 = CreateObject(Chest, 990, 327);
	Chest0111->SetClrModulation(0xd0ffffff);
	var Chest0112 = CreateObject(Chest, 947, 841);
	Chest0112->SetClrModulation(0xd0ffffff);

	CreateObject(Plane_Construction, 272, 415);

	var Plane_Wings0115 = CreateObject(Plane_Wings, 600, 129);
	Plane_Wings0115->SetR(117);
	Plane_Wings0115->SetPosition(600, 118);

	var Plane_Skids0116 = CreateObject(Plane_Skids, 457, 896);
	Plane_Skids0116->SetR(13);
	Plane_Skids0116->SetPosition(457, 887);

	var Plane_Chassis0117 = CreateObject(Plane_Chassis, 908, 973);
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
	CreateObject(Loam, 116, 755);
	CreateObject(Loam, 183, 480);
	CreateObject(Loam, 41, 486);
	CreateObject(Loam, 158, 432);
	CreateObject(Loam, 552, 187);
	CreateObject(Loam, 995, 349);
	CreateObject(Loam, 1052, 368);
	CreateObject(Loam, 177, 747);
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

	var Plane_Propeller0188 = CreateObject(Plane_Propeller, 1031, 334);
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
	CreateObject(Firestone, 163, 486);
	CreateObject(Firestone, 143, 444);
	CreateObject(Firestone, 37, 467);
	CreateObject(Firestone, 178, 442);
	CreateObject(Firestone, 566, 233);
	CreateObject(Firestone, 539, 176);
	CreateObject(Firestone, 102, 760);
	CreateObject(Firestone, 426, 916);
	CreateObject(Firestone, 479, 932);
	CreateObject(Firestone, 848, 1000);
	CreateObject(Firestone, 908, 1012);
	CreateObject(Firestone, 1063, 354);
	CreateObject(Firestone, 170, 446);
	CreateObject(Firestone, 259, 478);
	CreateObject(Firestone, 310, 477);
	CreateObject(Firestone, 350, 414);
	CreateObject(Firestone, 469, 913);
	Chest0109->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	Chest0109->CreateContents(Firestone);
	CreateObject(Firestone, 89, 766);
	CreateObject(Firestone, 135, 741);
	CreateObject(Firestone, 159, 748);
	CreateObject(Firestone, 199, 470);
	CreateObject(Firestone, 361, 351);
	return true;
}
