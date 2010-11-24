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
	if(team==1) SetMeshMaterial("FlagLeft");
	else SetMeshMaterial("FlagRight");
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
	if (clonk->GetProcedure() != "WALK")
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
	else
		return { Description = "$MsgGrabFlag$" };
}

public func Interact(object clonk)
{
	var controller = clonk->GetController();
	var ctrl_team = GetPlayerTeam(controller);
	if (team == ctrl_team)
		BeamFlag(true);
	else
	{
		Enter(clonk); // TODO: Checks here.
		Log("$MsgFlagStolen$", GetTeamName(ctrl_team), GetTeamName(team));
		AddEffect("FlagCarried", clonk, 100, 5, this);
	}
	return true;
}

protected func Initialize()
{
	PlayAnimation("Wave", 1, Anim_Linear(0, 0, GetAnimationLength("Wave"), 78, ANIM_Loop), Anim_Const(1000));
	return;
}

public func HasNoFadeOut() { return true; }

public func GetCarryMode() { return CARRY_Back; }
public func GetCarryTransform()
{
	return Trans_Mul(Trans_Translate(0,-17000,0),Trans_Rotate(90,0,1,0));
}

// Checks whether the carrier has reached its base.
protected func FxFlagCarriedTimer(object target)
{
	var controller = target->GetController();
	var ctrl_team = GetPlayerTeam(controller);
	if (FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", ctrl_team), Find_Distance(30))) 
	{
		var goal = FindObject(Find_ID(Goal_CaptureTheFlag));
		if (goal)
			goal->AddTeamScore(ctrl_team);
		Log("$MsgFlagCaptured$", GetTeamName(ctrl_team));
		BeamFlag(false);
		return -1;
	}
	return 1;
}

protected func Departure()
{
	RemoveEffect("FlagCarried");
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
		flag->SetPosition(base->GetX(), base->GetY() - 10);
		Log("$MsgFlagRestored$", GetTeamName(team));
	}
	return;
}

// Returns whether the flag is at its base.
public func IsAtBase()
{
	return !!FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", team), Find_Distance(30));
}

private func BeamFlag(bool msg)
{
	if (IsAtBase())
		return;
	if (msg)
		Log("$MsgFlagBeamed$", GetTeamName(team));
	Exit();
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", team));
	if (base)
	{
		if(ObjectDistance(base) > 30)
			SetPosition(base->GetX(), base->GetY() - 20);
	}
	else 
		RemoveObject();
	return;
}

local Name = "$Name$";
protected func Definition(def) 
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(-130000,-3000,0), Trans_Rotate(60,0,1,0)), def);
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(60, 0, 1, 0), Trans_Scale(1200), Trans_Translate(-4000,6500,-3000)), def);
}
