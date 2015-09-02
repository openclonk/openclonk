/* Coniferous alternate material */
// High up in the skies, coniferous trees look a little different

#appendto Tree_Coniferous

func Construction(...)
{
	SetMeshMaterial("Coniferous_AltMat");
	return _inherited(...);
}
