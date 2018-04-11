/**
	Power Library
	Handles the aspects for a single power networks, for each network in a round
	a copy of this library object is created. For each network it is being kept 
	track of which are the idle/active producers and idle/active consumers and 
	power is requested from producers and distributed for consumers according 
	to priority.
	
	Callbacks to the power producers (see producer library for details):
	 * OnPowerProductionStart(int amount)
	 * OnPowerProductionStop(int amount)
	 * GetProducerPriority()
	 * IsSteadyPowerProducer()
	 
	Callbacks to the power consumers (see consumer library for details):
	 * OnEnoughPower(int amount)
	 * OnNotEnoughPower(int amount, bool initial_call)
	 * GetConsumerPriority()
	 * GetActualPowerConsumer()
	 
	The network object keeps track of the producers and consumers in lists.
	 * lib_power.idle_producers (currently not producing power)
	 * lib_power.active_producers (currently producing power)
	 * lib_power.waiting_consumers (waiting for power)
	 * lib_power.active_consumers (supplied consumers)
	Producers are stored according to {obj, prod_amount, priority} and consumers
	according to {obj, cons_amount, priority}.
	
	The power library and its components Library_Producer, Library_Consumer and
	Library_Storage depend on the following other definitions:
	 * StatusSymbol
	
	OPEN TODOS:
	 * Remove all the if (!link) checks, they are not needed in principle but errors arise when they are removed.
	 * Fix overproduction flowing into power storage (producers can be deactivated).
	 * Think about the necessity of global func RedrawAllFlagRadiuses().
	 * Optimize network and flag removal.

	@author Zapper, Maikel
*/


// A static variable is used to store the different power networks.
// This variable is also accessed by the flag library.
static LIB_POWR_Networks;

// All power related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See 
// Construction for which variables are being used.
local lib_power;


// The definition of the power system. You can overload this function
// if you want to use a different power system.
global func GetPowerSystem()
{
	return Library_Power;
}

// Initialize the local variables needed to keep track of each power network.
protected func Construction()
{
	// Initialize the single proplist for the power library.
	if (lib_power == nil)
		lib_power = {};
	// Initialize producer and consumer lists.
	// Lists to keep track of the producers and consumers.
	lib_power.idle_producers = [];
	lib_power.active_producers = [];
	lib_power.waiting_consumers = [];
	lib_power.active_consumers = [];
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
	if (this != GetPowerSystem() || !producer || !producer->~IsPowerProducer())
		return FatalError("RegisterPowerProducer() either not called from definition context or no producer specified.");
	GetPowerSystem()->Init();
	// Find the network for this producer and add it.
	var network = GetPowerNetwork(producer);	
	network->AddPowerProducer(producer, amount, producer->GetProducerPriority());
	return;
}

// Definition call: unregisters a power producer.
public func UnregisterPowerProducer(object producer)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !producer || !producer->~IsPowerProducer())
		return FatalError("UnregisterPowerProducer() either not called from definition context or no producer specified.");
	GetPowerSystem()->Init();
	// Find the network for this producer and remove it.
	var network = GetPowerNetwork(producer);	
	network->RemovePowerProducer(producer);
	return;
}

// Definition call: checks whether producer is registered.
public func IsRegisteredPowerProducer(object producer)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !producer || !producer->~IsPowerProducer())
		return FatalError("IsRegisteredPowerProducer() either not called from definition context or no producer specified.");
	GetPowerSystem()->Init();
	var network = GetPowerNetwork(producer);
	return !!network->GetProducerLink(producer);
}

// Definition call: registers a power consumer with specified amount.
public func RegisterPowerConsumer(object consumer, int amount)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !consumer || !consumer->~IsPowerConsumer())
		return FatalError("RegisterPowerConsumer() either not called from definition context or no consumer specified.");
	GetPowerSystem()->Init();
	// Find the network for this consumer and add it.
	var network = GetPowerNetwork(consumer);	
	network->AddPowerConsumer(consumer, amount, consumer->GetConsumerPriority());
	return;
}

