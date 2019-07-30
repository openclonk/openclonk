/**
	Array.c
	Global array helper functions.

	@author Zapper, Sven2		
*/


// Concatenates two arrays and returns a new array.
global func Concatenate(array first, array second)
{
	var len_first = GetLength(first);
	var result = CreateArray(len_first + GetLength(second));
	result[:len_first] = first;
	result[len_first:] = second;
	return result;
}

// Returns a new array that contains the values of the first array minus the values of the second array.
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
		if (!removed)
			diff[GetLength(diff)] = obj;
	}
	return diff;
}

// Removes nil values from an array, returns the amount of values removed.
global func RemoveHoles(array leerdammer)
{
	var move = 0;
	var len = GetLength(leerdammer);
	for (var i = 0; i < len; ++i)
	{
		if (leerdammer[i] == nil)
		{
			++move;
			continue;
		}
		leerdammer[i - move] = leerdammer[i];
	}
	SetLength(leerdammer, len - move);
	// assert IsGouda(leerdammer)
	return move;
}

// Removes duplicate entries - returns the number of entries removed.
global func RemoveDuplicates(array arr)
{
	var working = [];
	var cnt = 0;
	
	var len = GetLength(arr);
	for (var i = 0; i < len; ++i)
	{
		if (IsValueInArray(working, arr[i]))
		{
			++cnt;
			continue;
		}
		working[GetLength(working)] = arr[i];
	}
	SetLength(arr, GetLength(working));
	for (var i = GetLength(working); --i >= 0;)
		arr[i] = working[i];
	return cnt;
}

// Tests whether a value is in an array.
global func IsValueInArray(array arr, /*any*/ value)
{
	return GetIndexOf(arr, value) != -1;
}

// Removes a value from an array.
global func RemoveArrayValue(array arr, /*any*/ value, bool unstable)
{
	var i = GetIndexOf(arr, value);
	if (i == -1)
		return false;
	if (unstable == true)
		return RemoveArrayIndexUnstable(arr, i);
	return RemoveArrayIndex(arr, i);
}

// Randomly shuffles an array.
global func ShuffleArray(array arr)
{
	var len = GetLength(arr);
	var working = arr[:];
  
	while (--len >= 0)
	{
		var i = Random(len + 1);
		arr[len] = working[i];
		working[i] = working[len];
	}
	return;
}

// Takes array of format [[x1, y1], [x2, y2], ...] and returns array [[x1, x2,...],[y1, y2,...]]
global func TransposeArray(array v)
{
	var result = [], i = 0;
	for (var vc in v)
	{
		var j = 0;
		for (var c in vc)
		{
			if (!result[j]) 
				result[j] = CreateArray(GetLength(v));
			result[j][i] = c;
			++j;
		}
		++i;
	}
	return result;
}

// Deletes an index from an array, does not change the order of items in the array.
global func RemoveArrayIndex(array arr, int index, bool unstable)
{
	if (unstable == true)
		return RemoveArrayIndexUnstable(arr, index);
	// move all elements right of index to the left
	arr[index:] = arr[index + 1:];
	return true;
}

// Deletes an array item - might change the order of elements, but is faster.
global func RemoveArrayIndexUnstable(array arr, int index)
{
	arr[index] = arr[-1];
	SetLength(arr, GetLength(arr) - 1);
	return true;
}

// Inserts an element at the end of an array.
global func PushBack(array arr, /*any*/ value)
{
	arr[GetLength(arr)] = value;
	return true;
}

// Inserts an element at the beginning of an array.
global func PushFront(array arr, /*any*/ value)
{
	// Move elements one to the right.
	arr[1:] = arr;
	arr[0] = value;
	return true;
}

// Removes the last element from an array and returns it.
global func PopBack(array arr)
{
	if (GetLength(arr) == 0)
		return nil;
	var o = arr[-1];
	arr[:] = arr[:-1];
	return o;
}

// Removes the first element from an array and returns it.
global func PopFront(array arr)
{
	if (GetLength(arr) == 0)
		return nil;
	var o = arr[0];
	arr[:] = arr[1:];
	return o;
}

// Returns a random element from an array.
global func RandomElement(array arr)
{
	return arr[Random(GetLength(arr))];
}

// Move array of indexes before given element. Called from editor on item move.
global func MoveArrayItems(array arr, array source_indices, int insert_before)
{
	if (!arr) return false;
	var len = GetLength(arr), off = 0, val;
	// make sure source indices are sorted
	if (len > 1)
	{
		source_indices = source_indices[:]; 
		SortArray(source_indices);
	}
	for (var idx in source_indices)
	{
		if (idx < 0) idx += (1-(idx + 1)/len)*len; // resolve negative indices
		if (idx >= len) continue;
		if (idx < insert_before) 
		{
			// Move element forward
			idx += off; --off; // Adjust for other elements already moved
			val = arr[idx];
			while (++idx < insert_before) arr[idx-1] = arr[idx];
			arr[insert_before - 1] = val;
		}
		else
		{
			// Move element backward
			val = arr[idx];
			while (idx-- >= insert_before) arr[idx + 1] = arr[idx];
			arr[insert_before] = val;
			++insert_before;
		}
	}
	return true;
}

// Deletes multiple indexes from an array, does not change the order of items in the array.
global func RemoveArrayIndices(array arr, array indices)
{
	indices = indices[:];
	SortArray(indices, true);
	for (var idx in indices)
		if (idx < GetLength(arr))
			RemoveArrayIndex(arr, idx);
	return true;
}

// Performs a left fold.
//
// Examples:
//  - Reduce([1, 2, 3, 4], Min) == 1
//  - func Add(int a, int b) { return a + b; }
//    Reduce([1, 2, 3, 4], Add, 100) == 110
global func Reduce(array arr, func fn, initial)
{
	var i = 0;
	var result = initial ?? arr[i++];
	while (i < GetLength(arr))
		result = Call(fn, result, arr[i++]);
	return result;
}
