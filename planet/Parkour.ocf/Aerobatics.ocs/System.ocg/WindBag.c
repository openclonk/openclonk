// Restrict amount of usages with the windbag.

#appendto WindBag

local usage_count;

protected func Initialize()
{
	this.Description = Format("%s %s", this.Description, "$MsgWindBagUsageCount$");
	SetUsageCount(10);
	return _inherited(...);
}

public func SetUsageCount(int count)
{
	usage_count = BoundBy(count, 1, 99);
	if (Contained())
		Contained()->~OnInventoryChange();
	return;
}

public func GetUsageCount() { return usage_count; }

private func BlastWind(object clonk, int x, int y)
{
	usage_count--;
	clonk->~OnInventoryChange();
	return _inherited(clonk, x, y, ...);
}

public func FxIntBurstWindStop(object target, proplist effect, int reason, bool temp)
{
	var result = _inherited(target, effect, reason, temp, ...);
	if (!temp)
	{
		if (usage_count <= 0)
			RemoveObject();	
	}
	return result;
}

// Display the remaining amount of usages.
public func GetInventoryIconOverlay()
{
	return {Left = "40%", Bottom = "50%", 
		tens = {Right = "60%", Symbol = Icon_Number, GraphicsName = Format("%d", usage_count / 10)},
		ones = {Left = "40%", Symbol = Icon_Number, GraphicsName = Format("%d", usage_count % 10)},
	};
}