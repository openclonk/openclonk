/**
	Bucket
	Transport earth from one spot to another to form the landscape.
	Replaces the old earth chunks in their behaviour.

	@author: Clonkonaut
*/

// Uses an extra-slot to store and display material.
#include Library_HasExtraSlot

#include Library_Flammable

/*-- Engine Callbacks --*/

protected func Hit()
{
	Sound("Hits::BucketHit?");
}

public func RejectCollect(id def, object obj)
{
	if (!obj->~IsBucketMaterial()) return true;
	// Can only contain one stackable object.
	if (Contents() && Contents(0)->~IsStackable()) return true;
	return false;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	return true;
}

/*-- Callbacks --*/

// Can collect IsBucketMaterial?
public func IsBucket() { return true; }

// When trying to put into a producer that can't take the item but its contents, just transfer the contents.
public func MergeWithStacksIn(object to_building, ...)
{
	if (to_building && to_building->~IsProducer() && !to_building->~IsCollectionAllowed(this))
	{
		var i = ContentsCount(), contents, num_collected = 0;
		while (i--)
			if (contents = Contents(i))
				if (to_building->Collect(contents))
					++num_collected;
		// Return if contents transfer was successful.
		if (num_collected > 0) return true;
	}
	return _inherited(to_building, ...);
	 
}

/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction(false, false, true);
}

public func ControlUse(object clonk, int iX, int iY)
{
	var angle = Angle(0, 0, iX, iY);

	// spill bucket
	if (IsBucketFilled())
	{
		Spill(angle);
		if (Contents(0) && !Contents(0)->IsInfiniteStackCount()) EmptyBucket();
		PlayBucketAnimation(clonk);
		return true;
	}
	else
	{
		Message("$UsageHotTip$");
	}

	return true;
}

public func EmptyBucket()
{
	var i = ContentsCount();
	while (--i >= 0)
		if (Contents(0)) Contents(0)->RemoveObject();
}

public func IsBucketFilled()
{
	return ContentsCount();
}

public func IsBucketEmpty()
{
	return !IsBucketFilled();
}

func PlayBucketAnimation(object clonk)
{
	// animation only available for jumping and walking
	if(!clonk->IsJumping() && !clonk->IsWalking())
		return;

	var arm, carry_bone;
	if(clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
	}
	else
	{
		arm = "R";
		carry_bone = "pos_hand2";
	}
	
	// figure out the kind of animation to use
	var length=15;
	var animation;
	if(clonk->IsJumping())
		animation = Format("SwordJump2.%s",arm);
	else
		animation = Format("SwordSlash2.%s", arm);

	clonk->PlayAnimation(animation, CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
	clonk->UpdateAttach();
}

func Spill(int angle)
{
	var obj = Contents(0);
	if (!obj) return;
	var material_name = obj->GetMaterialName();
	var material_amount = obj->GetMaterialAmount();
	var stack_count = obj->~GetStackCount();
	if (stack_count > 1) material_amount *= stack_count;

	// This will only spray out the material because no solid base to stick it on was found
	CastPXS(material_name, material_amount, 20, 0,0, angle, 15);
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode()
{
	return CARRY_HandBack;
}

public func GetCarryTransform(object clonk, bool idle, bool nohand)
{
	if (nohand) return Trans_Mul(Trans_Rotate(180, 0, 1, 0), Trans_Translate(3000));
	return Trans_Mul(Trans_Rotate(-90, 0, 1, 0), Trans_Translate(3500, 0, -4000));
}

protected func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(500,400,0), Trans_Rotate(-10,1,0,0), Trans_Rotate(30,0,1,0), Trans_Rotate(+25,0,0,1), Trans_Scale(1100)),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ForceFreeHands = true;
local Components = {Wood = 1, Metal = 1};
local ExtraSlotFilter = "IsBucketMaterial";
local BlastIncinerate = 30;
local MaterialIncinerate = true;
local BurnDownTime = 140;