// Definition call: unregisters a power consumer.
public func UnregisterPowerConsumer(object consumer)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !consumer || !consumer->~IsPowerConsumer())
		return FatalError("UnregisterPowerConsumer() either not called from definition context or no consumer specified.");
	GetPowerSystem()->Init();
	// Find the network for this consumer and remove it.
	var network = GetPowerNetwork(consumer);	
	network->RemovePowerConsumer(consumer);
	return;
}

// Definition call: checks whether consumer is registered.
public func IsRegisteredPowerConsumer(object consumer)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !consumer || !consumer->~IsPowerConsumer())
		return FatalError("IsRegisteredPowerConsumer() either not called from definition context or no consumer specified.");
	GetPowerSystem()->Init();
	var network = GetPowerNetwork(consumer);
	return !!network->GetConsumerLink(consumer);
}

// Definition call: transfers a power link from the network it is registered in to
// the network it is currently in (base radius).
public func TransferPowerLink(object link)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !link)
		return FatalError("TransferPowerLink() either not called from definition context or no link specified.");
	// Get the new network for this power link.
	var new_network = GetPowerNetwork(link);
	// Loop over existing networks and find the link.
	var old_network;
	for (var network in LIB_POWR_Networks)
	{
		if (network && network->ContainsPowerLink(link))
		{
			old_network = network;
			break;
		}
	}
	// Only perform a transfer if the link was registered in an old network which is not equal to the new network.
	if (old_network && old_network != new_network)
	{
		var producer = old_network->GetProducerLink(link);
		if (producer)
		{
			old_network->RemovePowerProducer(producer.obj);
			new_network->AddPowerProducer(producer.obj, producer.prod_amount, producer.priority);		
		}
		var consumer = old_network->GetConsumerLink(link);
		if (consumer)
		{
			old_network->RemovePowerConsumer(consumer.obj);
			new_network->AddPowerConsumer(consumer.obj, consumer.cons_amount, consumer.priority);		
		}
	}
	return;
}

// Definition call: updates the network for this power link.
public func UpdateNetworkForPowerLink(object link)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !link)
		return FatalError("UpdateNetworkForPowerLink() either not called from definition context or no link specified.");
	// Find the network for this link and update it.
	var network = GetPowerNetwork(link);
	network->CheckPowerBalance();
	return;
}

// Definition call: gives the power helper object.
public func GetPowerNetwork(object for_obj)
{
	// Definition call safety checks.
	if (this != GetPowerSystem() || !for_obj)
		return FatalError("GetPowerNetwork() either not called from definition context or no object specified.");
		
	Init();
	
	// Get the actual power consumer for this object. This can for example be the elevator for the case.
	var actual;
	while (actual = for_obj->~GetActualPowerConsumer())
	{
		// Stop a possible infinite loop.
		if (actual == for_obj) 
			break;
		for_obj = actual;
	}
	// Get the flag corresponding to the object.	
	var flag = GetFlagpoleForPosition(for_obj->GetX() - GetX(), for_obj->GetY() - GetY());
	
	// Find the network helper object for this flag.
	var helper = nil;
	// If no flag was available the object is neutral and needs a neutral helper.
	if (!flag)
	{
		for (var network in LIB_POWR_Networks)
		{
			if (!network || !network.lib_power.neutral_network) 
				continue;
			helper = network;
			break;
		}
		// Create the helper if it does not exist yet.
		if (helper == nil)
		{
			helper = CreateNetwork(true);
		}
	}
	// Otherwise just get the helper from the flag.
	else
	{
		helper = flag->GetPowerHelper();
		// Create the helper if it does not exist yet.
		if (helper == nil)
		{
			helper = CreateNetwork(false);
			// Add to all linked flags, report errors if the linked flags already have a power helper
			flag->SetPowerHelper(helper, true, true);
		}
	}	
	return helper;
}

