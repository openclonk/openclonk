/**
	Producer
	Library for production facilities. This library handles the automatic production of
	items in structures. The library provides the interface for the player, checks for
	components, need for liquids or fuel and power. Then handles the production process and may in the future
	provide an interface with other systems (e.g. railway).
	
	Assumes that a possible power system offers the following functions:
	- IsPowerConsumer())
	- RegisterPowerRequest(int amount)
	- UnregisterPowerRequest()
	and issues the following callbacks:
	- OnEnoughPower()
	- OnNotEnoughPower

	@author Maikel
*/

/*
	Properties of producers:
	* Storage of producers:
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
// Contains proplists of format {Product = <objid>, Amount = <int>, Infinite = (optional)<bool>, ProducingPlayer = (optional)<int>}. /Infinite/ == true -> infinite production.
local queue;

// Possibly connected cable station
local cable_station;

func Initialize()
{
	queue = [];
	AddTimer("ProcessQueue", 10);
	return _inherited(...);
}

/*-- Player interface --*/

public func IsProducer() { return true; }

// All producers are accessible.
public func IsContainer() { return true; }

// Provides an own interaction menu, even if it wouldn't be a container.
public func HasInteractionMenu() { return true; }


public func GetProductionMenuEntries(object clonk)
{
	var products = GetProducts(clonk);

	// default design of a control menu item
	var control_prototype =
	{
		BackgroundColor = {Std = 0, Selected = RGB(100, 30, 30)},
		OnMouseIn = GuiAction_SetTag("Selected"),
		OnMouseOut = GuiAction_SetTag("Std")
	};

	var custom_entry =
	{
		Right = "3em", Bottom = "2em",
		image = {Right = "2em", Prototype = control_prototype, Style = GUI_TextBottom | GUI_TextRight}
	};

	var menu_entries = [];
	var index = 0;
	for (var product in products)
	{
		++index;
		// Check if entry is already in queue.
		var info;
		for (info in queue)
		{
			if (info.Product == product) break;
			info = nil;
		}
		// Prepare menu entry.
		var entry = new custom_entry
		{
			image = new custom_entry.image{},
			remove = new control_prototype
			{
				Left = "2em", Bottom = "1em",
				Symbol = Icon_Cancel,
				Tooltip = "$QueueRemove$",
				disabled = {BackgroundColor = RGBa(0, 0, 0, 150)}
			}
		};

		entry.image.Symbol = product;
		if (info) // Currently in queue?
		{
			if (info.Infinite)
			{
				entry.image.infinity =
				{
					Top = "1em", Left = "1em",
					Symbol = Icon_Number,
					GraphicsName = "Inf",
				};
			}
			else // normal amount
				entry.image.Text = Format("%dx", info.Amount);
			entry.remove.OnClick = GuiAction_Call(this, "ModifyProduction", {Product = product, Amount = -1});
			entry.remove.disabled = nil;
			entry.BackgroundColor = RGB(50, 100, 50);
		}

		entry.Priority = 1000 * product->GetValue() + index; // Sort by (estimated) value and then by index.
		entry.Tooltip = product->GetName();
		entry.image.OnClick = GuiAction_Call(this, "ModifyProduction", {Product = product, Amount = +1});
		PushBack(menu_entries, {symbol = product, extra_data = nil, custom = entry});
	}

	// Below the symbols, we leave some space for a progress bar to indicate the current product progress.
	var entry =
	{
		Bottom = "1em", BackgroundColor = RGBa(0, 0, 0, 50),
		Priority = 999998,
		bar =
		{
			BackgroundColor = RGBa(200, 200, 200, 100),
			Right = "0%"
		}
	};
	var updating_effect = AddEffect("IntUpgradeProductProgressBar", this, 1, 2, this);
	PushBack(menu_entries, {symbol = nil, extra_data = nil, custom = entry, fx = updating_effect});
	// At the bottom of the menu, we add some helpful information about the additional features.
	entry =
	{
		Style = GUI_TextBottom | GUI_FitChildren,
		Bottom = "1em", BackgroundColor = RGBa(0, 0, 0, 100),
		Priority = 999999,
		Text = Format("<c 666666>%s + $Click$: $InfiniteProduction$</c>", GetPlayerControlAssignment(clonk->GetOwner(), CON_ModifierMenu1, true))
	};

	PushBack(menu_entries, {symbol = nil, extra_data = nil, custom = entry});
	return menu_entries;
}


