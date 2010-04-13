/*--- Fuse ---*/

local fHasMessage;

protected func Initialize ()
{
	SetProperty("LineColors", [RGB(100,50,0), RGB(1,1,1)]);
	// Put the first to vertices on the actual position
	SetVertex(0,0,GetX()); SetVertex(0,1,GetY());
	SetVertex(1,0,GetX()); SetVertex(1,1,GetY());
	fuse_dir = 0;
}

public func SetColorWarning(fOn)
{
	if(!fOn)
		SetProperty("LineColors", [RGB(100,50,0), RGB(1,1,1)]);
	else
		SetProperty("LineColors", [RGB(200,100,0), RGB(1,1,1)]);
}

public func Connect(pTarget1, pTarget2)
{
	SetAction("Connect", pTarget1, pTarget2);	
}

private func GetLineLength()
{
	var i = GetVertexNum()-1;
	var iDist = 0;
	while(i--)
	{
		// Calculate the length between the vertices
		iDist += Distance(GetVertex(i,0),GetVertex(i,1),GetVertex(i+1,0),GetVertex(i+1,1));
	}
	return iDist;
}

local fuse_vertex;
local fuse_x;
local fuse_y;
local fuse_call;
local fuse_dir;

public func StartFusing(object controler)
{
	if(fuse_dir != 0) return RemoveObject();
	if(GetActionTarget(0) == controler)
	{
		fuse_dir = +1;
		fuse_call = GetActionTarget(1);
		fuse_vertex = 0;
	}
	else
	{
		fuse_dir = -1;
		fuse_call = GetActionTarget(0);
		fuse_vertex = GetVertexNum()-1;
	}
	fuse_x = GetVertex(fuse_vertex, 0)*10;
	fuse_y = GetVertex(fuse_vertex, 1)*10;
	AddEffect("IntFusing", this, 1, 1, this);
	SetAction("Fusing");
}

func FxIntFusingTimer()
{
	var target_x = GetVertex(fuse_vertex+fuse_dir, 0)*10, target_y = GetVertex(fuse_vertex+fuse_dir, 1)*10;

	var speed = 20;
	if(Distance(fuse_x, fuse_y, target_x, target_y) < speed)
	{
		RemoveVertex(fuse_vertex);
		if(fuse_dir == -1) fuse_vertex--;
		speed -= Distance(fuse_x, fuse_y, target_x, target_y);
		if( (fuse_vertex == 0 && fuse_dir == -1) || fuse_vertex == GetVertexNum()-1)
		{
			fuse_call->~OnFuseFinished(this);
			RemoveObject(this);
			return -1;
		}
		fuse_x = GetVertex(fuse_vertex, 0)*10;
		fuse_y = GetVertex(fuse_vertex, 1)*10;
		target_x = GetVertex(fuse_vertex+fuse_dir, 0)*10;
		target_y = GetVertex(fuse_vertex+fuse_dir, 1)*10;
	}
	// Move spark position
	var iAngle = Angle(fuse_x, fuse_y, target_x, target_y);
	fuse_x += Sin(iAngle, speed);
	fuse_y +=-Cos(iAngle, speed);

	CastParticles("Spark",1,20,fuse_x/10-GetX(), fuse_y/10-GetY(),15,25,RGB(255,200,0),RGB(255,255,150));
	
	SetVertexXY(fuse_vertex, fuse_x/10, fuse_y/10);
}

public func IsFuse() { return true; }

func Definition(def) {
	SetProperty("ActMap", {
		Connect = {
		Prototype = Action,
		Name = "Connect",
		Length = 0,
		Delay = 0,
		Procedure = DFA_CONNECT,
		NextAction = "Connect",
		},
		Fusing = {
		Prototype = Action,
		Name = "Fusing",
		Length = 0,
		Delay = 0,
		Procedure = DFA_NONE,//CONNECT,
		NextAction = "Fusing",
	},  }, def);
	SetProperty("Name", "$Name$", def);
}