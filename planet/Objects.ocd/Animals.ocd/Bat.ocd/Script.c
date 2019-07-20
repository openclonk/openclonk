/**
	Bat
	Flutters around at night and in caves. The bat heals itself by
	biting prey animals or clonks. The less energy it has, the more
	agressive it will be towards preys.

	@author Win, Maikel
*/

#include Library_Animal


private func Construction()
{
	// Add effect for the behavior of the bat.	
	AddEffect("CoreBehavior", this, 100, 1, this);
	// Start flying.
	Fly();
	// Some bats don't stick to the swarm and start their own.
	if (!Random(100))
		MakeExpat(true);
	return _inherited(...);
}

public func IsBat() { return true; }

private func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 4 * amount;
	var loc_area = nil;
	if (rectangle) 
		loc_area = Loc_InArea(rectangle);
	var bats = [];
	var loc_bkg = Loc_Tunnel();
	// Place in sky as well if at night unless tunnel only is required.
	if (Time->IsNight() && (!settings || !settings.tunnel_only))
		loc_bkg = Loc_Or(Loc_Tunnel(), Loc_Sky());
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(loc_bkg, Loc_Space(10), loc_area);
		if (!spot)
			continue;
		
		var bat = CreateObject(this, spot.x, spot.y, NO_OWNER);
		if (!bat) 
			continue;
		
		if (bat->Stuck())
		{
			bat->RemoveObject();
			continue;
		}
		PushBack(bats, bat);
		--amount;
	}
	// Return a list of all created bats.
	return bats;
}


/*-- Behavior --*/

private func FxCoreBehaviorStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.move_direction = [0, 0];
	effect.attack_target = nil;
	effect.last_cave = [nil, nil];
	effect.time_since_startle = 0;
	effect.is_expat = false;
	return FX_OK;
}

