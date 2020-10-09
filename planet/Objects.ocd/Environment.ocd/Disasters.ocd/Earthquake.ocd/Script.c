/**
	Earthquake
	Trembles the earth.
	
	@author Maikel	
*/


/*-- Disaster Control --*/

public func SetChance(int chance)
{
	if (this != Earthquake)
		return;
	var effect = GetEffect("IntEarthquakeControl");
	if (!effect)
		effect = AddEffect("IntEarthquakeControl", nil, 100, 20, nil, Earthquake);
	effect.chance = chance;
	return;
}

public func GetChance()
{
	if (this != Earthquake)
		return;
	var effect = GetEffect("IntEarthquakeControl");
	if (effect)
		return effect.chance;
	return;
}

protected func FxIntEarthquakeControlTimer(object target, proplist effect, int time)
{
	if (Random(100) < effect.chance && !Random(10))
		LaunchEarthquake(Random(LandscapeWidth()), Random(LandscapeHeight()), Random(40) + 35);
	return FX_OK;
}

// Scenario saving
func FxIntEarthquakeControlSaveScen(obj, fx, props)
{
	props->Add("Earthquake", "Earthquake->SetChance(%d)", fx.chance);
	return true;
}

// Launches an earthquake with epicenter (x, y) in global coordinates.
global func LaunchEarthquake(int x, int y, int strength)
{
	// Earthquake should start in solid.
	if (!GBackSemiSolid(x - GetX(), y - GetY()))
		return false;
	// Minimum strength is 15, maximum strength is 100.
	strength = BoundBy(strength, 15, 100);
	// The earthquake is handled by a global effect.
	var effect = AddEffect("IntEarthquake", nil, 100, 1, nil, Earthquake);
	effect.x = x; // Epicentre x coordinate.
	effect.y = y; // Epicentre y coordinate.
	effect.strength = strength / 3; // Earthquake strength.
	effect.length = 3 * strength / 2; // Earthquake length.
	return true;
}

/*-- Earthquake --*/

protected func FxIntEarthquakeStart(object target, effect)
{
	// Start sound at quake local coordinates.
	// < Maikel> Global until someone implements non-object local sounds
	Sound("Environment::Disasters::Earthquake", true, 100, nil, 1);
	return FX_OK;
}

protected func FxIntEarthquakeStop(object target, effect)
{
	// Stop sound.
	Sound("Environment::Disasters::Earthquake", true, 100, nil, -1);
	Sound("Environment::Disasters::EarthquakeEnd",true);
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
	// Get quake coordinates.
	var x = effect.x;
	var y = effect.y;
	var l = 4 * str;
	// Shake viewport.
	if (!Random(10))
		ShakeViewport(str, x, y);
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
	} while (!GBackSemiSolid(x + dx, y + dy) && cnt < 10);
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


/* Editor */

public func Definition(def, ...)
{
	UserAction->AddEvaluator("Action", "Disasters", "$LaunchEarthquake$", "$LaunchEarthquakeHelp$", "launch_earthquake", [def, def.EvalAct_LaunchEarthquake], { Strength={Function="int_constant", Value = 50} }, { Type="proplist", Display="{{Position}}, {{Strength}}", EditorProps = {
		Position = new UserAction.Evaluator.Position { EditorHelp="$PositionHelp$" },
		Strength = new UserAction.Evaluator.Integer { Name="$Strength$", EditorHelp="$StrengthHelp$" },
		} } );
}

private func EvalAct_LaunchEarthquake(proplist props, proplist context)
{
	// Earthquake through user action
	var position = UserAction->EvaluatePosition(props.Position, context);
	var strength = UserAction->EvaluateValue("Integer", props.Strength, context);
	if (strength > 0)
	{
		LaunchEarthquake(position[0], position[1], strength);
	}
}
