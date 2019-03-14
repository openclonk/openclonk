/**
	Control Logic

	Controls the benchmark execution. Each benchmark has a timeout of 2 minutes,
	so that in the worst case you will not sit there waiting an eternity for the
	scenario to finish.
 */

static const BENCHMARK_TIMEOUT_MS = 120000;

/* --- Test Effect --- */

static const IntBenchmarkControl = new Effect
{
	// Data

	ScenarioQueue = [],
	ScenarioCurrent = nil,
	RunData = nil,
	RunNr = 1,

	// Internal status
	start_time = 0,  // Time in ms when the benchmark started, for FPS evaluation
	start_frame = 0, // Frame counter when the benchmark started, for FPS evaluation
	calc_fps = 0,    // Frames per second, as calculated
	is_running = false,
	is_profiling = false,

	Start = func (int temporary)
	{
		if (!temporary)
		{
			// Set default interval.
			this.Interval = 1;
		}
		return FX_OK;
	},

	Timer = func ()
	{
		// Load the next one
		if (ScenarioCurrent == nil)
		{
			// Done!
			if (GetLength(ScenarioQueue) == 0)
			{
				Log("=========================================================================================="); // Large separator
				Log("All benchmarks have been completed!");
				return FX_Execute_Kill;
			}
			else
			{
				ScenarioCurrent = PopFront(ScenarioQueue);

				Log("=========================================================================================="); // Large separator
				Log("Starting benchmark: %s", ScenarioCurrent.Description);
				Log("=========================================================================================="); // Large separator
			}
		}
		// Run all the benchmark runs
		else
		{
			if (RunData)
			{
				var millis_passed = Max(1, GetTime() - GetControl().start_time);
				var timeout_exceeded = millis_passed > RunData.Timeout;
				if (RunData->IsFinished() || timeout_exceeded)
				{
					StopTimer();
					StopRun();

					RunData = nil;
					FinishScenario();
				}

				var frames_passed = Max(0, FrameCounter() - GetControl().start_frame);
			}
			else // Start!
			{
				StartRun();
				StartTimer();
			}
		}
	},

	FinishScenario = func ()
	{
		// For skipping
		StopTimer();
		StopRun();

		// Real end
		Log("Finished benchmark: %s", ScenarioCurrent.Description);
		Log("=========================================================================================="); // Large separator
		ScenarioCurrent = nil;
		RunData = nil;
		RunNr = 1;
	},

	StartRun = func ()
	{
		var data = { Prototype = ScenarioCurrent.Run };
		data->OnStart(RunNr);
		RunData = data;
		is_running = true;
		Log("Run benchmark %d:", RunNr);
		Log("==============================");
	},

	StopRun = func ()
	{
		if (is_running)
		{
			RunData->OnFinished();
			is_running = false;
		}
	},

	StartTimer = func ()
	{
		start_time = GetTime();
		start_frame = FrameCounter();
		StartScriptProfiler();
		is_profiling = true;
	},

	StopTimer = func (bool force_stop)
	{
		if (is_profiling)
		{
			is_profiling = false;
			var frames_passed = Max(0, FrameCounter() - start_frame);
			var millis_passed = Max(1, GetTime() - start_time);
			calc_fps = 1000 * frames_passed / millis_passed;
			if (force_stop)
			{
				Log("==============================");
				Log("<<<       FORCE STOP       >>>");
			}
			//Log("    +   Benchmark result: %d frames passed, took %d milliseconds. Average FPS: %d", frames_passed, millis_passed, calc_fps);
			Log("Benchmark run result:");
			Log("=============================="); // Same width as profiler statistics
			Log("%06d    frames passed", frames_passed);
			Log("%06d    milliseconds passed", millis_passed);
			Log("%06d    FPS on average", calc_fps);
			Log("Note: Benchmark is non-deterministic regarding frames per second");
			Log("==============================");
			StopScriptProfiler();
		}
	},

	// -------------------------------------

	Queue = func (proplist scenario)
	{
		PushBack(this.ScenarioQueue, scenario);
	},
};


global func GetControl(bool create)
{
	if (create)
	{
		return Scenario->CreateEffect(IntBenchmarkControl, 100, 2);
	}
	else
	{
		return GetEffect("IntBenchmarkControl", Scenario);
	}
}


// Calling this function skips the current benchmark.
global func Skip()
{
	// Get the control effect.
	var benchmark = GetControl();
	if (benchmark)
	{
		Log("==============================");
		Log("<<<     SKIPPED BY USER    >>>");

		benchmark->FinishScenario();
	}
}

