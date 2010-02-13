/*-- Dynamite box --*/

public func Initialize()
{
	iCount = 5;
	aDynamites = [0,0,0,0,0];
	SetGraphics("5", this, 1, GFXOV_MODE_Picture);
}

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }
public func GetCarryPhase() { return 450; }

local iCount;
local aDynamites;

public func ControlUse(object clonk, int x, int y)
{
	if(clonk->GetAction() != "Walk") return true;

	iCount--;
	SetGraphics(Format("%d", iCount), this, 1, GFXOV_MODE_Picture);

	var pDyna = aDynamites[iCount] = CreateObject(DYNA, 0, clonk->GetDefHeight()/2);
	pDyna->SetReady();

	Message("%d left", clonk, iCount);

	if(iCount == 0)
	{
		var pos = clonk->GetItemPos(this);
		ChangeDef(IGNT);
		SetGraphics("Picture", this, 1, GFXOV_MODE_Picture);
		clonk->UpdateAttach();
		clonk->OnSlotFull(pos);
	}

	return true;
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 10000, def);
  SetProperty("PerspectiveTheta", 20, def);
  SetProperty("PerspectivePhi", 60, def);
}