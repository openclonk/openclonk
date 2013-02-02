/**
	ObjectInfoDisplay
	Helps showing informations about objects.

	Provides a base object for other specific display dialogues.
	Can either be overwritten or manipulated externally. For an example see the structure library.
*/

static const HUD_OBJECTINFODISPLAY_LINESIZE = 27;

static const HUD_OBJECTINFODISPLAY_NAME = 1;
static const HUD_OBJECTINFODISPLAY_BAR = 2;
static const HUD_OBJECTINFODISPLAY_BORDER = 3;
static const HUD_OBJECTINFODISPLAY_CUSTOM = 4;
static const HUD_OBJECTINFODISPLAY_ITEMLIST = 5;
static const HUD_OBJECTINFODISPLAY_TEXT = 6;
static const HUD_OBJECTINFODISPLAY_STATUS = 7;

local Plane = 800;

local Name = "$Name$";
local Description = "$Description$";

local data, target;

local border, bars;
local item_list_overlay;

local height, width;

// used by the several functions during update
// is the offset from top in Y direction
local current_offset;

func SetTarget(object to)
{
	target = to;
	var e = AddEffect("BoundToTarget", target, 1, 0, this);
	e.source = this;
}

func FxBoundToTargetStop(target, effect, reason, temp)
{
	if (temp) return;
	if (effect.source)
		effect.source->Close();
}

func SetDisplayData(array to)
{
	data = to;
	ScheduleUpdate();
}

func AddLine(proplist what)
{
	PushBack(data, what);
	ScheduleUpdate();
}

func ScheduleUpdate()
{
	if (!GetEffect("Update", this))
		AddEffect("Update", this, 1, 1, this);
}

func FxUpdateTimer(target, effect, time)
{
	this->UpdateDisplay();
	return -1;
}

func StandardDisplay()
{
	SetDisplayData(
	[
		{type = HUD_OBJECTINFODISPLAY_TEXT, name = "Hello!", lines = 1},
		{type = HUD_OBJECTINFODISPLAY_CUSTOM, fn = this.Custom, lines = 0},
		{type = HUD_OBJECTINFODISPLAY_BORDER},
		{type = HUD_OBJECTINFODISPLAY_STATUS, name = "Text", priority = 3},
		{type = HUD_OBJECTINFODISPLAY_NAME, owner = true, lines = 1},
		{type = HUD_OBJECTINFODISPLAY_ITEMLIST, name = "MyItems", items = [Rock, Wood]},
	]
	);
}

// Dummy
func Custom()
{
	Log("Custom called!");
}

public func Construction()
{
	data = [];
	border = [];
	bars = [];
	item_list_overlay = 10;
	return true;
}

