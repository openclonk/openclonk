/*
	Clonk
	Author: Randrian

	The protoganist of the game. Witty and nimble if skillfully controled ;-)
*/


// selectable by HUD and some performance optimizations for the HUD updates
#include Library_HUDAdapter
// standard controls
#include Library_ClonkControl
// manager for aiming
#include Library_AimManager

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
}



/* When adding to the crew of a player */

protected func Recruitment(int iPlr)
{
	//The clonk's appearance
	var skin = GetCrewExtraData("Skin");
	if(skin) SetSkin(skin);

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
		Sound("Hurt*");
	else
		Sound("FHurt*"); //female 'ouch' sounds TODO :/
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
	
	if(gender == 0)
		Sound("Die");
	else
		Sound("FDie");

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

/* Status */

// TODO: Make this more sophisticated, readd turn animation and other
// adaptions
public func IsClonk() { return true; }

public func IsJumping(){return GetProcedure() == "FLIGHT";}
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
	if(GetItem(0) == obj)
		DetachHandItem(0);
	if(GetItem(1) == obj)
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
	if(!iHandMesh) iHandMesh = [0,0];
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
	var obj = GetItem(sec);
	var other_obj = GetItem(!sec);
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
	if(GetItem(0) == obj)
		return iHandMesh[0];
	if(GetItem(1) == obj)
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
	if(GetAction() == "Walk" || GetAction() == "Jump" || GetAction() == "Kneel" || GetAction() == "Ride")
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
	var all_transformations = 0;
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


/* Turn */
local iTurnAction;
local iTurnAction2;
local iTurnAction3;

local iTurnKnot1;
local iTurnKnot2;

local iLastTurn;
local iTurnSpecial;

local turn_forced;

static const CLONK_TurnTime = 10;

func SetTurnForced(int dir)
{
	turn_forced = dir+1;
}

func FxIntTurnStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	effect.dir = GetDirection();
	var iTurnPos = 0;
	if(effect.dir == COMD_Right) iTurnPos = 1;

	effect.curr_rot = 24;
	effect.var1 = 0;
	effect.rot = 25;
	effect.var5 = -1;
	SetTurnType(0);
}

func FxIntTurnTimer(pTarget, effect, iTime)
{
	// Check wether the clonk wants to turn (Not when he wants to stop)
	var iRot = effect.rot;
	if( (effect.dir != GetDirection() || effect.var5 != iLastTurn) && GetAction() != "Jump")
	{
		effect.dir = GetDirection();
		if(effect.dir == COMD_Right)
		{
			if(iLastTurn == 0)
				iRot = 180-25;
			if(iLastTurn == 1)
				iRot = 180;
		}
		else
		{
			if(iLastTurn == 0)
				iRot = 25;
			if(iLastTurn == 1)
				iRot = 0;
		}
		// Save new ComDir
		effect.dir = GetDirection();
		effect.var5 = iLastTurn;
		// Notify effects
//		ResetAnimationEffects();
	}
	if(iRot != effect.curr_rot)
	{
		effect.curr_rot += BoundBy(iRot-effect.curr_rot, -18, 18);
		SetMeshTransformation(Trans_Rotate(effect.curr_rot, 0, 1, 0), 0);
//		SetProperty("MeshTransformation", Trans_Rotate(iNumber.curr_rot, 0, 1, 0));
	}
	effect.rot = iRot;
	return;
}

public func GetTurnPhase()
{
	var iEff = GetEffect("IntTurn", this);
	var iRot = iEff.curr_rot;
	if(iLastTurn == 0)
		return (iRot-25)*100/130;
	if(iLastTurn == 1)
		return iRot*100/180;
	return GetAnimationPosition(iTurnAction)*100/GetAnimationLength("TurnRoot120");
}

local iLastTurn;
local iTurnSpecial;

func SetTurnType(iIndex, iSpecial)
{
	if(iSpecial != nil && iSpecial != 0)
	{
		if(iSpecial == 1) // Start a turn that is forced to the clonk and overwrites the normal action's turntype
			iTurnSpecial = 1;
		if(iSpecial == -1) // Reset special turn (here the iIndex is ignored)
		{
			iTurnSpecial = 0;
			SetTurnType(iLastTurn);
			return;
		}
	}
	else
	{
		// Standart turn? Save and do nothing if we are blocked
		iLastTurn = iIndex;
		if(iTurnSpecial) return;
	}
	return;
}

// For test purpose
public func TurnFront()
{
	SetAnimationPosition(iTurnAction, Anim_Const(500));
}

func GetDirection()
{
	// Are we forced to a special direction?
	if(turn_forced)
	{
		if(turn_forced == 1) return COMD_Left;
		if(turn_forced == 2) return COMD_Right;
	}
	// Get direction from ComDir
	if(GetAction() != "Scale")
	{
		if(ComDirLike(GetComDir(), COMD_Right)) return COMD_Right;
		else if(ComDirLike(GetComDir(), COMD_Left)) return COMD_Left;
	}
	// if ComDir hasn't a direction, use GetDir
	if(GetDir()==DIR_Right) return COMD_Right;
	else return COMD_Left;
}

