/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Mul(Trans_Rotate(10, 0, 10), Trans_Scale(50));
}

protected func Construction()
{
	RandomPainting(3);
}

// Change when changing textures is prossible soon(?)
public func RandomPainting(iMax) {
	return Random(iMax);
}