func UpdateDisplay()
{
	SetPosition(target->GetX(), target->GetY());
	
	var status_text = [];
	
	// ask data how many lines it needs
	var lines = 1;
	width = target->GetDefCoreVal("Width", "DefCore") * 3 / 2;
	height = target->GetDefCoreVal("Height", "DefCore") * 2;
	current_offset = 0;
	
	for (var i = GetLength(data); --i >= 0;)
	{
		var what = data[i];
		if (!what) { data[i] = {}; continue; } // failsafe
		
		if ((what.type == HUD_OBJECTINFODISPLAY_BAR) || (what.type == HUD_OBJECTINFODISPLAY_ITEMLIST))
			what.lines = what.lines ?? 2;
		
		if (what.lines) lines += what.lines;
		if (what.width)
			width = Max(width, what.width);
	}
		
	height += HUD_OBJECTINFODISPLAY_LINESIZE * lines;
	
	SetObjDrawTransform((width * 1000) / 64, 0, 0, 0, (height * 1000) / 64, -height/2, 0);
	SetGraphics(nil, target->GetID(), 1, GFXOV_MODE_Base, nil, GFX_BLIT_Wireframe | GFX_BLIT_Additive);
	
	SetClrModulation(RGBa(10, 5, 0, 200), 0);
	SetClrModulation(RGBa(255, 255, 255, 100), 1);
	
	if (!GetEffect("Flash", this))
		AddEffect("Flash", this, 1, 1, this);
	
	var message = "";
	var text_lines = [];
	for (var what in data)
	{
		if (what.type == HUD_OBJECTINFODISPLAY_CUSTOM)
		{
			if (what.fn)
				PushBack(text_lines, this->Call(what.fn));
		}
		else if (what.type == HUD_OBJECTINFODISPLAY_BORDER)
			UpdateBorder(what);
		else if (what.type == HUD_OBJECTINFODISPLAY_TEXT)
			PushBack(text_lines, what.name);
		else if (what.type == HUD_OBJECTINFODISPLAY_NAME)
			PushBack(text_lines, GenerateName(what));
		else if (what.type == HUD_OBJECTINFODISPLAY_BAR)
			PushBack(text_lines, UpdateBar(what));
		else if (what.type == HUD_OBJECTINFODISPLAY_ITEMLIST)
			PushBack(text_lines, UpdateItemList(what));
		else if(what.type == HUD_OBJECTINFODISPLAY_STATUS)
		{
			var prio = what.priority ?? 0;
			if (!status_text[prio])
				status_text[prio] = [];
			PushBack(status_text[prio], what.name);
		}
	}
	
	for (var tl in text_lines)
		if (tl)
			message = Format("%s%s|", message, tl);
	
	//CustomMessage("@asdkjasdasd|ENERGY||asd", this, GetOwner(), -60, 0, nil, nil, {Hgt = 120, Wdt = 100}, MSG_Top | MSG_Left | MSG_XRel | MSG_YRel | MSG_WidthRel)
	var msg_off_x = -width/2;
	var msg_off_y = -height/2;
	var msg_rect = {Hgt = height, Wdt = width};
	var msg_flags = MSG_XRel | MSG_YRel | MSG_Zoom;
	CustomMessage(Format("@%s", message), this, GetOwner(), msg_off_x, msg_off_y, nil, nil, msg_rect, msg_flags | MSG_Top | MSG_Left);
	
	// assemble status message
	var status_msg = "";
	for (var i = GetLength(status_text); --i >= 0;)
	{
		if (!status_text[i]) continue;
		
		var color = 0xffffff;
		if (i == 3) color = 0xff0000;
		else if (i == 2) color = 0xffff00;
		else if (i == 1) color = 0x00ff00;
		
		for (var msg in status_text[i])
		{
			status_msg = Format("|<c %x>%s</c>%s", color, msg, status_msg);
		}
	}
	if (GetLength(status_msg) > 0)
		CustomMessage(Format("@%s", status_msg), this, GetOwner(), msg_off_x, msg_off_y, nil, nil, msg_rect, msg_flags | MSG_Bottom | MSG_Right | MSG_Multiple);
	
}

func GenerateName(proplist data)
{
	var name = target->GetName();
	var owner = "";
	if (data.owner)
	{
		owner = GetPlayerName(target->GetOwner()) ?? "$NotOwned$";

		if (GetLength(owner) > 10)
		{
			var nowner = "";
			for (var i = 0; i < 10; ++i)
				nowner = Format("%s%c", nowner, GetChar(owner, i));
			owner = Format("%s..", nowner);
		}
		name = Format("%s (%s)", name, owner);
	}
	name = Format("<c %x>%s</c>", GetPlayerColor(target->GetOwner()), name);
	current_offset += HUD_OBJECTINFODISPLAY_LINESIZE;
	return name;
}

func UpdateBorder(proplist data)
{
	if (GetType(border) != C4V_Array)
		border = [];
	for (var obj in border)
		obj->RemoveObject();
	
	// create four end point
	var w = target->GetDefCoreVal("Width", "DefCore");
	var h = target->GetDefCoreVal("Height", "DefCore");
	var x = [-w/2, w/2, w/2, -w/2];
	var y = [-h/2, -h/2, h/2, h/2];
	
	for (var i = 0; i < 4; ++i)
	{
		var obj = CreateObject(Dummy, 0, 0, GetOwner());
		obj.Visibility = VIS_Owner;
		
		obj->SetPosition(target->GetX() + x[i], target->GetY() + y[i]);
		obj->SetGraphics(nil, Icon_Cancel, 1, GFXOV_MODE_IngamePicture, nil, GFX_BLIT_Custom);
		obj.Plane = this.Plane + 1;
		border[i] = obj;
	}
}

