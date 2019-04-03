/**
	Explode.c
	Everything about the explosion.
	
	@author Newton
*/

// Particle definitions used by the explosion effect.
// They will be initialized lazily whenever the first blast goes off.
static ExplosionParticles_Smoke;
static ExplosionParticles_Blast;
static ExplosionParticles_BlastSmooth;
static ExplosionParticles_BlastSmoothBackground;
static ExplosionParticles_Star;
static ExplosionParticles_Shockwave;
static ExplosionParticles_Glimmer;

global func ExplosionParticles_Init()
{
	ExplosionParticles_Smoke =
	{	    
		Size = PV_KeyFrames(0, 180, 25, 1000, 50),
	    DampingY = PV_Random(890, 920, 5),
		DampingX = PV_Random(900, 930, 5),
		ForceY=-1,
		ForceX = PV_Wind(20, PV_Random(-2, 2)),
		Rotation=PV_Random(0,360,0),
		R=PV_KeyFrames(0, 0, 255, 260, 64, 1000, 64),
		G=PV_KeyFrames(0, 0, 128,  260, 64, 1000, 64),
		B=PV_KeyFrames(0, 0, 0, 260, 108, 1000, 108),
	    Alpha = PV_KeyFrames(0, 0, 0, 100, 20, 500, 20, 1000, 0)
	};
	
	ExplosionParticles_Blast =
	{
		Size = PV_KeyFrames(0, 0, 0, 260, 25, 1000, 40),
		DampingY = PV_Random(890, 920, 0),
		DampingX = PV_Random(900, 930, 0),
		ForceY = PV_Random(-8,-2,0),
		ForceX = PV_Random(-5,5,0),
		R = 255,
		G = PV_Random(64, 120, 0),
		Rotation = PV_Random(0, 360, 0),
		B = 0,
		Alpha = PV_KeyFrames(0, 260, 100, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Phase = PV_Random(0, 1)
	};
	
	ExplosionParticles_BlastSmooth =
	{
		Size = PV_KeyFrames(0, 0, 0, 250, PV_Random(30, 50), 1000, 80),
		R = PV_KeyFrames(0, 0, 255, 250, 128, 1000, 0),
		G = PV_KeyFrames(0, 0, 255, 125, 64, 1000, 0),
		Rotation = PV_Random(0, 360, 0),
		B = PV_KeyFrames(0, 0, 100, 250, 64, 100, 0),
		Alpha = PV_KeyFrames(0, 0, 255, 750, 250, 1000, 0),
		BlitMode = GFX_BLIT_Additive
	};
	
	ExplosionParticles_BlastSmoothBackground =
	{
		Prototype = ExplosionParticles_BlastSmooth,
		BlitMode = nil,
		R = PV_Linear(50, 0),
		G = PV_Linear(50, 0),
		B = PV_Linear(50, 0),
		Alpha = PV_Linear(128, 0)
	};
	
	ExplosionParticles_Star =
    {
        Size = PV_KeyFrames(0, 0, 0, 500, 60, 1000, 0),
        R = PV_KeyFrames(0, 750, 255, 1000, 0),
        G = PV_KeyFrames(0, 300, 255, 1000, 0),
        B = PV_KeyFrames(0, 300, 255, 500, 0),
		Rotation = PV_Random(0, 360, 4),
        Alpha = PV_KeyFrames(0, 750, 255, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Stretch = PV_Speed(1000, 1000)
    };
    
	ExplosionParticles_Shockwave =
    {
        Size = PV_Linear(0, 120),
        R = 255,
        G = 128,
        B = PV_KeyFrames(0, 0, 128, 200, 0),
		Rotation = PV_Step(20),
		BlitMode = GFX_BLIT_Additive,
        Alpha = PV_Linear(255, 0)
    };
    
    ExplosionParticles_Glimmer=
	{
		Size = PV_Linear(2, 0),
	    ForceY = GetGravity(),
		DampingY = PV_Linear(1000,700),
		DampingX = PV_Linear(1000,700),
		Stretch = PV_Speed(1000, 500),
		Rotation = PV_Direction(),
		OnCollision = PC_Die(),
		CollisionVertex = 500,
	    R = 255,
	    G = PV_Linear(128,32),
	    B = PV_Random(0, 128, 2),
	    Alpha = PV_Random(255,0,3),
		BlitMode = GFX_BLIT_Additive,
	};
}

/*-- Explosion --*/

// documented in /docs/sdk/script/fn
global func Explode(int level, bool silent, int damage_level)
{
	if(!this) FatalError("Function Explode must be called from object context");
	
	// Special: Implode
	if (level <= 0) return RemoveObject();

	// Shake the viewport.
	ShakeViewport(level);
	
	// Explosion parameters.
	var x = GetX(), y = GetY();
	var cause_plr = GetController();
	var container = Contained();
	var exploding_id = GetID();
	var layer = GetObjectLayer();

	// Explosion parameters saved: Remove object now, since it should not be involved in the explosion.
	RemoveObject();

	// Execute the explosion in global context.
	// There is no possibility to interact with the global context, apart from GameCall.
	// So at least remove the object context.
	exploding_id->DoExplosion(x, y, level, container, cause_plr, layer, silent, damage_level);
}

global func DoExplosion(int x, int y, int level, object inobj, int cause_plr, object layer, bool silent, int damage_level)
{
	// Zero-size explosion doesn't affect anything
	if (level <= 0) return;
	// If the object is contained, loop through all parent containers until the blast is contained or no container is found.
	// Blast the containers it passes through and the objects inside them.
	var container = inobj;
	var prev_container = nil;
	while (container)
	{
		// Determine first whether the container has a parent and if it contains the blast.
		// This is needed because the container might be exploded 
		var contains_blast = container.ContainBlast;
		var parent_container = container->Contained();
		// Blast the current container, but not the previous container.
		BlastObjects(x + GetX(), y + GetY(), level, container, cause_plr, damage_level, layer, prev_container);
		// Break the blasting if this container contains the blast.
		if (contains_blast)
			break;
		// Move one container up if possible.
		prev_container = container;
		container = parent_container;
	}
	
	// Explosion outside: Explosion effects.
	if (!container)
	{
		// Blast objects outside if there was no final container containing the blast.
		BlastObjects(x + GetX(), y + GetY(), level, container, cause_plr, damage_level, layer, prev_container);
		// Incinerate oil.
		if (!IncinerateLandscape(x, y, cause_plr))
			if (!IncinerateLandscape(x, y - 10, cause_plr))
				if (!IncinerateLandscape(x - 5, y - 5, cause_plr))
					IncinerateLandscape(x + 5, y - 5, cause_plr);
		// Graphic effects.
		Call("ExplosionEffect", level, x, y, 0, silent, damage_level);
		// Landscape destruction. Happens after BlastObjects, so that recently blown-free materials are not affected
		BlastFree(x, y, level, cause_plr);
	}

	return true;
}

/*
Creates a visual explosion effect at a position.
smoothness (in percent) determines how round the effect will look like.

Defined in Objects.ocd/System.ocg because it depends on particles/definitions to be loaded.
*/
global func ExplosionEffect(...)
{
	return _inherited(...);
}

/*-- Blast objects & shockwaves --*/

// Damage and hurl objects away.
// documented in /docs/sdk/script/fn
global func BlastObjects(int x, int y, int level, object container, int cause_plr, int damage_level, object layer, object prev_container)
{
	var obj;
	
	// Coordinates are always supplied globally, convert to local coordinates.
	var l_x = x - GetX(), l_y = y - GetY();
	
	// caused by: if not specified, controller of calling object
	if (cause_plr == nil)
		if (this)
			cause_plr = GetController();
	
	// damage: if not specified this is the same as the explosion radius
	if (damage_level == nil)
		damage_level = level;
	
	// In a container?
	if (container)
	{
		if (container->GetObjectLayer() == layer)
		{
			container->BlastObject(damage_level, cause_plr);
			if (!container)
				return true; // Container could be removed in the meanwhile.
			for (obj in FindObjects(Find_Container(container), Find_Layer(layer), Find_Exclude(prev_container)))
				if (obj)
					obj->BlastObject(damage_level, cause_plr);
		}
	}
	else
	{
		// Object is outside.
		var at_rect = Find_AtRect(l_x - 5, l_y - 5, 10, 10);
		// Damage objects at point of explosion.
		for (var obj in FindObjects(at_rect, Find_NoContainer(), Find_Layer(layer), Find_Exclude(prev_container)))
			if (obj) obj->BlastObject(damage_level, cause_plr);
			
		// Damage objects in radius.
		for (var obj in FindObjects(Find_Distance(level, l_x, l_y), Find_Not(at_rect), Find_NoContainer(), Find_Layer(layer), Find_Exclude(prev_container)))
			if (obj) obj->BlastObject(damage_level / 2, cause_plr);

		
		// Perform the shockwave at the location where the top level container previously was.
		// This ensures reliable flint jumps for explosives that explode inside a crew member.
		var off_x = 0, off_y = 0;
		if (prev_container)
		{	
			var max_offset = 300;
			off_x = BoundBy(-prev_container->GetXDir(100), -max_offset, max_offset);
			off_y = BoundBy(-prev_container->GetYDir(100), -max_offset, max_offset);			
		}
		DoShockwave(x, y, level, cause_plr, layer, off_x, off_y);
	}
	// Done.
	return true;
}

global func BlastObject(int level, int caused_by)
{
	var self = this;
	if (caused_by == nil)
		caused_by = GetController();

	DoDamage(level, FX_Call_DmgBlast, caused_by);
	if (!self) return;

	if (GetAlive())
		DoEnergy(-level, false, FX_Call_EngBlast, caused_by);
	if (!self) return;

	if (this.BlastIncinerate && GetDamage() >= this.BlastIncinerate)
		Incinerate(level, caused_by);
	return;
}

// documented in /docs/sdk/script/fn
global func DoShockwave(int x, int y, int level, int cause_plr, object layer, int off_x, int off_y)
{
	// Zero-size shockwave
	if (level <= 0) return;
	
	// Coordinates are always supplied globally, convert to local coordinates.
	var l_x = x - GetX(), l_y = y - GetY();
	
	// caused by: if not specified, controller of calling object
	if (cause_plr == nil)
		if (this)
			cause_plr = GetController();

	// Hurl objects in explosion radius.
	var shockwave_objs = FindObjects(Find_Distance(level, l_x, l_y), Find_NoContainer(), Find_Layer(layer),
		Find_Or(Find_Category(C4D_Object|C4D_Living|C4D_Vehicle), Find_Func("CanBeHitByShockwaves", cause_plr)), Find_Func("DoShockwaveCheck", x, y, cause_plr));
	var cnt = GetLength(shockwave_objs);
	if (cnt)
	{
		// The hurl energy is distributed over the objects.
		var shock_speed = Sqrt(2 * level * level / BoundBy(cnt, 2, 12));
		for (var obj in shockwave_objs)
		{
			if (obj) // Test obj, cause OnShockwaveHit could have removed objects.
			{
				var cat = obj->GetCategory();
				// Object has special reaction on shockwave?
				if (obj->~OnShockwaveHit(level, x, y, cause_plr))
					continue;
				// Killtracing for projectiles.
				if (cat & C4D_Object)
					obj->SetController(cause_plr);
				// Shockwave.
				var mass_fact = 20, mass_mul = 100;
				if (cat & C4D_Living)
				{
					mass_fact = 8;
					mass_mul = 80;
				}
				mass_fact = BoundBy(obj->GetMass() * mass_mul / 1000, 4, mass_fact);
				// Determine difference between object and explosion center, take into account the offset.
				var dx = 100 * (obj->GetX() - x) - off_x + Random(51) - 25;
				var dy = 100 * (obj->GetY() - y) - off_y + Random(51) - 25;
				var vx, vy;
				if (dx)
					vx = Abs(dx) / dx * (100 * level - Abs(dx)) * shock_speed / level / mass_fact;
				vy = (Abs(dy) - 100 * level) * shock_speed / level / mass_fact;
				if (cat & C4D_Object)
				{
					// Objects shouldn't move too fast.
					var ovx = obj->GetXDir(100), ovy = obj->GetYDir(100);
					if (ovx * vx > 0)
						vx = (Sqrt(vx * vx + ovx * ovx) - Abs(vx)) * Abs(vx) / vx;
					if (ovy * vy > 0)
						vy = (Sqrt(vy * vy + ovy * ovy) - Abs(vy)) * Abs(vy) / vy;
				}
				obj->Fling(vx, vy, 100, true);
			}
		}
	}
	return;
}

global func DoShockwaveCheck(int x, int y, int cause_plr)
{
	var def = GetID();
	// Some special cases, which won't go into FindObjects.
	if (def->GetDefHorizontalFix())
		return false;
	// Only move vehicles and floating objects which can be grabbed and pushed (Touchable = 1).	
	if (this.Touchable != 1)
	{
		if (GetCategory() & C4D_Vehicle)
			return false;
		if (GetProcedure() == "FLOAT")
			return false;
	}
	// Projectiles not when they fly downwards or are exactly in the explosion point.
	// This will catch the most cases in which multiple clonks throw flints at the same time.
	if (GetCategory() & C4D_Object)
	{
		if (GetX() == x && GetY() == y) return false;
		if (GetYDir() > 5) return false;
	}
	// No stuck objects.
	if (Stuck())
		return false;
	return true;
}


/** Shake the player viewports near the given position. This disorienting effect is used for earthquakes, explosions
and other rumbling effects. The further away the player is from the source, the less his viewport is shaken. The 
strength falls off linearly by distance from 100% to 0% when the player is 700 pixels away.

@param level strength of the shake in pixels. As a point of reference, for explosions, the shake strength is the same
             as the explosion level.
@param x_off x offset in relative coordinates from the calling object
@param y_off y offset in relative coordinates from the calling object
@param range range of clonk to explosion at which shaking falls off to 0%
*/
// documented in /docs/sdk/script/fn
global func ShakeViewport(int level, int x_off, int y_off, range)
{
	if (level <= 0)
		return false;
		
	x_off += GetX();
	y_off += GetY();
	
	AddEffect("ShakeViewport", nil, 300, 1, nil, nil, level, x_off, y_off, range ?? 700);
}

global func FxShakeViewportEffect(string new_name)
{
	// there is only one global ShakeViewport effect which manages all the shake positions and strengths
	if (new_name == "ShakeViewport")
		return -2;
	return;
}

global func FxShakeViewportStart(object target, effect e, int temporary, level, xpos, ypos, range)
{
	if(temporary != 0) return;
	
	e.shakers = CreateArray();
	e.shakers[0] = { x = xpos, y = ypos, strength = level, time = 0, range = range };
}

global func FxShakeViewportAdd(object target, effect e, string new_name, int new_timer, level, xpos, ypos, range)
{
	e.shakers[GetLength(e.shakers)] = { x = xpos, y = ypos, strength = level, time = e.Time, range = range };
}

global func FxShakeViewportTimer(object target, effect e, int time)
{
	var maxShakeStrength = 500;

	// shake for all players
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		var cursor = GetCursor(plr);
		if (!cursor)
			continue;

		var totalShakeStrength = 0;
		for(var shakerIndex = 0; shakerIndex < GetLength(e.shakers); ++shakerIndex)
		{
			// ignore shakers if the offset is too high already
			if (totalShakeStrength > maxShakeStrength)
			{
				continue;
			}
			var shaker = e.shakers[shakerIndex];
			var shakerTime = time - shaker.time;

			// shake strength lowers as a function of the distance
			var distance = Distance(cursor->GetX(), cursor->GetY(), shaker.x, shaker.y);
			var maxDistance = shaker.range;
			var level = shaker.strength * BoundBy(100-100*distance/maxDistance,0,100)/100;

			// calculate total shake strength by adding up all shake positions in the player's vicinity
			totalShakeStrength += level / Max(1,shakerTime*2/3) - shakerTime**2 / 400;
		}
		// limit the strength, just to be sure
		totalShakeStrength = Min(maxShakeStrength, totalShakeStrength);
		SetViewOffset(plr, Sin(time * 100, totalShakeStrength), Cos(time * 100, totalShakeStrength));
	}

	// remove shakers that are done shaking
	for(var shakerIndex = 0; shakerIndex < GetLength(e.shakers); ++shakerIndex)
	{
		var shaker = e.shakers[shakerIndex];
		var shakerTime = time - shaker.time;
		if (shaker.strength / Max(1,shakerTime*2/3) - shakerTime**2 / 400 <= 0)
			e.shakers[shakerIndex] = nil;
	}
	RemoveHoles(e.shakers);
	
	// no shakers left: remove this effect
	if(GetLength(e.shakers) == 0)
	{
		return -1;
	}
}

global func FxShakeViewportStop()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		SetViewOffset(GetPlayerByIndex(i), 0, 0);
	}
}