// Definition call: Initializes tracking of the power networks.
public func Init()
{
	// Definition call safety checks.
	if (this != GetPowerSystem())
		return;
	// Initialize the list of networks if not done already.
	if (GetType(LIB_POWR_Networks) != C4V_Array)
		LIB_POWR_Networks = [];
	return;
}


// Definition call: Create a new network and add it to the list of networks.
// Can be a neutral network, if desired.
private func CreateNetwork(bool neutral)
{
	Init();
	var network = CreateObject(GetPowerSystemNetwork(), 0, 0, NO_OWNER);
	PushBack(LIB_POWR_Networks, network);
	network.lib_power.neutral_network = neutral;
	return network;
}


// Definition call: Get the type of network helper object to create.
// You can overload this function if you want to use a different
// power system network helper object.
private func GetPowerSystemNetwork()
{
	return Library_Power;
}


// Definition call: Refreshes all power networks (Library_Power objects), so that they 
public func RefreshAllPowerNetworks()
{
	// Don't do anything if there are no power helpers created yet.
	if (GetType(LIB_POWR_Networks) != C4V_Array)
		return;
	
	// Special handling for neutral networks of which there should be at most one.
	var neutral_network_count = 0;
	
	// Do the same for all other helpers: delete / refresh.
	for (var index = GetLength(LIB_POWR_Networks) - 1; index >= 0; index--)
	{
		var network = LIB_POWR_Networks[index];
		if (!network) 
			continue;
		
		if (network->IsEmpty())
		{
			network->RemoveObject();
			RemoveArrayIndex(LIB_POWR_Networks, index);
			continue;
		}

		RefreshPowerNetwork(network);
		if (network.lib_power.neutral_network)
		{
			neutral_network_count += 1;
		}
	}
	
	if (neutral_network_count > 1)
	{
		FatalError(Format("There were a total of %d neutral networks, at most there should be one", neutral_network_count));
	}
	return;
}


// Definition call: Merge all the producers and consumers into their actual networks.
private func RefreshPowerNetwork(object network)
{
	// Merge all the producers and consumers into their actual networks.
	for (var link in Concatenate(network.lib_power.idle_producers, network.lib_power.active_producers))
	{
		if (!link || !link.obj)
			continue;
		var actual_network = GetPowerSystem()->GetPowerNetwork(link.obj);
		if (!actual_network || actual_network == network)
			continue;
		// Remove from old network and add to new network.
		network->RemovePowerProducer(link.obj);
		actual_network->AddPowerProducer(link.obj, link.prod_amount, link.priority);
	}
	for (var link in Concatenate(network.lib_power.waiting_consumers, network.lib_power.active_consumers))
	{
		if (!link || !link.obj)
			continue;
		var actual_network = GetPowerSystem()->GetPowerNetwork(link.obj);
		if (!actual_network || actual_network == network)
			continue;
		// Remove from old network and add to new network.
		network->RemovePowerConsumer(link.obj);
		actual_network->AddPowerConsumer(link.obj, link.cons_amount, link.priority);
	}
	return;
}

/*-- Library Code --*/

// Returns whether this power network is neutral.
public func IsNeutralNetwork()
{
	return lib_power.neutral_network;
}

public func AddPowerProducer(object producer, int amount, int prio)
{
	// Debugging logs.
	//Log("POWR - AddPowerProducer(): network = %v, frame = %d, producer = %v, amount = %d, priority = %d", this, FrameCounter(), producer, amount, prio);
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
	//Log("POWR - RemovePowerProducer(): network = %v, frame = %d, producer = %v", this, FrameCounter(), producer);
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
		RemoveArrayIndex(lib_power.active_producers, index);
		VisualizePowerChange(link.obj, link.prod_amount, 0, false);
		// Notify the active power producer that it should stop producing power.
		producer->OnPowerProductionStop(link.prod_amount);
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
	//Log("POWR - AddPowerConsumer(): network = %v, frame = %d, consumer = %v, amount = %d, priority = %d", this, FrameCounter(), consumer, amount, prio);
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
			lib_power.active_consumers[index] = {obj = consumer, cons_amount = amount, priority = prio};
			// Only visualize the power change if the consumer had a real power need.
			if (link.obj->HasPowerNeed())
				VisualizePowerChange(link.obj, link.cons_amount, amount, false);
			// Check the power balance of this network, since a change has been made.
			CheckPowerBalance();
		}
		// If it is in the list but nothing has changed just return.
		return;
	}
	// Consumer was in neither list, so add it to the list of waiting consumers.
	PushBack(lib_power.waiting_consumers, {obj = consumer, cons_amount = amount, priority = prio});
	// On not enough power callback to not yet active consumer.
	consumer->OnNotEnoughPower(amount, true);
	// Check the power balance of this network, since a change has been made.
	CheckPowerBalance();
	return;
}

