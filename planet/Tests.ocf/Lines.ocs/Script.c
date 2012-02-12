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
  var bridge = CreateObject(Ropebridge, 515, 547);
  bridge->MakeBridge(Anchor1, Anchor2);
  bridge->SetFragile();
  
  var Anchor1 = CreateObject(Ropebridge_Post, 266, 435+6);
  var Anchor2 = CreateObject(Ropebridge_Post, 346, 473+6);
  Anchor2->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
  Anchor2.Double->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
  CreateObject(Ropebridge, 515, 547)->MakeBridge(Anchor1, Anchor2);
  CreateObject(Rank, 505, 547);
  CreateObject(Rock,495, 547);
//	CreateObject(Ropeladder, 197, 432)->MakeBridge(Rock1, Rock2);
//   for(var i = 0; i < 10; i++)
//     bridge->CreateObject(Zap, RandomX(-30,30), -40+RandomX(-30,30));
  
  
  var obj;
  obj = CreateObject(Rank, 395, 484);
obj->SetR(40);
obj->SetProperty("MeshTransformation", [-1000, 0, 0, 0, 0, 1000, 0, 0, 0, 0, -1000, 0]);
obj.Plane = 400;
obj = CreateObject(Rank, 433, 509);
obj->SetR(-26);
obj->SetProperty("MeshTransformation", [-602, 0, -799, 0, 0, 1000, 0, 0, 799, 0, -602, 0]);
obj.Plane = 400;
obj = CreateObject(Rank, 448, 517);
obj->SetR(24);
obj->SetProperty("MeshTransformation", [-848, 0, 530, 0, 0, 1000, 0, 0, -530, 0, -848, 0]);
obj.Plane = 100;
obj = CreateObject(Rank, 366, 482);
obj->SetR(16);
obj->SetProperty("MeshTransformation", [-999, 0, -52, 0, 0, 1000, 0, 0, 52, 0, -999, 0]);
obj.Plane = 100;
obj = CreateObject(Rank, 501, 548);
obj->SetR(-16);
obj->SetProperty("MeshTransformation", [629, 0, 777, 0, 0, 1000, 0, 0, -777, 0, 629, 0]);
obj.Plane = 100;
obj = CreateObject(Rank, 417, 474);
obj->SetR(-4);
obj->SetProperty("MeshTransformation", [978, 0, 208, 0, 0, 1000, 0, 0, -208, 0, 978, 0]);
obj.Plane = 100;
obj = CreateObject(Rank, 482, 532);
obj->SetR(11);
obj->SetProperty("MeshTransformation", [743, 0, 669, 0, 0, 1000, 0, 0, -669, 0, 743, 0]);
obj.Plane = 100;
obj = CreateObject(Rank, 468, 511);
obj->SetR(40);
obj->SetProperty("MeshTransformation", [-423, 0, 906, 0, 0, 1000, 0, 0, -906, 0, -423, 0]);
obj.Plane = -400;
obj = CreateObject(Trunk, 402, 507);
obj->SetR(10);
obj->SetProperty("MeshTransformation", [940, 0, 342, 0, 0, 1000, 0, 0, -342, 0, 940, 0]);
obj.Plane = 100;
obj = CreateObject(Trunk, 456, 545);
obj->SetR(120);
obj->SetProperty("MeshTransformation", [-906, 0, -423, 0, 0, 1000, 0, 0, 423, 0, -906, 0]);
obj.Plane = -40;
obj = CreateObject(Tree_Coniferous, 375, 483);
obj->SetProperty("MeshTransformation", [139, 0, 990, 0, 0, 1000, 0, 0, -990, 0, 139, 0]);
obj.Plane = 100;
obj = CreateObject(Tree_Coniferous, 455, 526);
obj->SetProperty("MeshTransformation", [276, 0, 961, 0, 0, 1000, 0, 0, -961, 0, 276, 0]);
obj.Plane = 100;
obj = CreateObject(BigRock, 497, 561);
obj->SetProperty("MeshTransformation", [1003, -29, 18, 0, 0, -313, -521, 0, 44, 1096, -658, 0]);
obj.Plane = 100;
obj = CreateObject(BigRock, 448, 527);
obj->SetProperty("MeshTransformation", [916, 133, -534, 0, 0, -1124, -280, 0, -354, 142, -571, 0]);
obj.Plane = -100;
obj = CreateObject(BigRock, 363, 491);
obj->SetProperty("MeshTransformation", [-782, 469, 16, 0, 0, -40, 1162, 0, 465, 772, 27, 0]);
obj.Plane = -400;
obj = CreateObject(BigRock, 417, 512);
obj->SetProperty("MeshTransformation", [-759, -552, 9, 0, 0, -22, -1337, 0, 465, -639, 10, 0]);
obj.Plane = -400;
obj = CreateObject(Fern, 468, 530);
obj->SetProperty("MeshTransformation", [-156, 0, 988, 0, 0, 1000, 0, 0, -988, 0, -156, 0]);
obj.Plane = 400;
obj = CreateObject(Fern, 383, 481);
obj->SetProperty("MeshTransformation", [-891, 0, -454, 0, 0, 1000, 0, 0, 454, 0, -891, 0]);
obj.Plane = 100;
obj = CreateObject(Fern, 422, 512);
obj->SetProperty("MeshTransformation", [921, 0, -391, 0, 0, 1000, 0, 0, 391, 0, 921, 0]);
obj.Plane = 100;
obj = CreateObject(Mushroom, 368, 482);
obj->SetProperty("MeshTransformation", [-999, 0, 35, 0, 0, 1000, 0, 0, -35, 0, -999, 0]);
obj.Plane = 500;
obj = CreateObject(Mushroom, 472, 532);
obj->SetProperty("MeshTransformation", [-707, 0, 707, 0, 0, 1000, 0, 0, -707, 0, -707, 0]);
obj.Plane = 500;
}

func SerializeObjects(ids)
{
  if(!ids)
    ids = [Rank, Trunk, Tree_Coniferous, BigRock, Fern];
  for( findid in ids)
    for(var obj in FindObjects(Find_ID(findid)))
    {
      Log("obj = CreateObject(%i, %d, %d);", findid, obj->GetX(), obj->GetDefBottom());
      if(obj->GetR())
        Log("obj->SetR(%d);", obj->GetR());
      if(obj->GetProperty("MeshTransformation"))
        Log("obj->SetProperty(\"MeshTransformation\", %v);", obj->GetProperty("MeshTransformation"));
      if(obj->GetProperty("Plane"))
        Log("obj.Plane = %d;", obj->GetProperty("Plane"));
    }
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
  clonk->SetGraphics(nil, Skin_Steampunk);
	SetCursor(iPlr,clonk);
	JoinPlayer(iPlr);
	return;
}

 func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
//	clonk->SetPosition(502, 538);
 //   clonk->SetGraphics(nil, Skin_Steampunk);
	clonk->CreateContents(Musket);
	clonk->CreateContents(LeadShot);
//	clonk->CreateContents(Javelin);
//		clonk->CreateContents(DynamiteBox);
	clonk->CreateContents(GrappleBow);
	clonk->CreateContents(Bow);
	clonk->Collect(CreateObject(Arrow));
	return;
}