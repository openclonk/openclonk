
#strict 2

// Wrappers for conventient calls to FindObjects

/* Find */

global func Find_Not(Cond) {
	return [C4FO_Not, Cond];
}

global func Find_And() {
	var result = [C4FO_And];
	for(var i = 0; Par(i); i++) {
		result[i+1] = Par(i);
	}
	return result;
}

global func Find_Or() {
	var result = [C4FO_Or];
	for(var i = 0; Par(i); i++)
		result[i+1] = Par(i);
	return result;
}

global func Find_Exclude(object pObj) {
	if(!pObj) pObj = this;
	if(!pObj) return;
	return [C4FO_Exclude, pObj];
}

global func Find_ID(id idDef) {
	return [C4FO_ID, idDef];
}

global func Find_InRect(int x, int y, int wdt, int hgt) {
	return [C4FO_InRect, GetX() + x, GetY() + y, wdt, hgt];
}

global func Find_AtPoint(int x, int y) {
	return [C4FO_AtPoint, GetX() + x, GetY() + y];
}

global func Find_AtRect(int x, int y, int wdt, int hgt) {
	return [C4FO_AtRect, GetX() + x, GetY() + y, wdt, hgt];
}

global func Find_OnLine(int x, int y, int x2, int y2) {
	return [C4FO_OnLine, GetX() + x, GetY() + y, GetX() + x2, GetY() + y2];
}

global func Find_Distance(int r, int x, int y) {
	return [C4FO_Distance, GetX() + x, GetY() + y, r];
}

global func Find_OCF(int ocf) {
	return [C4FO_OCF, ocf];
}

global func Find_Category(int iCategory) {
	return [C4FO_Category, iCategory];
}

global func Find_Action(string act) {
	return [C4FO_Action, act];
}

global func Find_ActionTarget(object target) {
	return [C4FO_ActionTarget, target, 0];
}

global func Find_ActionTarget2(object target) {
	return [C4FO_ActionTarget, target, 1];
}

global func Find_Procedure(int procedure) {
	return [C4FO_Procedure, procedure];
}

global func Find_Container(object container) {
	return [C4FO_Container, container];
}

global func Find_NoContainer() {
	return Find_Container(nil);
}

global func Find_AnyContainer() {
	return [C4FO_AnyContainer];
}

global func Find_Owner(int owner) {
	return [C4FO_Owner, owner];
}

global func Find_Controller(int controller) {
	return [C4FO_Controller, controller];
}

global func Find_Hostile(int plr) {
	var p = [C4FO_Or];
	for(var i = -1; i < GetPlayerCount(); i++)
		if(Hostile(plr, GetPlayerByIndex(i)))
			p[GetLength(p)] = Find_Owner(GetPlayerByIndex(i));
	return p;
}

global func Find_Allied(int plr) {
	var p = [C4FO_Or];
	for(var i = -1; i < GetPlayerCount(); i++)
		if(!Hostile(plr, GetPlayerByIndex(i)))
			p[GetLength(p)] = Find_Owner(GetPlayerByIndex(i));
	return p;
}

global func Find_Func(string f, p1, p2, p3, p4, p5, p6, p7, p8, p9) {
	return [C4FO_Func, f, p1, p2, p3, p4, p5, p6, p7, p8, p9];
}

global func Find_Layer(object layer) {
  return [C4FO_Layer, layer];
}

global func Find_PathFree(object toobj) {
  if (!toobj) toobj = this;
  return [C4FO_Func, "Find_PathFreeCheck", toobj];
}

global func Find_PathFreeCheck(object toobj) {
  return PathFree(GetX(), GetY(), toobj->GetX(), toobj->GetY());
}
  

/* Sort */

global func Sort_Reverse(array sort) {
  return [C4SO_Reverse, sort];
}

global func Sort_Multiple() {
  var result = [C4SO_Multiple];
	for(var i = 0; Par(i); i++) {
		result[i+1] = Par(i);
	}
	return result;
}

global func Sort_Distance(int x, int y) {
	return [C4SO_Distance, GetX() + x, GetY() + y];
}

global func Sort_Random() {
	return [C4SO_Random];
}

global func Sort_Speed() {
	return [C4SO_Speed];
}

global func Sort_Mass() {
	return [C4SO_Mass];
}

global func Sort_Value() {
	return [C4SO_Value];
}

global func Sort_Func(string f, p1, p2, p3, p4, p5, p6, p7, p8, p9) {
	return [C4SO_Func, f, p1, p2, p3, p4, p5, p6, p7, p8, p9];
}
