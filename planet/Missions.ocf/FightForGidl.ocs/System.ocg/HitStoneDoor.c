// Special handling: Stone door is hit even if path was not free

global func Find_PathFree(target) { return Find_Or(Find_ID(StoneDoor), _inherited(target, ...)); }
