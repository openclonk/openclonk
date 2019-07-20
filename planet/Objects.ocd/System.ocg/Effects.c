/*
	Effect.c
	Contains functions for (visual) effects that require particles or definitions from Objects.ocd to be loaded.
	
	@author
*/

/*
Creates a visual explosion effect at a position.
smoothness (in percent) determines how round the effect will look like
*/
global func ExplosionEffect(int level, int x, int y, int smoothness, bool silent, int damage_level)
{
	// Zero-size explosion doesn't affect anything
	if (level <= 0) return;
	
	if (!silent) //Does object use it's own explosion sound effect?
	{
		// Select sound according to level: from 1 to 3, add the * to allow alternatives.
		var grade = BoundBy(level / 10 - 1, 1, 3);
		if (GBackLiquid(x, y))
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
	
	CreateParticle("SmokeDirty", PV_Random(x - 10, x + 10), PV_Random(y - 10, y + 10), 0, PV_Random(-2, 0), PV_Random(50, 100), {Prototype = ExplosionParticles_Smoke, Size = smoke_size}, Max(2, wilderness_level / 10));
	CreateParticle("SmokeDirty", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(20, 40), {Prototype = ExplosionParticles_BlastSmoothBackground, Size = blast_smooth_size}, smoothness_level / 5);
	CreateParticle("SmokeDirty", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(20, 40), {Prototype = ExplosionParticles_BlastSmooth, Size = blast_smooth_size}, smoothness_level / 5);
	CreateParticle("Dust", PV_Random(x - 5, x + 5), PV_Random(y - 5, y + 5), 0, 0, PV_Random(18, 25), {Prototype = ExplosionParticles_Blast, Size = blast_size}, smoothness_level / 5);
	CreateParticle("StarFlash", PV_Random(x - 6, x + 6), PV_Random(y - 6, y + 6), PV_Random(-wilderness_level/4, wilderness_level/4), PV_Random(-wilderness_level/4, wilderness_level/4), PV_Random(10, 12), {Prototype = ExplosionParticles_Star, Size = star_size}, wilderness_level / 3);
	CreateParticle("Shockwave", x, y, 0, 0, 15, {Prototype = ExplosionParticles_Shockwave, Size = shockwave_size}, nil);
	
	// cast either some sparks on land or bubbles under water
	if (GBackLiquid(x, y) && Global.CastBubbles)
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
	
	// Temporary light effect
	if (level > 5)
		Global->CreateLight(x, y, level, Fx_Light.LGT_Blast);

	return;
}

/**
Creates a raindrop effect upon the the calling object.
@par duration The duration of the effect. Default nil = unlimited.
@par interval The interval in which the raindrops are created. Default 10.
@material The material of which the drops are created. Default "Water".
@strength Specifies how many drops are created in a call.
@return The created effect.
*/
global func AddRainDropEffect(int duration, int interval, string material, int strength, int offset_x, int offset_y)
{
	if (!this || GetType(this) != C4V_C4Object)
		FatalError(Format("AddRainDropEffect needs to be called from object context, not from %v", this));
	return this->CreateEffect(FxRainDrop, 1, interval ?? 10, duration, material, strength, [offset_x, offset_y]);
}

static const FxRainDrop = new Effect
{
	duration = nil,
	material = nil,
	strength = nil,
	offset = [0, 0],
	particle_cache = {},
	
	Construction = func(int duration, string material, int strength, array offset)
	{
		this.duration = duration;
		this.material = material ?? "Water";
		this.strength = strength ?? 1;
		this.offset = offset ?? [0, 0];
	},
	
	Timer = func(int time)
	{
		if (!this.Target || (this.duration != nil && time > this.duration))
			return FX_Execute_Kill;
		
		if (this.Target->GBackSemiSolid(this.offset[0], this.offset[1]))
			return FX_OK;
		
		var con = this.Target->GetCon();
		var wdt = this.Target->GetDefWidth() * con / 500;
		var hgt = this.Target->GetDefHeight() * con / 700;
		var particle_name;
		
		var color = Cloud->GetMaterialColor(this.material);
		
		if (this.particle_cache.color != color)
		{
			this.particle_cache.color = color;
			this.particle_cache.particle = Particles_Rain(color);
			this.particle_cache.particle.Size = 5;
		}
		
		if (this.material == "Lava" || this.material == "DuroLava")
			particle_name = "RaindropLava";
		else
			particle_name = "Raindrop";
		this.Target->CreateParticle(
			particle_name,
			PV_Random(this.offset[0] - wdt, this.offset[0] + wdt),
			PV_Random(this.offset[1] - hgt, this.offset[1] + hgt),
			PV_Random(-5, 5),
			PV_Random(10, 30),
			PV_Random(200, 300),
			this.particle_cache.particle,
			this.strength
		);
		
		this.Target->~OnRainDropCreated(this);
		return FX_OK;
	}
};
