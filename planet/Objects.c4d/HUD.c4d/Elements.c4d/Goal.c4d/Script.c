local goal;

protected func Construction()
{
	// parallaxity
	this["Parallaxity"] = [0,0];
	// visibility
	this["Visibility"] = VIS_Owner;
}

public func SetGoal(object go)
{
	goal = go;
	Update();
}

public func Update()
{
	if(!goal) return;
	
	// short desc
	var hudinfo = goal->~GetShortDescription(GetOwner());
	if(hudinfo)
		CustomMessage(Format("@%s",hudinfo), this, GetOwner(), 0, 75);
	else 
		CustomMessage("", this, GetOwner(), 0, 75);
	
	SetGraphics(nil,goal->GetID(),1,GFXOV_MODE_IngamePicture);
}

public func MouseSelection(int plr)
{
	if(plr != GetOwner()) return;
	if(!goal) return;
	goal->Activate(plr);
}