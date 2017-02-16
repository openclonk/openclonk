/**
	Boom Attack
	An evil rocket which attacks you, can be ridden as well.

	@authors Randrian, Newton, Sven2
*/


public func Construction()
{
	SetAction("Fly");
	SetComDir(COMD_None);
	// Notify friendly fire rule.
	GameCallEx("OnCreationRuleNoFF", this);
	// Add flight effects.
	CreateEffect(FxFlightRotation, 100, 1);
	CreateEffect(FxFlight, 100, 2);
	return;
}


/*-- Flight --*/

// Rotates the boom attack slowly around its axis.
local FxFlightRotation = new Effect
{
	Construction = func()
	{
		this.rotation = 0;	
	},
	Timer = func(int time)
	{
		if (Target->GetRider())
			return FX_Execute_Kill;

		this.rotation += 2;
		if (this.rotation >= 360)
			this.rotation = 0;

		Target.MeshTransformation = Trans_Rotate(this.rotation, 0, 1, 0);
		return FX_OK;
	}
};

// Controls the boom attack flight by flying through the given waypoints or to the target.
// The way points is a list of coordinates as [{X = ??, Y = ??}, ...]. The waypoints are
// dealt with first and then the target is aimed for.
local FxFlight = new Effect
{
	Construction = func()
	{
		this.target = GetRandomAttackTarget(Target);
		// Get the boom attack waypoints from the scenario or make an array.
		this.waypoints = GameCall("GetBoomAttackWaypoints", Target) ?? [];
		this.current_waypoint = nil;
		if (this.target)
		{
			var dx = this.target->GetX() - Target->GetX();
			var dy = this.target->GetY() + this.target->GetBottom() - Target->GetY();
			Target->SetR(Angle(0, 0, dx, dy));
		}
		this->Timer(0);	
	},
	Timer = func(int time)
	{
		// Find target and if not explode.
		if (!this.target)
		{
			this.target = GetRandomAttackTarget(Target);
			if (!this.target && !this.current_waypoint && GetLength(this.waypoints) == 0)
			{
				Target->DoFireworks(NO_OWNER);
				return FX_Execute_Kill;	
			}
		}
		
		// Check if reached current waypoint.
		if (this.current_waypoint)
			if (Distance(this.current_waypoint.X, this.current_waypoint.Y, Target->GetX(), Target->GetY()) < 8)
				this.current_waypoint = nil;
		
		// Get relative coordinates to target.
		var dx, dy;
		
		// Handle waypoints and get coordinates to move to.
		if (this.current_waypoint || GetLength(this.waypoints) > 0)
		{
			if (!this.current_waypoint)
				this.current_waypoint = PopFront(this.waypoints);
			// Set relative coordinates to new waypoint.
			dx = this.current_waypoint.X - Target->GetX();
			dy = this.current_waypoint.Y - Target->GetY();		
		}
		
		// Explode if close enough to target.
		if (ObjectDistance(Target, this.target) < 12)
		{
			Target->DoFireworks(NO_OWNER);
			return FX_Execute_Kill;	
		}
		
		// Get relative coordinates to target.
		if (this.target && !this.current_waypoint)
		{
			dx = this.target->GetX() - Target->GetX();
			dy = this.target->GetY() + this.target->GetBottom() - Target->GetY();
			// Check if path is free to target, if not try to find a way around using waypoints.
			if (!PathFree(this.target->GetX(), this.target->GetY(), Target->GetX(), Target->GetY())/* && !Target->GBackSolid(dx, dy)*/)
			{
				// Try to set a waypoint half way on a line orthogonal to the current direction.
				for (var attempts = 0; attempts < 40; attempts++)
				{
					var d = Sqrt(dx**2 + dy**2);
					var try_dist = Max(20 + 2 * attempts, d * attempts / 80) + RandomX(-10, 10);
					var line_dist = (2 * Random(2) - 1) * try_dist;
					var way_x = Target->GetX() + dx / 2 + dy * line_dist / d;
					var way_y = Target->GetY() + dy / 2 - dx * line_dist / d;
					// Path to new waypoint must be free and inside the landscape borders.
					if (!PathFree(Target->GetX(), Target->GetY(), way_x, way_y) || !PathFree(this.target->GetX(), this.target->GetY(), way_x, way_y))
						continue;
					if (!Inside(way_x, 0, LandscapeWidth()) || !Inside(way_y, 0, LandscapeHeight()))
						continue; 
					this.current_waypoint = {X = way_x, Y = way_y};
					break;
				}
			}
		}
		
		// At this distance, fly horizontally. When getting closer, gradually turn to direct flight into target.
		if (this.target && !this.current_waypoint)
		{
			var aim_dist = 600; 
			dy = dy * (aim_dist - Abs(dx)) / aim_dist;
		}
		var angle_to_target = Angle(0, 0, dx, dy);
		var angle_rocket = Target->GetR();
		if (angle_rocket < 0)
			angle_rocket += 360;
		// Gradually update the angle.
		var angle_delta = angle_rocket - angle_to_target;
		if (Inside(angle_delta, 0, 180) || Inside(angle_delta, -360, -180))
			Target->SetR(Target->GetR() - Min(4, Abs(angle_delta)));
		else if (Inside(angle_delta, -180, 0) || Inside(angle_delta, 180, 360))
			Target->SetR(Target->GetR() + Min(4, Abs(angle_delta)));	

		// Update velocity according to angle.
		Target->SetXDir(Sin(Target->GetR(), Target.FlySpeed), 100);
		Target->SetYDir(-Cos(Target->GetR(), Target.FlySpeed), 100);
	
		// Create exhaust fire.
		var x = -Sin(Target->GetR(), 15);
		var y = +Cos(Target->GetR(), 15);	
		var xdir = Target->GetXDir() / 2;
		var ydir = Target->GetYDir() / 2;
		Target->CreateParticle("FireDense", x, y, PV_Random(xdir - 4, xdir + 4), PV_Random(ydir - 4, ydir + 4), PV_Random(16, 38), Particles_Thrust(), 5);
		return FX_OK;
	},
	
	AddWaypoint = func(proplist waypoint)
	{
		PushBack(this.waypoints, waypoint);	
	},
	
	SetWaypoints = func(array waypoints)
	{
		this.waypoints = waypoints;
	}	
};


