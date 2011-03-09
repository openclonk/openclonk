/* Lorry Test */

func Initialize()
{
//	CreateObject(Ropeladder, 174, 445)->Unroll(-1, 0, 25);
	CreateObject(Ropeladder, 328, 564);
	CreateObject(Ropeladder, 226, 330);

	var Rock1 = CreateObject(Rock, 159, 363);
	var Rock2 = CreateObject(Rock, 232, 388);
	Log("Give the lorry commands with SetDestination(target);");
	Log("target can be the number of the crossing or a pointer to the crossing");
//	CreateObject(Ropeladder, 197, 432)->MakeBridge(Rock1, Rock2);
}

 func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	JoinPlayer(iPlr);
	return;
}

 func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(Clonk, 0, 0, iPlr);
	clonk->MakeCrewMember(iPlr);
	SetCursor(iPlr,clonk);
	JoinPlayer(iPlr);
	return;
}

 func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	clonk->SetPosition(50, 490);
	clonk->CreateContents(CableLorryReel);
	clonk->CreateContents(Hammer);
	SetPlrKnowledge(0, CableCrossing);
	var lorry = clonk->CreateObject(CableLorry);
	var obj1 = CreateObject(CableCrossing, 104, 496);
	var obj2 = CreateObject(CableCrossing, 191, 438);
	var obj3 = CreateObject(CableCrossing, 247, 424);
	var obj4 = CreateObject(CableCrossing, 159, 369);
        var obj5 = CreateObject(CableCrossing, 235, 315);
	var obj6 = CreateObject(CableCrossing, 355, 477);
	var obj7 = CreateObject(CableCrossing, 317, 339);
	CreateObject(CableLine, 104, 486)->SetConnecteObjects(obj1, obj2);
	CreateObject(CableLine, 191, 438)->SetConnecteObjects(obj2, obj3);
	CreateObject(CableLine, 191, 438)->SetConnecteObjects(obj3, obj4);
	CreateObject(CableLine, 191, 438)->SetConnecteObjects(obj4, obj5);
	CreateObject(CableLine, 191, 438)->SetConnecteObjects(obj3, obj6);
	CreateObject(CableLine, 191, 438)->SetConnecteObjects(obj5, obj7);
	CreateObject(CableLine, 191, 438)->SetConnecteObjects(obj6, obj7);
	WaypointsMakeList();
	lorry->StartRail(obj1);
	lorry->SetDestination(0);
//	clonk->CreateContents(Javelin);
//		clonk->CreateContents(DynamiteBox);
//	clonk->CreateContents(GrappleBow);
//	clonk->CreateContents(Bow);
//	clonk->Collect(CreateObject(Arrow));
	return;
}