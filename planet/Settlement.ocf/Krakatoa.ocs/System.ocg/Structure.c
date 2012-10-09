// Energy bars above structures with hitpoints

#appendto *

public func Initialize()
{
	if (this.HitPoints != nil)
	{
		AddVertex(0, 0);
		CreateObject(EnergyBar)->SetTarget(this, GetVertexNum());
	}
	return _inherited(...);
}