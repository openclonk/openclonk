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
	->Queue(new Scenario_Coconuts{})
	->Queue(new Scenario_Firestone{})
	->Queue(new Scenario_Bombs{})
	->Queue(new Scenario_MixedObjects{});
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
	Succeed    = nil,    // Function call when the benchmark follows after another benchmark; Parameter: previous BenchmarkRun
	
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
	Progress = [Global.DoubleAmount],
};


static const Scenario_Firestone = new BenchmarkScenario
{
	Description = "Launch explosive objects (Firestone(s)) in a confined area",
	Run = Run_Firestone,
};

static const Run_Firestone = new Run_LaunchObjects
{
	Types = [Firestone],
	Amount = [1000],
	Progress = [Global.DoubleAmount],
};


static const Scenario_Bombs = new BenchmarkScenario
{
	Description = "Launch objects with effects (IronBomb(s)) in a confined area",
	Run = Run_Bombs,
};

static const Run_Bombs = new Run_LaunchObjects
{
	Types = [IronBomb],
	Amount = [50],
	Callback = ["Fuse"],
};


static const Scenario_MixedObjects = new BenchmarkScenario
{
	Description = "Launch complex objects (Clonk(s), Lorry(s)) that come under the effect of simple and more complex objects",
	Run = Run_MixedObjects,
};

static const Run_MixedObjects = new Run_LaunchObjects
{
	Types = [Clonk, Coconut, IronBomb, Lorry],
	Amount = [200, 100, 10, 2],
	Progress = [Global.DoubleAmount, Global.DoubleAmount, Global.DoubleAmount, Global.DoubleAmount],
	Callback = [nil, nil, "Fuse", nil],
	
	IsFinished = func ()
	{
		for (var target in this.Launched)
		{
			if (!target) continue;
			if (target->GetID() == Lorry) continue;

			if (target->GetSpeed() > 1)
			{
				return false;
			}
		}
		return true;
	},
};


/* --- Templates --- */

static const Run_LaunchObjects = new BenchmarkRun
{
	// It may be more useful to create multiple runs inside a scenario, 
	// instead of mapping the data by index
	
	Types    = [],        // This type of objects is launched
	Amount   = [1000],    // This amount is launched
	Progress = [],        // This is the progress function for successor runs; Defaults to adding the base amount
	Callback = [],        // This function is called in the launched target of that type
	RangeX   = [30, 240], // Position range for object
	RangeY   = [30, 120], // Position range for object
	Speed    = 50,        // Speed range for object
	Launched = [],        // Saves all launched objects

	OnStart = func (int nr)
	{
		this.objects = [];
		for (var t = 0; t < GetLength(this.Types); ++t)
		{
			var amount = Amount[t];
			var type = Types[t];
			var callback = Callback[t];
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
				if (callback)
				{
					to_launch->Call(callback);
				}
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
		// Remove known objects
		for (var target in this.Launched)
		{
			if (target) target->RemoveObject();
		}
		// Remove side effects
		for (var garbage in FindObjects(Find_Not(Find_Owner(), Find_OCF(OCF_CrewMember))))
		{
			if (garbage) garbage->RemoveObject();
		}
	},
	
	Succeed = func (proplist previous)
	{
		var progressed_amount = []; // Save everything in a temp array, because some functions rely on the array 'Amount'
		for (var i = 0; i < GetLength(Amount); ++i)
		{
			progressed_amount[i] = Call(Progress[i] ?? Global.AddInitialAmount, previous.Amount[i], Amount[i]);
		}
		Amount = progressed_amount;
	},
};

//-----------------------------------
// Successor functions
	
global func AddInitialAmount (int their_amount, int my_amount)
{
	return their_amount + my_amount;
}

global func DoubleAmount (int their_amount, int my_amount)
{
	return 2 * their_amount;
}
