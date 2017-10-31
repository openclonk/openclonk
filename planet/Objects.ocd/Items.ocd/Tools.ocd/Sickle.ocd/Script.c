/**
	Sickle
	Used for harvesting (wheat, cotton, ...)

	@author: Clonkonaut
*/

#include Library_Flammable

/*-- Engine Callbacks --*/

private func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !(clonk->IsWalking() || clonk->IsJumping()) || !clonk->HasHandAction();
}

public func ControlUseStart(object clonk, int x, int y)
{
	var arm = "R";
	var carry_bone = "pos_hand2";
	if(clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
	}
	var animation = Format("SwordSlash2.%s", arm);

	// Figure out the kind of animation to use
	var length=15;
	if(clonk->IsJumping())
		animation = Format("SwordJump2.%s",arm);

	clonk->PlayAnimation(animation, CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
	clonk->UpdateAttach();
	Sound("Objects::Weapons::WeaponSwing?", {pitch = 100});
	
	// Search for harvestable plants
	var crop = FindObject(Find_AtRect(AbsX(clonk->GetX()-8), AbsY(clonk->GetY()-10), 16,20), Find_NoContainer(), Find_Func("SickleHarvesting"), Find_Func("IsHarvestable"));
	if (crop)
		crop->Harvest();

	clonk->CancelUse();
	return true;
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	if (idle)
		return CARRY_Belt;

	return CARRY_HandBack;
}

public func GetCarryTransform(object clonk, bool idle, bool nohand)
{
	if (idle)
		return Trans_Mul(Trans_Rotate(-45, 1, 0, 0), Trans_Rotate(180, 0, 0, 1), Trans_Translate(0,0,1000));

	if (nohand)
		return Trans_Mul(Trans_Rotate(45, 0, 1), Trans_Translate(-3000, 0, 4000));

	return Trans_Rotate(90,1,0,0);
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(15, 0, 1, 0), Trans_Rotate(320, 0,0,1)),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 1, Metal = 1};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
local BurnDownTime = 140;