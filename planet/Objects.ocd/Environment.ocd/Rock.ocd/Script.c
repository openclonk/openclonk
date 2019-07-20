/*-- Big Rock --*/

private func Initialize()
{
// 	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0, 359),0, 1, 0));
  SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(1000 + RandomX(-500, 500),1000 + RandomX(-500, 500),1000 + RandomX(-500, 500)),Trans_Rotate(RandomX(0, 359),0, 1, 0), Trans_Rotate(RandomX(0, 359), 1, 0, 0)));
}

local Name = "$Name$";
