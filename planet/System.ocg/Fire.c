/**
	Fire.c
	The fire effect ported from the engine. Functions to be used as documented in the docs.
	
	@author Zapper, Maikel		
*/

static const FIRE_LIGHT_COLOR = 0xc86400;

// fire drawing modes
static const C4Fx_FireMode_Default   = 0; // determine mode by category
static const C4Fx_FireMode_LivingVeg = 2; // C4D_Living and C4D_StaticBack
static const C4Fx_FireMode_StructVeh = 1; // C4D_Structure and C4D_Vehicle
static const C4Fx_FireMode_Object    = 3; // other (C4D_Object and no bit set (magic))
static const C4Fx_FireMode_Last      = 3; // largest valid fire mode


// Returns whether the object is burning.
// documented in /docs/sdk/script/fn
global func OnFire()
{
	if (!this)
		return false;
	var effect;
	if (!(effect=GetEffect("Fire", this)))
		return false;
	return effect.strength;
}

// Extinguishes the calling object with specified strength.
// documented in /docs/sdk/script/fn
global func Extinguish(strength /* strength between 0 and 100 */)
{
	if (!this)
		return false;
	if (strength == nil)
		strength = 100;
	else if (!strength)
		return false;
	
	var effect = GetEffect("Fire", this);
	if (!effect)
		return false;
	effect.strength = BoundBy(effect.strength - strength, 0, 100);
	if (effect.strength == 0)
		RemoveEffect(nil, this, effect);
	return true;
}

// Incinerates the calling object with specified strength.
// documented in /docs/sdk/script/fn
global func Incinerate(
	strength /* strength between 0 and 100 */
	, int caused_by /* the player that caused the incineration */
	, blasted /* whether the object was incinerated by an explosion */
	, incinerating_object /* the object that caused the incineration */)
{
	if (!this)
		return false;
	if (strength == nil)
		strength = 100;
	else if (!strength)
		return false;
	
	var effect = GetEffect("Fire", this);
	if (effect) 
	{
		effect.strength = BoundBy(effect.strength + strength, 0, 100);
		return true;
	}
	else
	{
		effect = AddEffect("Fire", this, 100, 4, this, nil, caused_by, !!blasted, incinerating_object, strength);
		return !!effect;
	}
}

// Called if the object is submerged in incendiary material (for example in lava).
global func OnInIncendiaryMaterial()
{
	this->DoEnergy(-7, false, FX_Call_EngFire, NO_OWNER);
	// The object might have removed itself.
	if (!this) return true;
	return this->Incinerate(15, NO_OWNER);
}

// Makes the calling object non flammable.
global func MakeNonFlammable()
{
	if (!this) 
		return;
	return AddEffect("IntNonFlammable", this, 300);
}

/*-- Fire Effects --*/

global func FxIntNonFlammableEffect(string new_name)
{
	// Block fire effects.
	if (WildcardMatch(new_name, "*Fire*")) 
		return -1;
	// All other effects are okay.
	return 0;
}


