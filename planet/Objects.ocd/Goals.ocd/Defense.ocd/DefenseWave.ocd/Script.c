/**
	Defense Wave
	Defines standard waves which attack the player or its base. The wave is a list of properties, the main ones are:
	 * Name (string)       - The name of the attack wave.
	 * Duration (int)      - Duration of the wave in seconds.
	 * Bounty (int)        - The amount of clunkers received for beating the wave.
	 * Score (int)         - The amount of points obtained for beating this wave.
	 * Enemies (array)     - Array of enemies, see DefenseEnemy for more information.
	
	@author Maikel
 */


/*-- Wave Launching --*/

// Definition call which can be used to launch an attack wave.
public func LaunchWave(proplist prop_wave, int wave_nr, int enemy_plr)
{
	// Create count down until next wave and play wave start sound.
	GUI_Clock->CreateCountdown(prop_wave.Duration);
	CustomMessage(Format("$MsgWave$", wave_nr, prop_wave.Name));
	Sound("UI::Ding");

	// Launch enemies.
	if (prop_wave.Enemies)
		for (var enemy in prop_wave.Enemies)
			DefenseEnemy->LaunchEnemy(enemy, wave_nr, enemy_plr);
	return;
}


/*-- Waves --*/

// Default wave, all other waves inherit from this.
static const DefaultWave =
{
	Name = nil,
	Duration = 0,
	Bounty = nil,
	Score = nil,
	Enemies = nil
};

// A wave with no enemies: either at the start or to allow for a short break.
local Break = new DefaultWave
{
	Name = "$WaveBreak$",
	Duration = 60
};

// A group of rockets who move to a target.
local BoomAttack = new DefaultWave
{
	Name = "$WaveBoomAttack$",
	Duration = 60,
	Bounty = 50,
	Score = 100,
	Enemies = [new DefenseEnemy.BoomAttack {Amount = 10}]
};

// A group of rockets who move to a target.
local RapidBoomAttack = new DefaultWave
{
	Name = "$WaveRapidBoomAttack$",
	Duration = 60,
	Bounty = 50,
	Score = 100,
	Enemies = [new DefenseEnemy.RapidBoomAttack {Amount = 10}]
};

// A group of parachutists from above.
local Ballooner = new DefaultWave
{
	Name = "$WaveBallooner$",
	Duration = 60,
	Bounty = 25,
	Score = 100,
	Enemies = [new DefenseEnemy.Ballooner {Amount = 10}]
};

// A group of rocketeers.
local Rocketeers = new DefaultWave
{
	Name = "$WaveRocketeers$",
	Duration = 60,
	Bounty = 25,
	Score = 100,
	Enemies = [new DefenseEnemy.Rocketeer {Amount = 10}]
};