/*-- Smoke trails --*/

global func CreateSmokeTrail(int strength, int angle, int x, int y, int color, bool noblast)
{
	var e = AddEffect("SmokeTrail", nil, 300, 1, nil, nil, color);
	e.x = 100*(GetX() + x);
	e.y = 100*(GetY() + y);
	e.strength = strength;
	e.curr_strength = strength;
	e.noblast = noblast;
	e.angle = angle;
}

global func FxSmokeTrailStart(object target, proplist e, int temp, int color)
{
	if (temp)
		return;

	e.color = color ?? RGBa(255, 128, 0, 200);
	var alpha = (e.color >> 24) & 0xff;
	e.particles_smoke =
	{
		R = PV_Linear(255, 64),
		G = PV_Linear(255, 64),
		B = PV_Linear(255, 64),
		Alpha = PV_KeyFrames(0, 0, alpha/4, 200, alpha, 1000, 0),
		Rotation = PV_Random(-45,45),
		ForceX = PV_Wind(20),
		ForceY = PV_Gravity(-10),
	};
	
	e.particles_blast =
	{
		R = PV_Linear((e.color >> 16) & 0xff, 0),
		G = PV_Linear((e.color >>  8) & 0xff, 0),
		B = PV_Linear((e.color >>  0) & 0xff, 0),
		Alpha = PV_KeyFrames(0, 0, alpha, 500, 3*alpha/4, 1000, 0),
		Rotation = PV_Direction(),
		BlitMode = GFX_BLIT_Additive,
		Stretch = PV_Speed(1500, 1000)
	};
}