/*-- Waypoints --*/

public func AddWaypoint(proplist waypoint)
{
	var fx = GetEffect("FxFlight", this);
	if (fx)
		fx->AddWaypoint(waypoint);
	return;
}

public func SetWaypoints(array waypoints)
{
	var fx = GetEffect("FxFlight", this);
	if (fx)
		fx->SetWaypoints(waypoints);
	return;
}


/*-- Riding --*/

local riderattach;
local rider;

public func SetRider(object to)
{
	rider = to;
	return;
}

public func GetRider() { return rider; }

public func OnMount(object clonk)
{
	SetRider(clonk);
	var dir = -1;
	if (GetX() > LandscapeWidth() / 2)
		dir = 1;
	clonk->PlayAnimation("PosRocket", CLONK_ANIM_SLOT_Arms, Anim_Const(0));
	riderattach = AttachMesh(clonk, "main", "pos_tool1", Trans_Translate(-1000, 2000 * dir, 2000));
	return true;
}

public func OnUnmount(object clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	DetachMesh(riderattach);
	return;
}


/*-- Explosion --*/

public func IsProjectileTarget(target,shooter) { return true; }
public func OnProjectileHit(object shot) { return DoFireworks(shot->GetController()); }

public func ContactBottom() { return Hit(); }
public func ContactTop() { return Hit(); }
public func ContactLeft() { return Hit(); }
public func ContactRight() { return Hit(); }

public func Hit() { return DoFireworks(NO_OWNER); }
public func HitObject() { return DoFireworks(NO_OWNER); }

private func DoFireworks(int killed_by)
{
	if (rider)
	{
		rider->Fling(RandomX(-5, 5), -5);
		rider->SetAction("Walk");
		SetRider(nil);
	}
	// Notify defense goal for reward and score.
	var defense = FindObject(Find_ID(Goal_Defense));
	if (defense)
		defense->~OnClonkDeath(this, killed_by);
	Fireworks();
	Explode(40);
	return;
}

public func Destruction()
{
	// Notify friendly fire rule.
	GameCallEx("OnDestructionRuleNoFF", this);
}

public func HasNoNeedForAI() { return true; }


/*-- Properties --*/

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Length = 1,
		Delay = 0,
		Wdt = 15,
		Hgt = 27,
	}
};

local Name = "$Name$";
local Description = "$Description$";
local ContactCalls = true;
local FlySpeed = 100;
local HasNoFriendlyFire = true;
