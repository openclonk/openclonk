/*-- Grass --*/

protected func Initialize()
{
	DoCon(Random(50));
	//SetGraphics(Format("%d.10",Random(2))); //commented out for now due to SG("1.5") bug
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