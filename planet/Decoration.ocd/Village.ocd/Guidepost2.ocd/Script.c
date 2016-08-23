/**
	@author Nachtfalter
*/

#include EnvPack_Guidepost

local Name="$Name$";
local Description="$Description$";

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-180,180),0,10), Trans_Scale(130)));
}