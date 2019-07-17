#appendto Tree_Coconut

public func Construction()
{
	if (g_theme == MiamiIce)
		SetClrModulation(RGB());
	return _inherited();
}

public func BurstIntoAshes()
{
	if (g_theme != MiamiIce) return inherited(...);

	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	
	var r = GetR();
	var size = GetCon() * 110 / 100;
	
	for (var cnt = 0; cnt < 10; ++cnt)
	{
		var distance = Random(size/2);
		var x = Sin(r, distance);
		var y = -Cos(r, distance);
		
		for (var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("BlackIce", 5, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
}
