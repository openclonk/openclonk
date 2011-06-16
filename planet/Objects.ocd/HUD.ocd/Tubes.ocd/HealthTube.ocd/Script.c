/*
 * 	Health Bar
 *	Author: Mimmo
 *	
 *	Displays the Health in a curved way.
 *
 */ 


local current;
local crew;
protected func Construction()
{
	// parallaxity
	this["Parallaxity"] = [0,0];
	// visibility
	this["Visibility"] = VIS_Owner;
}

public func FxUpdateTimer() { Update(); }
public func FxUpdateNumberTimer() { UpdateNumber(); }
public func UpdateNumber()
{
	if(!GetCursor(GetOwner())) return 1;
	if(GetCursor(GetOwner()) != crew)
	{
		current = GetCursor(GetOwner())->GetEnergy();
	} 
	crew = GetCursor(GetOwner());
	Message("<c ff3300>%v",crew->GetEnergy());
}
public func Update()
{
	if(!GetCursor(GetOwner())) return 1;
	if(GetCursor(GetOwner()) != crew)
	{
		crew = GetCursor(GetOwner());
		if(!crew->GetMaxEnergy())
			return 1;
		current = GetCursor(GetOwner())->GetEnergy();
		var r = - 210 - ((( current * 1000) / crew->GetMaxEnergy()) * 69) / 100; 
		var fsin=Sin(r, 1000,10), fcos=Cos(r, 1000,10);
 		 // set matrix values
 		SetObjDrawTransform (
  	  	+fcos, +fsin, 0,
   	 	-fsin, +fcos, 0
   	   	);
   	   	
	}
	//if(!crew) return 1;
	CustomMessage(Format("<c dd0000>%v</c>",current),this,crew->GetOwner(),16,-72);
	
	if(Abs(current - crew->GetEnergy()))
	{
		if(Abs(current - crew->GetEnergy()) > 6 )
			if(current > crew->GetEnergy()) 
				current-=3;
			else
				current+=3;
		else
			if(current > crew->GetEnergy()) 
				current-=1;
			else
				current+=1;
	}
	else return 1;

	var r = - 210 - ((( current * 1000) / crew->GetMaxEnergy()) * 69) / 100; 
	var fsin=Sin(r, 1000,10), fcos=Cos(r, 1000,10);
 	 // set matrix values
 	SetObjDrawTransform (
    +fcos, +fsin, 0,
    -fsin, +fcos, 0
      );
     

    
	//SetR(r);
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
			Delay = 3,
			X = 0,
			Y = 0,
			Wdt = 450,
			Hgt = 225,
			NextAction = "Swirl",
		},

	}, def);
	SetProperty("Name", "$Name$", def);
}