/**
	Particles.c
	This file contains some default particle behavior definitions as well as helper functions.
	
	@author Zapper
*/


/*-- Helper/Effect Functions --*/

global func CreateMuzzleFlash(int x, int y, int angle, int size)
{
	// main muzzle flash
	CreateParticle("MuzzleFlash", x, y, 0, 0, 10, {Prototype = Particles_MuzzleFlash(), Size = size, Rotation = angle});
	// and some additional little sparks
	var xdir = Sin(angle, size * 2);
	var ydir = -Cos(angle, size * 2);
	CreateParticle("StarFlash", x, y, PV_Random(xdir - size, xdir + size), PV_Random(ydir - size, ydir + size), PV_Random(20, 60), Particles_Glimmer(), size);
}

// documented in /docs/sdk/script/fn
global func Smoke(int x, int y, int level, int color, bool heavy)
{
	level = level ?? 10;
	var particles = Particles_Smoke(heavy);
	if (color)
	{
		particles.Alpha = PV_Linear((color >> 24) & 0xff, 0);
		particles.R = (color >> 16) & 0xff;
		particles.G = (color >>  8) & 0xff;
		particles.B = (color >>  0) & 0xff;
	}
	particles.Size = PV_Linear(PV_Random(level/2, level), PV_Random(2 * level, 3 * level));
	CreateParticle("Smoke", x, y, PV_Random(-level/3, level/3), PV_Random(-level/2, -level/3), PV_Random(level * 2, level * 10), particles, BoundBy(level/5, 3, 20));
}


/*-- Particle Definitions --*/

global func Particles_Dust()
{
	return
	{
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceX = PV_Wind(20),
		ForceY = PV_Gravity(25),
		Alpha = PV_KeyFrames(0, 0, 0, 250, 60, 1000, 0),
		Rotation = PV_Random(0, 360),
		Size = PV_KeyFrames(0, 0, 5, 100, 12, 1000, 7)
	};
}

global func Particles_Cloud()
{
	return
	{
		Size = 200,
		Attach = ATTACH_MoveRelative,
		Phase = PV_Random(0, 15)
	};
}

global func Particles_Smoke(bool heavy)
{
	return
	{
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceY = PV_Gravity(-100 + heavy*90),
		ForceX = PV_Wind(200 - heavy*180),
		DampingX = 900, DampingY = 900,
		Alpha = PV_Linear(255, 0),
		R = 100, G = 100, B = 100,
		Size = PV_Linear(PV_Random(4, 10), PV_Random(20, 30)),
		Phase = PV_Random(0, 15)
	};
}

global func Particles_Fire()
{
	return
	{
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		Phase = PV_Random(0, 3, 10),
		ForceY = PV_Gravity(-100),
		DampingY = 950,
		Alpha = PV_KeyFrames(0, 0, 255, 500, 255, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Size = PV_KeyFrames(0, 0, PV_Random(5, 10), 500, 5, 1000, 0),
		Attach = ATTACH_Front,
		Rotation = PV_Direction()
	};
}

global func Particles_FireTrail()
{
	return
	{
		Prototype = Particles_Fire(),
		ForceY = 0,
		Attach = nil,
	};
}

global func Particles_Flash(int size)
{
	return
	{
		BlitMode = GFX_BLIT_Additive,
		Alpha = PV_KeyFrames(0, 0, 128, 250, 64, 1000, 0),
		Size = PV_KeyFrames(0, 0, 0, 100, size ?? 160, 1000, 0),
		R = 255, G = 255, B = 64
	};
}

global func Particles_Magic()
{
	return
	{
		BlitMode = GFX_BLIT_Additive,
		Alpha = PV_Linear(128, 0),
		Size = PV_Linear(0, PV_Random(5, 15)),
		CollisionVertex = 500,
		OnCollision = PC_Die(),
		Rotation = PV_Random(0, 360)
	};
}

global func Particles_MagicRing()
{
	return
	{
		BlitMode = GFX_BLIT_Additive,
		Alpha = PV_Linear(100, 0),
		Size = PV_KeyFrames(1, 0, 0, 500, 20, 1000, 0),
		Attach = ATTACH_Front | ATTACH_MoveRelative
	};
}

global func Particles_Spark()
{
	return
	{
		BlitMode = GFX_BLIT_Additive,
		Size = PV_Linear(PV_Random(5, 15), 0),
		CollisionVertex = 500,
		OnCollision = PC_Bounce(500),
		Rotation = PV_Direction(),
		ForceY = PV_Gravity(20)
	};
}

// documented in /docs/sdk/script/fn
global func Particles_Colored(prototype, color, color2)
{
	// Colors the given particle. If color2 is given, colors in a random fade between color and color2
	if (GetType(color2))
	{
		return {
			Prototype = prototype,
			R = PV_Random((color >> 16) & 0xff, (color2 >> 16) & 0xff),
			G = PV_Random((color >>  8) & 0xff, (color2 >>  8) & 0xff),
			B = PV_Random((color >>  0) & 0xff, (color2 >>  0) & 0xff),
		};
	}
	else
		return {
			Prototype = prototype,
			R = (color >> 16) & 0xff,
			G = (color >>  8) & 0xff,
			B = (color >>  0) & 0xff
		};
}

global func Particles_SparkFire()
{
	return
	{
		Prototype = Particles_Spark(),
		R = 255, G = 200, B = 10
	};
}

global func Particles_SmokeTrail()
{
	return
	{
		Prototype = Particles_Smoke(),
		ForceY = PV_Gravity(-10),
		ForceX = PV_Wind(20),
		DampingX = 950, DampingY = 950,
		Alpha = PV_Linear(128, 0),
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(3, 10), 1000, PV_Random(8, 11))
	};
}

