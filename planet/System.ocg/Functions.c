/*
	Functions.c
	Functions for copying functions.

	@author Marky
 */


/**
 Gets the plain name of a function, without the context information.

 @par call This is the function that provides its name.
 @return string Format("%v", call) will give you "source_type.function_name" for that function,
                but if you want to use that for a log output or for accessing properties you need
                the plain name "function_name". This function gives you exactly that.
*/
// documented in /docs/sdk/script/fn
global func GetFunctionName(func call)
{
	return RegexReplace(Format("%v", call), "(.+)\\.(.+)", "$2");
}


/**
 Adds a function from one thing to another.

 This can be called on a proplist, effect,
 definition, or objects.

 @note When using this function be sure that
       you know what you are doing. Replacing
       functions can have unintended side effects.

 @par call This is the function that should be added.
 @par override false means, that overriding
               an existing function is not allowed
               and will lead to a fatal error
*/
// documented in /docs/sdk/script/fn
global func AddFunction(func call, bool override)
{
	AssertNotNil(call);

	var name = GetFunctionName(call);

	if (this[name] && !override)
	{
		FatalError(Format("Cannot override function %s", name));
	}

	this[name] = call;
}


/**
 Adds several functions, calling AddFunction() on all items in the array.

 @par functions These are the functions that should be added.
 @par override false means, that overriding
               an existing function is not allowed
               and will lead to a fatal error
*/
// documented in /docs/sdk/script/fn
global func AddFunctions(array functions, bool override)
{
	AssertNotNil(functions);
	AssertNotEmpty(functions);

	for (var call in functions)
	{
		if (call)
		{
			AddFunction(call, override);
		}
	}
}