public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	var prod_menu =
	{
		title = "$Production$",
		entries_callback = this.GetProductionMenuEntries,
		callback = nil, // The callback is handled internally. See GetProductionMenuEntries.
		callback_hover = "OnProductHover",
		callback_target = this,
		BackgroundColor = RGB(0, 0, 50),
		Priority = 20
	};
	PushBack(menus, prod_menu);

	return menus;
}


public func OnProductHover(symbol, extra_data, desc_menu_target, menu_id)
{
	if (symbol == nil) return;

	var new_box =
	{
		Text = Format("%s:|%s", symbol.Name, symbol.Description,),
		requirements =
		{
			Top = "100% - 1em",
			Style = GUI_TextBottom
		}
	};

	var product_id = symbol;
	var costs = ProductionCosts(product_id);
	var cost_message = "";
	for (var cost_options in costs)
	{
		if (GetLength(cost_options) == 0) // Must be an array with entries
		{
			continue;
		}
		if (GetLength(cost_message))
		{
			cost_message = Format("%s +", cost_message);
		}

		// Append the original resource cost
		cost_message = Format("%s %s {{%i}}", cost_message, GetCostString(cost_options[0].Amount, CheckComponent(cost_options[0])), cost_options[0].Resource);
		// Append all of its substitutes
		for (var i = 1; i < GetLength(cost_options); ++i)
		{
			cost_message = Format("%s / %s {{%i}}", cost_message, GetCostString(cost_options[i].Amount, CheckComponent(cost_options[i])), cost_options[i].Resource);
		}
	}
	if (this->~FuelNeed(product_id))
	{
		cost_message = Format("%s %s {{Icon_Producer_Fuel}}", cost_message, GetCostString(1, CheckFuel(product_id)));
	}
	if (this->~PowerNeed(product_id))
	{
		cost_message = Format("%s + {{Library_PowerConsumer}}", cost_message);
	}
	new_box.requirements.Text = cost_message;
	GuiUpdate(new_box, menu_id, 1, desc_menu_target);
}


func GetCostString(int amount, bool available)
{
	// Format amount to colored string; make it red if it's not available
	if (available) return Format("%dx", amount);
	return Format("<c ff0000>%dx</c>", amount);
}


public func FxIntUpgradeProductProgressBarOnMenuOpened(object target, effect fx, int main_ID, int entry_ID, proplist menu_target)
{
	fx.main_ID = main_ID;
	fx.entry_ID = entry_ID;
	fx.menu_target = menu_target;
	// Force update on first 'Timer' call.
	fx.is_showing = true;
	EffectCall(target, fx, "Timer");
}


public func FxIntUpgradeProductProgressBarTimer(object target, effect fx, int time)
{
	if (fx.menu_target == nil) return -1;
	// Find (new?) production effect if not already given.
	if (fx.production_effect == nil)
	{
		fx.production_effect = GetEffect("ProcessProduction", this);
		if (fx.production_effect == nil)
		{
			if (fx.is_showing)
			{
				fx.is_showing = false;
				GuiUpdate({Text = "<c 777777>$Producing$: -</c>", bar = {Right = "0%"}}, fx.main_ID, fx.entry_ID, fx.menu_target);
			}
			return FX_OK;
		}
	}

	fx.is_showing = true;
	var max = ProductionTime(fx.production_effect.Product);
	var current = Min(max, fx.production_effect.Duration);
	var percent = 1000 * current / max;
	var percent_string = Format("%d.%d%%", percent / 10, percent % 10);
	GuiUpdate({Text = Format("<c aaaaaa>$Producing$: %s</c>", fx.production_effect.Product->GetName()), bar = {Right = percent_string}}, fx.main_ID, fx.entry_ID, fx.menu_target);
	return FX_OK;
}


/*-- Production  properties --*/

// This function may be overloaded by the actual producer.
// If set to true, the producer will show every product which is assigned to it instead of checking the knowledge base of its owner.
func IgnoreKnowledge() { return false; }


