/* Global editor props for all objects */

// Do not create
public func Construction() { RemoveObject(); }

// Basic properties of all objects
local EditorProp_Invincibility = { Type = "has_effect", Effect = "IntInvincible", Set = "SetInvincibility" };
local EditorProp_PlayerColor = { Type = "color", AsyncGet = "GetColor", Set = "SetColor" };
local Plane = 1;


func Definition(def)
{
	// Property delegate types
	def.CountedID = { Type = "proplist", Display = "{{count}}x{{id}}", DefaultValue = { count=1, id=nil }, Elements = {
		Name = "ID list entry",
		EditorProp_count = { Type = "int", Min = 1 },
		EditorProp_id = { Type = "def" } } };
	def.IDList = { Name = "ID list", Type = "array", Display = 3, Elements = def.CountedID };
	return true;
}
