/*-- 
	Moving Bricks 
	
--*/

local size; // Length of the brick.

// Constants
local MovementType = { // enum
	None = nil,
	Horizontal = 1,
	Vertical = 2,
	Graph = 3
};
local EdgeSpeed_Infinite = -1;
local EdgeAdvancement = { // enum: How to select the next edge when advancing on the graph
	Random = nil,
	Clockwise = 1,
	Counterclockwise = 2
};

// Movement properties (for editor)
local movement;
local movement_speed = 100;
// Movement on graph
local current_edge_index, current_vertex_index, target_vertex_index;
local vertex_wait_time;
local last_vx, last_vy, target_pos_x, target_pos_y;


protected func Initialize()
{
	// No movement by default
	movement = { Type=MovementType.None };
	// Size defaults to four.
	SetSize(4);
	
	// Allow for dynamically changing speeds.
	ActMap = { Prototype = this.Prototype.ActMap };
	ActMap.Moving = { Prototype = ActMap.Moving };
	movement_speed = ActMap.Moving.Speed;
	
	// Set floating action.
	SetAction("Moving");
	SetComDir(COMD_None);

	return;
}

public func SetSize(int to_size)
{
	size = BoundBy(to_size, 1, 4);
	// Update graphics.
	var graph = Format("Size%dN%d", size, 1 + Random(1));
	SetGraphics(graph);
	SetShape(-20,-4,size*10,8);
	// Update solid
	SetSolidMask(0,size*8-8,10*size,8);
	return;
}

public func SetMoveSpeed(int speed)
{
	movement_speed = ActMap.Moving.Speed = Max(0, speed);
	return;
}

public func ClearMovement()
{
	RemoveEffect("MoveHorizontal", this);
	RemoveEffect("MoveVertical", this);
	RemoveEffect("MoveOnGraph", this);
	movement = { Type=MovementType.None };
	current_edge_index = current_vertex_index = -1;
	SetComDir(COMD_None);
	SetXDir(); SetYDir();
	return true;
}

/*-- Horizontal movement --*/

public func MoveHorizontal(int left, int right, int speed)
{
	ClearMovement();
	var effect = AddEffect("MoveHorizontal", this, 100, 1, this);
	effect.Left = left;
	effect.Right = right;
	if (speed != nil)
		SetMoveSpeed(10 * speed);
	if (GetComDir() != COMD_Right) SetComDir(COMD_Left);
	// Props for editor display of movement range
	movement = { Type = MovementType.Horizontal, Graph = [{X=left, Y=GetY()}, {X=right, Y=GetY()}] };
	return;
}

private func FxMoveHorizontalTimer(object target, proplist effect)
{
	if (target->GetX() > effect.Right - 23 + 10*(4 - size))
		if (target->GetComDir() == COMD_Right)
			SetComDir(COMD_Left);
			
	if (target->GetX() < effect.Left + 23)
		if (target->GetComDir() == COMD_Left)
			SetComDir(COMD_Right);	

	return 1;
}

/*-- Vertical movement --*/

public func MoveVertical(int top, int bottom, int speed)
{
	ClearMovement();
	var effect = AddEffect("MoveVertical", this, 100, 1, this);
	effect.Top = top;
	effect.Bottom = bottom;
	if (speed != nil)
		SetMoveSpeed(10 * speed);
	if (GetComDir() != COMD_Down) SetComDir(COMD_Up);
	// Props for editor display of movement range
	movement = { Type=MovementType.Vertical, Graph = [{X=GetX(), Y=top}, {X=GetX(), Y=bottom}] };
	return;
}

private func FxMoveVerticalTimer(object target, proplist effect)
{
	if (target->GetY() > effect.Bottom - 7)
		if (target->GetComDir() == COMD_Down)
			SetComDir(COMD_Up);
			
	if (target->GetY() < effect.Top + 7)
		if (target->GetComDir() == COMD_Up)
			SetComDir(COMD_Down);

	return 1;
}


/*-- Movement on graph --*/

public func MoveOnGraph(proplist graph)
{
	ClearMovement();
	movement = { Type = MovementType.Graph, Graph = graph };
	OnGraphUpdate(movement.Graph);
	var effect = AddEffect("MoveOnGraph", this, 100, 1, this);
	effect.movement_graph = graph;
	return;
}

private func FxMoveOnGraphTimer(object target, proplist effect)
{
	// Vertex wait time?
	if (vertex_wait_time)
	{
		if (!--vertex_wait_time)
		{
			OnVertexReached(current_vertex_index, true);
		}
		return;
	}
	// Check if the target vertex has been reached
	var dx = target_pos_x - GetX();
	var dy = target_pos_y - GetY();
	if (dx * last_vx + dy * last_vy <= 0)
	{
		OnVertexReached(target_vertex_index);
	}
	return 1;
}

