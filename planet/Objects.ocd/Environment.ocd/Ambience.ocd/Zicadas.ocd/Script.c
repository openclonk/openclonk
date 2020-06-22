/**
	Zicadas
	Zicada sounds.
*/

local Name = "$Name$";
local Description = "$Description$";

func Place(int amount_percentage, proplist area)
{
	area = area ?? Shape->LandscapeRectangle();
	amount_percentage = amount_percentage ?? 100;
	
	// calculate amount that has to be placed
	var amount = LandscapeWidth() / 100;
	amount = (amount_percentage * amount) / 100;
	if (!amount) return;
	
	while (--amount)
	{
		// search for zicada spot position
		// ..
		// place zicada spot
		// ..
	}
}