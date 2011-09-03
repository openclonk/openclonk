// Stone door destructible, and auto control for the base.

#appendto StoneDoor

protected func Damage()
{
	if (GetDamage() > 180)
	{
			if (!this)
			return false;
		var ctr = Contained();
		// Transfer all contents to container.
		while (Contents())
			if (!ctr || !Contents()->Enter(ctr))
				Contents()->Exit();
		// Split components.
		for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
			for (var j = 0; j < GetComponent(compid); ++j)
			{
				var comp = CreateObject(compid, nil, nil, GetOwner());
				if (OnFire()) comp->Incinerate();
				if (!ctr || !comp->Enter(ctr))
				{
					comp->SetR(Random(360));
					comp->SetXDir(Random(3) - 1);
					comp->SetYDir(Random(3) - 1);
					comp->SetRDir(Random(3) - 1);
					comp->SetClrModulation(RGB(240,210,200));	//give rocks the color of brick
				}
				}
		RemoveObject();
	}
	return;
}

private func IsOpen()
{
	if (GBackSolid(0, -20))
	 	return true;
	return false;
}

private func IsClosed()
{
	if (GBackSolid(0, 19))
	 	return true;
	return false;
}

// Automatically open for team stored in effect var 0.
protected func FxAutoControlStart(object target, effect, int temporary, int team)
{
	if (temporary == 0)
	effect.team = team;
	return 1;
}

protected func FxAutoControlTimer(object target, effect, int time)
{
	var d = 0;
	if (IsOpen())
		d = 30;
	var team = effect.team;
	var open_door = false;
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_InRect(-50, d - 30, 100, 60)))
	{
		var plr = clonk->GetOwner();
		var plr_team = GetPlayerTeam(plr);
		if (plr_team == team)
			open_door = true;
		else
		{
			open_door = false;
			break;
		}
	}
	
	if (open_door && IsClosed())
		OpenGateDoor();
	if (!open_door && IsOpen())
		CloseGateDoor();
	
	return 1;
}