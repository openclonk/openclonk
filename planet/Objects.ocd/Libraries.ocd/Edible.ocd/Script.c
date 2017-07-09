/**
	Library_Edible

	Contains the logic for an object that can be eaten.

	@author Marky
	
	Objects using this library should call the following code when a user tries to eat it:
		Feed(user);
*/


// When the clonk is able to use the item
public func RejectUse(object clonk)
{
	return _inherited(clonk) || !clonk->IsWalking();
}


// Usage
public func ControlUse(object clonk, int x, int y)
{
	if (!_inherited(clonk, x, y, ...))
	{
		Feed(clonk);
	}
	return true;
}


// Call this when you want a user to eat the object.
public func Feed(object clonk)
{
	if (this->CanFeed(clonk))
	{
		clonk->Eat(this);
	}
	else
	{
		clonk->~PlaySoundDoubt();
	}
}


// Decides whether a user can eat this object. 
public func CanFeed(object clonk)
{
	return !(clonk->HasMaxEnergy()) && this->NutritionalValue() != nil; /// Only if the user is not at full energy and the item gives back energy
}


// The clonk gets this much health back (what actually happens with this value is governed by the implementation of Eat())
public func NutritionalValue()
{
	return 5;
}
