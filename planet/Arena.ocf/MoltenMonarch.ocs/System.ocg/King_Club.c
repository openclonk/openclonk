#appendto Club

local king_size;

public func MakeKingSize()
{
	king_size = true;
	SetMeshMaterial("KingClub", 0);
}

public func MakeNormalSize()
{
	king_size = false;
	SetMeshMaterial("Club", 0);
}

public func Departure() { MakeNormalSize(); }


func ClubDamage()
{
	return _inherited(...) + 3000;
}

func ClubVelocityPrecision()
{
	return _inherited(...) / 2;
}