func UpdateBar(proplist data)
{
	if (!data.index) data.index = GetLength(bars);
	else bars[data.index]->Close();
	
	var properties = {width = (width*3)/4, height = HUD_OBJECTINFODISPLAY_LINESIZE - 2, color = data.color };
	var offset = {x = width/2 - properties.width/2, y = -height/2 + current_offset};
	var name = data.name;
	if (data.lines > 1)
	{
		offset.y += HUD_OBJECTINFODISPLAY_LINESIZE;
		name = Format("%s|", name);
	}
	bars[data.index] = CreateProgressBar(GUI_ShadedSimpleProgressBar, target->Call(data.max), target->Call(data.cur), 0, GetOwner(), offset, VIS_Owner, properties);
	bars[data.index]->SetPlane(this.Plane + 1);
	
	current_offset += data.lines * HUD_OBJECTINFODISPLAY_LINESIZE;
	
	if (!GetEffect("UpdateBars", this))
		AddEffect("UpdateBars", this, 1, 30, this);
	
	
	return name;
}

func UpdateItemList(proplist data)
{
	// clear overlays owned by data
	if (data.overlays)
		for (var overlay in data.overlays)
		{
			SetGraphics(nil, nil, overlay, GFXOV_MODE_Base, nil, GFX_BLIT_Custom);
		}
	else
	{
		data.overlays = [];
	}
	
	var name = data.name;
	if (data.lines > 1)
	{
		current_offset += 1 * HUD_OBJECTINFODISPLAY_LINESIZE;
		name = Format("%s|", name);
	}
	
	// for every existing item, get overlay and draw
	var itemcount = GetLength(data.items);
	if (!itemcount) return;
	var left = width/4;
	var space = width - left;
	var distance = BoundBy(space / itemcount, 5, 20);
	var std_size = 64;
	
	var offset = 0;
	var size = (HUD_OBJECTINFODISPLAY_LINESIZE * 1000) / std_size;
	var std_offset_x = 1000 * (width/2 - 16);
	var std_offset_y = 1000 * (current_offset - height/2) - size/2;
	
	for (var i = GetLength(data.items); --i >= 0; offset += distance)
	{
		if ((GetLength(data.overlays) < i) || (!data.overlays[i]))
			data.overlays[i] = ++item_list_overlay;
			
		var overlay = data.overlays[i];
		SetGraphics(nil, data.items[i], overlay, GFXOV_MODE_IngamePicture, nil, GFX_BLIT_Custom);
		SetObjDrawTransform(size, 0, std_offset_x - 1000 * offset, 0, size, std_offset_y, overlay);
	}
	
	current_offset += 1 * HUD_OBJECTINFODISPLAY_LINESIZE;
	
	return name;
}

func Close()
{
	for (var obj in bars) obj->Close();
	for (var obj in border) obj->RemoveObject();
	
	RemoveObject();
}

func FxUpdateBarsTimer()
{
	for (var what in data)
	{
		if (what.type != HUD_OBJECTINFODISPLAY_BAR) continue;
		var bar = bars[what.index];
		if (!bar) continue;
		
		bar->SetValue(target->Call(what.cur));
	}
}

func FxFlashStart(target, effect, temp)
{
	if (temp) return;
	effect.flash = CreateObject(Dummy, 0, 4, GetOwner());
	effect.flash.Visibility = this.Visibility;
	effect.flash.Plane = this.Plane + 10;
	effect.flash->SetGraphics(nil, GetID(), 1, GFXOV_MODE_Base, nil, GFX_BLIT_Custom);
	effect.flash->SetObjDrawTransform((width * 1000) / 64, 0, 0, 0, (height * 1000) / 64, -height/2, 1);
}

func FxFlashTimer(target, effect, time)
{
	var t = time * 20;
	if (t > 255) return -1;
	effect.flash->SetClrModulation(RGBa(255, 255, 255, 255 - t), 1);
	return 1;
}

func FxFlashStop(target, effect, reason, temp)
{
	if (temp) return;
	if (effect.flash) effect.flash->RemoveObject();
}