global func FxFireStart(object target, proplist effect, int temp, int caused_by, bool blasted, object incinerating_object, strength)
{
	// safety
	if (!target) return -1;
	// temp readd
	if (temp) return 1;
	// fail if already on fire
	//if (target->OnFire()) return -1;
	// Fail if target is dead and has NoBurnDecay, to prevent eternal fires.
	if (target->GetCategory() & C4D_Living && !target->GetAlive() && target.NoBurnDecay)
		return -1;
	
	// structures must eject contents now, because DoCon is not guaranteed to be executed!
	// In extinguishing material
	var fire_caused = true;
	var burn_to = target.BurnTo;
	var mat;
	if (MaterialName(mat = GetMaterial(target->GetX(), target->GetY())))
		if (GetMaterialVal("Extinguisher", "[Material]", mat))
		{
			// blasts should changedef in water, too!
			if (blasted) 
				if (burn_to != nil)
					target->ChangeDef(burn_to);
			// no fire caused
			fire_caused = false;
		}
	// BurnTurnTo
	if (fire_caused) 
		if (burn_to != nil)
			target->ChangeDef(burn_to);
	
	// eject contents
	var obj;
	if (!target->GetDefCoreVal("IncompleteActivity", "DefCore") && !target.NoBurnDecay)
	{
		for (var i = target->ContentsCount(); obj = target->Contents(--i);)
		{
			if (target->Contained()) 
				obj->Enter(target->Contained());
			else
				obj->Exit();
		}		
		// Detach attached objects
		for (obj in FindObjects(Find_ActionTarget(target)))
		{
			if (obj->GetProcedure() == "ATTACH")
				obj->SetAction(nil);
		}
	}
	
	// fire caused?
	if (!fire_caused)
	{
		// if object was blasted but not incinerated (i.e., inside extinguisher), do a script callback
		if (blasted)
			target->~IncinerationEx(caused_by);
		return -1;
	}
	// determine fire appearance
	var fire_mode;
	if (!(fire_mode = target->~GetFireMode()))
	{
		// set default fire modes
		var cat = target->GetCategory();
		if (cat & (C4D_Living | C4D_StaticBack)) // Tiere, Bäume
			fire_mode = C4Fx_FireMode_LivingVeg;
		else if (cat & (C4D_Structure | C4D_Vehicle)) // Gebäude und Fahrzeuge sind unten meist kantig
			fire_mode = C4Fx_FireMode_StructVeh;
		else
			fire_mode = C4Fx_FireMode_Object;
	}
	else if (!Inside(fire_mode, 1, C4Fx_FireMode_Last))
	{
		DebugLog("Warning: FireMode %d of object %s (%i) is invalid!", fire_mode, target->GetName(), target->GetID());
		fire_mode = C4Fx_FireMode_Object;
	}
	// store causes in effect vars
	effect.strength = strength;
	effect.mode = fire_mode;
	effect.caused_by = caused_by; // used in C4Object::GetFireCause and timer! <- fixme?
	effect.blasted = blasted;
	effect.incinerating_obj = incinerating_object;
	effect.no_burn_decay = target.NoBurnDecay;
	
	// store fire particles
	effect.smoke =
	{
		R = 255,
		G = 255,
		B = 255,
		CollisionVertex = 1000,
		OnCollision = PC_Stop(),
		ForceX = PV_Wind(50),
		DampingX = PV_Linear(900,999),
		ForceY = -7,
		DampingY = PV_Linear(1000,PV_Random(650,750)),
		Rotation = PV_Random(0, 359),
		Phase = PV_Random(0, 3)
	};
	effect.chaoticspark =
	{
		Size = PV_Linear(1, 0),
		ForceX = PV_KeyFrames(10, 0, PV_Random(-6, 6), 333, PV_Random(-6, -6), 666, PV_Random(6, 6), 1000, PV_Random(-6, 6)),
		ForceY = PV_KeyFrames(10, 0, PV_Random(-8, 5), 333, PV_Random(-8, 5), 666, PV_Random(-10, 10), 1000, PV_Random(-10, 15)),
		Stretch = PV_Speed(1000, 500),
		Rotation = PV_Direction(),
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		R = 255,
		G = PV_Linear(255,200),
		B = PV_Random(0, 100),
		DampingX=950,
		DampingY=950,
		Alpha = PV_Random(40,140),
		BlitMode = GFX_BLIT_Additive
	};
	effect.redfiredense =
	{
		R = PV_Random(0, 255),
		G = 0,
		B = 0,
		Alpha = 120,
		Rotation = PV_Random(0, 359),
		Phase = PV_Random(0, 3),
		OnCollision = 0
	};
	effect.flameborder =
	{
		R = 255,
		G = PV_Linear(255,0),
		B = 0,
		Phase = PV_Random(0, 4),
		OnCollision = PC_Die(),
		OnCollision = 0,
		Alpha = PV_KeyFrames(0, 0, 0, 100, 80, 600, 80, 1000, 1)
	};
	effect.sharpflame =
	{
		R = 255,
		G = PV_KeyFrames(0, 0, 255, 666, 0, 627, 0),
		B = 0,
		Rotation = PV_Random(-5, 5),
		Phase = PV_Random(0, 5),
		OnCollision = 0,
		BlitMode = GFX_BLIT_Additive
	};
	
	// Optimization: Only for living beings the flame actually follows the object. That means for all other objects, the flames can be rendered as one batch.
	if (target->GetOCF() & OCF_Alive)
	{
		effect.sharpflame.Attach = ATTACH_MoveRelative;
		effect.flameborder.Attach = ATTACH_MoveRelative;
	}
	
	// Prepare some more values.
	EffectCall(target, effect, "UpdateEffectProperties");
		
	// Reduce checks, smoke and a few other particles if there are other burning objects around.
	effect.FreqReduction = 1;
	for (var nearby in FindObjects (Find_Distance(effect.height)))
		if (nearby->OnFire())
			++effect.FreqReduction;
	effect.FreqReduction = effect.Interval * BoundBy(effect.FreqReduction / 3, 1, 6);

	// Set values
	if ((4 * effect.width * effect.height) > 500)
		target->Sound("Fire::Inflame", false, 100);
	if (target->GetMass() >= 100)
		if (target->Sound("Fire::Fire", false, 100, nil, 1))
			effect.fire_sound = true;
	
	// callback
	target->~Incineration(effect.caused_by);
	
	// Done, success
	return FX_OK;
}


