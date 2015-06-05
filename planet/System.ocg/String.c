/**
	String.c
	Functions for string manipulation.	
	
	@authors Maikel
*/

// Returns the reduced string with only the characters in the interval [begin, end) are taken. The value begin starts
// at zero and end can be at most the length of the string, which then includes the last character.
global func TakeString(string str, int begin, int end)
{
	// Default values and safety.
	begin = begin ?? 0;
	begin = Max(begin, 0);
	end = end ?? GetLength(str);
	end = Min(end, GetLength(str));
	// Construct the reduced string by looping over all chars.
	var reduced_str = "";
	for (var index = begin; index < end; index++)
		reduced_str = Format("%s%c", reduced_str, GetChar(str, index));
	return reduced_str;
}