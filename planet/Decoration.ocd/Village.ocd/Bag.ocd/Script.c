/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(165); // average scale
}

func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-180, 180),0, 10), Trans_Scale(RandomX(787, 1212)), GetID().MeshTransformation));
}
