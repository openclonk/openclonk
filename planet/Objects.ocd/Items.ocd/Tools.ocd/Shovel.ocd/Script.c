/**
	Shovel
	Essential tool for the clonk, used to dig through materials.
	
	@author: Newton, Sven2, Zapper, Maikel
*/

#include Library_Flammable

local is_digging;

/*-- Engine Callbacks --*/

func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

func Destruction()
{
	// Stop shoveling when using the shovel when it's destroyed.
	var user = Contained();
	if (user)
		if (user->GetAction() == "Dig" && user->~GetUsedObject() == this)
		{
			user->CancelUse();
			user->SetAction("Walk");
			user->SetComDir(COMD_Stop);
		}
}

/*-- Usage --*/

public func IsDigging() { return is_digging; }

public func HoldingEnabled() { return true; }

public func DefaultCrosshairAngle(object clonk, int d) { return 900 * d; }

public func ControlUseStart(object clonk, int x, int y)
{
	AddEffect("ShovelDig", clonk, 1, 1, this);

	// Temporary workaround to allow clonks to dig free when they are stuck in dirt.
	if (clonk->Stuck())
		DigFree(clonk->GetX(), clonk->GetY(), 10);
	
	// Initial coordinate setup.
	ControlUseHolding(clonk, x, y);
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	var dig_effect = GetEffect("ShovelDig", clonk);
	if (dig_effect)
	{
		dig_effect.dig_x = x;
		dig_effect.dig_y = y;		
		dig_effect.dig_angle = Angle(0, 0, x, y);	
	
	}
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	return ControlUseStop(clonk, x, y);
}

public func ControlUseStop(object clonk, int x, int y)
{
	is_digging = false;
	RemoveEffect("ShovelDig", clonk);
	
	if (clonk->GetAction() != "Dig")
		return true;

	clonk->SetXDir(0);
	clonk->SetYDir(0);
	return true;
}

public func FxShovelDigTimer(object clonk, effect fx, int time)
{
	// Left the clonk?
	if (Contained() != clonk)
	{
		if (clonk->GetAction() == "Dig")
		{
			clonk->SetAction("Walk");
			clonk->SetComDir(COMD_Stop);
		}
		return FX_Execute_Kill;
	}
	var xdir_boost = 0, ydir_boost = 0;
	// Currently not digging?
	if (clonk->GetAction() != "Dig" || clonk->GBackLiquid(0, -4))
	{
		var is_scaling = (clonk->GetProcedure() == "SCALE");
		var can_dig = (clonk->GetAction() == "Walk" || is_scaling || clonk->GetProcedure() == "HANGLE");
		// Prevent clonk from starting to dig if in deep liquid.
		if (clonk->GBackLiquid(0, -4))
			can_dig = false;
		if (can_dig)
		{
			clonk->SetAction("Dig");
			clonk->SetComDir(COMD_Stop);
			if (is_scaling)
			{
				// Speed boost when Clonk started digging from scaling, so we don't drop down.
				var clnk_xdir = clonk->GetDir() * 2 - 1;
				// Only if player actually wants to go sideways in the scaling direction (|x|>|y| and sign(x)==sign(clnk_xdir)).
				if (Abs(fx.dig_x) > Abs(fx.dig_y) && fx.dig_x * clnk_xdir > 0) 
				{
					// Not if standing on ground (to prevent speed boost digging) or on ceiling (to make working your way upwards through earth a little harder).
					if (!clonk->GetContact(-1, CNAT_Top | CNAT_Bottom))
					{
						xdir_boost = clnk_xdir * 1000;
						ydir_boost = -100;
					}
				}
			}
		}
		else
		{
			if (is_digging)
			{
				is_digging = false;
			}
			return FX_OK;
		}
	}
	// Dig start procedure
	if (!is_digging && clonk->GetAction() == "Dig")
	{
		is_digging = true;
		fx.stuck_count = 0;
	}
	if (is_digging)
	{
		// Get animation-adjusted dig speed.
		var speed = this->GetDigSpeed(clonk);
		
		// Limit the digging angle: one can dig maximally 30 degrees upwards.
		fx.dig_angle = BoundBy(fx.dig_angle, 60, 300);
		// Also snap the dig angle to straight lines at 0, 90, 180 and 270 degrees with a 5 degree margin.
		var margin = 5;
		if (Inside(fx.dig_angle % 90, 0, margin) || Inside(fx.dig_angle % 90, 90 - margin, 90))
			fx.dig_angle = 90 * ((fx.dig_angle + margin) / 90);
		
		var dig_xdir = Sin(fx.dig_angle, speed);
		var dig_ydir = -Cos(fx.dig_angle, speed);

		// Stuck-check: When player wants to go horizontally while standing on the ground but can't, add a vertical redirect
		// This helps with Clonks getting "stuck" not moving on certain terrain when dig speed is not sufficient to push him up a slope
		// It also helps e.g. when just digging through earth just above rock, because it will nudge the clonk over the rock
		if (!clonk->GetXDir(100) && !clonk->GetYDir(100) && GetContact(-1, CNAT_Bottom) && Abs(dig_xdir) > Abs(dig_ydir) && Abs(dig_xdir) > 10)
		{
			if (fx.stuck_count++)
				ydir_boost -= 100;
		}
		else
		{
			fx.stuck_count = 0;
		}
		
		clonk->SetXDir(dig_xdir + xdir_boost, 100);
		clonk->SetYDir(dig_ydir + ydir_boost, 100);

		// Dust
		if (!Random(10))
			Dust(clonk);
	}
	return FX_OK;
}

