/*--
		Explode.c
		Authors: Newton
		
		Everything about the explosion.
		TODO: documentation.
--*/

/*--
Particle definitions used by the explosion effect.
They will be initialized lazily whenever the first blast goes off.
--*/
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

global func Explode(int level, bool silent)
{
	if(!this) FatalError("Function Explode must be called from object context");

	// Shake the viewport.
	ShakeViewPort(level, GetX(), GetY());

	// Sound must be created before object removal, for it to be played at the right position.
	if(!silent) //Does object use it's own explosion sound effect?
	{
		var grade = BoundBy(level / 10 - 1, 1, 3);
		if(GBackLiquid())
			Sound(Format("BlastLiquid%d",grade));
		else
			Sound(Format("Blast%d", grade));
	}

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
	exploding_id->DoExplosion(x, y, level, container, cause_plr, layer);
}

global func DoExplosion(int x, int y, int level, object inobj, int cause_plr, object layer)
{
	// Container to ContainBlast
	var container = inobj;
	while (container)
	{
		if (container->GetID()->GetDefContainBlast())
			break;
		else
			container = container->Contained();
	}

	// Explosion outside: Explosion effects.
	if (!container)
	{
		// Incinerate oil.
		if (!IncinerateLandscape(x, y))
			if (!IncinerateLandscape(x, y - 10))
				if (!IncinerateLandscape(x - 5, y - 5))
					IncinerateLandscape(x + 5, y - 5);
		// Graphic effects.
		ExplosionEffect(level, x, y);
	}
	// Damage in the objects, and outside.
	BlastObjects(x + GetX(), y + GetY(), level, inobj, cause_plr, layer);
	if (inobj != container)
		BlastObjects(x + GetX(), y + GetY(), level, container, cause_plr, layer);
	
	// Landscape destruction. Happens after BlastObjects, so that recently blown-free materials are not affected
	if (!container)
		BlastFree(x, y, level, cause_plr);

	return true;
}

/*
Creates a visual explosion effect at a position.
smoothness (in percent) determines how round the effect will look like
*/
global func ExplosionEffect(int level, int x, int y, int smoothness)
{
	// possibly init particle definitions?
	if (!ExplosionParticles_Blast)
		ExplosionParticles_Init();
	
	smoothness = smoothness ?? 0;
	var level_pow = level ** 2;
	var level_pow_fraction = Max(level_pow / 25, 5 * level);
	var wilderness_level = level * (100 - smoothness) / 100;
	var smoothness_level = level * smoothness / 100;
	
	var smoke_size = PV_KeyFrames(0, 180, level, 1000, level * 2);
	var blast_size = PV_KeyFrames(0, 0, 0, 260, level * 2, 1000, level);
	var blast_smooth_size = PV_KeyFrames(0, 0, 0, 250, PV_Random(level, level * 2), 1000, level);
	var star_size = PV_KeyFrames(0, 0, 0, 500, level * 2, 1000, 0);
	var shockwave_size = PV_Linear(0, level * 4);
	
	CreateParticle("SmokeDirty", PV_Random(x - 10,x + 10), PV_Random(y - 10, y + 10), 0, PV_Random(-2, 0), PV_Random(50, 100), {Prototype = ExplosionParticles_Smoke, Size = smoke_size}, Max(2, wilderness_level / 10));
	CreateParticle("SmokeDirty", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(20, 40), {Prototype = ExplosionParticles_BlastSmoothBackground, Size = blast_smooth_size}, smoothness_level / 5);
	CreateParticle("SmokeDirty", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(20, 40), {Prototype = ExplosionParticles_BlastSmooth, Size = blast_smooth_size}, smoothness_level / 5);
	CreateParticle("Dust", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), 0, 0, PV_Random(18, 25), {Prototype = ExplosionParticles_Blast, Size = blast_size}, smoothness_level / 5);
	CreateParticle("StarFlash", PV_Random(x - 6, x + 6), PV_Random(y - 6, y + 6), PV_Random(-wilderness_level/4, wilderness_level/4), PV_Random(-wilderness_level/4, wilderness_level/4), PV_Random(10, 12), {Prototype = ExplosionParticles_Star, Size = star_size}, wilderness_level / 3);
	CreateParticle("Shockwave", x, y, 0, 0, 15, {Prototype = ExplosionParticles_Shockwave, Size = shockwave_size}, nil);
	
	// cast either some sparks on land or bubbles under water
	if(GBackLiquid(x, y) && Global.CastBubbles)
	{
		Global->CastBubbles(level * 7 / 10, level, x, y);
	}
	else
	{
		CreateParticle("Magic", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-level_pow_fraction, level_pow_fraction), PV_Random(-level_pow_fraction, level_pow_fraction), PV_Random(25, 70), ExplosionParticles_Glimmer, level);
	}
	
	// very wild explosion? Smoke trails!
	var smoke_trail_count = wilderness_level / 10;
	var angle  = Random(360);
	var failsafe = 0; // against infinite loops
	while (smoke_trail_count > 0 && (++failsafe < smoke_trail_count * 10))
	{
		angle += RandomX(40, 80);
		var smokex = Sin(angle, RandomX(level / 4, level / 2));
		var smokey = -Cos(angle, RandomX(level / 4, level / 2));
		if (GBackSolid(x + smokex, y + smokey))
			continue;
		var lvl = 2 * wilderness_level;
		CreateSmokeTrail(lvl, angle, x + smokex, y + smokey);
		smoke_trail_count--;
	}

	return;
}

