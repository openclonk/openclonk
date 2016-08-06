/* Global editor props for all objects */

local Name = "EditorBase";

// Do not create
public func Construction() { RemoveObject(); }

// Basic properties of all objects
local EditorProps = {
	Invincibility = { Type = "has_effect", Effect = "IntInvincible", Set = "SetInvincibility" },
	PlayerColor = { Type = "color", AsyncGet = "GetColor", Set = "SetColor" },
	Name = { Type = "string", AsyncGet = "GetName", Set = "SetName" }
};
local Plane = 1;

local CountedID, IDList, AnyDef, IDSet, PlayerNumber, TeamID, PlayerMask;

local DefinitionPriority=100; // Call this definition early to allow EditorProp initialization
func Definition(def)
{
	// Property delegate types
	CountedID = { Type = "proplist", Display = "{{count}}x{{id}}", DefaultValue = { count=1, id=nil }, Name = "$IDListEntry$", EditorProps = {
		count = { Type = "int", Min = 1 },
		id = { Type = "def" } } };
	IDList = { Name = "ID list", Type = "array", Display = 3, Elements = CountedID };
	AnyDef = { Type = "def" };
	IDSet = { Name = "ID set", Type = "array", Display = 5, Elements = AnyDef };
	PlayerNumber = { Type="int" };
	TeamID = { Type="int" };
	PlayerMask = { Name="$PlayerMask$", Type="enum", OptionKey="Option", Options = [
		{ Name="$None$" },
		{ Name="$All$", Value={ Option="all" } },
		{ Name="$Specific$", Value={ Option="number" }, ValueKey="Data", Delegate=PlayerNumber },
		{ Name="$Team$", Value={ Option="team" }, ValueKey="Data", Delegate=TeamID },
		] };
	return true;
}

// Check if given player is in mask
public func EvaluatePlayerMask(proplist mask, int player)
{
	if (!mask) return false;
	var option = mask.Option;
	if (option == "all") return true;
	if (option == "number") return player == mask.Data;
	if (option == "team") return GetPlayerTeam(player) == mask.Data;
	// Unknown player mask option
	return false;
}

// Evaluate player mask to list of players
public func EvaluatePlayers(proplist mask)
{
	if (!mask) return [];
	var result = [], n=0;
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		if (EvaluatePlayerMask(mask, plr)) result[n++] = plr;
	}
	return result;
}

// Return an ID-List EditorProp with only IDs available that meet the condition
public func GetConditionalIDList(string condition, string name, proplist default_id, string help)
{
	var counted_id = { Type = "proplist", Display = "{{count}}x{{id}}", Name = Format("$Entry$", name), EditorProps = {
		count = { Type = "int", Min = 1 },
		id = { Type = "def", Filter=condition } } };
	return { Name = name, Type = "array", Display = 3, DefaultValue = { count=1, id=default_id }, Elements = counted_id, EditorHelp = help };
}
