/**
	Dynamite box
	Contains five dynamite sticks which can be placed and detonated from a distance.
	
	@author: Newton
*/

#include Library_HasExtraSlot

static const DYNA_MaxLength = 500;
static const DYNA_MaxCount  = 5;

local count;
local dynamite_sticks;
local wires;
local wire;

/*-- Engine Callbacks --*/

func Initialize()
{
	CreateContents(Dynamite, DYNA_MaxCount);

	count = DYNA_MaxCount;
	dynamite_sticks = [];
	wires = [];
	for (var i = 0; i < count; i++)
	{
		dynamite_sticks[i] = nil;
		wires[i] = nil;
	}

	// Hide it TODO: Remove if the mesh isn't shown if there is a picture set
	this.PictureTransformation = Trans_Scale();
	UpdatePicture();
}

func Hit()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
}

func Incineration(int caused_by)
{
	ActivateFuse();
	if (!GetEffect("Fuse", this)) AddEffect("Fuse", this, 100, 1, this);
	Sound("Fire::Fuse");
	SetController(caused_by);
}

func Damage(int change, int type, int by_player)
{
	Incinerate(nil, by_player);
}

func RejectCollect(id def, object obj)
{
	if (obj->GetID() != Dynamite)
		return true;
	// One dynamite box can only support 5 sticks of dynamite, regardless if these are in the box
	// or already taken out (connected with wires)
	var sticks = ContentsCount(Dynamite);
	for (var i = 0; i < GetLength(wires); i++)
		if (wires[i])
			sticks++;

	if (sticks >= DYNA_MaxCount)
		return true;

	return false;
}

func Ejection()
{
	count--;

	if (count == 0)
	{
		ChangeToIgniter();
		if (Contained())
		{
			var pos = Contained()->~GetItemPos(this);
			Contained()->~UpdateAttach();
			Contained()->~OnSlotFull(pos);
		}
	}
	else
	{
		UpdatePicture();
	}

	// Make sure the inventory gets notified of the changes.
	if (Contained())
		Contained()->~OnInventoryChange();
}

func ContentsDestruction()
{
	Ejection();
}

func Collection2()
{
	if (count == 0 && GetID() == Igniter)
	{
		ChangeToBox();
		if (Contained())
		{
			var pos = Contained()->~GetItemPos(this);
			Contained()->~UpdateAttach();
			Contained()->~OnSlotFull(pos);
		}
	}

	count++;

	UpdatePicture();

	if (Contained())
		Contained()->~OnInventoryChange();
}

/*-- Callbacks --*/

// Do not stack empty dynamite boxes with full ones.
public func CanBeStackedWith(object other)
{
	if (this.count != other.count) return false;
	return inherited(other, ...);
}

// Drop connected or fusing boxes
public func IsDroppedOnDeath(object clonk)
{
	return GetEffect("Fuse", this) || wire;
}

public func OnFuseFinished(object fuse)
{
	SetController(fuse->GetController());
	DoExplode();
}

public func OnCannonShot(object cannon)
{
	Incinerate(nil, cannon->GetController());
}

/*-- Usage --*/

public func SetDynamiteCount(int new_count)
{
	count = BoundBy(new_count, 1, DYNA_MaxCount);
	UpdatePicture();
	// Update inventory if contained in a crew member.
	if (Contained())
		Contained()->~OnInventoryChange();
}

public func ControlUse(object clonk, int x, int y)
{
	var dynamite = Contents();

	if (!dynamite || dynamite->GetID() != Dynamite)
		return false;

	if (!dynamite->ControlUse(clonk, x, y, 1))
		return true;

	if(wire)
		wire->Connect(dynamite_sticks[count], dynamite);

	wire = CreateObject(Fuse);
	wire->Connect(dynamite, this);
	Sound("Objects::Connect");
	wires[count] = wire;

	return true;
}

// Empty this box and turn it into an igniter
public func ChangeToIgniter()
{
	if (GetID() == Igniter) return;

	count = 0;
	UpdatePicture();
	ChangeDef(Igniter);
	SetGraphics("Picture", Igniter, 1, GFXOV_MODE_Picture);
	return true;
}

// Change back into a box
public func ChangeToBox()
{
	if (GetID() == DynamiteBox) return;

	ChangeDef(DynamiteBox);
	UpdatePicture();
	return true;
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

public func FxFuseTimer(object target, effect, int timer)
{
	CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-20, 10), PV_Random(10, 40), Particles_Glimmer(), 6);
	if (timer > 90)
		DoExplode();
	return FX_OK;
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsChemicalProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	if (idle) return CARRY_Back;
	if (clonk->~IsWalking() || clonk->~IsJumping()) return CARRY_BothHands;
	return CARRY_Back;
}

public func GetCarryTransform(object clonk, bool idle, bool nohand, bool second_on_back)
{
	if (idle)
	{
		if (!second_on_back)
			return Trans_Mul(Trans_Translate(0,3000, 00), Trans_Rotate(-45,0,1));
		else
			return Trans_Mul(Trans_Translate(-5000,3000), Trans_Rotate(-45,0,1));
	}
	if (nohand)
		return Trans_Mul(Trans_Translate(0,-3000, -2200), Trans_Rotate(-45,0,1));
}

public func GetCarryPhase()
{
	return 450;
}

func UpdatePicture()
{
	SetGraphics(Format("%d", 6 - count), DynamiteBox, 1, GFXOV_MODE_Picture);
}

// Display the remaining dynamite sticks in menus.
/*public func GetInventoryIconOverlay()
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
}*/

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(150, 1, 0, 0), Trans_Rotate(140, 0, 1, 0)), def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local BlastIncinerate = 1;
local ContactIncinerate = 2;
local Components = {Wood = 1, Coal = 2, Firestone = 2};
local MaxContentsCount = 5;