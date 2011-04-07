/**
	Producer
	Library for types of structures that produce objects.
	Should ideally be included by all producers.
	
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


// Production queue, a list of items to be produced.
local queue;


protected func Initialize()
{
	queue = [];
	AddEffect("ProcessQueue", this, 100, 5, this);
	return _inherited(...);
}


/* Production */

/** Determines whether the item specified can be produced. Should be overloaded by the producer.
	@param item_id item's id of which to determine if it is producible.
	@return \c true if the item can be produced, \c false otherwise.
*/
public func CanProduceItem(id item_id)
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

private func Produce(id item_id)
{

	return false;
}

public func IsProducing()
{
	return false;
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

/* Production queue */

/** Adds an item to the production queue.
	@param item_id id of the item.
	@param amount the amount of items of \c item_id which should be produced.
	@return \c current position of the item in the production queue.
*/
public func AddToQueue(id item_id, int amount)
{
	// Check if this producer can produce the requested item.
	if (!CanProduceItem(item_id))
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

/* Storage */

protected func RejectCollect(id item, object obj)
{
	// Just return RejectEntrance for this object.
	return RejectEntrance(obj);
}

protected func RejectEntrance(object obj)
{
	// Check if item is either a raw material needed or an item that can be produced.
	if (CanProduceItem(obj->GetID()))
		return false;
	if (NeedsRawMaterial(obj->GetID()))
		return false;
	return true;
}
