/* Global editor props for all objects */

local Name = "EditorBase";

// Do not create
public func Construction() { RemoveObject(); }

local EditorProps;
local Plane = 1;

local CountedID, IDList, AnyDef, IDSet, PlayerNumber, TeamID, PlayerMask;
local ItemPlusParameter, ItemPlusParameterOptionMap;
local ItemPlusParameterList;

local DefinitionPriority=100; // Call this definition early to allow EditorProp initialization
func Definition(def)
{
	// Basic properties of all objects
	def.EditorProps = {
		Invincibility = { Name = "$Invincibility$", EditorHelp = "$InVincibilityHelp$", Type = "has_effect", Effect = "IntInvincible", Set = "SetInvincibility" },
		Visibility = { Type = "enum", Name = "$Visibility$", EditorHelp = "$VisibilityHelp$", Options = [ { Name="$Unknown$", Value=-1 }, { Name="$DefaultVisible$" }, { Name="$Visible$", Value=VIS_All }, { Name="$Invisible$", Value=VIS_None }, { Name="$EditorVisible$", Value=VIS_Editor } ] },
		PlayerColor = { Name="$PlayerColor$", EditorHelp="$PlayerColorHelp$", Type = "color", AsyncGet = "GetColor", Set = "SetColor" },
		ClrModulation = { Name="$ClrModulation$", EditorHelp="$ClrModulationHelp$", Type = "color", AsyncGet = "GetClrModulation", Set = "SetClrModulation" },
		BlitMode = { Name="$BlitMode$", EditorHelp="$BlitModeHelp$", Type = "enum", AsyncGet = "GetObjectBlitMode", Set = "SetObjectBlitMode", Options = [
			{ Name="$Unknown$", Value=-1 },
			{ Name="$Default$", Value=0 },
			{ Name="$Additive$", Value=GFX_BLIT_Additive|GFX_BLIT_Custom },
			{ Name="$Mod2$", EditorHelp="$Mod2Help$", Value=GFX_BLIT_Mod2|GFX_BLIT_Custom },
			{ Name="$Wireframe$", EditorHelp="$WireframeHelp$", Value=GFX_BLIT_Wireframe|GFX_BLIT_Custom } ] },
		Name = { Name="$Name$", Type = "string", AsyncGet = "GetName", Set = "SetName" },
		CustomInitializationScript = { Type = "string", Name = "$CustomInitialization$", EditorHelp = "$CustomInitializationHelp$" }
	};
	// Property delegate types; initialize via new EditorBase.<Property> {} to avoid accidental changes to the EditorBase list, as proplists are a reference
	CountedID = { Type = "proplist", Display = "{{count}}x{{id}}", Name = "$IDListEntry$", EditorProps = {
		count = { Type = "int", Min = 1 },
		id = { Type = "def" } } };
	IDList = { Name = "ID list", Type = "array", Display = 3, DefaultValue = { count=1, id=nil }, Elements = CountedID };
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
	// Item plus extra stuff (contents, stack, etc.)
	ItemPlusParameterOptionMap = {};
	ItemPlusParameter = { Name="$Item", Type="enum", Sorted=true, Options = [ { Name="$Nothing$", Priority=50 } ] };
	var itemdef, i = 0, n = 0, option, contents_def, j, n2, contents_defs;
	while ((itemdef = GetDefinition(i++)))
		if (itemdef.Collectible || itemdef->~GetLiquidType())
		{
			var group = itemdef->GetDefinitionGroupPath();
			if (WildcardMatch(group, "Objects/Items/*"))
				group = ReplaceString(group, "Objects/Items/", ""); // Shortcut this group since most items will be here
			else
				group = "$Other$";
			option = { Name=itemdef->GetName(), Group=group, Value=itemdef };
			var def_id = Format("%i", itemdef);
			ItemPlusParameterOptionMap[def_id] = option;
			// Test various kinds of extra parameters for new items
			if (itemdef->~IsLiquidContainer() && itemdef->~GetLiquidContainerMaxFillLevel())
			{
				// Liquid container: Offer to fill with liquid
				j = 0; n2 = 0;
				contents_defs = [{Name="$Nothing$"}];
				while ((contents_def = GetDefinition(j++)))
				{
					var mat = contents_def->~GetLiquidType();
					if (mat)
						if (itemdef->IsLiquidContainerForMaterial(mat))
							contents_defs[++n2] = contents_def;
				}
				if (n2)
				{
					option.Value = { ItemPlusParameter="liquid", ID=itemdef };
					option.OptionKey = "ID";
					option.ValueKey = "Liquid";
					option.Delegate = { Name="$Liquid$", Type="enum", Sorted=true, Options=contents_defs }; // Options resolved later uisng ItemPlusParameterOptionMap
				}
			}
			else if (itemdef.ExtraSlotFilter)
			{
				// Extra slot objects: Offer contents
				j = 0; n2 = 0;
				contents_defs = [{Name="$Nothing$"}];
				while ((contents_def = GetDefinition(j++)))
					if (contents_def[itemdef.ExtraSlotFilter])
						if (contents_def->Call(itemdef.ExtraSlotFilter))
							contents_defs[++n2] = contents_def;
				if (n2)
				{
					option.Value = { ItemPlusParameter="contents", ID=itemdef };
					option.OptionKey = "ID";
					option.ValueKey = "Contents";
					option.Delegate = { Name="$Contents$", Type="enum", Sorted=true, Options=contents_defs }; // Options resolved later uisng ItemPlusParameterOptionMap
				}
			}
			else if (itemdef->~IsStackable())
			{
				// Stackable: Offer stack count
				option.Value = { ItemPlusParameter="stack", ID=itemdef };
				option.OptionKey = "ID";
				option.ValueKey = "StackCount";
				option.Delegate = { Name="$Contents$", Type="enum", Options=[
					{ Name=Format("$DefaultStack$", itemdef->InitialStackCount()) },
					{ Name="$CustomStack$", Type=C4V_Int, Value=itemdef->InitialStackCount(), Delegate={ Type="int", Min=1/*, Max=itemdef->MaxStackCount()*/ } }, // there's no reason to restrict the max stack in editor
					{ Name="$InfiniteStack$", Value="infinite" }
					]};
			}
			// Add to item list if it's an item
			// Do not add liquids; they're just processed here to get the stackable definition
			if (itemdef.Collectible) ItemPlusParameter.Options[++n] = option;
		}
	// Link item contents parameter menus, but ignore group because it's usually a low number of items anyway
	for (option in ItemPlusParameter.Options)
		if (option.ValueKey == "Contents" || option.ValueKey == "Liquid")
			for (i = 1; i < GetLength(option.Delegate.Options); ++i)
			{
				var option_item = ItemPlusParameterOptionMap[Format("%i", option.Delegate.Options[i])] ?? option.Delegate.Options[i];
				if (option_item.Prototype == Global)
					option.Delegate.Options[i] = { Name=option_item->GetName(), Value=option_item }; // Regular definition
				else
					option.Delegate.Options[i] = new option_item { Group=nil }; // Definition with extra parameter
			}
	ItemPlusParameterList = { Name = "$ItemPlusParameterList$", Type = "array", Display = 3, Elements = ItemPlusParameter };
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

// Create item specieid in ItemsPlusParameters delegate
public func CreateItemPlusParameter(proplist param, int x, int y, int owner)
{
	if (!param) return nil;
	var id;
	if (param.ItemPlusParameter) id = param.ID; else id = param;
	var obj = CreateObject(id, x, y, owner);
	return ApplyContentsPlusParameter(param, obj);
}

public func CreateContentsPlusParameter(proplist param, object container)
{
	if (!param || !container) return nil;
	var id;
	if (param.ItemPlusParameter) id = param.ID; else id = param;
	var obj = container->CreateContents(id);
	return ApplyContentsPlusParameter(param, obj);
}

private func ApplyContentsPlusParameter(proplist param, object to_obj)
{
	// Apply object contents or stack count setting
	if (to_obj && param.ItemPlusParameter)
	{
		if (param.ItemPlusParameter == "liquid")
		{
			CreateContentsPlusParameter(param.Liquid, to_obj);
		}
		else if (param.ItemPlusParameter == "contents")
		{
			CreateContentsPlusParameter(param.Contents, to_obj);
		}
		else if (param.ItemPlusParameter == "stack" && GetType(param.StackCount))
		{
			if (param.StackCount == "infinite")
			{
				to_obj->SetInfiniteStackCount();
			}
			else
			{
				to_obj->SetStackCount(param.StackCount);
			}
		}
	}
	return to_obj;
}
