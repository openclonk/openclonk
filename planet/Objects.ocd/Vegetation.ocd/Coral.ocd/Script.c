/*-- Coral --*/


local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;


local ActMap = {
	Exist = {
		Prototype = Action,
		Name = "Exist",
		Procedure = DFA_FLOAT,
		NextAction = "Exist",
		Delay = 0,
		FacetBase = 1,
	}
};

func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InRect(rectangle);
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Wall(CNAT_Bottom | CNAT_Top | CNAT_Left | CNAT_Right), loc_area);
		if (!spot) continue;
		
		CreateObjectAbove(this, spot.x, spot.y + 5, NO_OWNER);
		--amount;
	}
	return true;
}

func StartFloating()
{
	RemoveTimer("Seed");
	SetAction("Idle");
	this.Collectible = 1;
	this.NutritionalValue = this.NutritionalValue_;
}

func Construction()
{
	SetCon(10+Random(80));
	ScheduleCall(this, "AdjustPosition", 1, 0);
	this.MaxCon = RandomX(100, 200);
}

func AdjustPosition()
{
	var vec = GetSurfaceVector();
	var r = Angle(0, 0, vec.x, vec.y);
	SetR(r);
	
	// project a bit out of the ground
	r += 180;
	var d = 5 * GetCon() / 100;
	var cnt = 10;
	while (--cnt)
	{
		var stuck = Stuck();
		var dir = 1;
		if (!stuck) dir = -1;
		var old_x = GetX(), old_y = GetY();
		SetPosition(GetX() + dir * Sin(r - 180, 1), GetY() - dir * Cos(r - 180, 1));
		
		if (stuck != Stuck())
		{
			if (stuck)
				SetPosition(old_x, old_y);
			break;
		}			
	}
	
	SetClrModulation(HSL(Random(255), 255, 200));
	 
	this.MeshTransformation = Trans_Rotate(Random(360), 0, 1, 0);
	SetAction("Exist");
	
	AddTimer("Seed", 60+Random(60));
}


func Seed()
{
	if (GetCon() < this.MaxCon)
	{
		if (!Random(2))
		{
			DoCon(1);
			var cnt = 10, r = GetR();
			while (!Stuck() && -cnt)
			{
				SetPosition(GetX() + Sin(r - 180, 1), GetY() - Cos(r - 180, 1));
			}
				
		}
	}
	else
	if (GetCon() > 50 && !Random(50) && !GetEffect("Seeding", this))
	{
		AddEffect("Seeding", this, 1, 1, this);
	}
	
	if (!Stuck())
	{
		StartFloating();
	}
}

func FxSeedingTimer()
{
	if (!Random(20)) return -1;
	if (!Random(2)) return;
	var seed = CreateObjectAbove(CoralSeed, 0, 0, GetOwner());
	seed->SetClrModulation(GetClrModulation());
}

/* Harvesting */

func IsCrop() { return false; }
func IsPlant(){return true;}

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

func NutritionalValue_() { return 15; }
