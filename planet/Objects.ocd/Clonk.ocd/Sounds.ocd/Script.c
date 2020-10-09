/*--
	Default sounds of the Clonk, also requires the Clonk_Skins.
	
	These are encapsulated here in case sound names change.
--*/


public func PlaySoundConfirm(...)
{
	if (this->~GetSoundSkinName() != "Farmer")
		this->~PlaySkinSound("Confirm*", ...);
}
public func PlaySoundDecline(...)
{
	if (this->~GetSoundSkinName() != "Farmer")
		this->~PlaySkinSound("Decline*", ...);
}
// Doubtful sound, e.g. when trying a clearly impossible action.
public func PlaySoundDoubt(...)
{
	if (this->~GetSoundSkinName() != "Farmer")
		this->~PlaySkinSound("Doubt*", ...);
}

public func PlaySoundHurt(...) { this->~PlaySkinSound("Hurt*", ...); }
// Sound that is supposed to be funny in situations where the Clonk maybe did something "evil" like killing a teammate.
public func PlaySoundTaunt(...)
{
	if (this->~GetSoundSkinName() == "Alchemist")
		this->~PlaySkinSound("EvilConfirm*", ...);
	else if (this->~GetSoundSkinName() == "Steampunk")
		this->~PlaySkinSound("Laughter*", ...);
}
// Surprised sounds, e.g. when catching fire.
public func PlaySoundShock(...)
{
	if (this->~GetSoundSkinName() == "Steampunk" || this->~GetSoundSkinName() == "Adventurer")
		this->~PlaySkinSound("Shock*", ...);
}
public func PlaySoundScream() { this->~PlaySkinSound("Scream*"); }
// General idle sounds, played when also playing an idle animation.
public func PlaySoundIdle(...)
{
	if (this->~GetSoundSkinName() == "Steampunk")
		this->~PlaySkinSound("Singing*", ...);
}

public func PlaySoundGrab()
{
	Sound("Clonk::Action::Grab");
}

public func PlaySoundUnGrab()
{
	Sound("Clonk::Action::UnGrab");
}

public func PlaySoundDeepBreath()
{
	Sound("Clonk::Action::Breathing");
}

public func PlaySoundEat()
{
	Sound("Clonk::Action::Munch?");
}

public func PlaySoundStepHard(int material)
{
	Sound("Clonk::Movement::StepHard?");
}

public func PlaySoundStepSoft(int material)
{
	Sound("Clonk::Movement::StepSoft?");
}

public func PlaySoundRustle()
{
	Sound("Clonk::Movement::Rustle?");
}

public func PlaySoundKneel()
{
	Sound("Clonk::Movement::RustleLand");
}

public func PlaySoundRoll()
{
	Sound("Clonk::Movement::Roll");
}

