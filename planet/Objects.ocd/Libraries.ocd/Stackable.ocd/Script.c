/*--
	Stackable
	Author: Newton
	
	Including this object means, the object is stackable. Other objects of
	the same type will be added automatically to the object. This functionality
	is similar to the Pack-functionality of the arrows in old clonk titles only
	more general.
	The count of how many objects are stacked together (into a single one, this
	one) is shown in the picture of the object and can be queried and set via
	GetStackCount()/SetStackCount().
	To take one object of the stack, call TakeObject(). As long
	as the object exists, one can always take an object, even if it is the last
	one (self). This object is always outside.
	Infinite stackable count can by achieved using SetInfiniteStackCount.
	
	On entrance (or to be more precise: on RejectEntrance), it will be checked
	if the entering stackable object can be distributed over the other objects
	of the same ID. If yes, this object is deleted and the other object(s) will
	have a higher stack-count.
	
	Example 1:
	'15x Arrow' is about to enter a clonk which has '5x Arrow'. 15 will be added
	to the stack-count of the clonks '5x Arrow'-object (making it '20x Arrow'),
	the entering object will be deleted.
	
	Example 2:
	'17x Arrow' is about to enter a clonk which has '15x Arrow' and a bow with
	'10x Arrow' in it's ammunition slot. 10 will be added to the stack-count
	of the arrows in the bow, 5 to the stack-count of the arrows in the clonk
	(assuming MaxStackCount() is 20) and the original arrows-object will have
	2 arrows left. If there is an inventory slot left, the '2x Arrow" object
	will enter the clonk.
	
	Most objects which can be stacked might want to set different pictures
	and ingame graphics for different counts of objects. This can be done
	by overloading UpdatePicture(), but remember to write _inherited() then.
--*/


local count, count_is_infinite;

// Max size of stack
static const Stackable_Max_Count = 2147483647;
static const Stackable_Max_Display_Count = 999;

// What GetStackCount should return if the count is set to infinite
// Set this to a fairly large number and not e.g. -1, so naive
// implementations that update their graphics by GetStackCount() show a
// bunch of items. However, the number shouldn't be too large so the
// object doesn't get overly heavy.
// Note that count_is_infinite is a separate variable, so we can support
// stacks >999 but <Inf in the future.
static const Stackable_Infinite_Count = 50;

public func IsStackable() { return true; }
public func GetStackCount() { return count; }
public func MaxStackCount() { return 20; }
public func InitialStackCount() { return MaxStackCount(); }
public func IsFullStack() { return this->IsInfiniteStackCount() || (GetStackCount() >= MaxStackCount()); }
public func IsInfiniteStackCount() { return !!count_is_infinite; }

protected func Construction()
{
	count = InitialStackCount();
	return _inherited(...);
}

func Destruction()
{
	NotifyContainer();
	return _inherited(...);
}


/**
 * Puts the stack count of another object on top of this stack.
 * The stack count of the other object is not modified.
 * @par other the other object. Must be of the same ID as the stack.
 * @return the amount of objects that could be stacked.
 */
public func Stack(object other)
{
	if (other->GetID() != GetID()) return 0;
	if (other == this) return 0; 

	// Infinite stacks can always take everything
	if (this->IsInfiniteStackCount()) return other->GetStackCount();
	if (other->~IsInfiniteStackCount())
	{
		SetInfiniteStackCount();
		return other->GetStackCount();
	}

	var howmany = Min(other->GetStackCount(), MaxStackCount() - GetStackCount());
	var future_count = GetStackCount() + howmany;
	//Log("*** Added %d objects to stack", howmany);
	
	if (howmany <= 0 || future_count > Stackable_Max_Count)
		return 0;

	SetStackCount(future_count);
	return howmany;
}


/**
 * Defines how many objects are contained in this stack.
 * If the stack count is set to a value less than 1
 * the object gets removed.
 * If the stack was infinite, then it will be finite
 * after setting the stack count.
 * @par amount this many objects are in the stack.
 * @return always returns true.             
 */
public func SetStackCount(int amount)
{
	count = BoundBy(amount, 0, Stackable_Max_Count); // allow 0, so that the object can be removed in UpdateStackDisplay
	count_is_infinite = false;
	UpdateStackDisplay();
	return true;
}


/**
 * Changes the stack count.
 * If the stack count is set to a value less than 1
 * the object gets removed.
 * Does not affect infinite stacks.
 * @par change the stack count will be changed by this
        amount.
 */
public func DoStackCount(int change)
{
	if (!(this->IsInfiniteStackCount()))
	{
		count += change;
		UpdateStackDisplay();
	}
}


/**
 * Defines the stack as infinite:
 * - one can always TakeObject() from the stack
 * - always accepts stacking other stacks
 * - if put into a container the first contained
 *   stack becomes infinite, this object gets removed
 */
public func SetInfiniteStackCount()
{
	count = Stackable_Infinite_Count;
	count_is_infinite = true;
	UpdateStackDisplay();
	return true;
}


/**
 * Takes one object from the stack, the
 * stack count is reduced by 1.
 * @return the object that was taken.
 *         This object is not contained.
 */
public func TakeObject()
{
	if (GetStackCount() == 1)
	{
		Exit();
		return this;
	}
	else
	{
		DoStackCount(-1);
		var take = CreateObject(GetID(), 0, 0, GetOwner());
		take->SetStackCount(1);
		return take;
	}
}


/**
 * Updates the stack, concerning information
 * that is required by other objects and the GUI.
 * The stack is removed if the stack count is <= 0. 
 */
public func UpdateStackDisplay()
{
	// empty stacks have to be removed
	if (GetStackCount() <= 0)
	{
		RemoveObject();
		return;
	}
	// otherwise update the object
	UpdatePicture();
	UpdateMass();
	UpdateName();
	NotifyContainer();
}


