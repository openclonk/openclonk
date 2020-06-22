/*-- WallKit preview --*/

// definition call: create line
public func Create(int x1, int y1, int x2, int y2, int clr)
{
	var obj = CreateObjectAbove(WallKit_Preview);
	obj->Set(x1, y1, x2, y2, clr);
	return obj;
}

public func Set(int x1, int y1, int x2, int y2, int clr)
{
	SetVertexXY(0, x1, y1);
	SetVertexXY(1, x2, y2);
	this.LineColors = [clr, clr];
	return true;
}

public func SetColor(int clr)
{
	this.LineColors = [clr, clr];
	return true;
}
