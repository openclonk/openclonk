/**
	Power Library
	Cares about power management of a base.
	
	callbacks:
	QueryWaivePowerRequest()
	OnNotEnoughPower()
	OnEnoughPower()
	OnRemovedFromPowerSleepingQueue(): called when the object was removed from the sleeping queue
	
	globals:
	MakePowerConsumer(int amount)
		Note: power consumers include the library Library_PowerConsumer and should use UnmakePowerConsumer to turn off as power consumers
	MakePowerProducer(int amount)
	IsPowerAvailable(int amount)
	
	@author Zapper, Maikel
*/

static Library_Power_power_compounds;

// for the helper definitions
local power_links; // producers and consumers
local sleeping_links;
local power_balance; // performance
local neutral; // is "the" neutral helper?

protected func Initialize()
{
	power_links = [];
	sleeping_links = [];
	power_balance = 0;
	neutral = false;
	return;
}

// Adds a power consumer to the network which produces an amount of power.
public func AddPowerProducer(object producer, int amount)
{
	return AddPowerLink(producer, amount);
}

// Adds a power consumer to the network which consumes an amount of power.
public func AddPowerConsumer(object consumer, int amount)
{
	// Check whether this consumer is among the sleeping links of this network. 
	for (var i = GetLength(sleeping_links) - 1; i >= 0; i--)
	{
		var link = sleeping_links[i];
		if (link.obj != consumer) 
			continue;
		// Did not affect power balance, we can just remove/change the link.
		// If amount equals zero, we can remove the consumer from the network.
		if (amount == 0)
		{
			sleeping_links[i] = sleeping_links[GetLength(sleeping_links) - 1];
			SetLength(sleeping_links, GetLength(sleeping_links) - 1);
			// Visualize the consumption change and notify the link object.
			VisualizePowerChange(link.obj, 0, link.amount, false);
			link.obj->~OnRemovedFromPowerSleepingQueue();
			return true;
		}
		// Otherwise, set the new power consumption of this link.
		sleeping_links[i].amount = -amount;
		return true;
	}
	// Consumer is not a sleeping link, therefore add it to the network.
	return AddPowerLink(consumer, -amount);
}

// Removes a power link from its network.
public func RemovePowerLink(object power_link)
{
	return AddPowerLink(power_link, 0);
}

// Adds a power link to the network.
public func AddPowerLink(object power_link, int power_amount, bool surpress_balance_check)
{
	var new_link = {obj = power_link, amount = power_amount};
	
	var before = 0;
	var found = false;
	var first_empty = -1;
	var diff = 0;
	for (var i = GetLength(power_links) - 1; i >= 0; i--)
	{
		var link = power_links[i];
		// Possible
		if (link == nil) 
		{
			first_empty = i;
			continue; 
		}
		
		// Removed from outside.
		if (link.obj == nil) 
		{
			power_links[i] = nil;
			continue;
		}
		
		if (link.obj != power_link) 
			continue;
		found = true;
		
		diff = power_amount - link.amount;
		power_balance += diff;
		before = power_links[i].amount;
		
		if (power_amount == 0)
			power_links[i] = nil;
		else 
			power_links[i] = new_link;
		break;
	}
	
	if (!found)
	{
		// place to insert?
		if (power_amount != 0)
		{
			if (first_empty != -1)
				power_links[first_empty] = new_link;
			else
				power_links[GetLength(power_links)] = new_link;
		}
		diff = new_link.amount;
		power_balance += diff;
	}
	
	if ((new_link.amount > 0) || ((new_link.amount == 0) && (before > 0)))
		VisualizePowerChange(new_link.obj, new_link.amount, before, false);
	else if ((new_link.amount < 0) || ((new_link.amount == 0) && (before < 0)))
		VisualizePowerChange(new_link.obj, new_link.amount, before, false);
	if (new_link.amount < 0)
		new_link.obj->~OnEnoughPower(); // might be reverted soon, though
		
	if (!surpress_balance_check)
		CheckPowerBalance();
	return true;
}

