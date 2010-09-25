/*-- Club --*/

#include Library_MeleeWeapon

private func Hit()
{
	Sound("WoodHit");
}

public func GetCarryMode() { return CARRY_HandBack; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

local animation_set;

func Initialize()
{
	animation_set = {
		AimMode         = AIM_Weight,
		AnimationAim    = "BatAimArms",
		AnimationAim2   = "BatAim2Arms",
		AimTime         = 35*3,
		AnimationShoot  = "BatStrikeArms",
		AnimationShoot2 = "BatStrike2Arms",
		ShootTime       = 35/2,
		ShootTime2      = (35/2)*6/19, // At 6/19 of the shooting animation
	};
}

public func GetAnimationSet() { return animation_set; }

public func HoldingEnabled() { return true; }

local fAiming;

public func ControlUseStart(object clonk, int x, int y)
{
	// cooldown?
	if(!CanStrikeWithWeapon(clonk)) return 1;
	
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
		return true;

	fAiming = true;

	clonk->StartAim(this);
	return 1;
}

public func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);

	return true;
}

protected func ControlUseStop(object clonk, ix, iy)
{
	clonk->StopAim();
	return true;
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	clonk->StartShoot(this);
	
	// since the Clonk internal callback is only once, we cannot use it
	// ..
	AddEffect("DuringClubShoot", clonk, 1, 1, this, nil, angle);
	
	// aaaand, a cooldown
	AddEffect("ClubWeaponCooldown", clonk, 1, 5, this);
	return true;
}

protected func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

public func Reset(clonk)
{
	fAiming = 0;
}

// Called in the half of the shoot animation (when ShootTime2 is over)
public func DuringShoot(object clonk, int angle)
{
	// called only once. We don't want it only once..
	// DoStrike(clonk, angle);
}

func FxDuringClubShootStart(target, effect_number, temp, p1)
{
	if(temp) return;
	EffectVar(0, target, effect_number)=p1;
}

func FxDuringClubShootTimer(target, effect_number, effect_time)
{
	if(effect_time > 16) return -1;
	if(!this) return -1;
	this->DoStrike(target, EffectVar(0, target, effect_number));
}

func DoStrike(clonk, angle)
{
	var x=Sin(angle, 7);
	var y=-Cos(angle, 7);
	var found=false;
	for(var obj in FindObjects(Find_Distance(7, x, y), Find_Or(Find_OCF(OCF_Alive), Find_Category(C4D_Object)), Find_Exclude(clonk), Find_NoContainer(), Find_Layer(GetObjectLayer())))
	{
		if(obj->Stuck()) continue;
		
		var en=Format("CannotBeHitTwiceBy%d", this->ObjectNumber());
		if(GetEffect(en, obj)) continue;
		
		if(obj->GetOCF() & OCF_Alive)
		{
			var damage=5*1000;
			var f=ApplyShieldFactor(clonk, obj, damage);
			ApplyWeaponBash(obj, 200, angle);
			obj->DoEnergy(-damage, true, FX_Call_EngGetPunched, clonk->GetOwner());
		}
		else
		{
			var div=100;
			if(obj->GetContact(-1)) div*=10;
			
			// mass factor
			var fac1=10000/Max(2, obj->GetMass());
			var fac2=BoundBy(10-Abs(obj->GetDefCoreVal("Width", "DefCore")-obj->GetDefCoreVal("Height", "DefCore")), 1, 10);
			var speed=(2000 * fac1 * fac2) / 10 / 1000;
			obj->SetXDir((obj->GetXDir(100) + Sin(angle, speed)) / 2, div);
			obj->SetYDir((obj->GetYDir(100) - Cos(angle, speed)) / 2, div);
		}
		AddEffect(en, obj, 1, 15, 0);
		found=true;
		break;
	}
	
	if(found)
		RemoveEffect("DuringClubShoot", clonk);
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Rotate(-30,0,0,1),def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