/** Determines whether the product specified can be produced. Should be overloaded by the producer.
	@param product_id item's id of which to determine if it is producible.
	@return \c true if the item can be produced, \c false otherwise.
*/
func IsProduct(id product_id)
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
	{
		for_plr = for_clonk-> GetOwner();
	}
	var products = [];
	// Cycle through all definitions to find the ones this producer can produce.
	var index = 0, product;
	if (!IgnoreKnowledge() && for_plr != NO_OWNER)
	{
		while (product = GetPlrKnowledge(for_plr, nil, index, C4D_Object))
		{
			if (IsProduct(product))
			{
				products[GetLength(products)] = product;
			}
			index++;
		}
		index = 0;
		while (product = GetPlrKnowledge(for_plr, nil, index, C4D_Vehicle))
		{
			if (IsProduct(product))
			{
				products[GetLength(products)] = product;
			}
			index++;
		}
	}
	else
	{
		while (product = GetDefinition(index))
		{
			if (IsProduct(product))
			{
				products[GetLength(products)] = product;
			}
			index++;
		}
	}
	return products;
}


/**
	Determines the production costs for an item.
	
	For each component of the item the function returns an array
	of the original component definition and its possible substitutes.
	Substitutes can be defined in the product, by the callback:
	
	Callback to product ID:
	- GetSubstituteComponent(id component_id, int amount)
	- the function may return one of the following:
	  * an ID; in this case it is assumed, that the component
	           can be substituted with this ID and the original amount
	  * a proplist {Resource = substitude_id, Amount = amount};
	    in this case that proplist defines the new amount.
	  * an array of either ID or proplist as above (can be mixed, but this is not recommended)
	    for multiple substitute options.

	@param item_id id of the item under consideration.
	@return a list of objects and their respective amounts.
	        The list is an array of arrays. Each entry in the list
	        is an array of proplists {Resource = id, Amount = int}
	        for the options, e.g. [{Resource = Ore, Amount = 2}, {Resource = Metal, Amount = 1}].
*/
public func ProductionCosts(id item_id)
{
	/* NOTE: This may be overloaded by the producer */
	var component_list = [];
	var component_id, index = 0;
	while (component_id = item_id->GetComponent(nil, index))
	{
		var amount = item_id->GetComponent(component_id);
		var options = [ProductionCostData(component_id, amount)];
		var substitute = item_id->~GetSubstituteComponent(component_id, amount);
		if (GetType(substitute) == C4V_Array)
		{
			for (var option in substitute)
			{
				var data = ProductionCostData(option, amount);
				if (data)
				{
					PushBack(options, data);
				}
			}
		}
		else
		{
			var data = ProductionCostData(substitute, amount);
			if (data)
			{
				PushBack(options, data);
			}
		}
		component_list[index] = options;
		index++;
	}
	return component_list;
}


func ProductionCostData(component, int amount)
{
	// Assume that it already has the correct format
	if (GetType(component) == C4V_PropList)
	{
		return component;
	}
	// The actual format
	else if (GetType(component) == C4V_Def)
	{
		return {Resource = component, Amount = amount};
	}
	// Invalid value
	else
	{
		return nil;
	}
}


/*-- Production queue --*/


/** Returns the queue index corresponding to a product id or nil.
*/
public func GetQueueIndex(id product_id)
{
	var i = 0, l = GetLength(queue);
	for (;i < l; ++i)
	{
		if (queue[i].Product == product_id) return i;
	}
	return nil;
}


/** Modifies an item in the queue. The index can be retrieved via GetQueueIndex.
	@param position index in the queue
	@param amount change of amount or nil
	@param infinite_production Sets the state of infinite production for the item. Can also be nil to not modify anything.
	@return False if the item was in the queue and has now been removed. True otherwise.
*/
public func ModifyQueueIndex(int position, int amount, bool infinite_production)
{
	// safety
	var queue_length = GetLength(queue);
	if (position >= queue_length) return true;

	var item = queue[position];

	if (infinite_production != nil)
	{
		item.Infinite = infinite_production;
	}
	item.Amount += amount;

	// It might be necessary to remove the item from the queue.
	if (!item.Infinite && item.Amount <= 0)
	{
		// Move all things on the right one slot to the left.
		var index = position;
		while (++index < queue_length)
		{
			queue[index - 1] = queue[index];
		}
		SetLength(queue, queue_length - 1);
		return false;
	}
	return true;
}


