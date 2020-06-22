/**
	Dynamite box
	Contains five dynamite sticks which can be placed and detonated from a distance.
	
	@author: Newton
*/

#include Library_HasExtraSlot

static const DYNA_MaxLength = 500;
static const DYNA_MaxCount  = 5;

/*-- Initialization --*/

public func Initialize(...)
{
	CreateContents(Dynamite, DYNA_MaxCount);
	// Hide it TODO: Remove if the mesh isn't shown if there is a picture set
	this.PictureTransformation = Trans_Scale();
	UpdatePicture();
	return _inherited(...);
}

public func Hit()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
}


/*-- Dynamite stick contents --*/

public func RejectCollect(id def, object obj, ...)
{
	if (obj->GetID() != Dynamite)
		return true;
	// Max five dynamite sticks. However, longer sets of sticks can be constructed by putting more of them in while some are out on the wire
	if (GetDynamiteCount() >= DYNA_MaxCount)
		return true;

	return _inherited(def, obj, ...);
}

public func Ejection(...)
{
	if (GetDynamiteCount() == 0)
	{
		ChangeToIgniter();
	}
	else
	{
		UpdatePicture();
	}
	return _inherited(...);
}

public func ContentsDestruction(...)
{
	Ejection();
	return _inherited(...);
}

public func Collection2(...)
{
	if (GetID() == Igniter)
	{
		ChangeToBox();
	}
	UpdatePicture();
	return _inherited(...);
}

public func GetDynamiteCount()
{
	// Get number of contained dynamite sticks
	return ContentsCount(Dynamite);
}

public func SetDynamiteCount(int new_count)
{
	// Adjust dynamite counts to given amount
	var change = new_count - GetDynamiteCount();
	if (change > 0)
	{
		while (change--)
			CreateContents(Dynamite);
	} else if (change < 0)
	{
		while (change++)
		{
			var dynamite = FindObject(Find_ID(Dynamite), Find_Container(this));
			if (dynamite)
				dynamite->RemoveObject();
		}
	}
}

// Empty this box and turn it into an igniter
public func ChangeToIgniter()
{
	if (GetID() == Igniter) return;
	UpdatePicture();
	ChangeDef(Igniter);
	SetGraphics("Picture", Igniter, 1, GFXOV_MODE_Picture);
	// Update carrier
	var container = Contained();
	if (container)
	{
		var pos = container->~GetItemPos(this);
		container->~UpdateAttach();
		container->~OnSlotFull(pos);
	}
	return true;
}

// Change back into a box
public func ChangeToBox()
{
	if (GetID() == DynamiteBox) return;
	ChangeDef(DynamiteBox);
	UpdatePicture();
	// Update carrier
	if (Contained())
	{
		var pos = Contained()->~GetItemPos(this);
		Contained()->~UpdateAttach();
		Contained()->~OnSlotFull(pos);
	}
	
	return true;
}

// Do not stack empty dynamite boxes with full ones.
public func CanBeStackedWith(object other, ...)
{
	if (GetID() != other->GetID()) return false;
	if (this->GetDynamiteCount() != other->GetDynamiteCount()) return false;
	return inherited(other, ...);
}

// Drop connected or fusing boxes
public func IsDroppedOnDeath(object clonk)
{
	return GetEffect("Fuse", this) || GetLength(FindFuses());
}


/*-- Ignition --*/

public func ActivateFuse()
{
	// Activate all fuses.
	for (var obj in FindFuses())
		obj->~StartFusing(this);
}

public func DoExplode()
{
	// Activate all fuses.
	ActivateFuse();
	// Explode, calc the radius out of the area of a explosion of a single dynamite times the amount of dynamite
	// This results to 18, 25, 31, 36, and 40
	Explode(Sqrt(18**2*GetDynamiteCount()));
}

public func FxFuseTimer(object target, effect, int timer)
{
	CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-20, 10), PV_Random(10, 40), Particles_Glimmer(), 6);
	if (timer > 90)
		DoExplode();
	return FX_OK;
}

public func Incineration(int caused_by)
{
	ActivateFuse();
	if (!GetEffect("Fuse", this)) AddEffect("Fuse", this, 100, 1, this);
	Sound("Fire::Fuse");
	SetController(caused_by);
}

public func Damage(int change, int type, int by_player)
{
	Incinerate(nil, by_player);
}

private func OnFuseFinished(object fuse)
{
	SetController(fuse->GetController());
	DoExplode();
}

public func OnCannonShot(object cannon)
{
	Incinerate(nil, cannon->GetController());
}

public func IsExplosive() { return true; }

/*-- Usage --*/

public func ControlUse(object clonk, int x, int y)
{
	var dynamite = Contents();

	if (!dynamite || dynamite->GetID() != Dynamite)
		return false;

	if (!dynamite->ControlPlace(clonk, x, y, 1))
		return true;

	// Connect with a fuse: Move last wire to dynamite
	var wire = FindFuses()[0];
	if (wire)
		wire->Connect(wire->GetConnectedItem(this), dynamite);
	// Create new wire from box to dynamite
	Fuse->Create(dynamite, this);
	Sound("Objects::Connect");

	return true;
}

private func FindFuses()
{
	// return all fuses connected to this item
	return FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this));
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
			return Trans_Mul(Trans_Translate(0, 3000, 00), Trans_Rotate(-45, 0, 1));
		else
			return Trans_Mul(Trans_Translate(-5000, 3000), Trans_Rotate(-45, 0, 1));
	}
	if (nohand)
		return Trans_Mul(Trans_Translate(0,-3000, -2200), Trans_Rotate(-45, 0, 1));
}

public func GetCarryPhase()
{
	return 450;
}

func UpdatePicture()
{
	SetGraphics(Format("%d", 6 - GetDynamiteCount()), DynamiteBox, 1, GFXOV_MODE_Picture);
	// Update inventory if contained in a crew member.
	if (Contained())
		Contained()->~OnInventoryChange();
}

// Saving: Save custom dynamite stick count
public func SaveScenarioObject(proplist props)
{
	if (!_inherited(props, ...)) return false;
	var dyna_count = this->GetDynamiteCount();
	if (dyna_count != DYNA_MaxCount)
	{
		props->AddCall("Dynamite", this, "SetDynamiteCount", dyna_count);
	}
	return true;
}


func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(150, 1, 0, 0), Trans_Rotate(140, 0, 1, 0)), def);
}

/*-- Properties --*/

public func IsDynamiteBox() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local BlastIncinerate = 1;
local ContactIncinerate = 2;
local NoBurnDecay = true;
local Components = {Wood = 1, Coal = 2, Firestone = 2};
local MaxContentsCount = 5;
