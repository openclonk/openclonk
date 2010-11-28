// King size javelin hurts more.
#appendto Javelin

local king_size;

public func MakeKingSize() { king_size = true;  SetMeshMaterial("KingJavelin", 0); }
public func MakeNormalSize() { king_size = false;  SetMeshMaterial("javelin", 0); }
public func Departure() { MakeNormalSize(); }

protected func JavelinStrength()
{
	if (king_size)
		return 21;
	return 14;
}

public func DoThrow(object clonk, int angle)
{
	var javelin=TakeObject();
	if (king_size)
		javelin->MakeKingSize();
	
	// how fast the javelin is thrown depends very much on
	// the speed of the clonk
	//var speed = 1200 * clonk->GetPhysical("Throw") / 12000 + 150 * Abs(clonk->GetXDir());
	//var xdir = Sin(angle,+speed);
	//var ydir = Cos(angle,-speed);
	// The clonk can convert some, indicated by div in percentages, of its own kinetic energy to change the momenta of the javelin, a miracle.
	var div = 60; // 40% is converted to the direction of the throwing angle.
	var xdir = clonk->GetXDir(1000);
	var ydir = clonk->GetYDir(1000);
	var speed = clonk->GetPhysical("Throw") / 8 + (100 - div) * Sqrt(xdir**2 + ydir**2) / 100;
	var jav_x = div * xdir / 100 + Sin(angle, speed);
	var jav_y = div * ydir / 100 - Cos(angle, speed);
		
	javelin->SetXDir(jav_x, 1000);
	javelin->SetYDir(jav_y, 1000);
	
	SetController(clonk->GetController());
	javelin->AddEffect("Flight",javelin,1,1,javelin,nil);
	javelin->AddEffect("HitCheck",javelin,1,1,nil,nil,clonk);
	
	Sound("ThrowJavelin*.ogg");
	
	fAiming = -1;
	clonk->UpdateAttach();
}