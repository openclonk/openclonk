/**
	@author Dustin Neß (dness.de)
*/

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(10,0,10), Trans_Scale(50)));
	RandomPainting(3);
}

// Change when changing textures is prossible soon(?)
public func RandomPainting(iMax) {
	return Random(iMax);
}