public func RemovePowerConsumer(object consumer)
{
	// Debugging logs.
	//Log("POWR - RemovePowerConsumer(): network = %v, frame = %d, consumer = %v", this, FrameCounter(), consumer);
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
		// Remove the consumer from the list.
		RemoveArrayIndex(lib_power.active_consumers, index);
		// Check the power balance of this network, since a change has been made.
		CheckPowerBalance();
		// Only visualize the power change if the consumer had a real power need.
		if (link.obj->HasPowerNeed())
			VisualizePowerChange(link.obj, link.cons_amount, 0, true);
		return;
	}
	// If found in neither lists just return without doing anything.
	return;
}

// Checks the power balance after a change to this network: i.e. removal or addition
// of a consumer or producer. The producers and consumers will be refreshed such that 
// the ones with highest priority will be active.
public func CheckPowerBalance()
{
	// First determine whether the storage links in this network need to be producers
	// or may be consumers. Get the power needed by all non-storage consumers and the 
	// power available by all non-storage producers.
	var needed_power = GetPowerConsumptionNeed();
	// Debugging logs.
	//LogState(Format("balance_start nd_power = %d, av_power = %d", needed_power, GetBarePowerAvailable()));
	// First activate the producers to create the requested power.
	RefreshProducers(needed_power);
	// Then active the consumers according to the freed up power, it might be that
	// power storage was preferred over on-demand producers, which means that no
	// consuming storage will be activated.
	RefreshConsumers(GetActivePowerAvailable());
	// The producers may be underproducing, however, still producing too much for the 
	// active consumer need. Deactivate producers to correct for this.
	PostRefreshProducers(GetActivePowerAvailable() - GetPowerConsumption());
	// Notify other objects depending on the power system the balance has changed.		
	NotifyOnPowerBalanceChange();
	// Debugging logs.
	//LogState(Format("balance_end nd_power = %d, av_power = %d", needed_power, GetActivePowerAvailable()));
	return;
}

// Returns the total power available in the network: idle + active producers.
public func GetPowerAvailable()
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

// Returns the total bare power available in the network: that is the idle
// + active producers which are not power storages.
public func GetBarePowerAvailable()
{
	var total = 0;
	var all_producers = Concatenate(lib_power.idle_producers, lib_power.active_producers);
	for (var index = GetLength(all_producers) - 1; index >= 0; index--)
	{
		var link = all_producers[index];
		if (!link || link.obj->~IsPowerStorage())
			continue;
		total += link.prod_amount;
	}
	return total;
}

// Returns the total active power available in the network.
public func GetActivePowerAvailable(bool exclude_storages)
{
	var total = 0;
	for (var index = GetLength(lib_power.active_producers) - 1; index >= 0; index--)
	{
		var link = lib_power.active_producers[index];
		if (!link || (exclude_storages && link.obj->~IsPowerStorage()))
			continue;
		total += link.prod_amount;
	}
	return total;
}

// Returns the amount of power the currently active power consumers need.
public func GetPowerConsumption(bool exclude_storages)
{
	var total = 0;
	for (var index = GetLength(lib_power.active_consumers) - 1; index >= 0; index--)
	{
		var link = lib_power.active_consumers[index];
		// If the link does not exist or has no power need, just continue.
		if (!link || !link.obj || !link.obj->HasPowerNeed() || (exclude_storages && link.obj->~IsPowerStorage()))
			continue;
		total += link.cons_amount;
	}
	return total;	
}

