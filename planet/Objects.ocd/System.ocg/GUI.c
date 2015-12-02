/**
	Contains UI related functions that require definitions from Objects.ocd to be loaded.
*/

// The default menu decoration used in most places.
global func GetDefaultMenuDecoration()
{
	return GUI_MenuDeco;
}

// Returns the symbol used for aborting or canceling.
global func GetDefaultCancelSymbol()
{
	return Icon_Cancel;
}
