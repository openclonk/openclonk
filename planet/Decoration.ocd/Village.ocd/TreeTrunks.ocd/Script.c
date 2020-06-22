/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(550);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-40, 40),0, 20), GetID().MeshTransformation));
}
