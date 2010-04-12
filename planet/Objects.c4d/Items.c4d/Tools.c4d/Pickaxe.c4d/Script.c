/*
	Pickaxe
	Author: Ringwaul

	A useful but tedious tool for breaking through rock without
	explosives.
*/

local maxreach;
local swingtime;
local using;

local picked_materials;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarrySpecial(clonk) { if(using == 1) return "pos_hand2"; }
public func GetCarryTransform()
{
	return Trans_Rotate(-90, 0, 1, 0);
}

//TODO: The pick should probably have an internal array that
//keeps the data of how much of which material has been dug.
//I wouldn't have a clue how to do this, so if anyone else 
//wants to make that, good luck! :)

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(40, 0, 0, 1),Trans_Rotate(150, 0, 1, 0), Trans_Scale(900), Trans_Translate(600, 400, 1000)),def);
}

protected func Initialize()
{
	//maxreach is the length of the pick from the clonk's hand
	maxreach=16;
	swingtime=0;
	picked_materials = CreatePropList();
}

private func Hit()
{
	Sound("RockHit");
	return 1;
}

static const Pickaxe_SwingTime = 60;

func ControlUseStart(object clonk, int ix, int iy)
{
	using = 1;
	// Create an offset, so that the hit matches with the animation
	swingtime = Pickaxe_SwingTime*4/38;
	clonk->SetTurnType(1);
	clonk->PlayAnimation("StrikePickaxe", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikePickaxe"), Pickaxe_SwingTime, ANIM_Loop), Anim_Const(1000));

	AddEffect("IntPickaxe", clonk, 1, 1, this);
	return true;
}

protected func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	//Can clonk use pickaxe?
	if(clonk->GetProcedure() != "WALK")
	{
		clonk->CancelUse();
		return 1;
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
		Message("Hit %s",this, MaterialName(GetMaterial(x2,y2))); //for debug
		
		//special effects
		if(GetMaterialVal("DigFree","Material",GetMaterial(x2,y2))==0)
			CastParticles("Spark",RandomX(3,9),35,x2*9/10,y2*9/10,10,30,RGB(255,255,150),RGB(255,255,200));

		//dig out resources too! Don't just remove landscape pixels
		for(var i = -10; i < 10; i++)
			for(var j = -10; j < 10; j++)
				if(Distance(0,0,i*10,j*10) <= 90)
					PickPixel(i+x, j+y);

		//stops resources from launching into clonk while mining
		for(var resources in FindObjects(Find_Distance(7,x,y), Find_Category(C4D_Object), Find_Not(Find_OCF(OCF_Alive))))
			resources->SetSpeed();
	}
	else
		Message("Hit nothing",this); //for debug
}

func FxIntPickaxeTimer(clonk, number, time)
{
	++swingtime;
	if(swingtime >= Pickaxe_SwingTime) //Waits three seconds for animation to run (we could have a clonk swing his pick 3 times)
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

func PickPixel(int x, int y)
{
	var mat = GetMaterial(x,y);

	// Has the material do be converted instead of blasted?
	var shift = GetMaterialVal("BlastShiftTo", "Material", mat);
	if(shift != nil && shift != "")
	{
		x += GetX(); y += GetY();
		DrawMaterialQuad(shift, x,y,    x,y,   x,y+1,    x,y+1,  1);
		return;
	}

	// If material can't be blasted stop here
	if(!GetMaterialVal("BlastFree", "Material", mat)) return;

	// Remember name for prolist access
	var mat_name = MaterialName(mat);
	
	// Remove the pixel
	FreeRect(GetX()+x,GetY()+y,1,1);

	// Count pixels
	if(picked_materials[mat_name] == nil) picked_materials[mat_name] = 1;
	else picked_materials[mat_name]++;

	// Create freed objects from blast or dig
	var amount = GetMaterialVal("Blast2ObjectRatio", "Material", mat);
	var obj = C4Id(GetMaterialVal("Blast2Object", "Material", mat));
	if(!amount && !GetMaterialVal("Dig2ObjectRequest", "Material", mat))
	{
		amount = GetMaterialVal("Dig2ObjectRatio", "Material", mat);
		obj = C4Id(GetMaterialVal("Dig2Object", "Material", mat));
	}
	if(amount && picked_materials[mat_name] >= amount)
	{
		CreateObject(obj, x, y);
		picked_materials[mat_name] -= amount;
	}
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
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	swingtime=0;
	RemoveEffect("IntPickaxe", clonk);
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }
