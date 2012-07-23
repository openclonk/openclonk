/*--- Coconut ---*/

local seedTime;

protected func Construction()
{
	//AddEffect to grow trees
	AddEffect("Seed", this, 30, 18, this);
	seedTime = 15 * 2;
}

public func FxSeedTimer(object coconut, int num, int timer)
{
	//Start germination timer if in the environment
	if(!Contained()){
		seedTime--;
	}
	//If the coconut is on the earth
	if(seedTime <= 0 && !Contained() && GetMaterial(0,3) == Material("Earth") && GetCon() >= 100)
	{
		//Are there any trees too close? Is the coconut underwater?
		if(!FindObject(Find_Func("IsTree"), Find_Distance(80)) && !GBackLiquid())
		{
			Germinate();
			return -1;
		}
	}
	
	//Destruct if sitting for too long 
	if(seedTime == -120) this.Collectible = 0;
	if(timer < -120) DoCon(-5);
	
	return 0;
}

public func Germinate() { AddEffect("Germinate", this, 1,1, this); }
public func FxGerminateTimer(object coconut, int num, int timer)
{
	if(timer == 1)
	{
		this.Collectible = 0;
		//Tree sprouts
		PlaceVegetation(Tree_Coconut, 0, 0, 1,1, 100);
	}
	//Fade out
	SetObjAlpha(255 - (timer * 255 / 100));
	if(timer == 100) coconut->RemoveObject();
}
		
/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

public func NutritionalValue() { return 5; }

/* Bounce */

protected func Hit(x, y)
{
	//Bounce; useful for spreading seeds further from parent tree
	if(y > 1) SetSpeed(x, y / -2, 100);
	StonyObjectHit(x,y);
	return true;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
