/**
	Dynamite box
	Contains five dynamite sticks which can be placed and detonated from a distance. 
	
	@author Newton
*/

static const DYNA_MaxLength = 500;
static const DYNA_MaxCount  = 5;

local count;
local dynamite_sticks;
local wires;
local wire;

public func Initialize()
{
	count = DYNA_MaxCount;
	dynamite_sticks = [];
	wires = [];
	for (var i = 0; i < count; i++)
	{
		dynamite_sticks[i] = nil;
		wires[i] = nil;
	}

	this.PictureTransformation = Trans_Scale(); // Hide it TODO: Remove if the mesh isn't shown if there is a picture set
	UpdatePicture();
	return;
}

private func Hit()
{
	Sound("DullWoodHit?");
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }
public func GetCarryPhase() { return 450; }

public func ControlUse(object clonk, int x, int y)
{
	var dynamite = dynamite_sticks[count - 1] = CreateContents(Dynamite);
	if (!dynamite->ControlUse(clonk, x, y, 1))
	{
		dynamite->RemoveObject();
		return true;
	}
	if(wire)
		wire->Connect(dynamite_sticks[count], dynamite);

	wire = CreateObject(Fuse);
	wire->Connect(dynamite, this);
	Sound("Connect");
	wires[count - 1] = wire;
	
	count--;

	UpdatePicture();
	
	if (count == 0)
	{
		var pos = clonk->GetItemPos(this);
		ChangeDef(Igniter);
		SetGraphics("Picture", Igniter, 1, GFXOV_MODE_Picture);
		clonk->UpdateAttach();
		clonk->OnSlotFull(pos);
	}

	return true;
}

private func UpdatePicture()
{
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	SetGraphics(Format("%d", count), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
	SetGraphics(Format("%d", 6 - count), DynamiteBox, 1, GFXOV_MODE_Picture);
	return;
}

public func OnFuseFinished()
{
	DoExplode();
}

public func DoExplode()
{
	// Activate all fuses.
	for (var obj in FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this)))
		obj->~StartFusing(this);
	// Explode, calc the radius out of the area of a explosion of a single dynamite times the amount of dynamite
	// This results to 18, 25, 31, 36, and 40
	Explode(Sqrt(18**2*count));
}

protected func Incineration(int caused_by) 
{
	AddEffect("Fuse", this, 100, 1, this);
	Sound("Fuse");
	SetController(caused_by);
	return;
}

protected func Damage(int change, int type, int by_player)
{
	Incinerate(nil, by_player);
	return;
}

public func FxFuseTimer(object target, effect, int timer)
{
	CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-20, 10), PV_Random(10, 40), Particles_Glimmer(), 6);
	if (timer > 90)
		DoExplode();
	return FX_OK;
}

public func IsTool() { return true; }
public func IsChemicalProduct() { return true; }


/*-- Properties --*/

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(150, 1, 0, 0), Trans_Rotate(140, 0, 1, 0)), def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
local BlastIncinerate = 1;
local ContactIncinerate = 2;
