/*--- Cloud effect ---*/

func Initialize()
{
	SetAction("Fly");
}

func Show(int clr, int layer, int size,bool diffuse)
{
	// where on the z-axis?
	// FIXME
	this["Parallaxity"] = [layer,layer];
	if(layer < 100) SetCategory(GetCategory()|C4D_Background);
	else            SetCategory(GetCategory()|C4D_Foreground);
	
	if(!size) size = 1800;
	
	var clrmod = clr;
	var count = 5;
	
	// Create some clouds
	for(var i=0; i<count; ++i)
	{
		var x,y,size;
		
		var radius = RandomX(size/10);
		var angle =  Random(360);
		x = Sin(angle,+radius);
		y = Cos(angle,-radius/3);
		
		CreateParticle("Cloud",x,y,diffuse*RandomX(-1,1),0,size+RandomX(-200,200),clrmod,this);
	}
}

// re-draw the particles of the cloud
func UpdateTransferZone()
{
	Show();
}

local ActMap = {
Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Speed = 1000,
	Accel = 16,
	NextAction = "Hold"
},
};
local Name = "Cloud";