/** Adds an item to the production queue.
	@param product_id id of the item.
	@param amount the amount of items of \c item_id which should be produced. Amount must not be negative.
	@paramt infinite whether to enable infinite production.
*/
public func AddToQueue(id product_id, int amount, bool infinite, int producing_player)
{
	// Check if this producer can produce the requested item.
	if (!IsProduct(product_id))
		return nil;
	if (amount < 0)
	{
		FatalError("Producer::AddToQueue called with negative amount.");
	}

	// if the product is already in the queue, just modify the amount
	var found = false;
	for (var info in queue)
	{
		if (info.Product != product_id) continue;
		info.Amount += amount;
		if (infinite != nil) info.Infinite = infinite;
		found = true;
		break;
	}

	// Otherwise create a new entry in the queue.
	if (!found)
	{
		PushBack(queue, { Product = product_id, Amount = amount, Infinite = infinite, ProducingPlayer=producing_player });
	}
	// Notify all production menus open for this producer.
	UpdateInteractionMenus(this.GetProductionMenuEntries);
}


/**
	Shifts the queue one space to the left. The first item will be put in the very right slot.
*/
public func CycleQueue()
{
	if (GetLength(queue) <= 1) return;
	var first = queue[0];
	var queue_length = GetLength(queue);
	for (var i = 1; i < queue_length; ++i)
	{
		queue[i - 1] = queue[i];
	}
	queue[-1] = first;
}


/**
	Clears the complete production queue.
*/
public func ClearQueue(bool abort) // TODO: parameter is never used
{
	queue = [];
	UpdateInteractionMenus(this.GetProductionMenuEntries);
	return;
}


/**
	Modifies a certain production item arbitrarily. This is only used by the interaction menu.
	This also creates a new production order if none exists yet.
	@param info
		proplist with Product, Amount. If the player holds the menu-modifier key, this will toggle infinite production.
*/
func ModifyProduction(proplist info, int player)
{
	if (Hostile(GetOwner(), player)) return;

	var product = info.Product;
	var infinite = GetPlayerControlState(player, CON_ModifierMenu1) != 0;
	var amount = info.Amount;
	var index = GetQueueIndex(product);

	if (index == nil && (amount > 0 || infinite))
	{
		AddToQueue(product, amount, infinite, player);
	}
	else if (index != nil)
	{

		// Toggle infinity?
		if (infinite)
		{
			// Toggle it but always reset the amount to 0.
			// If the production is infinite afterwards, the amount is not displayed anyways. If it's not infinite, it's removed (because the amount is 0).
			ModifyQueueIndex(index, -queue[index].Amount, !queue[index].Infinite);
		}
		else // Just remove (or add) some amount from the queue.
		{
			ModifyQueueIndex(index, amount, false);
		}
	}
	UpdateInteractionMenus(this.GetProductionMenuEntries);
}


/**
	Returns the current queue.
	@return an array containing the queue elements (.Product for id, .Amount for amount).
*/
public func GetQueue()
{
	return queue;
}

func ProcessQueue()
{
	// If target is currently producing, don't do anything.
	if (IsProducing())
	{
		return FX_OK;
	}

	// Wait if there are no items in the queue.
	if (!queue[0])
	{
		return FX_OK;
	}

	// Produce first item in the queue.
	var product_id = queue[0].Product;
	var producing_player = queue[0].ProducingPlayer;
	// Check raw material need.
	if (!Produce(product_id, producing_player))
	{
		// No material available? request from cable network.
		RequestAllMissingComponents(queue[0]);
		// In the meanwhile, just cycle the queue and try the next one.
		CycleQueue();
		return FX_OK;
	}

	// Update queue, reduce amount.
	var is_still_there = ModifyQueueIndex(0, -1);
	// And cycle to enable rotational production of (infinite) objects.
	if (is_still_there)
	{
		CycleQueue();
	}
	// We changed something. Update menus.
	UpdateInteractionMenus(this.GetProductionMenuEntries);
	// Done with production checks.
	return FX_OK;
}


/*-- Production --*/

// These functions may be overloaded by the actual producer.
func ProductionTime(id product) { return product->~GetProductionTime(); }
func FuelNeed(id product) { return product->~GetFuelNeed(); }

