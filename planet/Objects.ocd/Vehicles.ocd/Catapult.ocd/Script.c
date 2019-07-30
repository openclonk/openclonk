/**
	Catapult
	Tosses objects farther than a clonk can, withour requiring gunpowder.

	@author Ringwaul
*/

#include Library_ElevatorControl
#include Library_Destructible

local aim_anim;
local turn_anim;
local olddir;
local dir;
local clonkmesh;

// Shooting strength for normal launching and self launches.
static const CATAPULT_MaxPower = 100;
static const CATAPULT_MaxPower_SelfLaunch = 75;

public func IsVehicle() { return true; }
public func IsArmoryProduct() { return true; }
public func FitsInDoubleElevator() { return true; }

protected func Initialize()
{
	dir = DIR_Right;
	olddir = DIR_Right;
	SetAction("Roll");
	aim_anim =  PlayAnimation("ArmPosition", 1,  Anim_Const(0));
	turn_anim = PlayAnimation("TurnRight", 5, Anim_Const(GetAnimationLength("TurnRight")));
}

protected func ContactLeft()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

protected func ContactRight()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

public func SetDir(int to_dir)
{
	if (to_dir == DIR_Left)
	{
		turn_anim = PlayAnimation("TurnLeft", 5, Anim_Const(GetAnimationLength("TurnLeft")));
		dir = DIR_Left;
	}
	if (to_dir == DIR_Right)
	{
		turn_anim = PlayAnimation("TurnRight", 5, Anim_Const(GetAnimationLength("TurnRight")));
		dir = DIR_Right;
	}
	return inherited(dir, ...);
}

/*-- Controls --*/

// Activate turn animations
func ControlLeft(object clonk)
{
	dir = DIR_Left;
	if (dir != olddir)
	{
		olddir = dir;

		// If the animation is playing for the other direction, turn back from where it already is in the animation.
		var animstart = 0;
		if (GetAnimationPosition(turn_anim) != GetAnimationLength("TurnRight"))
			animstart = GetAnimationLength("TurnRight") - GetAnimationPosition(turn_anim);

		turn_anim = PlayAnimation("TurnLeft", 5, Anim_Linear(animstart, 0, GetAnimationLength("TurnLeft"), Max(36 - (animstart * 204617 / 10000000), 1), ANIM_Hold));
	}
}

func ControlRight(object clonk)
{
	dir = DIR_Right;
	if (dir != olddir)
	{
		olddir = dir;

		// If the animation is playing for the other direction, turn back from where it already is in the animation.
		var animstart = 0;
		if (GetAnimationPosition(turn_anim) != GetAnimationLength("TurnLeft"))
			animstart = GetAnimationLength("TurnLeft") - GetAnimationPosition(turn_anim);

		turn_anim = PlayAnimation("TurnRight", 5, Anim_Linear(animstart, 0, GetAnimationLength("TurnRight"), Max(36 - (animstart * 204617 / 10000000), 1), ANIM_Hold));
	}
}

