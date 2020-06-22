
#appendto Clonk

func Initialize()
{
	AddEffect("Lifesteal", this, 100, 0, nil);
	_inherited(...);
}