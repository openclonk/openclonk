/**
	Producer
	Library for production facilities. This library handles the automatic production of 
	items in structures. The library provides the interface for the player, checks for 
	components, need for liquids or fuel and power. Then handles the production process and may in the future
	provide an interface with other systems (e.g. railway).
	
	@author Maikel
*/

/*
	Properties of producers:
	* Storage of producers:
	  * Producers can store sufficient amounts of raw material they need to produce their products.
	  * Producers can store the products they can produce.
	  * Nothing more, nothing less.
	* Production queue:
	  * Producers automatically produce the items in the production queue.
	  * Producible items can be added to the queue, with an amount specified.
	  * Also an infinite amount is possible, equals indefinite production.
	Possible interaction with cable network:
	* Producers request the cable network for raw materials.
	
*/

#include Library_PowerConsumer

// Production queue, a list of items to be produced.
local queue;


protected func Initialize()
{
	queue = [];
	AddEffect("ProcessQueue", this, 100, 5, this);
	return _inherited(...);
}

/*-- Player interface --*/

public func IsInteractable() { return GetCon() >= 100; }

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "Produce items", IconName = nil, IconID = nil };
}

public func Interact(object clonk)
{
	// Open production menu for the caller.
	OpenProductionMenu(clonk);
	return true;
}

private func OpenProductionMenu(object clonk)
{
	// Create menu for the clonk.
	clonk->CreateMenu(GetID(), this, C4MN_Extra_Components, "Production menu");
	// Cycle through all definitions to find the ones this producer can produce.
	var index = 0, product;
	while (product = GetDefinition(index))
	{
		if (IsProduct(product))
			clonk->AddMenuItem(Format("Produce %s", product->GetName()), "OnProductSelection", product);
		index++;	
	}
	return;
}

protected func OnProductSelection(id product, int par, bool alt)
{
	if (!product)
		return;
	
	var amount = nil;
	if (!alt)
		amount = 1;
		
	AddToQueue(product, amount);
	return;
}


/*-- Production  properties --*/

/** Determines whether the product specified can be produced. Should be overloaded by the producer.
	@param product_id item's id of which to determine if it is producible.
	@return \c true if the item can be produced, \c false otherwise.
*/
private func IsProduct(id product_id)
{
	return false;
}

/** Determines whether the raw material specified is needed for production. Should be overloaded by the producer.
	@param rawmat_id id of raw material for which to determine if it is needed for production.
	@return \c true if the raw material is needed, \c false otherwise.
*/
public func NeedsRawMaterial(id rawmat_id)
{
	return false; // Obsolete for now.
}

/**
	Determines the production costs for an item.
	@param item_id id of the item under consideration.
	@return a list of objects and their respective amounts.
*/
private func ProductionCosts(id item_id)
{
	/* NOTE: This may be overloaded by the producer */
	var comp_list = [];
	var comp_id, index = 0;
	while (comp_id = GetComponent(nil, index, nil, item_id))
	{
		var amount = GetComponent(comp_id, index, nil, item_id);
		comp_list[index] = [comp_id, amount];
		index++;		
	}
	return comp_list;
}

/*-- Production queue --*/

/** Adds an item to the production queue.
	@param item_id id of the item.
	@param amount the amount of items of \c item_id which should be produced.
	@return \c current position of the item in the production queue.
*/
public func AddToQueue(id item_id, int amount)
{
	// Check if this producer can produce the requested item.
	if (!IsProduct(item_id))
		return nil;
	var pos = GetLength(queue);
	queue[pos] = [item_id, amount];
	return pos;
}

/** Removes an item from the production queue.
	@param pos position of the item in the queue.
	@return \c nil.
*/
public func RemoveFromQueue(int pos)
{
	var length = GetLength(queue);
	// Safety, pos out of reach.
	if (pos > length - 1)
		return;
	// From pos onwards queue items should be shift downwards.
	for (var i = pos; i < GetLength(queue); i++)
		queue[i] = queue[i+1];
	// Reduce queue size by one.
	SetLength(queue, length - 1);
	return;
}

public func ClearQueue()
{
	// TODO: Implement
	return;
}

protected func FxProcessQueueStart()
{

	return 1;
}

protected func FxProcessQueueTimer(object target, proplist effect)
{
	// If target is currently producing, don't do anything.
	if (IsProducing())
		return 1;

	// Wait if there are no items in the queue.
	var to_produce = queue[0];
	if (!to_produce)
		return 1;
	
	// Produce first item in the queue.
	var item_id = to_produce[0];
	var amount = to_produce[1];
	// Check raw material need.
	if (!CheckMaterial(item_id))
	{
		// No material available? request from cable network.
		RequestMaterial(item_id);
		return 1;
	}
	// Start the item production.
	if (!Produce(item_id))
		return 1;

	// Update amount and or queue.
	if (amount == nil)
		return 1;
	amount--;
	// If amount is zero, remove item from queue.
	if (amount <= 0)
	{
		RemoveFromQueue(0);
		return 1;
	}
	// Update queue, insert new amount.
	queue[0] = [item_id, amount];
	// Done with production checks.
	return 1;
}

/*-- Production --*/

private func ProductionTime() { return 0; }
private func FuelNeed(id product) { return 0; }
private func LiquidNeed(id product) { return 0; }
private func PowerNeed(id product) { return 0; }

