/*
	Cotton
	Author: Clonkonaut

	Medium crop for farming.
*/

#include Library_Plant
#include Library_Crop

local direction = 1;
local capacity; // How much fruit this plant will yield
local branches; // Array with 3 proplists containing all the needed values for the 3 branches
local branch_proto = {
	// As returned by AttachMesh
	attach_slot = 0,
	 // Growing time until ready to bear fruit, growing time afterwards is determined by the fruit
	grow_time = 4500,
	// The branch's animation length is 875, 490 marks the point just before the branch start raising up
	first_animation_stage = 490,
	// Whether the branch is ready to bear fruit yet
	grown = false,
	// As returned by PlayAnimation
	grow_animation = -1,
	// As returned by AddEffect
	grow_effect = nil,
	// The fruit object (in Contents)
	fruit = nil,
	// As returned by AttachMesh
	fruit_slot = 0,
	// Internal value to (securely) stop new fruit from growing at this branch
	no_fruit = false,
};

private func Construction()
{
	_inherited(...);
	RemoveTimer("Seed");
	capacity = Random(3) + 3;

	StartGrowth(this.growth);
	AddTimer("WaterCheck", 70 + Random(10));

	// The mesh doesn't have more than 3 bones, beware
	branches = CreateArray(3);
	branches[0] = new branch_proto {};
	branches[1] = new branch_proto {};
	branches[2] = new branch_proto {};

	// Make half of the plants switch direction for more variety
	if (!Random(2)) direction = -1;
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(70, 110) * direction, 0, 1, 0));
}

private func Initialize()
{
	AddTimer("Reproduction", 72);
}

/* Reproduction */

private func Reproduction()
{
	if (OnFire()) return;
	if (GetCon() < 100) return RemoveTimer("Reproduction");
	if (!capacity && !HasFruit()) return Perish();

	if (CheckSeedChance())
	{
		var can_branch = GetNextGrowableBranch();
		var can_fruit = GetNextFruitableBranch();
		if ((can_branch != -1 && Random(3)) || can_fruit == -1)
			return GrowBranch();
		if (can_fruit != -1)
		{
			GrowFruit();
			capacity--;
		}
	}
}

// The plant will wither away after it has grown all its fruit
private func Perish()
{
	RemoveTimer("Reproduction");
	StartGrowth(this.degrowth);
	SetClrModulation(RGB(185, 122, 87));
}

/* Branch growth */

// Will grow the next branch unless all 3 are grown (returns false then, true otherwise)
// If fullgrown is true, the branch will be fully grown but not bear a fruit!
public func GrowBranch(bool fullgrown, int branch)
{
	var next_to_grow;
	if (branch != nil)
		next_to_grow = branch;
	else
		next_to_grow = GetNextGrowableBranch(fullgrown);
	if (next_to_grow == -1) return false;

	if (branches[next_to_grow].grow_animation != branch_proto.grow_animation) // Already growing, fullgrown must be true
	{
		if (!fullgrown) return false; // This can't happen
		FinishBranchGrowth(next_to_grow);
		return true;
	}
	branches[next_to_grow].attach_slot = AttachMesh(Cotton_Branch, Format("Stem%d", next_to_grow + 1), "main", GetBranchAttachTransform(next_to_grow, 1));
	branches[next_to_grow].grow_animation = PlayAnimation("grow", next_to_grow + 1, Anim_Linear(0, 0, branches[next_to_grow].first_animation_stage, branches[next_to_grow].grow_time, ANIM_Hold), Anim_Const(1000), nil, branches[next_to_grow].attach_slot);
	branches[next_to_grow].grow_effect = AddEffect("IntBranchGrowth", this, 1, 35, this);
	branches[next_to_grow].grow_effect.branch = next_to_grow;
	if (fullgrown) FinishBranchGrowth(next_to_grow);
	return true;
}

// Determines the next branch to grow. If fullgrown is true, growing but unfinished branches are also returned
private func GetNextGrowableBranch(bool fullgrown)
{
	var ret = -1;
	var i = -1;
	while (++i < GetLength(branches))
	{
		if (branches[i].grown) continue;
		if (branches[i].grow_animation != branch_proto.grow_animation && !fullgrown) continue;
		ret = i;
		break;
	}
	return ret;
}

