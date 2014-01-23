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
static const Stackable_Max_Count = 999;

// What GetStackCount should return if the count is set to infinite
// Set this to a fairly large number and not e.g. -1, so naive
// implementations that update their graphics by GetStackCount() show a
// bunch of items. However, the number shouldn't be too large so the
// object doesn't get overly heavy.
// Note that count_is_infinite is a separate variable, so we can support
// stacks >999 but <Inf in the future.
static const Stackable_Infinite_Count = 50;

public func IsStackable() { return true; }
public func GetStackCount() { return Max(1, count); }
public func MaxStackCount() { return 20; }

protected func Construction()
{
	count = MaxStackCount();
	return _inherited(...);
}

public func Stack(object obj)
{
	if (obj->GetID() != GetID())
		return 0;
	
	// Infinite stacks can always take everything
	if (IsInfiniteStackCount()) return obj->GetStackCount();
	if (obj->~IsInfiniteStackCount())
	{
		SetInfiniteStackCount();
		return obj->GetStackCount();
	}
	
	var howmany = Min(obj->GetStackCount(), MaxStackCount() - GetStackCount());
	
	if (count + howmany > Stackable_Max_Count)
		return 0;
	
	SetStackCount(count + howmany);

	return howmany;
}

public func SetStackCount(int amount)
{
	count = BoundBy(amount, 0, Stackable_Max_Count);
	count_is_infinite = false;
	UpdateStackDisplay();
	return true;
}

public func IsInfiniteStackCount() { return count_is_infinite; }

public func SetInfiniteStackCount()
{
	count = Stackable_Infinite_Count;
	count_is_infinite = true;
	UpdateStackDisplay();
	return true;
}

public func TakeObject()
{
	if (count == 1)
	{
		Exit();
		return this;
	}
	else if (count > 1)
	{
		if (!count_is_infinite) SetStackCount(count - 1);
		var take = CreateObject(GetID(), 0, 0, GetOwner());
		take->SetStackCount(1);
		if (!count_is_infinite) UpdateStackDisplay();
		return take;
	}
}

public func UpdateStackDisplay()
{
	UpdatePicture();
	UpdateMass();
	UpdateName();
	// notify hud
	var container = Contained();
	if (container)
	{
		// has an extra slot
		if (container->~HasExtraSlot())
			container->~NotifyHUD();
		// is a clonk with new inventory system
		else if (container->~GetSelected())
		{
			var pos = container->GetItemPos(this);
			container->~OnSlotFull(pos);
		}
	}
}

private func UpdatePicture()
{
	// Put a small number showing the stack count in the bottom right 
	// corner of the picture
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;

	if  (count_is_infinite)
	{
		SetGraphics(nil, nil, 10);
		SetGraphics(nil, nil, 11);
		SetGraphics("Inf", Icon_Number, 12, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
	}
	else
	{
		var one = GetStackCount() % 10;
		var ten = (GetStackCount() / 10) % 10;
		var hun = (GetStackCount() / 100) % 10;
		
		if (hun > 0)
		{
			SetGraphics(Format("%d", hun), Icon_Number, 10, GFXOV_MODE_Picture);
			SetObjDrawTransform(s, 0, xoffs - 2 * spacing, 0, s, yoffs, 10);
		}
		else
			SetGraphics(nil, nil, 10);

		if (ten > 0 || hun > 0)
		{
			SetGraphics(Format("%d", ten), Icon_Number, 11, GFXOV_MODE_Picture);
			SetObjDrawTransform(s, 0, xoffs - spacing, 0, s, yoffs, 11);
		}
		else
			SetGraphics(nil, nil, 11);
			
		SetGraphics(Format("%d", one), Icon_Number, 12, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
	}
	return _inherited(...);
}

private func UpdateName()
{
	if (IsInfiniteStackCount())
		SetName(Format("$Infinite$ %s", GetID()->GetName()));
	else
		SetName(Format("%dx %s", GetStackCount(), GetID()->GetName()));
}

private func UpdateMass()
{
	SetMass(GetID()->GetMass() * Max(GetStackCount(), 1) / MaxStackCount());
}

protected func RejectEntrance(object into)
{
	if (TryPutInto(into))
		return true;
	return _inherited(into, ...);
}

/* Value */

public func CalcValue(object in_base, int for_plr)
{
	return GetID()->GetValue() * Max(GetStackCount(), 1) / MaxStackCount();
}

private func TryPutInto(object into)
{
	var contents = FindObjects(Find_Container(into));

	// first check if stackable can be put into object with extra slot
	for (var content in contents)
	{
		if (!content)
			continue;
		if (content->~HasExtraSlot())
			if (TryPutInto(content))
				return true;
	}

	// then check this object
	for (var content in contents)
	{
		var howmany = 0;
		if (!content)
			continue;
		if (content->~IsStackable())
			if (content->GetID() == GetID())
				if (howmany = content->Stack(this))
				{
					count -= howmany;
					if(count <= 0)
					{
						RemoveObject();
						return true;
					}
				}
	}
	
	UpdateStackDisplay();
	return false;
}

// Save stack counts in saved scenarios
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Name");
	if (IsInfiniteStackCount())
		props->AddCall("Stack", this, "SetInfiniteStackCount");
	else if (GetStackCount() != MaxStackCount())
		props->AddCall("Stack", this, "SetStackCount", GetStackCount());
	return true;
}
