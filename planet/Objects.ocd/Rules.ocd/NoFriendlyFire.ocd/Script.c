/**
	No Friendly Fire Rule
	If this rule is active non-hostile crew members can't hit each other.
	Some of the implementation is in appendto scripts.
	
	@author Maikel
*/


public func Initialize()
{
	// Don't do anything if this is not the first rule of this type.
	if (ObjectCount(Find_ID(Rule_NoFriendlyFire)) > 1) 
		return;
		
	// Find all crew members and protect them from friendly fire.
	for (var crew in FindObjects(Find_Or(Find_OCF(OCF_CrewMember), Find_Property("HasNoFriendlyFire"))))
		DisableFriendlyFire(crew);
	return;
}

public func Destruction()
{
	// If this is not the last copy of this rule do nothing. 
	if (ObjectCount(Find_ID(Rule_NoFriendlyFire)) > 1)
		return;
	
	// Find all crew members and allow friendly fire.
	for (var crew in FindObjects(Find_Or(Find_OCF(OCF_CrewMember), Find_Property("HasNoFriendlyFire"))))
		EnableFriendlyFire(crew);
	return;
}

public func OnClonkRecruitment(object clonk, int plr)
{
	DisableFriendlyFire(clonk);
	return;
}

public func OnCreationRuleNoFF(object obj)
{
	DisableFriendlyFire(obj);
	return;
}

public func OnClonkDerecruitment(object clonk, int plr)
{
	EnableFriendlyFire(clonk);
	return;
}

public func OnClonkDeath(object clonk, int killed_by)
{
	EnableFriendlyFire(clonk);
	return;
}

public func OnDestructionRuleNoFF(object obj)
{
	EnableFriendlyFire(obj);
	return;
}


/*-- Friendly Fire --*/

public func DisableFriendlyFire(object for_obj)
{
	// Overload several functions to check for friendly fire.
	if (for_obj.QueryCatchBlow != Rule_NoFriendlyFire.NoFF_QueryCatchBlow)
	{
		for_obj.Backup_QueryCatchBlow = for_obj.QueryCatchBlow;
		for_obj.QueryCatchBlow = Rule_NoFriendlyFire.NoFF_QueryCatchBlow;
	}
	if (for_obj.BlastObject != Rule_NoFriendlyFire.NoFF_BlastObject)
	{
		for_obj.Backup_BlastObject = for_obj.BlastObject ?? Global.BlastObject;
		for_obj.BlastObject = Rule_NoFriendlyFire.NoFF_BlastObject;
	}
	if (for_obj.DoShockwaveCheck != Rule_NoFriendlyFire.NoFF_DoShockwaveCheck)
	{
		for_obj.Backup_DoShockwaveCheck = for_obj.DoShockwaveCheck ?? Global.DoShockwaveCheck;
		for_obj.DoShockwaveCheck = Rule_NoFriendlyFire.NoFF_DoShockwaveCheck;
	}
	if (for_obj.IsProjectileTarget != Rule_NoFriendlyFire.NoFF_IsProjectileTarget)
	{
		for_obj.Backup_IsProjectileTarget = for_obj.IsProjectileTarget ?? Global.IsProjectileTarget;
		for_obj.IsProjectileTarget = Rule_NoFriendlyFire.NoFF_IsProjectileTarget;
	}
	return;
}

public func EnableFriendlyFire(object for_obj)
{
	// Stop overloading functions to check for friendly fire.
	if (for_obj.Backup_QueryCatchBlow != nil)
	{
		for_obj.QueryCatchBlow = for_obj.Backup_QueryCatchBlow;
		for_obj.Backup_QueryCatchBlow = nil;
	}
	if (for_obj.Backup_BlastObject != nil)
	{
		for_obj.BlastObject = for_obj.Backup_BlastObject;
		for_obj.Backup_BlastObject = nil;
	}
	if (for_obj.Backup_DoShockwaveCheck != nil)
	{
		for_obj.DoShockwaveCheck = for_obj.Backup_DoShockwaveCheck;
		for_obj.Backup_DoShockwaveCheck = nil;
	}
	if (for_obj.Backup_IsProjectileTarget != nil)
	{
		for_obj.IsProjectileTarget = for_obj.Backup_IsProjectileTarget;
		for_obj.Backup_IsProjectileTarget = nil;
	}
	return;
}

public func NoFF_QueryCatchBlow(object projectile, ...)
{
	var w_controller = projectile->GetController();
	var t_controller = this->GetController();
	if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
		return true;
	if (this.Backup_QueryCatchBlow)
		return this->Backup_QueryCatchBlow(projectile, ...);
	return false;
}

public func NoFF_BlastObject(int level, int caused_by, ...)
{
	var w_controller = caused_by;
	var t_controller = this->GetController();
	if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
		return true;
	return this->Backup_BlastObject(level, caused_by, ...);
}

public func NoFF_DoShockwaveCheck(int x, int y, int caused_by, ...)
{
	var w_controller = caused_by;
	var t_controller = this->GetController();
	if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
		return false;
	return this->Backup_DoShockwaveCheck(x, y, caused_by, ...);
}

public func NoFF_IsProjectileTarget(object projectile, object shooter, ...)
{
	var w_controller = NO_OWNER;
	if (projectile)
		w_controller = projectile->GetController();
	if (shooter && w_controller == NO_OWNER)
		w_controller = shooter->GetController();
	var t_controller = this->GetController();
	if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
		return false;
	return this->Backup_IsProjectileTarget(projectile, shooter, ...);
}


/*-- Description --*/

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1;
