/* Global Array helper functions */

global func Concatenate(array first, array second)
{
	var result = CreateArray(GetLength(first)+GetLength(second));
	var i = 0;
	for (var something in first)
		result[i++] = something;
	for (var something in second)
		result[i++] = something;
	return result;
}

global func Subtract(array subject, array subtract)
{
	var diff = [];
	for (var obj in subject)
	{
		var removed = false;
		for (var rem_obj in subtract)
		{
			if (rem_obj == obj)
			{
				removed = true;
				break;
			}
		}
		if(!removed)
			diff[GetLength(diff)] = obj;
	}
	return diff;
}

global func RemoveHoles(array leerdamer)
{
	// gouda is a cheese with no holes ;-)
	var gouda = [];
	var hole = nil;
	for (var piece in leerdamer)
		if (piece != hole)
			gouda[GetLength(gouda)] = piece;
	return gouda;
}