private func Produce(id product)
{
	// Already producing? Wait a little.
	if (IsProducing())
		return false;	
		
	// Check if components are available.
	if (!CheckComponents(product))
		return false;	
	// Check need for fuel.
	if (!CheckFuel(product))
		return false;	
	// Check need for liquids.
	if (!CheckLiquids(product))
		return false;	
	// Check need for power.
	if (!CheckForPower(product))
		return false;	

	// Everything available? Start production.
	// Remove needed components, fuel and liquid.
	CheckComponents(product, true);
	CheckFuel(product, true);
	
	// Add production effect.
	AddEffect("ProcessProduction", this, 100, 2, this, nil, product);

	return true;
}

private func CheckComponents(id product, bool remove)
{
	for (var item in ProductionCosts(product))
	{
		var mat_id = item[0];
		var mat_cost = item[1];
		var mat_av = ObjectCount(Find_Container(this), Find_ID(mat_id));
		if (mat_av < mat_cost)
			return false; // Components missing.
		else if (remove)
		{
			for (var i = 0; i < mat_cost; i++)
				 FindObject(Find_Container(this), Find_ID(mat_id))->RemoveObject();
		}
	}
	return true;
}

private func CheckFuel(id product, bool remove)
{
	if (FuelNeed(product) > 0)
	{
		var fuel_amount = 0;
		// Find fuel in producers.
		for (var fuel in FindObjects(Find_Container(this), Find_Func("IsFuel")))
			fuel_amount += fuel->~GetFuelAmount();
		if (fuel_amount < FuelNeed(product))
			return false;
		else if (remove)
		{
			// Remove the fuel needed.
			for (var fuel in FindObjects(Find_Container(this), Find_Func("IsFuel")))
			{
				fuel_amount += fuel->~GetFuelAmount();
				fuel->RemoveObject();
				if (fuel_amount >= FuelNeed(product))
					break;
			}			
		}
	}
	return true;
}

private func CheckLiquids(id product, bool remove)
{
	// TODO
	return true;
}

private func CheckForPower(id product)
{
	if (PowerNeed() > 0)
	{
		// At least ten percent of the power need must be in the network.
		if (!CheckPower(PowerNeed() / 10, true))
			return false;		
	}
	return true;
}

private func IsProducing()
{
	if (GetEffect("ProcessProduction", this))
		return true;
	return false;
}

protected func FxProcessProductionStart(object target, proplist effect, int temporary, id product)
{
	if (temporary != 0)
		return 1;
		
	// Set product.
	effect.Product = product;
		
	// Set production duration to zero.
	effect.Duration = 0;
	
	// Set energy usage to zero.
	effect.Energy = 0;
	
	// Production is active.
	effect.Active = true;
	
	Log("Production started on %i", effect.Product);
	
	// Callback to the producer.
	this->~OnProductionStart(effect.Product);
	
	return 1;
}

protected func FxProcessProductionTimer(object target, proplist effect, int time)
{
	// Check if energy is available.
	if (PowerNeed() > 0)
	{
		var eng = PowerNeed() * (effect.Duration + effect.Interval) / ProductionTime();
		if (CheckPower(eng - effect.Energy))
		{
			// Energy available, add to Energy value and continue production.
			effect.Energy = eng;
			if (!effect.Active)
			{
				this->~OnProductionContinued(effect.Product, effect.Duration);
				effect.Active = true;
			}
		}
		else
		{
			// Hold production if energy is not available, callback to the producer.
			if (effect.Active)
			{
				this->~OnProductionHold(effect.Product, effect.Duration);
				effect.Active = false;
			}
			return 1;
		}
	}
	
	// Add effect interval to production duration.
	effect.Duration += effect.Interval;
	
	Log("Production in progress on %i, %d frames, %d time", effect.Product, effect.Duration, time);
	
	// Check if production time has been reached.
	if (effect.Duration >= ProductionTime())
		return -1;
	
	return 1;
}

protected func FxProcessProductionStop(object target, proplist effect, int reason)
{
	if (reason != 0)
		return 1;
	// Callback to the producer.
	Log("Production finished on %i after %d frames", effect.Product, effect.Duration);
	this->~OnProductionFinish(effect.Product);
	// Create product. 	
	var product = CreateObject(effect.Product);
	this->~OnProductEjection(product);

	return 1;
}

/*-- --*/

/**
	Determines whether there is sufficient material to produce an item.
*/
private func CheckMaterial(id item_id)
{
	for (var item in ProductionCosts(item_id))
	{
		var mat_id = item[0];
		var mat_cost = item[1];
		var mat_av = ObjectCount(Find_Container(this), Find_ID(mat_id));
		if (mat_av < mat_cost)
			return false;
	}
	return true;
}

/**
	Requests the necessary material from the cable network if available.
*/
private func RequestMaterial(id item_id)
{
	for (var item in ProductionCosts(item_id))
	{
		var mat_id = item[0];
		var mat_cost = item[1];
		var mat_av = ObjectCount(Find_Container(this), Find_ID(mat_id));
		if (mat_av < mat_cost)
			RequestObject(mat_id, mat_cost - mat_av);
	}
	return true;
}

// Must exist if Library_CableStation is not included by either this
// library or the structure including this library.
public func RequestObject(id obj_id, int amount)
{
	return _inherited(obj_id, amount, ...);
}

/*-- Storage --*/

protected func RejectCollect(id item, object obj)
{
	// Just return RejectEntrance for this object.
	return RejectEntrance(obj);
}

protected func RejectEntrance(object obj)
{
	// Check if item is either a raw material needed or an item that can be produced.
	if (IsProduct(obj->GetID()))
		return false;
	if (NeedsRawMaterial(obj->GetID()))
		return false;
	return true;
}
