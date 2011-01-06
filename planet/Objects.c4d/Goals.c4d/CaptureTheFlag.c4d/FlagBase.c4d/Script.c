/*--
		Flag Base
		Author: Maikel
		
		A base for the CTF goal, flags can be stolen here, and players must bring flags of opposing teams here.
--*/


local team;

public func SetTeam(int to_team)
{
	if (!to_team)
		return;
	team = to_team;
	UpdateColor();
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

// Returns whether there is a flag at this base.
public func IsBaseWithFlag()
{
	var flag = FindObject(Find_ID(Goal_Flag), Find_Func("FindTeam", team));
	if (flag->GetAction() != "AttachBase")
		return false;
	if (flag->GetActionTarget() != this)
		return false;
	return true;
}

private func UpdateColor()
{
	//SetClrModulation(GetTeamColor(team));
}

local Name = "Flagbase";