/*-- 
	Powder Keg
	Author: Ringwaul

	A barrel filled with black powder.
--*/

local oldamount;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryTransform(clonk)	{	return Trans_Mul(Trans_Translate(-3500,2000,0),Trans_Rotate(180,0,1,0));	}
public func GetCarryPhase() { return 900; }

protected func Initialize()
{
	Message("Lol");
	UpdatePicture();
}

protected func Construction()
{
	CreateContents(Blackpowder,12);
	UpdatePicture();
}

protected func MaxContentsCount() {	return 12;	}

public func ControlUse(object clonk, int iX, int iY)
{
	if(Contents(0) && clonk->ContentsCount() < clonk->MaxContentsCount())
	{
		Contents(0)->Enter(clonk);
		Sound("Grab"); //todo sound
		UpdatePicture();
		if(ContentsCount() == 0)
		{
			if(clonk->ContentsCount() < clonk->MaxContentsCount())
			{
				clonk->CreateContents(Barrel);
			}
			else
				CreateObject(Barrel);
			//Switch to proper postion
			var pBarrel = FindObject(Find_ID(Barrel),Find_Container(clonk));
			var pPowderKeg = FindObject(Find_ID(PowderKeg),Find_Container(clonk));
			clonk->Switch2Items(clonk->GetItemPos(pBarrel),clonk->GetItemPos(pPowderKeg));
			Exit();
			RemoveObject();
		}
	}
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

public func FxFuseTimer(object target, int num, int timer)
{
	CastParticles("Spark",1,10,0,0,20,30,RGB(255,255,0),RGB(255,255,0));
	if(timer > 90)
	{
		//20-42 explosion radius
		Explode(Sqrt(1 + ContentsCount() * 2) * 10);
	}
}

protected func RejectCollect(id objid, object obj)
{
	if(objid != Blackpowder || ContentsCount() >= MaxContentsCount()) return true;
}

public func Collection(object obj, bool put)
{//When a clonk throws a blackpowder pouch into the keg
	UpdatePicture();
}

func IsAlchemyProduct() { return 1; }
func AlchemyProcessTime() { return 100; }

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
}
