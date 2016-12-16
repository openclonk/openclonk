/**
	@author Dustin Neß (dness.de)
*/

protected func Construction()
{
	//SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-180,180),RandomX(-10,10),10), Trans_Scale(RandomX(5,10))));
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-180,180), 0, 1, 0), Trans_Scale(10)));
}