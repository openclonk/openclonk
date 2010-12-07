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

/*-- Interaction --*/

// Flag can be picked up by interaction.
public func IsInteractable(object clonk)
{
	if (GetAction() == "AttachCarrier")
		if (GetActionTarget() != clonk)
			return false;
	var controller = clonk->GetController();
	var ctrl_team = GetPlayerTeam(controller);
	if (ctrl_team == 0)
		return false;
	if (team == ctrl_team)
		if (IsAtBase())
			return false;
	return true;
}

public func GetInteractionMetaInfo(object clonk)
{
	var controller = clonk->GetController();
	var ctrl_team = GetPlayerTeam(controller);
	if (team == ctrl_team)
		return { Description = "$MsgBeamFlag$" };
	if (GetAction() == "AttachCarrier" && GetActionTarget() == clonk)
		return { Description = "$MsgDropFlag$" };
	return { Description = "$MsgGrabFlag$" };
}

public func Interact(object clonk)
{
	var controller = clonk->GetController();
	var ctrl_team = GetPlayerTeam(controller);
	// Picked up by owning team, beam flag to base.
	if (team == ctrl_team)
	{
		BeamFlag(true);
		return true;
	}
	// Carrying clonk, drop flag.
	if (GetAction() == "AttachCarrier" && GetActionTarget() == clonk)
	{
		SetAction("Idle");
		RemoveEffect("FlagCarried", clonk);
		Log("$MsgFlagDropped$", GetTaggedTeamName(ctrl_team), GetTaggedTeamName(team));
		return true;		
	}
	// Fiendly team, grab flag.
	SetAction("AttachCarrier", clonk);
	Log("$MsgFlagStolen$", GetTaggedTeamName(ctrl_team), GetTaggedTeamName(team));
	AddEffect("FlagCarried", clonk, 100, 5, this);
	return true;
}

protected func Initialize()
{
	PlayAnimation("Wave", 1, Anim_Linear(0, 0, GetAnimationLength("Wave"), 78, ANIM_Loop), Anim_Const(1000));
	return;
}

protected func FxFlagCarriedStart(object target, int num, int temp)
{
	if (temp == 0)
	{
		var trans = Trans_Mul(Trans_Translate(0, -17000, 0), Trans_Rotate(-90, 0, 1, 0));
		EffectVar(0, target, num) = target->AttachMesh(this, "pos_back1", "main", trans);
		this.Visibility = VIS_None;
		ReducePhysicals(target);
	}
	return 1;
}

// Checks whether the carrier has reached its base.
protected func FxFlagCarriedTimer(object target, int num)
{
	var controller = target->GetController();
	var ctrl_team = GetPlayerTeam(controller);
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

protected func FxFlagCarriedStop(object target, int num, int reason, bool temp)
{
	if (temp)
		return 1;
	this.Visibility = VIS_All;
	if (reason == 4)
		SetAction("Idle");
	if (target)
	{	
		target->DetachMesh(EffectVar(0, target, num));
		ResetPhysicals(target);
	}
	return 1;
}

// Reduces physicals by 80%.
private func ReducePhysicals(object clonk)
{
	var phys = ["Walk", "Jump", "Scale", "Hangle", "Swim"];
	for (var i = 0; i < GetLength(phys); i++)
		clonk->SetPhysical(phys[i], (8 * clonk->GetPhysical(phys[i], PHYS_Current)) / 10, PHYS_StackTemporary);
	return;
}

// Resets physicals.
private func ResetPhysicals(object clonk)
{
	var phys = ["Walk", "Jump", "Scale", "Hangle", "Swim"];
	for (var i = 0; i < GetLength(phys); i++)
		clonk->ResetPhysical(phys[i]);
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
	//SetAction("Idle");
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", team));
	if (base)
		SetAction("AttachBase", base);
	else 
		RemoveObject();
	return;
}

local Name = "$Name$";
local ActMap = {
	AttachCarrier = {
		Prototype = Action,
		Name = "AttachCarrier",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		NextAction = "Attach",
		Animation = "Wave",
	},
	AttachBase = {
		Prototype = Action,
		Name = "AttachBase",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		NextAction = "Attach",
		Animation = "Wave",
	},
};

protected func Definition(def) 
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(-79000, 1000, 0), Trans_Rotate(60, 0, 1, 0)), def);
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(60, 0, 1, 0), Trans_Scale(1200), Trans_Translate(-4000, 6500, -3000)), def);
}
