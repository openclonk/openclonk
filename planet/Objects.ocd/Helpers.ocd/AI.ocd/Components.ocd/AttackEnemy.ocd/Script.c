/**
	Basic AI component for attacking enemies.

	@author Sven2, Maikel
*/

// Enemy spawn definition depends on this
local DefinitionPriority = 50;

// AI Settings.
local MaxAggroDistance = 200; // Lose sight to target if it is this far away (unless we're ranged - then always guard the range rect).
local GuardRangeX = 300; // Search targets this far away in either direction (searching in rectangle).
local GuardRangeY = 150; // Search targets this far away in either direction (searching in rectangle).

/*-- Public interface --*/


public func SetAllyAlertRange(object clonk, int new_range)
{
	AssertDefinitionContext(Format("SetAllyAlertRange(%v, %d)", clonk, new_range));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	fx_ai.ally_alert_range = new_range;
	return true;
}


// Set callback function name to be called in game script when this AI is first encountered
// Callback function first parameter is (this) AI clonk, second parameter is player clonk.
// The callback should return true to be cleared and not called again. Otherwise, it will be called every time a new target is found.
public func SetEncounterCB(object clonk, string cb_fn)
{
	AssertDefinitionContext(Format("SetEncounterCB(%v, %s)", clonk, cb_fn));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	fx_ai.encounter_cb = cb_fn;
	return true;
}


// Enable/disable auto-searching of targets.
public func SetAutoSearchTarget(object clonk, bool new_auto_search_target)
{
	AssertDefinitionContext(Format("SetAutoSearchTarget(%v, %v)", clonk, new_auto_search_target));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	fx_ai.auto_search_target = new_auto_search_target;
	return true;
}


// Set the guard range to the provided rectangle.
public func SetGuardRange(object clonk, int x, int y, int wdt, int hgt)
{
	AssertDefinitionContext(Format("SetGuardRange(%v, %d, %d, %d, %d)", clonk, x, y, wdt, hgt));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	// Clip to landscape size.
	if (x < 0)
	{
		wdt += x;
		x = 0;
	}
	if (y < 0)
	{
		hgt += y;
		y = 0;
	}
	wdt = Min(wdt, LandscapeWidth() - x);
	hgt = Min(hgt, LandscapeHeight() - y);
	fx_ai.guard_range = {x = x, y = y, wdt = wdt, hgt = hgt};
	return true;
}


// Set the maximum distance the enemy will follow an attacking clonk.
public func SetMaxAggroDistance(object clonk, int max_dist)
{
	AssertDefinitionContext(Format("SetMaxAggroDistance(%v, %d)", clonk, max_dist));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	fx_ai.max_aggro_distance = max_dist;
	return true;
}


/*-- Callbacks --*/

// Callback from the effect Construction()-call
public func OnAddAI(proplist fx_ai)
{
	_inherited(fx_ai);

	// Add AI default settings.
	SetGuardRange(fx_ai.Target, fx_ai.home_x - fx_ai.GuardRangeX, fx_ai.home_y - fx_ai.GuardRangeY, fx_ai.GuardRangeX * 2, fx_ai.GuardRangeY * 2);
	SetMaxAggroDistance(fx_ai.Target, fx_ai.MaxAggroDistance);
	SetAutoSearchTarget(fx_ai.Target, true);	
	
	// Store whether the enemy is controlled by a commander.
	fx_ai.commander = fx_ai.Target.commander;
}


// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);
	
	// Set the additional editor properties
	var additional_props =
	{
		ignore_allies = { Name = "$IgnoreAllies$", Type = "bool" },
		guard_range = { Name = "$GuardRange$", Type = "rect", Storage = "proplist", Color = 0xff00, Relative = false },
		max_aggro_distance = { Name = "$MaxAggroDistance$", Type = "circle", Color = 0x808080 },
		auto_search_target = { Name = "$AutoSearchTarget$", EditorHelp = "$AutoSearchTargetHelp$", Type = "bool" },
	};
	
	AddProperties(def->GetControlEffect().EditorProps, additional_props);
}


// Callback from the effect SaveScen()-call
public func OnSaveScenarioAI(proplist fx_ai, proplist props)
{
	_inherited(fx_ai, props);

	if (fx_ai.ally_alert_range)
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetAllyAlertRange", fx_ai.Target, fx_ai.ally_alert_range);
	props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetGuardRange", fx_ai.Target, fx_ai.guard_range.x, fx_ai.guard_range.y, fx_ai.guard_range.wdt, fx_ai.guard_range.hgt);
	if (fx_ai.max_aggro_distance != fx_ai->GetControl().MaxAggroDistance)
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetMaxAggroDistance", fx_ai.Target, fx_ai.max_aggro_distance);
	if (!fx_ai.auto_search_target)
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetAutoSearchTarget", fx_ai.Target, false);
	if (fx_ai.encounter_cb)
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetEncounterCB", fx_ai.Target, Format("%v", fx_ai.encounter_cb));
}

/*-- Editor Properties --*/


// Gets an evaluator for the editor: Clonks
public func UserAction_EnemyEvaluator()
{
	var enemy_evaluator = UserAction->GetObjectEvaluator("IsClonk", "$Enemy$", "$EnemyHelp$");
	enemy_evaluator.Priority = 100;
	return enemy_evaluator;
}


// Gets an evaluator for the editor: Attack target
public func UserAction_AttackTargetEvaluator()
{
	return UserAction->GetObjectEvaluator("IsClonk", "$AttackTarget$", "$AttackTargetHelp$");
}
