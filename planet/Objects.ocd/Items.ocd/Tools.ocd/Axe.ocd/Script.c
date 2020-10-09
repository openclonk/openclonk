/**
	Axe
	Used for chopping down trees. Can also harvest wood from fallen trees,
	but will not yield as many logs as a sawmill.
	
	@author: Ringwaul, Clonkonaut
*/

#include Library_MeleeWeapon

#include Library_Flammable

local swing_anim;
local using;
local carry_bone;
local magic_number;

local movement_effect;

/*-- Engine Callbacks --*/

func Hit(int x, int y)
{
	StonyObjectHit(x, y);
	return 1;
}

func Departure(object container)
{
	// Always end the movement impairing effect when exiting
	if (movement_effect)
	{
		RemoveEffect(nil, container, movement_effect);
		movement_effect = nil;
	}
}

/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !clonk->IsWalking() && !clonk->IsJumping();
}

// used by this object
func ReadyToBeUsed(proplist data)
{
	var clonk = data.clonk;
	return !RejectUse(clonk) && CanStrikeWithWeapon(clonk) && clonk->HasHandAction();
}

public func HoldingEnabled()
{
	return GetEffect("IntAxe", this);
}

public func ControlUseStart(object clonk, int iX, int iY)
{
	/* Chopping */
	
	// Find tree that is closest to the clonk's axe when swung
	var x_offs = 10;
	if (clonk->GetDir() == DIR_Left)
	{
		x_offs = -x_offs;
	}
	
	if (clonk->IsWalking()) for (var tree in FindObjects(Find_AtPoint(x_offs, 0), Find_Func("IsTree"), Sort_Distance(x_offs, 0), Find_NoContainer()))
		{
			//treedist - the x-distance the clonk is from the centre of a tree-trunk
			var treedist = Abs(clonk->GetX() + x_offs - tree->GetX());
			if (tree->IsStanding() && treedist <= 6)
			{
				using = 1;
				
				//The clonk cannot hold other items in hand while swinging an axe
				clonk->SetHandAction(1);
				
				//Update the axe position in the clonk' hands and disable turning while he chops the tree
				clonk->UpdateAttach();
				clonk->SetTurnForced(clonk->GetDir());
				
				//Make sure the clonk is holding the axe in the correct position
				var hand = "Chop.R";
			if ((clonk->GetDir() == 0) != (clonk.Plane < tree.Plane)) hand = "Chop.L";
			swing_anim = clonk->PlayAnimation(hand, CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength(hand), SwingTime, ANIM_Loop), Anim_Const(1000));

				//The timed effect for when the axe actually hits the tree
				AddEffect("IntAxe", clonk, 1, 1, this, nil, tree);
				return true;
			}
			if (!tree->IsStanding())
			{
				// Tree has already been felled
				using = 1;
				
				//The clonk cannot hold other items in hand while swinging an axe
				clonk->SetHandAction(1);
				
				//Refresh hands
				clonk->UpdateAttach();
				
				//Make sure the clonk is holding the axe in the correct position
				var hand = "Chop.R";
			if (clonk->GetDir() == 0) hand = "Chop.L";
			swing_anim = clonk->PlayAnimation(hand, CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength("Chop.R"), SwingTime, ANIM_Loop), Anim_Const(1000));

				//clonk cannot turn around to face the screen while chopping
				clonk->SetTurnForced(clonk->GetDir());
				
				//The timed effect for when the axe actually hits the tree
				AddEffect("IntSplit", clonk, 1, 1, this, nil, tree);
				return true;
			}
		}
	// Do combat strike if no tree can be found.
	return ControlUse(clonk, iX, iY);
}

// Strike with the axe.
public func ControlUse(object clonk, int iX, int iY)
{
	if (!CanStrikeWithWeapon(clonk) || !clonk->HasHandAction())
	{
		clonk->PauseUse(this, "ReadyToBeUsed", {clonk = clonk});
		return true;
	}
	
	var rand = Random(2) + 1;
	var arm = "R";
	var animation = Format("SwordSlash%d.%s", rand, arm);
	carry_bone = "pos_hand2";
	
	if (clonk->IsWalking())
	{
		if (!GetEffect("AxeStrikeStop", clonk, 0))
			AddEffect("AxeStrikeStop", clonk, 2, StrikingLength, this);
	}
	if (clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
		animation = Format("SwordSlash%d.%s", rand, arm);
	}
	if (clonk->IsJumping())
	{
		rand = 1;
		if (clonk->GetYDir() < -5)
		{
			rand = 2;
		}
		animation = Format("SwordJump%d.%s", rand, arm);
	}

	PlayWeaponAnimation(clonk, animation, 10, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), StrikingLength, ANIM_Remove), Anim_Const(1000));

	clonk->UpdateAttach();
	
	Sound("Objects::Weapons::WeaponSwing?", {pitch = -Random(10)});
	
	magic_number = ((magic_number + 1) % 10) + (ObjectNumber() * 10);
	StartWeaponHitCheckEffect(clonk, StrikingLength, 1);
	
	return true;
}

public func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Can clonk use axe?
	if (!clonk->IsWalking() || GetXDir() != 0)
	{
		clonk->CancelUse();
		return true;
	}
	return true;
}

func ControlUseCancel(object clonk, int ix, int iy)
{
	Reset(clonk);
	return true;
}

public func ControlUseStop(object clonk, int ix, int iy)
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
	RemoveEffect("IntAxe", clonk);
	RemoveEffect("IntSplit", clonk);
	RemoveEffect("AxeStrike", clonk);
}

