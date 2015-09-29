/**
	Ambience
	Controls sound and music depending on the environment the player is in
	
	@author Sven2
*/

local exec_counter; // counter to distribute execution of players across frames
local player_environments;  // array indexed by player number: array of derived environments with per-player data
local all_environments; // array of available environments for which it is checked if the player is in. sorted by priority.

// Initialization
protected func Initialize()
{
	// Register default environments (overloadable)
	this->InitializeEnvironments();
	// Initial player data
	player_environments = [];
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		InitializePlayer(GetPlayerByIndex(i, C4PT_User));
	// Periodic execution of ambience events
	AddTimer(this.Execute, 10);
	return true;
}

func InitializeEnvironments()
{
	// Register all standard environments
	all_environments = [];
	
	// Underwater: Clonk is swimming in water
	var underwater = this.env_underwater = new Environment
	{
		Name = "Underwater",
		CheckPlayer = this.EnvCheck_Underwater,
		music = "underwater",
		sound_modifier = UnderwaterModifier,
		exclusive = true, // Do not player other stuff (such as cave sound) when diving.
	};
	
	AddEnvironment(underwater, 1400);
	
	// City: Clonk is surrounded by buildings
	var city = this.env_city = new Environment
	{
		Name = "City",
		CheckPlayer = this.EnvCheck_City,
	};
	//AddEnvironment(city, 1200); - no music/sound for now
	
	// Lava: Lava material is nearby
	var lava = this.env_lava = new Environment
	{
		Name = "Lava",
		CheckPlayer = this.EnvCheck_Lava,
		min_change_delays = [1, 3], // Easy to miss lava on search.
	};
	lava.mat_mask = CreateArray(); // material mask for lava materials. +1 cuz sky.
	lava.mat_mask[Material("Lava")+1] = true; // loop over materials and check incindiary instead? Whoever introduces the next lava type can do that...
	lava.mat_mask[Material("DuroLava")+1] = true;
	//AddEnvironment(lava, 1000); - no music/sound for now
	
	// Underground: Clonk in front of tunnel
	var underground = this.env_underground = new Environment
	{
		Name = "Underground",
		CheckPlayer = this.EnvCheck_Underground,
		music = "underground",
		sound_modifier = CaveModifier,
	};
	AddEnvironment(underground, 800);
	
	// Mountains: Overground and lots of rock around
	var mountains = this.env_mountains = new Environment
	{
		Name = "Mountains",
		CheckPlayer = this.EnvCheck_Mountains,
		min_change_delay = [3, 3], // Pretty unstable condition
	};
	mountains.mat_mask = CreateArray(); // material mask for mountain materials. +1 cuz sky.
	mountains.mat_mask[Material("Rock")+1] = true;
	mountains.mat_mask[Material("Granite")+1] = true;
	mountains.mat_mask[Material("Ore")+1] = true;
	mountains.mat_mask[Material("Gold")+1] = true;
	//AddEnvironment(mountains, 600); - no music/sound for now
	
	// Snow: It's snowing around the clonk
	var snow = this.env_snow = new Environment
	{
		Name = "Snow",
		CheckPlayer = this.EnvCheck_Snow,
		min_change_delay = [1, 6], // Persist a while after snowing stopped
	};
	snow.mat = Material("Snow");
	//AddEnvironment(snow, 400); - no music/sound for now
	
	// Night: Sunlight blocked by planet
	var night = this.env_night = new Environment
	{
		Name = "Night",
		CheckPlayer = this.EnvCheck_Night,
		music = "night",
	};
	AddEnvironment(night, 200);
	
	// Day: Default environment
	var day = this.env_day = new Environment
	{
		Name = "Day",
		CheckPlayer = this.EnvCheck_Day,
		music = "day",
	};
	AddEnvironment(day, 0);
	
	return true;
}

private func Execute()
{
	// Per-player execution every third timer (~.8 seconds)
	var i=GetPlayerCount(C4PT_User), plr;
	exec_counter += !(i%3);
	while (i--) if (!(++exec_counter % 3))
	{
		plr = GetPlayerByIndex(i, C4PT_User);
		ExecutePlayer(plr, player_environments[plr]);
	}
	return true;
}

