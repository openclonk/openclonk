/*-- Lorry --*/

#include Library_ItemContainer

local content_menu;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(13,0,1,0));
}

public func IsLorry() { return 1; }

public func IsToolProduct() { return 1; }

local drive_anim;
local tremble_anim;

protected func Initialize()
{
	drive_anim = PlayAnimation("Drive", 5, Anim_Const(0), Anim_Const(500) /* ignored anyway */);
	tremble_anim = PlayAnimation("Tremble", 5, Anim_Const(0), Anim_Const(500));

	iRotWheels = 0;
	iTremble = 0;
}

/*-- Movement --*/

protected func ContactLeft()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

protected func ContactRight()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

/*-- Contents --*/

private func MaxContentsCount()
{
	return 50;
}

private func MenuOnInteraction() { return false; }
private func MenuOnControlUse() { return true; }
private func MenuOnControlUseAlt() { return false; }

protected func RejectCollect(id object_id, object obj)
{
	if (ContentsCount() < MaxContentsCount())
	{
		Sound("Clonk");
		return false;
	}
	if (obj->Contained())
		return Message("$TxtLorryisfull$");
	return _inherited(...);
}

// Automatic unloading in buildings.
protected func Entrance(object container)
{
	// Only in buildings
	if (container->GetCategory() & (C4D_StaticBack | C4D_Structure))
		// Not if the building prohibits this action.
		if (!container->~NoLorryEjection(this))
			// Empty lorry.
			container->GrabContents(this);
}

local iRotWheels;
local iTremble;

func TurnWheels()
{
	// TODO: Use Anim_X(Dir), keep from timer=1
	// TODO: Could also use GetAnimationPosition() instead of these local variables...
	iRotWheels += GetXDir()*2000/100; // Einmal rum (20 frames mal 10fps) nach 10 m
	while(iRotWheels < 0) iRotWheels += 2000;
	while(iRotWheels > 2000) iRotWheels -= 2000;
	SetAnimationPosition(drive_anim, Anim_Const(iRotWheels));
	if(Random(100) < Abs(GetXDir()))
	{
		iTremble += 100;
		if(iTremble < 0) iTremble += 2000;
		if(iTremble > 2000) iTremble -= 2000;
		SetAnimationPosition(tremble_anim, Anim_Const(iTremble));
	}
}

/* Drive on Rail */
local pRailTarget;
local rail_direction; // 2 up the line, 1 down the line, 0 no movement
local rail_progress;
local rail_max_prog;
local rail_destination;

func StartRail(obj)
{
  SetPosition(obj->GetX(), obj->GetY());
  SetAction("OnRail");
  pRailTarget = obj;
  rail_direction = 0;
}

func SetDestination(obj)
{
  if(GetType(obj) == C4V_Int)
  {
    obj = FindObjects(Find_ID(CableCrossing))[obj];
  }
  rail_destination = obj;
  if(rail_direction == 0)
    CrossingReached();
}

func CrossingReached()
{
  var target;
  if(rail_destination != pRailTarget)
    if(target = pRailTarget->GetNextWaypoint(rail_destination))
      MoveTo(target);
}

func MoveTo(obj)
{
  if(GetType(obj) == C4V_Int)
  {
    obj = FindObjects(Find_ID(CableCrossing))[obj];
  }
  var rail = 0;
  for(var test_rail in FindObjects(Find_Func("IsConnectedTo", pRailTarget)))
  {
    if(test_rail->IsConnectedTo(obj))
    {
      rail = test_rail;
      break;
    }
  }
  if(!rail)
  {
    Message("No Rail availible!");
    return;
  }
  // Target the first or section action target?
  if(rail->GetActionTarget(0) == obj)
  {
    rail_direction = 1;
    rail_progress  = 0;
  }
  else
  {
    rail_direction = 2;
    rail_progress  = 0;
  }
  rail->GetActionTarget(0)->AddActive(0);
  rail->GetActionTarget(1)->AddActive(0);
  rail->AddActive(0);
  rail_max_prog = ObjectDistance(obj, pRailTarget);
  pRailTarget = rail;
//  Log("%d %d", rail_max_prog, pRailTarget, rail);
}

func OnRail()
{
  if(rail_direction == 0 || rail_direction == nil) return;
  var start = 0;
  var end = 1;
  if(rail_direction == 1)
  {
    start = 1;
    end = 0;
  }
  
  rail_progress++;
  if(rail_progress == rail_max_prog)
  {
    pRailTarget->GetActionTarget(0)->AddActive(1);
    pRailTarget->GetActionTarget(1)->AddActive(1);
    pRailTarget->AddActive(1);
    pRailTarget = pRailTarget->GetActionTarget(end);
    SetPosition(pRailTarget->GetX(), pRailTarget->GetY());
    rail_direction = 0;
    CrossingReached();
    return;
  }
  
  var prec = 100;
  var x = pRailTarget->GetActionTarget(start)->GetX(prec)+
          (pRailTarget->GetActionTarget(end)->GetX(prec)-pRailTarget->GetActionTarget(start)->GetX(prec))*rail_progress/rail_max_prog;
  var y = pRailTarget->GetActionTarget(start)->GetY(prec)+
          (pRailTarget->GetActionTarget(end)->GetY(prec)-pRailTarget->GetActionTarget(start)->GetY(prec))*rail_progress/rail_max_prog;
  SetPosition(x, y, 1, prec);	  
}

local ActMap = {
		Drive = {
			Prototype = Action,
			Name = "Drive",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 20,
			Delay = 2,
			X = 0,
			Y = 0,
			Wdt = 22,
			Hgt = 16,
			NextAction = "Drive",
			//Animation = "Drive",
		},
		OnRail = {
			Prototype = Action,
			Name = "OnRail",
			Procedure = DFA_FLOAT,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 1,
			X = 0,
			Y = 0,
			Wdt = 22,
			Hgt = 16,
			StartCall="OnRail",
			NextAction = "OnRail",
			//Animation = "Drive",
		},
};

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Rebuy = true;
