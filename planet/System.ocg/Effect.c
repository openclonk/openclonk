/**
	Effect.c
	Prototype for effect prototypes and useful effects.
	
	@author Günther
*/

static const Effect = new Global
{
	Remove = func(bool no_calls)
	{
		return RemoveEffect(nil, nil, this, no_calls);
	},
	// These properties are set on all effects by the engine.
	// They are declared here so that functions on proplists inheriting from this one can use them easily.
	Name = nil,
	Priority = 0,
	Interval = 0,
	Target = nil,
	Time = 0
};

// This effect blocks all damage.
static const FxBlockDamage = new Effect
{
	Damage = func()
	{
		// Block all damage.
		return 0;
	},
	SaveScen = func(proplist props)
	{
		if (Target)
			props->AddCall("BlockDamage", Target, "CreateEffect", Format("%v", FxBlockDamage), this.Priority);
		return true;
	}
};

// This effect blocks fire.
static const FxBlockFire = new Effect
{
	Effect = func(string new_name)
	{
		// Block fire effects.
		if (WildcardMatch(new_name, "*Fire*"))
			return FX_Effect_Deny;
		// All other effects are okay.
		return FX_OK;
	},
	SaveScen = func(proplist props)
	{
		if (Target)
			props->AddCall("BlockFire", Target, "CreateEffect", Format("%v", FxBlockFire), this.Priority);
		return true;
	}
};