// Returns the bare amount of power needed by all consumer requests which are not power storages.
public func GetPowerConsumptionNeed()
{
	var total = 0;
	var all_consumers = Concatenate(lib_power.waiting_consumers, lib_power.active_consumers);
	for (var index = GetLength(all_consumers) - 1; index >= 0; index--)
	{
		var link = all_consumers[index];
		// If the link does not exist, is a power storage or has no power need, just continue.
		if (!link || !link.obj || link.obj->~IsPowerStorage() || !link.obj->HasPowerNeed())
			continue;
		total += link.cons_amount;
	}
	return total;	
}

// Returns the total stored power of all power storages in the network (in power frames).
public func GetStoredPower()
{
	var total = 0;
	var all_links = GetAllPowerLinks();
	for (var index = GetLength(all_links) - 1; index >= 0; index--)
	{
		var link = all_links[index];
		if (link && link.obj->~IsPowerStorage())
			total += link.obj->GetStoredPower();
	}
	return total;
}

// Returns the total capacity for storing power of all power storages in the network (in power frames).
public func GetStoredPowerCapacity()
{
	var total = 0;
	var all_links = GetAllPowerLinks();
	for (var index = GetLength(all_links) - 1; index >= 0; index--)
	{
		var link = all_links[index];
		if (link && link.obj->~IsPowerStorage())
			total += link.obj->GetStorageCapacity();
	}
	return total;
}

// Activates producers according to priotrity from all producers in the network until needed power is met.
// This function automatically deactivates producers which had a lower priority over newly activated ones.
private func RefreshProducers(int power_need)
{
	// Debugging logs.
	//Log("POWR - RefreshProducers(): network = %v, frame = %d, power_need = %d", this, FrameCounter(), power_need);
	// Keep track of the power found and which was the last link to satisfy the need. 
	var power_found = 0;
	var satisfy_need_link = nil;
	// First update the priorities of the producers and then sort them according to priority.
	UpdatePriorities(lib_power.idle_producers);
	UpdatePriorities(lib_power.active_producers);
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
			// Update the power found and the last link.
			power_found += link.prod_amount;
			satisfy_need_link = index;
			var idx = GetIndexOf(lib_power.idle_producers, link);
			if (idx != -1)
			{
				PushBack(lib_power.active_producers, link);
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
				RemoveArrayIndex(lib_power.active_producers, idx);
				// On production stop callback to the deactivated producer.
				link.obj->OnPowerProductionStop(link.prod_amount);
				VisualizePowerChange(link.obj, link.prod_amount, 0, false);
			}
		}
	}
	// This procedure might actually have activated too much power and a power producer
	// with high priority but low production might not be necessary, deactivate these.
	for (var index = satisfy_need_link + 1; index < GetLength(all_producers); index++)
	{
		var link = all_producers[index];
		if (!link)
			continue;
		// Power producer is not needed so it can be deactivated.
		if (power_found - link.prod_amount >= power_need)
		{
			var idx = GetIndexOf(lib_power.active_producers, link);
			// It is not possible to deactivate a steady power producer.
			if (idx != -1 && !link.obj->IsSteadyPowerProducer())
			{
				power_found -= link.prod_amount;
				PushBack(lib_power.idle_producers, link);
				RemoveArrayIndex(lib_power.active_producers, idx);
				// On production stop callback to the deactivated producer.
				link.obj->OnPowerProductionStop(link.prod_amount);
				VisualizePowerChange(link.obj, link.prod_amount, 0, false);
			}		
		}	
	}
	return;
}

