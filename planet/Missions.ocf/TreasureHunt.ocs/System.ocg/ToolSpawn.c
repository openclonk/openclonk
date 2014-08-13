/* Save tool_spawn property to scenario */
// tool_spawn is evaluated by scenario script

#appendto Chest

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (this.tool_spawn) props->AddSet("ToolSpawn", this, "tool_spawn", this.tool_spawn);
	return true;
}