private func ExecutePlayer(int plr, array environments)
{
	var cursor = GetCursor(plr);
	if (!cursor) cursor = GetPlrView(plr);
	// Update active state of all player environments
	if (cursor)
	{
		var x = cursor->GetX(), y = cursor->GetY();
		for (var env in environments)
		{
			var was_active = env.is_active;
			var is_active = env->CheckPlayer(cursor, x, y, was_active);
			if (is_active == was_active)
			{
				// No change. Reset change delays.
				env.change_delay = 0;
			}
			else
			{
				// Environment change. The change must persist for a while to become active.
				if (++env.change_delay > env.min_change_delays[!is_active])
				{
					// Waited long enough. Activate or deactivate this environment.
					env.is_active = is_active;
					//Log("%s environment: %s set to %v", GetPlayerName(plr), env.Name, is_active);
				}
			}
		}
		// Sound and music by active environments
		var has_music = false, sound_modifier = nil;
		for (var env in environments) if (env.is_active)
		{
			// Music!
			if (env.music && !has_music)
			{
				this->SetPlayList(env.music, plr, true, 3000, 10000);
				has_music = true;
			}
			// Sound effects like cave reverb etc.
			if (env.sound_modifier && !sound_modifier) sound_modifier = env.sound_modifier;
			// Sounds and actions by environment
			for (var action in env.actions)
			{
				if (Random(1000) < action.chance)
				{
					cursor->Call(action.fn, action.par[0], action.par[1], action.par[2], action.par[3], action.par[4]);
				}
			}
			// Does this stop all other environments?
			if (env.is_exclusive) break;
		}
		// Apply or clear global sound modifier
		SetGlobalSoundModifier(sound_modifier, plr);
	}
	return true;
}

func InitializePlayer(int plr)
{
	if (GetPlayerType(plr) == C4PT_User)
	{
		// Every player keeps a copy of the environment list to maintain delays
		// Start with a large change delay to ensure first execution does set a proper environment
		var n = GetLength(all_environments);
		var envs = CreateArray(n);
		for (var i=0; i < n; ++i)
			envs[i] = new all_environments[i] { change_delay = 999, is_active = false };
		player_environments[plr] = envs;
		// Newly joining players should have set playlist immediately (so they don't start playing a random song just to switch it immediately)
		// However, this only works with a cursor
		ExecutePlayer(plr, envs);
	}
	return true;
}

func RemovePlayer(int plr)
{
	// Ensure newly joining players don't check on another player's environment
	player_environments[plr] = nil;
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
	this.all_environments[GetLength(all_environments)] = new_env;
	SortArrayByProperty(this.all_environments, "Priority", true);
	return true;
}

private func Env_AddSound(chance, string snd_name)
{
	return Env_AddAction(chance ?? 50, Global.Sound, snd_name);
}

private func Env_AddAction(achance, afn, par0, par1, par2, par3, par4)
{
	// Make sure to not write into prototype proplist.
	if (this.actions == this.Prototype.actions) this.actions = [];
	return this.actions[GetLength(this.actions)] = { chance=achance, fn=afn, par=[par0, par1, par2, par3, par4] };
}

/* Default environment checks */

private func EnvCheck_Underwater(object cursor, int x, int y, bool is_current)
{
	// Clonk should be swimming
	if (cursor->GetProcedure() != "SWIM") return false;
	// For initial change, clonk should also be diving: Check for breath below 95%
	// Use > instead of >= to ensure 0-breath-clonks can also get the environment
	if (!is_current && cursor->GetBreath() > cursor.MaxBreath*95/100) return false;
	return true;
}

private func EnvCheck_City(object cursor, int x, int y, bool is_current)
{
	// There must be buildings around the clonk
	var building_count = cursor->ObjectCount(cursor->Find_AtRect(-180,-100,360,200), Find_Func("IsStructure"));
	// 3 buildings to start the environment. Just 1 building to sustain it.
	if (building_count < 3-2*is_current) return false;
	return true;
}

private func EnvCheck_Lava(object cursor, int x, int y, bool is_current)
{
	// Check for lava pixels. First check if the last lava pixel we found is still in place.
	var search_range;
	if (is_current)
	{
		if (this.mat_mask[GetMaterial(this.last_x, this.last_y)+1])
			if (Distance(this.last_x, this.last_y, x, y) < 140)
				return true;
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
			return true;
		}
	}
	// No lava found
	return false;
}

private func EnvCheck_Underground(object cursor, int x, int y, bool is_current)
{
	// Check for underground: No sky at cursor or above
	if (GetMaterial(x,y)<0) return false;
	if (GetMaterial(x,y-30)<0) return false;
	if (GetMaterial(x-10,y-20)<0) return false;
	if (GetMaterial(x+10,y-20)<0) return false;
	return true;
}

private func EnvCheck_Mountains(object cursor, int x, int y, bool is_current)
{
	// Check for mountains: Rock materials below
	var num_rock;
	for (var y2=0; y2<=45; y2+=15)
		for (var x2=-75; x2<=75; x2+=15)
			num_rock += this.mat_mask[GetMaterial(x+x2,y+y2)+1];
	// need 15pts on first check; 5 to sustain
	if (num_rock < 15-is_current*10) return false;
	return true;
}

private func EnvCheck_Snow(object cursor, int x, int y, bool is_current)
{
	// Must be snowing from above
	if (GetPXSCount(this.mat, x-300, y-200, 600, 300) < 20 - is_current*15) return false;
	return true;
}

private func EnvCheck_Night(object cursor, int x, int y, bool is_current)
{
	// Night time.
	return Time->IsNight();
}

private func EnvCheck_Day(object cursor, int x, int y, bool is_current)
{
	// This is the fallback environment
	return true;
}

