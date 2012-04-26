func Initialize()
{
	var  a = CreateObject(Rock, 100, 100, NO_OWNER);
	var  b = CreateObject(Rock, 200, 100, NO_OWNER);
	a->CreateRope2(a, b, 100);
}
