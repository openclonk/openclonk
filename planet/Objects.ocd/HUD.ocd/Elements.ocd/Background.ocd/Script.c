/**
	HUD Background
	
	Decoration for the action bar, inventory items and backpack.
	Used layers:
	0 - unused
	1 - horizontal connection
	5-15 - Extra Slots
	50 - Actual Graphic
	
	@authors Mimmo_O
*/

local current;	//extendation by 5
local desired;
local padding;
local parent;

protected func Construction()
{
	// parallaxity
	this.Parallaxity = [0, 0];
	// visibility
	this.Visibility = VIS_Owner;
	padding = 90;

	SetGraphics(0,GetID(),50,GFXOV_MODE_Base);
	SetGraphics("",GUI_Controller,0);
	SetGraphics("Line",GetID(),1,GFXOV_MODE_Base);

	for(var i=5; i<20; i++)
		SetGraphics("Slot",GetID(),i,GFXOV_MODE_Base);
	
	SetObjDrawTransform(4000,0,0,0,1000,118*1000,1);
	SetObjDrawTransform(1000,0,352*1000,0,1000,115*1000,5);
	for(var i=6; i<20; i++)
		SetObjDrawTransform(1000,0,100*1000,0,1000,115*1000,i);
}

func SetControllerObject(object p) { parent=p; }

func SlideTo(int number)
{
	if(number*5 == current) return false;
	current=number*5;
	//for(var i = 2; i<GetLength(parent.actionbar); i++)
	//	parent.actionbar[i]->HideSelector();
	SetObjDrawTransform(4000 + current*padding*2,0,0,0,1000,118*1000,1);

	for(var i=5; i<20; i++)
		SetObjDrawTransform(1000,0,(352 + ((padding/5)*current) - (i-5) * padding) * 1000,0,1000,115*1000,i);
	//if(!GetEffect("BackDecoSlider",this)) AddEffect("BackDecoSlider",this,100,1,this);
}


func FxBackDecoSliderTimer()
{
	if(current==desired) 	
	{
		for(var i = 2; i<GetLength(parent.actionbar); i++)
		if(parent.actionbar[i]->ShowsItem()) parent.actionbar[i]->ShowSelector();
		return -1;	
	}
	var movingSpeed 	= 	(Abs(current-desired)/5)+1;
	if(current>desired)		current-=movingSpeed;	
	else					current+=movingSpeed;
			
	
	SetObjDrawTransform(4000 + current*padding*2,0,0,0,1000,118*1000,1);

	for(var i=5; i<20; i++)
		SetObjDrawTransform(1000,0,(352 + ((padding/5)*current) - (i-5) * padding) * 1000,0,1000,115*1000,i);
}

func IsEngaged(){ if(current%5) return false; else return true; }
func GetSlotNumber(){ if(IsEngaged()) return desired*5; else return -1;}
