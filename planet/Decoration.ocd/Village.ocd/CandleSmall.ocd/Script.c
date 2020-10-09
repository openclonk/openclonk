/**
	@author Dustin Neß (dness.de)
*/

local fShine = false;
local objShine;
local x = 0;
local y = 0;

protected func Initialize()
{
	return SetAction("Shine");
}

public func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(60);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-35, 35),0, 10), GetID().MeshTransformation));
	objShine = CreateObjectAbove(EnvPack_CandleSmall_Shine, x, y + 10, -1);
	objShine->SetCon(40);
	objShine->SetClrModulation(RGBa(255, 255, 255, 70));
}

private func Shining()
{

	// check if position changed
	if (x != GetX() && y != GetY())
	{
		x = GetX();
		y = GetY();
		objShine->SetObjectBlitMode(GFX_BLIT_Additive);
		//objShine->SetClrModulation(RGBa(255, 255, 255, 228));
		objShine->SetPosition(GetX(), GetY() + 1);
	}
	
	if (!fShine)
		fShine = true;
}

// flickering
private func Noise()
{
	if (RandomX(5) <= 2)
	{
		//objShine->SetClrModulation(RGBa(255, 255, 255, RandomX(190, 228)));
		objShine->SetClrModulation(RGBa(255, 255, 255, RandomX(60, 70)));
	}
}

public func SetOn(fOn)
{
	if (fOn)
	{
		if (!FindObject(Find_ID(objShine)))
			objShine = CreateObjectAbove(EnvPack_CandleSmall_Shine, 0, -5, -1);
		SetAction("Shine");
		SetClrModulation(RGB(255, 255, 255));
		fShine = true;
	}
	else
	{
		objShine->RemoveObject();
		SetAction("Idle");
		SetClrModulation(RGB(155, 155, 155)); // Turn model darker
		fShine = false;
	}
	return true;
}

local ActMap = {
	Shine: {
		Prototype: Action,
		Name: "Shine",
		StartCall: "Shining",
		NextAction: "Shine",
		EndCall: "Noise",
		Delay: 5
	}
};