/*-- Blast objects & shockwaves --*/

// Damage and hurl objects away.
global func BlastObjects(int x, int y, int level, object container, int cause_plr, object layer)
{
	var obj;
	
	// Coordinates are always supplied globally, convert to local coordinates.
	var l_x = x - GetX(), l_y = y - GetY();
	
	// caused by: if not specified, controller of calling object
	if(cause_plr == nil)
		if(this)
			cause_plr = GetController();
	
	// In a container?
	if (container)
	{
		if (container->GetObjectLayer() == layer)
		{
			container->BlastObject(level, cause_plr);
			if (!container)
				return true; // Container could be removed in the meanwhile.
			for (obj in FindObjects(Find_Container(container), Find_Layer(layer)))
				if (obj)
					obj->BlastObject(level, cause_plr);
		}
	}
	else
	{
		// Object is outside.
		// Damage objects at point of explosion.
		for (var obj in FindObjects(Find_AtRect(l_x - 5, l_y - 5, 10,10), Find_NoContainer(), Find_Layer(layer)))
			if (obj) obj->BlastObject(level, cause_plr);

		// TODO: -> Shockwave in own global func(?)

		// Hurl objects in explosion radius.
		var shockwave_objs = FindObjects(Find_Distance(level, l_x, l_y), Find_NoContainer(), Find_Layer(layer),
			Find_Or(Find_Category(C4D_Object|C4D_Living|C4D_Vehicle), Find_Func("CanBeHitByShockwaves")), Find_Func("BlastObjectsShockwaveCheck", x, y));
		var cnt = GetLength(shockwave_objs);
		if (cnt)
		{
			// The hurl energy is distributed over the objects.
			//Log("Shockwave objs %v (%d)", shockwave_objs, cnt);
			var shock_speed = Sqrt(2 * level * level / BoundBy(cnt, 2, 12));
			for (var obj in shockwave_objs)
				if (obj) // Test obj, cause OnShockwaveHit could have removed objects.
				{
					// Object has special reaction on shockwave?
					if (obj->~OnShockwaveHit(level, x, y))
						continue;
					// Living beings are hurt more.
					var cat = obj->GetCategory();
					if (cat & C4D_Living)
					{
						obj->DoEnergy(level / -2, false, FX_Call_EngBlast, cause_plr);
						obj->DoDamage(level / 2, FX_Call_DmgBlast, cause_plr);
					}
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
					var dx = 100 * (obj->GetX() - x) + Random(51) - 25;
					var dy = 100 * (obj->GetY() - y) + Random(51) - 25;
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
					//Log("%v v(%v %v)   d(%v %v)  m=%v  l=%v  s=%v", obj, vx,vy, dx,dy, mass_fact, level, shock_speed);
					obj->Fling(vx, vy, 100, true);
				}
		}
	}
	// Done.
	return true;
}

