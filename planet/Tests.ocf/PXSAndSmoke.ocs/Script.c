func Initialize()
{
	Schedule(nil, "Test1()", 1);
}

global func Test1()
{
	CastPXS("Lava", 8000, 0, 70, 70);
	Schedule(nil, "Test1()", 10);
}
