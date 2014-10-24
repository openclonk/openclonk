// The wall kit produces rock walls, because granite is indestructible.

#appendto WallKit

private func CreateBridge(object clonk, int x, int y)
{
	var c = Offset2BridgeCoords(clonk, x, y);
	x=clonk->GetX(); y=clonk->GetY();
	DrawMaterialQuad("Rock-rock", x+c.x1-c.dx,y+c.y1-c.dy, x+c.x1+c.dx,y+c.y1+c.dy, x+c.x2+c.dx,y+c.y2+c.dy, x+c.x2-c.dx,y+c.y2-c.dy, DMQ_Bridge);
	clonk->Sound("WallKitLock");
	return true;
}