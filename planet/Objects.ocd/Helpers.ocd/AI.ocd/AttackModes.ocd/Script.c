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

local AttackModes;

// Attack mode that just creates a weapon and uses the default attack procedures
local SingleWeaponAttackMode = {
	Construction = func(effect fx)
	{
		var weapon = fx.Target->CreateContents(fx.attack_mode.Weapon);
		if (weapon)
		{
			if (fx.attack_mode.Ammo)
			{
				var ammo = weapon->CreateContents(fx.attack_mode.Ammo);
				if (ammo) ammo->~SetInfiniteStackCount();
			}
			// Do not save in scenario, because it's automatically created through the attack mode setting
			AddEffect("IntNoScenarioSave", weapon, 1);
			// Automatic fadeout+inventory respawn of e.g. firestones
			if (fx.attack_mode.Respawn)
			{
				weapon->~SetStackCount(1); // Ensure departure is called on every object
				weapon.Departure = AI.Departure_WeaponRespawn;
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
		return true;
	},
	GetName = func()
	{
		if (this.Ammo)
			return Format("%s (%s)", this.Weapon->GetName(), this.Ammo->GetName());
		else
			return this.Weapon->GetName();
	},
	GetEditorHelp = func()
	{
		if (this.Ammo)
			return Format("$AttackWithAmmo$", this.Weapon->GetName(), this.Ammo->GetName());
		else
			return Format("$AttackWith$", this.Weapon->GetName());
	}
};

private func InitAttackModes()
{
	// First-time init of attack mode editor prop structures
	if (!this.AttackModes)
	{
		// All attack modes structures point to the base AI
		if (!AI.AttackModes) AI.AttackModes = {};
		if (this != AI) this.AttackModes = AI.AttackModes;
	}
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
	if (!this.FxAI.EditorProps.attack_mode) this.FxAI.EditorProps.attack_mode = AI.FxAI.EditorProps.attack_mode;
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
	// Registration only once for base AI
	if (this != AI) return;
	// Register presets for all the default weapons usable by the AI
	this->InitAttackModes();
	def->RegisterAttackMode("Default", { Name = "$Default$", EditorHelp = "$DefaultHelp$", FindWeapon = AI.FindInventoryWeapon });
	def->RegisterAttackMode("Sword", new SingleWeaponAttackMode { Weapon = Sword, Strategy = this.ExecuteMelee });
	def->RegisterAttackMode("Club", new SingleWeaponAttackMode { Weapon = Club, Strategy = this.ExecuteMelee });
	def->RegisterAttackMode("Axe", new SingleWeaponAttackMode { Weapon = Axe, Strategy = this.ExecuteMelee });
	def->RegisterAttackMode("BowArrow", new SingleWeaponAttackMode { Weapon = Bow, Ammo = Arrow, FindWeapon = this.FindInventoryWeaponBow });
	def->RegisterAttackMode("BowFireArrow", new SingleWeaponAttackMode { Weapon = Bow, Ammo = FireArrow, FindWeapon = this.FindInventoryWeaponBow });
	def->RegisterAttackMode("BowBombArrow", new SingleWeaponAttackMode { Weapon = Bow, Ammo = BombArrow, FindWeapon = this.FindInventoryWeaponBow });
	def->RegisterAttackMode("GrenadeLauncher", new SingleWeaponAttackMode { Weapon = GrenadeLauncher, Ammo = IronBomb, FindWeapon = this.FindInventoryWeaponGrenadeLauncher });
	def->RegisterAttackMode("Blunderbuss", new SingleWeaponAttackMode { Weapon = Blunderbuss, Ammo = LeadBullet, FindWeapon = this.FindInventoryWeaponBlunderbuss });
	def->RegisterAttackMode("Javelin", new SingleWeaponAttackMode { Weapon = Javelin, Respawn = true, FindWeapon = this.FindInventoryWeaponJavelin });
	def->RegisterAttackMode("Rock", new SingleWeaponAttackMode { Weapon = Rock, Respawn = true, Strategy = this.ExecuteThrow });
	def->RegisterAttackMode("Firestone", new SingleWeaponAttackMode { Weapon = Firestone, Respawn = true, Strategy = this.ExecuteThrow });
	def->RegisterAttackMode("Lantern", new SingleWeaponAttackMode { Weapon = Lantern, Respawn = true, Strategy = this.ExecuteThrow });
	return true;
}



/* Attack with respawning weapons */

func Departure_WeaponRespawn(object container)
{
	// Weapon used? Schedule to respawn a new one!
	if (container->GetAlive() || container->GetID()==Catapult)
	{
		container->ScheduleCall(container, AI_AttackModes.DoWeaponRespawn, 5, 1, GetID());
	}
	// Remove this weapon after a while
	// (This function should be save to be called in foreign context)
	ScheduleCall(this, Rule_ObjectFade.FadeOutObject, 120, 1, this);
	// No double-respawn in case it gets collected
	this.Departure = nil;
}

func DoWeaponRespawn(id_weapon)
{
	if (GetAlive() || GetID()==Catapult)
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