global func Particles_Material(int color)
{
	return
	{
		Stretch = PV_Speed(2000),
		CollisionVertex = 1000,
		OnCollision = PC_Die(),
		Size = 1,
		Rotation = PV_Direction(),
		ForceY = PV_Gravity(100),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff
	};
}

global func Particles_Trajectory()
{
	return
	{
		BlitMode = GFX_BLIT_Additive,
		Attach = ATTACH_Front | ATTACH_MoveRelative
	};
}

global func Particles_WoodChip()
{
	return
	{
		Size = PV_Random(1, 3),
		Phase = PV_Linear(0, 3),
		Alpha = PV_KeyFrames(0, 0, 255, 900, 255, 1000, 0),
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceX = PV_Wind(50),
		ForceY = PV_Gravity(100),
		DampingX = 975, DampingY = 975,
		Rotation = PV_Direction(PV_Random(750, 1250)),
		Attach = ATTACH_Front
	};
}

global func Particles_Straw()
{
	return
	{
		Prototype = Particles_WoodChip(),
		Phase = PV_Random(0, 3),
		Size = PV_Random(3, 5),
		Attach = nil
	};
}

global func Particles_Leaf(int color)
{
	return
	{
		Size = PV_Random(4, 6),
		Phase = PV_Random(0, 2),
		Rotation = PV_Random(0, 360),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		Alpha = PV_KeyFrames(0, 0, 255, 900, 255, 1000, 0),
		CollisionVertex = 800,
		OnCollision = PC_Die(),
		ForceX = PV_Wind(50),
		ForceY = PV_Gravity(100),
		DampingX = 975, DampingY = 975,
		Rotation = PV_Direction(PV_Random(750, 1250)),
		Attach = ATTACH_Front
	};
}

global func Particles_CottonBalloon()
{
	return
	{
		Prototype = Particles_WoodChip(),
		Phase = PV_Random(0, 3),
		Size = PV_Random(2, 4),
		ForceY = PV_Gravity(10),
		DampingX = 900, DampingY = 900,
		Attach = nil
	};
}

global func Particles_Air()
{
	return
	{
		Stretch = PV_Speed(500, 1000),
		Alpha = PV_Linear(255, 0),
		Phase = PV_Random(0, 3),
		DampingX = 990, DampingY = 990,
		ForceX = PV_Random(-5, 5, 30),
		ForceY = PV_Gravity(10, PV_Random(-5, 5)),
		Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(20, 30), 1000, 0),
		Rotation = PV_Direction(),
		CollisionVertex = 1000,
		OnCollision = PC_Bounce(500)
	};
}

