/**
	Proplists.c
	General helper functions that create or work with proplists.
		
	@author Guenther, Maikel, Marky
*/

/*
 Adds the contents of one proplist to the other.
 Usually you can do this by using a prototype, but 
 this is not possible when you receive a second proplist
 from another function.
 
 @par original This proplist will be expanded.
 @par additional These are the additional contents.
 @par no_overwrite By default the function overwrites
      existing properties in the original proplist,
      as it would happen if you derive from a prototype.
      If this parameter is 'true' the function will report
      an error instead of overwriting a parameter.
 
 @return proplist The original proplist. This is in fact the same
         proplist that was passed as an argument, the functions just
         returns it for convenience.
 */
global func AddProperties(proplist original, proplist additional, bool no_overwrite)
{
	if (original == nil)
	{
		FatalError("Cannot add properties to 'nil'");
	}
	if (additional == nil)
	{
		FatalError("Adding proplist 'nil' to another proplist makes no sense.");
	}
	
	for (var property in GetProperties(additional))
	{
		if (no_overwrite && (original[property] != nil)) // about to overwrite?
		{
			FatalError(Format("Cancelling overwrite of property '%s', original value %v, with new value %v", property, original[property], additional[property]));
		}
		else
		{
			original[property] = additional[property];
		}
	}
	
	return original;
}


// Turns a property into a writable one. This for example useful if an object
// inherits a property from a definition which should not stay read-only.
global func MakePropertyWritable(name, obj)
{
	obj = obj ?? this;
	if (GetType(obj[name]) == C4V_Array)
	{
		obj[name] = obj[name][:];
		var len = GetLength(obj[name]);
		for (var i = 0; i < len; ++i)
			MakePropertyWritable(i, obj[name]);
	}
	if (GetType(obj[name]) == C4V_PropList)
	{
		obj[name] = new obj[name] {};
		var properties = GetProperties(obj[name]);
		var len = GetLength(properties);
		for (var i = 0; i < len; ++i)
			MakePropertyWritable(properties[i], obj[name]);
	}
  	return;
}
