/*-- Balloon --*/

local rider;

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}

protected func Initialize()
{
	Message("Debug",this);
	SetProperty("Collectible",1,this);
	SetAction("Item");
	SquishVertices(true);
}

func ControlUseStart(object clonk, int ix, int iy)
{
	if(clonk->GetAction() != "Jump") return 1;

	rider = clonk;
	SetProperty("Collectible",0,this);
	Exit();
	SetAction("Inflate");
	SetSpeed(clonk->GetXDir(),clonk->GetYDir());
	clonk->SetAction("HangOnto",this);
	AddEffect("Float",this,1,1,this);
	SquishVertices(false);
}

func ControlJump()
{
	if(GetAction() != "Deflate") SetAction("Deflate");
}

func ControlLeft()
{
	if(GetXDir() > -7) SetXDir(GetXDir() - 1);
}

func ControlRight()
{
	if(GetXDir() < 7) SetXDir(GetXDir() + 1);
}

protected func Pack()
{
	SetProperty("Collectible",1,this);
	rider->SetAction("Walk");
	rider->SetXDir(this->GetXDir());
	RemoveEffect("Float",this);
	SquishVertices(true);
	Enter(rider);
}

protected func SquishVertices(bool squish)
{
	//Reshapes the vertices from a vehicle to an item and back
	if(squish == true)
	{
		SetVertex(0,VTX_X,-2,2);
		SetVertex(0,VTX_Y,2,2);
		SetVertex(1,VTX_X,2,2);
		SetVertex(1,VTX_Y,2,2);
		SetVertex(2,VTX_X,0,2);
		SetVertex(2,VTX_Y,0,2);
		SetVertex(3,VTX_X,0,2);
		SetVertex(3,VTX_Y,0,2);
		SetVertex(4,VTX_X,0,2);
		SetVertex(4,VTX_Y,14,2);
	return 1;
	}

	if(squish != true)
	{
		SetVertex(0,VTX_X,0,2);
		SetVertex(0,VTX_Y,40,2);
		SetVertex(1,VTX_X,0,2);
		SetVertex(1,VTX_Y,-16,2);
		SetVertex(2,VTX_X,-15,2);
		SetVertex(2,VTX_Y,0,2);
		SetVertex(3,VTX_X,15,2);
		SetVertex(3,VTX_Y,0,2);
		SetVertex(4,VTX_X,0,2);
		SetVertex(4,VTX_Y,48,2);
	return 0;
	}
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func OnProjectileHit()
{
	CreateObject(BurntBalloon,0,30);
	CastParticles("Air",20,5,0,-10,170,190,RGB(255,255,255),RGB(255,255,255));
	if(rider!=nil) rider->SetAction("Tumble");
	RemoveObject();
}

func FxFloatTimer(object target, int num, int time)
{
	var speed = 7;
	if(GetYDir() > speed) SetYDir(GetYDir() - 1);
	if(GetYDir() < speed) SetYDir(GetYDir() + 1);
	if(GetXDir() > speed * 3) SetXDir(GetXDir()-1);
	if(GetXDir() < -speed * 3) SetXDir(GetXDir()+1);

	if(GBackSolid(0,50) || GBackSolid(5,43) || GBackSolid(-5,43) || GBackLiquid(0,50))
	{
		if(GetAction() != "Deflate") SetAction("Deflate");
		return -1;
	}
}

func Definition(def) {
	SetProperty("ActMap", {

Float = {
	Prototype = Action,
	Name = "Float",
	Procedure = DFA_FLOAT,
	Directions = 1,
	FlipDir = 0,
	Length = 15,
	Delay = 5,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Float",
	Animation = "Float",
},

Inflate = {
	Prototype = Action,
	Name = "Inflate",
	Procedure = DFA_FLOAT,
	Directions = 1,
	FlipDir = 0,
	Length = 20,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Float",
	Animation = "Inflate",
},

Deflate = {
	Prototype = Action,
	Name = "Deflate",
	Procedure = DFA_FLOAT,
	Directions = 1,
	FlipDir = 0,
	Length = 20,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Item",
	EndCall = "Pack",
	Animation = "Deflate",
},

Item = {
	Prototype = Action,
	Name = "Item",
	Procedure = DFA_FLIGHT,
	Directions = 1,
	FlipDir = 0,
	Length = 1,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Item",
	Animation = "Item",
},
}, def);}