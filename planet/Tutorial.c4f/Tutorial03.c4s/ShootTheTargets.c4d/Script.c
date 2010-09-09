/*-- Target Helper Object --*/

local count;

func Initialize()
{
	SetPosition();
	count=0;
}

global func HasHitTarget()
{
	FindObject(Find_ID(ShootTheTargets))->TargetCounter();
	OnTargetDeath(FindObject(Find_ID(ShootTheTargets))->LocalN("count"));
}

public func TargetCounter()
{
	++count;
	var i = count;
	//Javelin Targets
	if(i == 1) MakeTarget(300,530,true);
	if(i == 2)
	{
		var target2 = MakeTarget(510,510,true);
		AddEffect("FlintDrop",target2,1,0,target2);
	}
	if(i == 3) MakeTarget(360,360,true);
	if(i == 4)
	{
		var target4 = MakeTarget(1030,244,true)->GetActionTarget();
		AddEffect("Moving",target4,1,1,target4);
	}
	if(i == 5)
	{
		var target5 = MakeTarget(1525,150,true);
		AddEffect("FlintDrop",target5,1,0,target5);
	}
	if(i == 6) MakeTarget(1908,362,true);

	//Bow Targets
	if(i == 7) MakeTarget(2352,247,true);
	if(i == 8) MakeTarget(2623,310,true);
	if(i == 9) MakeTarget(2090,578,false);
	if(i == 10) MakeTarget(2165,310,true);
	if(i == 11) MakeTarget(2750,239,true);
	if(i == 12) MakeTarget(2783,530,true);
}

global func FxMovingTimer(object target, int num, int time)
{
	target->SetXDir(Sin(time,20));
}

global func FxFlintDropStop(object target, int num, int reason, bool temporary)
{
	CreateObject(Firestone);
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
	return target;
}

local Name = "$Name$";