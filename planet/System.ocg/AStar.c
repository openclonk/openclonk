/**
	AStar.c
	Configurable pathfinder for graphs.

	@author Luchs	
*/

// Generic A* implementation.
static const AStar = new Global
{
	/* Overwrite these functions */
	/* ========================= */

	// Distance heuristic for selecting nodes. A good heuristic will speed up
	// A*, a bad heuristic can lead to non-optimal paths.
	distance = func(node, goal) { FatalError("This function needs to be implemented by an actual algorithm. The generic function does not exist and throws this error on purpose."); },

	// Cost between two neighboring nodes. You'll need a different
	// implementation if your goal isn't a regular node.
	cost = func() { return this->distance(...); },

	// Returns an array of neighboring nodes.
	successors = func(a) { FatalError("This function needs to be implemented by an actual algorithm. The generic function does not exist and throws this error on purpose."); },

	// Equality function for nodes. DeepEqual works well for for objects as
	// well as state proplists, but you may be able to supply a more efficient
	// implementation.
	node_equal = DeepEqual,

	// Goal identification. You'll need a different implementation if your goal
	// isn't a regular node.
	goal_equal = func(node, goal) { return this->node_equal(node, goal); },

	/* Use these functions */
	/* =================== */

	// Find a path from start to goal. Valid types for both are defined by the
	// functions above.
	FindPath = func(start, goal)
	{
		var state = {
			open = [[this->distance(start, goal), 0, start]],
			closed = [],
			goal = goal,
		};
		var current;
		while (GetLength(state.open))
		{
			current = MinHeap->Extract(state.open);
			if (this->goal_equal(current[2], goal))
			{
				// Reconstruct the path.
				var path = [current[2]];
				while (current = current[3])
					PushFront(path, current[2]);
				return path;
			}
			PushBack(state.closed, current[2]);
			_Expand(state, current);
		}
		return nil;
	},

	/* Internal functions */
	/* ================== */

	_Expand = func(proplist state, array current)
	{
		/* Log("open: %v, closed: %v", state.open, state.closed); */
		/* Log("current: %v", current); */
		for (var successor in this->successors(current[2]))
		{
			/* Log(" - successor: %v", successor); */
			// Skip successor if it's in the closed list.
			var i = 0, el;
			while ((el = state.closed[i++]) && !this->node_equal(el, successor));
			if (el)
				continue;
			var cost = current[1] + this->cost(current[2], successor);
			// Find successor in the open list.
			i = 0;
			while ((el = state.open[i++]) && !this->node_equal(el[2], successor));
			if (el && el[1] <= cost)
				continue;
			successor = [cost + this->distance(successor, state.goal), cost, successor, current];
			if (el)
			{
				el[:] = successor;
				MinHeap->DecreaseKey(state.open, i-1);
			}
			else
				MinHeap->Insert(state.open, successor);
		}
	},

};

// Sample implementation for searching paths on the map, i.e. "intelligent PathFree()".
//
// Graph nodes are a regular grid over the landscape with a configurable size
// (step). Nodes are represented as {x, y} proplists.
static const AStarMap = new AStar
{
	// This function is used both as heuristic (node to the goal) and as cost
	// function (two neighboring nodes).
	distance = func(proplist a, proplist b)
	{
		// Manhattan distance
		return Abs(a.x - b.x) + Abs(a.y - b.y);
	},

	// Returns all neighboring nodes (right/down/left/up) with a free path.
	successors = func(proplist a)
	{
		var successors = [], pt;
		if (pathfree(a, (pt = {x = a.x + this.step, y = a.y}))) PushBack(successors, pt);
		if (pathfree(a, (pt = {x = a.x, y = a.y + this.step}))) PushBack(successors, pt);
		if (pathfree(a, (pt = {x = a.x - this.step, y = a.y}))) PushBack(successors, pt);
		if (pathfree(a, (pt = {x = a.x, y = a.y - this.step}))) PushBack(successors, pt);
		return successors;
	},

	// Helper functions for successors()
	pathfree = func(proplist a, proplist b)
	{
		return PathFree(a.x, a.y, b.x, b.y);
	},

	// Start and goal may not be a multiple of `step` apart.
	goal_equal = func(proplist node, proplist goal)
	{
		var nx = node.x / step, ny = node.y / step;
		var gx = goal.x / step, gy = goal.y / step;
		return (nx == gx || nx == gx + 1) && (ny == gy || ny == gy + 1) && pathfree(node, goal);
	},

	step = 10

};

/* Binary Min-Heap */
static const MinHeap = new Global
{
	// A heap is an array of [key, value] array-tuples.
	Heapify = func(array heap, int i)
	{
		var min = i, size = GetLength(heap);
		var left = 2*i + 1, right = 2*i + 2;
		if (left < size && heap[left][0] < heap[min][0])
			min = left;
		if (right < size && heap[right][0] < heap[min][0])
			min = right;
		if (min != i)
		{
			ArraySwap(heap, min, i);
			Heapify(heap, min);
		}
	},

	BuildHeap = func(array a)
	{
		for (var i = GetLength(a)/2-1; i >= 0; --i)
			Heapify(a, i);
	},

	DecreaseKey = func(array heap, int i)
	{
		var parent;
		while (i > 0 && heap[i][0] < heap[(parent = (i-1)/2)][0])
		{
			ArraySwap(heap, i, parent);
			i = parent;
		}
	},

	Insert = func(array heap, array kv)
	{
		var i = GetLength(heap);
		heap[i] = kv;
		DecreaseKey(heap, i);
	},

	Extract = func(array heap)
	{
		var item = heap[0], last = PopBack(heap);
		if (GetLength(heap))
		{
			heap[0] = last;
			Heapify(heap, 0);
		}
		return item;
	},

	ArraySwap = func(array a, int i, int j)
	{
		var tmp = a[i];
		a[i] = a[j];
		a[j] = tmp;
	},

};
