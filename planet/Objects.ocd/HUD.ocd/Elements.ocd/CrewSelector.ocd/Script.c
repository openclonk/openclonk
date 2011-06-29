/*
	Crew selector HUD
	Author: Newton
	
	For each crew member, one of these HUD elements exist in the top bar.
	It shows the rank, health, breath and magic bars as well as the title
	(or portrait) and is clickable. If clicked, the associated crew member
	get's selected.
	HUD elements are passive, they don't update their status by themselves
	but rely on the HUD controller to be notified of any changes.

*/

#include Library_Bars

local crew, breathbar, magicbar, hotkey;
local energypos, magicpos;

public func BarSpacing() { return -4; }
public func HealthBarHeight() { return 14; }
public func BreathBarHeight() { return 8; }
public func MagicBarHeight() { return 14; }

/*
	usage of layers:
	-----------------
	layer 0 - unused
	layer 1 - title
	layer 2,3 - health bar
	layer 4,5 - breath bar
	layer 6,7 - magic bar
	
	layer 10 - rank
	layer 12 - hotkey
*/

/*
	The crew selector needs to be notified when
	-------------------
	...his clonk...
	+ changes his energy	->	UpdateHealthBar()
	+ changes his breath	->	UpdateBreathBar()
	+ changes his magic energy	->	UpdateMagicBar()
	+ (temporarily) changes his physical (energy, breath, magic energy)	-> see above
	+ gains a rank	->	UpdateRank()
	+ is selected/is deselected as cursor	->	UpdateSelectionStatus()
	+ changes it's title graphic (either by SetGraphics or by ChangeDef)	->	UpdateTitleGraphic()
	
*/

protected func Construction()
{
	_inherited();
	
	breathbar = false;
	magicbar = false;
	hotkey = false;
	energypos = 0;
	magicpos = 0;
	
	// parallaxity
	this["Parallaxity"] = [0,0];
	
	// visibility
	this["Visibility"] = VIS_None;
	
	// health bar
	SetBarLayers(2,0);
	SetBarOffset(0,BarOffset(0),0);
	SetBarDimensions(GetDefWidth(),HealthBarHeight(),0);
	SetClrModulation(RGB(200,0,0),3);
}

public func MouseSelection(int plr)
{
	if(!crew) return false;
	if(plr != GetOwner()) return false;
	if(!(crew->GetCrewEnabled())) return false;
	
	// stop previously selected crew
	StopSelected();
		
	// set cursor if not disabled etc.
	return SetCursor(plr, crew);
}

public func SetCrew(object c)
{
	crew = c;
	UpdateHealthBar();
	UpdateBreathBar();
	UpdateMagicBar();
	UpdateTitleGraphic();
	UpdateRank();
	UpdateController();
	UpdateSelectionStatus();
	UpdateName();
	
	this["Visibility"] = VIS_Owner;
}

