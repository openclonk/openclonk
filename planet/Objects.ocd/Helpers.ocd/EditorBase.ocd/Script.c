/* Global editor props for all objects */

local Name = "EditorBase";

// Do not create
public func Construction() { RemoveObject(); }

// Basic properties of all objects
local EditorProp_Invincibility = { Type = "has_effect", Effect = "IntInvincible", Set = "SetInvincibility" };
local EditorProp_PlayerColor = { Type = "color", AsyncGet = "GetColor", Set = "SetColor" };
local Plane = 1;

local DefinitionPriority=100; // Call this definition early to allow EditorProp initialization
func Definition(def)
{
	// Property delegate types
	def.CountedID = { Type = "proplist", Display = "{{count}}x{{id}}", DefaultValue = { count=1, id=nil }, Elements = {
		Name = "$IDListEntry$",
		EditorProp_count = { Type = "int", Min = 1 },
		EditorProp_id = { Type = "def" } } };
	def.IDList = { Name = "ID list", Type = "array", Display = 3, Elements = def.CountedID };
	def.AnyDef = { Type = "def" };
	def.IDSet = { Name = "ID set", Type = "array", Display = 5, Elements = def.AnyDef };
	def.PlayerNumber = { Type="int" };
	def.TeamID = { Type="int" };
	def.PlayerMask = { Name="$PlayerMask$", Type="enum", OptionKey="Option", Options = [
		{ Name="$None$" },
		{ Name="$All$", Value={ Option="all" } },
		{ Name="$Specific$", Value={ Option="number" }, ValueKey="Data", Delegate=def.PlayerNumber },
		{ Name="$Team$", Value={ Option="team" }, ValueKey="Data", Delegate=def.TeamID },
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

// Return an ID-List EditorProp with only IDs available that meet the condition
public func GetConditionalIDList(string condition, string name, proplist default_id)
{
	var counted_id = { Type = "proplist", Display = "{{count}}x{{id}}", DefaultValue = { count=1, id=default_id }, Elements = {
		Name = Format("$Entry$", name),
		EditorProp_count = { Type = "int", Min = 1 },
		EditorProp_id = { Type = "def", Filter=condition } } };
	return { Name = name, Type = "array", Display = 3, Elements = counted_id };
}

// TODO: Implement a true VIS_Editor in the engine
static const VIS_Editor = VIS_God;