func GetDigSpeed(object clonk)
{
	var speed = clonk.ActMap.Dig.Speed;
	if (clonk->GetActTime() <= clonk.ActMap.Dig.Length * 3 / 4)
	{
		// Adjust speed at current animation position.
		var animation = clonk->GetDiggingAnimation();
		var position = clonk->GetAnimationPosition(animation) * 180 / clonk->GetAnimationLength("Dig");
		speed = 2 * speed * Cos(position - 45, 50)**2 / 2500;
	}
	return speed;
}

public func Dust(object target)
{
	// Only when the clonk moves the shovel.
	var animation = target->GetDiggingAnimation();
	var position = target->GetAnimationPosition(animation) * 100 / target->GetAnimationLength("Dig");
	if (position > 50)
		return;
	var xdir = target->GetXDir();
	var ydir = target->GetYDir();
	
	// Particle effect.
	var angle = Angle(0, 0, xdir, ydir) + position - 25;
	var groundx = Sin(angle, 15);
	var groundy = -Cos(angle, 15);
	var mat = GetMaterial(groundx, groundy);
	var tex = GetTexture(groundx, groundy);
	if (GetMaterialVal("DigFree", "Material", mat))
	{
		var clr = GetAverageTextureColor(tex);
		var particles =
		{
			Prototype = Particles_Dust(),
			R = (clr >> 16) & 0xff,
			G = (clr >> 8) & 0xff,
			B = clr & 0xff,
			Size = PV_KeyFrames(0, 0, 0, 300, 20, 1000, 7),
		};
		CreateParticle("Dust", groundx / 2, groundy / 2, PV_Random(-6, 6), PV_Random(-6, 6), PV_Random(18, 36), particles, 8);
	}
	return;
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	if (!idle)
		return CARRY_Hand;
	else
		return CARRY_Back;
}

public func GetCarrySpecial(object clonk, bool idle)
{
	if (idle) return;
	if (clonk->~GetAction() == "Dig") return "pos_hand1";
}

func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Rotate(135, 0, 0, 1), Trans_Rotate(30, 0, 1, 0));
}

/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local Components = {Wood = 1, Metal = 1};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
local BurnDownTime = 140;