global func BlastObject(int Level, CausedBy)
{
	var self = this;
	if (CausedBy == nil)
		CausedBy = GetController();

	DoDamage(Level, FX_Call_DmgBlast, CausedBy);
	if (!self) return;

	if (GetAlive())
		DoEnergy(-Level/3, false, FX_Call_EngBlast, CausedBy);
	if (!self) return;

	if (this.BlastIncinerate && GetDamage() >= this.BlastIncinerate)
		Incinerate(Level, CausedBy);
}

global func BlastObjectsShockwaveCheck(int x, int y)
{
	var def = GetID();
	// Some special cases, which won't go into FindObjects.
	if (def->GetDefHorizontalFix())
		return false;
	if (def->GetDefGrab() != 1)
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


/*-- Shake view port --*/

global func ShakeViewPort(int level, int x_off, int y_off)
{
	if (level <= 0)
		return false;

	var eff = GetEffect("ShakeEffect", this);

	if (eff)
	{
		eff.level += level;
		return true;
	}

	eff = AddEffect("ShakeEffect", this, 200, 1);
	if (!eff)
		return false;

	eff.level = level;

	if (x_off || y_off)
	{
		eff.x = x_off;
		eff.y = y_off;
	}
	else
	{
		eff.x = GetX();
		eff.y = GetY();
	}
	return true;
}

// Duration of the effect: as soon as strength==0
// Strength of the effect: strength=level/(1.5*fxtime+3)-fxtime^2/400

global func FxShakeEffectTimer(object target, effect, int fxtime)
{
	var strength;

	var str = effect.level;
	var xpos = effect.x;
	var ypos = effect.y;


	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		var cursor = GetCursor(plr);
		if (!cursor)
			continue;
		var distance = Distance(cursor->GetX(), cursor->GetY(), xpos, ypos);

		// Shake effect lowers as a function of the distance.
		var level = (300 * str) / Max(300, distance);

		if ((strength = level / ((3 * fxtime) / 2 + 3) - fxtime**2 / 400) <= 0)
			continue;

		// FixME: Use GetViewOffset, make this relative, not absolute
		SetViewOffset(plr, Sin(fxtime * 100, strength), Cos(fxtime * 100, strength));
	}

	if (str / ((3 * fxtime) / 2 + 3) - fxtime**2 / 400 <= 0)
		return -1;
}

global func FxShakeEffectStart(object target, effect)
{
	FxShakeEffectTimer(target, effect, effect.Time);
}

global func FxShakeEffectStop()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		// FxME: Return the offset to the previous value, not zero
		SetViewOffset(GetPlayerByIndex(i), 0, 0);
	}
}

/*-- Smoke trails --*/

global func CreateSmokeTrail(int strength, int angle, int x, int y, int color, bool noblast)
{
	x += GetX();
	y += GetY();
	if (angle % 90 == 1) angle = 1;
	
	var effect = AddEffect("SmokeTrail", nil, 300, 1, nil, nil, color);
	EffectCall(nil, effect, "SetAdditionalParameters", x, y, angle, strength, noblast);
	return;
}

global func FxSmokeTrailSetAdditionalParameters(object target, proplist effect, int x, int y, int angle, int strength, bool noblast)
{
	effect.x = x;
	effect.y = y;
	effect.strength = strength;
	effect.curr_strength = strength;
	effect.xdir = Sin(angle, strength * 40);
	effect.ydir = -Cos(angle, strength * 40);
	effect.noblast = noblast;
}

