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
	return Trans_Rotate(90, 1, 0, 0);
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

public func RejectUse(object clonk)
{
	return clonk->GetProcedure() != "WALK";
}

func ControlUseStart(object clonk, int ix, int iy)
{
	using = 1;
	// Create an offset, so that the hit matches with the animation
	swingtime = Pickaxe_SwingTime*1/38;
	clonk->SetTurnType(1);
	clonk->SetHandAction(1);
	clonk->UpdateAttach();
	clonk->PlayAnimation("StrikePickaxe", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikePickaxe"), Pickaxe_SwingTime, ANIM_Loop), Anim_Const(1000));
	AddEffect("IntPickaxe", clonk, 1, 1, this);
	return true;
}

protected func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Can clonk use pickaxe?
	if (clonk->GetProcedure() != "WALK")
	{
		clonk->PauseUse(this);
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
			Sound(sound, nil, nil, nil, nil, nil, pitch);
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

// Reroute callback to clonk context to ensure DigOutObject callback is done in Clonk
public func DigOutObject(object obj)
{
	// TODO: it would be nice if the method of finding the clonk does not rely on it to be the container of the pickaxe
	var clonk = Contained();
	if (clonk)
		clonk->~DigOutObject(obj);
}

public func FxIntPickaxeStart(object clonk, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	// Ensure ActMap is local and writable
	if (clonk.ActMap == clonk.Prototype.ActMap) clonk.ActMap = new clonk.ActMap {};
	// Disable scaling during usage.
	effect.actmap_scale = clonk.ActMap.Scale;
	clonk.ActMap.Scale = nil;
	return FX_OK;
}

public func FxIntPickaxeTimer(object clonk, proplist effect, int time)
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
	speed = speed*(Cos(iPosition-45, 50)**2)/2500;
	// limit angle
	angle = BoundBy(angle,65,300);
	clonk->SetXDir(Sin(angle,+speed),100);
	clonk->SetYDir(Cos(angle,-speed),100);
}

public func FxIntPickaxeStop(object clonk, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	// Reset the clonk scaling entry in its ActMap.
	clonk.ActMap.Scale = effect.actmap_scale;
	return FX_OK;
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

// Effects that sets the category of C4D_Objects to C4D_None for some time to prevent those objects from hitting the Clonk.
private func FxIntNoHitAllowedStart(object target, effect fx, temp)
{
	if (temp) return;
	fx.category = target->GetCategory();
	target->SetCategory(C4D_None);
}

private func FxIntNoHitAllowedStop(object target, effect fx, int reason, temp)
{
	if (temp || !target) return;
	// If nothing magically changed the category, reset it.
	if (target->GetCategory() == C4D_None)
		target->SetCategory(fx.category);
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local MaxPickDensity = 70; // can't pick granite
