
local Name = "$Name$";
local Description = "$Description$";

public func Initialize()
{
	return true;
}

// Only save main bush object
func SaveScenarioObject() { return false; }