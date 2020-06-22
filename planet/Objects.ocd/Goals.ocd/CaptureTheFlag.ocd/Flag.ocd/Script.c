/*--
	Flag
	Author: Maikel
		
	Flag for CTF goal, to be captured and defended.
--*/


/*-- Team --*/

// Team to which the flag belongs.
local team;

public func SetTeam(int to_team)
{
	if (!to_team)
		return;
	team = to_team;
	//UpdateColor();
	return;
}

public func GetTeam()
{
	return team;
}

public func FindTeam(int find_team)
{
	if (!find_team || !team)
		return false;
	return team == find_team;
}

protected func Initialize()
{
	PlayAnimation("Wave", 1, Anim_Linear(0, 0, GetAnimationLength("Wave"), 78, ANIM_Loop));
	AddEffect("FlagAutoPickup", this, 100, 2, this);
	return;
}

public func DisablePickup()
{
	// Disable pickup search in case flag is used outside the regular CTF goal
	return RemoveEffect("FlagAutoPickup", this);
}

// Handles automatic picking up of the flag.
protected func FxFlagAutoPickupTimer(object target, effect)
{
	// Do nothing if flag is being carried.
	if (target->GetAction() == "AttachCarrier")
		return 1;

	// Find a near clonk which can grab the flag.
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(20), Sort_Distance()))
	{
		var plr = clonk->GetOwner();	
		if (GetPlayerTeam(plr) != team)
		{
			// Fiendly team, grab flag.
			SetAction("AttachCarrier", clonk);
			Log("$MsgFlagStolen$", GetTaggedTeamName(GetPlayerTeam(plr)), GetTaggedTeamName(team));
			AddEffect("FlagCarried", clonk, 100, 5, this);
			return 1;
		}
		else
		{
			// Friendly team can only beam flag if return delay is over.
			if (target->IsAtBase()) 
				continue;
			if (GetEffect("FlagReturnDelay",target))
				continue;
			target->BeamFlag(true);
			return 1;
		}
	}
	return 1;
}

// Return delay for friendly team after flag has been dropped.
protected func FxFlagReturnDelayTimer() { return -1; }

protected func FxFlagCarriedStart(object target, effect, int temp)
{
	ReducePhysicals(target, effect);
	if (temp) return;
	
	effect.x = target->GetX();
	effect.y = target->GetY();
	var trans = Trans_Mul(Trans_Translate(-17000, 0, 0), Trans_Rotate(90, 1, 0, 0));
	effect.mesh_id = target->AttachMesh(this, "pos_back1", "main", trans);
	this.Visibility = VIS_None;
	
	var color = GetTeamColor(this->GetTeam());
	effect.tracer_particles = 
	{
		Size = PV_KeyFrames(0, 0, 0, 200, 5, 900, 10, 1000, 0),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		Alpha = 200,
		Attach = ATTACH_Back
	};
	
	return 1;
}

// Checks whether the carrier has reached its base.
protected func FxFlagCarriedTimer(object target, effect)
{
	var controller = target->GetController();
	var ctrl_team = GetPlayerTeam(controller);
	var x = effect.x;
	var y = effect.y;
	var newx = target->GetX();
	var newy = target->GetY();
	// Draw partical line following the flag.
	if (Distance(x, y, newx, newy) > 5)
	{
		target->CreateParticle("SphereSpark", 0, 0, 0, 0, PV_Random(36 * 3, 36 * 3 + 10), effect.tracer_particles);
		effect.x = newx;
		effect.y = newy;
	}
	// Search for nearby base to drop flag and score a point.
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", ctrl_team), Find_Distance(20));
	if (base && base->IsBaseWithFlag()) 
	{
		var goal = FindObject(Find_ID(Goal_CaptureTheFlag));
		if (goal)
			goal->AddTeamScore(ctrl_team);
		Log("$MsgFlagCaptured$", GetTaggedTeamName(ctrl_team));
		BeamFlag(false);
		return -1;
	}
	return 1;
}

protected func FxFlagCarriedStop(object target, effect, int reason, bool temp)
{
	if (target)
		ResetPhysicals(target, effect);
	if (temp)
		return 1;
	this.Visibility = VIS_All;
	if (reason == 4)
	{
		SetAction("Idle");
		Log("$MsgFlagDropped$", GetTaggedTeamName(GetPlayerTeam(target->GetOwner())), GetTaggedTeamName(team));
	}
	if (target)
	{	
		target->DetachMesh(effect.mesh_id);
	}
	// Prevent beaming flag for 3 seconds.
	AddEffect("FlagReturnDelay", this, 100, 36 * 3, this);
	return 1;
}

// Reduces physicals by 80%.
private func ReducePhysicals(object clonk, effect)
{
	effect.clonk_jumpspeed = clonk.JumpSpeed;
	clonk.JumpSpeed = clonk.JumpSpeed * 8 / 10;
	var phys = ["Walk"/*, "Scale"*/, "Hangle", "Swim"];
	for (var i = 0; i < GetLength(phys); i++)
		clonk->PushActionSpeed(phys[i], 800, GetID());
	return;
}

// Resets physicals.
private func ResetPhysicals(object clonk, effect)
{
	clonk.JumpSpeed = effect.clonk_jumpspeed;
	var phys = ["Walk"/*, "Scale"*/, "Hangle", "Swim"];
	for (var i = 0; i < GetLength(phys); i++)
		clonk->PopActionSpeed(phys[i], GetID());
	return;

}

// Create a new flag on destruction.
protected func Destruction()
{
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", GetTeam()));
	if (base)
	{
		var flag = CreateObject(Goal_Flag, 0, 0, GetOwner());
		flag->SetTeam(GetTeam());
		SetAction("AttachBase", base);
		Log("$MsgFlagRestored$", GetTaggedTeamName(team));
	}
	return;
}

// Returns whether the flag is at its base.
public func IsAtBase()
{
	if (GetAction() == "AttachBase")
		return true;
	return false;
}

private func BeamFlag(bool msg)
{
	if (IsAtBase())
		return;
	if (msg)
		Log("$MsgFlagBeamed$", GetTaggedTeamName(team));
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", team));
	if (base)
		SetAction("AttachBase", base);
	else 
		RemoveObject();
	return;
}

func StartAttachCarrier()
{
	// attach fourth vertex of the flag to third vertex of Clonk
	// this results in the best overlapping of the shapes
	SetActionData((4 << 8) + 3);
}

func StartAttachBase()
{
	// reset possible action data
	SetActionData(0);
}

local Name = "$Name$";
local Plane = 310;
local ActMap = {
	AttachCarrier = {
		Prototype = Action,
		Name = "AttachCarrier",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		NextAction = "AttachCarrier",
		Animation = "Wave",
		StartCall = "StartAttachCarrier"
	},
	AttachBase = {
		Prototype = Action,
		Name = "AttachBase",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		NextAction = "AttachBase",
		Animation = "Wave",
		StartCall = "StartAttachBase"
	},
};

protected func Definition(def) 
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(-9000, 1000, 0), Trans_Rotate(60, 0, 1, 0)), def);
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(60, 0, 1, 0), Trans_Scale(1200), Trans_Translate(-4000, 6500, -3000)), def);
}
