/* Sky race */

func Initialize()
{
//	CreateObject(Ropeladder, 174, 445)->Unroll(-1, 0, 25);
	CreateObject(Ropeladder, 328, 564);
	CreateObject(Ropeladder, 226, 330);

	var Rock1 = CreateObject(Rock, 159, 363);
	var Rock2 = CreateObject(Rock, 232, 388);
  
  var Anchor1 = CreateObject(Ropebridge_Post, 515, 547);
  var Anchor2 = CreateObject(Ropebridge_Post, 602, 538);
  Anchor2->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
  Anchor2.Double->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
  CreateObject(Ropebridge, 515, 547)->MakeBridge(Anchor1, Anchor2);
  
  var Anchor1 = CreateObject(Ropebridge_Post, 266, 435+6);
  var Anchor2 = CreateObject(Ropebridge_Post, 346, 473+6);
  Anchor2->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
  Anchor2.Double->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
  CreateObject(Ropebridge, 515, 547)->MakeBridge(Anchor1, Anchor2);
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
	clonk->SetPosition(502, 538);
//	clonk->CreateContents(Musket);
//	clonk->CreateContents(LeadShot);
//	clonk->CreateContents(Javelin);
//		clonk->CreateContents(DynamiteBox);
	clonk->CreateContents(GrappleBow);
//	clonk->CreateContents(Bow);
//	clonk->Collect(CreateObject(Arrow));
	return;
}