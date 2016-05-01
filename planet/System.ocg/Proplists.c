/**
	Proplists.c
	General helper functions that create or work with proplists.
		
	@author Guenther, Maikel
*/

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
