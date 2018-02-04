/**
	Library_Flammable

	Contains the logic for any object/item that is flammable.
	Flammable objects (items) shouldn't burn like any other object because dealing with
	half burned (con < 100) items is a hassle.

	Implement OnBurnDown() to do this things right when the definition change is about to happen.
	Return true to prevent regular library behaviour.

	@author Clonkonaut
*/

// Obviously, we don't want these items to burn down
local NoBurnDecay = true;
// Items will burn for roughly 2 seconds until changing into a 'burned item' (no longer functional)
// Feel free to change this time in your object (in frames)
local BurnDownTime = 70;

public func Incineration(int caused_by)
{
	_inherited(caused_by);
	// You shouldn't remove the object in Incineration but let's check anyway
	if (!this) return;

	CreateEffect(LibraryFlammable, 1, 1, caused_by); // The effect will not last long, so timer 1 should be fine

	// Notify the clonk (if held) that this object is now burning
	if (Contained())
		Contained()->~OnInventoryChange();
}

public func Extinguishing()
{
	// Notify the clonk (if held) that this object is no longer burning
	if (Contained())
		Contained()->~OnInventoryChange();
}

local LibraryFlammable = new Effect {
	Construction = func (int caused_by)
	{
		this.caused_by = caused_by;
	},
	Timer = func (int time)
	{
		if (!this.Target->OnFire()) // Extinguished! No harm done
			return FX_Execute_Kill;
		if (time >= this.Target.BurnDownTime)
		{
			this.Target->BurnDown(this.caused_by);
			return FX_Execute_Kill;
		}
	}
};

public func BurnDown(int caused_by)
{
	if (this->~OnBurnDown())
		return;
	var burned = CreateObject(BurnedObject, 0, 0, GetOwner());
	var container = Contained();
	// Take over movement and rotation.
	burned->SetXDir(GetXDir(1000), 1000);
	burned->SetYDir(GetYDir(1000), 1000);
	burned->SetR(GetR());
	burned->SetRDir(GetRDir(1000), 1000);
	burned->Incinerate(100, caused_by);
	RemoveObject(true);
	if (container)
		burned->Enter(container);
}

public func GetInventoryIconOverlay() // Display a flame in the inventory bar
{
	if (!OnFire()) return;

	var overlay = 
	{
		Symbol = Icon_Flame
	};
	return overlay;
}