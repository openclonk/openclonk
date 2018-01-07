/*
	Contains functions for (visual) effects that require particles or definitions from Objects.ocd to be loaded.
*/

/*
Creates a visual explosion effect at a position.
smoothness (in percent) determines how round the effect will look like
*/
global func ExplosionEffect(int level, int x, int y, int smoothness, bool silent, int damage_level)
{
	if (g_theme != MiamiIce) return inherited(level, x, y, smoothness, silent, damage_level, ...);

	// Zero-size explosion doesn't affect anything
	if (level <= 0) return;
	
	if(!silent) //Does object use it's own explosion sound effect?
	{
		// Select sound according to level: from 1 to 3, add the * to allow alternatives.
		var grade = BoundBy(level / 10 - 1, 1, 3);
		if(GBackLiquid(x, y))
			SoundAt(Format("Fire::BlastLiquid%d*",grade), x, y);
		else
			SoundAt(Format("Fire::Blast%d*", grade), x, y);
	}
	
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
	
	var clr = HSL(Random(255), 255, 100);
	
	CreateParticle("SmokeDirty", PV_Random(x - 10,x + 10), PV_Random(y - 10, y + 10), 0, PV_Random(-2, 0), PV_Random(50, 100), {Prototype = Particles_Colored(ExplosionParticles_Smoke, clr), Size = smoke_size}, Max(2, wilderness_level / 10));
	CreateParticle("SmokeDirty", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(20, 40), {Prototype = Particles_Colored(ExplosionParticles_BlastSmoothBackground, clr), Size = blast_smooth_size}, smoothness_level / 5);
	CreateParticle("SmokeDirty", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(20, 40), {Prototype = Particles_Colored(ExplosionParticles_BlastSmooth, clr), Size = blast_smooth_size}, smoothness_level / 5);
	CreateParticle("Dust", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), 0, 0, PV_Random(18, 25), {Prototype = Particles_Colored(ExplosionParticles_Blast, clr), Size = blast_size}, smoothness_level / 5);
	CreateParticle("StarFlash", PV_Random(x - 6, x + 6), PV_Random(y - 6, y + 6), PV_Random(-wilderness_level/4, wilderness_level/4), PV_Random(-wilderness_level/4, wilderness_level/4), PV_Random(10, 12), {Prototype = Particles_Colored(ExplosionParticles_Star, clr), Size = star_size}, wilderness_level / 3);
	CreateParticle("Shockwave", x, y, 0, 0, 15, {Prototype = Particles_Colored(ExplosionParticles_Shockwave, clr), Size = shockwave_size}, nil);
	
	// cast either some sparks on land or bubbles under water
	if(GBackLiquid(x, y) && Global.CastBubbles)
	{
		Global->CastBubbles(level * 7 / 10, level, x, y);
	}
	else
	{
		CreateParticle("Magic", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-level_pow_fraction, level_pow_fraction), PV_Random(-level_pow_fraction, level_pow_fraction), PV_Random(25, 70), Particles_Colored(ExplosionParticles_Glimmer, clr), level);
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
		CreateSmokeTrail(lvl, angle, x + smokex, y + smokey, clr);
		smoke_trail_count--;
	}
	
	// Temporary light effect
	if (level > 5)
		Global->CreateLight(x, y, level, Fx_Light.LGT_Blast);

	return;
}

global func FxSmokeTrailStart(object target, proplist e, int temp, int color)
{
	if (g_theme != MiamiIce) return inherited(target, e, temp, color, ...);

	if (temp)
		return;

	e.color = color ?? RGBa(255, 128, 0, 200);
	
	var c = SplitRGBaValue(color);
	var alpha = (e.color >> 24) & 0xff;
	e.particles_smoke =
	{
		R = c.R,
		G = c.G,
		B = c.B,
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
