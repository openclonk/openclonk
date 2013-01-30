/**
	ObjectInfoDisplay
	Helps showing informations about objects.

	Provides a base object for other specific display dialogues.
	Can either be overwritten or manipulated externally. For an example see the structure library.
*/

static const HUD_OBJECTINFODISPLAY_LINESIZE = 8;

static const HUD_OBJECTINFODISPLAY_NAME = 1;
static const HUD_OBJECTINFODISPLAY_BAR = 2;
static const HUD_OBJECTINFODISPLAY_BORDER = 3;
static const HUD_OBJECTINFODISPLAY_CUSTOM = 4;

local Plane = 800;

local Name = "$Name$";
local Description = "$Description$";

local data, target;

local border, bars;

local height, width;

// used by the several functions during update
// is the offset from top in Y direction
local current_offset;

func SetTarget(object to)
{
	target = to;
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
		{type = HUD_OBJECTINFODISPLAY_CUSTOM, fn = this.Custom, lines = 0},
		{type = HUD_OBJECTINFODISPLAY_BORDER},
		{type = HUD_OBJECTINFODISPLAY_NAME, owner = true, lines = 1},
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
	return true;
}

func UpdateDisplay()
{
	SetPosition(target->GetX(), target->GetY());
	// ask data how many lines it needs
	var lines = 1;
	width = target->GetDefCoreVal("Width", "DefCore") * 3 / 2;
	height = target->GetDefCoreVal("Height", "DefCore") * 2;
	current_offset = 0;
	
	for (var i = GetLength(data); --i >= 0;)
	{
		var what = data[i];
		if (!what) { data[i] = {}; continue; } // failsafe
		
		if (what.lines) lines += what.lines;
		if (what.width)
			width = Max(width, what.width);
	}
		
	height += HUD_OBJECTINFODISPLAY_LINESIZE * lines;
	
	SetObjDrawTransform((width * 1000) / 64, 0, 0, 0, -height/2, (height * 1000) / 64, 0);
	SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, GFX_BLIT_Wireframe | GFX_BLIT_Additive, target);
	
	SetClrModulation(RGBa(100,50, 10, 50), 0);
	SetClrModulation(RGBa(255, 255, 255, 100), 1);
	
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
		else if (what.type == HUD_OBJECTINFODISPLAY_NAME)
			PushBack(text_lines, GenerateName(what));
		else if (what.type == HUD_OBJECTINFODISPLAY_BAR)
			PushBack(text_lines, UpdateBar(what));
	}
	
	for (var tl in text_lines)
		if (tl)
			message = Format("%s|", message);
	
	CustomMessage(message, this, GetOwner(), 0, -height/2, nil, nil, nil, MSG_Left | MSG_NoLinebreak);
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
		name = Format("%s %s", name, owner);
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
	
	var properties = {width = width/2, height = HUD_OBJECTINFODISPLAY_LINESIZE - 2};
	var offset = {x = width/2 + properties.width/2, y = height/2 + current_offset};
	
	
	bars[data.index] = CreateProgressBar(GUI_ShadedSimpleProgressBar, target->Call(data.max), target->Call(data.cur), 0, GetOwner(), offset, VIS_Owner, properties);
	
	current_offset += HUD_OBJECTINFODISPLAY_LINESIZE;
	
	if (!GetEffect("UpdateBars", this))
		AddEffect("UpdateBars", this, 1, 30, this);
	
	return data.name;
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