global func Particles_Thrust(int size)
{
	size = size ?? 10;
	return
	{
		Size = PV_KeyFrames(0, 0, 0, 50, size, 1000, size * 2),
		Alpha = PV_Linear(255, 0),
		R = PV_KeyFrames(0, 0, 255, 500, 100, 1000, 50),
		G = PV_KeyFrames(0, 0, 255, 500, 100, 1000, 50),
		B = PV_KeyFrames(0, 0, 255, 500, 100, 1000, 50),
		Phase = PV_Random(0, 3, 10),
		Rotation = PV_Random(0, 360),
		DampingX = 950, DampingY = 950,
		ForceY = PV_KeyFrames(0, 0, 0, 500, 0, 1000, PV_Gravity(20)),
		ForceX = PV_KeyFrames(0, 0, 0, 500, 0, 1000, PV_Wind(50)),
		CollisionVertex = 750
	};
}

global func Particles_MuzzleFlash()
{
	return
	{
		Attach = ATTACH_Front | ATTACH_MoveRelative,
		Size = 20,
		Phase = PV_Linear(0, 5),
		BlitMode = GFX_BLIT_Additive
	};
}

global func Particles_Glimmer()
{
	return
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

global func Particles_ElectroSpark1()
{
	return
	{
		Size = PV_Random(5, 9),
		Phase = PV_Linear(0, 5),
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 750,
		OnCollision = PC_Die(),
		Rotation = PV_Direction()
	};
}

global func Particles_ElectroSpark2()
{
	return
	{
		Prototype = Particles_ElectroSpark1(),
		Phase = PV_Linear(6, 11),
	};
}


/*-- Weather Particles --*/

global func Particles_Rain(int color)
{
	return
	{
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		ForceY = PV_Gravity(1000),
		Size = PV_Random(10, 30),
		R = GetRGBaValue(color, 1),
		G = GetRGBaValue(color, 2),
		B = GetRGBaValue(color, 3),
		Rotation = PV_Direction(),
		CollisionDensity = 25,
		Stretch = 3000,
	};
}

global func Particles_Snow(int color)
{
	return
	{
		Phase = PV_Random(0, 16),
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		DampingY = 1000,//PV_Cos(PV_Linear(0,1800),5,990),
		ForceY = 0,//GetGravity()/100,//PV_Gravity(100),
		// TODO: PV_Random() here?
		ForceX = PV_Sin(PV_Step(PV_Random(5, 10), PV_Random(0, 180)), PV_Random(5, 8), 0),
		Size = PV_Random(0, 3),
		R = GetRGBaValue(color, 1),
		G = GetRGBaValue(color, 2),
		B = GetRGBaValue(color, 3),
		Rotation = PV_Random(360),
		CollisionDensity = 25,
		Stretch = 1000,
	};
}

global func Particles_RainSmall(int color)
{
	return 
	{
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		ForceY = PV_Gravity(1000),
		Size = 1,
		R = GetRGBaValue(color, 1),
		G = GetRGBaValue(color, 2),
		B = GetRGBaValue(color, 3),
		Alpha = 200,
		Rotation = PV_Direction(),
		CollisionDensity = 25,
		Stretch = PV_Speed(4000),
	};
}

global func Particles_Splash(int color)
{
	return
	{
		Phase = PV_Linear(0, 4),
		Alpha = PV_KeyFrames(0, 0, 255, 500, 255, 1000, 0),
		Size = PV_Random(5, 10),
		R = GetRGBaValue(color, 1),
		G = GetRGBaValue(color, 2),
		B = GetRGBaValue(color, 3),
		Rotation = PV_Random(-5,5),
		Stretch = PV_Random(500, 1000),
	};
}

global func Particles_SplashWater(int color)
{
	return
	{
		Phase = PV_Linear(0, 13),
		Alpha = PV_KeyFrames(0, 0, 255, 500, 255, 1000, 0),
		Size = PV_Random(2, 5),
		R = GetRGBaValue(color, 1),
		G = GetRGBaValue(color, 2),
		B = GetRGBaValue(color, 3),
		Rotation = PV_Random(-5,5),
		Stretch = 3000,
		Attach = ATTACH_Back,
	};
}

global func Particles_Hail(int color)
{
	return
	{
		CollisionVertex = 0,
		ForceY = PV_Gravity(1000),
		OnCollision = PC_Stop(),
		Size = 2,
		Alpha = PV_KeyFrames(255, 0, 255, 500, 255, 1000, 0),
		R = GetRGBaValue(color, 1),
		G = GetRGBaValue(color, 2),
		B = GetRGBaValue(color, 3),
		Rotation = PV_Random(360),
	};
}