private func FxIntBranchGrowthTimer(object target, proplist effect, int time)
{
	UpdateBranchAttachTransform(effect.branch, GetAnimationPosition(branches[effect.branch].grow_animation, branches[effect.branch].attach_slot) * 500 / branches[effect.branch].first_animation_stage);
	if (GetAnimationPosition(branches[effect.branch].grow_animation, branches[effect.branch].attach_slot) >= branches[effect.branch].first_animation_stage)
		FinishBranchGrowth(effect.branch);
}

private func FinishBranchGrowth(int branch)
{
	branches[branch].grown = true;
	RemoveEffect(nil, nil, branches[branch].grow_effect);
	UpdateBranchAttachTransform(branch, 500);
	SetAnimationPosition(branches[branch].grow_animation, Anim_Const(branches[branch].first_animation_stage), branches[branch].attach_slot);
}

private func UpdateBranchAttachTransform(int branch, int scale)
{
	SetAttachTransform(branches[branch].attach_slot, GetBranchAttachTransform(branch, scale));
}

private func GetBranchAttachTransform(int branch, int scale)
{
	// These transforms have been determined by careful testing
	if (branch == 0) return Trans_Mul(Trans_Rotate(-30, 0, 1), Trans_Rotate(-100, 0, 0, 1), Trans_Rotate(-90, 0, 1), Trans_Translate(1000), Trans_Scale(scale));
	if (branch == 1) return Trans_Mul(Trans_Rotate(30, 0, 1), Trans_Rotate(-100, 0, 0, 1), Trans_Rotate(90, 0, 1), Trans_Translate(1000), Trans_Scale(scale));
	if (branch == 2) return Trans_Mul(Trans_Rotate(-100, 0, 0, 1), Trans_Rotate(-90, 0, 1), Trans_Scale(scale));
}

/* Fruit growth */

// Will grow a fruit but only if there's a fully grown branch without a fruit available
// If fullgrown is true, the fruit will be ripe immediately. If there was no full grown branch, a branch will also be complete.
// Does return the fruit object or nil
public func GrowFruit(bool fullgrown)
{
	var next_to_grow = GetNextFruitableBranch(fullgrown);
	if (next_to_grow == -1) return nil;

	if (branches[next_to_grow].fruit) // Already growing, fullgrown must be true
	{
		if (!fullgrown) return false; // This can't happen
		branches[next_to_grow].fruit->Grow(next_to_grow, fullgrown);
		return branches[next_to_grow].fruit;
	}
	if (!branches[next_to_grow].grown)
		GrowBranch(fullgrown, next_to_grow);

	branches[next_to_grow].fruit = CreateContents(Cotton_Fruit);
	if (!branches[next_to_grow].fruit) return false;
	branches[next_to_grow].fruit_slot = AttachMesh(branches[next_to_grow].fruit, "fruit_target", "leafes", GetFruitAttachTransform(1, 0), nil, branches[next_to_grow].attach_slot);
	branches[next_to_grow].fruit->Grow(next_to_grow, fullgrown);
	return branches[next_to_grow].fruit;
}

// Determines the next brnach to grow a fruit. If fullgrown is true, unfinished branches are also returned
private func GetNextFruitableBranch(bool fullgrown)
{
	var ret = -1;
	var i = -1;
	while (++i < GetLength(branches))
	{
		if (branches[i].no_fruit) continue;
		if (branches[i].fruit && !(fullgrown && branches[i].fruit->IsGrowing())) continue;
		if (!branches[i].grown && !fullgrown) continue;
		ret = i;
		break;
	}
	return ret;
}

// Usually called by the fruit
public func UpdateFruitAttachTransform(int branch, int scale)
{
	SetAttachTransform(branches[branch].fruit_slot, GetFruitAttachTransform(scale, 0));
}

private func GetFruitAttachTransform(int scale, int extra_r)
{
	return Trans_Mul(Trans_Rotate(180, 0, 1, 0), Trans_Rotate(extra_r, 0, 0, 1), Trans_Scale(scale));
}

