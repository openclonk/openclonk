/**
	Components.c
	Handles the components of an object, which are a property of the object / definition.
	Components are a property of the form
		local Components = {Def1 = cnt1, Def2 = cnt2, ...};
	which can be modified directly or via the script functions in this file:
		SetComponent(id component, int count)
		GetComponent(id component, int index)
		Split2Components()

	@author Maikel
*/


// Sets the component of an object or definition.
// documented in /docs/sdk/script/fn
global func SetComponent(id component, int count)
{
	// Safety: can only be called from object.
	if (!this || GetType(this) != C4V_C4Object)
	{
		return FatalError(Format("SetComponent must be called from object context and not from %v", this));
	}

	// Safety: component must be specified.
	if (!component || GetType(component) != C4V_Def)
	{
		return FatalError(Format("First parameter (id component) of SetComponent must be a definition but was %v.", component));
	}

	// Ensure count is non-negative.
	count = Max(count, 0);

	// Initialize Components if it does not exist yet.
	if (!this.Components || GetType(this.Components) != C4V_PropList)
	{
		this.Components = {};
	}
	// Ensure object property is different from definition property.
	if (this.Components == GetID().Components)
	{
		MakePropertyWritable("Components");
	}

	// Write the new count to the components properties.
	this.Components[Format("%i", component)] = count;
	return;
}

// Returns the amount if the component parameter is specified. If the component parameter is nil
// it returns the definition of the component for the given index.
// documented in /docs/sdk/script/fn
global func GetComponent(id component, int index)
{
	// Safety: can only be called from object or definition context.
	if (!this || (GetType(this) != C4V_C4Object && GetType(this) != C4V_Def))
	{
		return FatalError(Format("GetComponent must be called from object or definition context and not from %v", this));
	}

	// Safety: return nil if Components could not be found or are not a proplist.
	if (!this.Components || GetType(this.Components) != C4V_PropList)
	{
		return;
	}

	// If component is specified return the count for that definition.
	if (GetType(component) == C4V_Def)
	{
		return this.Components[Format("%i", component)];
	}

	// Ensure the index is valid.
	index = Max(index, 0);

	// If component is not specified return the definition of the component at the index.
	var iteration_index = 0;
	for (var entry in GetProperties(this.Components))
	{
		// Check if the entry is an actual valid definition.
		var entry_def = GetDefinition(entry);
		if (!entry_def)
		{
			continue;
		}
		if (index == iteration_index)
		{
			return entry_def;
		}
		iteration_index += 1;
	}
	return;
}

// Callback by the engine when the completion of an object changes.
global func OnCompletionChange(int old_con, int new_con)
{
	if (!this || GetType(this) != C4V_C4Object)
	{
		return _inherited(old_con, new_con, ...);
	}

	// Determine whether the object allows Oversize.
	var oversize = GetDefCoreVal("Oversize", "DefCore");

	// Loop over all components and set their new count.
	var index = 0, component;
	while (component = GetID()->GetComponent(nil, index))
	{
		var def_amount = GetID()->GetComponent(component);
		var new_amount = Max(def_amount * new_con / 100000, 0);
		// If the object has no Oversize then constrain the maximum number of components.
		if (!oversize)
		{
			new_amount = Min(new_amount, def_amount);
		}
		SetComponent(component, new_amount);
		index++;
	}
	return _inherited(old_con, new_con, ...);
}

// Splits the calling object into its components.
// documented in /docs/sdk/script/fn
global func Split2Components()
{
	// Safety: can only be called from object context.
	if (!this || GetType(this) != C4V_C4Object)
	{
		return FatalError(Format("Split2Components must be called from object context and not from %v", this));
	}

	// Transfer all contents to container.
	var container = Contained();
	while (Contents())
	{
		if (!container || !Contents()->Enter(container))
			Contents()->Exit();
	}

	// Split components.
	for (var i = 0, component_id; component_id = GetComponent(nil, i); ++i)
		for (var j = 0; j < GetComponent(component_id); ++j)
		{
			var component = CreateObjectAbove(component_id, nil, nil, GetOwner());
			if (OnFire()) component->Incinerate();
			if (!container || !component->Enter(container))
			{
				component->SetR(Random(360));
				component->SetXDir(Random(3) - 1);
				component->SetYDir(Random(3) - 1);
				component->SetRDir(Random(3) - 1);
			}
		}
	return RemoveObject();
}
