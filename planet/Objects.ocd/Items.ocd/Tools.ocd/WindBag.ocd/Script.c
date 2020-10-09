/**
	Windbag
	Automatically collects air when it is full which can be released into a blast.
	
	@author MimmoO, Maikel
*/

local fill_amount;

/*-- Engine Callbacks --*/

func Initialize()
{
	SetR(-45);
	AddEffect("IntReload", this, 100, 1, this);
}

func Hit()
{
	Sound("Hits::GeneralHit?");
}

/*-- Usage --*/

public func DefaultCrosshairAngle(object clonk, int d)
{
	// Easy mode for gamepad users: automatically boost a jump.
	if (clonk->GetYDir() < -10)
		return Angle(0, 0, -clonk->GetXDir(), -clonk->GetYDir(), 10);
	return 0;
}

protected func ControlUse(object clonk, x, y)
{
	if (!GetEffect("IntReload", this) && !GetEffect("IntBurstWind", this))
	{
		if (!GBackLiquid())
			BlastWind(clonk, x, y);
		return true;
	}
	clonk->Message("$MsgReloading$");
	clonk->PauseUse(this, "ReadyToBeUsed", {clonk = clonk});
	return true;
}

func ReadyToBeUsed(proplist data)
{
	return !GetEffect("IntReload", this);
}

/*-- Loading --*/

public func DoFullLoad()
{
	fill_amount = MaxIntake;
	return;
}

func FxIntReloadStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.Interval = 1;
	effect.sound = false;
	return FX_OK;
}

func FxIntReloadTimer(object target, proplist effect, int time)
{
	if (fill_amount > MaxIntake)
		return FX_Execute_Kill;
		
	if (GBackSolid(0, 0) || GBackLiquid(0, 0))
	{
		if (effect.sound)
		{
			Sound("Objects::Windbag::Charge", {loop_count = -1});
			Sound("Objects::Windbag::ChargeStop");
			effect.sound = false;
		}
		return FX_OK;
	}

	var radius = RandomX(12, 24);
	var angle = Random(360);
	var angle_var = RandomX(-25, 25);
	var x = Sin(angle + angle_var, radius);
	var y = Cos(angle + angle_var, radius);
	// Check for a spot of air from which to take the air in.
	if (!GBackSolid(x, y) && !GBackLiquid(x, y) && !GBackSolid(0, 0) && !GBackLiquid(0, 0)) 
	{
		if (!effect.sound)
		{
			Sound("Objects::Windbag::Charge", {loop_count = 1});
			effect.sound = true;
		}
		
		// Particles from the point where the air is sucked in.
		var air = {
			Prototype = Particles_Air(), 
			Size = PV_KeyFrames(0, 0, 0, 250, 3, 1000, 0)
		};
		CreateParticle("Air", x, y, -2 * x / 3, -2 * y / 3, 15, air);
		
		// Increase the fill amount proportional to the number of frames.
		fill_amount += effect.Interval; 
	}
	return FX_OK;
}

func FxIntReloadStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	if (effect.sound)
	{
		Sound("Objects::Windbag::Charge", {loop_count = -1});
		Sound("Objects::Windbag::ChargeStop");
	}
	return FX_OK;
}

/*-- Blasting --*/

public func BlastWind(object clonk, int x, int y)
{
	if (fill_amount <= 0)
	{
		fill_amount = 0;
		AddEffect("IntReload", this, 100, 1, this);
		return;
	}	
	// The blast is handled by an effect.
	AddEffect("IntBurstWind", this, 100, 1, this, nil, clonk, x, y);
}

func FxIntBurstWindStart(object target, proplist effect, int temp, object clonk, int x, int y)
{
	if (temp)
		return FX_OK;
	effect.Interval = 1;
	effect.clonk = clonk;
	effect.x = clonk->GetX();
	effect.y = clonk->GetY();
	effect.angle = Angle(0, 0, x, y);
	// Sound effect.
	Sound("Objects::Windbag::Gust");
	// Particle effect.
	for (var dr = 12; dr < 32; dr++)
	{
		var r = RandomX(-20, 20);
		var sx = Sin(effect.angle + r, dr / 2);
		var sy = -Cos(effect.angle + r, dr / 2);
		var vx = Sin(effect.angle + r, 2 * fill_amount / 3 + 12);
		var vy = -Cos(effect.angle + r, 2 * fill_amount / 3 + 12);
		if (!GBackSolid(sx, sy))
			CreateParticle("Air", sx, sy, vx, vy, 36, Particles_Air());
	}
	// Make a timer call for the instant movement effect.
	FxIntBurstWindTimer(target, effect, 0);
	return FX_OK;
}

