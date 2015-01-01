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
	 * lib_idle_producers (currently not producing power)
	 * lib_active_producers (currently producing power)
	 * lib_waiting_consumers (waiting for power)
	 * lib_active_consumers (supplied consumers)
	Producers are stored according to {obj, prod_amount, priority} and consumers
	according to {obj, cons_amount, priority}.
	
	OPEN TODOS:
	 * Figure out where the nil's and the errors come from.
	 * Fix the visualization of power on the nodes.
	 * Check the flag library for network merging.
	 * Check the power balance, also with merging.

	@author Zapper, Maikel
*/


// A static variable is used to store the different power networks.
// This variable is also accessed by the flag library.
static LIB_POWR_Networks;

// Lists to keep track of the producers and consumers.
local lib_idle_producers;
local lib_active_producers;
local lib_waiting_consumers;
local lib_active_consumers;

// Main variable which keeps track of the power balance in the network.
local lib_power_balance;

// Whether the network is neutral.
local lib_neutral_network;

// Initialize the local variables needed to keep track of each power network.
protected func Initialize()
{
	// Initialize producer and consumer lists.
	lib_idle_producers = [];
	lib_active_producers = [];
	lib_waiting_consumers = [];
	lib_active_consumers = [];
	// Set power balance to zero.
	lib_power_balance = 0;
	// Network is not neutral by default.
	lib_neutral_network = false;
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
			if (!network || !network.lib_neutral_network) 
				continue;
			helper = network;
			break;
		}
		
		if (helper == nil) // not yet created?
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			helper.lib_neutral_network = true;
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
	LogCallStack();
	// Check if it is not already in the list of idle producers.
	for (var index = GetLength(lib_idle_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_idle_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.prod_amount != amount || link.priority != prio)
		{
			lib_idle_producers[index] = {obj = producer, prod_amount = amount, priority = prio};
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Check if it is not already in the list of active producers.
	for (var index = GetLength(lib_active_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_active_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.prod_amount != amount || link.priority != prio)
		{
			// Adjust the power balance accordingly.
			lib_power_balance += amount - link.prod_amount;
			VisualizePowerChange(producer, link.prod_amount, amount, false);
			lib_active_producers[index] = {obj = producer, prod_amount = amount, priority = prio};
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
		lib_power_balance += amount;
		PushBack(lib_active_producers, {obj = producer, prod_amount = amount, priority = prio});
		VisualizePowerChange(producer, 0, amount, false);
	}
	else
	{
		PushBack(lib_idle_producers, {obj = producer, prod_amount = amount, priority = prio});
	}
	// Check the power balance of this network, since a change has been made.
	CheckPowerBalance();
	return;
}

public func RemovePowerProducer(object producer)
{
	// Debugging logs.
	Log("POWR - RemovePowerProducer(): network = %v, frame = %d, producer = %v", this, FrameCounter(), producer);
	LogCallStack();
	// Remove producer from the list of idle producers if it is in there.
	for (var index = GetLength(lib_idle_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_idle_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// Remove the producer from the list.
		RemoveArrayIndex(lib_idle_producers, index);
		return;
	}
	// Otherwise, remove producer from the list of active producers if it is in there.
	for (var index = GetLength(lib_active_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_active_producers[index];
		if (!link || link.obj != producer) 
			continue;
		// Reduce the power balance and remove the producer from the list.
		lib_power_balance -= link.prod_amount;
		RemoveArrayIndex(lib_active_producers, index);
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
	for (var index = GetLength(lib_waiting_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_waiting_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.cons_amount != amount || link.priority != prio)
		{
			lib_waiting_consumers[index] = {obj = consumer, cons_amount = amount, priority = prio};
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Check if it is not already in the list of active consumers.
	for (var index = GetLength(lib_active_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_active_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// If it is in the list update the values if they have changed.
		if (link.cons_amount != amount || link.priority != prio)
		{
			lib_active_consumers[index] = {obj = consumer, cons_amount = amount, priority = prio};
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Consumer was in neither list, so add it to the list of waiting consumers.
	PushBack(lib_waiting_consumers, {obj = consumer, cons_amount = amount, priority = prio});
	// Check the power balance of this network, since a change has been made.
	CheckPowerBalance();
	return;
}

public func RemovePowerConsumer(object consumer)
{
	// Debugging logs.
	Log("POWR - RemovePowerConsumer(): network = %v, frame = %d, consumer = %v", this, FrameCounter(), consumer);
	// Remove consumer from the list of waiting consumers if it is in there.
	for (var index = GetLength(lib_waiting_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_waiting_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// Remove the consumer from the list.
		RemoveArrayIndex(lib_waiting_consumers, index);
		return;
	}
	// Otherwise, remove consumer from the list of active consumers if it is in there.
	for (var index = GetLength(lib_active_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_active_consumers[index];
		if (!link || link.obj != consumer) 
			continue;
		// Increase the power balance and remove the consumer from the list.
		lib_power_balance += link.cons_amount;
		RemoveArrayIndex(lib_active_consumers, index);
		// Check the power balance of this network, since a change has been made.
		CheckPowerBalance();
		VisualizePowerChange(link.obj, link.cons_amount, 0, true);
		return;
	}
	// If found in neither lists just return without doing anything.
	return;
}

private func CheckPowerBalance()
{
	// First get the possible power surplus in the network from idle producers.
	var surplus = GetPowerSurplus();
	
	// Debugging logs.
	LogState("balance start");
	
	// If the surplus is non-positive it implies that first all possible idle producers
	// need to be activated and then some consumers need to be deactivated.
	if (surplus <= 0 && lib_power_balance < 0)
	{
		// Activate all idle producers.
		ActivateProducers(surplus - lib_power_balance);
		// The power balance may have changed but can't be positive, so disable sufficient consumers.
		DeactivateConsumers(-lib_power_balance);
		// All producers are running and the maximum amount of consumers as well.
		LogState("balance_end s < 0");
		return;
	}
	// Otherwise if the surplus is positive one or some idle consumers may be activated. 
	else if (surplus > 0)
	{
		// Activate waiting consumers according to the surplus and their priorities.
		ActivateConsumers(surplus);
		// These newly activated consumers and the already maybe negative power balance must then 
		// finally be counteracted by activating idle producers according to the new balance.
		// It is also possible that some producers may be deactivated.
		if (lib_power_balance > 0)
			DeactivateProducers(lib_power_balance);	
		else if (lib_power_balance < 0)
			ActivateProducers(-lib_power_balance);
		LogState("balance_end s > 0");
		return;
	}
	// TODO: what about the third case? Should we really do nothing there?
	// TODO: what about swapping consumers with different priority?
	LogState("balance_end s == 0");
	return;
}

// Returns the possible power production from idle producers combined with the current power balance.
private func GetPowerSurplus()
{
	// This is just the sum of the balance and possible power from all idle producers.
	var surplus = lib_power_balance;
	for (var index = GetLength(lib_idle_producers) - 1; index >= 0; index--)
	{
		var link = lib_idle_producers[index];
		if (!link)
			continue;
		surplus += link.prod_amount;
	}
	return surplus;
}

// Activates idle producers according to priority until power need is satisfied.
private func ActivateProducers(int power_need)
{
	// Debugging logs.
	Log("POWR - ActivateProducers(): network = %v, frame = %d, power_need = %d", this, FrameCounter(), power_need);
	var power_found = 0;
	// Sort the idle producers according to priority and then activate producers until balance is restored.
	if (GetLength(lib_idle_producers) > 1) SortArrayByProperty(lib_idle_producers, "priority");
	for (var index = GetLength(lib_idle_producers) - 1; index >= 0; index--)
	{
		var link = lib_idle_producers[index];
		if (!link)
			continue;
		power_found += link.prod_amount;
		PushBack(lib_active_producers, link);
		link.obj->OnPowerProductionStart(link.prod_amount);
		lib_power_balance += link.prod_amount;
		RemoveArrayIndex(lib_idle_producers, index);
		VisualizePowerChange(link.obj, 0, link.prod_amount, false);
		// Stop activatng producers if power need is satisfied.
		if (power_found >= power_need)
			return true;
	}
	return false;
}

// Deactivates idle producers according to priority until power surplus is gone.
private func DeactivateProducers(int power_surplus)
{
	// Debugging logs.
	Log("POWR - DeactivateProducers(): network = %v, frame = %d, power_surplus = %d", this, FrameCounter(), power_surplus);
	var power_killed = 0;
	// Sort the active producers according to priority and deactivate them until balance is restored.
	if (GetLength(lib_active_producers) > 1) SortArrayByProperty(lib_active_producers, "priority", true);
	for (var index = GetLength(lib_active_producers) - 1; index >= 0; index--)
	{
		var link = lib_active_producers[index];
		// It is not possible to deactivate a steady power producer.
		if (link.obj->IsSteadyPowerProducer())
			continue;		
		power_killed += link.prod_amount;
		// Stop deactivating producers if power surplus has been reached.
		if (power_killed > power_surplus)
			break;
		// Move active producer to idle producers.
		PushBack(lib_idle_producers, link);
		link.obj->OnPowerProductionStop();
		lib_power_balance -= link.prod_amount;
		RemoveArrayIndex(lib_active_producers, index);
		VisualizePowerChange(link.obj, link.prod_amount, 0, false);
	}
	return;
}

// Activates waiting consumers according to priotrity until available power is used.
private func ActivateConsumers(int power_available)
{
	// Debugging logs.
	Log("POWR - ActivateConsumers(): network = %v, frame = %d, power_available = %d", this, FrameCounter(), power_available);
	var power_used = 0;
	// Sort the waiting consumers according to priority and then activate consumers until available power is used.
	if (GetLength(lib_waiting_consumers) > 1) SortArrayByProperty(lib_waiting_consumers, "priority");
	for (var index = GetLength(lib_waiting_consumers) - 1; index >= 0; index--)
	{
		var link = lib_waiting_consumers[index];
		// If this consumer requires to much power try one of lower priority.
		if (power_used + link.cons_amount > power_available)
			continue;
		power_used += link.cons_amount;
		PushBack(lib_active_consumers, link);
		link.obj->OnEnoughPower(link.cons_amount);
		lib_power_balance -= link.cons_amount;
		RemoveArrayIndex(lib_waiting_consumers, index);
		VisualizePowerChange(link.obj, 0, link.cons_amount, false);
	}
	return;
}

// Deactivates active consumers according to priority until power need is satisfied.
private func DeactivateConsumers(int power_need)
{
	// Debugging logs.
	Log("POWR - DeactivateConsumers(): network = %v, frame = %d, power_need = %d", this, FrameCounter(), power_need);
	var power_restored = 0;
	// Sort the active consumers according to priority and then deactivate consumers until balance is restored.
	if (GetLength(lib_active_consumers) > 1) SortArrayByProperty(lib_active_consumers, "priority", true);
	for (var index = GetLength(lib_active_consumers) - 1; index >= 0; index--)
	{
		var link = lib_active_consumers[index];
		power_restored += link.cons_amount;
		PushBack(lib_waiting_consumers, link);
		link.obj->OnNotEnoughPower(link.cons_amount);
		lib_power_balance += link.cons_amount;
		RemoveArrayIndex(lib_active_consumers, index);
		VisualizePowerChange(link.obj, link.cons_amount, 0, true);
		// Stop deactivating consumers if enough power has been freed.
		if (power_restored >= power_need)
			return true;
	}
	// This should not ever happen, so put a log here.
	Log("Not enough power consumers to deactivate for restoring the power balance. How could this happen?");
	return false;
}


/*-- Network State --*/

// Returns whether the network does not control any power nodes.
public func IsEmpty()
{
	return GetLength(lib_idle_producers) == 0
		&& GetLength(lib_active_producers) == 0
		&& GetLength(lib_waiting_consumers) == 0
		&& GetLength(lib_active_consumers) == 0;
}


/*-- Logging --*/

private func LogState(string tag)
{
	Log("==========================================================================");
	Log("POWR - State for network %v in frame %d with tag %s", this, FrameCounter(), tag);
	Log("==========================================================================");
	Log("POWR - lib_neutral_network: %v", lib_neutral_network);
	Log("POWR - lib_idle_producers: %v", lib_idle_producers);
	Log("POWR - lib_active_producers: %v", lib_active_producers);
	Log("POWR - lib_waiting_consumers: %v", lib_waiting_consumers);
	Log("POWR - lib_active_consumers: %v", lib_active_consumers);
	Log("POWR - lib_power_balance = %d", lib_power_balance);
	Log("POWR - surplus: %d", GetPowerSurplus());
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
