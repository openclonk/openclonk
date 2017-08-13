/**
	Library_Wearable
	Library for all clothing and other things worn on the clonk.
	The library will make sure that not two objects are worn at the same
	position at the same time.
	
	@author: Clonkonaut
*/

/* Bone names of attachment on the clonk and also identifiers */

// Headwear like helmets, caps or similar
static const WEARABLE_Head = "skeleton_head";

local wear_effect;

local display_disabled = false;

/* Overloads */

// These functions must exist in order for this library to work

public func GetWearPlace()
{
	return; // must return one of the WEARABLE_* constants
}

/* Other functions that can be present in the object:

func GetWearBone: return the bone with which it is attached to the clonk (default: "main")
func GetWearTransform(clonk): transformation added when worn

func StayAfterDeath: return true if the item should remain on the clonk after its death (default is that it does not stay)

func OnPutOn(clonk): callback after the item was put on
func OnTakenOff(clonk): callback after the item was taken off

func OnDamage(damage_amount, cause, by_player): Callback whenever the wearer is damaged, parameters are passed on from the effect Damage callback
                                                Return value is returned (useful for protective clothing)

func GetCarryMode: must(!) return CARRY_None whenever display_disabled is true, otherwise display error will likely occur

*/

/* Engine Callbacks */

// It is assumed that a wearable must be contained in the clonk to be worn.
func Departure()
{
	if (IsWorn())
		TakeOff();
	_inherited(...);
}

func Destruction()
{
	if (IsWorn())
		TakeOff();
	_inherited(...);
}

/* Interface */

// The clonk will put on the item and take off any other item currently worn
// in the same place, unless no_force is true in which case false will be returned
// when something else is worn.
public func PutOn(object clonk, bool no_force)
{
	// ???
	if (!clonk->~IsClonk()) return false;

	// Remove all other things before putting on
	if (!no_force)
	{
		var effect;
		for (var i = GetEffectCount("Wearing", clonk); effect = GetEffect("Wearing", clonk, i-1); i--)
			if (effect.identifier == GetWearPlace())
				RemoveEffect(nil, clonk, effect);
	}

	// It is not impossible that the item is currently held in the hand of the clonk.
	// If so, temporarily disable display because the same mesh cannot be attached twice.
	// Any item must adhere this variable in GetCarryMode!
	display_disabled =true;
	clonk->~UpdateAttach();

	wear_effect = clonk->CreateEffect(Wearing, 2, nil, GetWearPlace(), this);

	if (wear_effect == -1) // got rejected
		wear_effect = nil;

	display_disabled = false;
	clonk->~UpdateAttach();

	if (wear_effect)
	{
		// Callback to do whatever
		this->~OnPutOn(clonk);
		return true;
	}

	return false;
}

public func IsWorn()
{
	return wear_effect;
}

public func TakeOff()
{
	if (!wear_effect)
		return false;

	return RemoveEffect(nil, nil, wear_effect);
}

func TakenOff()
{
	wear_effect = nil;
	if (Contained())
		Contained()->~UpdateAttach();
	// Callback to do whatever; note that at this point the item isn't necessary contained.
	this->~OnTakenOff();
}

/* Wearing effect */

local Wearing = new Effect {
	Construction = func(string wearing_identifier, object worn_item) {
		// Save where this thing is worn
		this.identifier = wearing_identifier;
		// Save what is worn
		this.item = worn_item;
	},

	Start = func() {
		// Check if parameters are properly set
		if (this.identifier == nil) return -1;
		if (this.item == nil) return -1;

		var attachment_bone = this.item->~GetWearBone() ?? "main";
		var attachment_transform = this.item->~GetWearTransform(this.Target);
		var attachment_flags = this.item->~GetWearFlags(); // does not need a default value

		this.attach = Target->AttachMesh(this.item, this.identifier, attachment_bone, attachment_transform, attachment_flags);
	},

	Damage = func(int damage, int cause, int by_player) {
		if (!this.item) return damage;

		var ret = this.item->~OnDamage(damage, cause, by_player);
		if (ret == nil)
			ret = damage;
		return ret;
	},

	Effect = func(string new_name, var1) {
		// Reject wearing effects if in the same place
		if (new_name == "Wearing")
			if (var1 == this.identifier)
				return -1;
	},

	Stop = func(int reason) {
		// Items can prevent being removed from the clonk on death
		if (reason == FX_Call_RemoveDeath)
			if (this.item && this.item->~StayAfterDeath(this.Target))
				return -1;

		if (this.Target) this.Target->DetachMesh(this.attach);
		this.attach = nil;
	},

	Destruction = func() {
		if (this.attach != nil && this.Target)
			this.Target->DetachMesh(this.attach);
		if (this.item)
			this.item->TakenOff();
	}
};