// Activates consumers according to priotrity from all consumers in the network until available power is used.
// This function automatically deactivates consumer which had a lower priority over newly activated ones.
private func RefreshConsumers(int power_available)
{
	// Debugging logs.
	//Log("POWR - RefreshConsumers(): network = %v, frame = %d, power_available = %d", this, FrameCounter(), power_available);
	// Keep track of the power used.
	var power_used = 0;
	// First update the priorities of the consumers and then sort them according to priority.
	UpdatePriorities(lib_power.waiting_consumers, true);
	UpdatePriorities(lib_power.active_consumers, true);
	var all_consumers = Concatenate(lib_power.waiting_consumers, lib_power.active_consumers);
	if (GetLength(all_consumers) > 1) // TODO: this check should not be necessary.
		SortArrayByProperty(all_consumers, "priority");
	for (var index = GetLength(all_consumers) - 1; index >= 0; index--)
	{
		var link = all_consumers[index];
		if (!link || !link.obj)
			continue;
		// Determine the consumption of this consumer, taking into account the power need.
		var consumption = link.cons_amount;
		if (!link.obj->HasPowerNeed())
			consumption = 0;			
		// Too much power has been used, check if this link was active, if so remove from active.
		// Or if the links is a power storage and there is other storage actively producing remove as well.
		if (power_used + consumption > power_available || (link.obj->~IsPowerStorage() && HasProducingStorage()))
		{
			var idx = GetIndexOf(lib_power.active_consumers, link);
			if (idx != -1)
			{
				PushBack(lib_power.waiting_consumers, link);
				RemoveArrayIndex(lib_power.active_consumers, idx);
				// On not enough power callback to the deactivated consumer.
				link.obj->OnNotEnoughPower(consumption, false);
				VisualizePowerChange(link.obj, consumption, 0, true);
			}
		}
		// In the other case see if consumer is not yet active, if so activate.
		else
		{
			power_used += consumption;
			var idx = GetIndexOf(lib_power.waiting_consumers, link);
			if (idx != -1)
			{
				PushBack(lib_power.active_consumers, link);
				RemoveArrayIndex(lib_power.waiting_consumers, idx);
				// On enough power callback to the activated consumer.
				link.obj->OnEnoughPower(consumption);
				VisualizePowerChange(link.obj, 0, consumption, false);
			}		
		}	
	}
	return;
}

// Deactivate superfluous producers which were not needed to match the current consumption.
private func PostRefreshProducers(int free_power)
{
	// Debugging logs.
	//Log("POWR - PostRefreshProducers(): network = %v, frame = %d, free_power = %d", this, FrameCounter(), free_power);
	if (free_power <= 0)
		return;
	var power_freed = 0;
	// First update the priorities of the active producers and then sort them according to priority.
	UpdatePriorities(lib_power.active_producers);
	if (GetLength(lib_power.active_producers) > 1) // TODO: this check should not be necessary.
		SortArrayByProperty(lib_power.active_producers, "priority", true);
	// Loop over all active power producers and free up according to priority.
	for (var index = GetLength(lib_power.active_producers) - 1; index >= 0; index--)
	{
		var link = lib_power.active_producers[index];
		if (!link)
			continue;
		// Check if power producer is not needed and if so deactivate it.
		if (power_freed + link.prod_amount <= free_power)
		{
			var idx = GetIndexOf(lib_power.active_producers, link);
			// It is not possible to deactivate a steady power producer.
			if (idx != -1 && !link.obj->IsSteadyPowerProducer())
			{
				power_freed += link.prod_amount;
				PushBack(lib_power.idle_producers, link);
				RemoveArrayIndex(lib_power.active_producers, idx);
				// On production stop callback to the deactivated producer.
				link.obj->OnPowerProductionStop(link.prod_amount);
				VisualizePowerChange(link.obj, link.prod_amount, 0, false);
			}		
		}	
	}
	return;
}

