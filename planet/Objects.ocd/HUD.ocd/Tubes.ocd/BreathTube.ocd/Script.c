/*
 * 	Breathe Bar
 *	Author: Mimmo
 *	
 *	Displays the Breathe in a curved way.
 *
 */ 

local current;
local visual;
local btub,ftub;
local crew;

protected func Construction()
{
	// parallaxity
	this["Parallaxity"] = [0,0];
	// visibility
	this["Visibility"] = VIS_Owner;
	visual=0;
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
		//return 1;
	}
	else if(visual>-36) FadeIn();
	if(Abs(current - crew->GetBreath())) current+=BoundBy(crew->GetBreath()-current,-1,7);
	//else return 1;
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
//	btub->SetR(0);
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

func Definition(def) 
{

SetProperty("ActMap", {
		Swirl = {
			Prototype = Action,
			Name = "Swirl",
			Procedure = DFA_NONE,
			Length = 25,
			Delay = 2,
			X = 0,
			Y = 0,
			Wdt = 500,
			Hgt = 250,
			NextAction = "Swirl",
		},
	}, def);
	SetProperty("Name", "$Name$", def);
}
