/*
	Clonk
	Author: Randrian

	The protoganist of the game. Witty and nimble if skillfully controlled ;-)
*/


// selectable by HUD and some performance optimizations for the HUD updates
#include Library_HUDAdapter
// standard controls
#include Library_ClonkControl
#include Library_CarryHeavyControl
// manager for aiming
#include Library_AimManager

#include Clonk_Animations

static const CLONK_MESH_TRANSFORM_SLOT_Turn = 0;             // for turning the clonk
static const CLONK_MESH_TRANSFORM_SLOT_Rotation_Scaling = 1; // for adjusting the rotation while scaling
static const CLONK_MESH_TRANSFORM_SLOT_Translation_Hook = 2; // for adjusting the rotation while scaling
static const CLONK_MESH_TRANSFORM_SLOT_Rotation_Hook = 3;    // for adjusting the rotation while scaling
static const CLONK_MESH_TRANSFORM_SLOT_Rotation_Ladder = 5;  // for adjusting the rotation while climbing a rope ladder
static const CLONK_MESH_TRANSFORM_SLOT_Scale = 6;            // for scaling the size of the clonk
static const CLONK_MESH_TRANSFORM_SLOT_Translation_Dive = 7; // for adjusting the position while diving

// ladder climbing
#include Library_CanClimbLadder
// has magic energy
#include Library_MagicEnergy

// modularization, order is important here
#include Clonk_Generic
#include Clonk_HandDisplay
#include Clonk_InteractionMenu
#include Clonk_Skins
#include Clonk_Sounds


/* Initialization */

func Construction()
{
	_inherited(...);

	AttachBackpack();
}

// Callback from Death() when the Clonk is really really dead
protected func DeathEffects(int killed_by)
{
	_inherited(killed_by,...);

	// Some effects on dying.
	if (!this.silent_death)
	{
		PlaySkinSound("Die*");
		DeathAnnounce();
		
		// When killed by a team member, the other Clonk randomly plays a sound.
		if (!Random(5) && killed_by != NO_OWNER && killed_by != GetOwner() && !Hostile(killed_by, GetOwner()))
		{
			var other_cursor = GetCursor(killed_by);
			if (other_cursor)
				other_cursor->~PlaySoundTaunt();
		}
	}
}

/* --- OC specific object interactions --- */

// Called when an object was dug free.
func DigOutObject(object obj)
{
	// Some materials can only be transported/collected with a bucket.
	if (obj->~IsBucketMaterial())
	{
		// Assume we might already be carrying a filled bucket and the object is stackable, try it!
		var handled = obj->~MergeWithStacksIn(this);
		if (!handled)
		{
			// Otherwise, force into empty buckets!
			var empty_bucket = FindObject(Find_Container(this), Find_Func("IsBucket"), Find_Func("IsBucketEmpty"));
			if (empty_bucket)
			{
				obj->Enter(empty_bucket);
				handled = true;
			}
		}
		// Those objects can only be carried with a bucket, sadly...
		if (!handled)
			obj->RemoveObject();
		// Object might have been removed now.
		return;
	}
	// Collect fragile objects when dug out
	if (obj->GetDefFragile())
		return Collect(obj, nil, nil, true);
	return false;
}

// Building material bridges (like loam bridge)
func Bridge()
{
	var proc = GetProcedure();
	// Clonk must stand on ground. Allow during SCALE; but Clonk won't keep animation if he's not actually near the ground
	if (proc != "WALK" && proc != "SCALE")
		return false;
	if (proc == "WALK")
		SetAction("BridgeStand");
	else
		SetAction("BridgeScale");
	SetComDir(COMD_Stop);
	SetXDir(0);
	SetYDir(0);
	return true;
}

public func IsBridging(){return WildcardMatch(GetAction(), "Bridge*");}

/* --- Status --- */

public func IsPrey() { return true; }


/* --- Backpack --- */

local backpack;

func AttachBackpack()
{
	//Places a backpack onto the clonk
	backpack = AttachMesh(BackpackGraphic, "skeleton_body", "main",       
	                      Trans_Mul(Trans_Rotate(180, 1, 0, 0), Trans_Scale(700, 400, 700), Trans_Translate(4000,-1000, 0)));
}

func RemoveBackpack()
{
	if (backpack)
	{
		DetachMesh(backpack);
		backpack = nil;
	}
	else return false;
}

func SetSkin(int new_skin)
{
	_inherited(new_skin, ...);

	RemoveBackpack(); //add a backpack
	AttachBackpack();
}

/* Act Map */

local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 48,
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
	StartCall = "OnStartRoll",
	AbortCall = "OnAbortRoll",
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
BridgeStand = {
	Prototype = Action,
	Name = "BridgeStand",
	Procedure = DFA_THROW,
	Directions = 2,
	Length = 16,
	Delay = 1,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "BridgeStand",
	StartCall = "StartStand",
	InLiquidAction = "Swim",
},
BridgeScale = {
	Prototype = Action,
	Name = "BridgeScale",
	Procedure = DFA_THROW,
	Directions = 2,
	Length = 16,
	Delay = 1,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "BridgeScale",
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
	Sound = "Clonk::Movement::DivingLoop*",
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
	Attach = CNAT_Bottom,
},
};

local Name = "Clonk";
local Description = "$Description$";
local MaxEnergy = 50000;
local MaxBreath = 720; // Clonk can breathe for 20 seconds under water.
local MaxMagic = 50000;
local JumpSpeed = 400;
local ThrowSpeed = 294;
local ContactIncinerate = 10;

func Definition(def) {
	// Set perspective
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0, 1000, 5000), Trans_Rotate(70, 0, 1, 0)), def);
	
	_inherited(def);
}
