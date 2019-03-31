/*-- Flagpole --*/

#include Library_Ownable
#include Library_Structure
#include Library_Flag
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

public func IsHammerBuildable() { return true; }

public func NoConstructionFlip() { return true; }

// This building is a base.
public func IsBaseBuilding() { return true; }
public func IsBase() { return true; }


/*-- Interaction --*/

// The flag can take valuables which are then auto-sold.
public func IsContainer() { return true; }

// Allow buying only if the rule is active
public func AllowBuyMenuEntries(){ return ObjectCount(Find_ID(Rule_BuyAtFlagpole), Find_AnyLayer());}

public func RejectCollect(id def, object obj)
{
	if (obj->~IsValuable())
		if (!obj->~QueryRejectSell(obj->GetController()))
	 		return _inherited(def, obj, ...);
	return true;
}

public func Collection(object obj)
{
	if (obj->~IsValuable() && !obj->~QueryRejectSell(obj->GetController()))
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
	neutral = to_val;
	// Neutral flagpoles: A bit smaller and different texture. No marker Radius.
	if (neutral)
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


/* Editor */

public func Definition(def, ...)
{
	_inherited(def, ...);
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.neutral = { Name="$Neutral$", EditorHelp="$NeutralHelp$", Set="SetNeutral", Type="bool" };
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local HitPoints = 60;
local FireproofContainer = true;
local neutral = false;
local Components = {Wood = 3, Metal = 1};