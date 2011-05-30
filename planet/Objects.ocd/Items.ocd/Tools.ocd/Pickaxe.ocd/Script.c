/*
	Pickaxe
	Author: Ringwaul

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

private func Hit()
{
	Sound("RockHit");
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
	swingtime = Pickaxe_SwingTime*4/38;
	clonk->SetTurnType(1);
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

	var x = Sin(180-angle,iDist-8);
	var y = Cos(180-angle,iDist-8);
	var x2 = Sin(180-angle,iDist);
	var y2 = Cos(180-angle,iDist);

	if(GBackSolid(x2,y2))
	{
//		Message("Hit %s", MaterialName(GetMaterial(x2,y2))); //for debug

		var mat = GetMaterial(x2,y2);
		var tex = GetTexture(x2,y2);
		
		// special effects
		if(GetMaterialVal("DigFree","Material",mat))
		{
			var clr = GetAverageTextureColor(tex);
			var a = 80;
			CreateParticle("Dust",x2,y2,RandomX(-3,3),RandomX(-3,3),RandomX(10,250),DoRGBaValue(clr,-255+a,0));
		}
		else
		{
			CastParticles("Spark",RandomX(3,9),35,x2*9/10,y2*9/10,10,30,RGB(255,255,150),RGB(255,255,200));
			Sound("Clang*");
		}

		// dig out resources too! Don't just remove landscape pixels
		BlastFree(GetX()+x2,GetY()+y2,5,GetController());
	}
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
local Rebuy = true;
