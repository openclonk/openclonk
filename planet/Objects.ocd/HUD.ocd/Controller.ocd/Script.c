/**
	HUD Controller
	Controls the player HUD and all its subsystems, which are:
		* Inventory
		* Actionbar
		* Crew selectors
		* Goal
		* Wealth
	All of the subsystems are handled by different included definitions
	and use overloaded functions.

	Receives all engine callbacks that are forwarded to environment objects.

	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action bar.

	@authors Newton, Mimmo_O, Zapper, Maikel
*/

// Include the different subsystems of the HUD. They all handle their part
// themselves via overloading of callbacks.
#include GUI_Controller_InventoryBar
#include GUI_Controller_ActionBar
#include GUI_Controller_CrewBar
#include GUI_Controller_Goal
#include GUI_Controller_Wealth
// Include the basic functionality of the HUD
#include Library_HUDController