/* Animation Manager */

local PropAnimations;

public func ReplaceAction(string action, byaction)
{
	if(PropAnimations == nil) PropAnimations = CreatePropList();
	if(byaction == nil || byaction == 0)
	{
		SetProperty(action, nil, PropAnimations);
		ResetAnimationEffects();
		return true;
	}
/*	if(GetAnimationLength(byaction) == nil)
	{
		Log("ERROR: No animation %s in Definition %s", byaction, GetID()->GetName());
		return false;
	}*/
	if(GetType(byaction) == C4V_Array)
	{
		var old = GetProperty(action, PropAnimations);
		SetProperty(action, byaction, PropAnimations);
		if(GetType(old) == C4V_Array)
		{
			if(ActualReplace == nil) return true;
			if(old[0] == byaction[0] && old[1] == byaction[1])
			{
				var i = 0;
				for (test in ActualReplace)
				{
					if(test && test[0] == action)
						break;
					i++;
				}
				if(i < GetLength(ActualReplace))
					SetAnimationWeight(ActualReplace[i][1], Anim_Const(byaction[2]));
				return true;
			}
		}
	}
	SetProperty(action, byaction, PropAnimations);
//	if(ActualReplace != nil)
//		SetAnimationWeight(ActualReplace, Anim_Const(byaction[2]));
	ResetAnimationEffects();
	return true;
}

public func ResetAnimationEffects()
{
	if(GetEffect("IntWalk", this))
		EffectCall(this, GetEffect("IntWalk", this), "Reset");
	if(GetAction() == "Jump")
		StartJump();
}

local ActualReplace;

public func PlayAnimation(string animation, int index, array position, array weight, int sibling)
{
	if(!ActualReplace) ActualReplace = [];
	ActualReplace[index] = nil;
	if(PropAnimations != nil)
		if(GetProperty(animation, PropAnimations) != nil)
		{
			var replacement = GetProperty(animation, PropAnimations);
			if(GetType(replacement) == C4V_Array)
			{
				var animation1 = inherited(replacement[0], index, position, weight);
				var animation2 = inherited(replacement[1], index, position, Anim_Const(500), animation1);
				var animationKnot = animation2 + 1;
				ActualReplace[index] = [animation, animationKnot];
				SetAnimationWeight(animationKnot, Anim_Const(replacement[2]));
				return animation1;
			}
			else
				animation = GetProperty(animation, PropAnimations);
		}
	return inherited(animation, index, position, weight, sibling, ...);
}

public func GetAnimationLength(string animation)
{
	var replacement;
	if(PropAnimations != nil)
		if(replacement = GetProperty(animation, PropAnimations))
		{
			if(GetType(replacement) == C4V_Array)
				animation = replacement[0];
			else
				animation = replacement;
		}
	return inherited(animation, ...);
}

/* Eyes */
func FxIntEyesTimer(target, effect, time)
{
	if(!Random(4))
		AddEffect("IntEyesClosed", this, 10, 6, this);
}

func FxIntEyesClosedStart(target, effect, tmp)
{
	CloseEyes(1);
}

func FxIntEyesClosedStop(target, effect, reason, tmp)
{
	CloseEyes(-1);
}

local closed_eyes;
func CloseEyes(iCounter)
{
	StopAnimation(GetRootAnimation(3));
//	PlayAnimation("CloseEyes", 6, Anim_Linear(0, 0, GetAnimationLength("CloseEyes")/2, 3, ANIM_Hold), Anim_Const(1000));
	closed_eyes += iCounter;
	if(closed_eyes >= 1)
		PlayAnimation("CloseEyes" , 3, Anim_Linear(0, 0, GetAnimationLength("CloseEyes")/2, 3, ANIM_Hold), Anim_Const(1000));
//		SetMeshMaterial("Clonk_Body_EyesClosed");
	else
		PlayAnimation("CloseEyes" , 3, Anim_Linear(GetAnimationLength("CloseEyes")/2, GetAnimationLength("CloseEyes")/2, GetAnimationLength("CloseEyes"), 3, ANIM_Remove), Anim_Const(1000));
//		SetMeshMaterial("Clonk_Body");
}

/* Walking backwards */
func SetBackwardsSpeed(int value)
{
	BackwardsSpeed = value;
	UpdateBackwardsSpeed();
}

local BackwardsSpeed;
local Backwards;

func UpdateBackwardsSpeed()
{
	if(GetComDir() != GetDirection() && Backwards != 1 && BackwardsSpeed != nil)
	{
		AddEffect("IntWalkBack", this, 1, 0, this, 0, BackwardsSpeed);
		Backwards = 1;
	}
	if( (GetComDir() == GetDirection() && Backwards == 1) || BackwardsSpeed == nil)
	{
		RemoveEffect("IntWalkBack", this);
		Backwards = nil;
	}
}

func FxIntWalkBackStart(pTarget, effect, fTmp, iValue)
{
	if(iValue == nil) iValue = 84;
	pTarget->PushActionSpeed("Walk", iValue);
}

