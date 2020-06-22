#appendto Bow

local king_size;

public func MakeKingSize()
{
	king_size = true;
	SetMeshMaterial("Kingwood", 1); 
	SetMeshMaterial("KingLeather", 0);
}

public func MakeNormalSize()
{
	king_size = false;
	SetMeshMaterial("wood", 0);
	SetMeshMaterial("Leather", 1);
}

public func Departure() { MakeNormalSize(); }
