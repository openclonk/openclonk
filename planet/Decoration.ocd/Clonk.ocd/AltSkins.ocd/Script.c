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

	@author Marky, Maikel	
*/


global func SetAlternativeSkin(string skin_name)
{
	if (GetType(this) != C4V_C4Object || !(this->GetOCF() & OCF_CrewMember))
		return FatalError(Format("SetAlternativeSkin must be called from crew member context (instead called from %i)", this->GetID()));
		
	// This is a list of valid skin names and which base skin of the clonk is used.
	var skin_names = [["Beggar", 0, 0x4040ff],
	                  ["Carpenter", 0, 0xefef40],
	                  ["DarkSkinned", 0, 0x906000],
	                  ["Doctor", 2, 0xd0d0d0],
	                  ["Guard", 0, 0xa0a050],
	                  ["Leather", 2, 0xa0a020],
	                  ["MaleBlackHair", 0, 0x4040ff],
	                  ["MaleBrownHair", 0, 0x2020ff],
	               	  ["MaleDarkHair", 0, 0x406d99],
	                  ["Sage", 0, 0x813100],
	                  ["Youngster", 0, 0xba8e37],
	                  ["YoungsterBlond", 0, 0x151366]
	                 ];
	// Find the given name in the list and update the base skin and color of the clonk.
	var found_name = false;
	for (var name in skin_names)
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