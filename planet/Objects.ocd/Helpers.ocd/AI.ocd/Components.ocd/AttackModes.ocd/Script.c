/**
	AI Attack Modes
	Initializes the AI to have different weapons and attack scripts
	
	@author Sven2
*/


/*-- Callbacks --*/

// Callback from the effect Construction()-call
public func OnAddAI(proplist fx_ai)
{
	_inherited(fx_ai);

	// Add AI default settings.	
	SetAttackMode(fx_ai.Target, "Default"); // also binds inventory
}


// Callback from the effect SaveScen()-call
public func OnSaveScenarioAI(proplist fx_ai, proplist props)
{
	_inherited(fx_ai, props);

	if (fx_ai.attack_mode.Identifier != "Default")
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetAttackMode", fx_ai.Target, Format("%v", fx_ai.attack_mode.Identifier));
}


/*-- Editor Properties --*/

// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);
	
	def->GetControlEffect().SetAttackMode = this.EditorDelegate_SetAttackMode;
	
	// Set the other options
	def->DefinitionAttackModes(def);
}


public func EditorDelegate_SetAttackMode(proplist attack_mode)
{
	// Called by editor delegate when attack mode is changed.
	// For now, attack mode parameter delegates are not supported. Just set by name.
	return this->GetControl()->SetAttackMode(this.Target, attack_mode.Identifier);
}


/*-- Internals --*/

// Set attack mode / spell to control attack behaviour
public func SetAttackMode(object clonk, string attack_mode_identifier)
{
	AssertDefinitionContext(Format("SetAttackMode(%v, %s)", clonk, attack_mode_identifier));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	var attack_mode = this.AttackModes[attack_mode_identifier];
	if (!attack_mode)
		return Log("Attack mode %s does not exist.", attack_mode_identifier);
	// Stop previous attack mode
	if (fx_ai.attack_mode && fx_ai.attack_mode.Destruction) Call(fx_ai.attack_mode.Destruction, fx_ai);
	// Set new
	fx_ai.attack_mode = attack_mode;
	if (fx_ai.attack_mode.Construction) Call(fx_ai.attack_mode.Construction, fx_ai);
	// Remember inventory to be destroyed on death
	this->BindInventory(clonk);
	return true;
}

// Attack mode that just creates a weapon and uses the default attack procedures
local SingleWeaponAttackMode = {
	Construction = func(effect fx)
	{
		// Contents hack: Carry heavy collection while "Walk" plays an annoying animation. Skip that by having a jump animation
		// (the jump animation skipping pickup is weird anyway, but it works for now)
		var is_carry_heavy_workaround = fx.attack_mode.Weapon->~IsCarryHeavy() && !fx.Target->Contained() && fx.Target->GetAction() == "Walk";
		if (is_carry_heavy_workaround) fx.Target->SetAction("Jump");
		var weapon = fx.default_weapon = fx.Target->CreateContents(fx.attack_mode.Weapon), ammo;
		if (is_carry_heavy_workaround) fx.Target->SetAction("Walk");
		if (weapon)
		{
			if (fx.attack_mode.Skin)
			{
				weapon->SetMeshMaterial(fx.attack_mode.Skin, 0);
			}
			if (fx.attack_mode.Ammo)
			{
				ammo = weapon->CreateContents(fx.attack_mode.Ammo);
				if (ammo) ammo->~SetInfiniteStackCount();
			}
			// Do not save in scenario, because it's automatically created through the attack mode setting
			AddEffect("IntNoScenarioSave", weapon, 1);
			// Automatic fadeout+inventory respawn of e.g. firestones
			if (fx.attack_mode.Respawn)
			{
				var respawning_object = ammo ?? weapon;
				respawning_object->~SetStackCount(1); // Ensure departure is called on every object
				respawning_object.WeaponRespawn_Departure = respawning_object.Departure;
				respawning_object.Departure = AI_AttackModes.Departure_WeaponRespawn;
				fx.has_ammo_respawn = true;
			}
		}
	},
	Destruction = func(effect fx)
	{
		var weapon = fx.Target->FindContents(fx.attack_mode.Weapon);
		if (weapon) weapon->RemoveObject();
	},
	FindWeapon = func(effect fx)
	{
		if (!(fx.weapon = fx.Target->FindContents(fx.attack_mode.Weapon))) return false;
		fx.strategy = fx.attack_mode.Strategy;
		if (fx.weapon->~HasExplosionOnImpact())
		{
			fx.can_attack_structures = true;
			fx.can_attack_structures_after_weapon_respawn = fx.attack_mode.Respawn; // allow structure attack even during respawn time
		}
		return true;
	},
	GetName = func()
	{
		var weapon_name = this.SkinName ?? this.Weapon->GetName();
		if (this.Ammo) weapon_name = Format("%s (%s)", weapon_name, this.Ammo->GetName());
		return weapon_name;
	},
	GetEditorHelp = func()
	{
		if (this.Ammo)
			return Format("$AttackWithAmmo$", this.Weapon->GetName(), this.Ammo->GetName());
		else
			return Format("$AttackWith$", this.Weapon->GetName());
	},
	GetInfoString = func(proplist ai_def)
	{
		// Info icon for given attack mode
		if (!this.Weapon) return "";
		if (this.Ammo) return Format("{{%i}}{{%i}}", this.Weapon, this.Ammo); else return Format("{{%i}}", this.Weapon);
	}
};

