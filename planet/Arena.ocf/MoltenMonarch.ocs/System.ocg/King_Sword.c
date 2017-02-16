#appendto Sword

local king_size;

public func MakeKingSize()
{
	king_size = true;
	SetMeshMaterial("KingSword2", 0);
}

public func MakeNormalSize()
{
	king_size = false;
	SetMeshMaterial("Sword2", 0);
}

public func Departure() { MakeNormalSize(); }

func SwordDamage(int shield)
{
	var damage = _inherited(shield, ...);
	if (king_size) damage += 3000 + Random(3000);
	return damage;
}
