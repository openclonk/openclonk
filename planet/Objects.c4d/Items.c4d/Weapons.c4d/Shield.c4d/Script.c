/*-- Shield --*/

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

local iMesh;

public func HoldingEnabled() { return true; }

public func Selection(pTarget, fSecond)
{
	if(fSecond) return;
  iMesh = pTarget->AttachMesh(SHIE, "pos_hand1", "main", 1000);
}

public func Deselection(pTarget, fSecond)
{
	if(fSecond) return;
	pTarget->DetachMesh(iMesh);
}