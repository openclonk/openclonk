/* Lorry Test */

protected func Initialize()
{
	var foundry = CreateObject(Foundry, 360, 475);
	var tools = CreateObject(ToolsWorkshop, 50, 495);
	var cross1 = CreateObject(CableCrossing, 90, 495);
	var cross2 = CreateObject(CableCrossing, 175, 445);
	var cross3 = CreateObject(CableCrossing, 240, 425);
	var cross4 = CreateObject(CableCrossing, 161, 369);
    var cross5 = CreateObject(CableCrossing, 235, 315);
	var cross6 = CreateObject(CableCrossing, 317, 339);
	var cross7 = CreateObject(CableCrossing, 105, 355);
	CreateObject(CableLine, 104, 486)->SetConnectedObjects(cross1, cross2);
	CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross2, cross3);
	CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross3, foundry);
	//CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross3, cross4);
	CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross4, cross5);
	CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross5, cross6);
	//CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross6, foundry);
	CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross7, cross4);
	//CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross4, cross2);
	CreateObject(CableLine, 191, 438)->SetConnectedObjects(cross1, tools);
	WaypointsMakeList();
	
	cross1->CreateContents(Ore, 5);
	cross2->CreateContents(Coal, 8);
	cross3->CreateContents(Wood, 4);
	
	var lorry = cross2->CreateObject(CableLorry);
	lorry->EngageRail(cross2);
	//lorry->SetDestination(foundry);
	//foundry->RequestObject(Ore, 5);
	//foundry->RequestObject(Coal, 5);
	
	tools->AddToQueue(Shovel, 2);
	tools->AddToQueue(Pickaxe, 1);

	Log("Give the lorry commands with SetDestination(target);");
	Log("target can be the number of the crossing or a pointer to the crossing");
	Log("Activate all crossings to stations and watch.");
	return;
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
	return;
}