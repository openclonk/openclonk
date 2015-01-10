/*-- Grass --*/

protected func Initialize()
{
	DoCon(Random(50));
	if (Random(2))
		SetGraphics("1");
}

public func Incineration()
{
	Destroy();
	return;
}

public func CanBeHitByShockwaves() { return true; }

public func OnShockwaveHit()
{	
	Destroy();
	return true;
}

private func Destroy()
{
	CreateParticle("Grass", 0, 0, PV_Random(-20, 20), PV_Random(-20, 10), PV_Random(30, 100), Particles_Straw(), 30);
	RemoveObject();
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Con");
	return true;
}

global func PlaceGrass(int amount, int start, int end, int height, int bottom)
{
	if (!start)
		start = 0;
	if (!end)
		end = LandscapeWidth();
	if(!height)
		height = 0;
	if(!bottom)
		bottom = LandscapeHeight();
		
	var x = start, y; 
	while (x < end)
	{
		y = height;
		while (y < bottom)
		{
			if (GetMaterial(AbsX(x), AbsY(y)) == Material("Sky"))
				if (GetMaterial(AbsX(x), AbsY(y + 3)) == Material("Earth"))
					if (Random(100) < amount)
						CreateObjectAbove(Grass, AbsX(x), AbsY(y + 1), NO_OWNER);
			y += 3;
		}
		x += 9;
	}
}

global func MakeGrasFunction()
{
	var x=[];
	var y=[];
	var r=[];
	for(var e in FindObjects(Find_ID(Grass)))
	{
		x[GetLength(x)]=e->GetX();
		y[GetLength(y)]=e->GetY();
		r[GetLength(r)]=e->GetR();
	}
	Log("private func PlaceGras()");
	Log("{");
	Log("	var x=%v;",x);
	Log("	var y=%v;",y);
	Log("	var r=%v;",r);

	Log("	for (var i = 0; i < GetLength(x); i++)");
	Log("	{");
	Log("		var grass=CreateObjectAbove(Grass, x[i], y[i] + 5, NO_OWNER);");
	Log("		grass->SetR(r[i]); ");
	Log("	}");
	Log("	return;");
	Log("}");
}

local Plane = -1;
local Name = "Grass";
local Placement = 0;
local BlastIncinerate = 1;
