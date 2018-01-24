/**
	Ambience
	Controls sound and music depending on the environment the player is in
	
	@author Sven2
*/

local exec_counter; // counter to distribute execution of players across frames
local last_environment; // array indexed by player number: pointer to environment the player was in last
local environments; // array of available environments for which it is checked if the player is in. sorted by priority.

// Initialization
protected func Initialize()
{
	// Base environment
	Environment = {
		actions = [],
		min_change_delay = 1,
		min_initial_change_delay = 5,
		AddSound = this.Env_AddSound,
		AddAction = this.Env_AddAction,
		SetMusic = this.Env_SetMusic
	};
	// Register default environments (overloadable)
	this->InitializeEnvironments();
	// Periodic execution of ambience events
	last_environment = [];
	AddTimer(this.Execute, 10);
	return true;
}

func InitializeEnvironments()
{
	// Register all standard environments
	environments = [];
	// Underwater: Clonk is swimming in water
	var underwater = this.env_underwater = new Environment {};
	underwater->SetMusic("underwater");
	underwater.CheckPlayer = this.EnvCheck_Underwater;
	AddEnvironment(underwater, 1400);
	// City: Clonk is surrounded by buildings
	var city = this.env_city = new Environment {};
	city->SetMusic("city");
	city.CheckPlayer = this.EnvCheck_City;
	AddEnvironment(city, 1200);
	// Lava: Lava material is nearby
	var lava = this.env_lava = new Environment {};
	lava->SetMusic("lava");
	lava.CheckPlayer = this.EnvCheck_Lava;
	lava.mat_mask = CreateArray(); // material mask for lava materials. +1 cuz sky.
	lava.mat_mask[Material("Lava")+1] = true; // loop over materials and check incindiary instead? Whoever introduces the next lava type can do that...
	lava.mat_mask[Material("DuroLava")+1] = true;
	lava.min_change_delay = 3; // Easy to miss lava on search.
	AddEnvironment(lava, 1000);
	// Underground: Clonk in front of tunnel
	var underground = this.env_underground = new Environment {};
	underground->SetMusic("underground");
	underground.CheckPlayer = this.EnvCheck_Underground;
	AddEnvironment(underground, 800);
	// Mountains: Overground and lots of rock around
	var mountains = this.env_mountains = new Environment {};
	mountains->SetMusic("mountains");
	mountains.CheckPlayer = this.EnvCheck_Mountains;
	mountains.mat_mask = CreateArray(); // material mask for mountain materials. +1 cuz sky.
	mountains.mat_mask[Material("Rock")+1] = true;
	mountains.mat_mask[Material("Granite")+1] = true;
	mountains.mat_mask[Material("Ore")+1] = true;
	mountains.mat_mask[Material("Gold")+1] = true;
	mountains.min_change_delay = 3; // Pretty unstable condition
	AddEnvironment(mountains, 600);
	// Snow: It's snowing around the clonk
	var snow = this.env_snow = new Environment {};
	snow->SetMusic("snow");
	snow.CheckPlayer = this.EnvCheck_Snow;
	snow.min_change_delay = 6; // Persist a while after snowing stopped
	snow.mat = Material("Snow");
	AddEnvironment(snow, 400);
	// Night: Sunlight blocked by planet
	var night = this.env_night = new Environment {};
	night->SetMusic("night");
	night.CheckPlayer = this.EnvCheck_Night;
	AddEnvironment(night, 200);
	// Overground: Default environment
	var overground = this.env_overground = new Environment {};
	overground->SetMusic("overground");
	overground.CheckPlayer = this.EnvCheck_Overground;
	overground->AddSound("UI::Ding", 100);
	AddEnvironment(overground, 0);
	return true;
}

private func Execute()
{
	// Per-player execution every third timer (~.8 seconds)
	var i=GetPlayerCount(C4PT_User);
	while (i--) if (!(++exec_counter % 3))
	{
		ExecutePlayer(GetPlayerByIndex(i, C4PT_User));
	}
	return true;
}

private func ExecutePlayer(int plr)
{
	var cursor = GetCursor(plr);
	// Determine environment the player is currently in
	var environment = nil;
	if (cursor)
	{
		var last_env = last_environment[plr];
		var x = cursor->GetX(), y = cursor->GetY();
		for (var test_environment in environments)
		{
			if (environment = test_environment->CheckPlayer(cursor, x, y, test_environment == last_env))
			{
				// We've found a matchign environment.
				// Was it a change? Then check delays first
				if (test_environment != last_env)
				{
					if (last_env && last_env.no_change_delay)
					{
						// Environment should change but a delay is specified. Keep last environment for now.
						--last_env.no_change_delay;
						environment = last_env;
						break;
					}
					// New environment and change delay has passed.
					environment.no_change_delay = environment.min_initial_change_delay;
					Log("%s environment: %s", GetPlayerName(plr), environment.music);
				}
				else
				{
					// Was no change: Reset change delays
					environment.no_change_delay = Max(environment.no_change_delay, environment.min_change_delay);
				}
				break;
			}
		}
	}
	last_environment[plr] = environment;
	if (!environment) return true;
	// Music by environment
	this->SetPlayList(environment.music, plr, true, 3000);
	// Sounds and actions by environment
	for (var action in environment.actions)
		if (Random(1000) < action.chance)
			cursor->Call(action.fn, action.par[0], action.par[1], action.par[2], action.par[3], action.par[4]);
	return true;
}

