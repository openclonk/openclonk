/**
	Chunk of Lava
	Hot molten stone from the inner earth.
	
	@author Maikel
*/

local mat;

protected func Initialize() 
{
	AddEffect("IntEvaporate", this, 100, 1, this);
	// Lava chunk is on fire.
	//Incinerate(); // TODO: Wait for decent graphics
	return;
}
  
protected func Hit()
{
	if (!mat)
		mat="DuroLava";
  	CastPXS(mat, GetCon()/2, 35);
  	return RemoveObject();
}

private func FxIntEvaporateTimer(object target, proplist effect, int time)
{
	// Some smoke trail.
	Smoke(0, 0, 5);
	Smoke(0, -5, Random(7));
	if (time > 75)
	{
		Hit();
		return -1;
	}
	return 1;
}

/*-- Proplist --*/

local Name = "$Name$";
