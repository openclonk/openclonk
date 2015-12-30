#appendto Airship

local health = 500;

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

public func PromoteNewCaptain()
{
	var crew = GetCrewMembers();
	if (!GetLength(crew)) return Sink();
	var captain = RandomElement(crew);
	var fx = GetEffect("AI", captain);
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
		var fx = GetEffect("AI", member);
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
		Sound("Attack");
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