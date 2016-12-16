/**
	@author Dustin Neß (dness.de)
*/

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-35,35),0,10), Trans_Scale(60)));
}