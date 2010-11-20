#appendto Musket

local bKingSize;

public func MakeKingSize() { bKingSize = true; return(SetMeshMaterial("KingMusket")); }
public func MakeNormalSize() { bKingSize = false; return(SetMeshMaterial("Musket")); }
public func Departure() { MakeNormalSize(); }

private func FireWeapon(object clonk, int angle)
{
	var shot = Contents(0)->TakeObject();
	
	if(bKingSize)
	{
		var box = CreateObject(LeadShot,0,0,this->GetOwner());
		shot->LessDamage();
		shot->Launch(clonk,angle,iBarrel,260+Random(40));
		shot=box->TakeObject();
		shot->LessDamage();
		shot->Launch(clonk,angle-3,iBarrel,240+Random(30));
		shot=box->TakeObject();
		shot->LessDamage();
		shot->Launch(clonk,angle-2,iBarrel,240+Random(30));
		shot=box->TakeObject();
		shot->LessDamage();
		shot->Launch(clonk,angle+2,iBarrel,240+Random(30));
		shot=box->TakeObject();
		shot->LessDamage();
		shot->Launch(clonk,angle+3,iBarrel,240+Random(30));
		
		box->RemoveObject();

	}
	else
		shot->Launch(clonk,angle,iBarrel,200);

	
	loaded = false;
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(1500,0,-1500),Trans_Rotate(170,0,1,0),Trans_Rotate(30,0,0,1)));

	Sound("GunShoot*.ogg");

	// Muzzle Flash & gun smoke
	var IX=Sin(180-angle,MuskFront);
	var IY=Cos(180-angle,MuskUp)+MuskOffset;
	if(Abs(Normalize(angle,-180)) > 90)
		IY=Cos(180-angle,MuskDown)+MuskOffset;

	for(var i=0; i<10; ++i)
	{
		var speed = RandomX(0,10);
		var r = angle;
		CreateParticle("ExploSmoke",IX,IY,+Sin(r,speed)+RandomX(-2,2),-Cos(r,speed)+RandomX(-2,2),RandomX(100,400),RGBa(255,255,255,50));
	}
	CreateParticle("MuzzleFlash",IX,IY,+Sin(angle,500),-Cos(angle,500),450,RGB(255,255,255),clonk);
	CreateParticle("Flash",0,0,0,0,800,RGBa(255,255,64,150));
}
