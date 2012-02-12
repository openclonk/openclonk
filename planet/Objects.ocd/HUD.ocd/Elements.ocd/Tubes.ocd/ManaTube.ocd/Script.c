/**
	Mana Tube
	Displays the mana of the controlled clonk in the HUD.
	 
	@authors Mimmo_O
*/

local current;
local visual;
local btub,ftub;
local crew;

protected func Initialize()
{
	// Set parallaxity
	this.Parallaxity = [0, 0];
	// Set visibility
	this.Visibility = VIS_Owner;
	visual = 0;
	return;
}


public func FxUpdateTimer() { Update(); }

public func Update()
{
	
	if(!GetCursor(GetOwner())) return 1;

	if(GetCursor(GetOwner()) != crew)
	{ 
		crew = GetCursor(GetOwner());	
		current = GetCursor(GetOwner())->GetBreath();	
		if(current != crew->GetMaxBreath())
			for(var i=0; i<11; i++)
				InstantFadeIn();
		else
			for(var i=0; i<36; i++)
				InstantFadeOut();
		var r = -210 - ((( current * 1000) / crew->GetMaxBreath()) * 69) / 100; 
		var fsin=Sin(r, 1000,10), fcos=Cos(r, 1000,10);
 		 // set matrix values
 		SetObjDrawTransform (
    	+fcos, +fsin, 0,
    	-fsin, +fcos, 0
      	);
	}
	if(current == crew->GetMaxBreath() && crew->GetAction()!="Swim")
	{
			FadeOut();
	}
	else if(visual>-36) FadeIn();
	
	if(Abs(current - crew->GetBreath())) current+=BoundBy(crew->GetBreath()-current,-1,7);
	if(visual<0)
		CustomMessage(Format("<c 6464ff>%v</c>",current),ftub,crew->GetOwner(),16,-63);
	else
		CustomMessage("",this,crew->GetOwner());
	var r = -210 - ((( current * 1000) / crew->GetMaxBreath()) * 69) / 100; 
	var fsin=Sin(r, 1000,10), fcos=Cos(r, 1000,10);
 	 // set matrix values
 	SetObjDrawTransform (
    +fcos, +fsin, 0,
    -fsin, +fcos, 0
      );
}

public func SetTubes(b)
{
//btub=a;
ftub=b;
}

public func FadeOut()
{
	if(visual>11) return ;
	visual++;
	if(visual<1) return;
	SetR(GetR()+10);
	//btub->SetR(btub->GetR()+10);
	ftub->SetR(ftub->GetR()+10);

	return ;
}
public func FadeIn()
{
	if(visual>-36) visual--;
	if(visual<0) return ;
	SetR(GetR()-10);
	//btub->SetR(btub->GetR()-10);
	ftub->SetR(ftub->GetR()-10);
	
	return ;
}

public func InstantFadeOut()
{
	visual=11;
	SetR(110);
	//btub->SetR(110);
	ftub->SetR(110);
	return ;
}

public func InstantFadeIn()
{
	visual=-36;
	SetR(0);
	//btub->SetR(0);
	ftub->SetR(0);
	return ;
}

public func MakeHolders()
{
	CreateObject(GetID(),GetX(),GetY())->MakeTop();
}

public func MakeTop()
{
	SetGraphics("2");
}

public func MakeBot()
{
	SetGraphics("1");
}

local ActMap = {
	Swirl = {
		Prototype = Action,
		Name = "Swirl",
		Procedure = DFA_NONE,
		Length = 25,
		Delay = 3,
		X = 0,
		Y = 0,
		Wdt = 400,
		Hgt = 220,
		NextAction = "Swirl",
	},
};
