/**
	Components.c
	Handles the components of an object, which are a property of the object / definition.
	Components are a property of the form
		local Components = [[def1, cnt1], [def2, cnt2], ...];
	which can be modified directly or via the script functions in this file:
		SetComponent2(id component, int count)
		GetComponent(id component, int index)
		Split2Components()
		
	@author Maikel
*/


// Sets the component of an object or definition.
global func SetComponent(id component, int count)
{
	// Safety: can only be called from object or definition context.
	if (!this || (GetType(this) != C4V_C4Object && GetType(this) != C4V_Def))
		return FatalError(Format("SetComponent must be called from object or definition context and not from %v", this));
		
	// Safety: component must be specified.
	if (!component || GetType(component) != C4V_Def)
		return FatalError(Format("First parameter (id component) of SetComponent must be a definition but was %v.", component));
		
	// Ensure count is non-negative.
	count = Max(count, 0);
	
	// Initialize Components if it does not exist yet.
	if (!this.Components || GetType(this.Components) != C4V_Array)
		this.Components = [];
	
	// Loop over all existing components and change count if the specified one has been found.
	for (var entry in this.Components)
	{
		if (GetType(entry) != C4V_Array || GetLength(entry) < 2)
			continue;
		if (entry[0] == component)
		{
			entry[1] = count;
			return;
		}
	}
	
	// If it did not succeed changing an existing component, then append the new component.
	PushBack(this.Components, [component, count]);
	return;
}

// Returns the amount if the component parameter is specified. If the component parameter is nil
// it returns the definition of the component for the given index.
global func GetComponent(id component, int index)
{
	// Safety: can only be called from object or definition context.
	if (!this || (GetType(this) != C4V_C4Object && GetType(this) != C4V_Def))
		return FatalError(Format("GetComponent must be called from object or definition context and not from %v", this));
		
	// If component is specified return the count for that definition.
	if (GetType(component) == C4V_Def)
	{
		if (!this.Components || GetType(this.Components) != C4V_Array)
			return 0;
		for (var entry in this.Components)
		{
			if (GetType(entry) != C4V_Array || GetLength(entry) < 2)
				continue;
			if (entry[0] == component)
				return entry[1];			
		}	
		return 0;
	}

	// If component is not specified return the definition of the component at the index.
	if (!this.Components || GetType(this.Components) != C4V_Array)
		return;
	// Ensure index is valid.	
	index = Max(index, 0);
	var cnt = 0;
	for (var entry in this.Components)
	{
		if (GetType(entry) != C4V_Array || GetLength(entry) < 2)
			continue;
		if (index == cnt)
			return entry[0];
		cnt++;
	}
	return;
}

// Splits the calling object into its components.
global func Split2Components()
{
	// Safety: can only be called from object context.
	if (!this || GetType(this) != C4V_C4Object)
		return FatalError(Format("Split2Components must be called from object context and not from %v", this));

	// Transfer all contents to container.
	var ctr = Contained();
	while (Contents())
		if (!ctr || !Contents()->Enter(ctr))
			Contents()->Exit();
			
	// Split components.
	for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
		for (var j = 0; j < GetComponent(compid); ++j)
		{
			var comp = CreateObjectAbove(compid, nil, nil, GetOwner());
			if (OnFire()) comp->Incinerate();
			if (!ctr || !comp->Enter(ctr))
			{
				comp->SetR(Random(360));
				comp->SetXDir(Random(3) - 1);
				comp->SetYDir(Random(3) - 1);
				comp->SetRDir(Random(3) - 1);
			}
		}
	return RemoveObject();
}
