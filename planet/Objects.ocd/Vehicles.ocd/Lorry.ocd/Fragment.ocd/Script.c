/**
	Lorry Fragment
	Fragment of the lorry created on explosion.

	@author Maikel
*/


local alpha;

protected func Initialize()
{
	// Set graphics to a random fragment.
	SetGraphics([nil, "Edge", "Metal", "Wheel"][Random(3)]);
	// Set initial alpha to zero.
	alpha = 0;
	// Add a timer for some special behavior.
	AddTimer("ProcessFragment", 4);
	return;
}

protected func ProcessFragment()
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
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";