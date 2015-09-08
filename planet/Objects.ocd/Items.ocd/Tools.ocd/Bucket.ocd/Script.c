/**
	Bucket
	Transport earth from one spot to another to form the landscape.
	Replaces the old earth chunks in their behaviour.

	@author Clonkonaut
*/

// Uses an extra-slot to store and display material.
#include Library_HasExtraSlot

// Maximum distance at which material is collected / spilled
local maxreach = 15;

// This is only temporary during drawing. The contents define the actual material & amount;
local material_amount;
local material_name;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{
	return Trans_Mul(Trans_Rotate(90, 0, 1, 0), Trans_Translate(3500, 0, -4000));
}

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction();
}

public func ControlUse(object clonk, int iX, int iY)
{
	var angle = Angle(0,0,iX,iY);
	var distance = GetBucketReachDistance(angle);
	var x2 = Sin(180-angle,distance);
	var y2 = Cos(180-angle,distance);

	// spill bucket
	if (IsBucketFilled())
	{
		Spill(x2, y2, distance >= maxreach);
		EmptyBucket();
		PlayAnimation(clonk);
		return true;
	}
	else
	{
		Message("$UsageHotTip$");
	}

	return true;
}

public func RejectCollect(id def, object obj)
{
	if (!obj->~IsBucketMaterial()) return true;
	// Can only contain one stackable object.
	if (Contents() && Contents(0)->~IsStackable()) return true;
	return false;
}

public func EmptyBucket()
{
	var i = ContentsCount();
	while (--i >= 0)
		if (Contents(0)) Contents(0)->RemoveObject();
	this.PictureTransformation = Trans_Mul(Trans_Translate(500,400,0), Trans_Rotate(-10,1,0,0), Trans_Rotate(30,0,1,0), Trans_Rotate(+25,0,0,1), Trans_Scale(1350));
}

public func FillBucket()
{
	this.PictureTransformation = Trans_Mul(Trans_Translate(500,400,0), Trans_Rotate(-20,1,0,0), Trans_Rotate(20,0,1,0), Trans_Rotate(-15,0,0,1), Trans_Scale(1350));
}

public func IsBucketFilled()
{
	return ContentsCount();
}
public func IsBucketEmpty()
{
	return !IsBucketFilled();
}

/** Creates an imaginary line which runs for 'maxreach' distance (units in pixels) or until it hits a solid wall */
private func GetBucketReachDistance(int angle)
{
	var distance = 0;
	while(!GBackSolid(Sin(180-angle,distance),Cos(180-angle,distance)) && distance < maxreach)
	{
		++distance;
	}
	return distance;
}

private func PlayAnimation(object clonk)
{
	// animation only available for jumping and walking
	if(!clonk->IsJumping() && !clonk->IsWalking())
		return;

	var arm, carry_bone;
	if(clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
	}
	else
	{
		arm = "R";
		carry_bone = "pos_hand2";
	}
	
	// figure out the kind of animation to use
	var length=15;
	var animation;
	if(clonk->IsJumping())
		animation = Format("SwordJump2.%s",arm);
	else
		animation = Format("SwordSlash2.%s", arm);

	clonk->PlayAnimation(animation, 10, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
	clonk->UpdateAttach();
}

private func Spill(int x, int y, bool soft_spill)
{
	var obj = Contents(0);
	if (!obj) return;
	material_name = obj->GetMaterialName();
	material_amount = obj->GetMaterialAmount();
	var stack_count = obj->~GetStackCount();
	if (stack_count > 1) material_amount *= stack_count;
	
	// This will only spray out the material because no solid base to stick it on was found
	if (soft_spill)
	{
		var angle = Angle(0,0, x,y);
		CastPXS(material_name, material_amount, 20, 0,0, angle, 15);
		return;
	}
	
	// Store as property. This is solely done so that we don't have to pass it as a parameter everywhere.
	
	for (var i = 0; i < 5; i++)
	{
		// Fix some holes
		if (i == 1)
		{
			DrawPixel(GetX()+x-1, GetY()+y-1);
			if (!material_amount) break;
			DrawPixel(GetX()+x+1, GetY()+y-1);
			if (!material_amount) break;
			DrawPixel(GetX()+x-1, GetY()+y+1);
			if (!material_amount) break;
			DrawPixel(GetX()+x+1, GetY()+y+1);
			if (!material_amount) break;
		}
		if (!DrawCircle(GetX()+x,GetY()+y,i))
			break;
	}
}

private func DrawCircle(int x0, int y0, int radius)
{
	// Midpoint circle algorithm

	var f = 1 - radius;
	var ddF_x = 1;
	var ddF_y = -2 * radius;
	var x = 0;
	var y = radius;

	DrawPixel(x0, y0 + radius);
	if (!material_amount) return false;
	DrawPixel(x0, y0 - radius);
	if (!material_amount) return false;
	DrawPixel(x0 + radius, y0);
	if (!material_amount) return false;
	DrawPixel(x0 - radius, y0);
	if (!material_amount) return false;

	while(x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		DrawPixel(x0 + x, y0 + y);
		if (!material_amount) return false;
		DrawPixel(x0 - x, y0 + y);
		if (!material_amount) return false;
		DrawPixel(x0 + x, y0 - y);
		if (!material_amount) return false;
		DrawPixel(x0 - x, y0 - y);
		if (!material_amount) return false;
		DrawPixel(x0 + y, y0 + x);
		if (!material_amount) return false;
		DrawPixel(x0 - y, y0 + x);
		if (!material_amount) return false;
		DrawPixel(x0 + y, y0 - x);
		if (!material_amount) return false;
		DrawPixel(x0 - y, y0 - x);
		if (!material_amount) return false;
	}

	return true;
}

private func DrawPixel(int x, int y)
{
	// Don't overwrite solid material
	if (GBackSolid(AbsX(x), AbsY(y))) return;

	DrawMaterialQuad(material_name, x,y, x+1,y, x+1,y+1, x,y+1, true);
	material_amount--;
}

protected func Hit()
{
	Sound("DullWoodHit?");
}


// Can collect IsBucketMaterial?
public func IsBucket() { return true; }
public func IsTool() { return true; }
public func IsToolProduct() { return true; }

// When trying to put into a producer that can't take the item but its contents, just transfer the contents.
public func TryPutInto(object to_building, ...)
{
	if (to_building && to_building->~IsProducer() && !to_building->~IsCollectionAllowed(this))
	{
		var i = ContentsCount(), contents, num_collected = 0;
		while (i--)
			if (contents = Contents(i))
				if (to_building->Collect(contents))
					++num_collected;
		// Return if contents transfer was successful.
		if (num_collected > 0) return true;
	}
	return _inherited(to_building, ...);
	 
}


public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	return true;
}

protected func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(500,400,0), Trans_Rotate(-10,1,0,0), Trans_Rotate(30,0,1,0), Trans_Rotate(+25,0,0,1), Trans_Scale(1350)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = true;
