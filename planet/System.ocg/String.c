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

// Converts a char into a string.
global func CharToString(int char)
{
	return Format("%c", char);
}

// Returns whether a char is a digit [0-9].
global func CharIsDigit(int char)
{
	return Inside(char, 48, 57);
}

// Returns whether a char is a letter [A-Za-z].
global func CharIsLetter(int char)
{
	return CharIsLowerCase(char) || CharIsUpperCase(char);
}

// Returns whether a char is a lower-case letter [a-z].
global func CharIsLowerCase(int char)
{
	return Inside(char, 97, 122);
}

// Returns whether a char is an upper-case letter [A-Z].
global func CharIsUpperCase(int char)
{
	return Inside(char, 65, 90);
}

// Converts a string to an integer if it consists of digits only.
global func StringToInteger(string str)
{
	var integer = 0;
	var power = 0;
	for (var index = GetLength(str) - 1; index >= 0; index--)
	{
		var char = GetChar(str, index);
		if (!CharIsDigit(char))
			return nil;
		integer += (char - 48) * 10**power;
		power++;	
	}
	return integer;
}
