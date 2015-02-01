/**
	Power Library
	Handles the aspects for a single power networks, for each network in a round
	a copy of this library object is created. For each network it is being kept 
	track of which are the idle/active producers and idle/active consumers and 
	power is requested from producers and distributed for consumers according 
	to priority.
	
	Callbacks to the power producers (see producer library for details):
	 * OnPowerProductionStart(int amount)
	 * OnPowerProductionStop()
	 * GetProducerPriority()
	 * IsSteadyPowerProducer()
	 
	Callbacks to the power consumers (see consumer library for details):
	 * OnEnoughPower(int amount)
	 * OnNotEnoughPower()
	 * GetConsumerPriority()
	 * GetActualPowerConsumer()
	 
	The network object keeps track of the producers and consumers in lists.
	 * lib_power.idle_producers (currently not producing power)
	 * lib_power.active_producers (currently producing power)
	 * lib_power.waiting_consumers (waiting for power)
	 * lib_power.active_consumers (supplied consumers)
	Producers are stored according to {obj, prod_amount, priority} and consumers
	according to {obj, cons_amount, priority}.
	
	OPEN TODOS:
	 * Figure out where the nil's and the errors come from.
	 * Remove all the if (!link) checks, they are not needed in principle but cause errors.
	 * Fix the visualization of power on the nodes.
	 * Check the flag library for network merging.
	 * Check the power balance, also with merging.
	 * Fix the compensator.
	 * Fix the pump.

	@author Zapper, Maikel
*/


// A static variable is used to store the different power networks.
// This variable is also accessed by the flag library.
static LIB_POWR_Networks;

// All power related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See 
// Initialize for which variables are being used.
local lib_power;

// Initialize the local variables needed to keep track of each power network.
protected func Initialize()
{
	// Initialize the single proplist for the power library.
	lib_power = {};
	// Initialize producer and consumer lists.
	// Lists to keep track of the producers and consumers.
	lib_power.idle_producers = [];
	lib_power.active_producers = [];
	lib_power.waiting_consumers = [];
	lib_power.active_consumers = [];
	// Main variable which keeps track of the power balance in the network.
	// Init power balance to zero.
	lib_power.power_balance = 0;
	// Whether the network is neutral.
	// Network is not neutral by default.
	lib_power.neutral_network = false;
	return;
}


/*-- Definition Calls --*/

// Definition call: registers a power producer with specified amount.
public func RegisterPowerProducer(object producer, int amount)
{
	// Definition call safety checks.
	if (this != Library_Power || !producer || !producer->~IsPowerProducer())
		return FatalError("RegisterPowerProducer() either not called from definition context or no producer specified.");
	Library_Power->Init();
	// Find the network for this producer and add it.
	var network = GetPowerNetwork(producer);	
	network->AddPowerProducer(producer, amount, producer->GetProducerPriority());
	return;
}

// Definition call: unregisters a power producer.
public func UnregisterPowerProducer(object producer)
{
	// Definition call safety checks.
	if (this != Library_Power || !producer || !producer->~IsPowerProducer())
		return FatalError("UnregisterPowerProducer() either not called from definition context or no producer specified.");
	Library_Power->Init();
	// Find the network for this producer and remove it.
	var network = GetPowerNetwork(producer);	
	network->RemovePowerProducer(producer);
	return;
}

// Definition call: registers a power consumer with specified amount.
public func RegisterPowerConsumer(object consumer, int amount)
{
	// Definition call safety checks.
	if (this != Library_Power || !consumer || !consumer->~IsPowerConsumer())
		return FatalError("RegisterPowerConsumer() either not called from definition context or no consumer specified.");
	Library_Power->Init();
	// Find the network for this consumer and add it.
	var network = GetPowerNetwork(consumer);	
	network->AddPowerConsumer(consumer, amount, consumer->GetConsumerPriority());
	return;
}

// Definition call: unregisters a power consumer.
public func UnregisterPowerConsumer(object consumer)
{
	// Definition call safety checks.
	if (this != Library_Power || !consumer || !consumer->~IsPowerConsumer())
		return FatalError("UnregisterPowerConsumer() either not called from definition context or no consumer specified.");
	Library_Power->Init();
	// Find the network for this consumer and remove it.
	var network = GetPowerNetwork(consumer);	
	network->RemovePowerConsumer(consumer);
	return;
}