// Called by the fruit when the first growing animation is done
// The branch will restart its animation with duration of time
public func FruitFills(int branch, int time, bool fullgrown)
{
	var pos = GetAnimationPosition(branches[branch].grow_animation, branches[branch].attach_slot);
	if (fullgrown)
		SetAnimationPosition(branches[branch].grow_animation, Anim_Const(GetAnimationLength("grow", branches[branch].attach_slot)), branches[branch].attach_slot);
	else
		SetAnimationPosition(branches[branch].grow_animation, Anim_Linear(pos,
		                                                                  0, 
		                                                                  GetAnimationLength("grow", branches[branch].attach_slot),
		                                                                  time,
		                                                                  ANIM_Hold), branches[branch].attach_slot);
}

// Called by the fruit before it flies off to give fair warning
public func ShakeFruit(int branch)
{
	var effect = AddEffect("IntShakeFruit", this, 1, 1, this);
	effect.branch = branch;
}

private func FxIntShakeFruitTimer(object target, proplist effect, int time)
{
	if (time > 24)
		return FX_Execute_Kill;
	var angle = Sin(time * 45, 4);
	SetAttachTransform(branches[effect.branch].fruit_slot, GetFruitAttachTransform(2000, angle));
	return FX_OK;
}

// Called by the fruit
// Returns the exit position of the fruit relative to the cotton plant
// TODO: Do not hardcode positions but get the bone's position in Clonk coordinates (as of now feature is missing)
public func GetFruitExitPosition(int branch)
{
	if (branch == 0) return { x = 7 * direction, y = 0};
	if (branch == 1) return { x = -7 * direction, y = -1};
	if (branch == 2) return { x = 11 * direction, y = -16};
	return {}; // Unknown
}

// Called by the fruit when it exits the plant
// Will reverse the animation to full grown but not with fruit state in 35 frames
// Accordingly, branches[].fruit will be cleared after 35 frames
public func DetachFruit(int branch)
{
	DetachMesh(branches[branch].fruit_slot);
	SetAnimationPosition(branches[branch].grow_animation, Anim_Linear(GetAnimationPosition(branches[branch].grow_animation,
	                                                                  branches[branch].attach_slot),
	                                                                  GetAnimationLength("grow",
	                                                                  branches[branch].attach_slot),
	                                                                  branches[branch].first_animation_stage,
	                                                                  35,
	                                                                  ANIM_Hold), branches[branch].attach_slot);
	ScheduleCall(this, "ClearFruit", 35, nil, branch);
	// This is to ensure that within the next 35 frames no new fruit will grow on this branch
	// This is necessary because the actual fruit object might be deleted before that time
	branches[branch].no_fruit = true;
}

private func ClearFruit(int branch)
{
	branches[branch].fruit = nil;
	branches[branch].fruit_slot = nil;
	branches[branch].no_fruit = nil;
}

private func HasFruit()
{
	for (var branch in branches)
		if (branch.fruit)
			return true;
	return false;
}

/* Crop Library */

public func SickleHarvesting()
{
	return true;
}

// Only harvestable if it has a ripe fruit
public func IsHarvestable()
{
	for (var branch in branches)
		if (branch.fruit)
			if (!branch.fruit->~IsGrowing())
				return true;
	return false;
}

public func Harvest(object clonk)
{
	var fruit = -1;
	for (var i = 0; i < GetLength(branches); i++)
		if (branches[i].fruit)
			if (!branches[i].fruit->~IsGrowing())
			{
				fruit = i;
				break;
			}
	if (fruit == -1) return false;

	branches[fruit].fruit->Fly();
	return true;
}

/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	// The fruit are not saved if still inside, do a rudimentary save.
	// Saving all little details of growth is a bit too much for such an always changing state.
	for (var i = 0; i < GetLength(branches); i++)
	{
		// Branch has a fruit, save if grown
		if (branches[i].fruit)
		{
			if (!branches[i].fruit->IsGrowing())
			{
				props->AddCall("Branch", this, "GrowBranch", true, i);
				props->AddCall("Fruit", this, "GrowFruit", true);
			}
			else // If fruit is not fully grown, just save the branch
				props->AddCall("Branch", this, "GrowBranch", true, i);
		}
		else if (branches[i].grown) // Save only fully grown branches
			props->AddCall("Branch", this, "GrowBranch", true, i);
	}
	return true;
}

/* Definition */

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;
local growth = 3;
local degrowth = -6;
local fastgrowth = 9;
