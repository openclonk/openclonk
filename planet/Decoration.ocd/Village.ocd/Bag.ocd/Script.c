/**
	@author Dustin Neß (dness.de)
*/

public func Definition(proplist def) {
	def.MeshTransformation = Trans_Scale(165); // average scale
}

public func Construction() {
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-180,180),0,10), Trans_Scale(RandomX(787,1212)), GetID().MeshTransformation));
}
