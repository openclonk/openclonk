// Dynamite only one vertex.

#appendto Dynamite

protected func Initialize()
{
	SetVertex(1, VTX_X, 0, 2);
	SetVertex(1, VTX_Y, 0, 2);
	SetVertex(2, VTX_X, 0, 2);
	SetVertex(2, VTX_Y, 0, 2);
	SetVertex(3, VTX_X, 0, 2);
	SetVertex(3, VTX_Y, 0, 2);
	return _inherited(...);
}