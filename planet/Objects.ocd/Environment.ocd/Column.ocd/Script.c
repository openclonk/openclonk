/**
	Column

	@author
*/

public func Initialize()
{
	this.MeshTransformation = Trans_Mul(this.MeshTransformation, Trans_Rotate(Random(360), 0, 1, 0));
	return;
}

public func Damage()
{
	if (GetDamage() > 80)
	{
			if (!this)
			return false;
		var ctr = Contained();
		// Transfer all contents to container.
		while (Contents())
			if (!ctr || !Contents()->Enter(ctr))
				Contents()->Exit();
		// Split components.
		for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
			for (var j = 0; j < GetComponent(compid); ++j)
			{
				var comp = CreateObjectAbove(compid, nil, -Random(50), GetOwner());
				if (OnFire()) comp->Incinerate();
				if (!ctr || !comp->Enter(ctr))
				{
					comp->SetR(Random(360));
					comp->SetXDir(Random(3) - 1);
					comp->SetYDir(Random(3) - 1);
					comp->SetRDir(Random(3) - 1);
					comp->SetClrModulation(RGB(240, 210, 200));	//give rocks the color of brick
				}
				}
		RemoveObject();
	}
	return;
}

/*-- Properties --*/

local Name = "$Name$";
local Plane = 200;
local Components = {Rock = 7};
local MeshTransformation = [152, 0, 0, 0, 0, 152, 0, 0, 0, 0, 152, 0]; // Trans_Scale(152)
local PictureTransformation = [152, 0, 0, 0, 0, 152, 0, 0, 0, 0, 152, 0]; // Trans_Scale(152)