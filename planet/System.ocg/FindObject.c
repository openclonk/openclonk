/**
	FindObject.c
	Wrappers for convenient calls to the FindObject family.

	@author		
*/


/*-- Find functions --*/

// documented in /docs/sdk/script/fn
global func Find_Not(cond)
{
	return [C4FO_Not, cond];
}

// documented in /docs/sdk/script/fn
global func Find_And(...)
{
	var result = [C4FO_And];
	for (var i = 0; Par(i); i++)
		result[i + 1] = Par(i);
	return result;
}

// documented in /docs/sdk/script/fn
global func Find_Or(...)
{
	var result = [C4FO_Or];
	for (var i = 0; Par(i); i++)
		result[i + 1] = Par(i);
	return result;
}

// documented in /docs/sdk/script/fn
global func Find_Exclude(object obj)
{
	if (!obj)
		obj = this;
	if (!obj)
		return;
	return [C4FO_Exclude, obj];
}

// documented in /docs/sdk/script/fn
global func Find_ID(id def)
{
	return [C4FO_ID, def];
}

// documented in /docs/sdk/script/fn
global func Find_InRect(int x, int y, int wdt, int hgt)
{
	return [C4FO_InRect, x, y, wdt, hgt];
}

// documented in /docs/sdk/script/fn
global func Find_AtPoint(int x, int y)
{
	return [C4FO_AtPoint, x, y];
}

global func Find_AtRect(int x, int y, int wdt, int hgt)
{
	return [C4FO_AtRect, x, y, wdt, hgt];
}

// documented in /docs/sdk/script/fn
global func Find_OnLine(int x, int y, int x2, int y2)
{
	return [C4FO_OnLine, x, y, x2, y2];
}

// documented in /docs/sdk/script/fn
global func Find_Distance(int r, int x, int y)
{
	return [C4FO_Distance, x, y, r];
}

// documented in /docs/sdk/script/fn
global func Find_Cone(int r, int cone_angle, int cone_width, int x, int y, int prec_angle)
{
	if (!prec_angle)
		prec_angle = 1;
	return [C4FO_Cone, x, y, r, cone_angle, cone_width, prec_angle];
}

// documented in /docs/sdk/script/fn
global func Find_OCF(int ocf)
{
	return [C4FO_OCF, ocf];
}

// documented in /docs/sdk/script/fn
global func Find_Category(int category)
{
	return [C4FO_Category, category];
}

// documented in /docs/sdk/script/fn
global func Find_Action(string action)
{
	return [C4FO_Action, action];
}

// documented in /docs/sdk/script/fn
global func Find_ActionTarget(object target)
{
	return [C4FO_ActionTarget, target, 0];
}

// documented in /docs/sdk/script/fn
global func Find_ActionTarget2(object target)
{
	return [C4FO_ActionTarget, target, 1];
}

// documented in /docs/sdk/script/fn
global func Find_ActionTargets(object target)
{
	return [C4FO_Or, Find_ActionTarget(target), Find_ActionTarget2(target)];
}

global func Find_Procedure(string procedure)
{
	return [C4FO_Procedure, procedure];
}

// documented in /docs/sdk/script/fn
global func Find_Container(object container)
{
	return [C4FO_Container, container];
}

// documented in /docs/sdk/script/fn
global func Find_NoContainer()
{
	return [C4FO_Container, nil];
}

// documented in /docs/sdk/script/fn
global func Find_AnyContainer()
{
	return [C4FO_AnyContainer];
}

// documented in /docs/sdk/script/fn
global func Find_Owner(int owner)
{
	return [C4FO_Owner, owner];
}

global func Find_Controller(int controller)
{
	return [C4FO_Controller, controller];
}

// documented in /docs/sdk/script/fn
global func Find_Hostile(int plr)
{
	var p = [C4FO_Or];
	for (var i = -1; i < GetPlayerCount(); i++)
		if (Hostile(plr, GetPlayerByIndex(i)))
			p[GetLength(p)] = Find_Owner(GetPlayerByIndex(i));
	return p;
}

/*
Similar to Find_Hostile, but defaults to treating all players as hostile when plr = NO_OWNER.
*/
global func Find_AnimalHostile(int plr)
{
	if (plr == NO_OWNER)
		return Find_Not(Find_Owner(NO_OWNER));
	return Find_Or(Find_Owner(NO_OWNER), Find_Hostile(plr));
}

// documented in /docs/sdk/script/fn
global func Find_Allied(int plr)
{
	var p = [C4FO_Or];
	for (var i = -1; i < GetPlayerCount(); i++)
		if (!Hostile(plr, GetPlayerByIndex(i)))
			p[GetLength(p)] = Find_Owner(GetPlayerByIndex(i));
	return p;
}

// documented in /docs/sdk/script/fn
global func Find_Team(int team)
{
	var p = [C4FO_Or];
	for (var i = -1; i < GetPlayerCount(); i++)
		if (GetPlayerTeam(GetPlayerByIndex(i)) == team)
			p[GetLength(p)] = Find_Owner(GetPlayerByIndex(i));
	return p;
}

// documented in /docs/sdk/script/fn
global func Find_Func(string f, p1, p2, p3, p4, p5, p6, p7, p8, p9)
{
	return [C4FO_Func, f, p1, p2, p3, p4, p5, p6, p7, p8, p9];
}

// documented in /docs/sdk/script/fn
global func Find_Layer(object layer)
{
	return [C4FO_Layer, layer];
}

// documented in /docs/sdk/script/fn
global func Find_InArray(array a)
{
	return [C4FO_InArray, a];
}

// documented in /docs/sdk/script/fn
global func Find_Property(string s, value)
{
	// Ideally, we'd check the parameter count here, but this information is
	// not available from the script.
	if (value == nil)
		return [C4FO_Property, s];
	else
		return [C4FO_Property, s, value];
}

// documented in /docs/sdk/script/fn
global func Find_AnyLayer()
{
	return [C4FO_AnyLayer];
}

global func Find_PathFree(object to_obj)
{
	if (!to_obj)
		to_obj = this;
	return [C4FO_Func, "Find_PathFreeCheck", to_obj];
}

global func Find_PathFreeCheck(object to_obj)
{
	return PathFree(GetX(), GetY(), to_obj->GetX(), to_obj->GetY());
}

/*-- Sort functions --*/

// documented in /docs/sdk/script/fn
global func Sort_Reverse(array sort)
{
	return [C4SO_Reverse, sort];
}

// documented in /docs/sdk/script/fn
global func Sort_Multiple(...)
{
	var result = [C4SO_Multiple];
	for (var i = 0; Par(i); i++)
		result[i + 1] = Par(i);
	return result;
}

// documented in /docs/sdk/script/fn
global func Sort_Distance(int x, int y)
{
	return [C4SO_Distance, x, y];
}

// documented in /docs/sdk/script/fn
global func Sort_Random()
{
	return [C4SO_Random];
}

// documented in /docs/sdk/script/fn
global func Sort_Speed()
{
	return [C4SO_Speed];
}

// documented in /docs/sdk/script/fn
global func Sort_Mass()
{
	return [C4SO_Mass];
}

// documented in /docs/sdk/script/fn
global func Sort_Value()
{
	return [C4SO_Value];
}

// documented in /docs/sdk/script/fn
global func Sort_Func(string f, p1, p2, p3, p4, p5, p6, p7, p8, p9)
{
	return [C4SO_Func, f, p1, p2, p3, p4, p5, p6, p7, p8, p9];
}
