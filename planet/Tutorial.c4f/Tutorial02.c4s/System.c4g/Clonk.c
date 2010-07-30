// Callback to scenario script, to notify the tutorial guide.
// No automatic shovel creation.

#appendto Clonk

public func CrewSelection(bool unselect)
{
	GameCall("OnClonkSelection");
	return _inherited(unselect, ...);
}

protected func Construction()
{
	_inherited(...);
	var shovel = FindObject(Find_ID(Shovel), Find_Container(this));
	if (shovel)
		shovel->RemoveObject();
	return;
}