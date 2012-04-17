/*
	Axe
	Author: Ringwaul, Clonkonaut

	Used for chopping down trees. Can also harvest
	wood from fallen trees, but will not yield as
	many logs as a sawmill.
*/

#include Library_MeleeWeapon

local tree;
local swing_anim;
local using;
local carry_bone;
local magic_number;

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
	return carry_bone;
}

public func ControlUseStart(object clonk, int iX, int iY)
{
	// Can clonk use the axe?
	if (!clonk->IsWalking() && !clonk->IsJumping())
		return true;

	if(Distance(0,0,iX,iY) < 35)
		tree = FindObject(Find_AtPoint(iX,iY), Find_Func("IsTree"), Sort_Distance(), Find_NoContainer());
	
	if(!tree)
		tree = FindObject(Find_AtPoint(0,0), Find_Func("IsTree"), Sort_Distance(), Find_NoContainer());

	// Chopping
	if(tree && clonk->IsWalking())
	{
		//treedist - the x-distance the clonk is from the centre of a tree-trunk
		var treedist = Abs(clonk->GetX() - tree->GetX());
		if(tree->IsStanding() && treedist < 15 && treedist > 6)
		{
			using = 1;
			//Set the clonk's dir to face the tree if he isn't
			if(clonk->GetX() < tree->GetX() && clonk->GetDir() == 0)
				clonk->SetDir(1);
			else if(clonk->GetX() > tree->GetX() && clonk->GetDir() == 1)
				clonk->SetDir(0);

			//The clonk cannot hold other items in hand while swinging an axe
			clonk->SetHandAction(1);

			//Update the axe position in the clonk' hands and disable turning while he chops the tree
			clonk->UpdateAttach();
			clonk->SetTurnForced(clonk->GetDir());

			//Make sure the clonk is holding the axe in the correct position
			var hand = "Chop.R";
			if(clonk->GetDir() == 0) hand = "Chop.L";
			swing_anim = clonk->PlayAnimation(hand, 10, Anim_Linear(0, 0, clonk->GetAnimationLength("Chop.R"), axe_swing_time, ANIM_Loop), Anim_Const(1000));

			//The timed effect for when the axe actually hits the tree
			AddEffect("IntAxe", clonk, 1, 1, this, 0, tree);
			return true;
		}
		if(! tree->IsStanding())
		{
			// Tree has already been felled
			using = 1;

			//The clonk cannot hold other items in hand while swinging an axe
			clonk->SetHandAction(1);

			//Refresh hands
			clonk->UpdateAttach();

			//Make sure the clonk is holding the axe in the correct position
			var hand = "Chop.R";
			if(clonk->GetDir() == 0) hand = "Chop.L";
			swing_anim = clonk->PlayAnimation(hand, 10, Anim_Linear(0, 0, clonk->GetAnimationLength("Chop.R"), axe_swing_time, ANIM_Loop), Anim_Const(1000));

			//clonk cannot turn around to face the screen while chopping
			clonk->SetTurnForced(clonk->GetDir());

			//The timed effect for when the axe actually hits the tree
			AddEffect("IntSplit", clonk, 1, 1, this, 0, tree);
			return true;
		}
	}

	// Combat
	if (!CanStrikeWithWeapon(clonk)) return true;

	// if the clonk doesn't have an action where he can use it's hands do nothing
	if (!clonk->HasHandAction())
		return true;

	var rand = Random(2)+1;
	var arm = "R";
	var animation = Format("SwordSlash%d.%s", rand, arm);
	var length = 15;
	carry_bone = "pos_hand2";

	if(clonk->IsWalking())
	{
		if(!GetEffect("AxeStrikeStop", clonk, 0))
			AddEffect("AxeStrikeStop", clonk, 2, 50, this);
	}
	if(clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
		animation  = Format("SwordSlash%d.%s", rand, arm);
	}
	if(clonk->IsJumping())
	{
		rand = 1;
		if(clonk->GetYDir() < -5) rand = 2;
		animation = Format("SwordJump%d.%s",rand,arm);
	}

	PlayWeaponAnimation(clonk, animation, 10, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
	clonk->UpdateAttach();

	magic_number=((magic_number+1)%10) + (ObjectNumber()*10);
	StartWeaponHitCheckEffect(clonk, length, 1);

	return true;
}

/* Chopping */

protected func HoldingEnabled() { return GetEffect("IntAxe", this); }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Can clonk use axe?
	if (!clonk->IsWalking() || GetXDir() != 0)
	{
		clonk->CancelUse();
		return true;
	}
	return true;
}

/* Chopping effect */

func FxIntAxeStart(object clonk, effect, int temp, object target_tree)
{
	if (temp) return;
	effect.tree = target_tree;
}