global func FxSmokeTrailTimer(object target, effect e, int fxtime)
{
	var strength = e.strength;
	e.curr_strength = e.curr_strength * 94 / 100;
	
	var str = e.curr_strength;
	
	var initial_speed = 100 * (strength+20)/6;
	var speed = initial_speed * str / strength;
	var angle = e.angle + RandomX(-20,20);
	var x_dir = Sin(angle, speed);
	var y_dir = -Cos(angle , speed);
	
	if (speed < 2*100) return -1;
	
	// gravity
	y_dir += GetGravity() * 15;
	
	// draw
	e.particles_smoke.Size = PV_KeyFrames(0, 0, str / 2, 1000, str);
	e.particles_blast.Size = PV_KeyFrames(0, 0, 0, 200, str / 3, 1000, 0);

	CreateParticle("SmokeThick", e.x/100, e.y/100, RandomX(-1,1), RandomX(-1,1), 50, e.particles_smoke,1);
		
	// then calc next position
	e.x += x_dir;
	e.y += y_dir;
	
	if (!e.noblast)
	{
		var x_dir_blast = x_dir / 200;
		var y_dir_blast = y_dir / 200;
		CreateParticle("SmokeDirty", e.x/100, e.y/100, PV_Random(x_dir_blast - 2, x_dir_blast + 2), PV_Random(y_dir_blast - 2, y_dir_blast + 2), 10, e.particles_blast, 1);
	}
	
	
	if (GBackSemiSolid(e.x/100, e.y/100))
		return -1;
	
	e.curr_strength = str;
}