// This time is called every frame for some basic actions.
private func FxCoreBehaviorTimer(object target, proplist effect, int time)
{
	// If the bat is hanging, either start flying or do nothing.
	if (GetAction() == "Hang")
	{
		// Start flying if there is nothing to hang on to.
		if (!GBackSolid(0, -12))
			Fly();
		// Start flying if it is day and not hanging in a tunnel, the bat needs to find a tunnel.
		if (GetMaterial(0, 0) != Material("Tunnel") && Time->IsDay())
			Fly();
		// Start flying by random.
		if (!Random(150))
			Fly();		
		return FX_OK;
	}
	
	/*-- Basic actions done while flying --*/

	// Remember direction we need to fly in to get back into the home cave.
	if (GetMaterial(0, 0) == Material("Tunnel"))
		effect.last_cave = [GetX(), GetY()];
	if (!PathFree(GetX(), GetY(), effect.last_cave[0], effect.last_cave[1]) || Distance(GetX(), GetY(), effect.last_cave[0], effect.last_cave[1]) > 30)
		effect.last_cave = [nil, nil];
		
	// Move the bat and update its in-flight appearance.
	ChangeMovementVelocity(effect.move_direction[0], effect.move_direction[1]);
	UpdateFlightAppearance();
	
	// Make flying sounds and sometimes do a sonar wave.
	if (!Random(250))
		Sound("Animals::Bat::Flutter*");
	if (!Random(225))
		DoSonarWave(false);	
		
	// Update the state of the bat.
	if (effect.time_since_startle > 0)
		effect.time_since_startle--;
		
	/*-- Random actions and actions based on events --*/
	
	// Only do these action updates every five frames.
	if ((time % 5) != 0)
		return FX_OK;

	// Stay out of water.
	if (GBackLiquid(0, 4) || GBackLiquid(0, 8))
	{
		effect.move_direction = [RandomX(-1, 1), -1];
		return FX_OK;
	}
	
	// Evade sources of light, fire and dangerous clonks (clonks are only attacked when in groups or very hungry).
	for (var danger in FindObjects(Find_Or(Find_OCF(OCF_CrewMember | OCF_OnFire), Find_Func("IsLightSource")), Find_Distance(75), Find_Exclude(this), Sort_Distance()))
	{
		if (!PathFree(GetX(), GetY(), danger->GetX(), danger->GetY()))
			continue;
		if ((danger->GetOCF() & OCF_CrewMember) && WillAttackPrey())
			continue;
		var x = -Sign(danger->GetX() - GetX() + RandomX(-20, 20));
		var y = -Sign(danger->GetY() - GetY() + RandomX(-20, 20));
		effect.move_direction = [x, y];
		DoSonarWave();
		// Loose the attack target, because safety is more important.
		effect.attack_target = nil;
		return FX_OK;
	}
	
	// Fly towards attack target when the bat has one.
	if (effect.attack_target)
	{
		// Give up chase after a while.
		if (Distance(GetX(), GetY(), effect.attack_target->GetX(), effect.attack_target->GetY()) > 200 || !PathFree(GetX(), GetY(), effect.attack_target->GetX(), effect.attack_target->GetY()))
		{
			effect.attack_target = nil;
			return FX_OK;
		}
		var x = Sign(effect.attack_target->GetX() - GetX());
		var y = Sign(effect.attack_target->GetY() - GetY());
		effect.move_direction = [x, y];

		if (Distance(GetX(), GetY(), effect.attack_target->GetX(), effect.attack_target->GetY()) < 10)
		{
			if (!effect.attack_target->Contained())	
				BitePrey(effect.attack_target);
			effect.attack_target = nil;
			SetRandomDirection();
		}
		return FX_OK;
	}
	
	// Look for prey to attack and bite for health.
	if (!Random(40))
	{
		var possible_prey = FindObject(Find_Func("IsPrey"), Find_OCF(OCF_Alive), Find_Distance(100), Find_NoContainer());
		// Check if path free and if there are enough buddies to mount an attack with.
		if (possible_prey && PathFree(GetX(), GetY(), possible_prey->GetX(), possible_prey->GetY()) && WillAttackPrey())
		{
			effect.attack_target = possible_prey;
			DoSonarWave(true);
		}
		return FX_OK;
	}
	
	// Fly towards other bats to loosely make a swarm.
	if (!Random(70) && !effect.is_expat)
	{
		var fellow = FindObject(Find_ID(Bat), Find_OCF(OCF_Alive), Find_Distance(200), Find_Exclude(this));
		if (fellow != nil)
		{
			var x = Sign(fellow->GetX() - GetX());
			var y = Sign(fellow->GetY() - GetY());
			effect.move_direction = [x, y];
		}
		return FX_OK;
	}
	
	// Explore.
	if (!Random(80))
	{
		SetRandomDirection();
		return FX_OK;
	}
	
	// If the bat has no activity move to the last known cave.
	if (GBackSky(0, 0))
	{
		if (effect.last_cave[0] != nil && effect.last_cave[1] != nil)
		{
			var x = Sign(effect.last_cave[0] - GetX());
			var y = Sign(effect.last_cave[1] - GetY());
			effect.move_direction = [x, y];
		}
		return FX_OK;
	}
	return FX_OK;
}

// An expat bat will not stay in the group.
public func MakeExpat(bool expat)
{
	var behavior_control = GetEffect("CoreBehavior", this);
	if (behavior_control)
		behavior_control.is_expat = expat;
	return;
}

// When some object hurts the bat.
public func CatchBlow(int damage, object obj)
{
	// Only catch a blow if alive.
	if (GetAction() == "Dead")
		return;
	// Make a sound.
	Sound("Animals::Bat::Noise*");
	// Get the most probable crew member causing this blow.
	var by_crewmember = GetCursor(obj->GetController());
	// When hurt, startle this bat and nearby bats.
	Startle(by_crewmember);
	for (var swarm_member in FindObjects(Find_ID(Bat), Find_OCF(OCF_Alive), Find_Distance(200), Find_Exclude(this)))
		swarm_member->Startle(by_crewmember);
	return;
}

// Startles the bat to make it fly if hanging and register the causing crew member.
private func Startle(object by_crewmember)
{
	var behavior_control = GetEffect("CoreBehavior", this);
	if (GetAction() == "Hang")
	{
		Fly();
		ScheduleCall(this, "Sound", Random(15), 0, "Animals::Bat::Noise*");
		if (behavior_control)
			behavior_control.time_since_startle = 150;
	}
	var behavior_control = GetEffect("CoreBehavior", this);
	if (behavior_control)
		behavior_control.attack_target = by_crewmember;
	return;
}