func FxIntAxeTimer(object clonk, effect, int time)
{
	// What happen...?
	if (this->Contained() != clonk) return -1;
	// Tree vanished
	if (!effect.tree) return -1;
	// Tree fell
	if (!effect.tree->IsStanding()) return -1;
	// Clonk did something
	if (!clonk->IsWalking()) return -1;

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

		// Damage tree
		tree->DoDamage(this.ChopStrength, 3, clonk->GetOwner()); // 3 = FX_Call_DmgChop
	}
	//Make sure the clonk does not move
	clonk->SetComDir(COMD_Stop);
}

func FxIntAxeStop(object clonk, effect, int temp)
{
	if (temp) return;
	if (this->Contained() == clonk) Reset(clonk);
}

/* Splitting effect */

func FxIntSplitStart(object clonk, effect, int temp, object target_tree)
{
	if (temp) return;
	effect.tree = target_tree;
}

func FxIntSplitTimer(object clonk, effect, int time)
{
	// What happen...?
	if (this->Contained() != clonk) return -1;
	// Tree vanished
	if (!effect.tree) return -1;
	// Tree moved away
	if (ObjectDistance(effect.tree, clonk) > Distance(0,0, effect.tree->GetObjWidth()/2, effect.tree->GetObjHeight()/2)) return -1;
	// Clonk did something
	if (!clonk->IsWalking()) return -1;

	//This block is executed when the axe hits the tree
	if ((time + 25) % axe_swing_time == 1)
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
	// Tree split!
	if ((axe_swing_time * 12) / time == 1)
	{
		var wood_count = effect.tree->GetComponent(Wood) / 2;
		CastObjects(Wood, wood_count, 5, AbsX(effect.tree->GetX()), AbsY(effect.tree->GetY()));
		effect.tree->RemoveObject();
	}
	//Make sure the clonk does not move
	clonk->SetComDir(COMD_Stop);
}

func FxIntSplitStop(object clonk, effect, int temp)
{
	if (temp) return;
	if (this->Contained() == clonk) Reset(clonk);
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
	clonk->SetHandAction(0);
	clonk->UpdateAttach();
	clonk->SetTurnForced(-1);
	clonk->StopAnimation(swing_anim);
	swing_anim = nil;
	tree = nil;
	RemoveEffect("IntAxe", clonk);
	RemoveEffect("IntSplit", clonk);
}

/* Combat */

func CheckStrike(iTime)
{
	var  offset_x=7;
	var offset_y=0;
	if(Contained()->GetDir() == DIR_Left) offset_x*=-1;

	if(!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y=10;

	var width=10;
	var height=20;

	for(var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height),
							   Find_NoContainer(),
							   Find_Exclude(Contained()),
							   Find_Layer(GetObjectLayer())))
	{
		if (obj->~IsProjectileTarget(this, Contained()) || obj->GetOCF() & OCF_Alive)
		{
			var effect_name=Format("HasBeenHitByAxeEffect%d", magic_number);
			var axe_name=Format("HasBeenHitByAxe%d", this->ObjectNumber());
			var first=true;
			// don't hit objects twice
			if(!GetEffect(effect_name, obj))
			{
				AddEffect(effect_name, obj, 1, 60 /* arbitrary */, nil, 0);

				if(GetEffect(axe_name, obj))
				{
					first=false;
				}
				else
				{
					AddEffect(axe_name, obj, 1, 40, nil, 0);
				}

				// Reduce damage by shield
				var shield=ApplyShieldFactor(Contained(), obj, 0); // damage out of scope?
				if(shield == 100)
					continue;

				// fixed damage (3)
				var damage=((100-shield)*3*1000 / 100);
				ProjectileHit(obj, damage, ProjectileHit_no_query_catch_blow_callback | ProjectileHit_exact_damage | ProjectileHit_no_on_projectile_hit_callback, FX_Call_EngGetPunched);

				if (obj)
					DoWeaponSlow(obj, 200);

				// sound and done. We can only hit one target
				Sound("WeaponHit?", false);
				break;
			}
		}
	}
}

func WeaponStrikeExpired()
{
	if(GetEffect("AxeStrikeStop", Contained()))
		RemoveEffect("AxeStrikeStop", Contained());
}

func OnWeaponHitCheckStop(clonk)
{
	carry_bone = nil;
	clonk->UpdateAttach();
}

func FxAxeStrikeStopStart(pTarget, effect, iTemp)
{
	if(iTemp) return;
	pTarget->PushActionSpeed("Walk", (pTarget.ActMap.Walk.Speed)/100);
}

func FxAxeStrikeStopStop(pTarget, effect, iCause, iTemp)
{
	if(iTemp) return;
	pTarget->PopActionSpeed("Walk");
}

func FxAxeStrikeStopTimer(pTarget, effect)
{
	return 1;
}


public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
local ChopStrength = 10;