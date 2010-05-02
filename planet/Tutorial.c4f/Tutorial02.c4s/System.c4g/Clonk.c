// Callback to scenario script.

#appendto Clonk

public func CrewSelection(bool unselect)
{
	GameCall("OnClonkSelection");
	return _inherited(unselect, ...);
}