public func SetHotkey(int num)
{
	if(num < 0 || num > 9)
	{
		SetGraphics(nil,nil,12);
		hotkey = false;
		return;
	}
	
	hotkey = true;
	var name = Format("%d",num);
	SetGraphics(name,Icon_Number,12,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(300,0,1000*GetDefWidth()/4,0,300,-1000*GetDefWidth()/4, 12);
	SetClrModulation(HSL(0,0,180),12);
}

public func CrewGone()
{
	RemoveObject();
}

private func UpdateTexts()
{
	CustomMessage("",this,crew->GetOwner());
	
	if(crew->GetEnergy() > 0)
		CustomMessage(Format("@<c dddd00>%d</c>",crew->GetEnergy()), this, crew->GetOwner(), energypos, GetDefHeight()/2 + BarOffset(0) + 14, nil, nil, nil, MSG_Multiple);
	
/*	if(crew->GetMagicEnergy() > 0)
		CustomMessage(Format("@<c 1188cc>%d</c>",crew->GetMagicEnergy()), this, crew->GetOwner(), magicpos, GetDefHeight()/2 + BarOffset(1) + 14, nil, nil, nil, MSG_Multiple);*/
	
	CustomMessage(Format("@%s",crew->GetName()), this, crew->GetOwner(), 0, GetDefHeight(), nil, nil, nil, MSG_Multiple);
}

public func UpdateController()
{
	if(!crew) return;
	// visibility
	SetOwner(crew->GetController());
	// name
	var fullname = Format("%s %s",crew->GetObjCoreRankName(),crew->GetName());
	SetName(Format("$TxtSelect$",fullname));
}

public func UpdateSelectionStatus()
{
	if(!crew) return;
	if(!hotkey) return;

	if(crew == GetCursor(crew->GetOwner()))
	{
		SetObjDrawTransform(1200,0,0,0,1200,0, 1);
	}
	else
	{
		SetObjDrawTransform(900,0,0,0,900,0, 1);
	}
}

public func UpdateRank()
{
	if(!crew) return;
	
	var rankx = -1000 * GetDefWidth()/2 + 10000;
	var ranky = -15000;
	
	SetGraphics(nil,0,10,GFXOV_MODE_Rank,0,0,crew);
	SetObjDrawTransform(1000,0,rankx,0,1000,ranky, 10);
}

public func UpdateTitleGraphic()
{
	if(!crew) return;
	
	//SetGraphics(nil,crew->GetID(),1,GFXOV_MODE_Object,nil,nil,crew);
	
	SetGraphics(nil,crew->GetID(),1,GFXOV_MODE_ObjectPicture, 0, 0, crew);
	
	// doesn't work:
	//SetColorDw(crew->GetColorDw());
}

public func UpdateHealthBar()
{
	if(!crew) return;
	var phys = crew->GetMaxEnergy();
	var promille;
	if(phys == 0) promille = 0;
	else promille = 1000 * crew->GetEnergy() / phys;

	energypos = -GetDefWidth()/2*(1000-promille)/1000;

	SetBarProgress(promille,0);
	UpdateTexts();
}

public func UpdateBreathBar()
{
	if(!crew) return;
	var phys = crew->GetMaxBreath();
	var promille;
	if(phys == 0) promille = 0;
	else promille = 1000 * crew->GetBreath() / phys;

	// remove breath bar if full breath
	if(promille == 1000)
	{
		if(breathbar)
			RemoveBreathBar();
	}
	// add breath bar if there is none
	else
	{
		if(!breathbar)
			AddBreathBar();
		
		SetBarProgress(promille,1);
	}

}

public func UpdateMagicBar()
{
	if(!crew) return;
	var phys = 0;
	var promille = 0;
	//if(phys != 0) promille = 1000 * crew->GetMagicEnergy() / (phys / 1000);

	magicpos = -GetDefWidth()/2*(1000-promille)/1000;
		
	// remove magic bar if no physical magic!
	if(phys == 0)
	{
		if(magicbar)
			RemoveMagicBar();
	}
	// add magic bar if there is none
	else
	{
		if(!magicbar)
			AddMagicBar();

		SetBarProgress(promille,2);
	}
	UpdateTexts();
}

private func UpdateName()
{
	UpdateTexts();
}

private func BarOffset(int num)
{
	var offset = GetDefHeight()/2 + HealthBarHeight()/2 + num * BarSpacing();
	if(num > 0) offset += HealthBarHeight();
	if(num > 1) offset += MagicBarHeight();
	return offset;
}

private func AddBreathBar()
{
	var num = 1;
	if(magicbar) num = 2;

	// breath bar
	SetBarLayers(4,1);
	SetBarOffset(0,BarOffset(num),1);
	SetBarDimensions(GetDefWidth(),BreathBarHeight(),1);
	SetClrModulation(RGB(0,200,200),5);
	
	breathbar = true;
}

private func RemoveBreathBar()
{
	RemoveBarLayers(4);

	breathbar = false;
	
	// update position of magic bar (if any)
	if(magicbar)
		SetBarOffset(0,BarOffset(1),2);
}

private func AddMagicBar()
{
	SetBarLayers(6,2);
	SetBarOffset(0,BarOffset(1),2);
	SetBarDimensions(GetDefWidth(),MagicBarHeight(),2);
	SetClrModulation(RGB(0,0,200),7);
	
	magicbar = true;
	
	// update position of breath bar (if any)
	if(breathbar)
		SetBarOffset(0,BarOffset(2),1);
}

private func RemoveMagicBar()
{
	RemoveBarLayers(6);

	magicbar = false;
}
