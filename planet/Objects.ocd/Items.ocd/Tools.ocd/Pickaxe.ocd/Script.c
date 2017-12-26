/*
	Pickaxe
	A useful but tedious tool for breaking through rock without explosives.
	
	@author: Randrian/Ringwaul
*/

#include Library_Flammable

local swingtime = 0;
local using;

static const Pickaxe_SwingTime = 40;

/*-- Engine Callbacks --*/

func Hit(x, y)
{
	StonyObjectHit(x, y);
}

// Reroute callback to clonk context to ensure DigOutObject callback is done in Clonk
public func DigOutObject(object obj)
{
	// TODO: it would be nice if the method of finding the clonk does not rely on it to be the container of the pickaxe
	var clonk = Contained();
	if (clonk)
		clonk->~DigOutObject(obj);
}

/*-- Usage --*/

public func HoldingEnabled() { return true; }

public func RejectUse(object clonk)
{
	var proc = clonk->GetProcedure();
	return proc != "WALK" && proc != "SCALE";
}

public func ControlUseStart(object clonk, int ix, int iy)
{
	using = 1;
	// Create an offset, so that the hit matches with the animation
	swingtime = Pickaxe_SwingTime*1/38;
	clonk->SetTurnType(1);
	clonk->SetHandAction(1);
	clonk->UpdateAttach();
	clonk->PlayAnimation("StrikePickaxe", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikePickaxe"), Pickaxe_SwingTime, ANIM_Loop), Anim_Const(1000));
	var fx = AddEffect("IntPickaxe", clonk, 1, 1, this);
	if (!fx) return false;
	fx.x = ix;
	fx.y = iy;
	return true;
}

public func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Can clonk use pickaxe?
	if (clonk->GetProcedure() != "WALK")
	{
		clonk->PauseUse(this);
		return true;
	}
	var fx = GetEffect("IntPickaxe", clonk);
	if (!fx) return clonk->CancelUse();
	fx.x = new_x; fx.y = new_y;
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
	using = 0;
	clonk->SetTurnType(0);
	clonk->SetHandAction(false);
	clonk->UpdateAttach();
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	swingtime=0;
	RemoveEffect("IntPickaxe", clonk);
}

func DoSwing(object clonk, int ix, int iy)
{
	var angle = 180-Angle(0,0,ix,iy);

	//Creates an imaginary line which runs for 'MaxReach' distance (units in pixels)
	//or until it hits a solid wall.
	var iDist=0;
	var x2, y2;
	while (!GBackSolid((x2=Sin(angle,iDist)), (y2=Cos(angle,iDist))) && iDist < MaxReach)
	{
		// Check some additional surrounding pixels in a cone to make hitting single pixels easier.
		for (var da = -StrikeCone/2; da <= StrikeCone/2; da += 2)
		{
			if (Abs(da) < 3) continue;
			var x3 = Sin(angle+da,iDist), y3 = Cos(angle+da, iDist);
			if (x3 != x2 || y3 != y2)
			{
				x2 = x3; y2 = y3;
				if (GBackSolid(x2, y2))
					break;
			}
		}
		++iDist;
	}
	
	//Point of contact, where the pick strikes the landscape
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
			Sound("Clonk::Action::Dig::Dig?");
		}
		//It's solid, but not diggable. So it is a hard mineral.
		else
		{
			var spark = Particles_Glimmer();
			var pitch = nil;
			var sound = "Objects::Pickaxe::Clang?";
			if (GetMaterialVal("Density","Material",mat) > MaxPickDensity)
			{
				sound = "Objects::Pickaxe::ClangHard?";
				pitch = RandomX(-20, 20);
				spark.B = 255;
				spark.R = PV_Random(0, 128, 2);
				spark.OnCollision = PC_Bounce();
			}
			CreateParticle("StarSpark", x2*9/10,y2*9/10, PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(10, 50), spark, 30);
			Sound(sound, {pitch = pitch});
		}
		
		// Do blastfree after landscape checks are made. Otherwise, mat always returns as "tunnel"
		BlastFree(GetX()+x2,GetY()+y2,5,GetController(),MaxPickDensity);
		
		// Make sure that new loose objects do not directly hit the Clonk and tumble it.
		for (var obj in FindObjects(Find_Distance(10, x2, y2), Find_Category(C4D_Object), Find_Layer(), Find_NoContainer()))
		{
			if (obj->Stuck()) continue;
			AddEffect("IntNoHitAllowed", obj, 1, 30, nil, GetID());
		}
	}
}

func FxIntPickaxeTimer(object clonk, proplist effect, int time)
{
	++swingtime;
	if(swingtime >= Pickaxe_SwingTime) // Waits three seconds for animation to run (we could have a clonk swing his pick 3 times)
	{
		DoSwing(clonk,effect.x,effect.y);
		swingtime = 0;
	}
	
	var angle = Angle(0,0,effect.x,effect.y);
	var speed = 50;

	var iPosition = swingtime*180/Pickaxe_SwingTime;
	speed = speed*(Cos(iPosition-45, 50)**2)/2500;
	// limit angle
	angle = BoundBy(angle,65,300);
	clonk->SetXDir(Sin(angle,+speed),100);
	clonk->SetYDir(Cos(angle,-speed),100);
}

// Effects that sets the category of C4D_Objects to C4D_None for some time to prevent those objects from hitting the Clonk.
func FxIntNoHitAllowedStart(object target, effect fx, temp)
{
	if (temp) return;
	fx.category = target->GetCategory();
	target->SetCategory(C4D_None);
}

func FxIntNoHitAllowedStop(object target, effect fx, int reason, temp)
{
	if (temp || !target) return;
	// If nothing magically changed the category, reset it.
	if (target->GetCategory() == C4D_None)
		target->SetCategory(fx.category);
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
		return CARRY_Back;
}

public func GetCarrySpecial(clonk) { if(using == 1) return "pos_hand2"; }

public func GetCarryTransform()
{
	return Trans_Rotate(90, 1, 0, 0);
}

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(40, 0, 0, 1),Trans_Rotate(150, 0, 1, 0), Trans_Scale(900), Trans_Translate(600, 400, 1000)),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
//MaxReach is the length of the pick from the clonk's hand
local MaxReach = 12;
// StrikeCone is the size of the cone checked for material hits in degrees.
local StrikeCone = 16;
local MaxPickDensity = 70; // can't pick granite
local ForceFreeHands = true;
local Components = {Wood = 1, Metal = 1};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
local BurnDownTime = 140;
