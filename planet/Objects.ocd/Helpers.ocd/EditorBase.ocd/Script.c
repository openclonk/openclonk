/* Global editor props for all objects */

// Do not create
public func Construction() { RemoveObject(); }

local EditorProp_Invincibility = { Type = "has_effect", Effect = "IntInvincible", Set = "SetInvincibility" };
local EditorProp_PlayerColor = { Type = "color", AsyncGet = "GetColor", Set = "SetColor" };
local Plane = 1;
