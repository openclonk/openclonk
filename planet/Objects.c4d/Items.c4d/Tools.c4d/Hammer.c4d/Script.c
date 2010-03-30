/*-- Hammer --*/

private func Hit()
{
	Sound("RockHit");
	return 1;
}

public func GetCarryMode()	{	return CARRY_HandBack;	}
public func GetCarryBone()	{	return "main";	}
public func GetCarryTransform()	{	return Trans_Rotate(90,0,1,0);	}

public func ControlUse(object clonk, int ix, int iy)
{ 
	Message("Using hammer", clonk);
	// Stop clonk
	clonk->SetComDir(COMD_Stop);

	// Is the clonk able to build?
	if (clonk && !clonk->GetPhysical("CanConstruct", PHYS_Current) && CheckCanUse(clonk)==true)
	{ 
		PlayerMessage(clonk->GetController(), "$TxtCantConstruct$", this, clonk->GetName()); 
		return 1; 
	}

	if(clonk->GetAction()=="Build") //Stop building
	{
		Message("Cancelling building",clonk);
		clonk->SetAction("Walk");
		clonk->SetActionTargets(0,0);
		return 1;
	}

	//Start building
	var structure=FindObject(Find_Category(C4D_Structure),Find_Distance(30));
	if(structure) {
		if(structure->GetCon()<100)
		{
			Message("Building",clonk);
			clonk->SetAction("Build",structure);
			return 1;
		}}
	
	// Create menu and fill it with the player's plans
	clonk->CreateMenu(Hammer, this, 1, "$TxtNoconstructionplansa$");
	var idType; var i = 0;
	while (idType = GetPlrKnowledge(clonk->GetOwner(), 0, i++, C4D_Structure))
		clonk->AddMenuItem("$TxtConstructions$", "CreateConstructionSite", idType);
	return 1;
}

protected func CreateConstructionSite(idType)
{
	// Only when the clonk is standing and outdoors
	if (Contained()->GetAction() != "Walk") return 0;
	if (Contained()->Contained()) return 0;
	// Check if the building can be build here
	if (idType->~RejectConstruction(0, 10, Contained()) ) return 0;
	// Set owner for CreateConstruction
	SetOwner(Contained()->GetOwner());
	// Create construction site
	var pSite;
	if (!(pSite = CreateConstruction(idType, 0, 10, Contained()->GetOwner(), 1, 1,1))) return 0;
	// Message
	Message("$TxtConstructions$", Contained(), pSite->GetName());
	return 1;
}

protected func CheckCanUse(object clonk)
{
	if(clonk->GetProcedure()=="WALK") return true;
	return false;
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("PictureTransformation",Trans_Rotate(20,1,0,1),def);
}
