/* Automatically created objects file */

func InitializeObjects()
{
	var CableCrossing001 = CreateObjectAbove(CableCrossing, 347, 389);
	CableCrossing001->SetCategory(C4D_StaticBack|C4D_Structure);
	var CableCrossing002 = CreateObjectAbove(CableCrossing, 554, 391);
	CableCrossing002->SetCategory(C4D_StaticBack|C4D_Structure);
	CableCrossing002->SetMeshMaterial("CableCarStation_SignStation", 1);
	CableCrossing002->ToggleStation(true);

	var CableLine001 = CreateObject(CableLine, 551, 749);
	CableLine001.LineColors = [-15461326,-15461326];
	CableLine001->SetPosition(551, 749);
	CableLine001->SetAction("Connect", CableCrossing001, CableCrossing002);
	CableLine001->SetConnectedObjects(CableCrossing001, CableCrossing002);

	var CableCrossing003 = CreateObjectAbove(CableCrossing, 209, 390);
	CableCrossing003->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine002 = CreateObject(CableLine, 206, 747);
	CableLine002.LineColors = [-15461326,-15461326];
	CableLine002->SetPosition(206, 747);
	CableLine002->SetAction("Connect", CableCrossing001, CableCrossing003);
	CableLine002->SetConnectedObjects(CableCrossing001, CableCrossing003);

	var CableCrossing004 = CreateObjectAbove(CableCrossing, 652, 491);
	CableCrossing004->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine003 = CreateObjectAbove(CableLine, 551, 1789);
	CableLine003.LineColors = [-15461326,-15461326];
	CableLine003->SetAction("Connect", CableCrossing004, CableCrossing002);
	CableLine003->SetConnectedObjects(CableCrossing004, CableCrossing002);

	var CableCrossing005 = CreateObjectAbove(CableCrossing, 694, 419);
	CableCrossing005->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine004 = CreateObjectAbove(CableLine, 651, 1889);
	CableLine004.LineColors = [-15461326,-15461326];
	CableLine004->SetAction("Connect", CableCrossing005, CableCrossing004);
	CableLine004->SetConnectedObjects(CableCrossing005, CableCrossing004);

	var CableCrossing006 = CreateObjectAbove(CableCrossing, 770, 360);
	CableCrossing006->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine005 = CreateObjectAbove(CableLine, 693, 1602);
	CableLine005.LineColors = [-15461326,-15461326];
	CableLine005->SetAction("Connect", CableCrossing006, CableCrossing005);
	CableLine005->SetConnectedObjects(CableCrossing006, CableCrossing005);

	var CableCrossing007 = CreateObjectAbove(CableCrossing, 810, 351);
	CableCrossing007->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine006 = CreateObjectAbove(CableLine, 771, 1365);
	CableLine006.LineColors = [-15461326,-15461326];
	CableLine006->SetAction("Connect", CableCrossing007, CableCrossing006);
	CableLine006->SetConnectedObjects(CableCrossing007, CableCrossing006);

	var CableCrossing008 = CreateObjectAbove(CableCrossing, 939, 360);
	CableCrossing008->SetCategory(C4D_StaticBack|C4D_Structure);
	CableCrossing008->SetMeshMaterial("CableCarStation_SignStation", 1);

	var CableLine007 = CreateObject(CableLine, 815, 679);
	CableLine007.LineColors = [-15461326,-15461326];
	CableLine007->SetPosition(815, 679);
	CableLine007->SetAction("Connect", CableCrossing008, CableCrossing007);
	CableLine007->SetConnectedObjects(CableCrossing008, CableCrossing007);

	var CableCrossing009 = CreateObjectAbove(CableCrossing, 599, 257);
	CableCrossing009->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine008 = CreateObject(CableLine, 808, 669);
	CableLine008.LineColors = [-15461326,-15461326];
	CableLine008->SetPosition(808, 669);
	CableLine008->SetAction("Connect", CableCrossing009, CableCrossing007);
	CableLine008->SetConnectedObjects(CableCrossing009, CableCrossing007);

	var CableCrossing010 = CreateObjectAbove(CableCrossing, 519, 259);
	CableCrossing010->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine009 = CreateObject(CableLine, 599, 722);
	CableLine009.LineColors = [-15461326,-15461326];
	CableLine009->SetPosition(599, 722);
	CableLine009->SetAction("Connect", CableCrossing010, CableCrossing009);
	CableLine009->SetConnectedObjects(CableCrossing010, CableCrossing009);

	var CableCrossing011 = CreateObjectAbove(CableCrossing, 465, 289);
	CableCrossing011->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine010 = CreateObject(CableLine, 517, 516);
	CableLine010.LineColors = [-15461326,-15461326];
	CableLine010->SetPosition(517, 516);
	CableLine010->SetAction("Connect", CableCrossing011, CableCrossing010);
	CableLine010->SetConnectedObjects(CableCrossing011, CableCrossing010);
	var CableLine011 = CreateObject(CableLine, 465, 645);
	CableLine011.LineColors = [-15461326,-15461326];
	CableLine011->SetPosition(465, 645);
	CableLine011->SetAction("Connect", CableCrossing003, CableCrossing011);
	CableLine011->SetConnectedObjects(CableCrossing003, CableCrossing011);

	var CableCrossing012 = CreateObjectAbove(CableCrossing, 744, 589);
	CableCrossing012->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine012 = CreateObjectAbove(CableLine, 652, 2191);
	CableLine012.LineColors = [-15461326,-15461326];
	CableLine012->SetAction("Connect", CableCrossing012, CableCrossing004);
	CableLine012->SetConnectedObjects(CableCrossing012, CableCrossing004);

	var CableCrossing013 = CreateObjectAbove(CableCrossing, 680, 619);
	CableCrossing013->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine013 = CreateObjectAbove(CableLine, 743, 2370);
	CableLine013.LineColors = [-15461326,-15461326];
	CableLine013->SetAction("Connect", CableCrossing013, CableCrossing012);
	CableLine013->SetConnectedObjects(CableCrossing013, CableCrossing012);

	var CableCrossing014 = CreateObjectAbove(CableCrossing, 568, 690);
	CableCrossing014->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine014 = CreateObjectAbove(CableLine, 678, 2616);
	CableLine014.LineColors = [-15461326,-15461326];
	CableLine014->SetAction("Connect", CableCrossing014, CableCrossing013);
	CableLine014->SetConnectedObjects(CableCrossing014, CableCrossing013);

	var CableCrossing015 = CreateObjectAbove(CableCrossing, 456, 711);
	CableCrossing015->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine015 = CreateObjectAbove(CableLine, 570, 2749);
	CableLine015.LineColors = [-15461326,-15461326];
	CableLine015->SetAction("Connect", CableCrossing015, CableCrossing014);
	CableLine015->SetConnectedObjects(CableCrossing015, CableCrossing014);

	var CableCrossing016 = CreateObjectAbove(CableCrossing, 372, 650);
	CableCrossing016->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine016 = CreateObjectAbove(CableLine, 454, 2770);
	CableLine016.LineColors = [-15461326,-15461326];
	CableLine016->SetAction("Connect", CableCrossing016, CableCrossing015);
	CableLine016->SetConnectedObjects(CableCrossing016, CableCrossing015);

	var CableCrossing017 = CreateObjectAbove(CableCrossing, 231, 590);
	CableCrossing017->SetCategory(C4D_StaticBack|C4D_Structure);

	var CableLine017 = CreateObjectAbove(CableLine, 374, 2524);
	CableLine017.LineColors = [-15461326,-15461326];
	CableLine017->SetAction("Connect", CableCrossing017, CableCrossing016);
	CableLine017->SetConnectedObjects(CableCrossing017, CableCrossing016);

	var CableCrossing018 = CreateObjectAbove(CableCrossing, 131, 528);
	CableCrossing018->SetCategory(C4D_StaticBack|C4D_Structure);
	CableCrossing018->SetMeshMaterial("CableCarStation_SignStation", 1);

	var CableLine018 = CreateObjectAbove(CableLine, 231, 2286);
	CableLine018.LineColors = [-15461326,-15461326];
	CableLine018->SetAction("Connect", CableCrossing018, CableCrossing017);
	CableLine018->SetConnectedObjects(CableCrossing018, CableCrossing017);

	var ToolsWorkshop001 = CreateObjectAbove(ToolsWorkshop, 76, 388);

	var CableCrossing019 = CreateObjectAbove(CableCrossing, 113, 387);
	CableCrossing019->SetCategory(C4D_StaticBack|C4D_Structure);
	CableCrossing019->SetMeshMaterial("CableCarStation_SignStation", 1);
	CableCrossing019->CombineWith(ToolsWorkshop001);

	var CableLine019 = CreateObject(CableLine, 208, 747);
	CableLine019.LineColors = [-15461326,-15461326];
	CableLine019->SetPosition(208, 747);
	CableLine019->SetAction("Connect", CableCrossing019, CableCrossing003);
	CableLine019->SetConnectedObjects(CableCrossing019, CableCrossing003);
	var CableLorry001 = CreateObjectAbove(CableLorry, 560, 382);

	var CableHoist001 = CreateObjectAbove(CableHoist, 560, 382);
	CableHoist001->SetComDir(COMD_None);
	CableHoist001->SetCableSpeed(1);
	CableHoist001->EngageRail(CableCrossing002, true);
	CableHoist001->PickupVehicle(CableLorry001);
	var Wood001 = CableLorry001->CreateContents(Wood);
	Wood001->SetPosition(560, 375);
	var Wood002 = CableLorry001->CreateContents(Wood);
	Wood002->SetPosition(560, 375);
	var Wood003 = CableLorry001->CreateContents(Wood);
	Wood003->SetPosition(560, 375);

	var Metal001 = ToolsWorkshop001->CreateContents(Metal);
	Metal001->SetPosition(76, 369);

	var CableReel001 = CreateObjectAbove(CableReel, 159, 389);
	CableReel001->Unstick(7);
	return true;
}