func InitializePlayer(int plr)
{
	// Newly joining players should have set playlist immediately (so they don't start playing a random song just to switch it immediately)
	ExecutePlayer(plr);
	return true;
}

func RemovePlayer(int plr)
{
	// Ensure newly joining players don't check on another player's environment
	last_environment[plr] = nil;
	return true;
}

protected func Activate(int byplr)
{
	MessageWindow(this.Description, byplr);
	return true;
}

/* Environment functions */

func AddEnvironment(proplist new_env, priority)
{
	if (GetType(priority)) new_env.Priority = priority;
	this.environments[GetLength(environments)] = new_env;
	SortArrayByProperty(this.environments, "Priority", true);
	return true;
}

private func Env_AddSound(string snd_name, chance)
{
	return Env_AddAction(Global.Sound, snd_name, chance ?? 50);
}

private func Env_AddAction(afn, par0, par1, par2, par3, par4)
{
	return this.actions[GetLength(this.actions)] = { fn=afn, par=[par0, par1, par2, par3, par4] };
}

private func Env_SetMusic(string playlist)
{
	this.music = playlist;
	return true;
}

/* Default environment checks */

private func EnvCheck_Underwater(object cursor, int x, int y, bool is_current)
{
	// Clonk should be swimming
	if (cursor->GetProcedure() != "SWIM") return nil;
	// For initial change, clonk should also be diving: Check for breath below 80%
	// Use > instead of >= to ensure 0-breath-clonks can also get the environment
	if (!is_current && cursor->GetBreath() > cursor.MaxBreath*4/5) return nil;
	return this;
}

private func EnvCheck_City(object cursor, int x, int y, bool is_current)
{
	// There must be buildings around the clonk
	var building_count = cursor->ObjectCount(cursor->Find_AtRect(-180,-100,360,200), Find_Func("IsStructure"));
	// 3 buildings to start the environment. Just 1 building to sustain it.
	if (building_count < 3-2*is_current) return nil;
	return this;
}

private func EnvCheck_Lava(object cursor, int x, int y, bool is_current)
{
	// Check for lava pixels. First check if the last lava pixel we found is still in place.
	var search_range;
	if (is_current)
	{
		if (this.mat_mask[GetMaterial(this.last_x, this.last_y)+1])
			if (Distance(this.last_x, this.last_y, x, y) < 140)
				return this;
		search_range = 140;
	}
	else
	{
		search_range = 70;
	}
	// Now search for lava in search range
	var ang = Random(360);
	for (; search_range >= 0; search_range -= 10)
	{
		ang += 200;
		var x2 = x + Sin(ang, search_range);
		var y2 = y + Cos(ang, search_range);
		if (this.mat_mask[GetMaterial(x2, y2)+1])
		{
			// Lava found!
			this.last_x = x2;
			this.last_y = y2;
			return this;
		}
	}
	// No lava found
	return nil;
}

private func EnvCheck_Underground(object cursor, int x, int y, bool is_current)
{
	// Check for underground: No sky at cursor or above
	if (GetMaterial(x,y)<0) return nil;
	if (GetMaterial(x,y-30)<0) return nil;
	if (GetMaterial(x-10,y-20)<0) return nil;
	if (GetMaterial(x+10,y-20)<0) return nil;
	return this;
}

private func EnvCheck_Mountains(object cursor, int x, int y, bool is_current)
{
	// Check for mountains: Rock materials below
	var num_rock;
	for (var y2=0; y2<=45; y2+=15)
		for (var x2=-75; x2<=75; x2+=15)
			num_rock += this.mat_mask[GetMaterial(x+x2,y+y2)+1];
	// need 15pts on first check; 5 to sustain
	if (num_rock < 15-is_current*10) return nil;
	return this;
}

private func EnvCheck_Snow(object cursor, int x, int y, bool is_current)
{
	// Must be snowing from above
	if (GetPXSCount(this.mat, x-300, y-200, 600, 300) < 20 - is_current*15) return nil;
	return this;
}

private func EnvCheck_Night(object cursor, int x, int y, bool is_current)
{
	// Night time.
	if (!Time->IsNight()) return nil;
	return this;
}

private func EnvCheck_Overground(object cursor, int x, int y, bool is_current)
{
	// This is the fallback environment
	return this;
}

/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
local Environment;
