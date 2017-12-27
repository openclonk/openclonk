/**
	Lightning Bolt
	Launch a single bolt of lightning which splits up into smaller branches.
	
	@author Maikel
*/

local xdir, ydir;
local xdev, ydev;
local strength;
local launcher;

// Creates a lightning bolt at the specified location. 
// x: X coordinate, always global.
// y: Y coordinate, always global.
// strength: Strength of the bolt, 0 - 100.
// xdir: Average horizontal speed of the bolt.
// ydir: Average vertical speed of the bolt.
// xdev: Maximum deviation from the average horizontal speed.
// ydev: Maximum deviation from the average vertical speed.
// no_sound: Whether to not play a sound.
global func LaunchLightning(int x, int y, int strength, int xdir, int ydir, int xdev, int ydev, bool no_sound)
{
	var lightning = CreateObject(Lightning, x - GetX(), y - GetY());
	// Ignore the launching object if not called from effect, scenario, etc..
	var launching_object = nil;
	if (this && GetType(this) == C4V_C4Object)
		launching_object = this;
	return lightning && lightning->Launch(x, y, strength, xdir, ydir, xdev, ydev, no_sound, launching_object);
}

public func Launch(int x, int y, int to_strength, int to_xdir, int to_ydir, int to_xdev, int to_ydev, bool no_sound, to_launcher)
{
	xdir = to_xdir; ydir = to_ydir;
	xdev = to_xdev; ydev = to_ydev;
	strength = to_strength ?? 20;
	launcher = to_launcher;
	AddVertex(x - GetX(), y - GetY());
	AddEffect("LightningMove", this, 1, 1, this, nil, !no_sound);
	return true;
}

protected func FxLightningMoveStart(object target, effect fx, int temp, bool play_sound)
{
	if (temp)
		return FX_OK;
	fx.play_sound = play_sound;
	if (play_sound && strength > 30)
		Sound("Environment::Lightning::Thunder?", false, strength);
	return FX_OK;
}

protected func FxLightningMoveTimer(object target, effect fx, int time)
{
	// Calculate new coordinates to move to.
	var vertices = GetVertexNum();
	var oldx = GetVertex(vertices - 1, 0);
	var oldy = GetVertex(vertices - 1, 1);
	var newx = oldx + xdir + xdev - Random(2 * xdev);
	var newy = oldy + ydir + ydev - Random(2 * ydev);
	
	// Check if a lightning attractor is in range.
	var range = Distance(0, 0, xdir, ydir);
	var cone_angle = Angle(0, 0, xdir, ydir);
	var cone_width = 30 + Distance(0, 0, xdev, ydev);
	var attractor = FindObject(Find_Cone(3 * range / 2, cone_angle, cone_width, oldx, oldy), Find_Property("IsLightningAttractor"), Find_Exclude(launcher), Find_NoContainer(), Sort_Distance(oldx, oldy));
	if (attractor)
	{
		// Move to lightning attractor.
		newx = attractor->GetX() - GetX();
		newy = attractor->GetY() - GetY();
	}
	
	// Check if lightning hits landscape, and adapt new coordinates.
	// Open question: should it penetrate liquids?
	var strike_solid = PathFree2(oldx + GetX(), oldy + GetY(), newx + GetX(), newy + GetY());
	if (strike_solid)
	{
		newx = strike_solid[0] - GetX();
		newy = strike_solid[1] - GetY();
	}
	AddVertex(newx, newy);
	
	// Draw the new line with lightning particles.
	DrawLightningLine(oldx, oldy, newx, newy, strength / 10, Min(16, strength / 3));
	
	// Update the light range for this strike, it is set at the new position with the given strength.
	SetLightRange(strength / 2, strength);
	SetLightColor(RGB(60, 60, 192));
	this.LightOffset = [newx, newy];
	
	// Strike objects on the line: only objects that are vehicle, items, alive or structures.
	for (var obj in FindObjects(Find_OnLine(oldx, oldy, newx, newy), Find_Or(Find_Category(C4D_Object | C4D_Living | C4D_Vehicle | C4D_Structure), Find_Func("IsLightningStrikable", this)), Find_Exclude(launcher), Find_NoContainer()))
	{
		var damage = 3 + strength / 10;
		// Check if the object rejects a lightning strike, also check if object still exists because an object
		// at the same location may have been struck first and have removed nearby objects.
		if (obj && !obj->~RejectLightningStrike(this, damage))
		{
			var is_attractor = obj.IsLightningAttractor;
			// Do a callback notifying the object that it has been struck by lightning.
			obj->~OnLightningStrike(this, damage);
			// Damage or hurt objects. Lightning strikes may have a controller, thus pass this for kill tracing.
			if (obj)
			{
				if (obj->GetOCF() & OCF_Alive)
					Punch(obj, damage);
				else
					obj->DoDamage(damage, FX_Call_DmgScript, GetController());
			}
			// Reduce strength of the lightning if an object is struck.
			strength -= damage;
			// Remove lightning if too weak or an attractor has been struck.
			if (strength <= 0 || is_attractor)
			{
				RemoveObject();
				return FX_OK;
			}
		}
	}
	
	// Remove lightning, if struck landscape or if strength is too low.
	if (strike_solid || (strength < 50 && !Random(strength / 4)))
	{
		RemoveObject();
		return FX_OK;
	}
	// Branch with chance inversely proportional to strength.
	if (strength > 24 && Random(Sqrt(10 * strength)) > 12)
	{
		var branch_strength = (strength + Random(strength)) / 4;
		var lightning = CreateObject(Lightning, newx, newy);
		if (lightning)
			lightning->Launch(newx + GetX(), newy + GetY(), branch_strength, xdir, ydir, xdev, ydev, !fx.play_sound);
		strength -= branch_strength / 3;
	}
	return FX_OK;
}

private func DrawLightningLine(int x1, int y1, int x2, int y2, int distance, int size)
{
	// Need at least two particles: start and end.
	distance = Max(distance, 1);
	var angle = Angle(x1, y1, x2, y2);
	var count = Max(2, Distance(x1, y1, x2, y2) / distance);
	var deltax = x2 - x1, deltay = y2 - y1;
	// Create a set of blue particles and smaller white particles on the line.
	var particle_blue = 
	{
		R = 0,
		G = 0,
		B = 240,
		Alpha = PV_Linear(128, 0),
		Size = size,
		Rotation = angle,
		BlitMode = GFX_BLIT_Additive
	};
	var particle_white = 
	{
		R = 240,
		G = 240,
		B = 240,
		Alpha = PV_Linear(192, 0),
		Size = 3 * size / 4,
		Rotation = angle,
		BlitMode = GFX_BLIT_Additive
	};
	for (var i = 1; i <= count; i++)
	{
		CreateParticle("LightningBolt", x1 + deltax * i / (count + 1), y1 + deltay * i / (count + 1), 0, 0, 20, particle_blue);
		CreateParticle("LightningBolt", x1 + deltax * i / (count + 1), y1 + deltay * i / (count + 1), 0, 0, 20, particle_white);
	}
	return;
}

// Don't save in scenarios.
func SaveScenarioObject() { return false; }

local Name = "$Name$";
