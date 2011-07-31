/*--
		Earthquake
		Author: Maikel
		
		This is the earthquake control object, earthquakes are realized through a global effect.
		Earthquakes can be activated in Scenario.txt under [Environment], Earthquake=x will result
		in a 10*x % earthquake level. You can also just create the control object and modify the
		chance with Get/Set/DoChance. The third option is to directly launch an earthquake with
		LaunchEarthquake(int x, int y, int strength) at the global coordinates (x,y).
--*/


/*-- Disaster Control --*/

protected func Initialize()
{
	// Check for other control objects.
	var ctrl = FindObject(Find_ID(GetID()), Find_Exclude(this));
	if (ctrl)
	{
		ctrl->DoChance(10);
		RemoveObject();
	}
	else
	{
		AddEffect("IntEarthquakeControl", this, 100, 35, this);
		SetChance(10);
	}
	return;
}

public func GetChance()
{
	var effect = GetEffect("IntEarthquakeControl", this);
	return effect.chance;
}

public func SetChance(int chance)
{
	var effect = GetEffect("IntEarthquakeControl", this);
	effect.chance = BoundBy(chance, 0, 100);
	return;
}

public func DoChance(int chance)
{
	SetChance(GetChance() + chance);
	return;
}

protected func FxIntEarthquakeControlTimer(object target, effect, int time)
{
	var chance = effect.chance;
	if (!Random(8))
		if (Random(100) < chance)
			LaunchEarthquake(Random(LandscapeWidth()), Random(LandscapeHeight()), Random(40) + 35);
	return FX_OK;
}

// Launches an earthquake with epicenter (x,y).
global func LaunchEarthquake(int x, int y, int strength)
{
	// Earthquake should start in solid.
	if (!GBackSemiSolid(x, y))
		return false;
	// Minimum strength is 15, maximum strength is 100.
	strength = BoundBy(strength, 15, 100);
	// The earthquake is handled by a global effect.
	var effect = AddEffect("IntEarthquake", 0, 100, 1, nil, Earthquake);
	effect.x = x; // Epicentre x coordinate.
	effect.y = y; // Epicentre y coordinate.
	effect.strength = strength / 3; // Earthquake strength.
	effect.length = 3 * strength / 2; // Earthquake length.
	return true;
}

/*-- Earthquake control --*/

protected func FxIntEarthquakeStart(object target, effect)
{
	// Start sound at quake local coordinates.
	//Sound("Earthquake", true, 100, nil, 1);
	return FX_OK;
}

protected func FxIntEarthquakeStop(object target, effect)
{
	// Stop sound.
	//Sound("Earthquake", true, 100, nil, -1);
	return FX_OK;
}

protected func FxIntEarthquakeTimer(object target, effect, int time)
{
	// Time is up?
	if (time > effect.length)
		return FX_Execute_Kill;
	// Some randomness.
	if (Random(3))
		return FX_OK;
	// Get strength.
	var str = effect.strength;
	// Shake viewport.
	if (!Random(10))
		ShakeViewPort(str, x, y);
	// Get quake coordinates.
	var x = effect.x;
	var y = effect.y;
	var l = 4 * str;
	// Shake ground & objects.
	ShakeFree(x, y, Random(str / 2) + str / 5 + 5);
	for (var obj in FindObjects(Find_NoContainer(), Find_OCF(OCF_Alive), Find_InRect(x - l, y - l, l, l)))
	{
		if (Random(3)) continue;
		if (!obj->GetAction()) continue;
		var act = obj.ActMap[obj->GetAction()];
		if (act.Attach || (act.Procedure
		    && act.Procedure != DFA_FLIGHT
		    && act.Procedure != DFA_LIFT
		    && act.Procedure != DFA_FLOAT
		    && act.Procedure != DFA_ATTACH
		    && act.Procedure != DFA_CONNECT))
			//if (!MatVehicle(obj->Shape.AttachMat))
			obj->Fling(Random(3)-1);
	}
	// Move the quake around a little.
	var dx, dy, cnt = 0;
	do
	{	// Try ten times to find a nearby in material location.
		dx = Random(str * 4 + 1) - str * 2;
		dy = Random(str * 4 + 1) - str * 2;
		cnt++;
	} while (!GBackSemiSolid(x + dx, y + dy) && cnt < 10)
	// No continuation.
	if (cnt >= 10)
		return FX_Execute_Kill;
	// Set new position.
	effect.x += dx;
	effect.y += dy;
	// Done.
	return FX_OK;
}

/*-- Proplist --*/

local Name = "$Name$";
