/*
	Returns a suitable position to fly to in order to perform a boarding attack on one of the islands.
	Will make enemies fly around the islands first if they spawn below them.

	Frequently used by Airship captains.
*/

static IslandRectangle;

global func GetBoardingPoint(object target)
{
	if (!this) return nil;
	if (!target) return nil;

	var x, y;

	// A rectangle surrounding the islands + some space (30px or so)
	if (!IslandRectangle) IslandRectangle = Shape->Rectangle(780, 780, 430, 310);

	// Below the target
	if (this->GetY() > target->GetY())
	{
		// Below the islands
		if (Inside(this->GetX(), 780, 1110))
		{
			if (GetX() < LandscapeWidth())
			{
				y = this->GetY() / 2;
				x = RandomX(100, 200);
			} else {
				y = this->GetY() / 2;
				x = RandomX(LandscapeWidth() - 200, LandscapeWidth() - 100);
			}
		} else { // way up is clear
			x = GetX();
			y = 780 - Random(200);
		}
	} else
	{
		// Draw a straight line towards the target and pick a point 30 pixels away
		var dist = ObjectDistance(target);
		var xdist = target->GetX() - GetX();
		var ydist = target->GetY() - GetY();
		x = target->GetX() - (xdist * 30) / dist;
		y = target->GetY() - (ydist * 30) / dist;
	}

	// Near enough to try an attack
	if (IslandRectangle->IsPointContained(this->GetX(), this->GetY()))
	{
		this->~PrepareToBoard();
		x = this->GetX();
		y = this->GetY();
	}

	return [x, y];
}

global func InsideIslandRectangle(object ship)
{
	if (!IslandRectangle) IslandRectangle = Shape->Rectangle(780, 780, 430, 310);

	return IslandRectangle->IsPointContained(ship->GetX(), ship->GetY());
}