public func TurnLeft()
{
	// Instantly set catapult orientation to face left
	var len = GetAnimationLength("TurnLeft");
	turn_anim = PlayAnimation("TurnLeft", 5, Anim_Linear(len, 0, len, 1, ANIM_Hold));
	dir = olddir = DIR_Left;
	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseStart(object clonk)
{
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	var power = DefinePower(x, y);
	DoArmAnimation(power);
	ShowTrajectory(power);
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	DoFire(clonk, DefinePower(x, y));
	return true;
}

public func ControlUseCancel(object clonk)
{
	Trajectory->Remove(this);
	return true;
}

public func ContainedUseStart(object clonk, int x, int y)
{
	return true;
}

public func ContainedUseHolding(object clonk, int x, int y)
{
	var power = DefinePower(x, y, CATAPULT_MaxPower_SelfLaunch);
	DoArmAnimation(power);
	ShowTrajectory(power);
	return true;
}

public func ContainedUseStop(object clonk, int x, int y)
{
	DoFire(clonk, DefinePower(x, y, CATAPULT_MaxPower_SelfLaunch));
	return true;
}

public func ContainedUseCancel(object clonk)
{
	Trajectory->Remove(this);
	return true;
}


/*-- Shooting --*/

// Define the catapult power according to where the player aims.
// Power will try to match such that the trajectory goes through (x, y).
public func DefinePower(int x, int y, int max_power)
{
	if (max_power == nil)
		max_power = CATAPULT_MaxPower;
	var min_power = 20;
	// Correct coordinates for the projectile exit.	
	var exit = GetProjectileExit();
	x -= exit[0];
	y -= exit[1];
	// Ensure max_power if aiming above the diagonal line.
	if (y + (2 * dir - 1) * x <= 0)
		return max_power;
	// Calculate the exit speed using a 45 degree angle.
	var vsquared = x**2 * GetGravity() / (y + (2 * dir - 1) * x);
	if (vsquared < 0)
		return min_power;
	return BoundBy(Sqrt(vsquared), min_power, max_power);
}

public func DoArmAnimation(int power)
{
	SetAnimationPosition(aim_anim, Anim_Const(759 - (power * 759 / 100)));
	return;
}

public func ShowTrajectory(int power)
{
	var exit = GetProjectileExit();
	var x = GetX() + exit[0];
	var y = GetY() + exit[1];
	var angle = exit[2] + GetR();
	var xdir = Sin(angle, power);
	var ydir = -Cos(angle, power);
	Trajectory->Create(this, x, y, xdir, ydir);
	return;
}

protected func DoFire(object clonk, int power)
{
	// Play the fire animation.
	aim_anim = PlayAnimation("ArmPosition", 1, Anim_Linear(GetAnimationPosition(aim_anim), 0, GetAnimationLength("ArmPosition"), 3, ANIM_Hold));

	// Sound.
	Sound("Objects::Catapult_Launch");
			
	// Remove trajectory display.
	Trajectory->Remove(this);
	
	// The clonk will be the projectile if he sits in the catapult.
	var projectile;
	if (clonk && (clonk->Contained() == this))
		projectile = clonk;
	// If clonk isn't sitting in there, shoot stuff placed into the catapult
	if (!projectile)
		projectile = Contents(0);
	// Otherwise, fire what is in the clonk's hand.
	if (!projectile)
		projectile = clonk->GetHandItem(0);

	// Don't do anything further if there is no projectile.
	if (!projectile)
		return;
		
	// Find the spot of the catapult's arm depending on rotation.
	var exit = GetProjectileExit();
	var x = exit[0];
	var y = exit[1];
	var angle = exit[2] + GetR();

	projectile->Exit();
	// Put the ammo at the catapult's arm.
	projectile->SetPosition(GetX() + x, GetY() + y);

	// Special behavior for crew members.
	if (projectile->GetOCF() & OCF_CrewMember)
	{
		CatapultDismount(projectile);
		projectile->SetAction("Tumble");
		
		// Make sure the Clonk can't shoot itself into solid material.
		if (projectile->Stuck())
		{
			// First, try to just put the Clonk a few pixels lower - might still look okay in some situations.
			for (var i = 0; i <= 4; ++i)
			{
				projectile->SetPosition(projectile->GetX(), projectile->GetY() + 2);
				if (!projectile->Stuck()) 
					break;
			}
			// Then as a safeguard, just place the Clonk at the catapult's feet and do nothing.
			if (projectile->Stuck())
			{
				projectile->SetPosition(GetX(), GetY());
				// Still stuck? Then we don't actually care if stuck here or at the end of the arm.
				if (projectile->Stuck())
				{
					// Go back to normal shooting position.
					projectile->SetPosition(GetX() + x, GetY() + y);
				}
				else
				{
					// We set the Clonk back down on the ground. This is not a normal shot.
					angle = 0;
					power = 20;
				}
			}
		}
	}

	// Set the speed of the projectile.
	projectile->SetVelocity(angle + GetR(), power);
	return;
}

private func GetProjectileExit()
{
	var xdir = 1;
	if (dir == DIR_Left)
		xdir = -1;
	var x = 8 * xdir;
	var y = -28;
	var angle = 45 * xdir;
	return [x, y, angle];
}

public func ActivateEntrance(object clonk)
{
	var cnt = ObjectCount(Find_Container(this), Find_OCF(OCF_CrewMember));
	if (cnt > 0)
	{
		if (clonk->Contained() == this)
		{
			CatapultDismount(clonk);
			clonk->Exit();
		}
		return;
	}
	if (cnt == 0)
	{
		clonk->Enter(this);
		SetOwner(clonk->GetController());
		clonkmesh = AttachMesh(clonk,"shot","skeleton_body",Trans_Mul(Trans_Rotate(180, 1, 0, 0), Trans_Translate(-3000, 1000, 0)), AM_DrawBefore);
		clonk->PlayAnimation("CatapultSit", CLONK_ANIM_SLOT_Movement, Anim_Const(0), Anim_Const(1000));
		DoArmAnimation(CATAPULT_MaxPower_SelfLaunch);
	}
	return;
}

public func CatapultDismount(object clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(CLONK_ANIM_SLOT_Movement));
	DetachMesh(clonkmesh);
	clonkmesh = nil;
	return true;
}


