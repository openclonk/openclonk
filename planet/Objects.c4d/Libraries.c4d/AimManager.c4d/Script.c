/*--
	Aim Manager
	Authors: Randrian

	Manages loading, aiming and shooting
	The weapon has to define an animations set which can have the following entries
	animation_set = {
		AimMode        = AIM_Position, // The aiming mode see constants for explanations
		AnimationAim   = "BowAimArms", // The animation that is used during aiming
		AnimationAim2  = "",           // The animation the first is blended with (only for mode AIM_Weight)
		AnimationLoad  = "BowLoadArms",// The animation during loading
		LoadTime       = 30,           // The duration of the loading animation
		AnimationShoot = nil,          // The animation for shooting
		ShootTime      = 20,           // The duration of shooting
		TurnType       = 1,            // Specify a special turn type (0: turn 120 degrees, 1: turn 180 degrees, 2: turn 220 degrees) see SetTurnType in clonk
		WalkSpeed      = 30000,        // Reduce the walk speed during aiming
		AimSpeed       = 5,            // the speed of aiming default 5° per frame
		AnimationReplacements = [      // Replace some animations with others during load/aim/shoot
			["Walk", "BowWalk"],
			["Walk_Position", 20],       // Walk_Position changes the distance the clonk travels in one cycle of the animation
			["Stand", "BowStand"],
			["Jump", "BowJump"],
			["KneelDown", "BowKneel"]
		],
		// If an animation is nil there is no new animation played and the clonk just waits *Time frames
	};
	The weapon gets the following callbacks
	GetAnimationSet();         // Has to return the animation set
	// The following Stop* Callbacks, have to return true if the clonk doesn't have to be reset (e.g. stating aiming after loading)
	StopLoad(object clonk);    // When the loading animation is over (after LoadTime frames)
	StopAim(object clonk, int angle); // When the clonk has finished loading and aiming at the disired position
	StopShoot(object clonk);   // When the shooting animation is over (after ShootTime frames)
	// When the clonk has during aiming an action where he can't use his hands, the aiming is paused
	OnPauseAim(object clonk);  // Callback when the clonk has to pause the aiming
	OnRestartAim(object clonk);// Callback when the clonk want's to restart aiming. Has to return true if aiming again is possible
	Reset(object clonk);       // Callback when the clonk has been reseted

	The Weapon can use the following functions on the clonk:
	StartLoad(object weapon);    // The weapon wants to start loading (e.g. on ControlUseStart)
	StartAim(object weapon);     // The weapon wants to switch in aim mode (e.g. after loading on StopLoad)
	SetAimPosition(int angle);   // The weapon specifies a new angle (e.g. on  ControlUseHolding)
	StopAim();                   // The weapon wants to shoot (e.g. on ControlUseStop) BUT: the stop is just scheduled! The clonk finished loading or aiming and then really stops aiming!
	StartShoot(object weapon);   // The weapon wants to start the shoot animation (e.g. on StopAim)

	CancelAiming();              // The weapon wants to cancel the aiming (e.g. on ControlUseCancel)
	// When the clonk starts with load/aim/shoot (when StartLoad, StartAim or StartShoot is called) he uses the animation set
	// the means he replaces the animtione specified in AnimationReplacements and adjust the TurnType and Walkspeed (if these are not nil)
--*/

local aim_set;
local aim_weapon;
local aim_animation_index;
local aim_angle;
local aim_stop;
local aim_pause;

local aim_schedule_timer;
local aim_schedule_call;

// Aim modes
static const AIM_Position = 1; // The aim angle alters the position of the animation (0° menas 0% animation position, 180° menas 100% andimation position)
static const AIM_Weight   = 2; // The aim angle alters with blending between AnimationAim (for 0°) and AnimationAim2 (for 180°)

func ReadyToAction() { return _inherited(...); }
func SetHandAction() { return _inherited(...); }
func ReplaceAction() { return _inherited(...); }
func SetTurnType  () { return _inherited(...); }
func SetTurnForced() { return _inherited(...); }

func FxIntAimCheckProcedureStart(target, number, tmp)
{
	if(tmp) return;
	aim_pause = 0;
}

func FxIntAimCheckProcedureTimer()
{
	// check procedure
	if(!ReadyToAction())
	{
		// Already released? cancel
		if(aim_stop)
		{
			CancelAiming();
			return -1;
		}
		if(aim_pause != 1)
		{
			PauseAim();
			aim_schedule_call = nil;
			aim_schedule_timer = nil;
			aim_pause = 1;
		}
	}
	else
	{
		if(aim_pause == 1)
		{
			if(!RestartAim()) // Can't start again? :-( stop
			{
				aim_weapon = nil;
				aim_set = nil;
				return -1;
			}
			aim_pause = 0;
		}
	}
	if(aim_schedule_timer != nil)
	{
		aim_schedule_timer--;
		if(aim_schedule_timer == 0)
		{
			Call(aim_schedule_call);
			aim_schedule_call = nil;
			aim_schedule_timer = nil;
		}
	}
}

func PauseAim()
{
	ResetHands(1);
	aim_weapon->~OnPauseAim(this);
}

func RestartAim()
{
	if(!aim_weapon->~OnRestartAim(this)) return false;
	// Applay the set
	ApplySet(aim_set);
	return true;
}

