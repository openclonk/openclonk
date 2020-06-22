/**
	LavaCore
	The lava high life. Moves rythmically, spews lava, acts as platform for clonks.

	@author Win, Maikel
*/

#include Library_Animal


// The outer shell of the core.
local shell;
local fx_behavior;


public func Construction()
{
	shell = CreateObjectAbove(LavaCoreShell);
	shell->InitAttach(this);
	this.MaxSize = RandomX(40, 50);
	// Add a reproduction timer (from the animal library).
	AddReproductionEffect();	
	// Add an effect to control the lava core.
	fx_behavior = CreateEffect(FxCoreBehavior, 100, 1);
	return;
}

public func Initialize()
{
	if (shell)
		shell->SetSize(GetCon());
	return;
}

public func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 5 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InArea(rectangle);
	var size_range = nil;
	if (settings)
		size_range = settings.size_range;
	var cores = [];
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Lava"), Loc_Space(20), loc_area);
		if (!Random(2))
			spot = FindLocation(Loc_Material("DuroLava"), Loc_Space(20), loc_area);
		if (!spot) continue;
		
		var core = CreateObjectAbove(this, spot.x, spot.y, NO_OWNER);
		if (!core) continue;
		
		if (settings.size_range)
		{
			var size = RandomX(settings.size_range[0], settings.size_range[1]);
			core.MaxSize = BoundBy(size, core.SizeLimitMin, core.SizeLimitMax);
			core->SetCon(core.MaxSize);
		}	
		if (core->Stuck())
		{
			core->RemoveObject();
			continue;
		}
		--amount;
		PushBack(cores, core);
	}
	return cores;
}

public func SetCon(int new_size)
{
	if (shell)
		shell->SetSize(new_size);
	return _inherited(new_size, ...);
}

public func DoCon(int add_size)
{
	if (shell)
		shell->SetSize(GetCon() + add_size);
	return _inherited(add_size, ...);
}

public func Destruction()
{
	if (shell)
		shell->RemoveObject();
	return;
}


/*-- Core Behaviour --*/

local FxCoreBehavior = new Effect
{
	Construction = func()
	{
		this.fossilized = false;
		this.move_vector = [0, 0];
		this.movement_step = 72;
		this.frames_per_attack = 9;
		this.evading_core = 0;
		// Change time to have cores perform actions at different times.
		this.Time = Random(this.movement_step);
		// Make core swim and able to move.
		Target->SetAction("Swim");
		Target->SetComDir(COMD_None);
		return;
	},
	
	Timer = func(int time)
	{
		// Check if fossilized and change state.
		this->CheckFossilized(time);
		if (this.fossilized)
			return FX_OK;
		
		// Do growth.
		this->DoGrowth(time);	
			
		// Attack prey if possible.
		if ((time % this.frames_per_attack) == 0)
			this->AttackPrey(time);	
		
		// Do movement.
		this->DoMovement(time);
		
		return FX_OK;
	},
	
	CheckFossilized = func(int time)
	{
		if (this.fossilized && !Random(120))
		{
			Target->Revive();
			this.fossilized = false;
		}
		// Fossilize more ofen at surface.
		if (!this.fossilized && (!Random(160) || (!Random(100) && !Target->GBackLiquid(0, -6))))
		{
			Target->Fossilize();
			this.fossilized = true;
		}
		return;
	},

	AttackPrey = func(int time)
	{
		// Only if fully grown and not fossilized.
		if (Target->GetCon() < Target.MaxSize || this.fossilized)
			return;
		
		// Shoot a bubble at a nearby prey if the path is free, sometimes randomly shoot a bubble.
		var prey = Target->FindObject(Find_OCF(OCF_Alive), Target->Find_Distance(100), Target->Find_PathFree(), Find_Not(Find_ID(LavaCore)));
		if (!prey && Random(60))
			return;
		
		var angle = Random(360);
		if (prey)
			angle = Angle(Target->GetX(), Target->GetY(), prey->GetX(), prey->GetY());
		// Test if shot is not blocked by the shell.
		if (Target.shell)
		{
			var shell_angle = Normalize(Target.shell->GetR() + 90, 0);
			var diff_angle = angle - shell_angle;
			if (!Inside(diff_angle, -30, 30) && diff_angle < 330 && diff_angle > -330)
				return;		
		}
		
		var bubble = Target->CreateObject(BoilingLava_Bubble);
		bubble->SetVelocity(angle, RandomX(30, 40));
		bubble->DoCon(RandomX(10, 35));
	},
	
	DoGrowth = func(int time)
	{
		if (Target->GetCon() >= Target.MaxSize)
			return;
		// Only grow in lava.
		var mat = MaterialName(Target->GetMaterial(0, 5));
		if (mat == "Lava" || mat == "DuroLava")
			if (Target->GetCon() < Target.MaxSize && !Random(45))
				Target->DoCon(1);
		return;
	},
	
	DoMovement = func(int time)
	{
		// Only perform movement if swimming.
		if (Target->GetAction() != "Swim")
			return;
		
		var forced = false;
		var speed = Target.MovementSpeed;
	
		// Evade other cores. This will keep them evenly spread.
		var cnat = Target->GetContact(-1);
		if (!Target->InLiquid())
		{
			cnat = cnat | CNAT_Top;
		}
		var other_core = Target->FindObject(Find_ID(LavaCore), Find_Exclude(Target), Target->Find_Distance(20 + Target.MaxSize));
		// Check for contact.
		if (cnat & (CNAT_Left | CNAT_Right | CNAT_Top | CNAT_Bottom) && this.evading_move <= 0)
		{
			this.move_vector = [RandomX(-1, 1), RandomX(-1, 1)];
			if ((cnat & CNAT_Left) && !(cnat & CNAT_Right))
				this.move_vector[0] = 1;
			if ((cnat & CNAT_Right) && !(cnat & CNAT_Left))
				this.move_vector[0] = -1;
			if ((cnat & CNAT_Top) && !(cnat & CNAT_Bottom))
				this.move_vector[1] = 1;
			if ((cnat & CNAT_Bottom) && !(cnat & CNAT_Top))
				this.move_vector[1] = -1;
			//Log("[%d]%v has contact: %d and move vector %v", FrameCounter(), Target, cnat, this.move_vector);	
			this.evading_move = 6;
			forced = true;
		}
		// After contact is dealt with we can evade other cores.
		else if (other_core && this.evading_move <= 0)
		{
			//Log("[%d]other core", FrameCounter());
			this.move_vector[0] = Sign(Target->GetX() - other_core->GetX());
			this.move_vector[1] = Sign(Target->GetY() - other_core->GetY());
			this.evading_move = 16;
			forced = true;
		}
		// Otherwise just do random movement.
		else
		{
			this.move_vector[0] = RandomX(-1, 1);
			this.move_vector[1] = RandomX(-1, 1);
			// Move upwards more often to stay at surface more often.
			if (Random(3))
				this.move_vector[1] = -1;
		}
	
		// Execute the movement.
		if ((time % this.movement_step) == 0 || forced)
		{
			Target->SetXDir(this.move_vector[0] * speed + RandomX(-speed, speed) / 10);
			Target->SetYDir(this.move_vector[1] * speed + RandomX(-speed, speed) / 10);
		}
		if ((time % this.movement_step) == 0 && Target.shell)
			Target.shell->SetRDir(5 * (2 * Random(2) - 1));
		
		// Dampen movement over time to make movements seem like impulses.
		var xdir = Target->GetXDir(1000);
		var ydir = Target->GetYDir(1000);
		if (xdir > 0)
			Target->SetXDir(xdir - 3 * speed, 1000);
		else
			Target->SetXDir(xdir + 3 * speed, 1000);
		if (ydir > 0)
			Target->SetYDir(ydir - 3 * speed, 1000);
		else
			Target->SetYDir(ydir + 3 * speed, 1000);
			
		this.evading_move--;	
		return;
	},
	
	IsFossilized = func()
	{
		return this.fossilized;
	}
};