public func SaveScenarioObject(proplist props, ...)
{
	// Only save ambience if it has modifications set for this scenario
	// However, modifications aren't possible for now so never save it
	return false;
}

/*-- Proplist --*/

local SoundModifier, CaveModifier, UnderwaterModifier;

private func ReleaseSoundModifier() { return ChangeSoundModifier(this, true); }
private func UpdateSoundModifier() { return ChangeSoundModifier(this, false); } // OpenAL-Soft implementation does not work for all modifiers

public func IsAmbienceController() { return true; }

public func Definition(def)
{
	// Base environment
	def.Environment = {
		actions = [],
		min_change_delays = [1, 1],
		AddSound = def.Env_AddSound,
		AddAction = def.Env_AddAction,
	};
	// Base sound modifier
	SoundModifier = {
		Release = Ambience.ReleaseSoundModifier,
		Update = Ambience.UpdateSoundModifier,
	};
	// Modifiers for different ambiences
	CaveModifier = new SoundModifier {
		Type = C4SMT_Reverb,
		Reverb_Late_Reverb_Gain = 300,
		Reverb_Late_Reverb_Delay = 10,
		Reverb_Decay_HFRatio = 800,
	};
	UnderwaterModifier = nil; // not supported yet
	return true;
}

local Environment;

local Name = "$Name$";
local Description = "$Description$";




/**
	Ambience Objects
	Cares about the placement of purely visual objects and handles sound ambience
	The placement uses categories and thus is forward-compatible.
*/

// this proplist defines the selectable environment objects
// "ID" might be nil or a valid id
// "includes" specifies what objects are created when the selected object is created (no specific order)
// any entry of "Environment_Attributes" might be nil or false instead of a proplist
// nil will log a warning on creating that object and false will silently ignore it
// thus something like Environment_Attributes["Foobar"] = false; will work to disable certain features
static const Environment_Attributes =
{
	All = {
		ID = nil,
		includes = ["Temperate", "Desert"],
	},
	
	Temperate = {
		ID = nil,
		includes = ["Zicadas", "Frogs", "BackgroundBirds"],
	},
	
	Desert = {
		ID = nil,
		includes = ["Zicadas"],
	},
	
	Zicadas = {
		ID = Ambience_Zicadas,
	},
	
	Frogs = {
		ID = nil /* not yet implemented: Environment_Frogs */,
	},
	
	BackgroundBirds = {
		ID = nil /* not yet implemented: Environment_BackgroundBirds */,
	},
};


// provides a simple interface for creation of environment objects and decoration with standard values
// the objects are placed on a best-effort-basis. That means f.e. objects that rely on water will not be placed when there is no water in the landscape.
global func CreateEnvironmentObjects(
	what /* array of strings or single string: what objects will be created, standard: "All" */
	, proplist area /* area where objects will be created, format {x = ??, y = ??, w = ??, h = ??}, standard: whole landscape */
	, int amount_percentage /* what percentage of the standard amount will be created, standard: 100 */
	)
{
/*
// half desert, half temperate - but birds everywhere
CreateEnvironmentObjects(["Desert", "BackgroundBirds"], Rectangle(0, 0, LandscapeWidth()/2, LandscapeHeight()));
CreateEnvironmentObjects("Temperate", Rectangle(LandscapeWidth()/2, 0, LandscapeWidth()/2, LandscapeHeight()));
*/
	what = what ?? "All";
	area = area ?? Shape->LandscapeRectangle();
	amount_percentage = amount_percentage ?? 100;
	
	// might be a string to allow CreateEnvironmentObjects("All")
	if(GetType(what) != C4V_Array)
		what = [what];
	
	// iteratively find all the objects that are included in the selection
	while(true)
	{
		var changed = false;
		var to_add = [];
		
		// go through every object in the list
		for(var obj in what)
		{
			var p = Environment_Attributes[obj];
			if(p == nil) {Log("Warning: Environment object %s does not exist!", obj);}
			else if(p == false) continue; // disabled by the scenario designer
			
			// add all objects included to the temporary list if existing
			if(!p["includes"]) continue;
			to_add = Concatenate(to_add, p["includes"]);
		}
		
		// add every unique item from the temporary list to the object list
		for(var obj in to_add)
		{
			if(IsValueInArray(what, obj)) continue;
			
			if(!!Environment_Attributes[obj]["includes"])
				changed = true; // found changes, need further checking
			
			PushBack(what, obj);
		}
		
		if(!changed)
			break;
	}
	
	// now create all the selected objects
	for(var obj in what)
	{
		var p, p_id;
		if(!(p = Environment_Attributes[obj])) continue;
		if(!(p_id = p["ID"])) continue;
		
		p_id->Place(amount_percentage, area);
	}
}



/* Global sound ambience object creation */

global func InitializeAmbience()
{
	// Fallback for when this call is not defined in scenario: Ensure there is an ambience controller object
	if (!FindObject(Find_Func("IsAmbienceController")))
	{
		CreateObject(Ambience);
	}
	return true;
}
