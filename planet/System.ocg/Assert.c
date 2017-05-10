/*
	This file contains functions that can be commonly used to make functions safe.

	Author: Marky
*/


/*
 Throws a fatal error if the index is outside the array bounds.
 Use this if you want to prevent expanding arrays.

 @par a The array in question.
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