global func FxFireUpdateEffectProperties(object target, proplist effect)
{
	effect.width = target->GetObjWidth() / 2;
	effect.height = target->GetObjHeight() / 2;
	
	effect.fire_width = effect.width * effect.strength / 100 + 1;
	effect.fire_height = effect.height * effect.strength / 100 + 2;
	
	effect.pspeed = -effect.fire_height/2;
	
	effect.redfiredense.Size = PV_KeyFrames(0, 0, 1, 500, PV_Random(effect.fire_width/10, effect.fire_width/6), 1000, 1);
	
	var smokesize = effect.fire_width * 2;
	if (smokesize > 60) smokesize = 50;
	effect.smoke.Size = PV_Linear(smokesize, smokesize * 5);
	effect.smoke.Alpha = PV_KeyFrames(0, 0, 1, 50, effect.strength, 1000, 0);
	
	var stretch = 500 * effect.fire_height / effect.fire_width;
	effect.maxhgt = effect.fire_height - effect.fire_width * stretch / 1500;
	var sharpbottom = 0;
	if (GetCategory() & C4D_Structure || GetCategory() & C4D_Vehicle) sharpbottom++;
	
	effect.flameborder.Stretch = stretch;
	effect.flameborder.Size = PV_KeyFrames(0, 0, effect.fire_width * (1 + sharpbottom), 500, effect.fire_width * 2, 1000, effect.fire_width);
	effect.border_active = (effect.strength > 49) && (effect.fire_height > 10);
	effect.sharpflame.Size = PV_KeyFrames(0, 0, effect.fire_width * (2 + sharpbottom), 500, effect.fire_width * 3, 1000, effect.fire_width*2);
	effect.sharpflame.Alpha = PV_KeyFrames(0, 0, 0, 350, 190 * effect.strength/100, 700, 190 * effect.strength/100, 1000, 0);
	effect.sharpflame.Stretch = stretch;
}

