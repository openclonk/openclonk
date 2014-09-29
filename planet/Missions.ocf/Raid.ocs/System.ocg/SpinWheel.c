/* SpinWheel requires a key */

#appendto SpinWheel

public func ControlUp(object clonk, ...)
{
	if (!CheckLock(clonk)) return false;
	return inherited(clonk, ...);
}

public func ControlDown(object clonk)
{
	if (!CheckLock(clonk)) return false;
	return inherited(clonk, ...);
}

func CheckLock(object clonk)
{
	// already unlocked?
	if (this.unlocked) return true;
	// otherwise, find key
	var key = clonk->FindContents(Key);
	if (!key)
	{
		Dialogue->MessageBox("$DoorNoKey$", clonk, clonk);
		clonk->Sound("WipfWhine");
		return false;
	}
	// too far away? (may happen when callback is from key)
	if (ObjectDistance(clonk, this) > 30)
	{
		Dialogue->MessageBox("$DoorTooFar$", clonk, clonk, nil, true);
		return false;
	}
	// unlock
	Dialogue->MessageBox("$DoorUnlocked$", clonk, clonk, nil, true);
	this.unlocked = true;
	key->RemoveObject();
	Sound("Click");
	return true;
}