private func InitAttackModes()
{
	// First-time init of attack mode editor prop structures
	// The attack mode structures are defined in every AI that includes this library
	if (!this.AttackModes) this.AttackModes = {};
	if (!this->GetControlEffect().EditorProps.attack_mode)
	{
		this->GetControlEffect().EditorProps.attack_mode = {
			Name="$AttackMode$",
			EditorHelp="$AttackModeHelp$",
			Type="enum",
			Sorted=true,
			Options=[],
			Set="SetAttackMode"
		};
	}
}

public func RegisterAttackMode(string identifier, proplist am, proplist am_default_values)
{
	// Definition call during Definition()-initialization:
	// Register a new attack mode selectable for the AI clonk
	// Add to attack mode info structure
	if (!this.AttackModes) this->InitAttackModes();
	this.AttackModes[identifier] = am;
	am.Identifier = identifier;
	if (!am_default_values) am_default_values = { Identifier=identifier };
	// Add to editor option for AI effect
	var am_option = {
		Name = am.Name ?? am->GetName(),
		EditorHelp = am.EditorHelp,
		Value = am_default_values
	};
	if (!am_option.EditorHelp && am.GetEditorHelp) am_option.EditorHelp = am->GetEditorHelp();
	var editor_opts = this->GetControlEffect().EditorProps.attack_mode.Options;
	editor_opts[GetLength(editor_opts)] = am_option;
}

private func DefinitionAttackModes(proplist def)
{
	// Make sure attack mode structures are initialized
	this->InitAttackModes();
	// Register presets for all the default weapons usable by the AI
	def->RegisterAttackMode("Default", { Name = "$Default$", EditorHelp = "$DefaultHelp$", FindWeapon = this.FindInventoryWeapon });
	def->RegisterAttackMode("Sword", new SingleWeaponAttackMode { Weapon = Sword, Strategy = this.ExecuteMelee });
	def->RegisterAttackMode("Axe", new SingleWeaponAttackMode { Weapon = Axe, Strategy = this.ExecuteMelee });
	def->RegisterAttackMode("Club", new SingleWeaponAttackMode { Weapon = Club, Strategy = this.ExecuteClub });
	def->RegisterAttackMode("PowderKeg", new SingleWeaponAttackMode { Weapon = PowderKeg, Strategy = this.ExecuteBomber });
	def->RegisterAttackMode("BowArrow", new SingleWeaponAttackMode { Weapon = Bow, Ammo = Arrow, FindWeapon = this.FindInventoryWeaponBow });
	def->RegisterAttackMode("BowFireArrow", new SingleWeaponAttackMode { Weapon = Bow, Ammo = FireArrow, FindWeapon = this.FindInventoryWeaponBow });
	def->RegisterAttackMode("BowBombArrow", new SingleWeaponAttackMode { Weapon = Bow, Ammo = BombArrow, FindWeapon = this.FindInventoryWeaponBow });
	def->RegisterAttackMode("GrenadeLauncher", new SingleWeaponAttackMode { Weapon = GrenadeLauncher, Ammo = IronBomb, Respawn = true, FindWeapon = this.FindInventoryWeaponGrenadeLauncher });
	def->RegisterAttackMode("Blunderbuss", new SingleWeaponAttackMode { Weapon = Blunderbuss, Ammo = LeadBullet, FindWeapon = this.FindInventoryWeaponBlunderbuss });
	def->RegisterAttackMode("Javelin", new SingleWeaponAttackMode { Weapon = Javelin, Respawn = true, FindWeapon = this.FindInventoryWeaponJavelin });
	def->RegisterAttackMode("Rock", new SingleWeaponAttackMode { Weapon = Rock, Respawn = true, Strategy = this.ExecuteThrow });
	def->RegisterAttackMode("Firestone", new SingleWeaponAttackMode { Weapon = Firestone, Respawn = true, Strategy = this.ExecuteThrow });
	def->RegisterAttackMode("Lantern", new SingleWeaponAttackMode { Weapon = Lantern, Respawn = true, Strategy = this.ExecuteThrow });
	return true;
}



/* Attack with respawning weapons */