func FxIntWalkBackStop(pTarget, effect)
{
	pTarget->PopActionSpeed("Walk");
}

/* Walk */

static const Clonk_WalkInside = "Inside";
static const Clonk_WalkStand = "Stand";
static const Clonk_WalkWalk  = "Walk";
static const Clonk_WalkRun   = "Run";
static Clonk_IdleActions;

func StartWalk()
{
	if(Clonk_IdleActions == nil)
		Clonk_IdleActions = [["IdleLookAround", 60], ["IdleHandwatch", 100], ["IdleScratch", 70], ["IdleStrech", 100], ["IdleShoe", 120], ["IdleShoeSole", 200], ["IdleHandstrech", 100]];
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func GetCurrentWalkAnimation()
{
	if(Contained())
	{
		if(Contained()->GetCategory() & C4D_Structure)
		{
			return Clonk_WalkInside;
		}
		return;
	}
	else SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,1000,5000), Trans_Rotate(70,0,1,0)), this);
	var velocity = Distance(0,0,GetXDir(),GetYDir());
	if(velocity < 1) return Clonk_WalkStand;
	if(velocity < 10) return Clonk_WalkWalk;
	return Clonk_WalkRun;
}

func GetWalkAnimationPosition(string anim, int pos)
{
	var dir = -1;
	if(GetDirection() == COMD_Right) dir = +1;
	if(PropAnimations != nil)
		if(GetProperty(Format("%s_Position", anim), PropAnimations))
		{
			var length = GetAnimationLength(anim);
			if(GetProperty(anim, PropAnimations)) length = GetAnimationLength(GetProperty(anim, PropAnimations));
			return Anim_X(pos, 0, length, GetProperty(Format("%s_Position", anim), PropAnimations)*dir);
		}
	// TODO: Choose proper starting positions, depending on the current
	// animation and its position: For Stand->Walk or Stand->Run, start
	// with a frame where one of the clonk's feets is on the ground and
	// the other one is in the air. For Walk->Run and Run->Walk, fade to
	// a state where its feets are at a similar position (just taking
	// over previous animation's position might work, using
	// GetAnimationPosition()). Walk->Stand is arbitrary I guess.
	// First parameter of Anim_Linear/Anim_AbsX is initial position.
	// Movement synchronization might also be tweaked somewhat as well.
	if(anim == Clonk_WalkInside)
		return Anim_Const(0);
	if(anim == Clonk_WalkStand)
		return Anim_Linear(pos, 0, GetAnimationLength(anim), 35, ANIM_Loop);
	else if(anim == Clonk_WalkWalk)
		return Anim_X(pos, 0, GetAnimationLength(anim), 20*dir);
	else if(anim == Clonk_WalkRun)
		return Anim_X(pos, 0, GetAnimationLength(anim), 50*dir);
}

func FxIntWalkStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	// Always start in Stand for now... should maybe fade properly from previous animation instead
	var anim = "Stand";  //GetCurrentWalkAnimation();
	effect.animation_name = anim;
	effect.animation_id = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.idle_animation_time = 0;

	effect.idle_time = 0; // Idle counter
	effect.idle_offset = Random(300); // Random offset for idle time
	// Update carried items
	UpdateAttach();
	// Set proper turn
	SetTurnType(0);
}

