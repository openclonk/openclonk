/*
	Axe
	Author: Ringwaul

	Used for chopping down trees. Can also harvest
	wood from fallent trees, but will not yield as
	many logs as a sawmill.
*/

local tree;
local swing_anim;
local using;

static const axe_swing_time = 30;

private func Hit(int x, int y)
{
	StonyObjectHit(x,y);
	return 1;
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{
	var act = Contained()->GetAction();
	if(act != "Walk" && act != "Jump")
		return Trans_Mul(Trans_Translate(0,4500,0), Trans_Rotate(-90,0,1,0), Trans_Rotate(180,0,0,1) );

	return Trans_Rotate(90, 0, 1, 0);
}
public func GetCarrySpecial(clonk)
{
	if(using == 1) 
	{
		if(clonk->GetDir() == 1)
			return "pos_hand2";
		else
			return "pos_hand1";
	}
}

public func ControlUseStart(object clonk, int iX, int iY)
{
	// Can clonk use pickaxe?
	if (clonk->GetProcedure() != "WALK")
		return true;
	using = 1;

	tree = FindObject(Find_AtPoint(0,0), Find_Func("IsTree"), Sort_Distance(), Find_NoContainer());

	if(tree)
	{
		//Set the clonk's dir to face the tree if he isn't
		if(clonk->GetX() < tree->GetX() && clonk->GetDir() == 0)
			clonk->SetDir(1);
		else if(clonk->GetX() > tree->GetX() && clonk->GetDir() == 1)
			clonk->SetDir(0);

		//treedist - the x-distance the clonk is from the centre of a tree-trunk
		var treedist = Abs(clonk->GetX() - tree->GetX());
		if(tree->IsStanding() == true && treedist < 15 && treedist > 6)
		{
			// Create an offset, so that the hit matches with the animation
			clonk->UpdateAttach();
			clonk->SetTurnForced(clonk->GetDir());

			//Make sure the clonk is holding the axe in the correct position
			var hand = "Chop.R";
			if(clonk->GetDir() == 0) hand = "Chop.L";
			swing_anim = clonk->PlayAnimation(hand, 10, Anim_Linear(0, 0, clonk->GetAnimationLength("Chop.R"), axe_swing_time, ANIM_Loop), Anim_Const(1000));

			//The timed effect for when the axe actually hits the tree
			AddEffect("IntAxe", clonk, 1, 1, this);
			return true;
		}
	}
	return 1;
}

protected func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Can clonk use axe?
	if (clonk->GetProcedure() != "WALK" || GetXDir() != 0)
	{
		clonk->CancelUse();
		return true;
	}
	return true;
}

func FxIntAxeTimer(clonk, effect, time)
{
	//This block is executed when the axe hits the tree
	if((time + 25) % axe_swing_time == 1)
	{
		Sound("Chop?");

		//Which direction does the clonk face?
		var x = 10;
		if(clonk->GetDirection() == COMD_Left) x = x * -1;
		
		//Create the woodchip particle
		var i;
		while(i != 4)
		{
			//random speed & angle
			i++;
			CreateParticle("Axe_WoodChip", x, 4, 5 - Random(11), RandomX(6,13) * -1, 20, RGB(255,255,255), tree);
		}
	}
	//Make sure the clonk does not move
	clonk->SetComDir(COMD_Stop);
}

func ControlUseStop(object clonk, int ix, int iy)
{
	Reset(clonk);
	return true;
}

protected func ControlUseCancel(object clonk, int ix, int iy)
{
	Reset(clonk);
	return true;
}

public func Reset(clonk)
{
	//Reset the clonk to normal control
	using = 0;
	clonk->UpdateAttach();
	clonk->SetTurnForced(-1);
	clonk->StopAnimation(swing_anim);
	swing_anim = nil;
	tree = nil;
	RemoveEffect("IntAxe", clonk);
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