func Departure_WeaponRespawn(object container, ...)
{
	// Weapon used? Schedule to respawn a new one!
	if (container->GetAlive() || !container->~IsClonk())
	{
		container->ScheduleCall(container, AI_AttackModes.DoWeaponRespawn, 5, 1, GetID());
	}
	// Remove this weapon after a while
	// (This function should be save to be called in foreign context)
	ScheduleCall(this, Rule_ObjectFade.FadeOutObject, 120, 1, this);
	// Revert to previous departure call. No double-respawn in case it gets collected
	this.Departure = this.WeaponRespawn_Departure;
	if (this.Departure)
	{
		return Call(this.Departure, container, ...);
	}
}

func DoWeaponRespawn(id_weapon)
{
	if (GetAlive() || !this->~IsClonk())
	{
		var re_weapon = CreateContents(id_weapon);
		if (re_weapon)
		{
			re_weapon->~SetStackCount(1); // Ensure departure is called on every object
			re_weapon.Departure = AI_AttackModes.Departure_WeaponRespawn;
			// Do not save in scenario, because it's automatically created through the attack mode setting
			AddEffect("IntNoScenarioSave", re_weapon, 1);
		}
		return re_weapon;
	}
}

/*-- Finding weapons --*/

public func FindInventoryWeapon(effect fx)
{
	// Find weapon in inventory, mark it as equipped and set according strategy, etc.
	fx.ammo_check = nil;
	fx.ranged = false;
	if (FindInventoryWeaponGrenadeLauncher(fx)) return true;
	if (FindInventoryWeaponBlunderbuss(fx)) return true;
	if (FindInventoryWeaponBow(fx)) return true;
	if (FindInventoryWeaponJavelin(fx)) return true;
	// Throwing weapons.
	if (FindInventoryWeaponBase(fx, Firestone) || FindInventoryWeaponBase(fx, Rock) || FindInventoryWeaponBase(fx, Lantern))
	{
		fx.can_attack_structures = fx.weapon->~HasExplosionOnImpact();
		fx.strategy = this.ExecuteThrow;
		return true;
	}
	// Melee weapons.
	if (FindInventoryWeaponBase(fx, Sword) || FindInventoryWeaponBase(fx, Axe)) // Sword attacks aren't 100% correct for Axe, but work well enough
	{
		fx.strategy = this.ExecuteMelee;
		return true;
	}
	if (FindInventoryWeaponBase(fx, PowderKeg))
	{
		fx.strategy = this.ExecuteBomber;
		return true;
	}
	if (FindInventoryWeaponBase(fx, Club))
	{
		fx.strategy = this.ExecuteClub;
		return true;
	}
	// No weapon.
	return false;
}

private func FindInventoryWeaponBase(effect fx, id type)
{
	var weapon = fx.Target->FindContents(type);
	if (weapon)
	{
		fx.weapon = weapon;
		return true;
	}
	return false;
}

private func FindInventoryWeaponGrenadeLauncher(effect fx)
{
	var weapon = fx.Target->FindContents(GrenadeLauncher);
	if (weapon && this->HasBombs(fx, weapon))
	{
		fx.weapon = weapon;
		fx.strategy = this.ExecuteRanged;
		fx.projectile_speed = 75;
		fx.ammo_check = this.HasBombs;
		fx.ranged = true;
		fx.can_attack_structures = true;
		return true;
	}
	return false;
}


private func FindInventoryWeaponBlunderbuss(effect fx)
{
	var weapon = fx.Target->FindContents(Blunderbuss);
	if (weapon && this->HasAmmo(fx, weapon))
	{
		fx.weapon = weapon;
		fx.strategy = this.ExecuteRanged;
		fx.projectile_speed = 200;
		fx.ammo_check = this.HasAmmo;
		fx.ranged = true;
		fx.ranged_direct = true;
		return true;
	}
	return false;
}


private func FindInventoryWeaponBow(effect fx)
{
	var weapon = fx.Target->FindContents(Bow);
	if (weapon && this->HasArrows(fx, weapon))
	{
		fx.weapon = weapon;
		fx.strategy = this.ExecuteRanged;
		fx.projectile_speed = 100;
		fx.ammo_check = this.HasArrows;
		fx.ranged = true;
		var arrow = weapon->Contents(0) ?? FindObject(Find_Container(fx.Target), Find_Func("IsArrow"));
		fx.can_attack_structures = arrow && arrow->~IsExplosive();
		return true;
	}
	return false;
}


private func FindInventoryWeaponJavelin(effect fx)
{
	var weapon = fx.Target->FindContents(Javelin);
	if (weapon)
	{
		fx.weapon = weapon;
		fx.strategy = this.ExecuteRanged;
		fx.projectile_speed = fx.Target.ThrowSpeed * weapon.shooting_strength / 100;
		fx.ranged = true;
		return true;
	}
	return false;
}

