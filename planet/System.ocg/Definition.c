/*--
		Definition.c
		Authors: Maikel

		Functions generally applicable to definitions.
--*/

static GetDefinition_Loaded_Definition_List;

// Returns the definition or nil if par is a string and the definition exists.
// See the documentation for the case when par is an integer.
global func GetDefinition(par)
{
	// Overload behavior when par is a string.
	if (GetType(par) == C4V_String)
	{
		if (GetDefinition_Loaded_Definition_List == nil)
		{
			// Fill the static list of definitions when is has not been generated yet.
			GetDefinition_Loaded_Definition_List = {};
			var i = 0, def;
			while (def = GetDefinition(i++))
				GetDefinition_Loaded_Definition_List[Format("%i", def)] = def;
		}
		// Return the definition if in the list and nil otherwise.
		return GetDefinition_Loaded_Definition_List[par];
	}
	return _inherited(par, ...);
}