// Definition call: gives the power helper object.
// TODO: Clean up this code!
public func GetPowerNetwork(object for_obj)
{
	// Definition call safety checks.
	if (this != Library_Power || !for_obj)
		return FatalError("GetPowerNetwork() either not called from definition context or no object specified.");
	
	var w;
	while (w = for_obj->~GetActualPowerConsumer())
	{
		if (w == for_obj) 
			break; // nope
		for_obj = w;
	}
	var flag = GetFlagpoleForPosition(for_obj->GetX() - GetX(), for_obj->GetY() - GetY());
	
	var helper = nil;
	if (!flag) // neutral - needs neutral helper
	{
		for (var network in LIB_POWR_Networks)
		{
			if (!network || !network.lib_power.neutral_network) 
				continue;
			helper = network;
			break;
		}
		
		if (helper == nil) // not yet created?
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			helper.lib_power.neutral_network = true;
			LIB_POWR_Networks[GetLength(LIB_POWR_Networks)] = helper;
		}		
	} 
	else
	{
		helper=flag->GetPowerHelper();
		
		if (helper == nil)
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			LIB_POWR_Networks[GetLength(LIB_POWR_Networks)] = helper;
			
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

// Definition call: Initializes tracking of the power networks.
public func Init()
{
	// Definition call safety checks.
	if (this != Library_Power)
		return;
	// Initialize the list of networks if not done already.
	if (GetType(LIB_POWR_Networks) != C4V_Array)
		LIB_POWR_Networks = [];
	return;
}


/*-- Library Code --*/

public func AddPowerProducer(object producer, int amount, int prio)
{
	// Debugging logs.
	Log("POWR - AddPowerProducer(): network = %v, frame = %d, producer = %v, amount = %d, priority = %d", this, FrameCounter(), producer, amount, prio);
	// Check if it is not already in the list of idle producers.
	for (var index = GetLength(lib_power.idle_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.idle_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.prod_amount != amount || link.priority != prio)
		{
			lib_power.idle_producers[index] = {obj = producer, prod_amount = amount, priority = prio};
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Check if it is not already in the list of active producers.
	for (var index = GetLength(lib_power.active_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.active_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.prod_amount != amount || link.priority != prio)
		{
			// Adjust the power balance accordingly.
			lib_power.power_balance += amount - link.prod_amount;
			lib_power.active_producers[index] = {obj = producer, prod_amount = amount, priority = prio};
			VisualizePowerChange(producer, link.prod_amount, amount, false);
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Producer was in neither list, so add it to the list of idle producers.
	// Add steady power producers directly into the list of active producers.
	if (producer->IsSteadyPowerProducer())
	{
		// Adjust the power balance accordingly.
		lib_power.power_balance += amount;
		PushBack(lib_power.active_producers, {obj = producer, prod_amount = amount, priority = prio});
		VisualizePowerChange(producer, 0, amount, false);
	}
	else
	{
		PushBack(lib_power.idle_producers, {obj = producer, prod_amount = amount, priority = prio});
	}
	// Check the power balance of this network, since a change has been made.
	CheckPowerBalance();
	return;
}

public func RemovePowerProducer(object producer)
{
	// Debugging logs.
	Log("POWR - RemovePowerProducer(): network = %v, frame = %d, producer = %v", this, FrameCounter(), producer);
	// Remove producer from the list of idle producers if it is in there.
	for (var index = GetLength(lib_power.idle_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.idle_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// Remove the producer from the list.
		RemoveArrayIndex(lib_power.idle_producers, index);
		return;
	}
	// Otherwise, remove producer from the list of active producers if it is in there.
	for (var index = GetLength(lib_power.active_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.active_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// Reduce the power balance and remove the producer from the list.
		lib_power.power_balance -= link.prod_amount;
		RemoveArrayIndex(lib_power.active_producers, index);
		VisualizePowerChange(link.obj, link.prod_amount, 0, false);
		// Notify the active power producer that it should stop producing power.
		producer->OnPowerProductionStop();
		// Check the power balance of this network, since a change has been made.
		CheckPowerBalance();
		return;
	}
	// If found in neither lists just return without doing anything.
	return;
}

public func AddPowerConsumer(object consumer, int amount, int prio)
{
	// Debugging logs.
	Log("POWR - AddPowerConsumer(): network = %v, frame = %d, consumer = %v, amount = %d, priority = %d", this, FrameCounter(), consumer, amount, prio);
	// Check if it is not already in the list of waiting consumers.
	for (var index = GetLength(lib_power.waiting_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.waiting_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.cons_amount != amount || link.priority != prio)
		{
			lib_power.waiting_consumers[index] = {obj = consumer, cons_amount = amount, priority = prio};
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Check if it is not already in the list of active consumers.
	for (var index = GetLength(lib_power.active_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.active_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.cons_amount != amount || link.priority != prio)
		{
			// Adjust the power balance accordingly.
			lib_power.power_balance -= amount - link.cons_amount;
			lib_power.active_consumers[index] = {obj = consumer, cons_amount = amount, priority = prio};
			VisualizePowerChange(link.obj, link.cons_amount, amount, false);
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Consumer was in neither list, so add it to the list of waiting consumers.
	PushBack(lib_power.waiting_consumers, {obj = consumer, cons_amount = amount, priority = prio});
	// Check the power balance of this network, since a change has been made.
	CheckPowerBalance();
	return;
}

public func RemovePowerConsumer(object consumer)
{
	// Debugging logs.
	Log("POWR - RemovePowerConsumer(): network = %v, frame = %d, consumer = %v", this, FrameCounter(), consumer);
	// Remove consumer from the list of waiting consumers if it is in there.
	for (var index = GetLength(lib_power.waiting_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.waiting_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// Remove the consumer from the list.
		RemoveArrayIndex(lib_power.waiting_consumers, index);
		return;
	}
	// Otherwise, remove consumer from the list of active consumers if it is in there.
	for (var index = GetLength(lib_power.active_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.active_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// Increase the power balance and remove the consumer from the list.
		lib_power.power_balance += link.cons_amount;
		RemoveArrayIndex(lib_power.active_consumers, index);
		// Check the power balance of this network, since a change has been made.
		CheckPowerBalance();
		VisualizePowerChange(link.obj, link.cons_amount, 0, true);
		return;
	}
	// If found in neither lists just return without doing anything.
	return;
}

// Checks the power balance after a change to this network: i.e. removal or addition
// of a consumer or producer. The producers and consumers will be refreshed such that 
// the ones with highest priority will be active.
private func CheckPowerBalance()
{
	// First refresh the consumers based on the total power capacity of the network.
	var available_power = GetPowerAvailable();
	// Debugging logs.
	LogState(Format("balance_start av_power = %d", available_power));
	RefreshConsumers(available_power);
	
	// Then calculate the need for power production and refresh the producers.	
	var power_production_need = GetPowerProductionNeed();
	RefreshProducers(power_production_need);	
	// Debugging logs.
	LogState(Format("balance_end needed_power = %d", power_production_need));
	return;
}

// Returns the total power available in the network: idle + active producers.
private func GetPowerAvailable()
{
	var total = 0;
	var all_producers = Concatenate(lib_power.idle_producers, lib_power.active_producers);
	for (var index = GetLength(all_producers) - 1; index >= 0; index--)
	{
		var link = all_producers[index];
		if (!link)
			continue;
		total += link.prod_amount;
	}
	return total;
}

// Activates consumers according to priotrity from all consumers in the network until available power is used.
// This function automatically deactivates consumer which had a lower priority over newly activated ones.
private func RefreshConsumers(int power_available)
{
	// Debugging logs.
	Log("POWR - RefreshConsumers(): network = %v, frame = %d, power_available = %d", this, FrameCounter(), power_available);
	var power_used = 0;
	var all_consumers = Concatenate(lib_power.waiting_consumers, lib_power.active_consumers);
	if (GetLength(all_consumers) > 1) // TODO: this check should not be necessary.
		SortArrayByProperty(all_consumers, "priority");
	for (var index = GetLength(all_consumers) - 1; index >= 0; index--)
	{
		var link = all_consumers[index];
		if (!link)
			continue;
		// Too much power has been used, check if this link was active, if so remove from active.	
		if (power_used + link.cons_amount > power_available)
		{
			var idx = GetIndexOf(lib_power.active_consumers, link);
			if (idx != -1)
			{
				PushBack(lib_power.waiting_consumers, link);
				lib_power.power_balance += link.cons_amount;
				RemoveArrayIndex(lib_power.active_consumers, idx);
				// On not enough power callback to the deactivated consumer.
				link.obj->OnNotEnoughPower(link.cons_amount);
				VisualizePowerChange(link.obj, link.cons_amount, 0, true);
			}
		}
		// In the other case see if consumer is not yet active, if so activate.
		else
		{
			power_used += link.cons_amount;
			var idx = GetIndexOf(lib_power.waiting_consumers, link);
			if (idx != -1)
			{
				PushBack(lib_power.active_consumers, link);
				lib_power.power_balance -= link.cons_amount;
				RemoveArrayIndex(lib_power.waiting_consumers, idx);
				// On enough power callback to the activated consumer.
				link.obj->OnEnoughPower(link.cons_amount);
				VisualizePowerChange(link.obj, 0, link.cons_amount, false);
			}		
		}	
	}
	return;
}

// Returns the amount of power the currently active power consumers need.
private func GetPowerProductionNeed()
{
	var total = 0;
	for (var index = GetLength(lib_power.active_consumers) - 1; index >= 0; index--)
	{
		var link = lib_power.active_consumers[index];
		if (!link)
			continue;
		total += link.cons_amount;
	}
	return total;	
}

// Activates producers according to priotrity from all producers in the network until needed power is met.
// This function automatically deactivates producers which had a lower priority over newly activated ones.
private func RefreshProducers(int power_need)
{
	// Debugging logs.
	Log("POWR - RefreshProducers(): network = %v, frame = %d, power_need = %d", this, FrameCounter(), power_need);
	var power_found = 0;
	var all_producers = Concatenate(lib_power.idle_producers, lib_power.active_producers);
	if (GetLength(all_producers) > 1) // TODO: this check should not be necessary.
		SortArrayByProperty(all_producers, "priority");
	for (var index = GetLength(all_producers) - 1; index >= 0; index--)
	{
		var link = all_producers[index];
		if (!link)
			continue;
		// Still need for a new producer, check is the link was not already active, if so activate.
		if (power_found < power_need)
		{
			power_found += link.prod_amount;
			var idx = GetIndexOf(lib_power.idle_producers, link);
			if (idx != -1)
			{
				PushBack(lib_power.active_producers, link);
				lib_power.power_balance += link.prod_amount;
				RemoveArrayIndex(lib_power.idle_producers, idx);
				// On production start callback to the activated producer.
				link.obj->OnPowerProductionStart(link.prod_amount);
				VisualizePowerChange(link.obj, 0, link.prod_amount, false);
			}
		}
		// No need to activate producers anymore, check if producer is active, if so deactivate.
		else
		{
			var idx = GetIndexOf(lib_power.active_producers, link);
			// It is not possible to deactivate a steady power producer.
			if (idx != -1 && !link.obj->IsSteadyPowerProducer())
			{
				PushBack(lib_power.idle_producers, link);
				lib_power.power_balance -= link.prod_amount;
				RemoveArrayIndex(lib_power.active_producers, idx);
				// On production stop callback to the deactivated producer.
				link.obj->OnPowerProductionStop();
				VisualizePowerChange(link.obj, link.prod_amount, 0, false);
			}
		}
	}
	return;
}


/*-- Network State --*/

// Returns whether the network does not control any power nodes.
public func IsEmpty()
{
	return GetLength(lib_power.idle_producers) == 0
		&& GetLength(lib_power.active_producers) == 0
		&& GetLength(lib_power.waiting_consumers) == 0
		&& GetLength(lib_power.active_consumers) == 0;
}


/*-- Logging --*/

private func LogState(string tag)
{
	Log("==========================================================================");
	Log("POWR - State for network %v in frame %d with tag %s", this, FrameCounter(), tag);
	Log("==========================================================================");
	Log("POWR - lib_power.neutral_network: %v", lib_power.neutral_network);
	Log("POWR - lib_power.idle_producers: %v", lib_power.idle_producers);
	Log("POWR - lib_power.active_producers: %v", lib_power.active_producers);
	Log("POWR - lib_power.waiting_consumers: %v", lib_power.waiting_consumers);
	Log("POWR - lib_power.active_consumers: %v", lib_power.active_consumers);
	Log("POWR - lib_power.power_balance = %d", lib_power.power_balance);
	Log("==========================================================================");
	return;
}


/*-- Power Visualization --*/

// Visualizes the power change on an object from before to to.
private func VisualizePowerChange(object obj, int old_val, int new_val, bool loss)
{
	// Safety: object must exist.
	if (!obj)
		return FatalError("VisualizePowerChange() is called for a non-existing object.");
		
	// Don't do anything if old and new value are the same.
	if (old_val == new_val)
		return;
		
	var before_current = nil;
	var effect = GetEffect("VisualPowerChange", obj);
	if (!effect)
		effect = AddEffect("VisualPowerChange", obj, 1, 5, nil, Library_Power);
	else 
		before_current = effect.current;
	
	var old_abs = Abs(old_val);
	var new_abs = Abs(new_val);
	
	effect.max = Max(old_abs, new_abs);
	effect.current = before_current ?? old_abs;
	effect.to = new_abs;
	
	if (loss)
		effect.back_graphics_name = "Red";
	else 
		effect.back_graphics_name = nil;
	
	if (new_val < 0) 
		effect.graphics_name = "Yellow";
	else if	(new_val > 0) 
		effect.graphics_name = "Green";
	else // off now
	{
		if (old_val < 0)
			effect.graphics_name = "Yellow";
		else 
			effect.graphics_name = "Green";
	}

	EffectCall(obj, effect, "Refresh");
	return;
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
	var bar_properties = {
		size = 1000, 
		bars = effect.max / 10, 
		graphics_name = effect.graphics_name, 
		back_graphics_name = effect.back_graphics_name, 
		image = Icon_Lightbulb, 
		fade_speed = 1	
	};
	
	effect.bar = target->CreateProgressBar(GUI_BarProgressBar, effect.max, effect.current, 35, controller, {x = off_x, y = off_y}, vis, bar_properties);
	
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
