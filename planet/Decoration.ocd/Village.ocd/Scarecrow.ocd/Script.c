/**
	@author Dustin Neß (dness.de)
*/

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-30,-80),1,10), Trans_Scale(90)));
}