private func StartEdgeMovement(int edge_index, int edge_direction)
{
	// Initiate movement towards target vertex
	//Log("%v Start edge movement %v (%v)", this, edge_index, edge_direction);
	current_edge_index = edge_index;
	current_vertex_index = -1;
	var edge = movement.Graph.Edges[edge_index];
	target_vertex_index = edge.Vertices[edge_direction];
	var target_vertex = movement.Graph.Vertices[target_vertex_index];
	target_pos_x = target_vertex.X;
	target_pos_y = target_vertex.Y;
	var speed = edge.Speed ?? movement_speed;
	if (speed == EdgeSpeed_Infinite)
	{
		SetPosition(target_pos_x, target_pos_y);
		speed = 100; // For trigger
	}
	else
	{
		var dx = target_pos_x - GetX();
		var dy = target_pos_y - GetY();
		var d = Max(1, Distance(dx, dy));
		SetXDir((last_vx = speed * dx / d), 100);
		SetYDir((last_vy = speed * dy / d), 100);
		ActMap.Moving.Speed = Max(0, speed);
	}
}

private func OnVertexReached(int vertex_index, bool after_wait)
{
	//Log("Vertex reached %v (%v)", vertex_index, after_wait);
	// Safety
	if (vertex_index < 0 || vertex_index >= GetLength(movement.Graph.Vertices))
	{
		vertex_index = 0;
	}
	// Called when a vertex has been reached. Perform vertex action & continue movement
	current_vertex_index = vertex_index;
	target_vertex_index = -1;
	var vertex = movement.Graph.Vertices[vertex_index];
	if (!after_wait)
	{
		// Stop for now (befure UserAction because UserAction may do movement)
		SetXDir(); SetYDir();
		// Vertex action
		if (vertex.ArrivalAction)
		{
			UserAction->EvaluateAction(vertex.ArrivalAction, this);
		}
		// Vertex wait time? Finish for now; function Will be called again later.
		if (vertex.WaitTime)
		{
			vertex_wait_time = vertex.WaitTime;
			return;
		}
	}
	// Find next vertex to continue to
	var vertex = movement.Graph.Vertices[vertex_index];
	var n_edges = GetLength(vertex._edges);
	if (!n_edges) return; // Stuck on an unconnected vertex
	// First figure out where we came from
	var last_edge_index = -1; // index in vertex._edges array
	if (current_edge_index >= 0)
	{
		last_edge_index = GetIndexOf(vertex._edges, movement.Graph.Edges[current_edge_index]);
	}
	// Advance in specified order
	var advancement = vertex.Advancement;
	var next_edge_index = 0; // index in vertex._edges array
	if (advancement == EdgeAdvancement.Random)
	{
		var exclude_current = (last_edge_index >= 0 && n_edges > 1);
		next_edge_index = Random(n_edges - exclude_current);
		if (next_edge_index == last_edge_index) next_edge_index += exclude_current;
	}
	else if (advancement == EdgeAdvancement.Clockwise)
	{
		next_edge_index = (last_edge_index + 1) % n_edges;
	}
	else if (advancement == EdgeAdvancement.Counterclockwise)
	{
		next_edge_index = last_edge_index - 1;
		if (next_edge_index < 0) next_edge_index = n_edges - 1;
	}
	var next_edge = vertex._edges[next_edge_index];
	StartEdgeMovement(next_edge._index, next_edge.Vertices[0] == current_vertex_index);
}


/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (size != 4) props->AddCall("Size", this, "SetSize", size);
	if (movement_speed != GetID().ActMap.Moving.Speed) props->AddCall("MoveSpeed", this, "SetMoveSpeed", movement_speed);
	if (GetComDir() == COMD_None) props->Remove("ComDir");
	return true;
}

func FxMoveHorizontalSaveScen(obj, fx, props)
{
	props->AddCall("Move", obj, "MoveHorizontal", fx.Left, fx.Right);
	return true;
}

func FxMoveVerticalSaveScen(obj, fx, props)
{
	props->AddCall("Move", obj, "MoveVertical", fx.Top, fx.Bottom);
	return true;
}

func FxMoveOnGraphSaveScen(obj, fx, props)
{
	props->AddCall("Move", obj, "MoveOnGraph", fx.movement_graph);
	return true;
}


/* Editor */

