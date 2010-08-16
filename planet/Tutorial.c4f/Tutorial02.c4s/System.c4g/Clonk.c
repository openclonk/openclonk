// Callback to scenario script, to notify the tutorial guide.
// No automatic shovel creation.

#appendto Clonk

public func CrewSelection(bool unselect)
{
	GameCall("OnClonkSelection");
	return _inherited(unselect, ...);
}
