// King size javelin hurts more.
#appendto Javelin

local king_size;

public func MakeKingSize()
{
	king_size = true;
	SetMeshMaterial("KingJavelin", 0);
}

public func MakeNormalSize()
{
	king_size = false;
	SetMeshMaterial("javelin", 0);
}

public func Departure() { MakeNormalSize(); }

protected func JavelinStrength()
{
	if (king_size)
		return 21;
	return 14;
}

public func DoThrow(object clonk, int angle)
{
	var javelin = _inherited(clonk, angle, ...);
	
	if (javelin && king_size)
	{
		javelin->MakeKingSize();
	}
	
	return javelin;
}