global func FxFireTimer(object target, proplist effect, int time)
{
	// safety
	if (!target) return FX_Execute_Kill;
	
	// get cause
	//if(!GetPlayerName(effect.caused_by)) effect.caused_by=NO_OWNER;;
		
	// strength changes over time
	if ((effect.strength < 100) && (time % 8 == 0))
	{
		if (effect.strength < 50)
		{
			if (time % 8 == 0)
			{
				effect.strength = Max(effect.strength - 1, 0);
				if (effect.strength <= 0)
			 		return FX_Execute_Kill;
			}
		}
		else
		{
			effect.strength = Min(100, effect.strength + 2);
			
			if (effect.strength > 49)
			{
				effect.smoke.R = 127;
				effect.smoke.G = 127;
				effect.smoke.B = 127;
			}
		}
			
		// update particle properties that depend on the effect strength
		EffectCall(target, effect, "UpdateEffectProperties");
	}
		
	// target is in liquid?
	if (time % (20 + effect.FreqReduction) == 0)
	{	
		// Extinguish when in water-like materials.
		var mat;
		if (mat = GetMaterial())
		{
			if (GetMaterialVal("Extinguisher", "Material", mat))
			{
				var steam =
				{
					Size = PV_Linear(effect.width*2, effect.width*4),
					Alpha = PV_Linear(87,0),
					R = 255,
					G = 255,
					B = 255,
					Rotation = PV_Random(0, 359)
				};
				CreateParticle("Dust", 0, -5, 0, 0, 180, steam, 2);
				return FX_Execute_Kill;
			}
		}
		
		// Incinerate landscape if possible.
		target->IncinerateLandscape(0, 0, effect.caused_by);
	
		// check spreading of fire
		if (effect.strength > 10)
		{
			var container = target->Contained(), inc_objs;
			// If contained do not incinerate anything. This would mostly affect inventory items and burns the Clonk too easily while lacking feedback.
			if (container)
			{
				inc_objs = [];
			}
			// Or if not contained incinerate all objects outside the burning object.
			// Do not incinerate inventory objects, since this, too, would mostly affect Clonks and losing one's bow in a fight without noticing it is nothing but annoying to the player.
			else
			{
				inc_objs = FindObjects(Find_AtRect(-effect.width/2, -effect.height/2, effect.width, effect.height), Find_Exclude(target), Find_Layer(target->GetObjectLayer()), Find_NoContainer());
			}
			// Loop through the selected set of objects and check contact incinerate.
			for (var obj in inc_objs)
			{
				// Check if the obj still exists, an object could be have been removed in this loop.
				if (!obj)
					continue;
				
				if (obj->GetCategory() & C4D_Living)
					if (!obj->GetAlive()) 
						continue;
				
				var inf = obj.ContactIncinerate;
				if (!inf)
					continue;
				var amount = (effect.strength / 3) / Max(1, inf);
	
				if (!amount)
					continue;
					
				var old_fire_value = obj->OnFire();
				if(old_fire_value < 100) // only if the object was not already fully ablaze
				{
					// incinerate the other object a bit
					obj->Incinerate(Max(10, amount), effect.caused_by, false, effect.incinerating_obj);
				
					// Incinerating other objects weakens the own fire.
					// This is done in order to make fires spread slower especially as a chain reaction.
					var min = Min(10, effect.strength);
					if(effect.strength > 50) min = 50;
					effect.strength = BoundBy(effect.strength - amount/2, min, 100);
				}
			}
		}
	}
	
	// causes on object
	//target->ExecFire(effect_number, caused_by);
	if (target->GetAlive())
	{
		target->DoEnergy(-effect.strength*4, true, FX_Call_EngFire, effect.caused_by); 
	}
	else 
	{
		if ((time*10) % 120 <= effect.strength)
		{
			target->DoDamage(2, FX_Call_DmgFire, effect.caused_by);
			if (target && !Random(2) && !effect.no_burn_decay)
			{
				target->DoCon(-1);
				if (target)
					EffectCall(target, effect, "UpdateEffectProperties");
			}
		} 
	}
	if (!target) 
		return FX_Execute_Kill;
	if (target->Contained()) 
		return FX_OK;

	// Fire particles
	// Fire denses, smoke, chaotic sparks
	if (time % effect.FreqReduction == 0)
	{
		// A few red fire particles popping up in the higher area. Does not affect very small flames.
		if (effect.fire_width + effect.fire_height > 40)
		{
			if (!Random(2)) CreateParticle("FireDense", PV_Random(-effect.fire_width, effect.pspeed), PV_Random(effect.pspeed, effect.fire_height), PV_Random(-3,3), effect.pspeed, effect.fire_height/2+6, effect.redfiredense);
			if (!Random(2)) CreateParticle("FireDense", PV_Random(effect.fire_width/2, effect.fire_width), PV_Random(effect.pspeed, effect.fire_height), PV_Random(-3,3), effect.pspeed, effect.fire_height/2+6, effect.redfiredense);
		}

		// Smoke
		CreateParticle("SmokeThick", PV_Random(-effect.fire_width,effect.fire_width), -effect.fire_height, 0, -6, 300, effect.smoke);

		// Chaotic particles
		if (!Random(3) && effect.FreqReduction < 14)
			CreateParticle("Magic", PV_Random(-effect.fire_width, effect.fire_width), PV_Random(-effect.fire_height, effect.fire_height), PV_Random(25, -25), PV_Random(-25, 12), 50, effect.chaoticspark);
		
		// Flameborders and sharp flames

		// Flame borders
		if (effect.border_active)
		{
			CreateParticle("FireBorder", 0, effect.maxhgt, 0, effect.pspeed, 30, effect.flameborder);
		}
		
		// Sharp flames
		CreateParticle("FireSharp", 0, effect.maxhgt, 0, effect.pspeed, 30, effect.sharpflame);
	}

	return FX_OK;
}

global func FxFireStop(object target, proplist effect, int reason, bool temp)
{
	// safety
	if (!target) 
		return false;
	// only if real removal is done
	if (temp)
	{
		return true;
	}
	// stop sound
	if (effect.fire_sound)
		target->Sound("Fire::Fire", false, 100, nil, -1);
	// callback
	target->~Extinguishing();
	// done, success
	return true;
}

global func FxFireInfo(object target,  effect)
{
	return Format("Is on %d%% fire.", effect.strength); // todo: <--
}
