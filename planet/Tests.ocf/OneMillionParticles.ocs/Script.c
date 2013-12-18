static const LOG_LINE_LEN = 50;
static PARTICLES;
static const PARTICLE_COUNT = 1000000; // in words ONE MILLION
static const TEST_RUN_COUNT = 2;
static PARTICLES_PER_RUN;

static MEASUREMENTS;

func Initialize()
{
	PARTICLES =
	{
		Prototype = Particles_Glimmer(),
		R = PV_Random(0, 255),
		G = PV_Random(0, 255),
		B = PV_Random(0, 255),
		Size = PV_Linear(1, 0),
		OnCollision = PC_Bounce(),
		ForceX = PV_Random(-5, 5, 10),
		ForceY = PV_Random(-5, 5, 10)
	};
	PARTICLES_PER_RUN = PARTICLE_COUNT / TEST_RUN_COUNT;
	MEASUREMENTS = [];
	
	Schedule(nil, "Test1()", 1);
	// PauseGame(); // used for testing purposes
}

global func Test1()
{
	var t = GetTime();
	CreateParticle("FireDense", PV_Random(0, LandscapeWidth()), PV_Random(0, LandscapeHeight()/2), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(100, 300), PARTICLES, PARTICLES_PER_RUN);
	t = GetTime() - t;
	FixLenLog("Test1 - batch creation", t, LOG_LINE_LEN, "ms");
	MEASUREMENTS[0] = t;
	Schedule(nil, "Test2()", 36);
	// PauseGame(); // used for testing purposes
}

global func Test2()
{
	var t = GetTime();
	for (var i = 0; i < PARTICLES_PER_RUN; ++i)
		CreateParticle("FireDense", PV_Random(0, LandscapeWidth()), PV_Random(0, LandscapeHeight()/2), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(100, 300), PARTICLES);
	t = GetTime() - t;
	FixLenLog("Test2 - seperate creation", t, LOG_LINE_LEN, "ms");
	MEASUREMENTS[1] = t;
	Schedule(nil, "Test3()", 300);
	// PauseGame(); // used for testing purposes
}

global func Test3()
{
	// SPAWN FIRE
	for (var fire_count = 20; fire_count > 0; --fire_count)
	{
		var x = RandomX(100, LandscapeWidth() - 100);
		var y = LandscapeHeight() / 2 - 20;
		AddEffect("ScorchEverything", nil, 1, 1, nil, nil, x, y);
	}
}

global func FxScorchEverythingStart(target, effect, temp, x, y)
{
	if (temp) return;
	effect.x = x;
	effect.y = y;
	effect.offset = Random(180);
	effect.glimmer = 
	{
		Prototype = Particles_Glimmer(),
		ForceY = PV_Random(-10, 5),
		ForceX = PV_Random(-10, 10, 10)
	};
	effect.fire =
	{
		Prototype = Particles_Fire(),
		R = PV_KeyFrames(0, 0, 255, 500, 255, 1000, 0),
		G = PV_KeyFrames(0, 0, 255, 500, 255, 1000, 0),
		B = PV_KeyFrames(0, 0, 255, 500, 255, 1000, 0),
		Alpha = PV_KeyFrames(0, 0, 255, 900, 255, 1000, 0),
		Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(10, 20), 1000, PV_Random(20, 30))
	};
	
	effect.fire_na =
	{
		Prototype = effect.fire,
		BlitMode = 0
	};
}

global func FxScorchEverythingTimer(target, effect, time)
{
	var strength = Abs(Sin(effect.offset + time / 2, 100));
	var glimmer_count = strength / 2;
	var fire_density = (strength - 30) / 10;
	var glimmer_speed = strength / 4;
	CreateParticle("Fire", effect.x, effect.y, PV_Random(-glimmer_speed, glimmer_speed), PV_Random(-glimmer_speed, 5), PV_Random(Max(30, strength), Max(60, strength * 2)), effect.glimmer, glimmer_count);
	if (fire_density > 0)
	{	
		var speed = 2 * fire_density;
		var size = 2 * fire_density;
		effect.fire.Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(size, 2 * size), 900, PV_Random(2 * size, 3 * size), 1000, 0);
		CreateParticle("FireDense", effect.x, effect.y, PV_Random(-speed, speed), PV_Random(-speed, speed), PV_Random(30, Min(60, fire_density * 20)), effect.fire_na, fire_density);
		CreateParticle("Fire", effect.x, effect.y, PV_Random(-speed, speed), PV_Random(-speed, speed), PV_Random(30, Min(60, fire_density * 20)), effect.fire, fire_density);
	}
}


global func FixLenLog(string msg, int value, int len, string unit)
{
	len = len ?? 30;
	unit = unit ?? "";
	var num_len = 4;
	
	while (GetLength(msg) < len - num_len)
		msg = Format("%s_", msg);
	Log("%s%4d%s", msg, value, unit);
}