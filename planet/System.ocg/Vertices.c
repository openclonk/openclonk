/*--
		Vertices.c
		Authors: Clonkonaut, flgr

		Vertex related functions.
--*/

// Sets both the X and Y-coordinate of one vertex.
global func SetVertexXY(int index, int x, int y)
{
	// Set vertices.
	SetVertex(index, VTX_X, x);
	SetVertex(index, VTX_Y, y);
	return;
}

// Returns the number of stuck vertices. (of this)
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
global func HalfVehicleFadeJump()
{
	if (!this)
		return FatalError("this function requires object context");
	AddEffect("IntHalfVehicleFadeJump", this, 1, 1);
}

global func FxIntHalfVehicleFadeJumpStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	if (!target) {
		return FX_Start_Deny;
	}
	effect.collideverts = CreateArray();
	for (var i = target->GetVertexNum(); i-->0;)
		if(!(target->GetVertex(i, VTX_CNAT) & CNAT_PhaseHalfVehicle)) {
			PushBack(effect.collideverts, i);
			target->SetVertexCNAT(i, CNAT_PhaseHalfVehicle, true);
		}
	effect.origpos = target->GetPosition();
	return FX_OK;
}

global func FxIntHalfVehicleFadeJumpTimer(object target, proplist effect, int time)
{
	if (DeepEqual(target->GetPosition(), effect.origpos))
		return FX_OK;
	for (var i = GetLength(effect.collideverts); i-->0;) {
		if (target->GetMaterial(target->GetVertex(effect.collideverts[i], VTX_X),
		                        target->GetVertex(effect.collideverts[i], VTX_Y)) == Material("HalfVehicle"))
			return FX_OK;
	}
	return FX_Execute_Kill;
}

global func FxIntHalfVehicleFadeJumpStop(object target, proplist effect, int reason, bool temp)
{
	if (reason == FX_Call_RemoveClear)
		return;
	for (var i = GetLength(effect.collideverts); i-->0;)
			target->SetVertexCNAT(effect.collideverts[i], CNAT_PhaseHalfVehicle, false);
}
