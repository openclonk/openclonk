/** 
	Gold Seller 
	Automatically sells gold and other valuables nearby when in a base area.
	Include this library in the object you wan to make an automatic gold 
	seller and don't forget to return _inherited(...) in Initialize().
	
	The radius for selling gold can be set by overloading the function
	AutoSellValuablesRadius() and returning the new radius.
	
	@author Maikel
*/


// Standard the object has an auto sell radius of 50 pixels.
private func AutoSellValuablesRadius() { return 50; }

protected func Initialize()
{
	AddEffect("AutoSellValuables", this, 1, 4, this);
	return _inherited(...);
}

protected func FxAutoSellValuablesStart(object target, proplist effect, int temp)
{
	if (temp) 
		return FX_OK;
	// Ensure a rather short interval to not miss any fast objects.
	effect.Interval = 4;
	// Define particles.
	effect.dust_particles =
	{
		Prototype = Particles_Dust(),
		Size = PV_KeyFrames(0, 0, 0, 100, 10, 1000, 0),
		Alpha = PV_KeyFrames(0, 0, 255, 750, 255, 1000, 0),
		R = 200,
		G = 125,
		B = 125,
	};
	effect.flash_particles =
	{ 
		Prototype = Particles_Flash(),
		Size = 12,
	};	
	return FX_OK;
}

protected func FxAutoSellValuablesTimer(object target, proplist effect, int time)
{
	var owner = GetOwner();
	var sell_objects = [];
	// Find objects which can be sold.
	for (var obj in FindObjects(Find_Distance(AutoSellValuablesRadius()), Find_NoContainer(), Find_Func("IsValuable")))
	{
		if (obj->Stuck()) 
			continue;
		if (!IsAllied(owner, obj->GetController()))
			continue;
		PushBack(sell_objects, obj);
	} 
	// Don't do anything if there are no objects to sell.
	if (GetLength(sell_objects) <= 0)
		return FX_OK;
		
	// Loop over all objects which are to be sold.
	var has_sold = false;
	for (var to_sell in sell_objects)
	{
		// Check if object may be sold and add wealth.
		if (to_sell->~QueryOnSell(to_sell->GetController())) 
			continue;
		DoWealth(to_sell->GetController(), to_sell->GetValue());
		// Set has sold variable to true.
		has_sold = true;
		// For each of the objects to sell create a floating message indicating the value.
		var floating_message = CreateObjectAbove(FloatingMessage, to_sell->GetX() - GetX(), to_sell->GetY() - GetY(), NO_OWNER);
		floating_message->SetColor(250, 200, 50);
		floating_message->FadeOut(2, 10);
		floating_message->SetSpeed(0, -5);
		floating_message->SetMessage(Format("%d</c>{{Icon_Coins}}", to_sell->GetValue()));
		// Create some particles and remove object.
		CreateParticle("Flash", to_sell->GetX() - GetX(), to_sell->GetY() - GetY(), 0, 0, 8, effect.flash_particles);
		CreateParticle("Dust", to_sell->GetX() - GetX(), to_sell->GetY() - GetY(), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(18, 36), effect.dust_particles, 10);
		to_sell->RemoveObject();
	}
	
	// Play a sound if a valuable has been sold.
	if (has_sold)
		Sound("Cash");
	
	return FX_OK;
}
