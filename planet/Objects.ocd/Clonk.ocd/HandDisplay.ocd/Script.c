/*--
	Logic for displaying items in the hand.
	
	May require further libraries, such as the Clonk inventory.
--*/


func Construction()
{
	_inherited(...);

	this.hand_display = {};
	this.hand_display.hand_mesh = [0, 0];
	this.hand_display.hand_action = 0;
	this.hand_display.both_handed = false;
	this.hand_display.on_back = false;
}


/* Carry items on the clonk */

local iHandMesh;
local fHandAction;
local fBothHanded;

// Mesh attachment handling
local hand_display;
/* Features 4 properties:
	hand_mesh: Array of attachment numbers for items on the clonk.
	hand_action: Determines whether the clonk's hands are busy if items can be used.
		one of three ints: -1, 0 or 1
		-1: no items are drawn on the clonk but they are usable
		 0: items are drawn and can be used
		+1: items are not drawn and cannot be used
	both_handed: The first item held is held with both hands, so draw the second one differently.
	on_back: The first item is currently on the clonk's back, so draw the second one differently (if it also goes on the back).
*/

func OnSelectionChanged(int oldslot, int newslot, bool secondaryslot)
{
	AttachHandItem(secondaryslot);
	return _inherited(oldslot, newslot, secondaryslot);
}
func OnSlotEmpty(int slot)
{
	AttachHandItem(slot);
	return _inherited(slot);
}
func OnSlotFull(int slot)
{
	AttachHandItem(slot);
	return _inherited(slot);
}

public func DetachObject(object obj)
{
	if (this->GetHandItem(0) == obj)
		DetachHandItem(0);
	if (this->GetHandItem(1) == obj)
		DetachHandItem(1);
}

func DetachHandItem(bool secondary)
{
	if (this.hand_display.hand_mesh[secondary])
	{
		DetachMesh(this.hand_display.hand_mesh[secondary]);
		var anim = "Close2Hand";
		if (secondary) anim = "Close1Hand";
		PlayAnimation(anim, CLONK_ANIM_SLOT_Hands + secondary, Anim_Const(0));
	}
	this.hand_display.hand_mesh[secondary] = 0;
}

func AttachHandItem(bool secondary)
{
	DetachHandItem(secondary);
	UpdateAttach();
}

func UpdateAttach()
{
	StopAnimation(GetRootAnimation(CLONK_ANIM_SLOT_Hands));

	if (this.hand_display && this.hand_display.hand_mesh)
	{
		DetachHandItem(0);
		DetachHandItem(1);
	}

	DoUpdateAttach(0);
	DoUpdateAttach(1);
}

