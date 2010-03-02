/*-- Dynamite Igniter --*/

#include DynamiteBox

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
local aWires;

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

	ScheduleCall(this, "Ignite", iIgniteTime, 1, clonk);

	RemoveEffect("IntLength", this);
	
	return true;
}

local iIgniteX;
local iIgniteY;
local iIgniteNumber;
local pIgniteClonk;
local iVertexCounter;

local pHelper;

public func Ignite(clonk)
{
	iIgniteNumber = 0;
	iVertexCounter = aWires[0]->GetVertexNum()-1;
	var iIgniteX = aWires[iIgniteNumber]->GetVertex(iVertexCounter,0)*10;
	var iIgniteY = aWires[iIgniteNumber]->GetVertex(iVertexCounter,1)*10;
	pHelper = CreateObject(Fuse);
	pHelper->SetPosition(iIgniteX/10, iIgniteY/10);
	aWires[iIgniteNumber]->Connect(aDynamites[iIgniteNumber], pHelper);
	
	pIgniteClonk = clonk;
	
	AddEffect("IntIgnite", this, 1, 1, this);
}

func FxIntIgniteTimer(pTarget, iNumber, iTime)
{
	// End of the line?
	if(iIgniteNumber == GetLength(aDynamites))
	{
		ResetClonk(pIgniteClonk);
		return -1;
	}

	// Get vertex positions
	iVertexCounter = aWires[iIgniteNumber]->GetVertexNum()-1;
	iIgniteX = aWires[iIgniteNumber]->GetVertex(iVertexCounter,0)*10;
	iIgniteY = aWires[iIgniteNumber]->GetVertex(iVertexCounter,1)*10;
	var iX = aWires[iIgniteNumber]->GetVertex(iVertexCounter-1,0)*10;
	var iY = aWires[iIgniteNumber]->GetVertex(iVertexCounter-1,1)*10;

	// Move spark position
	var iAngle = Angle(iIgniteX, iIgniteY, iX, iY);
	iIgniteX += Sin(iAngle, 20);
	iIgniteY +=-Cos(iAngle, 20);
	pHelper->SetPosition(iIgniteX/10+RandomX(-1,1), iIgniteY/10+RandomX(0,1));
	CreateParticle("Spark",iIgniteX/10-GetX(), iIgniteY/10-GetY(), RandomX(-5,5), RandomX(-5,5), RandomX(15,25),RGB(255,200,0));

	// next vertex
	if(Distance(iX, iY, iIgniteX, iIgniteY) < 30)
	{
		aWires[iIgniteNumber]->RemoveVertex(iVertexCounter);
		if(iVertexCounter>1) iVertexCounter--;
		else
		{
			iIgniteNumber++;
			if(iIgniteNumber < GetLength(aDynamites))
			{
				iVertexCounter = aWires[iIgniteNumber]->GetVertexNum()-1;
				iIgniteX = aWires[iIgniteNumber]->GetVertex(iVertexCounter,0)*10;
				iIgniteY = aWires[iIgniteNumber]->GetVertex(iVertexCounter,1)*10;
				pHelper->SetPosition(iIgniteX/10, iIgniteY/10);
				aWires[iIgniteNumber]->Connect(aDynamites[iIgniteNumber], pHelper);
			}
			aDynamites[iIgniteNumber-1]->DoExplode();
		}
		return;
	}
}

public func ResetClonk(clonk)
{
	StopAnimation(GetRootAnimation(1));
	
	// Reset animation
  clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->SetAction("Walk");
	clonk->DetachObject(this);

	// The igniter isn't used anymore...
	pHelper->RemoveObject();
	RemoveObject();
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 7000, def);
  SetProperty("PerspectiveTheta", 20, def);
  SetProperty("PerspectivePhi", -30, def);
}