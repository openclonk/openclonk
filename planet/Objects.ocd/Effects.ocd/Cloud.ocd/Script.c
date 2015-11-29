/*--- Cloud effect ---*/

func Initialize()
{
	SetAction("Fly");
	SetComDir(COMD_None);
}

func Show(int clr, int layer, int size,bool diffuse)
{
	// where on the z-axis?
	// FIXME
	this["Parallaxity"] = [layer,layer];
	if(layer < 100) SetCategory(GetCategory()|C4D_Background);
	else            SetCategory(GetCategory()|C4D_Foreground);
	
	if(!size) size = 360;
	
	var clrmod = clr;
	var count = 5;
	
	var particles =
	{
		Prototype = Particles_Cloud(),
		Alpha = (clr >> 24) & 0xff,
		R = (clr >> 16) & 0xff,
		G = (clr >> 8) & 0xff,
		B = clr & 0xff,
		Size = PV_Random(size - 20, size + 20)
	};
	
	// Create some clouds
	for(var i=0; i<count; ++i)
	{
		var x, y;
		
		var radius = RandomX(size/10);
		var angle =  Random(360);
		x = Sin(angle,+radius);
		y = Cos(angle,-radius/3);
		
		CreateParticle("Cloud", x, y, PV_Random(-diffuse, +diffuse), 0, 0, particles);
	}
}

// re-draw the particles of the cloud
func OnSynchronized()
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
	Decel = 16,
	NextAction = "Hold"
},
};
local Name = "Cloud";
local Plane = 300;