public func SetMoveType(new_movement)
{
	var movement_type = new_movement.Type;
	// Set movement in editor: Set default graphs
	if (movement_type == MovementType.Horizontal)
	{
		MoveHorizontal(Max(GetX()-50, 10), Min(GetX()+50, LandscapeWidth()-10));
	}
	else if (movement_type == MovementType.Vertical)
	{
		MoveVertical(Max(GetY()-50, 10), Min(GetY()+50, LandscapeHeight()-10));
	}
	else if (movement_type == MovementType.Graph)
	{
		var x = BoundBy(GetX(), 30, LandscapeWidth()-30), y = BoundBy(GetY(), 50, LandscapeHeight()-50);
		MoveOnGraph({ Vertices=[{X=x, Y=y}, {X=x-20, Y=y-40}, {X=x+20, Y=y-40}, {X=x, Y=y+40}], Edges=[{Vertices=[0, 1]}, {Vertices=[0, 2]}, {Vertices=[0, 3]}] });
	}
	else
	{
		ClearMovement();
	}
	return true;
}

public func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.movement_speed = { Name="$Speed$", EditorHelp="$SpeedHelp$", Type="int", Min=5, Set="SetMoveSpeed" };
	def.EditorProps.movement = { Name="$Movement$", EditorHelp="$MovementHelp$", Type="enum", Set="SetMoveType", OptionKey="Type", ValueKey="Graph", Options=[
		{ Name="$NoMovement$", EditorHelp="$NoMovementHelp$", Value={Type=MovementType.None} },
		{ Name="$Horizontal$", EditorHelp="$HorizontalHelp$", Value={Type=MovementType.Horizontal}, Delegate={ Type="polyline", VerticalFix=true, StructureFix=true, OnUpdate="OnHorizontalGraphUpdate", Relative=false, Color=0xff2010 } },
		{ Name="$Vertical$", EditorHelp="$VerticalHelp$", Value={Type=MovementType.Vertical}, Delegate={ Type="polyline", HorizontalFix=true, StructureFix=true, OnUpdate="OnVerticalGraphUpdate", Relative=false, Color=0x20ff10 } },
		{ Name="$Graph$", EditorHelp="$GraphHelp$", Value={Type=MovementType.Graph}, Delegate={ Type="graph", OnUpdate="OnGraphUpdate", Relative=false, Color=0xef8000,
			EdgeDelegate = { Name="$Edge$", EditorHelp="$EdgeHelp$", EditorProps={
				Speed = { Name="$Speed$", EditorHelp="$EdgeSpeedHelp$", Type="enum", Options=[
					{Name="$Default$", EditorHelp="$DefaultHelp$"},
					{Name="$Instant$", EditorHelp="$InstantSpeedHelp$", Value=EdgeSpeed_Infinite },
					{Name="$CustomSpeed$", EditorHelp="$CustomSpeedHelp$", Type=C4V_Int, Delegate={Type="int", Min=5} }
					] },
				} },
			VertexDelegate = { Name="$Vertex$", EditorHelp="$VertexHelp$", EditorProps={
				ArrivalAction = new UserAction.Prop { Name="$ArrivalAction$", EditorHelp="$ArrivalActionHelp$" },
				WaitTime = {Name="$VertexWaitTime$", EditorHelp="$VertexWaitTimeHelp$", Type="int", Min=0},
				Advancement = { Name="$Advancement$", EditorHelp="$AdvancementHelp$", Type="enum", Options=[
					{ Name="$Random$", EditorHelp="$RandomAdvancementHelp$", Value=EdgeAdvancement.Random },
					{ Name="$Clockwise$", EditorHelp="$CWAdvancementHelp$", Value=EdgeAdvancement.Clockwise },
					{ Name="$Counterclockwise$", EditorHelp="$CCWAdvancementHelp$", Value=EdgeAdvancement.Counterclockwise }
					] }
				} },
		 } },
	]};
	// TODO: Button to flip direction
}

public func EditCursorMoved(int old_x, int old_y, bool movement_finished)
{
	// Move horizontal/vertical graph elements
	if (movement.Type == MovementType.Horizontal)
	{
		movement.Graph = [{X=movement.Graph[0].X, Y=GetY()}, {X=movement.Graph[1].X, Y=GetY()}];
	}
	else if (movement.Type == MovementType.Vertical)
	{
		movement.Graph = [{Y=movement.Graph[0].Y, X=GetX()}, {Y=movement.Graph[1].Y, X=GetX()}];
	}
	else if (movement.Type == MovementType.Graph)
	{
		// Snap to nearest edge if mouse is let go
		if (movement_finished)
		{
			SetPositionToNearestEdge();
		}
		else
		{
			// Stop movment during drag (will be reactivated in SetPositionToNearestEdge)
			ActMap.Moving.Speed = 0;
		}
	}
	// return true to signal engine to update shapes
	return true;
}

private func OnHorizontalGraphUpdate(array vertices)
{
	var fx = GetEffect("MoveHorizontal", this);
	if (fx)
	{
		fx.Left = Min(vertices[0].X, vertices[1].X);
		fx.Right = Max(vertices[0].X, vertices[1].X);
	}
}

