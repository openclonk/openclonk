local MENU =
{
	left = 
	{
		Wdt = 500,
		title = 
		{
			Wdt = 500,
			Decoration = GUI_MenuDeco
		}
	},
	
	right =
	{
		X = 500,
		objects = 
		{
			o1 = {Symbol = Rock, X = [0, 50], Y = [0, 40], Wdt = [0, 50 ERROR HERE! 64], Hgt=[0, 40 + 64]},
			o2 = {Symbol = Gold, X = [500, 50 - 32], Y = [500, 40 - 32], Wdt = [500, 50 +32], Hgt = [500, 40 + 32]},
			o3 = {Symbol = Wood, X = [1000, -64], Y = [1000, -64], Wdt = [1000, 0], Hgt = [500, 0]},
		},
		text =
		{
			Y = [1000, -300],
			Hgt = 1000
		}
	}
};