public func StartLoad(object weapon)
{
	// only if we aren't adjusted to this weapon already
	if(weapon != aim_weapon)
	{
		// Reset old
		if(aim_weapon != nil) aim_weapon->~Reset();
		if(aim_set    != nil) ResetHands();

		// Remember new
		aim_weapon = weapon;
		aim_set = weapon->~GetAnimationSet();

		// Applay the set
		ApplySet(aim_set);

		// Add effect to ensure procedure
		AddEffect("IntAimCheckProcedure", this, 1,  1, this);
	}

	if(aim_set["AnimationLoad"] != nil)
		PlayAnimation(aim_set["AnimationLoad"], 10, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationLoad"]), aim_set["LoadTime"], ANIM_Remove), Anim_Const(1000));

	aim_schedule_timer = aim_set["LoadTime"];
	aim_schedule_call  = "StopLoad";
}

public func StopLoad()
{
	if(!aim_weapon->~StopLoad(this)) // return 1 means the weapon goes on doing something (e.g. start aiming) then we don't reset
		ResetHands();
}

public func StartAim(object weapon)
{
	// only if we aren't adjusted to this weapon already
	if(weapon != aim_weapon)
	{
		// Reset old
		if(aim_weapon != nil) aim_weapon->~Reset();
		if(aim_set    != nil) ResetHands();

		// Remember new
		aim_weapon = weapon;
		aim_set = weapon->~GetAnimationSet();

		// Applay the set
		ApplySet(aim_set);

		// Add effect to ensure procedure
		AddEffect("IntAimCheckProcedure", this, 1,  1, this);
	}

	if(aim_set["AnimationAim"] != nil)
	{
		if(aim_set["AimMode"] == AIM_Position)
			aim_animation_index = PlayAnimation(aim_set["AnimationAim"], 10, Anim_Const(GetAnimationLength(aim_set["AnimationAim"])/2), Anim_Const(1000));
	}

	AddEffect("IntAim", this, 1, 1, this);
}

func FxIntAimTimer(target, number, time)
{
	var angle, delta_angle, length;
	var speed = aim_set["AimSpeed"];
	if(speed == nil) speed = 50;
	if(aim_angle < 0) SetTurnForced(DIR_Left);
	if(aim_angle > 0) SetTurnForced(DIR_Right);
	if(aim_set["AimMode"] == AIM_Position)
	{
		length = GetAnimationLength(aim_set["AnimationAim"]);
		angle = GetAnimationPosition(aim_animation_index)*1800/length;
		delta_angle = BoundBy(Abs(aim_angle*10)-angle, -speed, speed);
		SetAnimationPosition(aim_animation_index, Anim_Const( (angle+delta_angle)*length/1800 ));
	}
	// We have reached the angle and we want to stop
	if(Abs(delta_angle) <= 5 && aim_stop == 1)
	{
		DoStopAim();
		return -1;
	}
}

public func SetAimPosition(int angle)
{
	// Save angle
	aim_angle = angle;
	// Remove scheduled stop if aiming again
	aim_stop = 0;
}

public func StopAim()
{
	// Schedule Stop
	aim_stop = 1;
}

private func DoStopAim()
{
	if(!aim_weapon->~StopAim(this, aim_angle)) // return 1 means the weapon goes on doing something (e.g. start aiming) then we don't reset
		ResetHands();
}

public func StartShoot(object weapon)
{
	// only if we aren't adjusted to this weapon already
	if(weapon != aim_weapon)
	{
		// Reset old
		if(aim_weapon != nil) aim_weapon->~Reset();
		if(aim_set    != nil) ResetHands();

		// Remember new
		aim_weapon = weapon;
		aim_set = weapon->~GetAnimationSet();

		// Applay the set
		ApplySet(aim_set);

		// Add effect to ensure procedure
		AddEffect("IntAimCheckProcedure", this, 1,  1, this);
	}

	if(aim_set["AnimationShoot"] != nil)
		PlayAnimation(aim_set["AnimationShoot"], 10, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot"]), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000));

	aim_schedule_timer = aim_set["ShootTime"];
	aim_schedule_call  = "StopShoot";
}

public func StopShoot()
{
	if(!aim_weapon->~StopShoot(this)) // return 1 means the weapon goes on doing something (e.g. start aiming) then we don't reset
		ResetHands();
}

public func CancelAiming()
{
	ResetHands();
}

public func ApplySet(set)
{
		// Setting the hands as blocked, so that no other items are carried in the hands
	SetHandAction(1);

	if(set["TurnType"] != nil)
		SetTurnType(set["TurnType"], 1);

	if(set["AnimationReplacements"] != nil)
		for(var replace in set["AnimationReplacements"])
			ReplaceAction(replace[0], replace[1]);

	if(set["WalkSpeed"] != nil)
		AddEffect("IntWalkSlow", this, 1, 0, this, 0, set["WalkSpeed"]);
}

public func ResetHands(bool pause)
{
	if(aim_weapon != nil)
	{
		aim_weapon->~Reset(this);

		if(aim_set["AnimationReplacements"] != nil)
			for(var replace in aim_set["AnimationReplacements"])
				ReplaceAction(replace[0], nil);
	}

	aim_stop = 0;
	aim_angle = -90+180*GetDir();

	StopAnimation(GetRootAnimation(10));

	RemoveEffect("IntWalkSlow", this);

	RemoveEffect("IntAim", this);

	SetTurnForced(-1);

	SetTurnType(0, -1);
	SetHandAction(0);

	if(!pause)
	{
		aim_weapon = nil;
		aim_set = nil;

		RemoveEffect("IntAimCheckProcedure", this);
	}
}

/* +++++++++++ Slow walk +++++++++++ */

func FxIntWalkSlowStart(pTarget, iNumber, fTmp, iValue)
{
	if(iValue == nil) iValue = 30000;
	pTarget->SetPhysical("Walk", iValue, PHYS_StackTemporary);
}

func FxIntWalkSlowStop(pTarget, iNumber)
{
	pTarget->ResetPhysical("Walk");
}