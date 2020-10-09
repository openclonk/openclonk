/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(10, 10, 100);
}

func Construction()
{
	// Note: We apply the scaling after the rotation, which is the "wrong"
	//       order. However, due to what I believe is an engine bug,
	//       scaling down too much along the z axis makes the object disappear.
	SetProperty("MeshTransformation", Trans_Mul(GetID().MeshTransformation, Trans_Rotate(RandomX(-180, 180), 0, 1, 0)));
}
