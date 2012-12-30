/*--
	Carry-Heavy Library
	Author: Ringwaul

	Special circumstances for when the clonk is carrying heavy objects
--*/

local liftheavy_carrier;

public func IsCarryHeavy() { return true; }
public func IsInvInteract() { return true; }

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarrySpecial(clonk)
{
	var action = clonk->~GetAction();
	if(action == "Scale" || action == "Hangle" || action == "Push")
		return "skeleton_body";
}

public func GetDropDescription() { return Format("$TxtPutDown$", GetName()); }

public func Grabbed(object clonk, bool grab)
{
	if(grab)
	{
		// clonk already carries something. Don't pick up
		if(clonk->~IsCarryingHeavy())
		{
			CustomMessage("$TxtHandsFull$", clonk, clonk->GetController(), 0, 0, 0xff0000);
			//Let go of object
			clonk->SetAction("Walk");
			//Clear animation
			clonk->SetCommand("None");
			
			return false;
		}
		
		// do pickup stuff enter stuff whatever
		liftheavy_carrier = clonk;
		
		Enter(liftheavy_carrier);
		DoLift();
	}
}


private func DoLift(bool forceEnter)
{
	if(!liftheavy_carrier)
		return;
	if(!IsCarryingHeavy(liftheavy_carrier))
		AddEffect("IntLiftHeavy", liftheavy_carrier, 1, 1, this, nil, forceEnter);
}

// ------------------
// Lifting the object
// ------------------
static const lift_heavy_time = 60;
func FxIntLiftHeavyStart(object clonk, proplist effect, bool tmp, bool forceEnter)
{
	if(tmp) return;
	if(!clonk) return -1;
	if(Contained() != clonk) return -1;
	
	// if the clonk is inside, we can skip the animation
	if(clonk->Contained())
	{
		AddEffect("IntCarryHeavy", clonk, 1, 1, this);
		return -1;
	}

	//Stop the clonk from moving, and tell the clonk's control library
	//it now has a hand action
	clonk->SetTurnForced(clonk->GetDir());
	clonk->SetHandAction(1);
	clonk->SetAction("Stand");

	//Attach the mesh of the object. It is not displayed normally because the
	//hands are told they have an action in the next few lines
	effect.mesh = clonk->AttachMesh(this->GetID(), "pos_tool1", "main", this->~GetCarryTransform(clonk));

	//Play the animation of the clonk picking up the object
	effect.anim = clonk->PlayAnimation("CarryArmsPickup", 10, Anim_Linear(0,0,clonk->GetAnimationLength("CarryArmsPickup"), lift_heavy_time, ANIM_Remove), Anim_Const(1000));
	
	effect.doExit = !forceEnter; // default: true
}

func FxIntLiftHeavyTimer(object clonk, proplist effect, int timer)
{
	//If the clonk moves, he'll stop lifting and drop the object
	if(timer < lift_heavy_time)
	{
		//from 0 to 40, the clonk will drop the object if he moves
		if(timer < lift_heavy_time - 20)
		{
			if(clonk->GetAction() != "Stand" || clonk->IsJumping() || Abs(clonk->GetXDir()) > 0)
			{
				return -1;
			}
		}
		//There is a small over-shot at the end the player may mistake for finished lifting
		//So stop the clonk from moving during 40-60
		else
		{
			if(clonk->GetAction() != "Stand")
			{
				//If the clonk moved when he was disabled from doing so (or jumped), cancel lifting
				return -1;
			}
		}
	}
		
	//When the clonk has finished lifting, remove movement-restrictions and add carry effect
	if(timer >= lift_heavy_time)
	{
		AddEffect("IntCarryHeavy", clonk, 1, 1, this);
		// don't exit the object
		effect.doExit = false;
		return -1;
	}
	
	// we got moved out during lifting
	if(Contained() != clonk)
		return -1;
}

func FxIntLiftHeavyStop(object clonk, proplist effect, int reason, bool tmp)
{
	if(tmp) return;
	
	// drop the object
	if(effect.doExit && Contained()==clonk) // only if still in the clonk
		Exit();
	
	clonk->DetachMesh(effect.mesh);
	clonk->StopAnimation(effect.anim);
	
	UndoLift(clonk);
}

