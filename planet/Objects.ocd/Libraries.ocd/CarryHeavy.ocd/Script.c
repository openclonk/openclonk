/*--
	Carry-Heavy Library
	Author: Ringwaul

	Special circumstances for when the clonk is carrying heavy objects
--*/

local liftheavy_carrier;
local liftheavy_mesh;
local liftheavy_anim;

public func IsCarryHeavy() { return true; }
public func IsInvInteract() { return true; }

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
		
		Enter(clonk);
		
		// do pickup stuff enter stuff whatever
		liftheavy_carrier = clonk;

		AddEffect("IntLiftHeavy", liftheavy_carrier, 1, 1, this);
	}
}

static const lift_heavy_time = 60;

// ------------------
// Lifting the object
// ------------------
func FxIntLiftHeavyStart(object clonk, proplist effect, bool tmp)
{
	if(tmp) return;
	if(!clonk) return -1;
	if(Contained() != clonk) return -1;

	//Stop the clonk from moving, and tell the clonk's control library
	//it now has a hand action
	clonk->SetTurnForced(clonk->GetDir());
	clonk->SetHandAction(1);
	clonk->SetAction("Stand");

	//Attach the mesh of the object. It is not displayed normally because the
	//hands are told they have an action in the next few lines
	liftheavy_mesh = clonk->AttachMesh(this->GetID(), "pos_tool1", "main", this->~GetCarryTransform(clonk));

	//Play the animation of the clonk picking up the object
	liftheavy_anim = clonk->PlayAnimation("CarryArmsPickup", 10, Anim_Linear(0,0,clonk->GetAnimationLength("CarryArmsPickup"), lift_heavy_time, ANIM_Remove), Anim_Const(1000));
	
	effect.noExit = true;
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
				effect.noExit = false;
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
				effect.noExit = false;
				return -1;
			}
		}
	}
		
	//When the clonk has finished lifting, remove movement-restrictions and add carry effect
	if(timer >= lift_heavy_time)
	{
		AddEffect("IntCarryHeavy", clonk, 1, 1, this);
		return -1;
	}
}

func FxIntLiftHeavyStop(object clonk, proplist effect, int reason, bool tmp)
{
	if(tmp) return;
	UndoLift(clonk, effect.noExit);
}

// -------------------
// Carrying the object
// -------------------
func FxIntCarryHeavyTimer(object clonk, proplist effect, int timer)
{
/*
	//Is there more than one carry-heavy object in the clonk? Then exit one
	if(clonk->GetItemPos(this) !=0 && ObjectCount(Find_Func("IsCarryHeavy"), Find_Container(clonk)) > 1)
		this->Exit();

	//If the carry-heavy object is not in the first hand slot, move it there
	if(clonk->GetItemPos(this) != 0)
		clonk->Switch2Items(0,clonk->GetItemPos(this));

	//Is there an object in the second hand slot? If so, remove it.
	if(clonk.inventory[1])
	{
		//exclude carry-heavy objects from the counting process. Only counts normal items
		var contentscount = clonk->ContentsCount();
		if(FindObject(Find_Container(clonk), Find_Func("IsCarryHeavy"))) contentscount--;

		if(contentscount < clonk->MaxContentsCount())
		{
			this->EmptyHandSlot(clonk, 1);

			//If the clonk had an object dropped into his inventory by other means than collection, drop it
			if(clonk.inventory[1])
			{
				clonk.inventory[1]->Exit();
				return -1;
			}
		}
	}
*/
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

	clonk->SetTurnForced(clonk->GetDir());
	clonk->SetHandAction(1);
	clonk->SetAction("Stand");

	//Attach the mesh of the object. It is not displayed normally because the
	//hands are told they have an action in the next few lines
	liftheavy_mesh = clonk->AttachMesh(this->GetID(), "pos_tool1", "main", this->~GetCarryTransform(clonk));

	//Play the animation of the clonk setting down the object
	liftheavy_anim = clonk->PlayAnimation("CarryArmsPickup", 10, Anim_Linear(clonk->GetAnimationLength("CarryArmsPickup"),clonk->GetAnimationLength("CarryArmsPickup"),0, lift_heavy_time, ANIM_Remove), Anim_Const(1000));
	//liftheavy_anim = clonk->PlayAnimation("CarryArmsSetdown", 10, Anim_Linear(0,0,clonk->GetAnimationLength("CarryArmsSetdown"), lift_heavy_time, ANIM_Remove), Anim_Const(1000));
}

func FxIntDropHeavyTimer(object clonk, proplist effect, int timer)
{
	//Clonk was interrupted?
	if(clonk->GetAction() != "Stand")
		return -1;

	// animation finished?
	if(timer >= lift_heavy_time)
		return -1;
}

func FxIntDropHeavyStop(object clonk, proplist effect, int reason, bool tmp)
{
	if(tmp) return;
	
	UndoLift(clonk);
}

func UndoLift(object clonk, bool noExit)
{
	//allow the clonk to turn again
	clonk->SetTurnForced(-1);
	clonk->SetHandAction(0);
	if(clonk->GetAction() == "Stand") clonk->SetAction("Walk");
	clonk->DetachMesh(liftheavy_mesh);
	//Stop the lifting animation
	clonk->StopAnimation(liftheavy_anim);
	if(!noExit)
		clonk->SetCommand("Drop",this);
}

func Drop()
{
	if(liftheavy_carrier == nil)
		return;
	if(!IsCarryingHeavy(liftheavy_carrier))
		return;
	
	AddEffect("IntDropHeavy", liftheavy_carrier, 1, 1, this);
}

func IsCarryingHeavy(object clonk)
{
	//Is the clonk in the process of lifting or carrying a heavy object?
	if(GetEffect("IntLiftHeavy", clonk)) return 1;
	if(GetEffect("IntCarryHeavy", clonk)) return 2;
	if(GetEffect("IntDropHeavy", clonk)) return 3;
	return false;
}

// Cannot pickup other carryheavy objects (is that really what you intended, Ringwaul?)
func RejectCollect(id collectid, object collect)
{
	if(collect->~IsCarryHeavy()) return true;
	else
		return false;
}