/*
	Menu control
	Author: Newton

	
*/

local menu;

func Construction()
{
	menu = nil;
	return _inherited(...);
}

func HasMenuControl()
{
	return true;
}

func SetMenu(object m)
{
	// destroy old one:
	// multiple menus are currently not supported (yet)
	if (menu)
	{
		menu->Close();
	}
	// new one
	menu = m;
}

func GetMenu()
{
	return menu;
}
