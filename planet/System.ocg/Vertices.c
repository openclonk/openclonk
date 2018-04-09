/**
	Vertices.c
	Vertex related functions.
	
	@author Clonkonaut, flgr
*/

// Sets both the X and Y-coordinate of one vertex.
// documented in /docs/sdk/script/fn
global func SetVertexXY(int index, int x, int y)
{
	// Set vertices.
	SetVertex(index, VTX_X, x);
	SetVertex(index, VTX_Y, y);
	return;
}

// Returns the number of stuck vertices. (of this)
// documented in /docs/sdk/script/fn
global func VerticesStuck()
{
	var vertices = 0;
	// Loop through vertices.
	for (var i = -1; i < GetVertexNum(); i++)
		// Solid?
		if (GBackSolid(GetVertex(i, VTX_X), GetVertex(i, VTX_Y)))
			// Count vertices.
			vertices++;
	return vertices;
}

// Automatically move an object up to <range> pixels in each direction if it is stuck
// documented in /docs/sdk/script/fn
global func Unstick(int range)
{
	if (Stuck())
	{
		for (var i = 1; i <= (range ?? 7); ++i)
		{
			for (var d in [[0,-i], [0,i], [-i,0], [i,0]])
			{
				if (!Stuck(d[0], d[1]))
				{
					if (Inside(GetX()+d[0], 0, LandscapeWidth()-1) && GetY()+d[1] < LandscapeHeight()) // But do not push outside landscape!
					{
						return SetPosition(GetX() + d[0], GetY() + d[1]);
					}
				}
			}
		}
	}
	// Unsticking failed (or not stuck).
	return false;
}

// Returns whether the object has a vertex with the given CNAT value
global func HasCNAT(int cnat)
{
	for (var i = -1; i < GetVertexNum(); i++)
		if (GetVertex(i, VTX_CNAT) == cnat)
			return true;
	return false;
}

// e.g. clonk->SetVertexCNAT(k, CNAT_CollideHalfVehicle, true); for k != 2 makes the clonk behave correctly wrt. to HalfVehicle
global func SetVertexCNAT(int vtx, int val, bool set)
{
	if (val == nil || set == nil)
		return FatalError("this function requires its second and third parameter to be non-nil");
	if (!this)
		return FatalError("this function requires object context");
	if (vtx == nil)
		for (var i = GetVertexNum(); i-->0;)
			SetVertexCNAT(i, val, set);
	else
		SetVertex(vtx, VTX_CNAT, GetVertex(vtx, VTX_CNAT) & ~val | (set && val), 2);
}

// Allow falling down throughg a HalfVehicle platform
global func HalfVehicleFadeJumpStart()
{
	if (!this)
		return FatalError("this function requires object context");
	if (GetEffect("IntHalfVehicleFadeJump", this))
		return;
	AddEffect("IntHalfVehicleFadeJump", this, 1);
}


global func HalfVehicleFadeJumpStop()
{
	if (!this)
		return FatalError("this function requires object context");
	var fx = GetEffect("IntHalfVehicleFadeJump", this);
	if (fx)
		fx.Interval = 1;
}

global func FxIntHalfVehicleFadeJumpStart(object target, effect fx, int temp)
{
	if (temp)
		return FX_OK;
	if (!target) {
		return FX_Start_Deny;
	}
	fx.collideverts = CreateArray();
	for (var i = target->GetVertexNum(); i-->0;)
		if(!(target->GetVertex(i, VTX_CNAT) & CNAT_PhaseHalfVehicle)) {
			PushBack(fx.collideverts, i);
			target->SetVertexCNAT(i, CNAT_PhaseHalfVehicle, true);
		}
	fx.origpos = target->GetPosition();
	return FX_OK;
}

global func FxIntHalfVehicleFadeJumpTimer(object target, effect fx, int time)
{
	if (DeepEqual(target->GetPosition(), fx.origpos))
		return FX_OK;
	for (var i = GetLength(fx.collideverts); i-- > 0;)
	{
		if (target->GetMaterial(target->GetVertex(fx.collideverts[i], VTX_X),
		                        target->GetVertex(fx.collideverts[i], VTX_Y)) == Material("HalfVehicle"))
			return FX_OK;
	}
	return FX_Execute_Kill;
	// The way this is implemented, it may ignore smaller cracks beteween half-solid masks at high speeds. Fix if necessary.
}

global func FxIntHalfVehicleFadeJumpStop(object target, effect fx, int reason, bool temp)
{
	if (reason == FX_Call_RemoveClear)
		return;
	for (var i = GetLength(fx.collideverts); i-- > 0;)
		target->SetVertexCNAT(fx.collideverts[i], CNAT_PhaseHalfVehicle, false);
}
