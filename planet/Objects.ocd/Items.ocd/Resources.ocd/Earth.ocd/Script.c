/*--- Earth ---*/

// The bucket uses stacked earth-objects.
#include Library_Stackable

protected func Construction()
{
	var graphic = Random(5);
	if (graphic)
		SetGraphics(Format("%d",graphic));
	_inherited(...);
}

protected func Hit()
{
	
	CastPXS("Earth", GetMaterialAmount() * GetStackCount(), 18);
	Sound("Hits::GeneralHit?");
	RemoveObject();
	return 1;
}

func Entrance(object into)
{
	// The stackable library has cared stacking into existing stacks.
	// Look for new ones..
	// If we reach this point, no existing earth-bucket is carried by the Clonk.
	var empty_bucket = FindObject(Find_Container(into), Find_Func("IsBucket"), Find_Func("IsBucketEmpty"));
	if (empty_bucket) Enter(empty_bucket);
	return _inherited(into, ...);
}

func RejectEntrance(object into)
{
	// The stackable library will care about stacking into existing slots.
	var handled = _inherited(into, ...);
	if (handled) return true;
	// Otherwise the container has to care about this.
	return false;
}

// Only X earth objects fit in one bucket.
public func MaxStackCount() { return 5; }
public func InitialStackCount() { return 1;}
// Can only be collected with a bucket! The Clonk will put this into a bucket or directly remove it when digging.
public func IsBucketMaterial() { return true; }
// When using the bucket, you will create this material.
public func GetMaterialName() { return "Earth"; }
public func GetMaterialAmount() { return GetMaterialVal("Dig2ObjectRatio", "Material", Material(GetMaterialName()));}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 450;