public func PowerNeed() { return 80; }

public func GetConsumerPriority() { return 50; }


func Produce(id product, producing_player)
{
	// Already producing? Wait a little.
	if (IsProducing())
	{
		return false;
	}

	// Check if components are available.
	if (!CheckComponents(product))
	{
		return false;
	}
	// Check need for fuel.
	if (!CheckFuel(product))
	{
		return false;
	}
	// Check need for power.
	if (!CheckForPower())
	{
		return false;
	}

	// Everything available? Start production.
	// Remove needed components, fuel and liquid.
	// Power will be substracted during the production process.
	CheckComponents(product, true);
	CheckFuel(product, true);

	// Add production effect.
	AddEffect("ProcessProduction", this, 100, 2, this, nil, product, producing_player);
	return true;
}


func CheckComponents(id product, bool remove)
{
	for (var item in ProductionCosts(product))
	{
		// No cost specified?
		if (GetLength(item) == 0)
		{
			return false;
		}
		var mat_id;
		var mat_cost;

		var found = false;
		// Check all possible resource options
		for (var option in item)
		{
			if (CheckComponent(option))
			{
				mat_id = option.Resource;
				mat_cost = option.Amount;
				found = true;
				break;
			}
		}
		if (!found)
		{
			return false; // Resources are missing.
		}
		if (remove)
		{
			for (var i = 0; i < mat_cost; i++)
			{
				var obj = FindObject(this->Find_AvailableToProducer(), Find_ID(mat_id));
				// First try to remove some objects from the stack.
				if (obj->~IsStackable())
				{
					var num = obj->GetStackCount();
					obj->DoStackCount(-(mat_cost - i));
					i += num - 1; // -1 to offset loop advancement
				}
				else
				{
					obj->RemoveObject();
				}
			}
		}
	}
	return true;
}


public func GetAvailableComponentAmount(id material)
{
	var contents = FindObjects(this->Find_AvailableToProducer(), Find_ID(material));
	// Normal object?
	if (!material->~IsStackable())
	{
		return GetLength(contents);
	}
	// If not, we need to check stacked objects.
	var real_amount = 0;
	for (var obj in contents)
	{
		var count = obj->~GetStackCount() ?? 1;
		real_amount += count;
	}
	return real_amount;
}


public func CheckComponent(component, int amount)
{
	if (GetType(component) == C4V_Def)
	{
		return GetAvailableComponentAmount(component) >= amount;
	}
	else if (GetType(component) == C4V_PropList)
	{
		return CheckComponent(component.Resource, component.Amount);
	}
	else
	{
		FatalError(Format("Unsupported parameter format: %v", component));
		return false;
	}
}


public func CheckFuel(id product, bool remove)
{
	var fuel_needed = FuelNeed(product);
	if (fuel_needed > 0)
	{
		var fuel_amount = 0;
		// Find fuel in this producer.
		for (var fuel in FindObjects(this->Find_AvailableToProducer(), Find_Func("IsFuel")))
		{
			fuel_amount += fuel->~GetFuelAmount();
		}
		if (fuel_amount < fuel_needed)
		{
			return false;
		}
		else if (remove)
		{
			// Convert existing objects.
			for (var fuel in FindObjects(this->Find_AvailableToProducer(), Find_Func("IsFuel")))
			{
				// Extract the fuel amount from stored objects
				var fuel_extracted = fuel->~GetFuelAmount(fuel_needed);

				if (fuel_extracted > 0)
				{
					if (!fuel->~OnFuelRemoved(fuel_extracted)) fuel->RemoveObject();
					fuel_needed -= fuel_extracted;
				}

				// Converted enough? Stop here.
				if (fuel_needed <= 0)
					break;
			}
		}
	}
	return true;
}

/**
	Overloadable function, for finding objects that are available to
	the producer (e.g. certain producers might take objects from their
	surrounding, instead of their inventory).

	Defaults to being contained in the producer.
 */
func Find_AvailableToProducer()
{
	return Find_Container(this);
}


func CheckForPower()
{
	return true; // always assume that power is available
}


func IsProducing()
{
	if (GetEffect("ProcessProduction", this))
		return true;
	return false;
}


