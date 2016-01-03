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
	Sound("Hits::Materials::Wood::DullWoodHit?");
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
	Sound("Objects::Connect");
	wires[count - 1] = wire;
	
	count--;
	
	if (count == 0)
	{
		var pos = clonk->GetItemPos(this);
		ChangeToIgniter();
		clonk->UpdateAttach();
		clonk->OnSlotFull(pos);
	}
	else
	{
		UpdatePicture();
	}

	// Make sure the inventory gets notified of the changes.
	clonk->~OnInventoryChange();
	return true;
}

// Empty this box and turn it into an igniter
public func ChangeToIgniter()
{
	count = 0;
	UpdatePicture();
	ChangeDef(Igniter);
	SetGraphics("Picture", Igniter, 1, GFXOV_MODE_Picture);
	return true;
}

public func SetDynamiteCount(int new_count)
{
	count = BoundBy(new_count, 1, DYNA_MaxCount);
	UpdatePicture();
	// Update inventory if contained in a crew member.
	if (Contained())
		Contained()->~OnInventoryChange();
	return;
}

private func UpdatePicture()
{
	SetGraphics(Format("%d", 6 - count), DynamiteBox, 1, GFXOV_MODE_Picture);
	return;
}

// Do not stack empty dynamite boxes with full ones.
public func CanBeStackedWith(object other)
{
	if (this.count != other.count) return false;
	return inherited(other, ...);
}

// Display the remaining dynamite sticks in menus.
public func GetInventoryIconOverlay()
{
	// Full boxes don't need an overlay. Same for igniters.
	if (count == DYNA_MaxCount || count <= 0) return nil;

	// Overlay the sticks.
	var overlay = 
	{
		Top = "0.1em",
		Bottom = "1.1em",
		back_stripe = 
		{
			Priority = -1,
			Margin = ["0em", "0.3em", "0em", "0.2em"],
			BackgroundColor = RGBa(0, 0, 0, 200)
		}
	};
	
	for (var i = 0; i < count; ++i)
	{
		var left = -i * 4 - 10;
		var left_string = ToEmString(left);
		var stick = 
		{
			Left = Format("100%% %s", left_string),
			Right = Format("100%% %s + 1em", left_string),
			Symbol = Dynamite
		};
		GuiAddSubwindow(stick, overlay);
	}
	
	return overlay;
}

public func OnFuseFinished(object fuse)
{
	SetController(fuse->GetController());
	DoExplode();
}

public func ActivateFuse()
{
	// Activate all fuses.
	for (var obj in FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this)))
		obj->~StartFusing(this);
}

public func DoExplode()
{
	// Activate all fuses.
	ActivateFuse();
	// Explode, calc the radius out of the area of a explosion of a single dynamite times the amount of dynamite
	// This results to 18, 25, 31, 36, and 40
	Explode(Sqrt(18**2*count));
}

protected func Incineration(int caused_by) 
{
	ActivateFuse();
	if (!GetEffect("Fuse", this)) AddEffect("Fuse", this, 100, 1, this);
	Sound("Fire::Fuse");
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


/* Drop connected or fusing boxes */

public func IsDroppedOnDeath(object clonk)
{
	return GetEffect("Fuse", this) || wire;
}



/*-- Properties --*/

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(150, 1, 0, 0), Trans_Rotate(140, 0, 1, 0)), def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local BlastIncinerate = 1;
local ContactIncinerate = 2;
