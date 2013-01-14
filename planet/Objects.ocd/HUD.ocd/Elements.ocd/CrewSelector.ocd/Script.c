/**
	HUD	Crew Selector
	For each crew member, one of these HUD elements exist in the top bar.
	It shows the rank, health, breath and magic bars as well as the title
	(or portrait) and is clickable. If clicked, the associated crew member
	get's selected.
	HUD elements are passive, they don't update their status by themselves
	but rely on the HUD controller to be notified of any changes.
	
	@authors Newton
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
	layer 2 - title
	layer 1,3 - background-effects
	layer 4,5 - health bar
	layer 6,7 - breath bar
	layer 8,9 - magic bar
	
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

static const GUI_CS_Base		= 0;
static const GUI_CS_Title		= 2;
static const GUI_CS_BreathBG	= 1;
static const GUI_CS_HealthBG	= 3;
static const GUI_CS_HealthBar	= 4;
static const GUI_CS_HealthText	= 5;
static const GUI_CS_BreathBar	= 6;
static const GUI_CS_BreathText	= 7;
static const GUI_CS_SpecialBar	= 8;
static const GUI_CS_SpecialText	= 9;
static const GUI_CS_Rank		= 10;
static const GUI_CS_Hotkey		= 12;

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
	SetBarLayers(GUI_CS_HealthBar,0);
	SetBarOffset(0,BarOffset(0),0);
	SetBarDimensions(GetDefWidth(),HealthBarHeight(),0);
	SetClrModulation(RGB(200,0,0),GUI_CS_HealthText);
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
	// reset old crew, if there
	if(crew)
		RemoveEffect("GUIHealthMonitor", crew);
	
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
	
	ScheduleCall(this, "AddMonitorEffect", 1);
}

private func AddMonitorEffect()
{
	if(!crew)
		return;
	
	AddEffect("GUIHealthMonitor", crew, 50, 0, this);
}

public func SetHotkey(int num)
{
	if(num < 0 || num > 9)
	{
		SetGraphics(nil,nil,GUI_CS_Hotkey);
		hotkey = false;
		return;
	}
	
	var kx = 1000 * GetDefWidth()/2 - 10000;
	var ky = -1000 * GetDefWidth()/2 + 10000;
	
	hotkey = true;
	var name = Format("%d",num);
	SetGraphics(name,Icon_Number,GUI_CS_Hotkey,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(300,0,kx,0,300,ky, GUI_CS_Hotkey);
	SetClrModulation(HSL(0,0,180),GUI_CS_Hotkey);
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
	SetName(crew->GetName());
	this.Tooltip = Format("$TxtSelect$",fullname);
}

public func UpdateSelectionStatus()
{
	if(!crew) return;
	if(!hotkey) return;

	if(crew == GetCursor(crew->GetOwner()))
	{
		SetObjDrawTransform(1100,0,0,0,1100,0, GUI_CS_Title);
	}
	else
	{
		SetObjDrawTransform(800,0,0,0,800,0, GUI_CS_Title);
	}
}

public func UpdateRank()
{
	if(!crew) return;
	
	var rankx = -1000 * GetDefWidth()/2 + 12000;
	var ranky = -13000;
	
	SetGraphics(nil,0,GUI_CS_Rank,GFXOV_MODE_Rank,nil,0,crew);
	SetObjDrawTransform(1000,0,rankx,0,1000,ranky, GUI_CS_Rank);
}

public func UpdateTitleGraphic()
{
	if(!crew) return;
	
	//SetGraphics(nil,crew->GetID(),1,GFXOV_MODE_Object,nil,nil,crew);
	
	SetGraphics(nil,crew->GetID(),GUI_CS_Title,GFXOV_MODE_ObjectPicture, nil, 0, crew);
	
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
		var clr = GetAverageTextureColor(crew->GetTexture(0,0));
		
		SetClrModulation(clr, GUI_CS_BreathBG);
		//SetClrModulation(255-(1000-promille)*76/1000), GUI_CS_BreathBG);
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
	SetBarLayers(GUI_CS_BreathBar,1);
	SetBarOffset(0,BarOffset(num),1);
	SetBarDimensions(GetDefWidth(),BreathBarHeight(),1);
	SetClrModulation(RGB(0,200,200),GUI_CS_BreathText);
	
	breathbar = true;
	
	// also enable background-coloring
	SetGraphics("Background", GUI_CrewSelector, GUI_CS_BreathBG, GFXOV_MODE_Base);
}

private func RemoveBreathBar()
{
	RemoveBarLayers(GUI_CS_BreathBar);
	SetGraphics(nil, nil, GUI_CS_BreathBG);

	breathbar = false;
	
	// update position of magic bar (if any)
	if(magicbar)
		SetBarOffset(0,BarOffset(1),2);
}

private func AddMagicBar()
{
	SetBarLayers(GUI_CS_SpecialBar,2);
	SetBarOffset(0,BarOffset(1),2);
	SetBarDimensions(GetDefWidth(),MagicBarHeight(),2);
	SetClrModulation(RGB(0,0,200),GUI_CS_SpecialText);
	
	magicbar = true;
	
	// update position of breath bar (if any)
	if(breathbar)
		SetBarOffset(0,BarOffset(2),1);
}

private func RemoveMagicBar()
{
	RemoveBarLayers(GUI_CS_SpecialBar);

	magicbar = false;
}


// highlight
public func OnMouseOver(int plr)
{
	if(GetOwner() != plr)
		return nil;
	
	SetGraphics("Focussed", GUI_CrewSelector);
}

public func OnMouseOut(int plr)
{
	if(GetOwner() != plr)
		return nil;
	
	SetGraphics(nil, nil);
}

public func FxGUIHealthMonitorDamage(object target, proplist effect, int damage, int cause)
{

	var change = Abs(damage)/(target->GetMaxEnergy()*10);
	// for really small changes, like fire or higher precision DoEnergy
	if(change == 0)
		change = 3;
	
	//if(!GetEffect("GUIHealthMonitorWorker", this))
	if(!effect.worker)
	{
		effect.worker = AddEffect("GUIHealthMonitorWorker", target, 150, 4, this);
		effect.worker.intensity = change+20;
	}
	else
		effect.worker.intensity += change;
	
	effect.worker.intensity = BoundBy(effect.worker.intensity, 0, 80);
	
	// heal
	if(damage > 0)
		effect.worker.type = 1;
	// damage
	else
		effect.worker.type = 0;

	// fire
	if(cause == FX_Call_DmgFire || cause == FX_Call_EngFire)
		effect.worker.type = 2;
	

	return damage;
}

public func FxGUIHealthMonitorWorkerStart(object target, proplist effect, bool tmp)
{
	if(tmp) return;
	
	SetGraphics("Hurt", GUI_CrewSelector, GUI_CS_HealthBG, GFXOV_MODE_Base, nil, GFX_BLIT_Additive);
	EffectCall(target, effect, "Timer");
}

public func FxGUIHealthMonitorWorkerTimer(object target, proplist effect, int time)
{
	// do graphical effect
	var a = BoundBy(effect.intensity*7, 20, 255);
	// damage
	if(effect.type == 0)
	{
		var r = Min(200 + effect.intensity, 255);
		SetClrModulation(RGBa(r,0,0,a), GUI_CS_HealthBG);
	}
	// heal
	else if(effect.type == 1)
	{
		var g = Min(100 + effect.intensity, 255);
		a = Min(a+100, 255);
		SetClrModulation(RGBa(50,g,50,a), GUI_CS_HealthBG);
	}
	// fire!
	else if(effect.type == 2)
	{
		var r = Min(150 + effect.intensity, 255);
		var g = Min(effect.intensity, 100);
		a = Min(a+100, 255);
		SetClrModulation(RGBa(r,g,0,a), GUI_CS_HealthBG);
	}
	// not set yet? might happen at the start sometimes. we just hide in that case.
	else
		SetClrModulation(RGBa(0,0,0,0), GUI_CS_HealthBG);
	
	var dec = 3;
	
	// fade faster if huge numbers
	if(effect.intensity > 60)
		dec += 2;
	
	effect.intensity = effect.intensity-dec;
	
	// we're done, remove effect
	if(effect.intensity <= 0)
		return -1;
}

public func FxGUIHealthMonitorWorkerStop(object target, proplist effect, int reason, bool tmp)
{
	if(tmp) return;
	
	SetGraphics(nil, nil, GUI_CS_HealthBG);
}