// Returns whether network has power storage which is producing power for the network.
private func HasProducingStorage()
{
	for (var index = GetLength(lib_power.active_producers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.active_producers[index];
		if (!link) 
			continue;
		if (link.obj->~IsPowerStorage())
			return true;
	}
	return false;
}

// Returns whether network has power storage which is consuming power from the network.
private func HasConsumingStorage()
{
	for (var index = GetLength(lib_power.active_consumers) - 1; index >= 0; index--)
	{ 
		var link = lib_power.active_consumers[index];
		if (!link) 
			continue;
		if (link.obj->~IsPowerStorage())
			return true;
	}
	return false;
}

// Updates the priorities of either a list of consumers or producers.
private func UpdatePriorities(array link_list, bool for_consumers)
{
	for (var link in link_list)
	{
		if (!link || !link.obj)
			continue;
		if (for_consumers)
			link.priority = link.obj->~GetConsumerPriority();
		else
			link.priority = link.obj->~GetProducerPriority();
	}
	return;
}

// Called when the power balance of this network changes: notify other objects depending on this.
private func NotifyOnPowerBalanceChange()
{
	// Notify all power display objects a balance change has occured.
	for (var display_obj in FindObjects(Find_Func("IsPowerDisplay")))
	{
		if (GetPowerSystem()->GetPowerNetwork(display_obj) == this)
			display_obj->~OnPowerBalanceChange(this);
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

// Returns whether this network contains a power link.
public func ContainsPowerLink(object link)
{
	return !!GetProducerLink(link) || !!GetConsumerLink(link);
}

// Returns the producer link in this network.
public func GetProducerLink(object producer)
{
	for (var test_link in Concatenate(lib_power.idle_producers, lib_power.active_producers))
		if (test_link.obj == producer)
			return test_link;
	return;
}

// Returns the consumer link in this network.
public func GetConsumerLink(object consumer)
{
	for (var test_link in Concatenate(lib_power.waiting_consumers, lib_power.active_consumers))
		if (test_link.obj == consumer)
			return test_link;
	return;
}

// Returns a list of all the power links in this network.
public func GetAllPowerLinks()
{
	// Combine producers and consumers into a list of all links.
	var all_producers = Concatenate(lib_power.idle_producers, lib_power.active_producers);
	var all_consumers = Concatenate(lib_power.waiting_consumers, lib_power.active_consumers);
	var all_links = Concatenate(all_producers, all_consumers);
	// Remove duplicate entries with the same link object.
	for (var index = GetLength(all_links) - 1; index >= 1; index--)
	{
		var obj = all_links[index].obj;
		for (var test_index = index - 1; test_index >= 0; test_index--)
		{
			if (obj == all_links[test_index].obj)
			{
				RemoveArrayIndex(all_links, index);
				break;
			}
		}
	}
	return all_links;
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
	Log("POWR - GetPowerConsumptionNeed() = %d", GetPowerConsumptionNeed());
	Log("POWR - GetBarePowerAvailable() = %d", GetBarePowerAvailable());
	Log("POWR - GetPowerAvailable() = %d", GetPowerAvailable());
	Log("POWR - GetActivePowerAvailable() = %d", GetActivePowerAvailable());
	Log("POWR - GetPowerConsumption() = %d", GetPowerConsumption());
	Log("==========================================================================");
	return;
}

// Definition call: registers a power producer with specified amount.
public func LogPowerNetworks()
{
	// Definition call safety checks.
	if (this != GetPowerSystem())
		return FatalError("LogPowerNetworks() not called from definition context.");
	GetPowerSystem()->Init();
	for (var network in LIB_POWR_Networks)
		if (network) 
			network->LogState("");
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
		effect = AddEffect("VisualPowerChange", obj, 1, 5, nil, GetPowerSystem());
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
		return FX_Execute_Kill;
	if (effect.current == effect.to) 
		return FX_OK;
	
	if (effect.to < effect.current) 
		effect.current = Max(effect.current - 15, effect.to);
	else 
		effect.current = Min(effect.current + 15, effect.to);

	effect.bar->SetValue(effect.current);
	return FX_OK;
}


/*-- Saving --*/

// Helper object should not be saved.
public func SaveScenarioObject()
{
	if (GetID() == GetPowerSystem()) 
		return false;
	return inherited(...);
}