/**
 * Updates the picture. Called by UpdateStack().
 */
private func UpdatePicture()
{
	// Allow other objects to adjust their picture.
	return _inherited(...);
}


/**
 * Updates the name. Called by UpdateStack().
 * By default the name is changed to e.g. "5x Name"
 * if the stack count is 5, or "Infinite Name" if
 * the stack is infinite.
 */
private func UpdateName()
{
	if (this->IsInfiniteStackCount())
		SetName(Format("$Infinite$ %s", GetID()->GetName()));
	else
		SetName(Format("%dx %s", GetStackCount(), GetID()->GetName()));
}


/**
 * Updates the mass. Called by UpdateStack().
 * The mass is proportional to the mass of the definition.
 * It is assumed that the mass of the definition is that of
 * InitialStackCount() objects in one stack.
 */
private func UpdateMass()
{
	SetMass(GetID()->GetMass() * Max(GetStackCount(), 1) / InitialStackCount());
}


/**
 * Tells a possible container that the stack was
 * changed.
 * Calls NotifyHUD() in containers with extra slots,
 * or OnInventoryChange() otherwise. 
 */
private func NotifyContainer()
{
	// notify hud
	var container = Contained();
	if (container)
	{
		// has an extra slot
		if (container->~HasExtraSlot())
		{
			container->~NotifyHUD();
		}
		// is a clonk with new inventory system
		else
		{
			container->~OnInventoryChange();
		}
	}
}


/**
 * Tries merging packs BEFORE entering the container.
 * That means that a container can not prevent objects stacking into it.
 * However, the other way round (after the object has entered) presents more issues.
 */
protected func RejectEntrance(object into)
{
	var try_put = TryPutInto(into);
	//Log("***** TryPutInto did in fact return %v", try_put);
	if (try_put)
	{
		//Log("****** Rejected entrance into %s!!", into->GetName());
		return true;
	}
	//Log("***** Entered %v %s!!", this, into->GetName());
	return _inherited(into, ...);
}


/**
 * Value calculation. The value is proportional to the value of the definition.
 * It is assumed that the definition value defines the value for InitialStackCount()
 * objects in one stack.
 */
public func CalcValue(object in_base, int for_plr)
{
	return GetID()->GetValue() * Max(GetStackCount(), 1) / InitialStackCount();
}


/**
 * Tries to add this object to another stack.
 * This call removes this item if its stack
 * count is reduced to 0, or if it is an
 * infinite stack.
 * @par other the other stack.
 * @return true if successful. That is, if
 *         one or more objects were transferred
 *         to the other stack.
 */
public func TryAddToStack(object other)
{
	if (other == this) return false;
	
	// Is a stack possible in theory?
	if (other->~IsStackable() && other->GetID() == GetID())
	{
		var howmany = other->Stack(this);
		if (howmany > 0)
		{
			var stack = this;
			DoStackCount(-howmany);
			if (stack && stack->IsInfiniteStackCount()) stack->RemoveObject(); 
			// Stack succesful! No matter how many items were transfered.
			return true;
		}
	}
	return false;
}


/**
 * Attempts to add this stack to existing stacks in an object.
 * If only_add_to_existing_stacks is false, it will also 
 * try to stack recursively into containers with HasExtraSlot in that object.
 */
public func TryPutInto(object into, bool only_add_to_existing_stacks)
{
	only_add_to_existing_stacks = only_add_to_existing_stacks ?? false;
	var contents = FindObjects(Find_Container(into));

	if (!only_add_to_existing_stacks)
	{
		// first check if stackable can be put into object with extra slot
		for (var container in contents)
		{
			if (!container)
				continue;
			if (container->~HasExtraSlot())
				if (TryPutInto(container))
					return true;
		}
	}
	
	// then check this object
	for (var stack in contents)
	{
		if (!stack)
			continue;
		//added_to_stack = TryAddToStack(content) || added_to_stack;
		TryAddToStack(stack); // TODO: This is the original implementation. Maybe the function should be structured differently?
		if (!this) return true;
	}

	//Log("***** Stack can enter the object %s? TryPutInto will return %v", into->GetName(), added_to_stack);
	return false; // TODO was: added_to_stack
}


/**
 * Relevant for the GUI only. Infinite stacks can be stacked on top of other infinite
 * stacks only.
 * This does not affect the functions Stack() or TryAddToStack().
 */
public func CanBeStackedWith(object other)
{
	if (other->~IsInfiniteStackCount() != this->IsInfiniteStackCount()) return false;
	return _inherited(other, ...);
}


/**
 * Infinite stacks show a little symbol in their corner.
 */
public func GetInventoryIconOverlay()
{
	if (!(this->IsInfiniteStackCount())) return nil;
	return {Left = "50%", Bottom="50%", Symbol=Icon_Number, GraphicsName="Inf"};
}


/**
 * Saves stack counts in saved scenarios.
 */
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Name");
	if (this->IsInfiniteStackCount())
		props->AddCall("Stack", this, "SetInfiniteStackCount");
	else if (GetStackCount() != InitialStackCount())
		props->AddCall("Stack", this, "SetStackCount", GetStackCount());
	return true;
}


/**
 * Tells the interaction menu as how many objects this object should be displayed.
 */
func GetInteractionMenuAmount()
{
	var object_amount = this->GetStackCount() ?? 1;
	// Infinite stacks work differently - showing an arbitrary amount would not make sense.
	if (object_amount > 1 && this->IsInfiniteStackCount())
		object_amount = 1;
	return Min(object_amount, Stackable_Max_Display_Count);
}
