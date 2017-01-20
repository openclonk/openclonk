#appendto Airship

local HitPoints = 500;

public func AllStop()
{
	if (GetType(this) != C4V_Def) return;

	for (var airship in FindObjects(Find_ID(Airship)))
	{
		airship->SetCommand("None");
		airship->SetXDir();
		airship->SetYDir();
	}
}

public func IsInsideGondola(object clonk)
{
	if (!clonk) return false;

	if (Inside(clonk->GetX() - GetX(), this.gondola[0], this.gondola[2]))
		if (Inside(clonk->GetY() - GetY(), this.gondola[1], this.gondola[3]))
			return true;
	return false;
}

public func PromoteNewCaptain()
{
	var crew = GetCrewMembers();
	if (!GetLength(crew)) return Sink();
	var captain = RandomElement(crew);
	var fx = captain.ai;
	if (!fx) return Sink(); // shouldn't happen
	fx.weapon = this;
	fx.vehicle = this;
	fx.strategy = CustomAI.ExecutePilot;
}

public func PrepareToBoard(object cpt)
{
	var crew = GetCrewMembers();
	var count = 0;
	for (var member in crew)
	{
		var fx = member.ai;
		if (!fx) continue;
		fx.weapon = nil;
		fx.strategy = nil;
		fx.vehicle = nil;
		fx.carrier = nil;
		if (member->GetProcedure() == "PUSH") member->SetAction("Walk");
		count++;
	}
	// Suitable crew
	if (count >= 5)
		Sound("Clonk::Action::GroupAttack");
	// Let the captain yell something
	if (!Random(3) && cpt)
		cpt->Message(Translate(Format("MsgAttack%d", Random(4))));
}

private func Sink()
{
	Incinerate();
}

private func GetCrewMembers()
{
	return FindObjects(Find_InRect(this.gondola[0], this.gondola[1], this.gondola[2], this.gondola[3]), Find_Owner(GetOwner()), Find_OCF(OCF_Alive));
}

// When fading out the airship, the fade out rule does not check for position changes
public func FadeOutForced() { return true; }