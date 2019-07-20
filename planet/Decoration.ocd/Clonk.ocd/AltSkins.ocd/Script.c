/**
	Alternative Skins
	Alternative skins for the clonk (can be used for NPCs).
	
	Use SetAlternativeSkin(string skin_name) with a crew member as caller to set the skin.
	The following skins are available:
	 - Beggar
	 - Carpenter
	 - DarkSkinned
	 - Doctor
	 - Guard
	 - Leather
	 - MaleBlackHair
	 - MaleBrownHair
	 - Sage
	 - Youngster

	@author Marky, Maikel, Sven2
*/

// This is a list of valid skin names and which base skin of the clonk is used.
local skin_names = [["Amazon", CSKIN_Farmer, 0xd0a000],
									["Beggar", 0, 0x4040ff],
                  ["Carpenter", 0, 0xefef40],
                  ["DarkSkinned", 0, 0x906000],
                  ["Doctor", 2, 0xd0d0d0],
                  ["Guard", 0, 0xa0a050],
                  ["Leather", 2, 0xa0a020],
                  ["MaleBlackHair", 0, 0x4040ff],
                  ["MaleBrownHair", 0, 0x2020ff],
                  ["MaleDarkHair", 0, 0x406d99],
                  ["Mime", 2, 0xffffff],
                  ["Ogre", CSKIN_Alchemist, 0x20ff20],
                  ["Sage", 0, 0x813100],
                  ["Youngster", 0, 0xba8e37],
                  ["YoungsterBlond", 0, 0x151366]
                 ];

local alt_skin_mesh_material_list;

local DefinitionPriority = -10; // Load after clonk so it can change the clonk definition

public func Definition(def)
{
	// Make skin function available
	Clonk.SetAlternativeSkin = def.SetAlternativeSkin;
	Clonk.GetAlternativeSkin = def.GetAlternativeSkin;
	Clonk.SaveScenarioObject_AltSkinSave = Clonk.SaveScenarioObject;
	Clonk.SaveScenarioObject = def.SaveScenarioObject;
	// Add skins to editor list
	var n = GetLength(Clonk.EditorProps.skin.Options), i;
	alt_skin_mesh_material_list = [];
	for (var alt_skin in skin_names)
	{
		Clonk.EditorProps.skin.Options[n++] = { Value = alt_skin[0], Name = alt_skin[0] };
		alt_skin_mesh_material_list[i++] = Format("Clonk_%s", alt_skin[0]);
	}
	Clonk.EditorProps.skin.AsyncGet = "GetAlternativeSkin";
	Clonk.EditorProps.skin.Set = "SetAlternativeSkin";
}


// Clonk appends

private func GetAlternativeSkin()
{
	// Overload for GetSkin in the Clonk editor props
	var alt_skin_index = GetIndexOf(Clonk_AltSkins.alt_skin_mesh_material_list, GetMeshMaterial());
	if (alt_skin_index >= 0) return Clonk_AltSkins.skin_names[alt_skin_index][0];
	return this->GetSkin();
}

private func SaveScenarioObject(props, ...)
{
	// Clonk scenario saving: Save alternative skins
	if (!this->~SaveScenarioObject_AltSkinSave(props, ...)) return false;
	var alt_skin = this->~GetAlternativeSkin();
	if (GetType(alt_skin) == C4V_String)
	{
		props->Remove("Portrait");
		props->Remove("Skin");
		props->AddCall("Skin", this, "SetAlternativeSkin", Format("%v", alt_skin));
	}
	return true;
}

private func SetAlternativeSkin(skin_name)
{
	// Overload for SetSkin in the Clonk editor props
	// Numeric argument: Fall through to regular skins
	if (GetType(skin_name) != C4V_String)
	{
		this->SetPortrait(nil); // Reset portrait because it would have been set by the alt skin
		return this->SetSkin(skin_name);
	}
	// Find the given name in the list and update the base skin and color of the clonk.
	var found_name = false;
	for (var name in Clonk_AltSkins.skin_names)
	{
		if (name[0] == skin_name)
		{
			// Update mesh to the one required for the new skin.
			this->SetSkin(name[1]);
			// Update color to match the main clothing color found in the skin
			this->SetColor(name[2]);
			found_name = true;
			break;
		}
	}
	// Check if a valid skin name has been given and return error otherwise.
	if (!found_name)
		return FatalError(Format("%s is not a valid skin name for SetAlternativeSkin", skin_name));
		
	// Remove backpack if possible (the clonk has a backpack by default).
	this->~RemoveBackpack();

	// Set the mesh material and update the portrait for dialogues.
	this->SetMeshMaterial(Format("Clonk_%s", skin_name));
	this->SetPortrait({Source = Clonk_AltSkins, Name = Format("Portrait%s", skin_name), Color = this->GetColor()});
	return;
}