// Main function of the power library which checks the power balance in a closed network.
// This function will deactivate consumers and activate producers whenever necessary.
public func CheckPowerBalance()
{
	// Special handling for ownerless links: always sleep them.
	if (neutral)
	{
		for (var i = GetLength(power_links) - 1; i >= 0; i--)
		{
			var link = power_links[i];
			if (link == nil) 
				continue;
			// Power producers don't need te be put down.
			if (link.amount > 0) 
				continue; 
			SleepLink(i);
		}
		return false;
	}
	
	// Network is not neutral so we can revive sleeping links if the balance is positive.	
	if (power_balance >= 0) 
	{
		// Loop over all sleeping links and find the best one to revive.
		// This is the link with highest priority and which fits the power surplus.
		var highest_priority = -0xffffff;
		var best_link = nil;
		for (var i = GetLength(sleeping_links) - 1; i >= 0; i--)
		{
			var link = sleeping_links[i];
			if (link.obj == nil)
			{
				sleeping_links[i] = sleeping_links[GetLength(sleeping_links) - 1];
				SetLength(sleeping_links, GetLength(sleeping_links) - 1);
				continue;
			}
			// Check if there is enough power to revive this link.
			if (power_balance + link.amount < 0) 
				continue;
			// Store the link with current highest priority.	
			var priority = link.obj->~GetConsumerPriority();	
			if (priority > highest_priority)
			{
				highest_priority = priority;
				best_link = i;		
			}
		}
		// Revive the best link if found and don't execute the rest of this function.
		if (best_link != nil)
			UnsleepLink(best_link);
		return true;
	}
	
	// Network has not enough available power: we need to deactivate some consumers.
	// Look for the best link to sleep.
	var best_amount = -0xffffff;
	var least_priority = 0xffffff;
	var best_link = nil;
	for (var i = GetLength(power_links) - 1; i >= 0; i--)
	{
		var link = power_links[i];
		// Do not shut down producers or invalid links.
		if (link == nil || link.amount > 0) 
			continue;
		// New best link if priority is less or equal and amount is larger.
		var amount = -link.amount;
		var priority = link.obj->~GetConsumerPriority();
		if (priority < least_priority || (priority == least_priority && amount > best_amount))
		{
			best_amount = amount;
			least_priority = priority;
			best_link = i;		
		}
	}
	
	// Total blackout? No consumer active anymore?
	if (best_link == nil)
		return false;
	
	// There is a link which we can sleep.
	SleepLink(best_link);
	
	// Recurse, because we might need to sleep another link or might activate a consumer with less demands.
	CheckPowerBalance();	
	return false;
}

// Removes a link with given index from the power links and adds it to the sleeping links.
public func SleepLink(int index)
{
	// Safety: index must be valid.
	if (index < 0 || index >= GetLength(power_links))
		return FatalError("SleepLink() called without or invalid index!");
	// Only sleep consumers, it does not make sense to sleep producers.
	var link = power_links[index];
	if (link.amount > 0) 
		return FatalError("SleepLink() trying to sleep a producer!");
	// Delete link from power links and add to sleeping links.
	power_links[index] = nil;
	power_balance -= link.amount;
	sleeping_links[GetLength(sleeping_links)] = link;
	// Notify link that it has not enough power and visualize the change.
	link.obj->~OnNotEnoughPower();
	VisualizePowerChange(link.obj, 0, link.amount, true);
	return true;
}

// Removes a link with given index from the sleeping links and adds it to the power links.
public func UnsleepLink(int index)
{
	// Safety: index must be valid.
	if (index < 0 || index >= GetLength(sleeping_links))
		return FatalError("UnsleepLink() called without or invalid index!");
	// Get the sleeping link.
	var link = sleeping_links[index];
	// Delete this link from the list of sleeping links.
	sleeping_links[index] = sleeping_links[GetLength(sleeping_links) - 1];
	SetLength(sleeping_links, GetLength(sleeping_links) - 1);
	// Revive the link by adding it to the power links.
	return AddPowerLink(link.obj, link.amount);
}

// get requested power of nodes that are currently sleeping
public func GetPendingPowerAmount()
{
	var sum = 0;
	for (var i = GetLength(sleeping_links) - 1; i >= 0; i--)
		sum += -sleeping_links[i].amount;
	return sum;
}

// should always be above zero - otherwise an object would have been deactivated
public func GetPowerBalance()
{
	return power_balance;
}

public func IsPowerAvailable(object obj, int amount)
{
	// Ignore object for now.
	return power_balance >= amount;
}

public func Init()
{
	if (GetType(Library_Power_power_compounds) != C4V_Array)
		Library_Power_power_compounds = [];
	return;
}