/*-- Fireworks --*/

global func Fireworks(int color, int x, int y)
{
	if (!color)
		color = HSL(Random(8) * 32, 255, 160);
	
	var flash =
	{
		Prototype = Particles_Flash(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		Size = PV_KeyFrames(0, 0, 0, 100, 200, 1000, 0),
	};
	
	var glimmer = 
	{
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		DampingY = PV_Linear(950,800),
		DampingX = PV_Linear(950,800),
		Stretch = PV_Speed(3000, 500),
		Size = PV_Linear(2, 1),
		ForceY = PV_Gravity(50),
		Rotation = PV_Direction(),
		OnCollision = PC_Die(),
		CollisionVertex = 500,
		Alpha = PV_Random(255,0,3),
		BlitMode = GFX_BLIT_Additive,
	};
	
	var start_angle = Random(360);
	
	var num = 25;
	for(var i=0; i<num; ++i)
	{
		for(var j=0; j<num; ++j)
		{
			var speed = 1000;
			var angle = start_angle + i*360/num + Random(15) + j*15;
			var speed3d = Cos(j*90/num - Random(15),speed);
			var xdir = Sin(angle, speed3d/10);
			var ydir = -Cos(angle,speed3d/10);
		
			CreateParticle("MagicFire", x, y, xdir, ydir, PV_Random(50, 200), glimmer, 1);
		}
	}
	
	CreateParticle("Flash", x, y, 0, 0, 20, flash);
}