func FxProcessProductionStart(object target, proplist effect, int temporary, id product, int producing_player)
{
	if (temporary)
		return FX_OK;

	// Set product information
	effect.Product = product;
	effect.producing_player = producing_player;

	// Set production duration to zero.
	effect.Duration = 0;

	// Production is active.
	effect.Active = true;

	// Callback to the producer.
	this->~OnProductionStart(effect.Product);

	// Consume power by registering as a consumer for the needed amount.
	// But first hold the production until the power system gives it ok.
	// Always register the power request even if power need is zero. The
	// power network handles this correctly and a producer may decide to
	// change its power need during production. Only do this for producers
	// which are power consumers.
	if (this->~IsPowerConsumer())
	{
		this->~RegisterPowerRequest(this->PowerNeed());
	}

	return FX_OK;
}


public func OnNotEnoughPower()
{
	var effect = GetEffect("ProcessProduction", this);
	if (effect)
	{
		effect.Active = false;
		this->~OnProductionHold(effect.Product, effect.Duration);
	}
	else
	{
		FatalError("Producer effect removed when power still active!");
	}
	return _inherited(...);
}


public func OnEnoughPower()
{
	var effect = GetEffect("ProcessProduction", this);
	if (effect)
	{
		effect.Active = true;
		this->~OnProductionContinued(effect.Product, effect.Duration);
	}
	else
	{
		FatalError("Producer effect removed when power still active!");
	}
	return _inherited(...);
}


func FxProcessProductionTimer(object target, proplist effect, int time)
{
	if (!effect.Active)
		return FX_OK;

	// Add effect interval to production duration.
	effect.Duration += effect.Interval;

	// Check if production time has been reached.
	var production_time = ProductionTime(effect.Product);
	if (effect.Duration >= production_time)
	{
		return FX_Execute_Kill;
	}

	// Issue a callback to the producer, where production progress of the product is indicated from 0 to 1000
	// The 1000 mark should never be reached, though, because OnProductionFinish() is called before that.
	this->~OnProductionProgress(effect.Product, BoundBy(effect.Duration * 1000 / production_time, 0, 1000));
	return FX_OK;
}


func FxProcessProductionStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;

	// No need to consume power anymore. Always unregister even if there's a queue left to
	// process, because OnNotEnoughPower relies on it and it gives other producers the chance
	// to get some power. Do not unregister if this producer does not consumer power.
	if (this->~IsPowerConsumer())
	{
		this->~UnregisterPowerRequest();
	}

	if (reason != 0)
	{
		return FX_OK;
	}

	// Callback to the producer.
	this->~OnProductionFinish(effect.Product);
	// Create product.
	var product = CreateObject(effect.Product);
	OnProductEjection(product);
	// Global callback.
	if (product)
	{
		GameCallEx("OnProductionFinished", product, effect.producing_player);
	}
	// Try to process the queue immediately and don't wait for the timer to prevent pauses.
	ProcessQueue();
	return FX_OK;
}


// Standard behaviour for product ejection.
public func OnProductEjection(object product)
{
	// Safety for the product removing itself on construction.
	if (!product)
	{
		return;
	}
	// Vehicles in front of buildings, and objects with special needs as well.
	if (product->GetCategory() & C4D_Vehicle || product->~OnCompletionEjectProduct())
	{
		var x = GetX();
		var y = GetY() + GetDefHeight() / 2 - product->GetDefHeight() / 2;
		product->SetPosition(x, y);
		// Sometimes, there is material in front of the building. Move vehicles upwards in that case
		var max_unstick_range = Max(GetDefHeight() / 5, 5); // 8 pixels for tools workshop
		var y_off = 0;
		while (product->Stuck() && y_off < max_unstick_range)
			product->SetPosition(x, y - ++y_off);
	}
	// Items should stay inside.
	else
	{
		product->Enter(this);
	}
	return;
}


/*-- Cable Network --*/

public func AcceptsCableStationConnection() { return true; }

public func IsNoCableStationConnected() { return !cable_station; }

public func ConnectCableStation(object station)
{
	cable_station = station;
}

