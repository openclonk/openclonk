/* When door goes down, remove its dummy */
// (Doors have immovable dummies for the AI to punch on)

#appendto StoneDoor

func Destruction(...)
{
	if (this.dummy_target) this.dummy_target->RemoveObject();
	return _inherited(...);
}
