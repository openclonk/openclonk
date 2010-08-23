/*-- Power usage --*/

protected func Activate(int iByPlayer)
{
	MessageWindow(GetProperty("Description"), iByPlayer);
	return true;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}