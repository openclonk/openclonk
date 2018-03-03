/**
	Library_Destructible

	Contains the logic for any object/vehicle that is destructible, mainly with explosions.
	But can also be applied to flammable objects.

	Objects will break apart after the damage exceeds local HitPoints (before that OnMaxDamage() is called).
	After half the hitpoints are gone, any mesh model will change it's texture to *_Damaged (same material name).
	OnHalfDamage is called, to e.g. change more submesh textures.
	When repaired over half the amount of  hitpoints, OnHalfDamageReverse is called.

	@author Clonkonaut
*/

// Override!
local HitPoints = 0;
// Don't override!
local lib_destructible_is_damaged = false;

public func Damage(int change, int cause, int by_player)
{
	_inherited(change, cause, by_player, ...);

	// You shouldn't remove the object in Damage when using this library, but better check
	if (!this) return;

	// Damaged
	if (!lib_destructible_is_damaged && GetDamage() >= this.HitPoints / 2)
	{
		// Become damaged
		this->~OnHalfDamage(change, cause, by_player);
		lib_destructible_is_damaged = true;

		if (GetMeshMaterial())
		{
			var mat_name = GetMeshMaterial();
			mat_name = Format("%s_Damaged", mat_name);
			SetMeshMaterial(mat_name);
		}
	}
	// Repaired
	if (lib_destructible_is_damaged && GetDamage() <= this.HitPoints / 2)
	{
		// Become undamaged
		this->~OnHalfDamageReverse(change, cause, by_player);
		lib_destructible_is_damaged = false;

		if (GetMeshMaterial())
		{
			var mat_name = GetMeshMaterial();
			mat_name = TakeString(mat_name, 0, GetLength(mat_name) - 8);
			SetMeshMaterial(mat_name);
		}
	}
	// Destroyed
	if (GetDamage() >= this.HitPoints)
	{
		// Only by explosions? Check!
		if (IsDestroyedByExplosions())
			if (cause != FX_Call_DmgBlast)
				return;

		if (this->~OnDestruction(change, cause, by_player)) // Custom effects
			return;

		// First eject the contents in different directions.
		for (var obj in FindObjects(Find_Container(this)))
		{
			var speed = RandomX(3, 5);
			var angle = Random(360);
			var dx = Cos(angle, speed);
			var dy = Sin(angle, speed);
			// First set the controller, because on exit the object may be removed.
			obj->SetController(by_player);
			obj->Exit(RandomX(-4, 4), RandomX(-4, 4), Random(360), dx, dy, RandomX(-20, 20));
		}

		// Toss around some fragments with particles attached.
		for (var i = 0; i < 6; i++)
		{
			var fragment = CreateObject(DestructibleFragment, RandomX(-4, 4), RandomX(-4, 4), GetOwner());
			var speed = RandomX(40, 60);
			var angle = Random(360);
			var dx = Cos(angle, speed);
			var dy = Sin(angle, speed);
			fragment->SetXDir(dx, 10);
			fragment->SetYDir(dy, 10);
			fragment->SetR(360);
			fragment->SetRDir(RandomX(-20, 20));
			// Set the controller of the fragments to the one causing the blast for kill tracing.
			fragment->SetController(by_player);
			// Incinerate the fragments.
			fragment->Incinerate(100, by_player);
		}

		// Remove
		RemoveObject(true);
	}
}

// Default behaviour: only explosion damage will cause the object to be destroyed
// But any kind of damage will trigger a change of texture etc.
public func IsDestroyedByExplosions() { return true; }


/*-- Interaction Menu --*/

// Always show an interaction menu with at least the damage entry.
public func HasInteractionMenu() { return true; }

public func RejectInteractionMenu(object clonk)
{
	if (GetCon() < 100)
		return Format("$MsgNotFullyConstructed$", GetName());
	return _inherited(clonk, ...);
}

// Show damage in the interaction menu.
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];		
	var damage_menu =
	{
		title = "$Damage$",
		entries_callback = this.GetDamageMenuEntries,
		entries_callback_target = this,
		callback_hover = "OnDamageMenuHover",
		callback_target = this,
		BackgroundColor = RGB(75, 50, 0),
		Priority = 90
	};
	PushBack(menus, damage_menu);
	return menus;
}

// Returns the contents of the "damage" section in the interaction menu.
public func GetDamageMenuEntries()
{
	var is_invincible = this.HitPoints == nil || this->IsInvincible();
	var damage_text = "$Invincible$";
	var color = RGB(0, 150, 0);
	
	if (!is_invincible)
	{
		if (GetDamage() == 0)
			damage_text = "$NotDamaged$";
		else if (GetDamage() < this.HitPoints / 2)
		{
			damage_text = "$SlightlyDamaged$";
			color = RGB(200, 150, 0);
		}
		else
		{
			damage_text = "<c ff0000>$HeavilyDamaged$</c>";
			color = RGB(150, 0, 0);
		}
	}
	
	var menu = 
	{
		Bottom = "2em",
		bar =
		{
			Left = "0.2em",
			Right = "100% - 0.2em",
			bottom = {Top = "50%", Margin = "0.1em", BackgroundColor = RGB(0, 0, 0)},
			top = {Text = damage_text, Style = GUI_TextHCenter}
		}
	};
	// Show hit points.
	var percent = "100%";	
	if (!is_invincible)
		percent = Format("%d%%", 100 * (this.HitPoints - GetDamage()) / this.HitPoints);
	menu.bar.bottom.fill = {BackgroundColor = color, Right = percent, Margin = "0.1em"};
	return [{symbol = this, extra_data = "repair", custom = menu}];
}

// On hovering, show a list of materials that are needed for repairing the structure.
public func OnDamageMenuHover(id symbol, string action, desc_menu_target, menu_id)
{
	var is_invincible = this.HitPoints == nil || this->IsInvincible();
	var damage_text = "$Invincible$";
	if (!is_invincible)
	{
		if (GetDamage() == 0)
			damage_text = "$NotDamaged$";
		else if (GetDamage() < this.HitPoints / 2)
			damage_text = "$SlightlyDamaged$";
		else
			damage_text = "<c ff0000>$HeavilyDamaged$</c>";
	}		
	GuiUpdateText(damage_text, menu_id, 1, desc_menu_target);
}