// Definition call: gives the power helper object.
public func GetPowerHelperForObject(object who)
{
	var w;
	while(w = who->~GetActualPowerConsumer())
	{
		if (w == who) 
			break; // nope
		who = w;
	}
	var flag = GetFlagpoleForPosition(who->GetX() - GetX(), who->GetY() - GetY());
	
	var helper = nil;
	if (!flag) // neutral - needs neutral helper
	{
		for (var obj in Library_Power_power_compounds)
		{
			if (!obj || !obj.neutral) 
				continue;
			helper = obj;
			break;
		}
		
		if (helper == nil) // not yet created?
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			helper.neutral = true;
			Library_Power_power_compounds[GetLength(Library_Power_power_compounds)] = helper;
		}		
	} 
	else
	{
		helper=flag->GetPowerHelper();
		
		if (helper == nil)
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			Library_Power_power_compounds[GetLength(Library_Power_power_compounds)] = helper;
			
			// Add to all linked flags.
			flag->SetPowerHelper(helper);
			for (var f in flag->GetLinkedFlags())
			{
				// Assert different power helpers for same compound.
				if (f->GetPowerHelper() != nil) 
					FatalError("Flags in compound have different power helper!");
				f->SetPowerHelper(helper);
			}
		}
	}	
	return helper;
}


/*-- Global Interface --*/

// Returns the amount of unavailable power that is currently being requested.
global func GetPendingPowerAmount()
{
	if (!this) 
		return 0;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->GetPendingPowerAmount();
}

// Returns the current power balance of the area an object is in.
// This is roughly equivalent to produced power - consumed power. 
global func GetCurrentPowerBalance()
{
	if (!this) 
		return 0;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->GetPowerBalance();
}

// Turns the object into a power producer that produces amount of power until this function is called again with amount = 0.
global func MakePowerProducer(int amount)
{
	if (!this) 
		return false;
	Library_Power->Init();
	var power_helper = Library_Power->GetPowerHelperForObject(this);
	if (!power_helper) 
		return false;
	return power_helper->AddPowerProducer(this, amount);
}

// Turns the power producer into an object that does not produce power.
global func UnmakePowerProducer()
{
	MakePowerProducer(0);
	return;
}

// Returns true if the current power balance is bigger or equal to the requested amount.
global func IsPowerAvailable(int amount)
{
	if (!this) 
		return false;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->IsPowerAvailable(this, amount);
}

// Turns the object into a power consumer, consumption is turned of with amoun = 0.
global func MakePowerConsumer(int amount)
{
	if (!this) 
		return false;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->AddPowerConsumer(this, amount);
}


/*-- Power Visualization --*/

// Visualizes the power change on an object.
private func VisualizePowerChange(object obj, int to, int before, bool loss)
{
	var before_current = nil;
	var effect = GetEffect("VisualPowerChange", obj);
	if (!effect)
		effect = AddEffect("VisualPowerChange", obj, 1, 5, nil, Library_Power);
	else 
		before_current = effect.current;
	
	var to_abs = Abs(to);
	var before_abs = Abs(before);
	
	effect.max = Max(to_abs, before_abs);
	effect.current = before_current ?? before_abs;
	effect.to = to_abs;
	
	if (loss)
		effect.back_graphics_name = "Red";
	else 
		effect.back_graphics_name = nil;
	
	if (to < 0) 
		effect.graphics_name = "Yellow";
	else if	(to > 0) 
		effect.graphics_name = "Green";
	else // off now
	{
		if (before < 0)
			effect.graphics_name = "Yellow";
		else 
			effect.graphics_name = "Green";
	}

	EffectCall(obj, effect, "Refresh");
}

protected func FxVisualPowerChangeRefresh(object target, proplist effect)
{
	if (effect.bar) 
		effect.bar->Close();
	var vis = VIS_Allies | VIS_Owner;
	var controller = target->GetController();
	if (controller == NO_OWNER) 
		vis = VIS_All;
	var off_x = -(target->GetDefCoreVal("Width", "DefCore") * 3) / 8;
	var off_y = target->GetDefCoreVal("Height", "DefCore") / 2 - 10;
	
	effect.bar = target->CreateProgressBar(GUI_BarProgressBar, effect.max, effect.current, 35
		, controller, {x = off_x, y = off_y}, vis
		, {size = 1000, bars = effect.max / 25, graphics_name = effect.graphics_name, back_graphics_name = effect.back_graphics_name, image = Icon_Lightbulb, fade_speed = 1});
	
	// Appear on a GUI level in front of other objects, e.g. trees.
	effect.bar->SetPlane(1010);
	return;
}

protected func FxVisualPowerChangeTimer(object target, proplist effect, int time)
{
	if (!effect.bar) 
		return -1;
	if (effect.current == effect.to) 
		return 1;
	
	if (effect.to < effect.current) 
		effect.current = Max(effect.current - 15, effect.to);
	else 
		effect.current = Min(effect.current + 15, effect.to);

	effect.bar->SetValue(effect.current);
	return 1;
}


/*-- Saving --*/

// Helper object should not be saved.
public func SaveScenarioObject()
{
	if (GetID() == Library_Power) 
		return false;
	return inherited(...);
}
