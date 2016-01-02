/*-- Shovel --*/

private func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

private func Destruction()
{
	if (Contained())
		if (Contained()->GetAction("Dig"))
		{
			// We assume that the clonk digs with this shovel. If not, too bad. You stop shoveling.
			Contained()->SetAction("Walk");
			Contained()->SetComDir(COMD_Stop);
		}
}

public func GetCarryMode(clonk) { return CARRY_Back; }

public func GetCarrySpecial(clonk) { if(clonk->~GetAction() == "Dig") return "pos_hand1"; }

local fDigging;
local DigAngle;
local DigX, DigY;
local stuck_count;

public func IsDigging() { return fDigging; }


public func ControlUseStart(object clonk, int x, int y)
{
	AddEffect("ShovelDig",clonk,1,1,this);
//	ControlUseHolding(clonk, x, y);

	//temporary workaround to allow clonks to dig free when they are stuck in dirt
	if(clonk->Stuck()){
		DigFree(clonk->GetX(), clonk->GetY(), 10);
	}
	
	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int x, int y)
{
	DigX = x;
	DigY = y;
	DigAngle = Angle(0,0,x,y);
	
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	ControlUseStop(clonk, x, y);
}

public func ControlUseStop(object clonk, int x, int y)
{
	fDigging = 0;
	RemoveEffect("ShovelDig",clonk);
	if(clonk->GetAction() != "Dig") return true;

//	EffectCall(clonk, GetEffect("IntDig", clonk), "StopDig");
	clonk->SetXDir(0,100);
	clonk->SetYDir(0,100);
//	clonk->SetAction("Walk");
//	clonk->SetComDir(COMD_Stop);

	return true;
}

public func FxShovelDigTimer(object clonk, effect, int time)
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
	if(clonk->GetAction() != "Dig" || clonk->GBackLiquid(0,-4))
	{
		var is_scaling = (clonk->GetProcedure() == "SCALE");
		var can_dig = (clonk->GetAction() == "Walk" || is_scaling || clonk->GetProcedure() == "HANGLE");
		// Prevent clonk from starting to dig if in deep liquid
		if (clonk->GBackLiquid(0,-4)) can_dig = false;
		if (can_dig)
		{
			clonk->SetAction("Dig");
			clonk->SetComDir(COMD_Stop);
			if (is_scaling)
			{
				// speed boost when Clonk started digging from scaling, so we don't drop down
				var clnk_xdir = clonk->GetDir()*2-1;
				if (Abs(DigX) > Abs(DigY) && DigX*clnk_xdir > 0) // only if player actually wants to go sideways in the scaling direction (|x|>|y| and sign(x)==sign(clnk_xdir))
				{
					// not if standing on ground (to prevent speed boost digging) or on ceiling (to make working your way upwards through earth a little harder)
					if (!clonk->GetContact(-1, CNAT_Top|CNAT_Bottom))
					{
						xdir_boost = clnk_xdir*1000;
						ydir_boost = -100;
					}
				}
			}
		}
		else
		{
			if (fDigging)
			{
				fDigging = false;
				RemoveEffect("ShovelDust",clonk);
			}
			return true;
		}
	}
	// Dig start procedure
	if(!fDigging && clonk->GetAction() == "Dig")
	{
		AddEffect("ShovelDust",clonk,1,1,this);
		fDigging = true;
		stuck_count = 0;
	}
	if(fDigging)
	{
		// Adjust speed at current animation position
		var speed = clonk.ActMap.Dig.Speed*2;

		var iAnimation = GetEffect("IntDig", clonk).var1;
		var iPosition = clonk->GetAnimationPosition(iAnimation)*180/clonk->GetAnimationLength("Dig");
		speed = speed*(Cos(iPosition-45, 50)**2)/2500;

		// limit angle
		DigAngle = BoundBy(DigAngle,65,300);
		var dig_xdir = Sin(DigAngle,+speed);
		var dig_ydir = Cos(DigAngle,-speed);

		// Stuck-check: When player wants to go horizontally while standing on the ground but can't, add a vertical redirect
		// This helps with Clonks getting "stuck" not moving on certain terrain when dig speed is not sufficient to push him up a slope
		// It also helps e.g. when just digging through earth just above rock, because it will nudge the clonk over the rock
		if (!clonk->GetXDir(100) && !clonk->GetYDir(100) && GetContact(-1, CNAT_Bottom) && Abs(dig_xdir) > Abs(dig_ydir) && Abs(dig_xdir) > 10)
		{
			if (stuck_count++)
			{
				ydir_boost -= 100;
			}
		}
		else
		{
			stuck_count = 0;
		}
		
		clonk->SetXDir(dig_xdir+xdir_boost,100);
		clonk->SetYDir(dig_ydir+ydir_boost,100);

		// Dust
		if (!Random(10))
			Dust(clonk);
	}
}

public func Dust(object target)
{
	// Only when the clonk moves the shovel
	var iAnimation = GetEffect("IntDig", target).var1;
	var iPosition = target->GetAnimationPosition(iAnimation)*100/target->GetAnimationLength("Dig");
	if(iPosition > 50)
		return;
	var xdir = target->GetXDir();
	var ydir = target->GetYDir();
	
	// particle effect
	var angle = Angle(0,0,xdir,ydir)+iPosition-25;//RandomX(-25,25);
	var groundx = Sin(angle,15);
	var groundy = -Cos(angle,15);
	var mat = GetMaterial(groundx, groundy);
	var tex = GetTexture(groundx,groundy);
	if(GetMaterialVal("DigFree","Material",mat))
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
		CreateParticle("Dust", groundx/2, groundy/2, PV_Random(-6, 6), PV_Random(-6, 6), PV_Random(18, 1 * 36), particles, 8);
	}
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(135, 0, 0, 1),Trans_Rotate(30, 0, 1, 0)),def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