// Returns whether the bat is still startled.
private func IsStartled()
{
	var behavior_control = GetEffect("CoreBehavior", this);
	if (!behavior_control)
		return false;
	return behavior_control.time_since_startle > 0;
}

// Changes the direction the bat currently wants to move to.
private func ChangeMovementDirection(int x, int y)
{
	var behavior_control = GetEffect("CoreBehavior", this);
	if (behavior_control)
		behavior_control.move_direction = [Sign(x), Sign(y)];
	return;
}

// Sets the bats current movement direction to the location with coordinates (x, y).
// If you specify the speed, the bat will also change its speed.
private func ChangeMovementVelocity(int x, int y, int speed)
{
	// Set the default speed.
	speed = speed ?? 120;
	var weight = 500;
	if (x > 0 && GetDir() == DIR_Left)
		SetDir(DIR_Right);
	if (x < 0 && GetDir() == DIR_Right)
		SetDir(DIR_Left);
	SetXDir((GetXDir(100) * (1000 - weight) + x * speed * weight) / 1000, 100);
	SetYDir((GetYDir(100) * (1000 - weight) + y * speed * weight) / 1000, 100);
	return;
}

private func UpdateFlightAppearance()
{
	// The rotation around the z-axis depends on the angle of the bat.
	var angle = Angle(0, 0, GetXDir(), GetYDir());
	var dir = 1;
	if (GetXDir() > 0)
	{
		angle = BoundBy(angle, 30, 150);
		dir = 1;
	}
	else
	{
		angle = 2 * 270 - BoundBy(angle, 210, 330);
		dir = -1;	
	}
	this.MeshTransformation = Trans_Mul(Trans_Scale(1000, dir * 1000, 1000), Trans_Rotate(angle, 0, 0, 1));
	return;
}

// Changes the direction of the bat randomly.
private func SetRandomDirection()
{
	var x = RandomX(-1, 1);
	var y = RandomX(-1, 1);
	ChangeMovementDirection(x, y);
	ChangeMovementVelocity(x, y);
	return;
}

// Lets the bat start flying.
private func Fly()
{
	// Stop current animation and start flying.
	PlayAnimation("Fly", 1, Anim_Linear(0, 0, GetAnimationLength("Fly"), 10, ANIM_Loop));
	SetAction("Flight");
	SetComDir(COMD_None);			
	SetRandomDirection();
	return;
}

private func Hang()
{
	// Stop current animation and start hanging.
	PlayAnimation("Hang", 1, Anim_Linear(0, 0, GetAnimationLength("Hang"), 50, ANIM_Loop));
	// Set the action and stop moving.
	SetAction("Hang");
	SetComDir(COMD_Stop);
	SetXDir(0);
	SetYDir(0);
	this.MeshTransformation = Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Rotate(Random(360), 1, 0, 0));
	return;
}

// Lets the bat do a sonar wave to echolocate itself.
private func DoSonarWave(bool agressive)
{
	// Handle cool down for the sonar wave.
	if (GetEffect("SonarWaveCoolDown", this))
		return;
	AddEffect("SonarWaveCoolDown", this, 100, 36 * 4, this);
	// Define sonar wave particles.
	var sonar_particle = {
		R = 255,
		B = 255,
		G = 255,
		Alpha = PV_Linear(255, 0),
		Size = PV_Linear(6, 72),
		//Stretch = 1000,
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = 0,
		CollisionVertex = 0,
		OnCollision = PC_Stop(),
	};
	if (agressive)
	{
		sonar_particle.R = 255;
		sonar_particle.G = 0;
		sonar_particle.B = 0;
	}
	// Create the particle.
	CreateParticle("Shockwave", 0, 0, 0, 0, 24, sonar_particle, 1);
	// Make a sound dependent on the agression.		
	if (agressive)
		Sound("Animals::Bat::Chirp");
	else
		Sound("Animals::Bat::Noise*", false, 50);	
	return;
}

// Cool down effect which is removed upon first timer call.
public func FxSonarWaveCoolDownTimer(object target, proplist effect, int time)
{
	return FX_Execute_Kill;
}

// The aggressiveness depends on the amount of health the bat is missing, ranges from 0 to 100.
private func GetAggressiveness()
{
	return 10 * (this.MaxEnergy - 1000 * GetEnergy()) / this.MaxEnergy;
}

