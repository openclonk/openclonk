/**
	Destructible Fragment
	Generic fragment of something

	@author Maikel, Clonkonaut
*/


local alpha;

func Initialize()
{
	// Set graphics to a random fragment.
	var graphic = Random(4);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	// Set initial alpha to zero.
	alpha = 0;
	// Add a timer for some special behavior.
	AddTimer("ProcessFragment", 4);
}

func ProcessFragment()
{
	// Remove object if con is too small or when alpha is too high.
	if (GetCon() < 40 || alpha > 200)
		return RemoveObject();
	// Fade out object when it has been extinguished.
	if (!OnFire())
	{
		alpha += 4;
		SetObjAlpha(255 - alpha);
	}
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";