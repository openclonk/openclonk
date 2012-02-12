/**
	HUD Marker
	Informs the player about various events.
	Marker will inform the toInform object via:
	* MarkerSelected(marker)
	* MarkerAltSelected(marker)
	* MarkerRemoved(marker)
	
	@authors Mimmo_O
*/

local picture, toInform;

protected func FxIntRemoveMarkerTimer()
{
	RemoveObject();
}

protected func FxIntUrgentMarkerTimer(target,eff,t)
{
	SetClrModulation(RGB(255,175+Sin(t*10,80),175+Sin(t*10,80)),2);
	SetClrModulation(RGB(255,175+Sin(t*10,80),175+Sin(t*10,80)),1);
}

protected func FxIntUrgentMarkerStop()
{
	SetClrModulation(RGB(255,255,255),2);
	SetClrModulation(RGB(255,255,225),1);
}

protected func Construction()
{

	picture = 0;
	toInform = nil;

	
	// parallaxity
	this["Parallaxity"] = [0,0];
	
	// visibility
	this["Visibility"] = VIS_Owner;
	
	SetGraphics(0,GetID(),2,GFXOV_MODE_Base);
	SetGraphics("",GUI_Controller,0);
	SetGraphics("Line",GetID(),1,GFXOV_MODE_Base);
	SetObjDrawTransform(1000,0,-225*100,0,1000,0,1);
}

public func SetVisual(picture, altpicture)
{
	if(!picture)
	{
		SetGraphics(nil, nil, 10);
		SetGraphics(nil, nil, 11);
		SetGraphics(nil, nil, 12);
	}
	else
	{
		if (GetType(picture) == C4V_C4Object)
		{
			SetGraphics(nil, nil, 10, GFXOV_MODE_ObjectPicture, 0, 0, picture);
			SetObjDrawTransform(780, 0, 0, 0, 780, 0, 10);
			if (picture->~HasExtraSlot())
			{
				SetGraphics(nil, GUI_ExtraSlot, 11, GFXOV_MODE_Base);
				SetObjDrawTransform(780, 0, 16*780, 0, 780, 16*780, 11);
				var content = picture->Contents(0);
				if (content)
				{
					SetGraphics(nil, nil, 12, GFXOV_MODE_ObjectPicture, 0, 0, content);
					SetObjDrawTransform(1500/3, 0, 16*780, 0, 1500/3, 16*780, 12);
				}
				else
					SetGraphics(nil, nil, 12);
			}
			else
			{
				SetGraphics(nil, nil, 11);
				SetGraphics(nil, nil, 12);
			}
		}
		else
		if(!altpicture)
			SetGraphics(nil,picture,10,GFXOV_MODE_IngamePicture);
		else
			SetGraphics(altpicture,picture,10,GFXOV_MODE_IngamePicture);
		
		SetName(picture->GetName());
	}
}

public func Hide()
{
	this["Visibility"] = VIS_None;
}

public func Show()
{
	this["Visibility"] = VIS_Owner;
}

public func MouseSelectionAlt(int plr)
{
	if(toInform) toInform->~MarkerAltSelected(this);
}

public func MouseSelection(int plr)
{
	if(GetEffect("IntUrgentMarker",this)) RemoveEffect("IntUrgentMarker",this);
	if(toInform) toInform->~MarkerSelected(this);
}

protected func Destruction()
{
	if(toInform) toInform->~MarkerRemoved(this);
}