func FxIntBurstWindTimer(object target, proplist effect, int time)
{
	var real_time = time + 1;
	if (real_time > 8)
		return FX_Execute_Kill;
		
	// Determine blast strength.	
	var vx = Sin(effect.angle, 20 * fill_amount + 150);
	var vy = -Cos(effect.angle, 20 * fill_amount + 150);
	
	// Change the velocity of the shooter.
	if (effect.clonk && effect.clonk->GetAction() != "Walk")
	{									
		var cx = effect.clonk->GetXDir(100);
		var cy = effect.clonk->GetYDir(100);
		effect.clonk->SetXDir(cx - vx / (8 * real_time), 100);
		effect.clonk->SetYDir(cy - vy / (8 * real_time), 100);
	}
	
	// Move other objects in a cone around the burst direction.
	var criteria = Find_And(Find_Not(Find_Category(C4D_Structure)), Find_Not(Find_Func("IsEnvironment")), Find_Not(Find_Func("RejectWindbagForce")),
	                        Find_Layer(GetObjectLayer()), Find_NoContainer(), Find_Exclude(effect.clonk), Find_PathFree(effect.clonk));
	var dist = 14 + 9 * real_time / 2;
	var rad = 8 + 8 * real_time / 3;
	var cone_x = Sin(effect.angle, dist);
	var cone_y = -Cos(effect.angle, dist);
	var vx_cone = vx / 6;
	var vy_cone = vy / 6;
	//DrawParticleRing(rad, cone_x + AbsX(effect.x), cone_y + AbsY(effect.y));
	for (var obj in FindObjects(Find_Distance(rad, cone_x + AbsX(effect.x), cone_y + AbsY(effect.y)), criteria))
	{
		var ox = obj->GetXDir(100);
		var oy = obj->GetYDir(100);
		var vx_cone_reduced = vx_cone / 2 + vx_cone / (2 * Max(1, obj->GetMass() / 4));
		var vy_cone_reduced = vy_cone / 2 + vy_cone / (2 * Max(1, obj->GetMass() / 4)); 
		obj->SetXDir(ox + vx_cone_reduced, 100);
		obj->SetYDir(oy + vy_cone_reduced, 100);
		obj->SetController(GetController());
	}
	return FX_OK;
}

func FxIntBurstWindStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	// Reset the fill amount and start reloading.
	fill_amount = 0;
	AddEffect("IntReload", target, 100, 1, target);
	return FX_OK;
}

func DrawParticleRing(int r, int x, int y)
{
	for (var angle = 0; angle < 360; angle += 15)
		CreateParticle("SphereSpark", x + Cos(angle, r), y + Sin(angle, r), 0, 0, 36, { Size = 2 });
}

/*-- Production --*/

public func IsInventorProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk)
{
	return CARRY_Blunderbuss;
}

public func GetCarryTransform(object clonk, bool idle, bool nohand, bool second_on_back)
{
	if (idle)
	{
		if (!second_on_back)
			return Trans_Mul(Trans_Rotate(180, 1), Trans_Translate(0,-3000));
		else
			return Trans_Mul(Trans_Rotate(180, 1), Trans_Translate(3000,-3000), Trans_Rotate(-30, 0, 1));
	}
	if (nohand)
		return Trans_Mul(Trans_Rotate(180, 1), Trans_Translate(0, 3000));

	return Trans_Mul(Trans_Rotate(220, 0, 1, 0), Trans_Rotate(30, 0, 0, 1), Trans_Rotate(-26, 1, 0, 0));
}

public func GetCarryPhase() { return 600; }


/*-- Saving --*/

public func SaveScenarioObject(props, ...)
{
	if (!inherited(props, ...)) return false;
	// Save fully loaded.
	if (fill_amount == this.MaxIntake) props->AddCall("FullLoad", this, "DoFullLoad");
	return true;
}


public func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Scale(1500), Trans_Rotate(150, 0, 0, 1), Trans_Rotate(-170, 1, 0, 0), Trans_Rotate(10, 0, 1, 0)), def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local MaxIntake = 30;
local Components = {Cloth = 1, Metal = 1};
