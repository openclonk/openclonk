#appendto LeadShot
local lowering;
public func LessDamage() { lowering=6; SetClrModulation(RGB(255,128,0)); }
public func ProjectileDamage() { return 10-lowering; }
public func TrailColor(int time)
{
	if(!lowering)	return RGBa(255,255,255,240*Max(0,FlightTime()-time)/FlightTime());
	else	return RGBa(255-Min(time*5),255-Min(time*18,255),0,240*Max(0,FlightTime()-time)/FlightTime());
}