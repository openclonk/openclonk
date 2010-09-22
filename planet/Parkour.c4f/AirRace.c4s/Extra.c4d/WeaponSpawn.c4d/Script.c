/*--
	Weapon Spawn
	Author: Maikel

	Spawns weapons for the air plane, pick up on fly through.
--*/

/*-- Checkpoint size --*/
local ws_size;

public func SetSpawnSize(int size)
{
	ws_size = BoundBy(size, 10, 100);
	return;
}

public func GetSpawnSize() { return ws_size; }

/*-- Initialize --*/

local spawn_weapon;

protected func Initialize()
{
	ws_size = 20;
	spawn_weapon = FindSpawnWeapon();
	UpdateGraphics();
	AddEffect("IntWeaponSpawn", this, 100, 1, this);
	return;
}

protected func FxIntWeaponSpawnTimer(object target, int fxnum, int fxtime)
{
	// Check if there is a weapon.
	if (!spawn_weapon && !GetEffect("IntSpawnWait", this))
		FindSpawnWeapon();
	// Check for planes.
	if (!(fxtime % 5))
		CheckForPlanes();	
	// Update graphics.
	UpdateGraphics(fxtime);
	return FX_OK;
}

protected func CheckForPlanes()
{
	// Loop through all clonks inside the checkpoint.
	var plane = FindObject(Find_ID(Plane), Find_Distance(ws_size), Sort_Distance());
	if (!plane)
		return;
	if (!spawn_weapon)
		return;
	
	plane->CreateContents(spawn_weapon);
	spawn_weapon = nil;
	DoGraphics();
	AddEffect("IntSpawnWait", this, 100, 108, this);
	return;
}

protected func FxIntSpawnWaitStop()
{
	return 1;
}

protected func FindSpawnWeapon()
{
	var index = 0, weapon_def, weapon_list = [];
	while (weapon_def = GetDefinition(index))
	{
		if (weapon_def->~IsPlaneWeapon())
			weapon_list[GetLength(weapon_list)] = weapon_def;
		index++;	
	}
	spawn_weapon = weapon_list[Random(GetLength(weapon_list))];
	DoGraphics();
	return;
}

// Mode graphics.
protected func DoGraphics()
{
	// Clear all overlays first.
	SetGraphics(0, 0, 1);
	if (spawn_weapon)
	{
		SetGraphics("", spawn_weapon, 1, GFXOV_MODE_Base);
		SetObjDrawTransform(50 * ws_size, 0, 0, 0, 50 * ws_size, 0, 1);
		//SetClrModulation(RGBa(255, 255, 255, 160) , 1);
	}
	return;
}

// Player graphics.
protected func UpdateGraphics(int time)
{
	// Create two sparks at opposite sides.
	var angle = (time * 10) % 360;
	var color = RGB(255, 255, 255);
	CreateParticle("PSpark", Sin(angle, ws_size), -Cos(angle, ws_size), 0, 0, 32, color);
	angle = (angle + 180) % 360;
	var color = RGB(255, 255, 255);
	CreateParticle("PSpark", Sin(angle, ws_size), -Cos(angle, ws_size), 0, 0, 32, color);
	return;
}

/*-- Proplist --*/
local Name = "$Name$";
