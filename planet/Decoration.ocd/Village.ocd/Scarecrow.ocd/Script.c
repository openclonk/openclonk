/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(90);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-30,-80),1, 10), GetID().MeshTransformation));
}
