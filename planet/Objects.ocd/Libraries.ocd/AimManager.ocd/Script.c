/*--
	Aim Manager
	Authors: Randrian

	Manages loading, aiming and shooting
	The weapon has to define an animations set which can have the following entries
	animation_set = {
		AimMode         = AIM_Position, // The aiming mode see constants for explanations
		AnimationAim    = "BowAimArms", // The animation that is used during aiming
		AnimationAim2   = "",           // The animation the first is blended with (only for mode AIM_Weight)
		AimTime         = nil,          // Time for the aim animation cycle (only for mode AIM_Weight)
		AnimationLoad   = "BowLoadArms",// The animation during loading
		LoadTime        = 30,           // The duration of the loading animation
		LoadTime2       =  5,           // For a callback LoadTime2 frames after LoadShoot: DuringLoad
		AnimationShoot  = nil,          // The animation for shooting
		AnimationShoot2 = nil,          // If not nil the shooting will blend according to angle (0° animation 1, 180° animation 2)
		AnimationShoot3 = nil,          // If not nil blending between 3 actions (90° anination 1, 0° animation 2, 180° animation 3)
		ShootTime       = 20,           // The duration of shooting
		ShootTime2      =  5,           // For a callback ShootTime2 frames after StartShoot: DuringShoot
		TurnType        = 1,            // Specify a special turn type (0: turn 120 degrees, 1: turn 180 degrees, 2: turn 220 degrees) see SetTurnType in clonk
		WalkSpeed       = 84,           // Reduce the walk speed during aiming
		WalkBack        = 56,           // Reduce the walk speed for walking backwards (speed 0 means no walking backwards at all)
		AimSpeed        = 5,            // the speed of aiming default 5° per frame
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
	GetAnimationSet(object clonk);         // Has to return the animation set; The clonk is passed, so that modifications can be made according to its state.
	// The following Stop* Callbacks, have to return true if the clonk doesn't have to be reset (e.g. stating aiming after loading)
	FinishedLoading(object clonk);             // When the loading animation is over (after LoadTime frames)
	FinishedAiming(object clonk, int angle);   // When the clonk has finished loading and aiming at the desired position
	FinishedShooting(object clonk, int angle); // When the shooting animation is over (after ShootTime frames)
	DuringLoad(object clonk);                  // LoadTime2 frames after load start
	DuringShoot(object clonk, int angle);      // ShootTime2 frames after shoot start
	// When the clonk has during aiming an action where he can't use his hands, the aiming is paused
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

local aim_schedule_timer;
local aim_schedule_call;

local aim_schedule_timer2;
local aim_schedule_call2;

// Aim modes
static const AIM_Position = 1; // The aim angle alters the position of the animation (0° menas 0% animation position, 180° menas 100% andimation position)
static const AIM_Weight   = 2; // The aim angle alters with blending between AnimationAim (for 0°) and AnimationAim2 (for 180°)

func ReadyToAction() { return _inherited(...); }
func SetHandAction() { return _inherited(...); }
func ReplaceAction() { return _inherited(...); }
func SetTurnType  () { return _inherited(...); }
func SetTurnForced() { return _inherited(...); }
func SetBackwardsSpeed() { return _inherited(...); }

func IsAiming() { return !!GetEffect("IntAim", this); }

func FxIntAimCheckProcedureStart(target, effect, tmp)
{
	if (tmp) return;
}

func FxIntAimCheckProcedureTimer()
{
	// Care about the aim schedules
	if (aim_schedule_timer != nil)
	{
		aim_schedule_timer--;
		if (aim_schedule_timer == 0)
		{
			Call(aim_schedule_call);
			aim_schedule_call = nil;
			aim_schedule_timer = nil;
		}
	}
	if (aim_schedule_timer2 != nil)
	{
		aim_schedule_timer2--;
		if (aim_schedule_timer2 == 0)
		{
			Call(aim_schedule_call2);
			aim_schedule_call2 = nil;
			aim_schedule_timer2 = nil;
		}
	}
	
	// check procedure
	if (!ReadyToAction())
		PauseAim();
}

func FxIntAimCheckProcedureStop(target, effect, reason, tmp)
{
	if (tmp) return;
	if (reason == 4)
	{
		CancelAiming();
	}
}

func PauseAim()
{
	if (!aim_weapon || aim_stop)
	{
		return CancelAiming();
	}
	// reissue the CON_Use command to the weapon when ready
	this->PauseUse(aim_weapon);
	CancelAiming();
}

public func StartLoad(object weapon)
{
	// only if we aren't adjusted to this weapon already
	if (weapon != aim_weapon)
	{
		// Reset old
		if (aim_weapon != nil)
		{
			aim_weapon->~Reset();
		}
		if (aim_set    != nil)
		{
			ResetHands();
		}

		// Remember new
		aim_weapon = weapon;
		aim_set = weapon->~GetAnimationSet(this);

		// Apply the set
		ApplySet(aim_set);

		// Add effect to ensure procedure
		AddEffect("IntAimCheckProcedure", this, 1,  1, this);
	}

	if (aim_set["AnimationLoad"] != nil)
	{
		PlayAnimation(aim_set["AnimationLoad"], CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationLoad"]), aim_set["LoadTime"], ANIM_Remove), Anim_Const(1000));
	}

	aim_schedule_timer = aim_set["LoadTime"];
	aim_schedule_call  = "StopLoad";
	
	if (aim_set["LoadTime2"] != nil)
	{
		aim_schedule_timer2 = aim_set["LoadTime2"];
		aim_schedule_call2  = "DuringLoad";
	}
}

public func DuringLoad() { aim_weapon->~DuringLoad(this); }

public func StopLoad()
{
	if (!aim_weapon || !aim_weapon->~FinishedLoading(this)) // return 1 means the weapon goes on doing something (e.g. start aiming) then we don't reset
	{
		ResetHands();
	}
}

public func StartAim(object weapon, int angle)
{
	// only if we aren't adjusted to this weapon already
	if (weapon != aim_weapon)
	{
		// Reset old
		if (aim_weapon != nil)
		{
			aim_weapon->~Reset();
		}
		if (aim_set    != nil)
		{
			ResetHands();
		}

		// Remember new
		aim_weapon = weapon;
		aim_set = weapon->~GetAnimationSet(this);

		// Apply the set
		ApplySet(aim_set);

		// Add effect to ensure procedure
		AddEffect("IntAimCheckProcedure", this, 1,  1, this);
	}

	if (aim_set["AnimationAim"] != nil)
	{
		if (aim_set["AimMode"] == AIM_Position)
		{
			aim_animation_index = PlayAnimation(aim_set["AnimationAim"], CLONK_ANIM_SLOT_Arms, Anim_Const(GetAnimationLength(aim_set["AnimationAim"]) / 2), Anim_Const(1000));
		}
		else if (aim_set["AimMode"] == AIM_Weight)
		{
			aim_animation_index = PlayAnimation(aim_set["AnimationAim"],  CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationAim"]),  aim_set["AimTime"], ANIM_Loop), Anim_Const(1000));
			aim_animation_index = PlayAnimation(aim_set["AnimationAim2"], CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationAim2"]), aim_set["AimTime"], ANIM_Loop), Anim_Const(1000), aim_animation_index);
			aim_animation_index++;
			SetAnimationWeight(aim_animation_index, Anim_Const(500));
		}
	}
	AddEffect("IntAim", this, 1, 1, this);
}

func FxIntAimTimer(target, effect, time)
{
	var angle, delta_angle, length;
	var speed = aim_set["AimSpeed"];
	if (speed == nil)
	{
		speed = 50;
	}
	else
	{
		speed *= 10;
	}
	if (aim_angle < 0)
	{
		SetTurnForced(DIR_Left);
	}
	if (aim_angle > 0)
	{
		SetTurnForced(DIR_Right);
	}
	if (aim_set["AimMode"] == AIM_Position)
	{
		length = GetAnimationLength(aim_set["AnimationAim"]);
		angle = Abs(aim_angle) * 10;//GetAnimationPosition(aim_animation_index)*1800/length;
		delta_angle = 0;//BoundBy(Abs(aim_angle*10)-angle, -speed, speed);
		SetAnimationPosition(aim_animation_index, Anim_Const( (angle + delta_angle) * length / 1800 ));
	}
	else if (aim_set["AimMode"] == AIM_Weight)
	{
		angle = Abs(aim_angle) * 10;//GetAnimationWeight(aim_animation_index)*1800/1000;
		delta_angle = 0;//BoundBy(Abs(aim_angle*10)-angle, -speed, speed);
		SetAnimationWeight(aim_animation_index, Anim_Const( (angle + delta_angle) * 1000 / 1800 ));
	}
	// We have reached the angle and we want to stop
	if (Abs(delta_angle) <= 5 && aim_stop == 1)
	{
		DoStopAim();
		return -1;
	}
}

// returns the current aiming angle
public func GetAimPosition() { return aim_angle; }

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
	// Return true means the weapon goes on doing something (e.g. start aiming) then we don't reset.
	if (!aim_weapon || !aim_weapon->~FinishedAiming(this, aim_angle))
	{
		ResetHands();
	}
	return;
}

public func StartShoot(object weapon)
{
	// only if we aren't adjusted to this weapon already
	if (weapon != aim_weapon)
	{
		// Reset old
		if (aim_weapon != nil)
		{
			aim_weapon->~Reset();
		}
		if (aim_set    != nil)
		{
			ResetHands();
		}

		// Remember new
		aim_weapon = weapon;
		aim_set = weapon->~GetAnimationSet(this);

		// Applay the set
		ApplySet(aim_set);

		// Add effect to ensure procedure
		AddEffect("IntAimCheckProcedure", this, 1,  1, this);
	}

	if (aim_set["AnimationShoot"] != nil)
	{
		// Do we just have one animation? Then just play it
		if (aim_set["AnimationShoot2"] == nil)
		{
			PlayAnimation(aim_set["AnimationShoot"], CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot"]), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000));
		}
		// Well two animations blend betweend them (animtion 1 is 0° animation2 for 180°)
		else if (aim_set["AnimationShoot3"] == nil)
		{
			var iAim;
			iAim = PlayAnimation(aim_set["AnimationShoot"],  CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot"] ), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000));
			iAim = PlayAnimation(aim_set["AnimationShoot2"], CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot2"]), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000), iAim);
			SetAnimationWeight(iAim + 1, Anim_Const(1000*Abs(aim_angle)/180));
		}
		// Well then we'll have three to blend (animation 1 is 90°, animation 2 is 0°, animation 2 for 180°)
		else
		{
			var iAim;
			if (Abs(aim_angle) < 90)
			{
				iAim = PlayAnimation(aim_set["AnimationShoot2"], CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot2"]), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000));
				iAim = PlayAnimation(aim_set["AnimationShoot"],  CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot"] ), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000), iAim);
				SetAnimationWeight(iAim + 1, Anim_Const(1000*Abs(aim_angle)/90));
			}
			else
			{
				iAim = PlayAnimation(aim_set["AnimationShoot"],  CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot"] ), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000));
				iAim = PlayAnimation(aim_set["AnimationShoot3"], CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, GetAnimationLength(aim_set["AnimationShoot3"]), aim_set["ShootTime"], ANIM_Remove), Anim_Const(1000), iAim);
				SetAnimationWeight(iAim + 1, Anim_Const(1000*(Abs(aim_angle)-90)/90));
			}
		}
	}

	aim_schedule_timer = aim_set["ShootTime"];
	aim_schedule_call  = "StopShoot";

	if (aim_set["ShootTime2"] != nil)
	{
		aim_schedule_timer2 = aim_set["ShootTime2"];
		aim_schedule_call2  = "DuringShoot";
	}
}

public func DuringShoot() { if (aim_weapon) aim_weapon->~DuringShoot(this, aim_angle); }

public func StopShoot()
{
	if (aim_weapon == nil || !aim_weapon->~FinishedShooting(this, aim_angle)) // return 1 means the weapon goes on doing something (e.g. start aiming) then we don't reset
	{
		ResetHands();
	}
}

public func CancelAiming()
{
	ResetHands();
}

public func ApplySet(set)
{
	// Setting the hands as blocked, so that no other items are carried in the hands
	SetHandAction(1);

	if (set["TurnType"] != nil)
	{
		SetTurnType(set["TurnType"], 1);
	}

	if (set["AnimationReplacements"] != nil)
	{
		for (var replace in set["AnimationReplacements"])
		{
			ReplaceAction(replace[0], replace[1]);
		}
	}

	if (set["WalkSpeed"] != nil)
	{
		AddEffect("IntWalkSlow", this, 1, 0, this, nil, set["WalkSpeed"]);
	}

	if (set["WalkBack"] != nil)
	{
		SetBackwardsSpeed(set["WalkBack"]);
	}
}

public func ResetHands()
{
	if (!GetEffect("IntAimCheckProcedure", this))
	{
		return;
	}
		
	if (aim_weapon != nil)
	{
		aim_weapon->~Reset(this);

		if (aim_set["AnimationReplacements"] != nil)
		{
			for (var replace in aim_set["AnimationReplacements"])
			{
				ReplaceAction(replace[0], nil);
			}
		}
	}

	aim_stop = 0;
	aim_angle = -90 + 180*GetDir();

	// Stop the aim/shoot animation
	StopAnimation(GetRootAnimation(CLONK_ANIM_SLOT_Arms));

	RemoveEffect("IntWalkSlow", this);
	SetBackwardsSpeed(nil);
	
	RemoveEffect("IntAim", this);

	SetTurnForced(-1);

	SetTurnType(0, -1);
	SetHandAction(0);

	aim_weapon = nil;
	aim_set = nil;
		
	aim_schedule_call = nil;
	aim_schedule_timer = nil;
	aim_schedule_call2 = nil;
	aim_schedule_timer2 = nil;
		
	RemoveEffect("IntAimCheckProcedure", this);
}

/* +++++++++++ Slow walk +++++++++++ */

func FxIntWalkSlowStart(pTarget, effect, fTmp, iValue)
{
	iValue = iValue ?? 420;
	pTarget->PushActionSpeed("Walk", iValue, Library_AimManager);
}

func FxIntWalkSlowStop(pTarget, effect)
{
	pTarget->PopActionSpeed("Walk", Library_AimManager);
}
