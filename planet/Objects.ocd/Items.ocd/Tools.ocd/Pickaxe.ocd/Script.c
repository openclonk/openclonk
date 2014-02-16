/*
	Pickaxe
	Author: Randrian/Ringwaul

	A useful but tedious tool for breaking through rock without
	explosives.
*/

local maxreach;
local swingtime;
local using;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarrySpecial(clonk) { if(using == 1) return "pos_hand2"; }
public func GetCarryTransform()
{
	return Trans_Rotate(-90, 0, 1, 0);
}


func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(40, 0, 0, 1),Trans_Rotate(150, 0, 1, 0), Trans_Scale(900), Trans_Translate(600, 400, 1000)),def);
}

protected func Initialize()
{
	//maxreach is the length of the pick from the clonk's hand
	maxreach=12;
	swingtime=0;
}

private func Hit(x, y)
{
	StonyObjectHit(x, y);
	return 1;
}

static const Pickaxe_SwingTime = 40;

func ControlUseStart(object clonk, int ix, int iy)
{
	// Can clonk use pickaxe?
	if (clonk->GetProcedure() != "WALK")
		return true;
	using = 1;
	// Create an offset, so that the hit matches with the animation
	swingtime = Pickaxe_SwingTime*1/38;
	clonk->SetTurnType(1);
	clonk->SetHandAction(1);
	clonk->UpdateAttach();
	clonk->PlayAnimation("StrikePickaxe", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikePickaxe"), Pickaxe_SwingTime, ANIM_Loop), Anim_Const(1000));

	AddEffect("IntPickaxe", clonk, 1, 1, this);
	return true;
}

protected func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Can clonk use pickaxe?
	if (clonk->GetProcedure() != "WALK")
	{
		clonk->CancelUse();
		return true;
	}

	x = new_x; y = new_y;
	return true;
}

local x, y;

func ControlUseStop(object clonk, int ix, int iy)
{
	Reset(clonk);
	return true;
}

protected func DoSwing(object clonk, int ix, int iy)
{
	var angle = Angle(0,0,ix,iy);

	//Creates an imaginary line which runs for 'maxreach' distance (units in pixels)
	//or until it hits a solid wall.
	var iDist=0;
	while(!GBackSolid(Sin(180-angle,iDist),Cos(180-angle,iDist)) && iDist < maxreach)
	{
		++iDist;
	}
	
	//Point of contact, where the pick strikes the landscape
	var x2 = Sin(180-angle,iDist);
	var y2 = Cos(180-angle,iDist);
	var is_solid = GBackSolid(x2,y2);
	
	// alternatively hit certain objects
	var target_obj = FindObject(Find_AtPoint(x2, y2), Find_Func("CanBeHitByPickaxe"));
	
	// notify the object that it has been hit
	if(target_obj)
		target_obj->~OnHitByPickaxe(this, clonk);
		
	// special effects only ifhit something
	if(is_solid || target_obj)
	{	
		var mat = GetMaterial(x2,y2);
		var tex = GetTexture(x2,y2);

		//Is the material struck made of a diggable material?
		if(is_solid && GetMaterialVal("DigFree","Material",mat))
		{
			var clr = GetAverageTextureColor(tex);
			var particles =
			{
				Prototype = Particles_Dust(),
				R = (clr >> 16) & 0xff,
				G = (clr >> 8) & 0xff,
				B = clr & 0xff,
				Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 50), 1000, 0),
			};
			CreateParticle("Dust", x2, y2, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 3);
			Sound("Dig?");
		}
		//It's solid, but not diggable. So it is a hard mineral.
		else
		{
			CreateParticle("StarSpark", x2*9/10,y2*9/10, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 10);
			Sound("Clang?");
		}
		
		// Do blastfree after landscape checks are made. Otherwise, mat always returns as "tunnel"
		BlastFree(GetX()+x2,GetY()+y2,5,GetController(),MaxPickDensity);
	}

}

// Reroute callback to clonk context to ensure DigOutObject callback is done in Clonk
public func DigOutObject(object obj)
{
	// TODO: it would be nice if the method of finding the clonk does not rely on it to be the container of the pickaxe
	var clonk = Contained();
	if (clonk)
		clonk->~DigOutObject(obj);
}

func FxIntPickaxeTimer(clonk, effect, time)
{
	++swingtime;
	if(swingtime >= Pickaxe_SwingTime) // Waits three seconds for animation to run (we could have a clonk swing his pick 3 times)
	{
		DoSwing(clonk,x,y);
		swingtime = 0;
	}
	
	var angle = Angle(0,0,x,y);
	var speed = 50;

	var iPosition = swingtime*180/Pickaxe_SwingTime;
	//Message("%d", clonk, iPosition);
	speed = speed*(Cos(iPosition-45, 50)**2)/2500;
	//Message("%d", clonk, speed);
	// limit angle
	angle = BoundBy(angle,65,300);
	clonk->SetXDir(Sin(angle,+speed),100);
	clonk->SetYDir(Cos(angle,-speed),100);
}

protected func ControlUseCancel(object clonk, int ix, int iy)
{
	Reset(clonk);
	return true;
}

public func Reset(clonk)
{
	using = 0;
	clonk->SetTurnType(0);
	clonk->SetHandAction(false);
	clonk->UpdateAttach();
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	swingtime=0;
	RemoveEffect("IntPickaxe", clonk);
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
local MaxPickDensity = 70; // can't pick granite
