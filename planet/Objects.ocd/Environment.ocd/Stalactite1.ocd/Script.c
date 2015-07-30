/**
	Stalactite1
	Hangs from the ceiling

	@author 
*/

local Name = "$Name$";
local Description = "$Description$";

func Initialize()
{
	SetGraphics(Format("%d",Random(5)));
	
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(600+Random(900)), Trans_Rotate(-25+Random(50),0,1,0)));
	
	var tinys = CreateObject(TinyStalactite, 0, 0);
	tinys->SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(1000), Trans_Rotate(180,0,0,1)));
	tinys->SetParent(this);
}

func SetStalagmite()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(1000), Trans_Rotate(180,0,0,1)));
}