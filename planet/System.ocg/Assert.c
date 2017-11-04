/**
	Assert.c
	Functions that can be commonly used to make functions safe.

	@author Marky
*/


/*
 Throws a fatal error if the index is outside the array bounds.
 Use this if you want to prevent expanding arrays.

 @par arr The array in question.
 @par index The index that is checked.
 */
global func AssertArrayBounds(array arr, int index)
{
	if (index < 0 || index >= GetLength(arr))
	{
		FatalError(Format("Parameter outside array bounds (0 to %d): %d", Max(1, GetLength(arr)-1), index));
	}
}


/*
 Throws a fatal error function is not called from definition context.

 @par function_name [optional] A function name for the error message output.
 */
global func AssertDefinitionContext(string function_name)
{
	if (!this || GetType(this) != C4V_Def)
	{
		FatalError(Format("%s must be called from definition context! Was instead called from: %v", function_name ?? "The function", this));
	}
}


/*
 Throws a fatal error function is called from global context.

 @par function_name [optional] A function name for the error message output.
 */
global func AssertObjectContext(string function_name)
{
	if (!this)
	{
		FatalError(Format("%s must be called from object context!", function_name ?? "The function"));
	}
}


/*
 Throws a fatal error if the tested parameter has a length of 0.
 Use this if you want to prevent expanding arrays.

 @par value The value to check. Can be a string or array. 
 */
global func AssertNotEmpty(value)
{
	if (GetLength(value) == 0)
	{
		FatalError("Input must not be empty, but you passed an empty string or array.");
	}
}


/*
 Throws a fatal error a given value is 'nil'. This can be important in functions
 that do not provide a default value for parameters.

 @par value_name [optional] A function name for the error message output.
 */
global func AssertNotNil(value, string value_name)
{
	if (value == nil)
	{
		FatalError(Format("%s must not be 'nil'!", value_name ?? "The parameter"));
	}
}
