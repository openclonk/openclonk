/* Automatically created objects file */

func InitializeObjects()
{
	var CableCrossing001 = CreateObjectAbove(CableCrossing, 347, 389);
	var CableCrossing002 = CreateObjectAbove(CableCrossing, 554, 391);

	var CableLine001 = CreateObjectAbove(CableLine, 551, 749);
	CableLine001->SetAction("Connect", CableCrossing001, CableCrossing002);
	CableLine001->SetConnectedObjects(CableCrossing001, CableCrossing002);

	var CableCrossing003 = CreateObjectAbove(CableCrossing, 209, 390);

	var CableLine002 = CreateObjectAbove(CableLine, 206, 747);
	CableLine002->SetAction("Connect", CableCrossing001, CableCrossing003);
	CableLine002->SetConnectedObjects(CableCrossing001, CableCrossing003);

	var CableCrossing004 = CreateObjectAbove(CableCrossing, 652, 491);

	var CableLine003 = CreateObjectAbove(CableLine, 551, 849);
	CableLine003->SetAction("Connect", CableCrossing004, CableCrossing002);
	CableLine003->SetConnectedObjects(CableCrossing004, CableCrossing002);

	var CableCrossing005 = CreateObjectAbove(CableCrossing, 694, 419);

	var CableLine004 = CreateObjectAbove(CableLine, 651, 949);
	CableLine004->SetAction("Connect", CableCrossing005, CableCrossing004);
	CableLine004->SetConnectedObjects(CableCrossing005, CableCrossing004);

	var CableCrossing006 = CreateObjectAbove(CableCrossing, 770, 360);

	var CableLine005 = CreateObjectAbove(CableLine, 693, 806);
	CableLine005->SetAction("Connect", CableCrossing006, CableCrossing005);
	CableLine005->SetConnectedObjects(CableCrossing006, CableCrossing005);

	var CableCrossing007 = CreateObjectAbove(CableCrossing, 810, 351);

	var CableLine006 = CreateObjectAbove(CableLine, 771, 687);
	CableLine006->SetAction("Connect", CableCrossing007, CableCrossing006);
	CableLine006->SetConnectedObjects(CableCrossing007, CableCrossing006);

	var CableCrossing008 = CreateObjectAbove(CableCrossing, 939, 360);

	var CableLine007 = CreateObjectAbove(CableLine, 815, 679);
	CableLine007->SetAction("Connect", CableCrossing008, CableCrossing007);
	CableLine007->SetConnectedObjects(CableCrossing008, CableCrossing007);

	var CableCrossing009 = CreateObjectAbove(CableCrossing, 599, 257);

	var CableLine008 = CreateObjectAbove(CableLine, 808, 669);
	CableLine008->SetAction("Connect", CableCrossing009, CableCrossing007);
	CableLine008->SetConnectedObjects(CableCrossing009, CableCrossing007);

	var CableCrossing010 = CreateObjectAbove(CableCrossing, 519, 259);

	var CableLine009 = CreateObjectAbove(CableLine, 599, 484);
	CableLine009->SetAction("Connect", CableCrossing010, CableCrossing009);
	CableLine009->SetConnectedObjects(CableCrossing010, CableCrossing009);

	var CableCrossing011 = CreateObjectAbove(CableCrossing, 465, 289);

	var CableLine010 = CreateObjectAbove(CableLine, 517, 516);
	CableLine010->SetAction("Connect", CableCrossing011, CableCrossing010);
	CableLine010->SetConnectedObjects(CableCrossing011, CableCrossing010);
	var CableLine011 = CreateObjectAbove(CableLine, 465, 645);
	CableLine011->SetAction("Connect", CableCrossing003, CableCrossing011);
	CableLine011->SetConnectedObjects(CableCrossing003, CableCrossing011);

	var CableCrossing012 = CreateObjectAbove(CableCrossing, 744, 589);

	var CableLine012 = CreateObjectAbove(CableLine, 652, 1046);
	CableLine012->SetAction("Connect", CableCrossing012, CableCrossing004);
	CableLine012->SetConnectedObjects(CableCrossing012, CableCrossing004);

	var CableCrossing013 = CreateObjectAbove(CableCrossing, 680, 619);

	var CableLine013 = CreateObjectAbove(CableLine, 743, 1174);
	CableLine013->SetAction("Connect", CableCrossing013, CableCrossing012);
	CableLine013->SetConnectedObjects(CableCrossing013, CableCrossing012);

	var CableCrossing014 = CreateObjectAbove(CableCrossing, 568, 690);

	var CableLine014 = CreateObjectAbove(CableLine, 678, 1278);
	CableLine014->SetAction("Connect", CableCrossing014, CableCrossing013);
	CableLine014->SetConnectedObjects(CableCrossing014, CableCrossing013);

	var CableCrossing015 = CreateObjectAbove(CableCrossing, 456, 711);

	var CableLine015 = CreateObjectAbove(CableLine, 570, 1369);
	CableLine015->SetAction("Connect", CableCrossing015, CableCrossing014);
	CableLine015->SetConnectedObjects(CableCrossing015, CableCrossing014);

	var CableCrossing016 = CreateObjectAbove(CableCrossing, 372, 650);

	var CableLine016 = CreateObjectAbove(CableLine, 454, 1390);
	CableLine016->SetAction("Connect", CableCrossing016, CableCrossing015);
	CableLine016->SetConnectedObjects(CableCrossing016, CableCrossing015);

	var CableCrossing017 = CreateObjectAbove(CableCrossing, 231, 590);

	var CableLine017 = CreateObjectAbove(CableLine, 374, 1266);
	CableLine017->SetAction("Connect", CableCrossing017, CableCrossing016);
	CableLine017->SetConnectedObjects(CableCrossing017, CableCrossing016);

	var CableCrossing018 = CreateObjectAbove(CableCrossing, 131, 528);

	var CableLine018 = CreateObjectAbove(CableLine, 231, 1148);
	CableLine018->SetAction("Connect", CableCrossing018, CableCrossing017);
	CableLine018->SetConnectedObjects(CableCrossing018, CableCrossing017);

	CreateObjectAbove(CableHoist, 527, 380);

	CreateObjectAbove(CableLorry, 503, 380);

	CreateObjectAbove(CableLorryReel, 530, 388);
	return true;
}