/* Register enemy spawn with catapult */

func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-1000, -4000, 0), Trans_Rotate(-20, 1, 0, 0), Trans_Rotate(35, 0, 1, 0));
	var spawn_editor_props = { Type="proplist", Name = def->GetName(), EditorProps= {
		Gunner = new EnemySpawn->GetAICreatureEditorProps(EnemySpawn->GetAIClonkDefaultPropValues("Firestone"), "$NoGunnerHelp$") { Name="$Gunner$", EditorHelp="$GunnerHelp$" },
	} };
	var spawn_default_values = {
		Gunner = { Type="Clonk", Properties = EnemySpawn->GetAIClonkDefaultPropValues("Firestone") },
	};
	EnemySpawn->AddEnemyDef("Catapult",
			{ SpawnType = Catapult,
				SpawnFunction = def.SpawnCatapult,
				OffsetAttackPathByPos = false,
				GetInfoString = def.GetSpawnInfoString },
		spawn_default_values,
		spawn_editor_props);
}

private func SpawnCatapult(array pos, proplist enemy_data, proplist enemy_def, array attack_path, object spawner)
{
	// First spawn the catapult
	var catapult = CreateObjectAbove(Catapult, pos[0], pos[1], g_enemyspawn_player);
	catapult->Unstick(10);
	if (!catapult) return;
	// Next let a clonk steer the catapult
	var clonk = EnemySpawn->SpawnAICreature(enemy_data.Gunner, pos, enemy_def, attack_path, spawner);
	if (!clonk) return;
	clonk->SetAction("Push", catapult);
	// Set attack mode
	AI->SetVehicle(clonk, catapult);
	// Only the clonk is an actual enemy
	return clonk;
}

private func GetSpawnInfoString(proplist enemy_data)
{
	// Prepend balloon to clonk info string
	return Format("{{Catapult}}%s", EnemySpawn->GetAICreatureInfoString(enemy_data.Gunner));
}

/* Properties */

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;
local Components = {Metal = 1, Wood = 6};
local HitPoints = 100;

local ActMap = {
	Roll = {
		Prototype = Action,
		Name = "Roll",
		Procedure = DFA_NONE,
		Directions = 2,
		//FlipDir = 1,
		Length = 50,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 22,
		Hgt = 16,
		NextAction = "Roll",
	},
};
