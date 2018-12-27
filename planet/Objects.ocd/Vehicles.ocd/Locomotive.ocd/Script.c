/**
	Locomotive
	Transportation device for long distances and heavy cargo.
	
	@author Pyrit
*/

#include Library_ElevatorControl
#include Library_LiquidContainer
#include Library_PipeControl


local move_dir;
local fuel_amount;
local drive_anim;
local rot_wheels;

protected func Initialize()
{
	move_dir = 0;
	fuel_amount = 0;
	rot_wheels = 0;
	AddTimer("Move", 1);
	SetAction("Drive");
	// Entrance is always open.
	SetEntrance(true);
	// Start driving animation.
	drive_anim = PlayAnimation("Main TrainAction", 1, Anim_Const(0));
	return;
}

public func OnContentMenuOpened()
{
	//return Sound("Locomotive::Chuff");
}

public func OnContentMenuClosed()
{
	//return Sound("Locomotive::Chuff");
}

public func Collection2(object obj)
{
	if (obj->GetOCF() & OCF_CrewMember)
	{
		obj->SetAction("Walk");
		obj->PlayAnimation("Drive", CLONK_ANIM_SLOT_Movement, Anim_Const(10), Anim_Const(1000));
	}
	return;
}

public func Ejection(object obj)
{
	if (obj->GetOCF() & OCF_CrewMember)
	{
		
	}
	return;
}


/*-- Liquid Control --*/

public func IsLiquidContainerForMaterial(string liquid)
{
	return WildcardMatch("Water", liquid);
}

public func GetLiquidContainerMaxFillLevel(liquid_name)
{
	return this.LiquidCapacity;
}

// Set to source or drain pipe.
public func OnPipeConnect(object pipe, string specific_pipe_state)
{
	if (specific_pipe_state == PIPE_STATE_Drain)
	{
		SetDrainPipe(pipe);
		pipe->SetDrainPipe();
	}
	else
	{
		if (!GetDrainPipe())
			OnPipeConnect(pipe, PIPE_STATE_Drain);
	}
	pipe->Report("$MsgConnectedPipe$");
}



/*-- Movement --*/

public func Move()
{
	UpdateFuel();
	var water_amount = GetLiquidAmount(Water);
	
	if (move_dir == 0)
		return;
	
	if (fuel_amount <= 0 && water_amount <= 0)
		return Message("$MsgNoCoalAndWater$");
	if (water_amount <= 0)
		return Message("$MsgNoWater$");
	if (fuel_amount <= 0)
		return Message("$MsgNoCoal$");
	if (Stuck())
		return Message("$MsgStuck$");
		
	fuel_amount--;
	RemoveLiquid(Water, 1);
	
	if (move_dir == -1)
		SetDir(DIR_Left);
	if (move_dir == 1)
		SetDir(DIR_Right);
	SetXDir(15 * move_dir);
	DoExhaust();
	TurnWheels();
	return;
}

private func DoExhaust()
{
	//Sound("Locomotive::Chuff");
	Smoke(11 * (2 * GetDir() - 1), -18, 6);
	var spark = {
		Size = PV_Linear(1, 0), 
	    ForceY = RandomX(-50, -60),
	    ForceX = PV_Random(-1, 1, 10),
		DampingY = PV_Linear(600, 0),
		Stretch = PV_Speed(1000, 500),
		Rotation = PV_Direction(),
		CollisionVertex = 500, 
		OnCollision = PC_Stop(),
	    R = 255,
	    G = PV_Linear(128, 32),
	    B = PV_Random(0, 128, 2),
	    Alpha = PV_Random(255, 0, 3),
		BlitMode = GFX_BLIT_Additive,
	};
	CreateParticle("Magic", 12 * (2 * GetDir() - 1), -18, PV_Random(-3, 3), PV_Random(-3, -4), 36, spark, 1);
	return;
}

private func TurnWheels()
{
	var anim_pos = GetAnimationPosition(drive_anim);
	anim_pos += Abs(GetXDir()) * 8;
	while (anim_pos < 0) 
		anim_pos += 2000;
	while (anim_pos > 2000) 
		anim_pos -= 2000;
	SetAnimationPosition(drive_anim, Anim_Const(anim_pos));
	return;
}

private func UpdateFuel()
{
	if (fuel_amount > 0)
		return;
	var fuel = FindObject(Find_Or(Find_ID(Wood), Find_ID(Coal)), Find_Container(this));
	if (fuel)
	{
		fuel_amount += fuel->GetFuelAmount();
		fuel->RemoveObject();	
	}
	return;
}

public func ContainedDown(object clonk)
{
	move_dir = 0;
}

public func ContainedLeft(object clonk)
{
	move_dir = -1;
}

public func ContainedRight(object clonk)
{
	move_dir = 1;
}

public func ContainedStop(object clonk)
{
	move_dir = 0;
}

public func Hit2()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}

public func IsVehicle() { return true; }
public func IsContainer() { return true; }


/*-- Contents --*/

protected func RejectCollect(id object_id)
{
	if (object_id == Coal || object_id->~IsLiquid())
		return false;
	return true;
}


/*-- Properties --*/

local ActMap = {
	Drive = {
		Prototype = Action,
		Name = "Drive",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		X = 0,
		Y = 0,
		Wdt = 24,
		Hgt = 22,
		NextAction = "Drive",
	},
};

// this.MeshTransformation = Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Scale(580, -580, 580), Trans_Translate(24000, -27000, 0))
local MeshTransformation = [0, 580, 0, -15660, 580, 0, 0, 13920, 0, 0, 580, 0];
// this.PictureTransformation = Trans_Mul(Trans_Translate(-18000, 27000, 0), Trans_Rotate(90, 0, 0, 1), Trans_Rotate(-20, 1, 0, 0))
local PictureTransformation = [0, -940, -342, -18000, 1000, 0, 0, 27000, 0, -342, 940, 0];
local Name = "$Name$";
local Description = "$Description$";
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;
local Components = {Wood = 1, Metal = 4};
local LiquidCapacity = 2400;

// The locomotive may only have a drain pipe.
local PipeLimit_Air = 0;
local PipeLimit_Drain = 1;
local PipeLimit_Source = 0;
