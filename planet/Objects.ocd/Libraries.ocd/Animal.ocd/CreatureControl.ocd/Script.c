/**
	AnimalControl
	Cares about the ownership of non-player-controlled units.
	They are hostile to every other player.
	
	global func GetCreaturePlayer()
	global func SetAnimalControlled()
*/

static CreatureControl_animal_player;
static CreatureControl_yet_to_set;
static CreatureControl_initializing;

private func Enqueue(obj)
{
	if(GetType(CreatureControl_yet_to_set) != C4V_Array) 
		CreatureControl_yet_to_set = [];
	PushBack(CreatureControl_yet_to_set, obj);
}

/**
	Returns the hostile NPC player or creates it.
	Returns nil when the NPC player is currently joining.
*/
global func GetCreaturePlayer()
{
	if(CreatureControl_animal_player != nil)
		return CreatureControl_animal_player;
	if(CreatureControl_initializing == true)
		return nil;
		
	CreatureControl_initializing = true;
	CreateScriptPlayer("Creatures", RGB(50, 100, 50), 0, 	CSPF_NoScenarioInit | CSPF_NoEliminationCheck | CSPF_Invisible, Library_CreatureControl); 
	return nil;
}

/**
	Sets the owner of an object to the hostile NPC player.
*/
global func SetCreatureControlled()
{
	if(!this) return false;
	var o = GetCreaturePlayer();
	if(o != nil) return SetOwner(o);
	
	// No owner during creation. If the scripter overwrites it to a real owner, it's not changed later.
	SetOwner(NO_OWNER);
	Library_CreatureControl->Enqueue(this);
}

public func InitializeScriptPlayer(plr, team)
{
	CreatureControl_animal_player = plr;
	
	if(CreatureControl_yet_to_set != nil)
	{
		for(var obj in CreatureControl_yet_to_set)
		{
			if (!obj) continue;
			
			// Overwritten by the scripter?
			if (obj->GetOwner() != NO_OWNER) continue;
			obj->SetOwner(CreatureControl_animal_player);
		}
	}
	CreatureControl_yet_to_set = nil;
	
	// hostile!
	for(var i = 0; i < GetPlayerCount(); ++i)
	{
		var p = GetPlayerByIndex(i);
		if(p == CreatureControl_animal_player) continue;
		SetHostility(p, plr, true, true, true);
		SetHostility(plr, p, true, true, true);
	}
}