// Also find pilots in other layers.

#appendto Airship

private func HasAirshipPilot()
{
	// Looks for a clonk within the gondola.
	return !!FindObject(Find_ID(Clonk), Find_OCF(OCF_Alive), Find_InRect(gondola[0], gondola[1], gondola[2], gondola[3]), Find_AnyLayer());
}