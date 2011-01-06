/*--
	Powder Keg
	Author: Ringwaul

	A barrel filled with black powder.
--*/

local oldcount;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryTransform(clonk)	{	return Trans_Mul(Trans_Translate(-1000,-800,0),Trans_Rotate(180,0,1,0));	}
public func GetCarryPhase() { return 900; }

protected func Initialize()
{
	UpdatePicture();
}

protected func Construction()
{
	oldcount = ContentsCount();
	CreateContents(Blackpowder,12);
	AddEffect("Update",this,1,1,this);
}

protected func MaxContentsCount() {	return 12;	}

public func FxUpdateTimer(object target, effect, int timer)
{
	if(ContentsCount() != oldcount)
		UpdatePicture();
	if(ContentsCount() == 0)
	{
		ChangeDef(Barrel);
		return -1;
	}
	oldcount = ContentsCount();
	return 1;
}

private func UpdatePicture()
{
	//modified script from Stackable.c4d
	var one = ContentsCount() % 10;
	var ten = (ContentsCount() / 10) % 10;
	
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;
	
	if (ten > 0)
	{
		SetGraphics(Format("%d", ten), Icon_Number, 11, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - spacing, 0, s, yoffs, 11);
	}
	else
		SetGraphics(nil, nil, 11);
		
	SetGraphics(Format("%d", one), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
}

public func Incineration()
{
	AddEffect("Fuse",this,1,1,this);
}

public func FxFuseTimer(object target, effect, int timer)
{
	CastParticles("Spark",1,10,0,0,20,30,RGB(255,255,0),RGB(255,255,0));
	if(timer > 90)
	{
		//17-32 explosion radius
		var radius = Sqrt(64 * (4 + ContentsCount()));
		Explode(radius);
	}
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func Damage(int change, int byplayer)
{
	Incinerate();
}

public func OnProjectileHit()
{
	Incinerate();
}

func IsAlchemyProduct() { return 1; }
func AlchemyProcessTime() { return 100; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
