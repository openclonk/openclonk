/**
	This file contains some default particle behavior definitions.
*/

global func Particles_Dust()
{
	return
	{
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceX = PV_Wind(20),
		ForceY = PV_Gravity(25),
		Alpha = PV_Linear(30, 0),
		Rotation = PV_Random(0, 360),
		Size = PV_KeyFrames(0, 0, 0, 100, 25, 1000, 15)
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


global func Particles_Smoke()
{
	return
	{
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceY = PV_Gravity(-100),
		ForceX = PV_Wind(200),
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

global func Particles_Flash()
{
	return
	{
		BlitMode = GFX_BLIT_Additive,
		Alpha = PV_KeyFrames(0, 0, 128, 250, 64, 1000, 0),
		Size = PV_KeyFrames(0, 0, 0, 100, 160, 1000, 0),
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
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(10, 30), 1000, PV_Random(25, 35))
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
