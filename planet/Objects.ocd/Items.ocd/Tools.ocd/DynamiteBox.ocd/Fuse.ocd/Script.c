/** 
	Dynamite Fuse 
	Line object that connects the dynamite sticks with the igniter.
	
	@author Newton, Maikel
*/

// Definition call: Create a fuse between two objects
public func Create(object o1, object o2)
{
	if (!o1 || !o2) return;
	var fuse = CreateObject(Fuse, AbsX(o1->GetX()), AbsY(o1->GetY()));
	if (fuse)
	{
		fuse->Connect(o1, o2);
	}
	return fuse;
}

protected func Initialize()
{
	SetProperty("LineColors", [RGB(100, 50, 0), RGB(1, 1, 1)]);
	// Put the first to vertices on the actual position.
	SetVertex(0, 0, GetX()); SetVertex(0, 1, GetY());
	SetVertex(1, 0, GetX()); SetVertex(1, 1, GetY());
	return;
}

public func IsFuse() { return true; }

public func Connect(object target1, object target2)
{
	SetAction("Connect", target1, target2);
}

public func GetConnectedItem(object source)
{
	// Return connected target on the other side of given source
	if (source == GetActionTarget(0)) return GetActionTarget(1);
	if (source == GetActionTarget(1)) return GetActionTarget(0);
	// source is invalid
	return nil;
}

public func StartFusing(object controller)
{
	var fx = GetEffect("IntFusing", this);
	if (fx) 
	{
		// Double fuse start scheduled? Ignore.
		if (fx.fuse_source == controller)
			return false;
		// Fuse from both sides not supported
		return RemoveObject();
	}
	
	// Set a controller for this wire for killtracing, which can be passed to the fusing object.
	SetController(controller->GetController());	
		
	var fuse_dir;	
	var fuse_call;
	var fuse_vertex;
	if (GetActionTarget(0) == controller)
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
	fx = AddEffect("IntFusing", this, 100, 1, this);
	fx.fuse_dir = fuse_dir;
	fx.fuse_source = controller;
	fx.fuse_call = fuse_call;
	fx.fuse_vertex = fuse_vertex;
	fx.fuse_x = GetVertex(fuse_vertex, VTX_X) * 10;
	fx.fuse_y = GetVertex(fuse_vertex, VTX_Y) * 10;
	return true;
}

protected func FxIntFusingStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return FX_OK;
	SetAction("Fusing");
	Sound("Fire::FuseLoop", false, 75, nil, 1);
	return FX_OK;
}

protected func FxIntFusingTimer(object target, proplist effect, int time)
{
	var target_x = GetVertex(effect.fuse_vertex + effect.fuse_dir, VTX_X) * 10;
	var target_y = GetVertex(effect.fuse_vertex + effect.fuse_dir, VTX_Y)*10;

	var speed = 20;
	if (Distance(effect.fuse_x, effect.fuse_y, target_x, target_y) < speed)
	{
		RemoveVertex(effect.fuse_vertex);
		if (effect.fuse_dir == -1) 
			effect.fuse_vertex--;
		speed -= Distance(effect.fuse_x, effect.fuse_y, target_x, target_y);
		if ((effect.fuse_vertex == 0 && effect.fuse_dir == -1) || (effect.fuse_vertex == GetVertexNum() - 1 && effect.fuse_dir == +1))
		{
			if (effect.fuse_call)
				effect.fuse_call->~OnFuseFinished(this);
			if (effect.fuse_call) 
				RemoveObject();
			return FX_Execute_Kill;
		}
		effect.fuse_x = GetVertex(effect.fuse_vertex, VTX_X) * 10;
		effect.fuse_y = GetVertex(effect.fuse_vertex, VTX_Y) * 10;
		target_x = GetVertex(effect.fuse_vertex + effect.fuse_dir, VTX_X) * 10;
		target_y = GetVertex(effect.fuse_vertex + effect.fuse_dir, VTX_Y) * 10;
	}
	// Move spark position
	var angle = Angle(effect.fuse_x, effect.fuse_y, target_x, target_y);
	effect.fuse_x += Sin(angle, speed);
	effect.fuse_y +=-Cos(angle, speed);

	CreateParticle("Fire", effect.fuse_x / 10 - GetX(), effect.fuse_y / 10 - GetY(), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 40), Particles_Glimmer(), 3);
	SetVertexXY(effect.fuse_vertex, effect.fuse_x / 10, effect.fuse_y / 10);
	return FX_OK;
}

protected func FxIntFusingStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return FX_OK;
	Sound("Fire::FuseLoop", false, 75, nil, -1);
	return FX_OK;
}

// Store as connector
public func SaveScenarioObject(proplist props)
{
	if (!_inherited(props)) return false;
	props->AddCall("Fuse", GetID(), "Create", GetActionTarget(0), GetActionTarget(1));
	return true;
}


/*-- Properties --*/

local ActMap = {
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
		Procedure = DFA_NONE,
		NextAction = "Fusing",
	},
};
local Name = "$Name$";
	
