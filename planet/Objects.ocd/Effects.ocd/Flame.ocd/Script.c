/** 
	Flame
	Spreads fire.
	
	@author Maikel
*/


public func Initialize()
{
	Incinerate(100, GetController());
	AddTimer("Burning", RandomX(24, 26));
	return;
}

public func Burning()
{
	if (!OnFire())
		return RemoveObject();
	// Consume inflammable material and make the flame a little bigger.
	if (FlameConsumeMaterial() && GetCon() <= 80)
	{
		if (!this.NoBurnDecay)
		{
			DoCon(6);	
			SetXDir(RandomX(-8, 8));
		}
	}
	// Split the flame if it is large enough and not too many flames are nearby.
	var amount = ObjectCount(Find_ID(GetID()), Find_Distance(10));	
	if (amount < 5 && GetCon() > 50 && !this.NoBurnDecay && !Random(4))
	{
		var x = Random(15);
		var new_flame = CreateObjectAbove(GetID());
		new_flame->SetSpeed(x, -7);
		new_flame->SetCon(GetCon() / 2);
		SetSpeed(-x, -7);
		SetCon(GetCon() / 2);	
	}
	return;
}

public func DoCon(...)
{
	var res = _inherited(...);
	// Update any existing fire effect, because it does not do it internally when NoBurnDecay is active.
	var fire_fx = GetEffect("Fire", this);
	if (fire_fx)
		EffectCall(this, fire_fx, "UpdateEffectProperties");
	return res;
}

public func SetCon(...)
{
	var res = _inherited(...);
	// Update any existing fire effect, because it does not do it internally when NoBurnDecay is active.
	var fire_fx = GetEffect("Fire", this);
	if (fire_fx)
		EffectCall(this, fire_fx, "UpdateEffectProperties");
	return res;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		return false;
	// Don't incinerate twice in saved scenarios.
	props->Remove("Fire");
	// Store eternal flame type.
	if (this.NoBurnDecay)
		props->AddSet("NoBurnDecay", this, "NoBurnDecay", this.NoBurnDecay);
	return true;
}


/*-- Editor --*/

public func EditorInitialize()
{
	// Assume the flame is eternal when placed in the editor
	this.NoBurnDecay = true;
	return;
}

public func Definition(proplist def)
{
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.NoBurnDecay = { Name = "$EditorEternal$", EditorHelp = "$EditorEternalHelp$", Type = "enum", Options = [{ Name = "$EditorEternalOff$", Value = false }, { Name = "$EditorEternalOn$", Value = true }] };
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 500;
