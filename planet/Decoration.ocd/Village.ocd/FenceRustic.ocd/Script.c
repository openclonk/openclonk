/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(60);
}

public func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-35, 35),0, 10), GetID().MeshTransformation));
}