// -------------------
// Carrying the object
// -------------------
func FxIntCarryHeavyTimer(object clonk, proplist effect, int timer)
{
	//Delete this effect if not contained in the clonk anymore
	if(Contained() != clonk) return -1;
}

// ------------------
// Dropping the object
// ------------------
func FxIntDropHeavyStart(object clonk, proplist effect, bool tmp)
{
	if(tmp) return;
	if(!clonk) return -1;
	if(Contained() != clonk) return -1;

	if(clonk->GetEffect("IntCarryHeavy"))
		clonk->RemoveEffect("IntCarryHeavy");

	// if the clonk is inside, we don't play the animation
	if(clonk->Contained())
		return -1;

	clonk->SetTurnForced(clonk->GetDir());
	clonk->SetHandAction(1);
	clonk->SetAction("Stand");
	
	//Stop the clonk if he is moving
	if(clonk->GetXDir() != 0) clonk->SetXDir();

	//Attach the mesh of the object. It is not displayed normally because the
	//hands are told they have an action in the next few lines
	effect.mesh = clonk->AttachMesh(this->GetID(), "pos_tool1", "main", this->~GetCarryTransform(clonk));

	//Play the animation of the clonk setting down the object
	effect.anim = clonk->PlayAnimation("CarryArmsPickup", 10, Anim_Linear(clonk->GetAnimationLength("CarryArmsPickup"),clonk->GetAnimationLength("CarryArmsPickup"),0, lift_heavy_time, ANIM_Remove), Anim_Const(1000));
	//liftheavy_anim = clonk->PlayAnimation("CarryArmsSetdown", 10, Anim_Linear(0,0,clonk->GetAnimationLength("CarryArmsSetdown"), lift_heavy_time, ANIM_Remove), Anim_Const(1000));
}

func FxIntDropHeavyTimer(object clonk, proplist effect, int timer)
{
	//Clonk was interrupted?
	if(clonk->GetAction() != "Stand")
	{
		return -1;
	}

	// animation finished?
	if(timer >= lift_heavy_time)
		return -1;
	
	// we got moved out during lifting
	if(Contained() != clonk)
		return -1;
}

func FxIntDropHeavyStop(object clonk, proplist effect, int reason, bool tmp)
{
	if(tmp) return;

	clonk->DetachMesh(effect.mesh);
	clonk->StopAnimation(effect.anim);
	
	if(clonk->GetAction() != "Stand")
		Exit();
	else
	{
		var dir = 1;
		if(clonk->GetDir() == DIR_Left)
			dir = -1;
		// Set down at barrel position
		Exit(6*dir, 9);
	}
	
	UndoLift(clonk);
}

func Drop()
{
	if(liftheavy_carrier == nil)
		return;
	if(!IsCarryingHeavy(liftheavy_carrier))
		return;
	
	AddEffect("IntDropHeavy", liftheavy_carrier, 1, 1, this);
}

func UndoLift(object clonk)
{
	//allow the clonk to turn again
	clonk->SetTurnForced(-1);
	clonk->SetHandAction(0);
	if(clonk->GetAction() == "Stand") clonk->SetAction("Walk");
}

func IsCarryingHeavy(object clonk)
{
	//Is the clonk in the process of lifting or carrying a heavy object?
	if(GetEffect("IntLiftHeavy", clonk)) return 1;
	if(GetEffect("IntCarryHeavy", clonk)) return 2;
	if(GetEffect("IntDropHeavy", clonk)) return 3;
	return false;
}

protected func Entrance(object obj)
{
	// tell the carrier to carryheavy if it got moved into it by script
	if(!liftheavy_carrier)
		if(obj->~GetCarryHeavy() == this)
		{
			liftheavy_carrier = obj;
			if(obj->GetAction() == "Walk" && !obj->Contained())
				DoLift(true);
			else
				AddEffect("IntCarryHeavy",obj, 1, 1, this);
		}
}

protected  func Departure(object obj)
{
	if(!liftheavy_carrier)
		return;
	
	liftheavy_carrier = nil;
}


// Cannot pickup other carryheavy objects (is that really what you intended, Ringwaul?)
protected func RejectCollect(id collectid, object collect)
{
	if(collect->~IsCarryHeavy()) return true;
	else
		return false;
}