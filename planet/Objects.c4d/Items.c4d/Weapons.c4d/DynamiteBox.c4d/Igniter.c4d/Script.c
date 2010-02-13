/*-- Dynamite Igniter --*/

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }
public func GetCarryPhase() { return 250; }

public func GetCarrySpecial(clonk) { if(fIgnite) return "pos_hand2"; }

local fIgnite;
local aDynamites;

public func ControlUse(object clonk, int x, int y)
{
	if(clonk->GetAction() != "Walk") return true;
	
	fIgnite = 1;
	
	// The clonk has to stand
	clonk->SetAction("Stand");
	clonk->SetXDir(0);

	
	var iIgniteTime = 35*2;
	clonk->PlayAnimation("DoIgnite", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("DoIgnite"),  iIgniteTime, ANIM_Hold), Anim_Const(1000));
	PlayAnimation("Ignite", 1, Anim_Linear(0, 0, GetAnimationLength("Ignite"),  iIgniteTime, ANIM_Hold), Anim_Const(1000));

	ScheduleCall(this, "Ignite", iIgniteTime+20, 1, clonk);
	
	return true;
}

public func Ignite(clonk)
{
	StopAnimation(GetRootAnimation(1));

	for(var i = 0; i < GetLength(aDynamites); i++)
		ScheduleCall(aDynamites[i], "SetFuse", i*5+5);
	
	// Reset animation
  clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->SetAction("Walk");
	clonk->DetachObject(this);

	// The igniter isn't used anymore...
//	RemoveObject();
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 7000, def);
  SetProperty("PerspectiveTheta", 20, def);
  SetProperty("PerspectivePhi", -30, def);
}