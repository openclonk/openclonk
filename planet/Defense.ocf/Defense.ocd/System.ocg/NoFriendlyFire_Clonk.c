/* Disable friendly fire */
// This could also be a general rule

global func NOFF_QueryCatchBlow(object projectile, ...)
{
	// 100% shield if allied
	var w_controller = projectile->GetController();
	var t_controller = GetController();
	if (w_controller >= 0) // NO_OWNER is probably lost controller management (e.g. chain reactions). Always hit.
		if ((t_controller == ENEMY) == (w_controller == ENEMY)) // ENEMY can't hit ENEMY and others can't hit others.
			return true; // reject
	if (this.NOFF_backup_qcb) return this->NOFF_backup_qcb(projectile, ...);
	return false;
}

global func NOFF_BlastObject(int level, int caused_by, ...)
{
	var w_controller = caused_by;
	var t_controller = GetController();
	if (w_controller >= 0) // NO_OWNER is probably lost controller management (e.g. chain reactions). Always hit.
		if ((t_controller == ENEMY) == (w_controller == ENEMY)) // ENEMY can't hit ENEMY and others can't hit others.
			return true; // reject
	return this->NOFF_backup_bo(level, caused_by, ...);
}

global func NOFF_DoShockwaveCheck(int x, int y, int caused_by, ...)
{
	var w_controller = caused_by;
	var t_controller = GetController();
	if (w_controller >= 0) // NO_OWNER is probably lost controller management (e.g. chain reactions). Always hit.
		if ((t_controller == ENEMY) == (w_controller == ENEMY)) // ENEMY can't hit ENEMY and others can't hit others.
			return false; // reject
	return this->NOFF_backup_sw(x, y, caused_by, ...);
}

global func MakeInvincibleToFriendlyFire()
{
	// Overload QueryCatchBlow
	if (this.QueryCatchBlow != Global.NOFF_QueryCatchBlow)
	{
		this.NOFF_backup_qcb = this.QueryCatchBlow;
		this.QueryCatchBlow = Global.NOFF_QueryCatchBlow;
		this.NOFF_backup_sw = this.DoShockwaveCheck ?? Global.DoShockwaveCheck;
		this.DoShockwaveCheck = Global.NOFF_DoShockwaveCheck;
		this.NOFF_backup_bo = this.BlastObject ?? Global.BlastObject;
		this.BlastObject = Global.NOFF_BlastObject;
	}
	return true;
}
