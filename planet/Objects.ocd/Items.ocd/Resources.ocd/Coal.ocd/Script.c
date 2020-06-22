/*-- Coal --*/

#include Library_Flammable

func Construction()
{
	var graphic = Random(5);
	if (graphic)
		SetGraphics(Format("%d",graphic));
}

func Hit(x, y)
{
	StonyObjectHit(x, y);
}

func OnBurnDown()
{
	// Burst into ashes
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	
	var r = GetR();

	for (var cnt = 0; cnt < 5; ++cnt)
	{
		var distance = 3;
		var x = Sin(r, distance);
		var y = -Cos(r, distance);

		for (var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("Ashes", 1, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
	return true;
}

public func IsFuel() { return true; }
public func GetFuelAmount(int requested_amount)
{
    // disregard the parameter, because only a complete chunk should be removed 
	if (this != Coal) return GetCon();
	return 100;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Plane = 460;
// Coal burns for about 7 seconds
local BurnDownTime = 245;