func FxIntWalkTimer(pTarget, effect)
{

	// Test Waterlevel
	if(GBackLiquid(0, -5))
	{
		SetAction("Swim");
		if(GetComDir() == COMD_Left)
			SetComDir(COMD_UpLeft);
		else if(GetComDir() == COMD_Right)
			SetComDir(COMD_UpRight);
		else if(GetComDir() != COMD_Down && GetComDir() != COMD_DownLeft && GetComDir() != COMD_DownRight)
			SetComDir(COMD_Up);
		return;
	}
	if(BackwardsSpeed != nil)
		UpdateBackwardsSpeed();
	if(effect.idle_animation_time)
	{
		effect.idle_animation_time--;
		if(effect.idle_animation_time == 0)
			effect.animation_name = nil;
	}
	var anim = GetCurrentWalkAnimation();
	if(anim != effect.animation_name && !effect.var4)
	{
		effect.animation_name = anim;
		effect.idle_time = 0;
		effect.animation_id = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim, 0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	// The clonk has to stand, not making a pause animation yet and not doing other actions with the hands (e.g. loading the bow)
	else if(anim == Clonk_WalkStand && !GetHandAction())
	{
		if(!effect.idle_animation_time)
		{
			effect.idle_time++;
			if(effect.idle_time > 300+effect.idle_offset)
			{
				effect.idle_time = 0;
				effect.idle_offset = Random(300);
				var rand = Random(GetLength(Clonk_IdleActions));
				PlayAnimation(Clonk_IdleActions[rand][0], 5, Anim_Linear(0, 0, GetAnimationLength(Clonk_IdleActions[rand][0]), Clonk_IdleActions[rand][1], ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				effect.idle_animation_time = Clonk_IdleActions[rand][1]-5;
			}
		}
	}
	else
	{
		effect.idle_time = 0;
		if(effect.idle_animation_time)
		{
			effect.animation_name = nil;
			effect.idle_animation_time = 0;
		}
	}
}

func FxIntWalkReset(pTarget, effect)
{
	effect.animation_name = nil;
}

func StartStand()
{
	PlayAnimation(Clonk_WalkStand, 5, GetWalkAnimationPosition(Clonk_WalkStand), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
}

/* Scale */

func StartScale()
{
	if(!GetEffect("IntScale", this))
		AddEffect("IntScale", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopScale()
{
	if(GetAction() != "Scale") RemoveEffect("IntScale", this);
}

func CheckPosition(int off_x, int off_y)
{
	var free = 1;
	SetPosition(GetX()+off_x, GetY()+off_y);
	if(Stuck()) free = 0;
	SetPosition(GetX()-off_x, GetY()-off_y);
	return free;
}

func CheckScaleTop()
{
	// Test whether the clonk has reached a top corner
	if(GBackSolid(-8+16*GetDir(),-8)) return false;
	if(!CheckPosition(-7*(-1+2*GetDir()),-17)) return false;
	return true;
}

func FxIntScaleStart(target, effect, tmp)
{
	if(tmp) return;
	effect.animation_id = PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.animation_mode = 0;
}

func FxIntScaleTimer(target, number, time)
{
	if(GetAction() != "Scale") return;
	// When the clonk reaches the top play an extra animation
	if(CheckScaleTop())
	{
		// If the animation is not already set
		var dist = 0;
		while(!GBackSolid(-8+16*GetDir(),dist-8) && dist < 10) dist++;
		dist *= 100;
		dist += GetY(100)-GetY()*100;
		if(number.animation_mode != 1)
		{
			number.animation_id = PlayAnimation("ScaleTop", 5, Anim_Const(GetAnimationLength("ScaleTop")*dist/1000), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			number.animation_mode = 1;
			number.var2 = COMD_Up;
		}
		this.dist = dist;
		SetAnimationPosition(number.animation_id, Anim_Const(GetAnimationLength("ScaleTop")*dist/1000));
		// The animation's graphics has to be shifet a bit to adjust to the clonk movement
		var pos = GetAnimationPosition(number.animation_id);
		//var percent = pos*1000/GetAnimationLength("ScaleTop");
		var offset_list = [[0,0], [0,-1], [-1,-2], [-2,-3], [-2,-5], [-2,-7], [-4,-8], [-6,-10], [-7,-9], [-8,-8]];
		var offset = offset_list[dist/100-1];
		var rot = 0;
		if(dist/100-1 > 5) rot = 5*dist/100-25;
		SetScaleRotation(0, -offset[0]*(-1+2*GetDir())*1000, offset[1]*1000, -rot*(-1+2*GetDir()), 0, 1);
	}
	else if(!GBackSolid(-10+20*GetDir(), 8))
	{
		if(number.animation_mode != 2)
		{
			var pos = GetAnimationPosition(number.animation_id);
			number.animation_id = PlayAnimation("ScaleHands" , 5, Anim_Y(pos, GetAnimationLength("ScaleHands"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			number.animation_id2 = PlayAnimation("ScaleHands2", 5, Anim_Y(pos, GetAnimationLength("ScaleHands2"), 0, 15), Anim_Const(1000), number.animation_id);
			number.animation_id2++;
			number.animation_mode = 2;
		}
		SetAnimationWeight(number.animation_id2, Anim_Const(Cos(time*2, 500)+500));
		SetScaleRotation(0);
	}
	// If not play the normal scale animation
	else if(number.animation_mode != 0)
	{
		if(number.var3)
		{
			SetComDir(COMD_Stop);
			number.var3 = 0;
		}
		var pos = 0;
		if(number.animation_mode == 2) pos = GetAnimationPosition(number.animation_id);
		number.animation_id = PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		number.animation_mode = 0;
		SetScaleRotation(0);
	}
	if(number.animation_mode == 0)
	{
		var x, x2;
		var y = -7, y2 = 8;
		var dir = -1+2*GetDir();
		for(x = 0; x < 10; x++)
			if(GBackSolid(x*dir, y)) break;
		for(x2 = 0; x2 < 10; x2++)
			if(GBackSolid(x2*dir, y2)) break;
		var angle = Angle(x2, y2, x, y)*dir;
		var mid = (x+x2)*1000/2 - 5000 - this.Off;
		this.TestAngle = angle;
		this.TestMid = mid;
		SetScaleRotation(angle, mid*dir);
	}
}

func FxIntScaleRotTimer(target, eff, time)
{
	eff.oldR += BoundBy(eff.r-eff.oldR, -3, 3);
	eff.oldX += BoundBy(eff.xoff-eff.oldX, -500, 500);
	eff.oldY += BoundBy(eff.yoff-eff.oldY, -500, 500);
	var turnx = -1000;
	var turny = 10000;
	SetMeshTransformation(Trans_Mul(Trans_Translate(eff.oldX-turnx, eff.oldY-turny), Trans_Rotate(eff.oldR,0,0,1), Trans_Translate(turnx, turny)), 1);
}

func SetScaleRotation (int r, int xoff, int yoff, int rotZ, int turny, int instant) {
	if(r < -180) r += 360;
	if(r > 180) r -= 360;
	// set matrix values
	var turnx = -1000;
	var turny = 10000;
	if(instant)
	{
		RemoveEffect("IntScaleRot", this);
		SetMeshTransformation(Trans_Mul(Trans_Translate(xoff-turnx, yoff-turny), Trans_Rotate(r,0,0,1), Trans_Translate(turnx, turny), Trans_Rotate(rotZ, 0, 1, 0)), 1);
	}
	else
	{
		var eff = GetEffect("IntScaleRot", this);
		if(!eff)
			eff = AddEffect("IntScaleRot", this, 1, 1, this);
		eff.r = r;
		eff.xoff = xoff;
		eff.yoff = yoff;
	}
}

func FxIntScaleStop(target, number, reason, tmp)
{
	if(tmp) return;
	// Set the animation to stand without blending! That's cause the animation of Scale moves the clonkmesh wich would result in a stange blend moving the clonk around while blending
/*	if(number.animation_mode == 1) PlayAnimation(Clonk_WalkStand, 5, GetWalkAnimationPosition(Clonk_WalkStand), Anim_Const(1000));
	// Finally stop if the user has scheduled a stop
	if(number.var3) SetComDir(COMD_Stop);*/
	// and reset the transform
	SetScaleRotation(0);
//	SetObjDrawTransform(1000, 0, 0, 0, 1000, 0);
}

/* Jump */

func StartJump()
{
	//which leg to kick off with?
	var side = "R";
	if(Random(2)) side = "L";

	//Normal forward jump
	if(Abs(GetXDir()) >= 1)
	PlayAnimation(Format("Jump.%s",side), 5, Anim_Linear(0, 0, GetAnimationLength("Jump.L"), 8*5, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	//Walk kick jump
	if(GetEffect("WallKick",this))
	{
		SetAction("WallJump");
		var side = "L";
		if(GetDir() == DIR_Left) side = "R";
		PlayAnimation(Format("JumpWall.%s", side), 5, Anim_Linear(0, 0, GetAnimationLength("JumpWall.L"), 8*5, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	//Upwards jump
	else if(GetXDir() == 0)
	{
		PlayAnimation(Format("JumpUp.%s", side), 5, Anim_Linear(0, 0, GetAnimationLength("JumpUp.L"), 8*5, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}

	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
	//Dive jump
	var flight = SimFlight(AbsX(GetX()), AbsY(GetY()), GetXDir()*2, GetYDir()*2, 25); //I have no clue why the dirs must be doubled... but it seems to fix it
			if(GBackLiquid(flight[0] - GetX(), flight[1] - GetY()) && GBackLiquid(flight[0] - GetX(), flight[1] + GetDefHeight() / 2 - GetY()))
			{
				PlayAnimation("JumpDive", 5, Anim_Linear(0, 0, GetAnimationLength("JumpDive"), 60, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				return 1;
			}

		if(!GetEffect("Fall", this))
			AddEffect("Fall",this,1,1,this);
		RemoveEffect("WallKick",this);
}

func FxFallEffect(string new_name, object target)
{
	// reject more than one fall effects.
	if(new_name == "Fall") return -1;
}

func FxFallTimer(object target, effect, int timer)
{
	//falling off ledges without jumping results in fall animation
	if(timer == 2 && GetYDir() > 1)
	{
		PlayAnimation("FallShort", 5, Anim_Linear(0, 0, GetAnimationLength("FallShort"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
	if(timer == 2 && GetYDir() < 1)
	{
		Sound("Rustle*.ogg");
	}

	if(GetYDir() > 55 && GetAction() == "Jump")
	{
		PlayAnimation("FallLong", 5, Anim_Linear(0, 0, GetAnimationLength("FallLong"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		return -1;
	}
	if(GetAction() != "Jump")
		return -1;
}

/* Hangle */

/* Replaces the named action by an instance with a different speed */
func PushActionSpeed(string action, int n)
{
	if (ActMap == this.Prototype.ActMap)
		ActMap = { Prototype = this.Prototype.ActMap };
	ActMap[action] = { Prototype = ActMap[action], Speed = n };
	if (this.Action == ActMap[action].Prototype)
		this.Action = ActMap[action];
}

/* Resets the named action to the previous one */
func PopActionSpeed(string action, int n) {
	// FIXME: This only works if PushActionSpeed and PopActionSpeed are the only functions manipulating the ActMap
	if (this.Action == ActMap[action])
		this.Action = ActMap[action].Prototype;
	ActMap[action] = ActMap[action].Prototype;
}

func StartHangle()
{
/*	if(Clonk_HangleStates == nil)
		Clonk_HangleStates = ["HangleStand", "Hangle"];*/
	if(!GetEffect("IntHangle", this))
		AddEffect("IntHangle", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopHangle()
{
	if(GetAction() != "Hangle") RemoveEffect("IntHangle", this);
}

func FxIntHangleStart(pTarget, effect, fTmp)
{
	effect.hangle_speed = ActMap.Hangle.Speed;
	PushActionSpeed("Hangle", effect.hangle_speed);
	if(fTmp) return;

	// is_moving: whether the clonk is currently moving or not (<=> current animation is Hangle or HangleStand)
	// request_stop: Player requested the clonk to stop
	// facing_front: Whether the HangleStand animation is shown front-facing or back-facing

	effect.animation_id = PlayAnimation("HangleStand", 5, Anim_Linear(0, 0, 2000, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

}

func FxIntHangleStop(pTarget, effect, iReasonm, fTmp)
{
	PopActionSpeed("Hangle");
	if(fTmp) return;
}

func FxIntHangleTimer(pTarget, effect, iTime)
{
	// (TODO: Instead of effect.is_moving we should be able
	// to query the current animation... maybe via a to-be-implemented
	// GetAnimationName() engine function.

	// If we are currently moving
	if(effect.is_moving)
	{
		// Use a cosine-shaped movement speed (the clonk only moves when he makes a "stroke")
		var iSpeed = 50-Cos(GetAnimationPosition(effect.animation_id)/10*360*2/1000, 50);
		ActMap.Hangle.Speed = effect.hangle_speed*iSpeed/50;

		// Exec movement animation (TODO: Use Anim_Linear?)
		var position = GetAnimationPosition(effect.animation_id);
		position += (effect.hangle_speed*5/48*1000/(14*2));

		SetAnimationPosition(effect.animation_id, Anim_Const(position % GetAnimationLength("Hangle")));

		// Continue movement, if the clonk still has momentum
		if(GetComDir() == COMD_Stop && iSpeed>10)
		{
			// Make it stop after the current movement
			effect.request_stop = 1;

			if(GetDir())
				SetComDir(COMD_Right);
			else
				SetComDir(COMD_Left);
		}
		// Stop movement if the clonk has lost his momentum
		else if(iSpeed <= 10 && (GetComDir() == COMD_Stop || effect.request_stop))
		{
			effect.request_stop = 0;
			SetComDir(COMD_Stop);

			// and remeber the pose (front or back)
			if(GetAnimationPosition(effect.animation_id) > 2500 && GetAnimationPosition(effect.animation_id) < 7500)
				effect.facing_front = 1;
			else
				effect.facing_front = 0;

			// Change to HangleStand animation
			var begin = 4000*effect.facing_front;
			var end = 2000+begin;
			effect.animation_id = PlayAnimation("HangleStand", 5, Anim_Linear(begin, begin, end, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			effect.is_moving = 0;
		}
	}
	else
	{
		// We are currently not moving
		if(GetComDir() != COMD_Stop)
		{
			// Switch to move
			effect.is_moving = 1;
			// start with frame 100 or from the back hanging pose frame 600
			var begin = 10*(100 + 500*effect.facing_front);
			effect.animation_id = PlayAnimation("Hangle", 5, Anim_Const(begin), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
	}
}

/* Swim */

func StartSwim()
{
/*	if(Clonk_SwimStates == nil)
		Clonk_SwimStates = ["SwimStand", "Swim", "SwimDive", "SwimTurn", "SwimDiveTurn", "SwimDiveUp", "SwimDiveDown"];*/
	if(!GetEffect("IntSwim", this))
		AddEffect("IntSwim", this, 1, 1, this);
	SetVertex(1,VTX_Y,-4,2);
}

func StopSwim()
{
	if(GetAction() != "Swim") RemoveEffect("IntSwim", this);
	SetVertex(1,VTX_Y,-7,2);
}

func FxIntSwimStart(pTarget, effect, fTmp)
{
	if(fTmp) return;

	effect.animation_name = "SwimStand";
	effect.animation = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Set proper turn type
	SetTurnType(0);
	// Update carried items
	UpdateAttach();
	SetAnimationWeight(iTurnKnot2, Anim_Const(1000));
}

func FxIntSwimTimer(pTarget, effect, iTime)
{
	var iSpeed = Distance(0,0,GetXDir(),GetYDir());

	// TODO: Smaller transition time between dive<->swim, keep 15 for swimstand<->swim/swimstand<->dive

	// Play stand animation when not moving
	if(Abs(GetXDir()) < 1 && !GBackSemiSolid(0, -5))
	{
		if (GetContact(-1) & CNAT_Bottom)
		{
			SetAction("Walk");
			return -1;
		}
		if(effect.animation_name != "SwimStand")
		{
			effect.animation_name = "SwimStand";
			effect.animation = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
		SetAnimationWeight(iTurnKnot1, Anim_Const(0));
	}
	// Swimming
	else if(!GBackSemiSolid(0, -5))
	{
		var percent = GetAnimationPosition(GetRootAnimation(5))*200/GetAnimationLength("Swim");
		percent = (percent%100);
		if( percent < 40 )
		{
			for(var i = 0; i < 2; i++)
				CreateParticle("Splash", (-1+2*GetDir())*7+RandomX(-5,5), -4, (RandomX(-5,5)-(-1+2*GetDir())*4)/4, -2, RandomX(30,50), RGB(240+Random(10),240+Random(10),255));
			if(iTime%5 == 0)
			{
				var particle_name = "WaveLeft";
				if( GetDir() == 1 ) particle_name = "WaveRight";
				var color = GetAverageTextureColor(GetTexture(0, 0));
				CreateParticle(particle_name, (0), -4, (RandomX(-5,5)-(-1+2*GetDir())*4)/4, 0, 100, color, this, 1);
			}
			Sound("Splash*");
		}
		// Animation speed by X
		if(effect.animation_name != "Swim")
		{
			effect.animation_name = "Swim";
			// TODO: Determine starting position from previous animation
			PlayAnimation("Swim", 5, Anim_AbsX(0, 0, GetAnimationLength("Swim"), 25), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
		SetAnimationWeight(iTurnKnot1, Anim_Const(0));
	}
	// Diving
	else
	{
		if(effect.animation_name != "SwimDive")
		{
			effect.animation_name = "SwimDive";
			// TODO: Determine starting position from previous animation
			effect.animation2 = PlayAnimation("SwimDiveUp", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveUp"), 40, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
			effect.animation3 = PlayAnimation("SwimDiveDown", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveDown"), 40, ANIM_Loop), Anim_Const(500), effect.animation2);
			effect.animation = effect.animation3 + 1;

			// TODO: This should depend on which animation we come from
			// Guess for SwimStand we should fade from 0, otherwise from 90.
			effect.rot = 90;
		}

		if(iSpeed)
		{
			var iRot = Angle(-Abs(GetXDir()), GetYDir());
			effect.rot += BoundBy(iRot - effect.rot, -4, 4);
		}

		// TODO: Shouldn't weight go by sin^2 or cos^2 instead of linear in angle?
		var weight = 1000*effect.rot/180;
		SetAnimationWeight(effect.animation, Anim_Const(1000 - weight));
		SetAnimationWeight(iTurnKnot1, Anim_Const(1000 - weight));
	}
}

func GetSwimRotation()
{
	var effect = GetEffect("IntSwim", this);
	if(!effect) return 0;
	return effect.rot*(-1+2*(GetDirection()==COMD_Right));
}

func Hit(int iXSpeed, int iYSpeed)
{
	if(iYSpeed < 450) return;
	if(GetAction() != "Walk") return;

	//Roll :D
	if(GetComDir() == COMD_Right && GetDir() == 1 || GetComDir() == COMD_Left && GetDir() == 0)
	{
		if(Abs(iXSpeed) > 130 && iYSpeed <= 80 * 10)
			SetAction("Roll");
		else
			DoKneel();
	}
	else
	{
		DoKneel();
	}
}

func DoKneel()
{
	var iKneelDownSpeed = 18;

	SetXDir(0);
	SetAction("Kneel");
	Sound("RustleLand.ogg");
	PlayAnimation("KneelDown", 5, Anim_Linear(0, 0, GetAnimationLength("KneelDown"), iKneelDownSpeed, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	ScheduleCall(this, "EndKneel", iKneelDownSpeed, 1);
	return 1;
}

func EndKneel()
{
	if(GetAction() != "Roll") SetAction("Walk");
}

local rolllength;
local rolldir;

//rollp
func StartRoll()
{	
	Sound("Roll.ogg");
	if(GetDir() == 1) rolldir = 1;
	else
		rolldir = -1;

	rolllength = 22;
	PlayAnimation("KneelRoll", 5, Anim_Linear(0, 0, 1500, rolllength, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	AddEffect("Rolling", this, 1, 1, this);
}

func FxRollingTimer(object target, int num, int timer)
{
	if(GetContact(-1)) SetXDir(23 * rolldir);

	//Hacky fun
	var i = 3;
	while(GBackSolid(rolldir, 9) && i != 0)
	{
		SetPosition(GetX(),GetY() - 1);
		i--;
	}

	if(timer > rolllength)
	{
		SetAction("Walk");
		rolldir = nil;
		return -1;
	}
}

func StartDigging()
{
	if(!GetEffect("IntDig", this))
		AddEffect("IntDig", this, 1, 1, this);
}

func StopDigging()
{
	if(GetAction() != "Dig") RemoveEffect("IntDig", this);
}

func FxIntDigStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	effect.var1 = PlayAnimation("Dig", 5, Anim_Linear(0, 0, GetAnimationLength("Dig"), 36, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Update carried items
	UpdateAttach();

	// Sound
	Sound("Dig*");

	// Set proper turn type
	SetTurnType(0);
}

func FxIntDigTimer(pTarget, effect, iTime)
{
	if(iTime % 36 == 0)
	{
		Sound("Dig*");
	}
	if( (iTime-18) % 36 == 0 ||  iTime > 35)
	{
		var noDig = 1;
		for(var pShovel in FindObjects(Find_ID(Shovel), Find_Container(this)))
			if(pShovel->IsDigging()) noDig = 0;
		if(noDig)
		{
			SetAction("Walk");
			SetComDir(COMD_Stop);
			return -1;
		}
	}
}

// custom throw
public func ControlThrow(object target, int x, int y)
{
	// standard throw after all
	if (!x && !y) return false;
	if (!target) return false;

	var throwAngle = Angle(0,0,x,y);

	// walking (later with animation: flight, scale, hangle?) and hands free
	if ( (GetProcedure() == "WALK" || GetAction() == "Jump" || GetAction() == "Dive")
		&& this->~HasHandAction())
	{
		if (throwAngle < 180) SetDir(DIR_Right);
		else SetDir(DIR_Left);
		//SetAction("Throw");
		this->~SetHandAction(1); // Set hands ocupied
		AddEffect("IntThrow", this, 1, 1, this, 0, target, throwAngle);
		return true;
	}
	// attached
	if (GetProcedure() == "ATTACH")
	{
		//SetAction("RideThrow");
		return DoThrow(target,throwAngle);
	}
	return false;
}

func FxIntThrowStart(target, effect, tmp, targetobj, throwAngle)
{
	var iThrowTime = 16;
	if(tmp) return;
	PlayAnimation("ThrowArms", 10, Anim_Linear(0, 0, GetAnimationLength("ThrowArms"), iThrowTime), Anim_Const(1000));
	effect.targetobj = targetobj;
	effect.angle = throwAngle;
}

func FxIntThrowTimer(target, effect, time)
{
	// cancel throw if object does not exist anymore
	if(!effect.targetobj)
		return -1;
	var iThrowTime = 16;
	if(time == iThrowTime*8/15)
		DoThrow(effect.targetobj, effect.angle);
	if(time >= iThrowTime)
		return -1;
}

func FxIntThrowStop(target, effect, reason, tmp)
{
	if(tmp) return;
	StopAnimation(GetRootAnimation(10));
	this->~SetHandAction(0);
}
func StartDead()
{
	PlayAnimation("Dead", 5, Anim_Linear(0, 0, GetAnimationLength("Dead"), 20, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
}

func StartTumble()
{
	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	CloseEyes(1);
	PlayAnimation("Tumble", 5, Anim_Linear(0, 0, GetAnimationLength("Tumble"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
	AddEffect("IntTumble", this, 1, 0);
}

func StopTumble()
{
	if(GetAction() != "Tumble")
	{
		RemoveEffect("IntTumble", this);
		CloseEyes(-1);
	}
}

/* Riding */
public func StartRiding()
{
	if(!GetEffect("IntRiding", this))
		AddEffect("IntRiding", this, 1, 0, this);
}

public func AttachTargetLost()
{
	if(GetEffect("IntRiding", this))
		RemoveEffect("IntRiding", this);
}

public func StopRiding()
{
	if(GetEffect("IntRiding", this))
		RemoveEffect("IntRiding", this);
}

func FxIntRidingStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	var pMount = GetActionTarget();
	if(!pMount) return -1;
	if(pMount->~OnMount(this)) // Notifiy the mount, that the clonk is mounted (it should take care, that the clonk get's attached!
	{
		// if mount has returned true we should be attached
		// So make the clonk object invisible
		effect.vis = GetProperty("Visibility");
		SetProperty("Visibility", VIS_None);
	}
	else effect.vis = -1;
	effect.mount = pMount;
}

func FxIntRidingStop(pTarget, effect, fTmp)
{
	if(fTmp) return;
	if(effect.vis != -1)
		SetProperty("Visibility", effect.vis);

	var pMount = effect.mount;
	if(pMount)
		pMount->~OnUnmount(this);
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

func StartPushing()
{
//	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	PlayAnimation("Push", 5, Anim_AbsX(0, 0, GetAnimationLength("Push"), 20), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
//	AddEffect("IntTumble", this, 1, 0);
}

protected func StopPushing()
{
	return _inherited(...);
}

func StartHangOnto()
{
//	if(GetEffect("IntTumble", this)) return;
	// Close eyes
	PlayAnimation("OnRope", 5, Anim_Linear(0, 0, GetAnimationLength("OnRope"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
//	AddEffect("IntTumble", this, 1, 0);
}

protected func AbortHangOnto()
{
	if (GetActionTarget(0))
		GetActionTarget(0)->~HangOntoLost(this);
	return;
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
	//Save to player's crew-member file which skin they are using
	SetCrewExtraData("Skin", skin);

	//Adventurer
	if(skin == 0)
	{	SetGraphics();
		gender = 0;	}

	//Steampunk
	if(skin == 1)
	{	SetGraphics(nil, Skin_Steampunk);
		gender = 1; }

	//Alchemist
	if(skin == 2)
	{	SetGraphics(nil, Skin_Alchemist);
		gender = 0;	}

	RemoveBackpack(); //add a backpack
	AttachBackpack();
	SetAction("Jump"); //refreshes animation

	return skin;
}

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
	Attach = CNAT_MultiAttach,
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
};
local Name = "Clonk";
local MaxEnergy = 50000;
local MaxBreath = 252; // Clonk can breathe for 7 seconds under water.
local JumpSpeed = 400;
local ThrowSpeed = 294;

func Definition(def) {
	// Set perspective
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,1000,5000), Trans_Rotate(70,0,1,0)), def);

	_inherited(def);
}
