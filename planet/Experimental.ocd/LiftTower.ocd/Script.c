/*--
	LiftTower
	Authors: Clonkonaut
--*/

local hook, rope;
local hook_pos, anim_no, stopped, direction;

static const LIFTTOWER_HOOK_LOOSEDIST = 50;

public func Construction()
{
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(-20, 20),0, 1, 0));
	return _inherited(...);
}

protected func Initialize()
{
	OnRopeBreak();
	hook_pos = CreateArray();
	anim_no = PlayAnimation("Turn", 10, Anim_Const(0));
	stopped = true;
	AddEffect("SpinWheel", this, 0, 5);
}

/* Rope */

func OnRopeBreak()
{
	if (!hook)
		hook = CreateObjectAbove(LiftTower_Hook, 0, 0, NO_OWNER);
	hook->Enter(this);
}

/* Interaction */

func IsInteractable(object clonk)
{
	return GetCon() >= 100;
}

func GetInteractionMetaInfo()
{
	if (!hook) OnRopeBreak();

	if (hook->Contained() == this)
		return { IconID = LiftTower_Hook, Description = "$TakeHook$" };
	else
		return { IconID = LiftTower_Hook, Description = "$Grab$" };
}

func Interact(object clonk)
{
	if (!hook) OnRopeBreak();

	if (hook->Contained() == this)
	{
		if (clonk->Collect(hook, nil, nil, true))
			hook->SetRope();
	}
	else
	{
		clonk->ObjectCommand("Grab", this);
	}

	return true;
}

func SetRope(object rope_to_set)
{
	rope = rope_to_set;
}

/* Control */

public func ControlUp(object clonk)
{
	return DrawIn();
}
public func ControlStop(object clonk)
{
	return RemoveEffect("DrawIn", this);
}

public func DrawIn()
{
	if (!rope) return false;
	if (!hook) OnRopeBreak();
	if (hook->Contained() == this) return false;
	if (ObjectDistance(hook) < LIFTTOWER_HOOK_LOOSEDIST) return false;
	if (GetEffect("DrawIn", this)) return false;
	rope->ConnectPull();
	return AddEffect("DrawIn", this, 1, 1, this);
}

private func FxDrawInTimer(effect)
{
	if (!rope) return -1;
	if (!hook)
	{
		OnRopeBreak();
		return -1;
	}
	rope->DoLength(-1);
	if (ObjectDistance(hook) < LIFTTOWER_HOOK_LOOSEDIST) return -1;
}

private func FxDrawInStop(object target, effect, int temp)
{
	if (temp) return;
	if (!rope) return;
	rope->ConnectLoose();
}

/* Animation */

protected func FxSpinWheelTimer()
{
	if (!hook) return StopWheel();
	if (hook->Contained() == this) return StopWheel();
	if (hook->GetX() == hook_pos[0])
		if (hook->GetY() == hook_pos[1])
			return StopWheel();

	stopped = false;
	var new_direction = false;
	if (!direction)
	{
		direction = 100;
		new_direction = true;
	}
	if (Distance(GetX(),GetY(), hook->GetX(), hook->GetY()) < Distance(GetX(),GetY(), hook_pos[0], hook_pos[1]))
	{
		if (direction > 0)
		{
			direction = -100;
			new_direction = true;
		}
	}
	else if (direction < 0)
	{
		direction = 100;
		new_direction = true;
	}
	hook_pos = [hook->GetX(), hook->GetY()];
	if (!new_direction) return;

	if (direction < 0)
		anim_no = PlayAnimation("Turn", 10, Anim_Linear(GetAnimationPosition(anim_no), GetAnimationLength("Turn"), 0, 40, ANIM_Loop));
	else
		anim_no = PlayAnimation("Turn", 10, Anim_Linear(GetAnimationPosition(anim_no), 0, GetAnimationLength("Turn"), 40, ANIM_Loop));
}

private func StopWheel()
{
	if (stopped) return;
	var position = GetAnimationPosition(anim_no);
	stopped = true;
	direction = 0;
	anim_no = PlayAnimation("Turn", 10, Anim_Const(position), Anim_Const(1000));
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000, 0, 7000),Trans_Rotate(-20, 1, 0, 0),Trans_Rotate(30, 0, 1, 0)), def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local ContainBlast = true;
local BlastIncinerate = 100;
