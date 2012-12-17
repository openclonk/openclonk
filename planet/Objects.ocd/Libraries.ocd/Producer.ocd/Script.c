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

// All producers are accessible. 
public func IsContainer() { return true; }

public func IsInteractable() { return GetCon() >= 100; }

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$DescInteraction$", IconName = nil, IconID = nil };
}

// On interaction the production menu should be opened.
public func Interact(object clonk)
{
	var open_menu = clonk->GetMenu();
	// First try to close any other menu, which is open in the clonk.
	if (open_menu)
	{
		var is_prod_menu = open_menu->~IsProductionMenu();
		// Remove the open menu.
		open_menu->RemoveObject();
		clonk->SetMenu(nil);
		// If open_menu is production menu, return and don't open a new one.
		if (is_prod_menu)
			return true;	
	}
	// Open production menu for the caller.
	clonk->CreateProductionMenu(this);
	return true;
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

// This function may be overloaded by the actual producer.
// If set to true, the producer will show every product which is assigned to it instead of checking the knowledge base of its owner.
private func IgnoreKnowledge() { return false; }

/** Determines whether the product specified can be produced. Should be overloaded by the producer.
	@param product_id item's id of which to determine if it is producible.
	@return \c true if the item can be produced, \c false otherwise.
*/
private func IsProduct(id product_id)
{
	return false;
}

/** Returns an array with the ids of products which can be produced at this producer.
	@return array with products.
*/
public func GetProducts(object for_clonk)
{
	var for_plr = GetOwner();
	if (for_clonk)
		for_plr = for_clonk-> GetOwner();
	var products = [];
	// Cycle through all definitions to find the ones this producer can produce.
	var index = 0, product;
	if (!IgnoreKnowledge() && for_plr != NO_OWNER)
	{
		while (product = GetPlrKnowledge(for_plr, nil, index, C4D_Object))
		{
			if (IsProduct(product))
				products[GetLength(products)] = product;
			index++;
		}
		index = 0;
		while (product = GetPlrKnowledge(for_plr, nil, index, C4D_Vehicle))
		{
			if (IsProduct(product))
				products[GetLength(products)] = product;
			index++;
		}
	} 
	else
	{
		while (product = GetDefinition(index))
		{
			if (IsProduct(product))
				products[GetLength(products)] = product;
			index++;	
		}
	}
	return products;
}

/** Determines whether the raw material specified is needed for production. Should be overloaded by the producer.
	@param rawmat_id id of raw material for which to determine if it is needed for production.
	@return \c true if the raw material is needed, \c false otherwise.
*/
public func NeedRawMaterial(id rawmat_id)
{
	return false; // Obsolete for now.
}

/**
	Determines the production costs for an item.
	@param item_id id of the item under consideration.
	@return a list of objects and their respective amounts.
*/
public func ProductionCosts(id item_id)
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
	@param product_id id of the item.
	@param amount the amount of items of \c item_id which should be produced.
	@return current position of the item in the production queue.
*/
public func AddToQueue(id product_id, int amount)
{
	// Check if this producer can produce the requested item.
	if (!IsProduct(product_id))
		return nil;
	var pos = GetLength(queue);
	
	// Check if the same product is in the position before, cause of possible redundancy.
	if (amount != nil && pos > 0 && queue[pos-1].Product == product_id)
		queue[pos-1].Amount += amount;
	// Otherwise create a new entry in the queue.
	else	
		queue[pos] = { Product = product_id, Amount = amount };
	// Notify all production menus open for this producer.
	for (var menu in FindObjects(Find_ID(Library_ProductionMenu), Find_Func("HasCommander", this)))
		menu->UpdateMenuQueue(this);		
	return pos;
}

/** Inserts an item into the production queue at the specified position.
	@param product_id id of the item.
	@param amount the amount of items of \c item_id which should be inserted.
	@param pos the position at which the object should be inserted, the rest of the queue is shifted downwards.
	@return current position of the item in the production queue.
*/
public func InsertIntoQueue(id product_id, int amount, int pos)
{
	// Check if this producer can produce the requested item.
	if (!IsProduct(product_id))
		return nil;
	
	// Make sure the position is valid.
	BoundBy(pos, 0, GetLength(queue) - 1);

	// Check if there is the same product at that position, for a possible merge.
	if (amount != nil && queue[pos].Product == product_id)
		queue[pos].Amount += amount;
	// Check if there is the same product at the position before, for a possible merge.
	else if (amount != nil && pos > 0 && queue[pos-1].Product == product_id)
		queue[pos-1].Amount += amount;
	// Otherwise insert a new item into the queue.
	else
	{
		// First shift all queue items from that position up by one.
		var length = GetLength(queue);
		for (var i = length; i > pos; i--)
			queue[i] = queue[i-1];
		// Then create new item.
		queue[pos] = { Product = product_id, Amount = amount };
	}
	// Notify all production menus open for this producer.
	for (var menu in FindObjects(Find_ID(Library_ProductionMenu), Find_Func("HasCommander", this)))
		menu->UpdateMenuQueue(this);		
	return pos;
}

/** Removes a item or some of it from from the production queue.
	@param pos position of the item in the queue.
	@param amount the amount of this item which should be removed.
	@return \c nil.
*/
public func RemoveFromQueue(int pos, int amount)
{
	var length = GetLength(queue);
	// Safety, pos out of reach.
	if (pos > length - 1)
		return;
	// Reduce and check amount.
	queue[pos].Amount -= amount;
	// If amount <= 0, remove item from queue.
	// From pos onwards queue items should be shift downwards.
	if (queue[pos].Amount <= 0)
	{
		for (var i = pos; i < GetLength(queue); i++)
			queue[i] = queue[i+1];
		// Reduce queue size by one.
		SetLength(queue, length - 1);
		// Check if the removed product was in between equal products, cause of a new possible redundancy.
		if (pos > 0 && pos <= length - 2)
		{
			if (queue[pos-1].Product == queue[pos].Product)
			{
				queue[pos-1].Amount += queue[pos].Amount;
				for (var i = pos; i < GetLength(queue); i++)
					queue[i] = queue[i+1];
				// Reduce queue size by one.
				SetLength(queue, length - 2);				
			}
		}		
	}
	// Notify all production menus open for this producer.
	for (var menu in FindObjects(Find_ID(Library_ProductionMenu), Find_Func("HasCommander", this)))
		menu->UpdateMenuQueue(this);
	return;
}

/** Clears the complete production queue.
	@param abort determines whether to abort the current production process.
	@return \c nil.
*/
public func ClearQueue(bool abort)
{
	if (abort)
	{
		queue = [];
		return;
	}
	//
	
	
	return;
}

/** Returns the current queue.
	@return an array containing the queue elements (.Product for id, .Amount for amount).
*/
public func GetQueue()
{
	return queue;
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
	if (!queue[0])
		return 1;
	
	// Produce first item in the queue.
	var product_id = queue[0].Product;
	var amount = queue[0].Amount;
	// Check raw material need.
	if (!CheckMaterial(product_id))
	{
		// No material available? request from cable network.
		RequestMaterial(product_id);
		return 1;
	}
	// Start the item production.
	if (!Produce(product_id))
		return 1;

	// Update amount and or queue.
	if (amount == nil)
		return 1;
		
	// Update queue, reduce amount.
	RemoveFromQueue(0, 1);
	
	// Done with production checks.
	return 1;
}

/*-- Production --*/

// These functions may be overloaded by the actual producer.
private func ProductionTime(id product) { return product->~GetProductionTime(); }
private func FuelNeed(id product) { return product->~GetFuelNeed(); }
private func LiquidNeed(id product) { return product->~GetLiquidNeed(); }
private func MaterialNeed(id product) { return product->~GetMaterialNeed(); }

private func PowerNeed() { return 200; }

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
	// Check need for materials.
	if (!CheckMaterials(product))
		return false;
	// Check need for power.
	if (!CheckForPower())
		return false;

	// Everything available? Start production.
	// Remove needed components, fuel and liquid.
	// Power will be substracted during the production process.
	CheckComponents(product, true);
	CheckFuel(product, true);
	CheckLiquids(product, true);
	CheckMaterials(product, true);
	
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
		// Find fuel in this producer.
		for (var fuel in FindObjects(Find_Container(this), Find_Func("IsFuel")))
			fuel_amount += fuel->~GetFuelAmount();
		if (fuel_amount < FuelNeed(product))
			return false;
		else if (remove)
		{
			// Remove the fuel needed.
			fuel_amount = 0;
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
	var liq_need = LiquidNeed(product);
	if (liq_need)
	{
		var liquid_amount = 0;
		var liquid = liq_need[0];
		var need = liq_need[1];
		// Find liquid containers in this producer.
		for (var liq_container in FindObjects(Find_Container(this), Find_Func("IsLiquidContainer")))
			if (liq_container->~GetBarrelMaterial() == liquid)
				liquid_amount += liq_container->~GetFillLevel();
		if (liquid_amount < need)
			return false;
		else if (remove)
		{
			// Remove the liquid needed.
			var extracted = 0;
			for (var liq_container in FindObjects(Find_Container(this), Find_Func("IsLiquidContainer")))
			{
				var val = liq_container->~GetLiquid(liquid, need - extracted);
				extracted += val[1];
				if (extracted >= need)
					break;			
			}			
		}		
	}
	return true;
}

private func CheckMaterials(id product, bool remove)
{
	var mat_need = MaterialNeed(product);
	if (mat_need)
	{
		var material_amount = 0;
		var material = mat_need[0];
		var need = mat_need[1];
		// Find liquid containers in this producer.
		for (var mat_container in FindObjects(Find_Container(this), Find_Func("IsMaterialContainer")))
			if (mat_container->~GetContainedMaterial() == material)
				material_amount += mat_container->~GetFillLevel();
		if (material_amount < need)
			return false;
		else if (remove)
		{
			// Remove the material needed.
			var extracted = 0;
			for (var mat_container in FindObjects(Find_Container(this), Find_Func("IsMaterialContainer")))
			{
				var val = mat_container->~RemoveContainedMaterial(material, need - extracted);
				extracted += val;
				if (extracted >= need)
					break;
			}
		}
	}
	return true;
}

private func CheckForPower()
{
	return true; // always assume that power is available
}

private func IsProducing()
{
	if (GetEffect("ProcessProduction", this))
		return true;
	return false;
}

protected func FxProcessProductionStart(object target, proplist effect, int temporary, id product)
{
	if (temporary)
		return 1;
		
	// Set product.
	effect.Product = product;
		
	// Set production duration to zero.
	effect.Duration = 0;
	
	// Production is active.
	effect.Active = true;

	// Callback to the producer.
	this->~OnProductionStart(effect.Product);
	
	// consume power
	if(PowerNeed() > 0)
		MakePowerConsumer(PowerNeed());
	
	return 1;
}

public func OnNotEnoughPower()
{
	var effect = GetEffect("ProcessProduction", this);
	if(effect)
	{
		effect.Active = false;
		this->~OnProductionHold(effect.Product, effect.Duration);
	} 
	else
		FatalError("Producer effect removed when power still active!");
	return _inherited(...);
}

public func OnEnoughPower()
{
	var effect = GetEffect("ProcessProduction", this);
	if(effect)
	{
		effect.Active = true;
		this->~OnProductionContinued(effect.Product, effect.Duration);
	} 
	else 
		FatalError("Producer effect removed when power still active!");
	return _inherited(...);
}

protected func FxProcessProductionTimer(object target, proplist effect, int time)
{
	if (!effect.Active)
		return 1;
	
	// Add effect interval to production duration.
	effect.Duration += effect.Interval;
	
	//Log("Production in progress on %i, %d frames, %d time", effect.Product, effect.Duration, time);
	
	// Check if production time has been reached.
	if (effect.Duration >= ProductionTime(effect.Product))
		return -1;
	
	return 1;
}

protected func FxProcessProductionStop(object target, proplist effect, int reason, bool temp)
{
	if(temp) return;
	
	// no need to consume power anymore
	UnmakePowerConsumer();
		
	if (reason != 0)
		return 1;
		
	// Callback to the producer.
	//Log("Production finished on %i after %d frames", effect.Product, effect.Duration);
	this->~OnProductionFinish(effect.Product);
	// Create product. 	
	var product = CreateObject(effect.Product);
	OnProductEjection(product);
	
	return 1;
}

// Standard behaviour for product ejection.
public func OnProductEjection(object product)
{
	// Vehicles in front fo buildings.
	if (product->GetCategory() & C4D_Vehicle)
	{
		var x = GetX();
		var y = GetY() + GetDefHeight()/2 - product->GetDefHeight()/2;
		product->SetPosition(x, y);	
	}
	// Items should stay inside.
	else
		product->Enter(this);
	return;
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
	var obj_id = obj->GetID();
	// Products itself may be collected.
	if (IsProduct(obj_id))
		return false;
		
	// Components of products may be collected.
	for (var product in GetProducts())
	{
		var i = 0, comp_id;
		while (comp_id = GetComponent(nil, i, nil, product))
		{
			if (comp_id == obj_id)
				return false;
			i++;
		}
	}
	// Fuel for products may be collected.
	if (obj->~IsFuel())
	{
		for (var product in GetProducts())
			if (FuelNeed(product) > 0)
				return false;
	}
	// Liquid containers may be collected if a product needs them.
	if (obj->~IsLiquidContainer())
	{
		for (var product in GetProducts())
			if (LiquidNeed(product))
				return false;
	}
	// Material containers may be collected if a product needs them.
	if (obj->~IsMaterialContainer())
	{
		for (var product in GetProducts())
			if (MaterialNeed(product))
				return false;
	}
	return true;
}
