func Initialize()
{
	var a = [3];
	RemoveArrayIndex(a, 0);
	SortArrayByProperty(a, "priority");
	Log("%v-%d", a, GetLength(a));
	for (var i = -1; i >= 0; i--)
		Log("%d", i);
}