// Whether the bat will attack crew members and other living animals depends on how many they are in a group and their aggressiveness.
private func WillAttackPrey()
{
	var nr_buddies = ObjectCount(Find_ID(Bat), Find_OCF(OCF_Alive), Find_Distance(200));
	var buddies_needed = 4 * GetAggressiveness() / 100;
	return nr_buddies > buddies_needed;
}

// Lets the bat bite a pray animal, assumes the bat is close to the prey.
private func BitePrey(object prey)
{
	// A small blow to the prey and heal the bat itself.
	prey->DoEnergy(-4);
	prey->~CatchBlow(4, this);
	DoEnergy(4);
	// Make a bite/attack sound.
	Sound("Hits::ProjectileHitLiving*");
	return;
}

private func ContactBottom()
{
	// The dead bat changes it animation once it has touched the ground.
	if (!GetAlive())
		PlayAnimation("Dead", 1, Anim_Linear(0, 0, GetAnimationLength("Dead"), 1, ANIM_Hold));
	return UpdateMovementDirectionOnContact();
}

private func ContactTop()
{
	if (!IsStartled() && !Random(5))
	{
		Hang();
		return;
	}
	if (!IsStartled() && Time->IsDay()) 
	{
		Hang();
		return;
	}
	return UpdateMovementDirectionOnContact();
}

private func ContactLeft()
{
	return UpdateMovementDirectionOnContact();
}

private func ContactRight()
{
	return UpdateMovementDirectionOnContact();
}

private func UpdateMovementDirectionOnContact()
{
	var contact = GetContact(-1);
	var xdir = RandomX(-1, 1);
	var ydir = RandomX(-1, 1);
	if (contact & CNAT_Right)
		xdir = Min(xdir, 0);
	if (contact & CNAT_Left)
		xdir = Max(xdir, 0);
	if (contact & CNAT_Bottom)
		ydir = Min(ydir, 0);
	if (contact & CNAT_Top)
		ydir = Max(ydir, 0);
	if (xdir == 0 && ydir == 0)
	{
		xdir = RandomX(-1, 1);
		ydir = RandomX(-1, 1);
	}
	ChangeMovementDirection(xdir, ydir);
	return;
}

/*-- Dead State --*/

private func Death()
{
	// Stop current animation and play dying animation.
	PlayAnimation("Land", 1, Anim_Linear(0, 0, GetAnimationLength("Land"), 19, ANIM_Hold));
	this.MeshTransformation = Trans_Rotate(Random(360), 0, 1, 0);
	
	// Remove behavior effect and play dead animation.
	RemoveEffect("CoreBehaviour", this);
	SetAction("Dead");
	// Set border bound to zero when dead, also disable contactcalls.
	this.BorderBound = 0;
	this.ContactCalls = false;
	
	Decay();
}


/*-- Reproduction --*/

// Overloading animal library default values
local animal_reproduction_area_size = 1200;
local animal_reproduction_rate = 50;
local animal_max_count = 10;

// Only bats with full health reproduce.
private func SpecialReproductionCondition()
{
	return this->GetEnergy() >= this.MaxEnergy / 1000;
}

// Overload animal library: Only set new completion.
public func Birth(object parent)
{
	SetCon(RandomX(40, 60));
	return;
}


/*-- Properties --*/

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(3000, 0, 0), Trans_Rotate(-65, 0, 1, 0), Trans_Rotate(-35, 0, 0, 1));
	return _inherited(def, ...);
}

local Name = "$Name$";
local Description = "$Description$";

local MaxEnergy = 20000;
local MaxBreath = 180;
local NoBurnDecay = true;
local ContactIncinerate = 10;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;

local ActMap = {
	Hang = {
		Prototype = Action,
		Name = "Hang",
		Procedure = DFA_FLOAT,
		Speed = 100,
		Accel = 16,
		Decel = 16,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Hang",
	},
	Flight = {
		Prototype = Action,
		Name = "Flight",
		Procedure = DFA_FLOAT,
		Speed = 100,
		Accel = 16,
		Decel = 16,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Flight",
	},
	Dead = {
		Prototype = Action,
		Name = "Dead",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		Directions = 2,
		FlipDir = 1,
		NextAction = "Hold",
		NoOtherAction = 1,
		ObjectDisabled = 1,
	}
};