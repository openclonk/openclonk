/**
	Player start
	Controls start position and modalities
	
	@author Sven2
*/


/* Definition */

local starting_players = { Option="all" };
local starting_knowledge = { Option="all" };
local starting_crew; // const arrays not supported yet
local starting_material;
local starting_wealth = 0;
local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local Plane = 311;

public func Definition(def)
{
	def.starting_crew = GetDefaultCrew();
	def.starting_material = GetDefaultMaterial();
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.starting_players = EditorBase.PlayerMask;
	def.EditorProps.starting_knowledge = { Name="$Knowledge$", Type="enum", OptionKey="Option", Options = [
		{ Name="$None$" },
		{ Name="$All$", Value={ Option="all" } },
		{ Name="$AllExcept$", Value={ Option="allexcept", Data=[] }, ValueKey="Data", Delegate=EditorBase.IDSet },
		{ Name="$Specific$", Value={ Option="idlist", Data=[] }, ValueKey="Data", Delegate=EditorBase.IDSet },
		] };
	def.EditorProps.starting_crew = EditorBase->GetConditionalIDList("IsClonk", "$Crew$", Clonk);
	def.EditorProps.starting_material = EditorBase->GetConditionalIDList("Collectible", "$StartingMaterial$", nil);
	def.EditorProps.starting_wealth = { Name="$Wealth$", Type="int", Min=0 };
	return true;
}

public func GetDefaultCrew() { return [{id=Clonk, count=2}]; }
public func GetDefaultMaterial() { return [{id=Shovel, count=2}, {id=Hammer, count=1}, {id=Axe, count=1}]; }

public func Initialize()
{
	// Re-init default
	starting_crew = GetDefaultCrew();
	starting_material = GetDefaultMaterial();
	return true;
}


/* Interface */

public func SetStartingPlayers(string setting, param)
{
	if (setting)
		starting_players = { Option=setting, Data=param };
	else
		starting_players = nil; // None
	return true;
}

public func SetStartingKnowledge(string setting, param)
{
	if (setting)
		starting_knowledge = { Option=setting, Data=param };
	else
		starting_knowledge = nil; // None
	return true;
}

public func SetStartingCrew(array new_crew)
{
	starting_crew = new_crew;
	return true;
}

public func SetStartingMaterial(array new_material)
{
	starting_material = new_material;
	return true;
}

public func SetStartingWealth(int new_wealth)
{
	starting_wealth = new_wealth;
	return true;
}


/* Player initialization checks */

public func InitializePlayer(int plr, x, y, base, team, script_id)
{
	// Find which one to evaluate
	var possible_startpoints = FindObjects(Find_ID(PlayerStart), Find_Func("IsStartFor", plr));
	var n = GetLength(possible_startpoints);
	if (!n) return false;
	// This callback will be done for every start point (unfortunately)
	// So ensure initialization happens only once
	// (Could speed up things by setting a variable in the other start points to avoid the redundant search. Meh it's just initialization anyway.)
	// Note that this method assumes that starting points are returned in a predictable order
	if (this != possible_startpoints[0]) return false;
	// Pick best starting point: Away from other players, especially enemies
	for (var startpoint in possible_startpoints)
	{
		var other_clonks = startpoint->FindObjects(Find_Distance(50), Find_OCF(OCF_CrewMember));
		var hostile = 0;
		for (var c in other_clonks) if (Hostile(c->GetOwner(), plr)) ++hostile;
		startpoint.penalty = GetLength(other_clonks) + hostile*1000;
	}
	SortArrayByProperty(possible_startpoints, "penalty");
	var n_best = 1, best_penalty = possible_startpoints[0].penalty;
	if (n>1) while (possible_startpoints[n_best].penalty == best_penalty) if (++n_best == n) break;
	// Launch there
	possible_startpoints[Random(n_best)]->DoPlayerStart(plr);
	return true;
}

public func IsStartFor(int plr)
{
	return EditorBase->EvaluatePlayerMask(starting_players , plr);
}


/* Actual player initialization */

public func DoPlayerStart(int plr)
{
	// Player launch controlled by this object!
	// Give wealth
	SetWealth(plr, starting_wealth);
	// Create requested crew
	InitializeCrew(plr);
	// Put contents into crew
	InitializeMaterial(plr);
	// Give knowledge
	InitializeKnowledge(plr);
	return true;
}