/* Chopping */

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
	if ((time + 25) % SwingTime == 1)
	{
		Sound("Environment::Tree::Chop?");
		
		//Which direction does the clonk face?
		var x = 10;
		if (clonk->GetDirection() == COMD_Left)
			x = x * -1;
		
		//Create the woodchip particles
		var particles = Particles_WoodChip();
		// need to be behind the Clonk?
		if (clonk.Plane > effect.tree.Plane)
			particles = {Prototype = Particles_WoodChip(), Attach = ATTACH_Back};
		clonk->CreateParticle("WoodChip", x, 4, PV_Random(-12, 12), PV_Random(-13, -6), PV_Random(36 * 3, 36 * 10), particles, 10);
		// Damage tree
		effect.tree->DoDamage(this.ChopStrength, FX_Call_DmgChop, clonk->GetOwner());
	}
	//Make sure the clonk does not move
	clonk->SetComDir(COMD_Stop);
}

func FxIntAxeStop(object clonk, effect, int temp)
{
	if (temp) return;
	if (this->Contained() == clonk) Reset(clonk);
}

/* Splitting */

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
	if (ObjectDistance(effect.tree, clonk) > Distance(0, 0, effect.tree->GetObjWidth()/2, effect.tree->GetObjHeight()/2)) return -1;
	// Clonk did something
	if (!clonk->IsWalking()) return -1;

	//This block is executed when the axe hits the tree
	if ((time + 25) % SwingTime == 1)
	{
		Sound("Environment::Tree::Chop?");
		
		//Which direction does the clonk face?
		var x = 10;
		if (clonk->GetDirection() == COMD_Left) x = x * -1;

		//Create the woodchip particle
		clonk->CreateParticle("WoodChip", x, 4, PV_Random(-12, 12), PV_Random(-13, -6), PV_Random(36 * 3, 36 * 10), Particles_WoodChip(), 10);
	}
	// Tree split!
	if ((SwingTime * 12) / time == 1)
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

/* Combat */

func CheckStrike(iTime)
{
	var offset_x = 7;
	var offset_y = 0;
	if (Contained()->GetDir() == DIR_Left) offset_x*=-1;

	
	if (!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y = 10;
	
	var width = 10;
	var height = 20;
	
	for (var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height),
							   Find_NoContainer(),
							   Find_Exclude(Contained()),
							   Find_Layer(GetObjectLayer())))
	{
		if (obj->~IsProjectileTarget(this, Contained()))
		{
			var effect_name = Format("HasBeenHitByAxeEffect%d", magic_number);
			var axe_name = Format("HasBeenHitByAxe%d", this->ObjectNumber());
			var first = true;
			// don't hit objects twice
			if (!GetEffect(effect_name, obj))
			{
				AddEffect(effect_name, obj, 1, 60);
				
				if (GetEffect(axe_name, obj))
				{
					first = false;
				}
				else
				{
					AddEffect(axe_name, obj, 1, 40);
				}
				
				// Reduce damage by shield
				var shield = ApplyShieldFactor(Contained(), obj, 0); // damage out of scope?
				if (shield == 100)
					continue;
				
				// fixed damage (3)
				var damage = (100 - shield) * this.WeaponStrength * 1000 / 100;
				WeaponDamage(obj, damage, FX_Call_EngGetPunched, true);
				
				if (obj)
					DoWeaponSlow(obj, 200);
				
				// sound and done. We can only hit one target
				Sound("Objects::Weapons::WeaponHit?", false);
				break;
			}
		}
	}
}

func WeaponStrikeExpired()
{
	if (GetEffect("AxeStrikeStop", Contained()))
		RemoveEffect("AxeStrikeStop", Contained());
}

func OnWeaponHitCheckStop(clonk)
{
	carry_bone = nil;
	clonk->UpdateAttach();
}

func FxAxeStrikeStopStart(pTarget, effect, iTemp)
{
	if (iTemp) return;
	pTarget->PushActionSpeed("Walk", 10, GetID());
}

func FxAxeStrikeStopStop(pTarget, effect, iCause, iTemp)
{
	if (iTemp)
		return;
	pTarget->PopActionSpeed("Walk", GetID());
	movement_effect = nil;
}

func FxAxeStrikeStopTimer(pTarget, effect)
{
	return -1;
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	if (!idle)
		return CARRY_HandBack;
	else
		return CARRY_Belt;
}

public func GetCarryTransform(object clonk, bool idle)
{
	if (idle) return;

	var act = clonk->GetAction();
	
	if (act != "Walk" && act != "Jump")
		return Trans_Mul(Trans_Translate(4500, 0, 0), Trans_Rotate(90, 1, 0, 0), Trans_Rotate(180, 0, 1, 0));
	
	return Trans_Rotate(-90, 1, 0, 0);
}

public func GetCarrySpecial(clonk)
{
	if (using == 1)
	{
		if (clonk->GetDir() == 1)
			return "pos_hand2";
		else
			return "pos_hand1";
	}
	return carry_bone;
}

/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local Components = {Wood = 1, Metal = 1};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
local BurnDownTime = 140;
// Damage dealt to trees when chopping.
local ChopStrength = 10;
// Damage dealt to living beings when hit with an axe.
local WeaponStrength = 6;
// When using the axe to chop a tree.
local SwingTime = 30;
// When using the axe as a weapon (without trees).
local StrikingLength = 20; // in frames