private func OnVerticalGraphUpdate(array vertices)
{
	var fx = GetEffect("MoveVertical", this);
	if (fx)
	{
		fx.Top = Min(vertices[0].Y, vertices[1].Y);
		fx.Bottom = Max(vertices[0].Y, vertices[1].Y);
	}
}

private func OnGraphUpdate(proplist graph)
{
	// Graph update
	// Update vertex indices
	var vertex, ivertex = 0, n, edge, iedge = 0;
	for (vertex in graph.Vertices)
	{
		vertex._index = ivertex++;
	}
	for (edge in graph.Edges)
	{
		edge._index = iedge++;
	}
	// Update all connectivities
	for (vertex in graph.Vertices)
	{
		// Order connected edges by angle
		n = 0;
		vertex._edges = [];
		for (var edge in graph.Edges)
		{
			var evtxidx = GetIndexOf(edge.Vertices, vertex._index);
			if (evtxidx > -1)
			{
				var connected_vertex = graph.Vertices[edge.Vertices[1 - evtxidx]];
				vertex._edges[n++] = edge;
				edge._angle = Angle(vertex.X, vertex.Y, connected_vertex.X, connected_vertex.Y);
			}
		}
		// Order them by angle to allow clockwise/counter-clockwise graph traversal
		SortArrayByProperty(vertex._edges, "_angle");
	}
	// Reset position to closest edge
	SetPositionToNearestEdge();
	// Done
	return true;
}

private func SetPositionToNearestEdge()
{
	// First check if we're close to a current vertex
	var x = GetX(), y = GetY();
	//Log("x=%d, y=%d", x, y);
	if (current_vertex_index >= 0 && current_vertex_index < GetLength(movement.Graph.Vertices))
	{
		var current_vertex = movement.Graph.Vertices[current_vertex_index];
		if (Distance(current_vertex.X, current_vertex.Y, x, y) < 8)
		{
			// no reset necessery. But do restart edge movement if e.g. stuck on an unconnected vertex that just got connected.
			if (!vertex_wait_time) OnVertexReached(current_vertex_index, true);
		}
	}
	current_vertex_index = target_vertex_index = -1;
	vertex_wait_time = 0;
	// Reset position to closest edge
	var best_distance, edge_index = 0, best_edge;
	var proj_x, proj_y, edge_direction;
	for (var edge in movement.Graph.Edges)
	{
		var v0 = movement.Graph.Vertices[edge.Vertices[0]], v1 = movement.Graph.Vertices[edge.Vertices[1]];
		var dx0 = x - v0.X, dy0 = y - v0.Y;
		var dx1 = v1.X - v0.X, dy1 = v1.Y - v0.Y;
		var d = dx1*dx1 + dy1*dy1;
		// Perpendicular distance
		var dp = Abs(dx0 * dy1 - dx1 * dy0);
		// Penalty if outside edge along the edge axis
		var da = dx0 * dx1 + dy0 * dy1;
		// Total distance (multiplied by d1 length)
		var distance = dp + Max(-da) + Max(da - d);
		//Log("%d (%v, %v) dist = %d", edge_index, {X=v0.X, Y=v0.Y, idx=v0._index}, {X=v1.X, Y=v1.Y, idx=v1._index}, distance);
		//Log("dx0=%d, dx1=%d, dy0=%d, dy1=%d, d=%d, dp=%d, da=%d, distance=%d", dx0, dx1, dy0, dy1, d, dp, da, distance);
		if (!edge_index || distance < best_distance)
		{
			best_edge = edge_index;
			best_distance = distance;
			edge_direction = (last_vx * dx1 + last_vy * dy1 >= 0);
			if (best_edge == current_edge_index && best_distance/Distance(dx1, dy1) < 6)
			{
				// Stay on current edge
				StartEdgeMovement(best_edge, edge_direction);
				return;
			}
			da = BoundBy(da, 0, d); // Da da da! Stay inside edge range!
			proj_x = x - dx0 + dx1 * da / d;
			proj_y = y - dy0 + dy1 * da / d;
		}
		++edge_index;
	}
	current_edge_index = -1;
	if (!edge_index) return; // No edges. Nothing to do.
	//Log("Snap to edge %d", best_edge);
	// Project position onto closest edge
	SetPosition(proj_x, proj_y);
	// Start movement along this edge
	StartEdgeMovement(best_edge, edge_direction);
}


/* Properties */

local ActMap = {
	Moving = {
		Prototype = Action,
		Name = "Moving",
		Procedure = DFA_FLOAT,
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Accel = 20,
		Decel = 20,
		Speed = 100,
		Wdt = 40,
		Hgt = 8,
		NextAction = "Moving",
	},
};
local Name = "MovingBrick";
local Plane = 200;
local SolidMaskPlane = 150; // move almost everything so background stuff can be put onto moving bricks
