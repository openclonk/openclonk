/*
	Clonk
	Author: Randrian

	The protoganist of the game. Witty and nimble if skillfully controlled ;-)
*/


// selectable by HUD and some performance optimizations for the HUD updates
#include Library_HUDAdapter
// standard controls
#include Library_ClonkControl
// manager for aiming
#include Library_AimManager

#include Clonk_Animations

// un-comment them as soon as the new controls work with context menus etc.^
// Context menu
//#include Library_ContextMenu
// Auto production
//#include Library_AutoProduction

// ladder climbing
#include Library_CanClimbLadder

local pInventory;

/* Initialization */

protected func Construction()
{
	_inherited(...);

	SetAction("Walk");
	SetDir(Random(2));
	// Broadcast for rules
	GameCallEx("OnClonkCreation", this);

	AddEffect("IntTurn", this, 1, 1, this);
	AddEffect("IntEyes", this, 1, 35+Random(4), this);

	AttachBackpack();
	iHandMesh = [0,0];

	SetSkin(0);
}

/* When adding to the crew of a player */

protected func Recruitment(int iPlr)
{
	//The clonk's appearance
	//Player settings can be overwritten for individual Clonks. In your clonk file: "ExtraData=1;Skin=iX" (X = chosen skin)
	var skin = GetCrewExtraData("Skin");
	if (skin == nil) skin = GetPlrClonkSkin(iPlr);
	if(skin != nil) SetSkin(skin);
	else SetSkin(Random(GetSkinCount()));

	// Broadcast for crew
	GameCallEx("OnClonkRecruitment", this, iPlr);
	
	return _inherited(iPlr,...);
}

protected func DeRecruitment(int iPlr) {
	// Broadcast for crew
	GameCallEx("OnClonkDeRecruitment", this, iPlr);
	
	return _inherited(iPlr,...);
}


protected func ControlCommand(szCommand, pTarget, iTx, iTy, pTarget2, Data)
{
	// RejectConstruction Callback for building via Drag'n'Drop form a building menu
	// TODO: Check if we still need this with the new system
	if(szCommand == "Construct")
	{
		if(Data->~RejectConstruction(iTx - GetX(), iTy - GetY(), this) )
		{
			return 1;
		}
	}
	// No overloaded command
	return 0;
}


/* Transformation */

public func Redefine(idTo)
{
	// save data of activity
	var phs=GetPhase(),act=GetAction();
	// Transform
	ChangeDef(idTo);
	// restore action
	var chg=SetAction(act);
	if (!chg) SetAction("Walk");
	if (chg) SetPhase(phs);
	// Done
	return 1;
}

/* Events */

protected func CatchBlow()
{
	if (GetAction() == "Dead") return;
	if (!Random(5)) Hurt();
}
	
protected func Hurt()
{
	if(gender == 0)
		Sound("Hurt?");
	else
		Sound("FHurt?");
}
	
protected func Grab(object pTarget, bool fGrab)
{
	Sound("Grab");
}

protected func Get()
{
	Sound("Grab");
}

protected func Put()
{
	Sound("Grab");
}

protected func Death(int killed_by)
{
	// this must be done first, before any goals do funny stuff with the clonk
	_inherited(killed_by,...);
	
	// Info-broadcasts for dying clonks.
	GameCallEx("OnClonkDeath", this, killed_by);
	
	// The broadcast could have revived the clonk.
	if (GetAlive())
		return;
	
	// Some effects on dying.
	if(gender == 0)
		Sound("Die");
	else
		Sound("FDie");
	CloseEyes(1);
	
	DeathAnnounce();
	return;
}

protected func Destruction()
{
	_inherited(...);
	// If the clonk wasn't dead yet, he will be now.
	if (GetAlive())
		GameCallEx("OnClonkDeath", this, GetKiller());
	// If this is the last crewmember, do broadcast.
	if (GetCrew(GetOwner()) == this)
	if (GetCrewCount(GetOwner()) == 1)
		// Only if the player is still alive and not yet elimnated.
			if (GetPlayerName(GetOwner()))
				GameCallEx("RelaunchPlayer", GetOwner(), GetKiller());
	return;
}

protected func DeepBreath()
{
	Sound("Breath");
}

protected func CheckStuck()
{
	// Prevents getting stuck on middle vertex
	if(!GetXDir()) if(Abs(GetYDir()) < 5)
		if(GBackSolid(0, 3))
			SetPosition(GetX(), GetY() + 1);
}

