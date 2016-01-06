/**
	Alternative Skins
	Alternative skins for the clonk (can be used for NPCs).
	
	Use SetAlternativeSkin(string skin_name) with a crew member as caller to set the skin.
	The following skins are available:
	 - Beggar
	 - Carpenter
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
	var skin_names = [["Beggar", 0], ["Carpenter", 0], ["Doctor", 2], ["Guard", 0], ["Leather", 2], ["MaleBlackHair", 0], ["MaleBrownHair", 0], ["Sage", 0], ["Youngster", 0]];
	// Find the given name in the list and update the base skin of the clonk.
	var found_name = false;
	for (var name in skin_names)
	{
		if (name[0] == skin_name)
		{
			// Update base skin before setting new skin.
			this->SetSkin(name[1]);
			found_name = true;
			break;
		}
	}
	// Check if a valid skin name has been given and return error otherwise.
	if (!found_name)
		return FatalError(Format("%s is not a valid skin name for SetAlternativeSkin", skin_name));

	// Set the mesh material and update the portrait for dialogues.
	this->SetMeshMaterial(Format("Clonk_%s", skin_name));
	this->SetPortrait({Source = Clonk_AltSkins, Name = Format("Portrait%s", skin_name), Color = this->GetColor()});
	return;
}