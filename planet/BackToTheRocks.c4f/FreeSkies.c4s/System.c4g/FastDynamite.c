/* Everyone wins! */

#appendto Dynamite

local fast;

func Initialize()
{
	fast=0;
	_inherited();
}

func MakeFast(int f) { fast=f; }

private func Fusing()
{
	var sin=Sin(180-GetR(),5);
	var cos=Cos(180-GetR(),5);

	if(Contained()!=nil)
	{
		//If the dynamite is held, sparks come from clonk's center.
		sin=0;
		cos=0;
	}

	// Effekt
	if(GetActTime() < (120-fast))
		CastParticles("Spark",1,20,sin,cos,15,25,RGB(255,200,0),RGB(255,255,150));
	// Explosion
	else if(GetActTime() > (140-fast))
		DoExplode();
}