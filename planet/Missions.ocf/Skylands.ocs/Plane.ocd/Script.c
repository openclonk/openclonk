/*--
	Plane construction site
	Author: Sven2

	Successive buildup of the plane
--*/

local progress, next_part;

public func Initialize()
{
	SetProgress(0);
	AddTimer("Timer", 10);
}

func SetProgress(int new_progress)
{
	var parts = [Airplane_Skids, Airplane_Chassis, Airplane_Wings, Airplane_Engine, Airplane_Propeller, nil];
	progress = new_progress;
	if (!progress)
	{
		SetGraphics("Site");
		SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
	}
	else if (progress < 5)
	{
		SetGraphics(Format("%d", new_progress));
		SetGraphics("Site", GetID(), GFX_Overlay, GFXOV_MODE_Base);
		GameCallEx("OnPlanePartAdded", this);
	}
	else
	{
		SetGraphics();
		SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
		GameCall("OnPlaneFinished", this);
		return true;
	}
	next_part = parts[progress];
	return true;
}

func Timer()
{
	if (next_part)
		for (var part in FindObjects(Find_ID(next_part), Find_InRect(-30,-15,60,30), Find_Layer(GetObjectLayer())))
			if (part->GetCon() >= 100)
			{
				AddPart(part);
				return;
			}
}

func AddPart(object part)
{
	part->RemoveRestoreMode();
	part->RemoveObject();
	Sound("UI::Ding", true);
	SetProgress(progress+1);
	return true;
}


func Definition(def) {

}

local Name = "$Name$";
