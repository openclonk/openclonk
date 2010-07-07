/*-- Grass --*/

protected func Initialize()
{
	DoCon(Random(50));
	if(Random(2)) SetGraphics("1");
}

public func Incineration()
{
	Destroy();
	return 1;
}

private func Destroy()
{
	CastParticles("Grass",10,35,0,0,30,50,RGB(255,255,255),RGB(255,255,255));
	RemoveObject();
}