/*-- Flagpole --*/

#include Library_Ownable
#include Library_Structure
#include Library_Flag
#include Library_Base // Needed for DoBuy...
#include Library_PowerDisplay
#include Library_Vendor

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		NextAction = "Hold",
	},
};

protected func Initialize()
{
	// The flag is not supposed to be moved ever, ever.
	// But setting the category to StaticBack would mess with other things that rely on recognizing buildings by category.
	// no: SetCategory(C4D_StaticBack);
	// Thus the flag can now fly.
	SetAction("Fly");
	SetComDir(COMD_Stop);
	return _inherited(...);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Translate(0,4000,0));
	return _inherited(...);
}

public func NoConstructionFlip() { return true; }

/*-- Interaction --*/

// The flag can take valuables which are then auto-sold.
public func IsContainer() { return true; }

// Allow buying only if the rule is active
public func AllowBuyMenuEntries(){ return ObjectCount(Find_ID(Rule_BuyAtFlagpole));}

public func RejectCollect(id def, object obj)
{
	if (obj->~IsValuable())
		if (!obj->~QueryOnSell(obj->GetController()))
	 		return _inherited(def, obj, ...);
	return true;
}

public func Collection(object obj)
{
	if (obj->~IsValuable() && !obj->~QueryOnSell(obj->GetController()))
	{
		DoSell(obj, obj->GetController());
	}
	return _inherited(obj, ...);
}

func OnOwnerRemoved(int new_owner)
{
	// Our owner is dead :(
	// Flag is passed on to the next best owner
	SetOwner(new_owner);
	return true;
}

/* Neutral banners - used as respawn points only */

func IsNeutral() { return neutral; }

func SetNeutral(bool to_val)
{
	// Neutral flagpoles: A bit smaller and different texture. No marker Radius.
	if (neutral = to_val)
	{
		SetMeshMaterial("NeutralFlagBanner",0);
		//SetMeshMaterial("NeutralFlagPole",1);
		SetFlagRadius(0);
		this.MeshTransformation = Trans_Mul(Trans_Scale(700,700,700), Trans_Translate(0,6000,0));
		this.Name = "$NameNeutral$";
	}
	else
	{
		SetMeshMaterial("FlagBanner",0);
		//SetMeshMaterial("SettlementFlagPole",1);
		SetFlagRadius(this.DefaultFlagRadius);
		this.MeshTransformation = Trans_Translate(0,4000,0);
		this.Name = this.Prototype.Name;
	}
	return true;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (neutral)
	{
		props->Remove("Radius");
		props->Remove("Color");
		props->Remove("MeshMaterial");
		props->AddCall("Neutral", this, "SetNeutral", true);
	}
	return true;
}



/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local HitPoints = 60;
local neutral = false;
