// Callback to scenario script, to notify the tutorial guide.

#appendto Clonk

public func CrewSelection(bool unselect)
{
	GameCall("OnClonkSelection");
	return _inherited(unselect, ...);
}
