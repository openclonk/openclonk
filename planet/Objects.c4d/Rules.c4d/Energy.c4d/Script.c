/*-- Power usage --*/

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}