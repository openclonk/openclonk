/*-- Trunk --*/

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

public func IsPlant() { return true; }
public func IsTree() { return true; }

public func IsStanding() { return GetCategory() & C4D_StaticBack; }

public func ChopDown()
{
	// Use Special Vertex Mode 1 (see documentation) so the removed vertex won't come back when rotating the tree.
	SetVertex(3, VTX_Y, 0, 1);
	// Remove the bottom vertex
	RemoveVertex(3);
	// Make pushable
	this.Touchable = 1;
	this.Plane = 300;
	SetCategory(GetCategory()&~C4D_StaticBack);
	if (Stuck())
	{
		var i = 5;
		while(Stuck() && i)
		{
			SetPosition(GetX(), GetY()-1);
			i--;
		}
	}
	// Effect
	Sound("TreeCrack");
	AddEffect("TreeFall", this, 1, 1, nil, Library_Plant);
}

private func MaxDamage()
{
	return 50;
}

protected func Damage()
{
	// Max damage reached -> fall down
	if (GetDamage() > MaxDamage() && IsStanding()) ChopDown();
	if(!IsStanding() && OnFire() && GetDamage() > MaxDamage() * 2)
		BurstIntoAshes();
	_inherited(...);
}

func BurstIntoAshes()
{
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	
	var r = GetR();
	var size = GetCon() * 110 / 100;
	
	for(var cnt = 0; cnt < 10; ++cnt)
	{
		var distance = Random(size/2);
		var x = Sin(r, distance);
		var y = -Cos(r, distance);
		
		for(var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("Ashes", 5, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
}

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;
