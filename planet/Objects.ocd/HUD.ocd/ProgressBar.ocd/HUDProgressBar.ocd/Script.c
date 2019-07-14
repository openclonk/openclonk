/**
	HUDProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	height: height of the progress bar in 10 em (10 = 1em)
	text: text shown in the progress bar
	color: foreground color
	back_color: background color
	priority: sorting order in the HUD (lower is higher)
*/

local maximum, timeout_time;

// The manager object will abuse the "current" property to keep track of the bar count.
local current = 0;
// For the manager, this is the main window ID. For the other progress bars, this is the subwindow ID.
local window_id;
// For the manager, this keeps track of the height of the bars. For the others, this is their heigth (always 10em).
local total_height = 0;
// Whether this object is a player-object (only one per player).
local is_manager = false;
public func IsManagementObject() { return is_manager; }
// The others remember their manager.
local manager;

private func GetManagementObject()
{
	var other = FindObject(Find_ID(GetID()), Find_Exclude(this), Find_Owner(GetOwner()), Find_Func("IsManagementObject"));
	if (!other)
	{
		other = CreateObject(GetID(), nil, nil, GetOwner());
		other->MakeManager();
	}
	return other;
}

private func MakeManager()
{
	current = 1;
	total_height = 0;
	is_manager = true;
	this.Visibility = VIS_Owner;
}

public func Destruction()
{
	if (manager)
		manager->RemoveBar(window_id, total_height);
}

// The management object provides a progress bar template, which can be adjusted before adding the bar.
public func GetBarTemplate(int height, int priority, int back_color, int fore_color)
{
	return
	{
		Priority = priority,
		Bottom = ToEmString(height),
		Right = "100%",
		outer = 
		{
			Margin = "0.1em",
			BackgroundColor = back_color,
			inner = 
			{
				Priority = 1,
				Margin = "0.2em",
				BackgroundColor = fore_color,
				Right = "0%"
			},
			text = 
			{
				Priority = 2,
				Style = GUI_TextRight | GUI_TextVCenter,
				Text = nil,
			}
		}
	};
}

/* Called on the manager to add a new progress bar. Returns the bar's subwindow ID.
	The manager will force the ID and Target properties of the bars. */
public func AddBar(proplist bar, int height)
{
	total_height += height;
	bar.ID = ++current;
	bar.Target = this;
	
	// Do we have to open a completely new menu?
	if (!window_id)
	{
		// Make the position depend on the inventory bar.
		var max_left = GUI_Controller_InventoryBar->CalculateButtonPosition(0, 6);
		var max_right = GUI_Controller_InventoryBar->CalculateButtonPosition(5, 6);
		var layout = 
		{
			Style = GUI_IgnoreMouse | GUI_Multiple,
			Target = this,
			inner = 
			{
				Target = this, ID = 1,
				Left = max_left.Left, Right = max_right.Right,
				Top	 = Format("100%%%s", ToEmString(CalculateTop())),
				Style = GUI_VerticalLayout | GUI_NoCrop
			}
		};
		GuiAddSubwindow(bar, layout.inner);
		window_id = GuiOpen(layout);
	}
	else
	{
		GuiUpdate({_new_child = bar, Top = Format("100%%%s", ToEmString(CalculateTop()))}, window_id, 1, this);
	}
	return bar.ID;
}

/* Removes a bar that has been previously opened with AddBar (which returned subwindow_id). Height in 10em is required. */
public func RemoveBar(int subwindow_id, int height)
{
	total_height -= height;
	GuiClose(window_id, subwindow_id, this);
	GuiUpdate({Top = ToEmString(CalculateTop())}, window_id);
}

private func CalculateTop()
{
	return -total_height - (GUI_Controller_InventoryBar_IconMarginScreenTop + GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconSize/2);
}

// Called once on creation by CreateProgressBar.
public func Init(to, max, cur, timeout, offset, visibility, data)
{
	data.color = data.color ?? RGBa(255, 255, 255, 100);
	data.back_color = data.back_color ?? RGBa(0, 0, 0, 100);
	data.height = data.height ?? 25;
	data.priority = data.priority ?? 1;
	total_height = data.height; // keep track for deleting later
	
	maximum = max;
	current = cur;
	timeout_time = timeout;
	
	if (timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, BoundBy(timeout_time/2, 5, 35), this);
		e.t = timeout_time;
	}
	
	manager = GetManagementObject();
	var template = manager->GetBarTemplate(data.height, data.priority, data.back_color, data.color);
	if (current != 0)
		template.outer.inner.Right = Format("%d%%", 100 * current / maximum);
	if (data.text)
		template.outer.text.Text = data.text;
	window_id = manager->AddBar(template, data.height);
}


private func FxTimeOutTimer(target, effect, time)
{
	effect.t -= effect.Interval;
	if (effect.t > 0) return 1;
	Close();
	return -1;
}

// sets the value of the progress bar, updates the progress bar
public func SetValue(int to)
{
	var changed = current != to;
	
	current = BoundBy(to, 0, maximum);;
	var e = GetEffect("TimeOut", this);
	if (e)
		e.t = timeout_time;
	
	if (changed)
	{
		GuiUpdate({outer = { inner = {Right = Format("%d%%", 100 * current / maximum)}}}, manager.window_id, window_id, manager);
	}
}

// closes the progress bar and usually removes it
public func Close(){ return RemoveObject(); }

// changes the value of the progress bar by the specified amount, usually calls SetValue
public func DoValue(int change){ SetValue(current + change); }

// changes the offset {x = ?, y = ?}  of the progress bar relative to the attached object (or global)
public func SetOffset(proplist offset){ FatalError("Not implemented for global bars"); return nil;}

// makes the progress bar 100% parallax
public func SetParallax(){ FatalError("Not implemented for global bars"); return nil; }

// sets the Plane property of the progress bar
public func SetPlane(int to) { FatalError("Not implemented for global bars"); return nil; }

// makes the object a HUD element by setting parallaxity and the category C4D_StaticBack | C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax
public func MakeHUDElement() {return _inherited(...);}

local Name = "$Name$";
local Description = "$Description$";
