/*
	Bow
	Author: Newton
	
	The standard bow. This object is a standard projectile weapon
	with an extra slot.
*/

// has extra slot
#include L_ES

local aimtime;

public func HoldingEnabled() { return true; }

protected func ControlUse(object clonk, int x, int y)
{
	// check for ammo
	if(!Contents(0))
	{
		// reload
		var obj;
		if(obj = FindObject(Find_Container(clonk), Find_Func("IsArrow")))
		{
			obj->Enter(this);
		}
	}
	aimtime = 40;
	
	if(!Contents(0)) return false;
	
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	// check procedure
	var p = clonk->GetProcedure();
	if(p != "WALK" && p != "ATTACH" && p != "FLIGHT")	return -1;

	// angle
	var angle = Normalize(Angle(0,0,x,y),-180);
	// adapt aiming animation
	// ...
	if(aimtime > 0) aimtime--;

	// debug....
	var points = "";
	for(var i=2; i>=0; --i) { if(i<(aimtime/5)%4) points = Format(".%s",points); else points = Format("%s ",points); }

	if(aimtime > 0)
		Message("Reload arrow%s",clonk,points);
	else if(Abs(angle) > 160 || clonk->GetDir() == 1 && angle < 0 || clonk->GetDir() == 0 && angle > 0)
		Message("Cannot aim there",clonk);
	else
		Message("Aiming",clonk);
}

protected func ControlUseStop(object clonk, int x, int y)
{
	Message("",clonk);
	// "canceled"
	if(aimtime > 0) return true;
	
	var p = clonk->GetProcedure();
	if(p != "WALK" && p != "ATTACH" && p != "FLIGHT") return true;

	var angle = Normalize(Angle(0,0,x,y),-180);
	if(Abs(angle) > 160 || clonk->GetDir() == 1 && angle < 0 || clonk->GetDir() == 0 && angle > 0) return true;
	
	if(Contents(0))
	{
		if(Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle,100,clonk);
		}
	}
	return true;
}


func RejectCollect(id arrowid, object arrows)
{
	// arrows are not arrows? decline!
	if(!(arrows->~IsArrow())) return true;
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}