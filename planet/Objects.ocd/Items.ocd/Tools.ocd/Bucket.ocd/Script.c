/**
	Bucket
	Transport earth from one spot to another to form the landscape.
	Replaces the old earth chunks in their behaviour.

	@author Clonkonaut
*/

// Maximum distance at which material is collected / spilled
local maxreach = 15;
// Variable to store extracted material
local in_bucket_mat, in_bucket_amount;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{
	return Trans_Mul(Trans_Rotate(-90, 0, 0, 1), Trans_Translate(-4000,3500));
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
	
	// otherwise try to fill bucket
	if(GBackSolid(x2,y2))
	{
		if (clonk->HasHandAction())
		{
			var mat = GetMaterial(x2,y2);

			if(GetMaterialVal("DigFree","Material",mat))
			{
				Sound("SoftTouch2");
				var amount = DigFree(GetX()+x2,GetY()+y2,5, true);
				FillBucket(mat, amount);
				PlayAnimation(clonk);
			}
		}
	}
	return true;
}

public func EmptyBucket()
{
	in_bucket_amount = 0;
	in_bucket_mat = nil;
	this.PictureTransformation = Trans_Mul(Trans_Translate(500,400,0), Trans_Rotate(-10,1,0,0), Trans_Rotate(30,0,1,0), Trans_Rotate(+25,0,0,1), Trans_Scale(1350));
}

public func FillBucket(int mat, int amount)
{
	in_bucket_amount = amount;
	in_bucket_mat = mat;
	this.PictureTransformation = Trans_Mul(Trans_Translate(500,400,0), Trans_Rotate(-20,1,0,0), Trans_Rotate(20,0,1,0), Trans_Rotate(-15,0,0,1), Trans_Scale(1350));
}

public func IsBucketFilled()
{
	return in_bucket_mat != nil;
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
	// This will only spray out the material because no solid base to stick it on was found
	if (soft_spill)
	{
		var angle = Angle(0,0, x,y);
		CastPXS(MaterialName(in_bucket_mat), in_bucket_amount, 20, 0,0, angle, 15);
		return;
	}

	for (var i = 0; i < 5; i++)
	{
		// Fix some holes
		if (i == 1)
		{
			DrawPixel(GetX()+x-1, GetY()+y-1);
			if (!in_bucket_amount) break;
			DrawPixel(GetX()+x+1, GetY()+y-1);
			if (!in_bucket_amount) break;
			DrawPixel(GetX()+x-1, GetY()+y+1);
			if (!in_bucket_amount) break;
			DrawPixel(GetX()+x+1, GetY()+y+1);
			if (!in_bucket_amount) break;
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
	if (!in_bucket_amount) return false;
	DrawPixel(x0, y0 - radius);
	if (!in_bucket_amount) return false;
	DrawPixel(x0 + radius, y0);
	if (!in_bucket_amount) return false;
	DrawPixel(x0 - radius, y0);
	if (!in_bucket_amount) return false;

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
		if (!in_bucket_amount) return false;
		DrawPixel(x0 - x, y0 + y);
		if (!in_bucket_amount) return false;
		DrawPixel(x0 + x, y0 - y);
		if (!in_bucket_amount) return false;
		DrawPixel(x0 - x, y0 - y);
		if (!in_bucket_amount) return false;
		DrawPixel(x0 + y, y0 + x);
		if (!in_bucket_amount) return false;
		DrawPixel(x0 - y, y0 + x);
		if (!in_bucket_amount) return false;
		DrawPixel(x0 + y, y0 - x);
		if (!in_bucket_amount) return false;
		DrawPixel(x0 - y, y0 - x);
		if (!in_bucket_amount) return false;
	}

	return true;
}

private func DrawPixel(int x, int y)
{
	// Don't overwrite solid material
	if (GBackSolid(AbsX(x), AbsY(y))) return;

	DrawMaterialQuad(MaterialName(in_bucket_mat), x,y, x+1,y, x+1,y+1, x,y+1, true);
	in_bucket_amount--;
}

protected func Hit()
{
	Sound("DullWoodHit?");
}

// Production stuff (for loam)
public func IsMaterialContainer() { return true; }
public func GetContainedMaterial() { return MaterialName(in_bucket_mat); }
public func RemoveContainedMaterial(string material, int amount)
{
	if (material != MaterialName(in_bucket_mat)) return 0;
	if (amount > in_bucket_amount)
	{
		var ret = in_bucket_amount;
		in_bucket_amount = 0;
		in_bucket_mat = nil;
		return ret;
	}
	in_bucket_amount -= amount;
	return amount;
}
public func GetFillLevel() { return in_bucket_amount; }

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

public func SetFilled(string mat, int amount)
{
	in_bucket_mat = mat;
	in_bucket_amount = amount;
	return true;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (in_bucket_mat) props->AddCall("Fill", this, "SetFilled", Format("%v", in_bucket_mat), in_bucket_amount);
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