/**
	Requests the necessary material from the cable network if available.
*/
public func RequestAllMissingComponents(proplist product)
{
	if (!cable_station)
	{
		return false;
	}

	var item_id = product.Product;
	var amount = product.Amount;
	// Take by batches of five for infinite production.
	// TODO: Can we somehow make this smarter? Take all available from source container?
	if (product.Infinite)
	{
		amount = 5;
	}

	// Request all currently unavailable components.
	for (var item in ProductionCosts(item_id))
	{
		// Must actually define items
		if (GetLength(item) == 0)
		{
			continue;
		}
		// TODO: Smartly request resources? Currently it will only request the first of the possible options (the default components)
		var mat_id = item[0].Resource;
		var mat_cost = item[0].Amount;
		// No way to request liquids currently, player must use pumps instead.
		if (mat_id->~IsLiquid())
		{
			continue;
		}
		var available = GetAvailableComponentAmount(mat_id);
		if (available < mat_cost)
		{
			RequestObject(mat_id, mat_cost - available, amount * mat_cost - available);
		}
	}

	// Also check item fuel need.
	var fuel_needed = FuelNeed(item_id);
	if (fuel_needed > 0)
	{
		// For now just use coal as a fuel.
		var coal_needed = 1 + (fuel_needed - 1) / Coal->GetFuelAmount();
		RequestObject(Coal, coal_needed, amount * coal_needed);
	}
	return true;
}

public func RequestObject(id item_id, int min_amount, int max_amount)
{
	if (cable_station)
	{
		cable_station->AddRequest({type = item_id, min_amount = min_amount, max_amount = max_amount});
	}
	return;
}


/*-- Storage --*/

// Whether an object could enter this storage.
public func IsCollectionAllowed(object item)
{
	// Some objects might just bypass this check
	if (item->~ForceEntry(this))
	{
		return false;
	}
	var item_id = item->GetID();
	// Products itself may be collected.
	if (IsProduct(item_id)) return true;
	var products = GetProducts();
	// Components of products may be collected.
	for (var product in products)
	{
		var i = 0, component_id;
		while (component_id = product->GetComponent(nil, i))
		{
			if (component_id == item_id)
			{
				return true;
			}
			if (product->~GetSubstituteComponent(component_id))
			{
				var subs = product->GetSubstituteComponent(component_id);
				if (GetType(subs) == C4V_Array)
				{
					if (IsValueInArray(subs, item_id))
					{
						return true;
					}
				}
				else if (subs == item_id)
				{
					return true;
				}
			}
			i++;
		}
	}
	// Fuel for products may be collected.
	if (item->~IsFuel())
	{
		for (var product in products)
			if (FuelNeed(product) > 0)
				return true;
	}
	// Convertable liquid objects (ice is the only one so far) may be collected if a product needs them.
	// This uses the queue instead of the product list, because other items may need the original object.
	// This extremely special case is used by the ice object only, and should be removed in my opinion,
	// but it is included for compatibility reasons at the moment.
	// TODO
	//Log("Checking for conversion: queue is %v", queue);
	if (item->~CanConvertToLiquidType())
	{
		for (var queued in queue)
		{
			var product = queued.Product;

			var i = 0, component_id;
			while (component_id = product->GetComponent(nil, i))
			{
				if (component_id->~GetLiquidType() == item->~CanConvertToLiquidType())
				{
					ConvertToLiquid(item);
					return true;
				}
				i++;
			}
		}
	}
	return false;
}


public func RejectCollect(id item_id, object item)
{
	// Is the object a container? If so, try to empty it. Don't empty extra slots.
	if ((item->~IsContainer() && !item->~HasExtraSlot()) || item->~IsLiquidContainer() || item->~IsBucket())
	{
		// this is not optimal, because it grabs everything, even things that should not go into the producer normally:
		// the function GrabContents issues no callbacks - however, please don't change the behavior of GrabContents,
		// the missing callbacks are a very good thing for certain purposes
	 	GrabContents(item);
	}
	// Can we collect the object itself?
	if (IsCollectionAllowed(item))
	{
		return false;
	}
	return true;
}


// Converts a convertable object to the liquid.
// Currently the only convertable object is ice,
// and this functionality may be removed in
// the near future.
// TODO
func ConvertToLiquid(object obj)
{
	var liquid = GetDefinition(obj->CanConvertToLiquidType())->CreateLiquid(obj->GetLiquidAmount());

	if (liquid)
	{
		liquid->Enter(this);
		obj->RemoveObject();
	}
}
