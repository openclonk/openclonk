/*-- Shoot the Targets --*/

#include Library_Goal

local count;

func Initialize()
{
	SetPosition(); //remove silly offset
	count=1;
	HideSettlementScoreInEvaluation(true); 
	inherited(...);
}

global func HasHitTarget()
{
	FindObject(Find_ID(Goal_ShootTheTargets))->TargetCounter();
}

public func TargetCounter()
{
	++count;
	var i = count;
	if(i == 2) MakeTarget(126,359,true);
	if(i == 3) MakeTarget(629,185,true);
	if(i == 4) MakeTarget(54,573,false);
	if(i == 5) MakeTarget(853,396,true);
	if(i == 6) MakeTarget(788,531,true);
}

protected func MakeTarget(int ix, int iy, bool flying)
{
	if(flying == nil) balloon = false;

	var target = CreateObject(PracticeTarget,ix,iy);
	if(flying == true)
	{
		var balloon = CreateObject(TargetBalloon,ix,iy-30);
		target->SetAction("Attach",balloon);
		CreateParticle("Flash",ix,iy-50,0,0,500,RGB(255,255,255));
	}

	if(flying == false)
	{
		CreateParticle("Flash",ix,iy,0,0,500,RGB(255,255,255));
		target->SetAction("Float");
	}
}

public func IsFulfilled()
{
	if(count==7) return true;
	else
		return false;
}

public func Activate(int byplr)
{
	MessageWindow(GetDesc(), byplr);
	return;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}