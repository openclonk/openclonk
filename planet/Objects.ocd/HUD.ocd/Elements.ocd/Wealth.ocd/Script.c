/**
	HUD Wealth
	Displays the wealth for each player in the HUD.
	
	@authors Newton
*/

protected func Initialize()
{
	// Set parallaxity
	this.Parallaxity = [0, 0];
	// Set visibility
	this.Visibility = VIS_Owner;
	return;
}

public func Update()
{
	var plr = GetOwner();
	var wealth = GetWealth(plr);
	// Display wealth via text.
	CustomMessage(Format("@%d", wealth), this, plr, 0, 90);
	// Display wealth via graphics.
	var num;
	if (wealth < 180) num = 4;
	if (wealth < 120) num = 3;
	if (wealth < 70) num = 2;
	if (wealth < 30) num = 1;
	if (wealth < 10) num = 0;
	SetGraphics(Format("%d", num));
	return;
}
