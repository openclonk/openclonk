/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(500);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-50, 50),0, 10), GetID().MeshTransformation));
}