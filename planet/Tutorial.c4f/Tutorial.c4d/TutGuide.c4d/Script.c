/*-- 
		Tutorial Guide
		Author: Maikel
		
		The tutorial guide can be clicked on by the player, it supplies the player with information and hints.
--*/


local msg;

protected func Construction()
{
	// parallaxity
	this["Parallaxity"] = [0,0];
	// visibility
	this["Visibility"] = VIS_Owner;
}

global func CreateTutorialGuide(int plr)
{
	var guide = CreateObject(TutorialGuide, 0, 0 , plr);
	guide->SetPosition(- 128 - 32 - TutorialGuide->GetDefHeight() / 2, 8 + TutorialGuide->GetDefHeight() / 2);
	return guide;
}

static const GUIDE_Graphics_Normal = 0;
static const GUIDE_Graphics_Balloon = 1;
static const GUIDE_Graphics_Idea = 2;

public func SetGuideGraphics(int mode, extra)
{
	if (mode == GUIDE_Graphics_Normal)
		SetGraphics(0, 0, 0, GFXOV_MODE_IngamePicture);
	else if (mode == GUIDE_Graphics_Balloon)
	{
		SetGraphics("2", 0, 0, GFXOV_MODE_IngamePicture);
		if (extra)
		{
			SetGraphics(0, extra, 1, GFXOV_MODE_IngamePicture);
			SetObjDrawTransform(400, 0, -15000, 0, 400, -13000, 1);
		} else
		{
			SetGraphics(0, 0, 1, GFXOV_MODE_IngamePicture);
		}
	}
	else if (mode == GUIDE_Graphics_Idea)
		SetGraphics("3", 0, 0, GFXOV_MODE_IngamePicture);

	return;
}	

public func SetGuideMessage(string to_msg)
{
	msg = to_msg;
	return;
}

public func ShowGuideMessage()
{
	if (GetOwner() == NO_OWNER)
		return;
	if (!msg) 
		return;	
	return MessageWindow(msg, GetOwner());
}

public func MouseSelection(int plr)
{
	if (plr != GetOwner())
		return;
	if (!msg) 
		return;
	return MessageWindow(msg, plr);
}

protected func Definition(def)
{
	def["Name"] = "Prof. clonkine";
}
