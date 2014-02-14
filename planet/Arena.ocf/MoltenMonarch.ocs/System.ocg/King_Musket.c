#appendto Musket

local king_size;

public func MakeKingSize() { king_size = true; return(SetMeshMaterial("KingMusket",0)); }
public func MakeNormalSize() { king_size = false; return(SetMeshMaterial("Musket",0)); }
public func Departure() { MakeNormalSize(); }

private func FireWeapon(object clonk, int angle)
{
	var shot = Contents(0)->TakeObject();
	
	if(king_size)
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

	Sound("GunShoot?");

	// Muzzle Flash & gun smoke
	var IX=Sin(180-angle,MuskFront);
	var IY=Cos(180-angle,MuskUp)+MuskOffset;
	if(Abs(Normalize(angle,-180)) > 90)
		IY=Cos(180-angle,MuskDown)+MuskOffset;

	var x = Sin(angle, 20);
	var y = -Cos(angle, 20);
	CreateParticle("Smoke", IX, IY, PV_Random(x - 20, x + 20), PV_Random(y - 20, y + 20), PV_Random(40, 60), Particles_Smoke(), 20);
	clonk->CreateMuzzleFlash(IX, IY, angle, 20);

	CreateParticle("Flash", 0, 0, 0, 0, 8, Particles_Flash());
}