private func InitializeCrew(int plr)
{
	// Collect IDs of crew to create
	var requested_crew = [], n=0, i, obj, idx, def;
	for (var idlist_entry in starting_crew)
		for (i=0; i<idlist_entry.count; ++i)
			requested_crew[n++] = idlist_entry.id;
	// Match them to existing crew
	for (i = GetCrewCount()-1; i>=0; --i)
		if (obj = GetCrew(plr, i))
			if ((idx = GetIndexOf(requested_crew, obj->GetID())) >= 0)
			{
				obj->SetPosition(GetX(), GetY() + GetDefHeight()/2 - obj->GetDefHeight()/2);
				requested_crew[idx] = nil;
			}
			else
				obj->RemoveObject(); // not in list: Kill
	// Create any missing crew
	for (def in requested_crew)
		if (def)
			if (obj = CreateObjectAbove(def, 0, GetDefHeight()/2, plr))
				obj->MakeCrewMember(plr);
	// Done!
	return true;
}

private func InitializeMaterial(int plr)
{
	// Spread material across clonks. Try to fill them evenly and avoid giving the same item twice to the same clonk
	// So e.g. each clonk can get one shovel
	for (var idlist_entry in starting_material)
		for (var i=0; i<idlist_entry.count; ++i)
		{
			var best_target = nil, target_score, id = idlist_entry.id, clonk;
			var obj = CreateObjectAbove(id, 0,GetDefHeight()/2, plr);
			if (!obj || !obj.Collectible) continue;
			for (var j=0; j<GetCrewCount(plr); ++j)
				if (clonk = GetCrew(plr, j))
				{
					var clonk_score = 0;
					// High penalty: Already has item of same type
					clonk_score += clonk->ContentsCount(id)*1000;
					// Low penalty: Already has items
					clonk_score += clonk->ContentsCount();
					if (!best_target || clonk_score < target_score)
					{
						best_target = clonk;
						target_score = clonk_score;
					}
				}
			if (best_target) best_target->Collect(obj); // May fail due to contents full
		}
	return true;
}

private func InitializeKnowledge(int plr)
{
	var def;
	if (!starting_knowledge) return true; // No knowledge
	if (starting_knowledge.Option == "all" || starting_knowledge.Option == "allexcept")
	{
		var i=0, exceptlist = [];
		if (starting_knowledge.Option == "allexcept") exceptlist = starting_knowledge.Data;
		while (def = GetDefinition(i++))
			if (!(def->GetCategory() & (C4D_Rule | C4D_Goal | C4D_Environment)))
				if (GetIndexOf(exceptlist, def) == -1)
					SetPlrKnowledge(plr, def);
	}
	else if (starting_knowledge.Option == "idlist")
	{
		for (def in starting_knowledge.Data)
			SetPlrKnowledge(plr, def);
	}
	else
	{
		// Unknown option
		return false;
	}
	return true;
}




/* Scenario saving */

public func SaveScenarioObject(props, ...)
{
	if (!inherited(props, ...)) return false;
	if (!DeepEqual(starting_players, GetID().starting_players))
		if (starting_players)
			props->AddCall("Players", this, "SetStartingPlayers", Format("%v", starting_players.Option), starting_players.Data);
		else
			props->AddCall("Players", this, "SetStartingPlayers", nil);
	if (!DeepEqual(starting_knowledge, GetID().starting_knowledge))
		if (starting_knowledge)
			props->AddCall("Knowledge", this, "SetStartingKnowledge", Format("%v", starting_knowledge.Option), starting_knowledge.Data);
		else
			props->AddCall("Knowledge", this, "SetStartingKnowledge", nil);
	if (!DeepEqual(starting_crew, GetID().starting_crew)) props->AddCall("Crew", this, "SetStartingCrew", starting_crew);
	if (!DeepEqual(starting_material, GetID().starting_material)) props->AddCall("Material", this, "SetStartingMaterial", starting_material);
	if (starting_wealth != GetID().starting_wealth) props->AddCall("Wealth", this, "SetStartingWealth", starting_wealth);
	return true;
}
