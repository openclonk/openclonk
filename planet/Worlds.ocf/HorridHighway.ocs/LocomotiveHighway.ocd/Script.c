/** 
	Locomotive Highway
	Construct and protect the locomotive highway.
	
	@author Maikel
*/


#include Library_Goal
#include Library_SwitchTarget


/*-- Control --*/

// Reaction to operation by a switch.
public func OnSetInputSignal(object operator, object switch, bool right)
{
	if (!right)
	{
		if (!GetEffect("FxRunLocomotives", this) && !FindObject(Find_ID(Locomotive)))
			CreateEffect(FxRunLocomotives, 100, 36, this);
		switch->ControlSwitchDir(nil, -1);
	}
	_inherited(operator, switch, right, ...);
}

local FxRunLocomotives = new Effect
{
	Construction = func(object goal)
	{
		this.goal = goal;
		this.Interval = 36;
		this.train_int = 15;
		this.train_start = 1;
		this.goal->SetPassed(0);
		return FX_OK;
	},
	Timer = func(int time)
	{
		if (time > this.Interval * this.train_int * Target.LocomotiveGoal)
			return FX_Execute_Kill;
		// Launch a locomotive.
		if ((time / this.Interval) % this.train_int == this.train_start)
		{
			var loco = CreateObjectAbove(Locomotive, 12, LandscapeHeight() / 2 - 3);
			loco->SetEntrance(false);
			loco.IsContainer = Goal_LocomotiveHighway.FxRunLocomotives.NoContainer;
			loco.HasInteractionMenu = Goal_LocomotiveHighway.FxRunLocomotives.NoInteractions;
			loco->SetDir(DIR_Right);
			loco->CreateContents(Coal, 100);
			loco.LiquidCapacity = 100000;
			loco->PutLiquid(Water, 100000);
			loco->ContainedRight();
			loco->CreateEffect(Target.FxCheckLocomotive, 100, 2, this.goal);
		}
		return FX_OK;
	},
	NoContainer = func()
	{
		return false;
	},
	NoInteractions = func()
	{
		return false;
	}
};

local FxCheckLocomotive = new Effect
{
	Construction = func(object goal)
	{
		this.goal = goal;
		// The allowed time means that no obstacles must be in the way.
		// The train moves roughly at a pace of 1.3-1.4 pixels per frame.
		this.time_allowed = 4 * LandscapeWidth() / 5;
		this.Interval = 2;
		return FX_OK;
	},
	Timer = func(int time)
	{
		if (Target->GetX() > LandscapeWidth() - 14)
		{
			if (time <= this.time_allowed)
				this.goal->DoPassed(1);
			Target->RemoveObject();
			return FX_Execute_Kill;
		}
		if (time > this.time_allowed)
		{
			Target->FadeOut(18, true);
			return FX_Execute_Kill;		
		}
		return FX_OK;
	}	
};


/*-- Goal interface --*/

local nr_passed = 0;

public func SetPassed(int to_passed)
{
	nr_passed = to_passed;
	return;
}

public func GetPassed() { return nr_passed; }

public func DoPassed(int add_passed)
{
	SetPassed(GetPassed() + add_passed);
	return;
}

public func IsFulfilled()
{
	return nr_passed >= this.LocomotiveGoal;
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
	{
		message = "$MsgGoalFulfilled$";		
	}
	else
	{
		if (GetEffect("FxRunLocomotives", this) || FindObject(Find_ID(Locomotive)))
			message = Format("$MsgGoalRunning$", GetPassed(), this.LocomotiveGoal);
		else
			message = "$MsgGoalUnfulfilled$";
	}
	return message;
}

// Shows or hides a message window with information.
public func Activate(int plr)
{
	// If goal message open -> hide it.
	if (GetEffect("GoalMessage", this))
	{
		CustomMessage("", nil, plr, nil, nil, nil, nil, nil, MSG_HCenter);
		RemoveEffect("GoalMessage", this);
		return;
	}
	// Otherwise open a new message.
	AddEffect("GoalMessage", this, 100, 0, this);
	var message;
	if (IsFulfilled())
	{
		message = "@$MsgGoalFulfilled$";		
	}
	else
	{
		if (GetEffect("FxRunLocomotives", this) || FindObject(Find_ID(Locomotive)))
			message = Format("@$MsgGoalRunning$", GetPassed(), this.LocomotiveGoal);
		else
			message = "@$MsgGoalUnfulfilled$";
	}
	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}


/*-- Proplist --*/

local Name = "$Name$";
local LocomotiveGoal = 12;
