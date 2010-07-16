/*-- Ropeladder_Grabber --*/


// Opens a menu when the clonk grabs this chest.
protected func Grabbed(object by_object, bool grab)
{
	GetActionTarget()->Grabbed(by_object, grab);
	return;
}


func Definition(def) {
	SetProperty("ActMap", {

Attach = {
	Prototype = Action,
	Name = "Attach",
	Procedure = DFA_ATTACH,
},}, def);
	SetProperty("Name", "$Name$", def);
}