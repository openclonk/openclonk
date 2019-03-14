/**
	Benchmarks
	
	@author Marky
*/

/* --- Setup --- */

static script_player;

func Initialize()
{
	if (IsNetwork())
	{
		Log("Benchmark does not work in a network game!");
		GameOver();
		return;
	}
	
	// Create a script player for some tests.
	script_player = nil;
	CreateScriptPlayer("Buddy", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);
}

func InitializePlayer(int player)
{
	// Set zoom to full map size.
	SetPlayerZoomByViewRange(player, LandscapeWidth(), nil, PLRZOOM_Direct);
	
	// No FoW to see everything happening.
	SetFoW(false, player);
		
	// Initialize script player.
	if (GetPlayerType(player) == C4PT_Script)
	{
		// Store the player number.
		if (script_player == nil)
		{
			script_player = player;
		}
		// No crew needed.
		GetCrew(player)->RemoveObject();
		return;
	}
	else
	{
		// Move player to the start of the scenario.
		GetCrew(player)->SetPosition(265, 180);
		StartBenchmarks();
	}
}

func RemovePlayer(int player)
{
	// Remove script player.
	if ((GetPlayerType(player) == C4PT_Script) && (player == script_player))
	{
		script_player = nil;
	}
}

/* --- Benchmark logic --- */

global func StartBenchmarks()
{
	GetControl(true)
	->Queue(new Scenario_Coconuts{});
}

static const BenchmarkScenario = new Global
{
	Description = nil, // String, defines the title of the scenario
	Run         = nil, // Proplist, a benchmark run template
};

static const BenchmarkRun = new Global
{
	OnStart    = nil,    // Function call when the benchmark starts
	IsFinished = nil,    // Function call that determines whether the benchmark is over
	OnFinish   = nil,    // Function call when the benchmark is over
	Timeout    = 120000, // Timeout in milliseconds when the run is force-stopped,
	FPSLimit   = 5,      // Queue another run with more object if the limit is not hit
};

/* --- Benchmark Scenarios --- */

static const Scenario_Coconuts = new BenchmarkScenario
{
	Description = "Launch bouncy objects (Coconut(s)) in a confined area",
	Run = Run_Coconuts,
};

static const Run_Coconuts = new Run_LaunchObjects
{
	Types = [Coconut],
	Amount = [1000],
};


/* --- Templates --- */

static const Run_LaunchObjects = new BenchmarkRun
{
	Types = [],
	Amount = [1000],
	RangeX = [30, 240],
	RangeY = [30, 120],
	Speed = 50,
	Launched = [],

	OnStart = func (int nr)
	{
		this.objects = [];
		for (var t = 0; t < GetLength(this.Types); ++t)
		{
			var amount = Amount[t];
			var type = Types[t];
			for (var i = 0; i < amount; ++i)
			{
				var to_launch = CreateObject(type, 0, 0, NO_OWNER);
				PushBack(this.Launched, to_launch);
				
				var placement_invalid = true;
				for (var j = 0; placement_invalid && j < 50; ++j)
				{
					var x = RandomX(this.RangeX[0], this.RangeX[1]);
					var y = RandomX(this.RangeY[0], this.RangeY[1]);
					to_launch->SetPosition(x, y);
					placement_invalid = to_launch->Stuck();
				}
				to_launch->SetSpeed(RandomX(-this.Speed, this.Speed), RandomX(-this.Speed, this.Speed));
			}
			Log("Launched %d %i(s)", amount, type);
		}
	},
	
	IsFinished = func ()
	{
		for (var target in this.Launched)
		{
			if (target && target->GetSpeed() > 1)
			{
				return false;
			}
		}
		return true;
	},
	
	OnFinished = func ()
	{
		for (var target in this.Launched)
		{
			if (target) target->RemoveObject();
		}
	},
};

//-----------------------------------

global func Test4_OnStart(int player)
{

	GetControl().start_time = GetTime();
	GetControl().start_frame = FrameCounter();
	StartScriptProfiler();
	return true;
}

global func Test4_OnFinished()
{
	StopScriptProfiler();
	var frames_passed = Max(0, FrameCounter() - GetControl().start_frame);
	var millis_passed = Max(1, GetTime() - GetControl().start_time);
		var fps = 1000 * frames_passed / millis_passed;
		Log("Benchmark result: %d frames passed, took %d milliseconds. Average FPS: %d", frames_passed, millis_passed, fps);
		Log("Note: Benchmark is non-deterministic regarding frame count");
}

