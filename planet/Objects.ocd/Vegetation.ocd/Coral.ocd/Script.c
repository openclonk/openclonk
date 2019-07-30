/*-- Coral --*/

#include Library_Edible

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;

local mesh_attachments;

local ActMap = {
	Exist = {
		Prototype = Action,
		Name = "Exist",
		Procedure = DFA_FLOAT,
		NextAction = "Exist",
		Delay = 0,
		FacetBase = 1,
	}
};

func Place(int amount, proplist area, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (area) loc_area = Loc_InArea(area);
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Wall(CNAT_Bottom | CNAT_Top | CNAT_Left | CNAT_Right), loc_area);
		if (!spot) continue;
		
		var plant = CreateObject(this, spot.x, spot.y, NO_OWNER);
		plant->SetClrModulation(HSL(Random(255), 255, 200));
		plant->AddFork(nil, true);

		--amount;
	}
	return true;
}

func StartFloating()
{
	RemoveTimer("Seed");
	SetAction("Idle");
	this.Collectible = 1;
	
	for (var attachment in mesh_attachments)
		DetachMesh(attachment);
	for (var obj in FindObjects(Find_Container(this)))
	{
		obj->Exit();
		obj->StartFloating();
	}
}

func Construction()
{
	mesh_attachments = [];

	ScheduleCall(this, "AdjustPosition", 1, 0);
	this.MaxCon = RandomX(100, 200);
		
	SetAction("Exist");
	
	AddTimer("Seed", 60 + Random(60));
}

public func MakeAttachment(object parent)
{
	RemoveTimer("Seed");
	SetClrModulation(parent->GetClrModulation());
}

private func SetScale(int scale)
{
	this.MeshTransformation = Trans_Mul(Trans_Scale(scale, scale, scale), Trans_Rotate(Random(360), 0, 1, 0));
}

private func AddFork(int allowed_depth, bool is_root)
{
	if (is_root)
	{
		allowed_depth = 3;
		SetScale(750);
	}
	if (allowed_depth <= 0) return;
	
	var bones = ["Bone.10", "Bone.11", "Bone.13", "Bone.17", "Bone.16"];
	var attach_counts = BoundBy(RandomX(1, 3) + RandomX(-1, 1) + RandomX(-1, 0), 0, 3);
	
	while (attach_counts-- > 0)
	{
		var bone_index = Random(GetLength(bones));
		var bone = bones[bone_index];
		RemoveArrayIndex(bones, bone_index, true);
		
		var other_coral = CreateContents(GetID());
		other_coral->MakeAttachment(this);
		other_coral->SetScale(500);
		var mesh = AttachMesh(other_coral, bone, "Bone.00", Trans_Mul(Trans_Rotate(Random(360), 1, 0, 0)));
		PushBack(mesh_attachments, mesh); 
		
		other_coral->AddFork(allowed_depth - 1);
	}
}

func AdjustPosition()
{
	if (Contained()) return;
	
	var vec = GetSurfaceVector();
	var r = Angle(0, 0, vec[0], vec[1]);
	SetR(r);
	
	// project a bit out of the ground
	r += 180;
	var cnt = 10;
	while (--cnt)
	{
		var stuck = Stuck();
		var dir = 1;
		if (!stuck) dir = -1;
		var old_x = GetX(), old_y = GetY();
		SetPosition(GetX() + dir * Sin(r - 180, 1), GetY() - dir * Cos(r - 180, 1));
		
		if (stuck != Stuck())
		{
			if (stuck)
				SetPosition(old_x, old_y);
			break;
		}			
	}
}


func Seed()
{
	if (GetCon() < this.MaxCon)
	{
		if (!Random(2))
		{
			DoCon(1);
			var cnt = 10, r = GetR();
			while (!Stuck() && -cnt)
			{
				SetPosition(GetX() + Sin(r - 180, 1), GetY() - Cos(r - 180, 1));
			}
				
		}
	}
	else
	if (GetCon() > 50 && !Random(50) && !GetEffect("Seeding", this))
	{
		AddEffect("Seeding", this, 1, 1, this);
	}
	
	if (!Stuck())
	{
		StartFloating();
	}
}

func FxSeedingTimer()
{
	if (!Random(20)) return -1;
	if (!Random(2)) return;
	var seed = CreateObjectAbove(CoralSeed, 0, 0, GetOwner());
	seed->SetClrModulation(GetClrModulation());
}

/* Harvesting */

func IsCrop() { return false; }
func IsPlant(){return true;}

/* Eating */

// the nutritional value will the set to this function once the coral starts floating and
// can be collected by a Clonk.
// Otherwise, fish would eat fixed corals - that's not intended.
func NutritionalValue() { if (this.Collectible) return 5; else return 0; }
