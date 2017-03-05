/**
	AI Attack Modes
	Initializes the AI to have different weapons and attack scripts
	
	@author Sven2
*/

// Set attack mode / spell to control attack behaviour
public func SetAttackMode(object clonk, string attack_mode_identifier)
{
	if (GetType(this) != C4V_Def)
		Log("WARNING: SetAttackMode(%v, %s) not called from definition context but from %v", clonk, attack_mode_identifier, this);
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

local AttackModes = {}; // empty pre-init to force proplist ownership in base AI

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
				respawning_object.Departure = AI.Departure_WeaponRespawn;
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
	// All attack modes structures point to the base AI
	this.AttackModes = AI.AttackModes;
	if (!AI.FxAI.EditorProps.attack_mode)
	{
		AI.FxAI.EditorProps.attack_mode = {
			Name="$AttackMode$",
			EditorHelp="$AttackModeHelp$",
			Type="enum",
			Sorted=true,
			Options=[],
			Set="SetAttackMode"
		};
	}
	this.FxAI.EditorProps.attack_mode = AI.FxAI.EditorProps.attack_mode;
}

public func RegisterAttackMode(string identifier, proplist am, proplist am_default_values)
{
	// Definition call during Definition()-initialization:
	// Register a new attack mode selectable for the AI clonk
	// Add to attack mode info structure
	if (!AttackModes) this->InitAttackModes();
	AttackModes[identifier] = am;
	am.Identifier = identifier;
	if (!am_default_values) am_default_values = { Identifier=identifier };
	// Add to editor option for AI effect
	var am_option = {
		Name = am.Name ?? am->GetName(),
		EditorHelp = am.EditorHelp,
		Value = am_default_values
	};
	if (!am_option.EditorHelp && am.GetEditorHelp) am_option.EditorHelp = am->GetEditorHelp();
	var editor_opts = this.FxAI.EditorProps.attack_mode.Options;
	editor_opts[GetLength(editor_opts)] = am_option;
}

private func DefinitionAttackModes(proplist def)
{
	// Make sure attack mode structures are initialized
	this->InitAttackModes();
	// Registration only once for base AI
	if (this != AI) return;
	// Register presets for all the default weapons usable by the AI
	def->RegisterAttackMode("Default", { Name = "$Default$", EditorHelp = "$DefaultHelp$", FindWeapon = AI.FindInventoryWeapon });
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
	// Revert to previou sdeparture call. No double-respawn in case it gets collected
	if ((this.Departure = this.WeaponRespawn_Departure))
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