public func Eat(object food)
{
	if(GetProcedure() == "WALK")
	{
		DoEnergy(food->NutritionalValue());
		food->RemoveObject();
		Sound("Munch?");
		SetAction("Eat");
	}
}

/* Status */

// TODO: Make this more sophisticated, readd turn animation and other
// adaptions
public func IsClonk() { return true; }

public func IsJumping(){return WildcardMatch(GetAction(), "*Jump*");}
public func IsWalking(){return GetProcedure() == "WALK";}

/* Carry items on the clonk */

local iHandMesh;
local fHandAction;
local fBothHanded;

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
	if(GetHandItem(0) == obj)
		DetachHandItem(0);
	if(GetHandItem(1) == obj)
		DetachHandItem(1);
}

func DetachHandItem(bool secondary)
{
	if(iHandMesh[secondary])
		DetachMesh(iHandMesh[secondary]);
	iHandMesh[secondary] = 0;
}

func AttachHandItem(bool secondary)
{
	DetachHandItem(secondary);
	UpdateAttach();
}

func UpdateAttach()
{
	StopAnimation(GetRootAnimation(6));
	DoUpdateAttach(0);
	DoUpdateAttach(1);
}

func DoUpdateAttach(bool sec)
{
	var obj = GetHandItem(sec);
	var other_obj = GetHandItem(!sec);
	if(!obj) return;
	var iAttachMode = obj->~GetCarryMode(this);
	if(iAttachMode == CARRY_None) return;

	if(iHandMesh[sec])
	{
		DetachMesh(iHandMesh[sec]);
		iHandMesh[sec] = 0;
	}

	var bone = "main";
	var bone2;
	if(obj->~GetCarryBone())  bone  = obj->~GetCarryBone(this);
	if(obj->~GetCarryBone2()) bone2 = obj->~GetCarryBone2(this);
	else bone2 = bone;
	var nohand = 0;
	if(!HasHandAction(sec, 1)) nohand = 1;
	var trans = obj->~GetCarryTransform(this, sec, nohand);

	var pos_hand = "pos_hand2";
	if(sec) pos_hand = "pos_hand1";
	var pos_back = "pos_back1";
	if(sec) pos_back = "pos_back2";
	var closehand = "Close2Hand";
	if(sec) closehand = "Close1Hand";

	if(!sec) fBothHanded = 0;

	var special = obj->~GetCarrySpecial(this);
	var special_other;
	if(other_obj) special_other = other_obj->~GetCarrySpecial(this);
	if(special)
	{
		iHandMesh[sec] = AttachMesh(obj, special, bone, trans);
		iAttachMode = 0;
	}

	if(iAttachMode == CARRY_Hand)
	{
		if(HasHandAction(sec, 1))
		{
			iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
		}
		else
			; // Don't display
	}
	else if(iAttachMode == CARRY_HandBack)
	{
		if(HasHandAction(sec, 1))
		{
			iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_HandAlways)
	{
		iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
		PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
	}
	else if(iAttachMode == CARRY_Back)
	{
		iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_BothHands)
	{
		if(sec) return;
		if(HasHandAction(sec, 1) && !sec && !special_other)
		{
			iHandMesh[sec] = AttachMesh(obj, "pos_tool1", bone, trans);
			PlayAnimation("CarryArms", 6, Anim_Const(obj->~GetCarryPhase(this)), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			; // Don't display
	}
	else if(iAttachMode == CARRY_Spear)
	{
		if(HasHandAction(sec, 1) && !sec)
		{
			PlayAnimation("CarrySpear", 6, Anim_Const(0), Anim_Const(1000));
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_Musket)
	{
		if(HasHandAction(sec, 1) && !sec)
		{
			iHandMesh[sec] = AttachMesh(obj, "pos_hand2", bone, trans);
			PlayAnimation("CarryMusket", 6, Anim_Const(0), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
	else if(iAttachMode == CARRY_Grappler)
	{
		if(HasHandAction(sec, 1) && !sec)
		{
			iHandMesh[sec] = AttachMesh(obj, "pos_hand2", bone, trans);
			PlayAnimation("CarryCrossbow", 6, Anim_Const(0), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone2, trans);
	}
}//AttachMesh(DynamiteBox, "pos_tool1", "main", Trans_Translate(0,0,0));

public func GetHandMesh(object obj)
{
	if(GetHandItem(0) == obj)
		return iHandMesh[0];
	if(GetHandItem(1) == obj)
		return iHandMesh[1];
}

static const CARRY_None         = 0;
static const CARRY_Hand         = 1;
static const CARRY_HandBack     = 2;
static const CARRY_HandAlways   = 3;
static const CARRY_Back         = 4;
static const CARRY_BothHands    = 5;
static const CARRY_Spear        = 6;
static const CARRY_Musket       = 7;
static const CARRY_Grappler     = 8;

func HasHandAction(sec, just_wear)
{
	if(sec && fBothHanded)
		return false;
	if(just_wear)
	{
		if( HasActionProcedure() && !fHandAction ) // For wear purpose fHandAction==-1 also blocks
			return true;
	}
	else
	{
		if( HasActionProcedure() && (!fHandAction || fHandAction == -1) )
			return true;
	}
	return false;
}

func HasActionProcedure()
{
	var action = GetAction();
	if (action == "Walk" || action == "Jump" || action == "WallJump" || action == "Kneel" || action == "Ride")
		return true;
	return false;
}

public func ReadyToAction(fNoArmCheck)
{
	if(!fNoArmCheck)
		return HasActionProcedure();
	return HasHandAction(0);
}

public func SetHandAction(bool fNewValue)
{
	if(fNewValue > 0)
		fHandAction = 1; // 1 means can't use items and doesn't draw items in hand
	else if(fNewValue < 0)
		fHandAction = -1; // just don't draw items in hand can still use them
	else
		fHandAction = 0;
	UpdateAttach();
}

public func GetHandAction()
{
	if(fHandAction == 1)
		return true;
	return false;
}

/* Mesh transformations */

local mesh_transformation_list;

func SetMeshTransformation(array transformation, int layer)
{
	if(!mesh_transformation_list) mesh_transformation_list = [];
	if(GetLength(mesh_transformation_list) < layer)
		SetLength(mesh_transformation_list, layer+1);
	mesh_transformation_list[layer] = transformation;
	var all_transformations = nil;
	for(var trans in mesh_transformation_list)
	{
		if(!trans) continue;
		if(all_transformations)
			all_transformations = Trans_Mul(trans, all_transformations);
		else
			all_transformations = trans;
	}
	SetProperty("MeshTransformation", all_transformations);
}

/* Backpack */

local backpack;

func AttachBackpack()
{
	//Places a backpack onto the clonk
	backpack = AttachMesh(BackpackGraphic, "skeleton_body", "main",       
	                      Trans_Mul(Trans_Rotate(180,0,1,0), Trans_Scale(700,700,400), Trans_Translate(0,4000,1000)));
}

func RemoveBackpack()
{
	if(backpack)
	{
		DetachMesh(backpack);
		backpack = nil;
	}
	else return false;
}

// calback from engine
func OnMaterialChanged(int new, int old)
{
	if(!GetAlive()) return;
	var newdens = GetMaterialVal("Density","Material",new);
	var olddens = GetMaterialVal("Density","Material",old);
	var newliquid = (newdens >= C4M_Liquid) && (newdens < C4M_Solid);
	var oldliquid = (olddens >= C4M_Liquid) && (olddens < C4M_Solid);
	// into water
	if(newliquid && !oldliquid)
		AddEffect("Bubble", this, 1, 52, this);
	// out of water
	else if(!newliquid && oldliquid)
		RemoveEffect("Bubble", this);
}

func FxBubbleTimer(pTarget, effect, iTime)
{
	if(GBackLiquid(0,-5))
	{
		var iRot = GetSwimRotation();
		Bubble(1, +Sin(iRot, 9), Cos(iRot, 9));
	}
}

func QueryCatchBlow(object obj)
{
	var r=0;
	var e=0;
	var i=0;
	while(e=GetEffect("*Control*", this, i++))
	{
		if(EffectCall(this, e, "QueryCatchBlow", obj))
		{
			r=true;
			break;
		}
		
	}
	if(r) return r;
	return _inherited(obj, ...);
}

local gender;

func SetSkin(int skin)
{
	//Adventurer
	if(skin == 0)
	{	SetGraphics();
		gender = 0;	}

	//Steampunk
	if(skin == 1)
	{	SetGraphics("Steampunk");
		gender = 1; }

	//Alchemist
	if(skin == 2)
	{	SetGraphics("Alchemist");
		gender = 0;	}
	
	//Farmer
	if(skin == 3)
	{	SetGraphics("Farmer");
		gender = 1;	}

	RemoveBackpack(); //add a backpack
	AttachBackpack();
	SetAction("Jump"); //refreshes animation

	return skin;
}
func GetSkinCount() { return 4; }

/* Act Map */

local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 22,
	Speed = 200,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartWalk",
	AbortCall = "StopWalk",
//	InLiquidAction = "Swim",
},
Stand = {
	Prototype = Action,
	Name = "Stand",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartStand",
	InLiquidAction = "Swim",
},
Kneel = {
	Prototype = Action,
	Name = "Kneel",
	Procedure = DFA_KNEEL,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
//	StartCall = "StartKneel",
	InLiquidAction = "Swim",
},
Roll = {
	Prototype = Action,
	Name = "Roll",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartRoll",
	NextAction = "Walk",
	InLiquidAction = "Swim",
},
Scale = {
	Prototype = Action,
	Name = "Scale",
	Procedure = DFA_SCALE,
	Speed = 60,
	Accel = 20,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 20,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 0,
	StartCall = "StartScale",
	AbortCall = "StopScale",
},
Tumble = {
	Prototype = Action,
	Name = "Tumble",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 0,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 40,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Tumble",
	ObjectDisabled = 1,
	InLiquidAction = "Swim",
	StartCall = "StartTumble",
	AbortCall = "StopTumble",
	EndCall = "CheckStuck",
},
Dig = {
	Prototype = Action,
	Name = "Dig",
	Procedure = DFA_DIG,
	Speed = 50,
	Directions = 2,
	Length = 16,
	Delay = 0,//15*3*0,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Dig",
	StartCall = "StartDigging",
	AbortCall = "StopDigging",
	DigFree = 11,
//	InLiquidAction = "Swim",
	Attach = CNAT_Left | CNAT_Right | CNAT_Bottom,
},
Bridge = {
	Prototype = Action,
	Name = "Bridge",
	Procedure = DFA_THROW,
	Directions = 2,
	Length = 16,
	Delay = 1,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Bridge",
	InLiquidAction = "Swim",
},
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = 96,
	Accel = 7,
	Decel = 9,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 80,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 0,
//	SwimOffset = -5,
	StartCall = "StartSwim",
	AbortCall = "StopSwim",
},
Hangle = {
	Prototype = Action,
	Name = "Hangle",
	Procedure = DFA_HANGLE,
	Speed = 48,
	Accel = 20,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 100,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 3,
	StartCall = "StartHangle",
	AbortCall = "StopHangle",
	InLiquidAction = "Swim",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 14,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
	StartCall = "StartJump",
},
WallJump = {
	Prototype = Action,
	Name = "WallJump",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 14,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
},
Dive = {
	Prototype = Action,
	Name = "Dive",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 16,
	Directions = 2,
	Length = 8,
	Delay = 4,
	X = 0,
	Y = 160,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Hold",
	ObjectDisabled = 1,
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	X = 0,
	Y = 240,
	Wdt = 8,
	Hgt = 20,
	Length = 1,
	Delay = 0,
	NextAction = "Hold",
	StartCall = "StartDead",
	NoOtherAction = 1,
	ObjectDisabled = 1,
},
Ride = {
	Prototype = Action,
	Name = "Ride",
	Procedure = DFA_ATTACH,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 128,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartRiding",
	AbortCall = "StopRiding",
	InLiquidAction = "Swim",
},
Push = {
	Prototype = Action,
	Name = "Push",
	Procedure = DFA_PUSH,
	Speed = 200,
	Accel = 100,
	Directions = 2,
	Length = 8,
	Delay = 15,
	X = 128,
	Y = 140,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Push",
	StartCall = "StartPushing",
	AbortCall = "StopPushing",
	InLiquidAction = "Swim",
},
Build = {
	Prototype = Action,
	Name = "Build",
	Directions = 2,
	Length = 8,
	Delay = 15,
	X = 128,
	Y = 140,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Build",
	InLiquidAction = "Swim",
	Attach = CNAT_Bottom,
},
HangOnto = {
	Prototype = Action,
	Name = "HangOnto",
	Procedure = DFA_ATTACH,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 128,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartHangOnto",
	AbortCall = "AbortHangOnto",
	InLiquidAction = "Swim",
},
Eat = {
	Prototype = Action,
	Name = "Eat",
	Procedure = DFA_NONE,
	Directions = 2,
	Length = 1,
	Delay = 45,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartEat",
	NextAction = "Walk",
	InLiquidAction = "Swim",
	Attach=CNAT_Bottom,
},
};
local Name = "Clonk";
local MaxEnergy = 50000;
local MaxBreath = 252; // Clonk can breathe for 7 seconds under water.
local JumpSpeed = 400;
local ThrowSpeed = 294;
local NoBurnDecay = 1;
local ContactIncinerate = 10;

func Definition(def) {
	// Set perspective
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,1000,5000), Trans_Rotate(70,0,1,0)), def);

	_inherited(def);
}