func DoUpdateAttach(int sec)
{
	var obj = this->GetHandItem(sec);
	var other_obj = this->GetHandItem(!sec);
	if (!obj) return;

	var attach_mode = obj->~GetCarryMode(this, sec);
	if (attach_mode == CARRY_None) return;

	if (!sec)
	{
		this.hand_display.both_handed = false;
		this.hand_display.on_back = false;
	}

	if (this.hand_display.hand_mesh[sec])
	{
		DetachMesh(this.hand_display.hand_mesh[sec]);
		this.hand_display.hand_mesh[sec] = 0;
	}

	var bone = "main";
	var bone2;
	if (obj->~GetCarryBone())  bone  = obj->~GetCarryBone(this, sec);
	if (obj->~GetCarryBone2()) bone2 = obj->~GetCarryBone2(this, sec);
	else bone2 = bone;
	var nohand = false;
	if (!HasHandAction(sec, 1)) nohand = true;
	
	var trans = obj->~GetCarryTransform(this, sec, nohand, this.hand_display.on_back);

	var pos_hand = "pos_hand2";
	if (sec) pos_hand = "pos_hand1";
	var pos_back = "pos_back1";
	if (sec) pos_back = "pos_back2";
	var closehand = "Close2Hand";
	if (sec) closehand = "Close1Hand";
	var pos_belt = "skeleton_leg_upper.R";
	if (sec) pos_belt = "skeleton_leg_upper.L";

	var special = obj->~GetCarrySpecial(this);
	var special_other;
	if (other_obj) special_other = other_obj->~GetCarrySpecial(this, sec);
	if (special)
	{
		this.hand_display.hand_mesh[sec] = AttachMesh(obj, special, bone, trans);
		attach_mode = 0;
	}

	if (attach_mode == CARRY_Hand)
	{
		if (HasHandAction(sec, 1))
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, CLONK_ANIM_SLOT_Hands + sec, Anim_Const(GetAnimationLength(closehand)));
		}
	}
	else if (attach_mode == CARRY_HandBack)
	{
		if (HasHandAction(sec, 1))
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, CLONK_ANIM_SLOT_Hands + sec, Anim_Const(GetAnimationLength(closehand)));
		}
		else
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
			if (!sec)
				this.hand_display.on_back = true;
		}
	}
	else if (attach_mode == CARRY_HandAlways)
	{
		this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
		PlayAnimation(closehand, CLONK_ANIM_SLOT_Hands + sec, Anim_Const(GetAnimationLength(closehand)));
	}
	else if (attach_mode == CARRY_Back)
	{
		this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
		if (!sec)
			this.hand_display.on_back = true;
	}
	else if (attach_mode == CARRY_BothHands)
	{
		if (sec) return;

		if (HasHandAction(sec, 1) && !sec && !special_other)
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, "pos_tool1", bone, trans);
			PlayAnimation("CarryArms", CLONK_ANIM_SLOT_Hands + sec, Anim_Const(obj->~GetCarryPhase(this)));
			this.hand_display.both_handed = true;
		}
	}
	else if (attach_mode == CARRY_Spear)
	{
		// This is a one sided animation, so switch to back if not in the main hand
		if (HasHandAction(sec, 1) && !sec)
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation("CarrySpear", CLONK_ANIM_SLOT_Hands + sec, Anim_Const(0));
		}
		else
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
			if (!sec)
				this.hand_display.on_back = true;
		}
	}
	else if (attach_mode == CARRY_Blunderbuss)
	{
		if (HasHandAction(sec, 1) && !sec)
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, "pos_hand2", bone, trans);
			PlayAnimation("CarryMusket", CLONK_ANIM_SLOT_Hands + sec, Anim_Const(0), Anim_Const(1000));
			this.hand_display.both_handed = true;
		}
		else
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
			if (!sec)
				this.hand_display.on_back = true;
		}
	}
	else if (attach_mode == CARRY_Grappler)
	{
		if (HasHandAction(sec, 1) && !sec)
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, "pos_hand2", bone, trans);
			PlayAnimation("CarryCrossbow", CLONK_ANIM_SLOT_Hands + sec, Anim_Const(0), Anim_Const(1000));
			this.hand_display.both_handed = true;
		}
		else
		{
			this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
			if (!sec)
				this.hand_display.on_back = true;
		}
	}
	else if (attach_mode == CARRY_Belt)
	{
		// Do some extra transforms for this kind of carrying
		if (trans)
			trans = Trans_Mul(trans, Trans_Rotate(160, 0, 0, 1), Trans_Rotate(5, 0, 1), Trans_Rotate(30, 1), Trans_Translate(-2500, 0, 700), Trans_Scale(700));
		else
			trans = Trans_Mul(Trans_Rotate(160, 0, 0, 1), Trans_Rotate(5, 0, 1), Trans_Rotate(30, 1), Trans_Translate(-2500, 0, 800), Trans_Scale(700));
		this.hand_display.hand_mesh[sec] = AttachMesh(obj, pos_belt, bone, trans);
	}
	else if (attach_mode == CARRY_Sword)
	{
		this.hand_display.hand_mesh[sec] = AttachMesh(obj, "skeleton_hips", bone, trans);
	}
}

public func GetHandMesh(object obj)
{
	if (this->GetHandItem(0) == obj)
		return this.hand_display.hand_mesh[0];
	if (this->GetHandItem(1) == obj)
		return this.hand_display.hand_mesh[1];
}

static const CARRY_None         = 0;
static const CARRY_Hand         = 1;
static const CARRY_HandBack     = 2;
static const CARRY_HandAlways   = 3;
static const CARRY_Back         = 4;
static const CARRY_BothHands    = 5;
static const CARRY_Spear        = 6;
static const CARRY_Blunderbuss  = 7;
static const CARRY_Grappler     = 8;
static const CARRY_Belt         = 9;
static const CARRY_Sword        = 10;

func HasHandAction(sec, just_wear, bool force_landscape_letgo)
{
	// Check if the clonk is currently able to use hands
	// sec: Needs both hands (e.g. CarryHeavy?)
	// just_wear: ???
	// force_landscape_letgo: Also allow from actions where hands are currently grabbing the landscape (scale, hangle)
	if (sec && this.hand_display.both_handed)
		return false;
	if (just_wear)
	{
		if ( HasActionProcedure(force_landscape_letgo) && !this.hand_display.hand_action )
		// For wear purpose this.hand_display.hand_action==-1 also blocks
			return true;
	}
	else
	{
		if ( HasActionProcedure(force_landscape_letgo) && (!this.hand_display.hand_action || this.hand_display.hand_action == -1) )
			return true;
	}
	return false;
}

func HasActionProcedure(bool force_landscape_letgo)
{
	// Check if the clonk is currently in an action where he could use his hands
	// if force_landscape_letgo is true, also allow during scale/hangle assuming the clonk will let go
	var action = GetAction();
	if (action == "Walk" || action == "Jump" || action == "WallJump" || action == "Kneel" || action == "Ride" || action == "BridgeStand")
		return true;
	if (force_landscape_letgo) if (action == "Scale" || action == "Hangle")
		return true;
	return false;
}

public func ReadyToAction(fNoArmCheck)
{
	if (!fNoArmCheck)
		return HasActionProcedure();
	return HasHandAction(0);
}

public func SetHandAction(bool fNewValue)
{
	if (fNewValue > 0)
		this.hand_display.hand_action = 1; // 1 means can't use items and doesn't draw items in hand
	else if (fNewValue < 0)
		this.hand_display.hand_action = -1; // just don't draw items in hand can still use them
	else
		this.hand_display.hand_action = 0;
	UpdateAttach();
}

public func GetHandAction()
{
	if (this.hand_display.hand_action == 1)
		return true;
	return false;
}