public func StartNonSwim() 
{
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	return;
}


/*-- Reproduction --*/

private func ReproductionCondition()
{
	return GetAlive() && GetCon() >= this.MaxSize;
}

private func SpecialReproductionCondition()
{
	if (fx_behavior)
		return !fx_behavior->IsFossilized();
	return false;
}

public func Birth(object parent)
{
	SetCon(BoundBy(GetCon(), this.SizeLimitMin, this.SizeLimitMax));
	if (shell)
		shell->SetSize(GetCon());
	return;
}


/*-- Core State --*/

// Explode on death.
public func Death()
{
	if (shell)
		shell->RemoveObject();
		
	Explode(BoundBy(GetCon() / 2, 20, 90));
	return;
}

// Turns the core into a fossil.
public func Fossilize()
{
	Sound("Animals::LavaCore::Fossilize", {volume = GetCon()});
	SetComDir(COMD_Stop);
	SetMeshMaterial("LavaCoreStoneMat");
	if (shell)
	{	
		shell->SetMeshMaterial("LavaShellStoneMat");
		shell->SetRDir(0);
		shell->AddSolidMask();
	}
	return;	
}

// Turns the fossil into a core again.
public func Revive()
{
	SetAction("Swim");
	SetComDir(COMD_None);
	SetMeshMaterial("LavaCoreMat");
	if (shell)
	{
		shell->SetMeshMaterial("LavaShellMat");
		shell->RemoveSolidMask();	
	}
	return;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		return false;
	// Avoid saving some stuff that's reinitialized anyway.
	props->Remove("MeshMaterial");
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 424;
local CorrosionResist = true;
local MaxEnergy = 100000;
local ContactCalls = true;
local SizeLimitMin = 15; // Min size of all cores.
local SizeLimitMax = 100; // Max size of all cores.
local MaxSize = 50; // To which size this core will grow.
local MovementSpeed = 20;

local ActMap = {
	Swim = {
		Prototype = Action,
		Name = "Swim",
		Procedure = DFA_SWIM,
		Speed = 100,
		Accel = 16,
		Decel = 16,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Swim"
	},
	Walk = {
		Prototype = Action,
		Name = "Walk",
		Procedure = DFA_WALK,
		Speed = 100,
		Accel = 16,
		Decel = 16,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Walk",
		StartCall = "StartNonSwim",
		InLiquidAction = "Swim",
	},	
	Jump = {
		Prototype = Action,
		Name = "Jump",
		Procedure = DFA_FLIGHT,
		Speed = 100,
		Accel = 16,
		Decel = 16,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Jump",
		StartCall = "StartNonSwim",
		InLiquidAction = "Swim"
	},	
	Dead = {
		Prototype = Action,
		Name = "Dead",
		Procedure = DFA_NONE,
		Speed = 10,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		Directions = 2,
		FlipDir = 1,
		NextAction = "Hold",
		NoOtherAction = 1,
		ObjectDisabled = 1
	}
};