global func FxSmokeTrailStart(object target, proplist effect, int temp, color)
{
	if (temp)
		return;

	effect.color = color ?? RGBa(255, 128, 0, 200);
	var alpha = (effect.color >> 24) & 0xff;
	effect.particles_smoke =
	{
		R = PV_KeyFrames(0, 0, 250, 400, 200, 1000, 100),
		G = PV_KeyFrames(0, 0, 250, 400, 200, 1000, 100),
		B = PV_KeyFrames(0, 0, 250, 400, 200, 1000, 100),
		Alpha = PV_KeyFrames(0, 0, 0, 300, alpha, 600, (alpha * 4) / 5, 1000, 0),
		Rotation = PV_Random(0, 360),
		ForceX = PV_Wind(20),
		ForceY = PV_Gravity(-10),
	};
	
	effect.particles_blast =
	{
		R = PV_Linear((effect.color >> 16) & 0xff, 0),
		G = PV_Linear((effect.color >>  8) & 0xff, 0),
		B = PV_Linear((effect.color >>  0) & 0xff, 0),
		Alpha = PV_KeyFrames(0, 0, alpha, 600, (alpha * 4) / 5, 1000, 0),
		Rotation = PV_Direction(),
		BlitMode = GFX_BLIT_Additive,
		Stretch = PV_Speed(1500, 1000)
	};
}

global func FxSmokeTrailTimer(object target, proplist effect, int fxtime)
{
	var strength = effect.strength;
	effect.curr_strength = (effect.curr_strength * 5) / 6;
	if (effect.curr_strength < 5) return -1;
	
	var str = effect.curr_strength;
	var x = effect.x;
	var y = effect.y;
	
	var x_dir = effect.xdir;
	var y_dir = effect.ydir;

	y_dir += GetGravity() * 10 / 3;

	var x_dir = x_dir * str / strength;
	var y_dir = y_dir * str / strength;

	// new: random
	x += RandomX(-3,3);
	y += RandomX(-3,3);
	
	// draw
	effect.particles_smoke.Size = PV_KeyFrames(0, 0, 0, 250, str / 2, 1000, str);
	effect.particles_blast.Size = PV_KeyFrames(0, 0, 0, 100, str / 3, 1000, 0);
	if (!effect.noblast)
	{
		var x_dir_blast = x_dir / 200;
		var y_dir_blast = y_dir / 200;
		CreateParticle("SmokeDirty", x, y, PV_Random(x_dir_blast - 2, x_dir_blast + 2), PV_Random(y_dir_blast - 2, y_dir_blast + 2), 18, effect.particles_blast, 2);
	}
	CreateParticle("Smoke", x, y, PV_Random(-2, 2), PV_Random(-2, 2), 50, effect.particles_smoke, 2);

		
	// then calc next position
	x += x_dir / 100;
	y += y_dir / 100;
	
	if (GBackSemiSolid(x, y))
		return -1;
	
	effect.curr_strength = str;
	effect.x = x;
	effect.y = y;
	effect.ydir = y_dir;
}

/*-- Fireworks --*/

global func Fireworks(int color, int x, int y)
{
	if (!color)
		color = HSL(Random(8) * 32, 255, 127);
	
	var glimmer = 
	{
		Prototype = Particles_Glimmer(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
	};
	CreateParticle("MagicFire", x, y, PV_Random(-100, 100), PV_Random(-100, 100), PV_Random(20, 200), glimmer, 100);
	
	Smoke(x, y, 30);
	
	var sparks =
	{
		Prototype = Particles_Spark(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		DampingX = 900, DampingY = 900,
		ForceY = PV_Random(-10, 10),
		ForceX = PV_Random(-10, 10),
		Stretch = PV_Speed(500, 1000)
	};
	CreateParticle("Spark", x, y, PV_Random(-200, 200), PV_Random(-200, 200), PV_Random(20, 60), sparks, 100);
	
	var flash =
	{
		Prototype = Particles_Flash(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
	};
	CreateParticle("Flash", x, y, 0, 0, 8, flash);
	return;
}

