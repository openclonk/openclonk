/*-- 
	Javelin
	Author: Ringwaul
	
	A simple but dangerous throwing weapon.
--*/

#include L_ST
public func MaxStackCount() { return 3; }

local fAiming;
local power;

public func GetCarryMode(clonk) { return CARRY_Back; }
public func GetCarryBone() { return "Javelin"; }
public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

public func ControlUseStart(object pClonk, int x, int y)
{
	fAiming=true;
	pClonk->UpdateAttach();
	return 1;
}

public func HoldingEnabled() { return true; }

protected func ControlUseHolding(object pClonk, int ix, int iy)
{
	if(power<30) power=power+2;
	Message("%d",pClonk,power+30);
	return 1;
}

protected func ControlUseStop(object pClonk, int ix, int iy)
{
	var p = pClonk->GetProcedure();
	if(p != "WALK" && p != "FLIGHT" && p != "SWIM") return 1;

	var javelin=TakeObject();
	//If someone wants to use HitCheck for this, feel free to implement it. I can't seem to get it to work, anyways.
	javelin->AddEffect("Flight",javelin,1,1,javelin,nil);
	javelin->LaunchProjectile(Angle(0,0,ix,iy)+RandomX(-1, 1), 6, power+30);

	power=0;
	fAiming=false;
	pClonk->UpdateAttach();
}

private func Hit()
{	
	Sound("WoodHit");
	SetSpeed();
	RemoveEffect("HitCheck",this);
	RemoveEffect("Flight",this);
}

protected func FxFlightStart(object pTarget, int iEffectNumber)
{
	pTarget->SetProperty("Collectible",0);
}

protected func FxFlightTimer(object pTarget,int iEffectNumber, int iEffectTime)
{
	//Using Newton's arrow rotation. This would be much easier if we had tan^-1 :(
	var oldx = EffectVar(0,pTarget,iEffectNumber);
	var oldy = EffectVar(1,pTarget,iEffectNumber);
	var newx = GetX();
	var newy = GetY();

	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	pTarget->SetRDir(anglediff/2);
	pTarget->EffectVar(0,pTarget,iEffectNumber) = newx;
	pTarget->EffectVar(1,pTarget,iEffectNumber) = newy;
}

protected func FxFlightStop(object pTarget,int iEffectNumber)
{
	pTarget->SetProperty("Collectible", 1);
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 15000, def);
  SetProperty("PerspectiveTheta", 10, def);
  SetProperty("PerspectivePhi", -10, def);
}