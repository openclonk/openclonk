// Limits the use of an object to a certain amount.

#appendto JarOfWinds
#appendto Club

local use_count;

protected func Initialize()
{
	SetUseCount(15);
	return _inherited(...);
}

public func SetUseCount(int count)
{
	use_count = Max(0, count);
	// Update HUD.
	UpdatePicture();
	return;
}

public func ControlUseStart()
{
	var inh = _inherited(...);
	SetUseCount(use_count - 1);

	return inh;
}

public func ControlUseStop()
{
	var inh = _inherited(...);
	
	if (use_count == 0)
		ScheduleRemove();
		
	return inh;
}

public func ControlUse()
{
	var inh = _inherited(...);
	SetUseCount(use_count - 1);
	
	if (use_count == 0)
		ScheduleRemove();
	
	return inh;
}

private func ScheduleRemove()
{
	Exit();
	RemoveObject();
	return;
}

private func UpdatePicture()
{
	var one = use_count % 10;
	var ten = (use_count / 10) % 10;
	var hun = (use_count / 100) % 10;
	
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;
	
	if (hun > 0)
	{
		SetGraphics(Format("%d", hun), Icon_Number, 10, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - 2 * spacing, 0, s, yoffs, 10);
	}
	else
		SetGraphics(nil, nil, 10);

	if (ten > 0 || hun > 0)
	{
		SetGraphics(Format("%d", ten), Icon_Number, 11, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - spacing, 0, s, yoffs, 11);
	}
	else
		SetGraphics(nil, nil, 11);
		
	SetGraphics(Format("%d", one), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
	return;
}