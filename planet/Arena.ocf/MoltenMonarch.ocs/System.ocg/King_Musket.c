#appendto Blunderbuss

local king_size;

public func MakeKingSize()
{
	king_size = true;

	this.BulletsPerShot = 10;
	this.BulletSpread = 500;
	this.BulletSpeed = [240, 270];	

	SetMeshMaterial("KingMusket", 0);
}

public func MakeNormalSize()
{
	king_size = false;

	this.BulletsPerShot = 5;
	this.BulletSpread = 300;
	this.BulletSpeed = [196, 204];	

	SetMeshMaterial("Musket", 0);
}

public func Departure() { MakeNormalSize(); }
