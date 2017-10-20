/**
	GUI.c
	This file contains functions that are used for layouting custom menus.

	@author Zapper
*/

/* -- constants used for layout -- */

static const GUI_AlignLeft = -1;
static const GUI_AlignCenter = 0;
static const GUI_AlignRight = +1;

static const GUI_AlignTop = -1;
static const GUI_AlignBottom = +1;

/*
 Prototype for the alternative layout:
 
 Propterties:
 - Align: proplist, defines the alignment via alignment constnats GUI_Align*
          - X: Horizontal alignment: Left, Center, Right
          - Y: Vertical alginment: Top, Center, Bottom
 - Margin: proplist, defines the margin of the GUI element
           - Left: Margin on the left side of the element
           - Right: Margin on the right side of the element
           - Top: Margin on top of the element
           - Bottom: Margin on the bottom of the element
           Note: The margin dimension is defined by the property "Dimension"
 - Width: int, width of the element
 - Height: int, height of the element
 - Dimension: function, defines the unit for the properties "Margin", "Width", and "Height";
              Should be one of: Global.ToPercentString (default), or Global.ToEmString 
 */
static const GUI_BoxLayout = new Global {
	Align = { X = GUI_AlignLeft, Y = GUI_AlignTop,},
	Margin = { Left = 0, Right = 0, Top = 0, Bottom = 0,},
	Width = 0,
	Height = 0,
	Dimension = Global.ToPercentString,
};

/*
 Prototype for grid layout:
 
 Properties:
 - Grid: proplist, as GUI_BoxLayout; additional properties:
         - Rows (default = 1): The number of rows in the grid
         - Columns (default = 1): The nnumber of columns in the grid
         Defines the layout for the box that contains the grid
 - Cell: proplist, as GUI_BoxLayout
         Defines the layout for individual cells in the grid
 
 */
static const GUI_GridCellLayout = new Global {
	Grid = { Prototype = GUI_BoxLayout, Rows = 1, Columns = 1},
	Cell = { Prototype = GUI_BoxLayout, },
};


/* -- menu functions -- */

// documented in /docs/sdk/script/fn
global func GuiAction_Call(proplist target, string function, value)
{
	return [GUI_Call, target, function, value];
}

// documented in /docs/sdk/script/fn
global func GuiAction_SetTag(string tag, int subwindow, object target)
{
	return [GUI_SetTag, tag, subwindow, target];
}

global func GuiAddCloseButton(proplist menu, proplist target, string callback, parameter)
{
	var close_button =
	{
		Tooltip = "$TooltipGUIClose$",
		Priority = 0x0fffff,
		Left = "100%-2em", Top = "0%+0em",
		Right = "100%", Bottom = "0%+2em",
		Symbol = GetDefaultCancelSymbol(),
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(target, callback, parameter)
	};
	GuiAddSubwindow(close_button, menu);
	return close_button;
}

global func GuiUpdateText(string text, int menu, int submenu, object target)
{
	var update = {Text = text};
	GuiUpdate(update, menu, submenu, target);
	return true;
}

// adds proplist /submenu/ as a new property to /menu/
global func GuiAddSubwindow(proplist submenu, proplist menu)
{
	do
	{
		// use an anonymous name starting with an underscore
		var uniqueID = Format("_child%d", RandomX(10000, 0xffffff));
		if (menu[uniqueID] != nil) continue;
		menu[uniqueID] = submenu;
		return true;
	} while (true);
}

// Converts an integer into a floating "em"-value, as the given value is divided by the given factor (10 by default).
global func ToEmString(int value, int factor)
{
	// Make sure factor is a power of ten.
	factor = factor ?? 10;
	var power_of_ten = 0;
	while (10**power_of_ten != factor)
	{
		if (10**power_of_ten > factor)
		{
			Log("WARNING: factor in ToEmString(%d, %d) is not a multiple of ten, falling back to default", value, factor);
			factor = 10;
			power_of_ten = 1;
			break;
		}
		power_of_ten++;	
	}
	// Construct the string using sign, value and decimal notation.
	var em_sign = "+";
	if (value < 0)
		em_sign = "-";
	var em_value = Format("%d", Abs(value / factor));
	var em_decimal = Format("%011dem", Abs(value % factor));
	em_decimal = TakeString(em_decimal, GetLength(em_decimal) - power_of_ten - 2);
	if (power_of_ten == 0)
		em_decimal = "0";
	return Format("%s%s.%s", em_sign, em_value, em_decimal);
}

// Converts an integer into a floating percent value, as the given value is divided by the given factor (10 by default).
global func ToPercentString(int value, int factor)
{
	// Make sure factor is a power of ten.
	factor = factor ?? 10;
	var power_of_ten = 0;
	while (10**power_of_ten != factor)
	{
		if (10**power_of_ten > factor)
		{
			Log("WARNING: factor in ToPercentString(%d, %d) is not a multiple of ten, falling back to default", value, factor);
			factor = 10;
			power_of_ten = 1;
			break;
		}
		power_of_ten++;	
	}
	// Construct the string using sign, value and decimal notation.
	var percent_sign = "+";
	if (value < 0)
		percent_sign = "-";
	var percent_value = Format("%d", Abs(value / factor));
	var percent_decimal = Format("%011d%%", Abs(value % factor));
	percent_decimal = TakeString(percent_decimal, GetLength(percent_decimal) - power_of_ten - 1);
	if (power_of_ten == 0)
		percent_decimal = "0";
	return Format("%s%s.%s", percent_sign, percent_value, percent_decimal);
}

/*
Returns true if /this/ object is allowed to be displayed on the same stack as the /other/ object in a GUI.
*/
global func CanBeStackedWith(object other)
{
	return this->GetID() == other->GetID();
}

// Returns the default symbol used for the "cancel" icon displayed e.g. in the top-right corner of menus.
global func GetDefaultCancelSymbol()
{
	return _inherited(...);
}

/* -- layout functions -- */

/*
 Checks a layout for errors, and throw a fatal error if the layout is wrong.
 
 @par layout A proplist with prototype GUI_BoxLayout.
             It is checked that:
             - Width is not 0
             - Height is not 0
             - Dimension is Global.ToPercentString or Global.ToEmString
 */
global func GuiCheckLayout(proplist layout)
{
	var errors = [];
	if (layout.Width == 0)
	{
		PushBack(errors, "property 'Width' must not be 0");
	}
	if (layout.Height == 0)
	{
		PushBack(errors, "property 'Height' must not be 0");
	}
	if (layout.Dimension != Global.ToEmString && layout.Dimension != Global.ToPercentString)
	{
		PushBack(errors, Format("property 'Dimension' must be Global.ToEmString, or Global.ToPerccentString, but it is %v", layout.Dimension));
	}
	
	if (GetLength(errors) > 0)
	{
		var message = "Error in layout";
		for (var error in errors)
		{
			message = Format("%s, %s", message, error);
		}
		FatalError(message);
	}
}


/*
 Calculates the position for a box element.
 
 This function returns a proplist with the properties: Left, Right, Top, Bottom.
 The proplist can be added to a GUI proplist in order to define the position
 of said GUI element - simply merge with AddProperties(element, position);
 
 @par layout A proplist with prototype GUI_BoxLayout;
 */
global func GuiCalculateBoxElementPosition(proplist layout)
{
	GuiCheckLayout(layout);

	var element_width = layout.Width + layout.Margin.Left + layout.Margin.Right;
	var element_height = layout.Height + layout.Margin.Top + layout.Margin.Bottom;

	// determine alignment on x axis
	var align_x;
	var offset_x;
	if (layout.Align.X == GUI_AlignLeft)
	{
		align_x = "0%";
		offset_x = 0;
	}
	else if (layout.Align.X == GUI_AlignCenter)
	{
		align_x = "50%";
		offset_x = -element_width / 2;
	}
	else if (layout.Align.X == GUI_AlignRight)
	{
		align_x = "100%";
		offset_x = -element_width;
	}

	// determine alignment on y axis
	var align_y;
	var offset_y;
	if (layout.Align.Y == GUI_AlignTop)
	{
		align_y = "0%";
		offset_y = 0;
	}
	else if (layout.Align.Y == GUI_AlignCenter)
	{
		align_y = "50%";
		offset_y = -element_height / 2;
	}
	else if (layout.Align.Y == GUI_AlignBottom)
	{
		align_y = "100%";
		offset_y = -element_height;
	}

	// determine actual dimensions

	var element_x = offset_x + layout.Margin.Left;
	var element_y = offset_y + layout.Margin.Top;

	return
	{
		Left =   Format("%s%s", align_x, Call(layout.Dimension, element_x)),
		Top =    Format("%s%s", align_y, Call(layout.Dimension, element_y)),
		Right =  Format("%s%s", align_x, Call(layout.Dimension, element_x + layout.Width)),
		Bottom = Format("%s%s", align_y, Call(layout.Dimension, element_y + layout.Height))
	};
}


/*
 Calculates the position for a box element inside a grid of elements.
 For example, the buttons in the inventory bar could be modeled as a grid
 with 1 row and x columns - this function calculates the position of a single
 button in that grid, based on the layout.
 
 This function returns a proplist with the properties: Left, Right, Top, Bottom.
 The proplist can be added to a GUI proplist in order to define the position
 of said GUI element - simply merge with AddProperties(element, position);
 
 @par layout A proplist with prototype GUI_GridCellLayout; Alternatively,
             you can use a the prototype GUI_BoxLayout (In that case, however,
             the margin and alignment are shared between cells and the grid,
             and you need two additional properties "Rows" and "Columns")
             
 @par row The vertical position of the element in the grid. First row is 0.
 @par column The horizontal position of the element in the grid. First column is 0.
 */
global func GuiCalculateGridElementPosition(proplist layout, int row, int column)
{
	// determine internal layout
	var grid_layout, cell_layout;
	
	if (layout["Grid"] && layout["Cell"])
	{
		grid_layout = layout["Grid"];
		cell_layout = layout["Cell"];
	}
	else
	{
		grid_layout = {Prototype = layout};
		cell_layout = {Prototype = layout};
		grid_layout.Rows = grid_layout.Rows;
		grid_layout.Columns = grid_layout.Columns;
	}

	// determine position of the cell in the grid
	var cell_width = cell_layout.Width + cell_layout.Margin.Left + cell_layout.Margin.Right;
	var cell_height = cell_layout.Height + cell_layout.Margin.Top + cell_layout.Margin.Bottom;

	var cell_pos_x = cell_layout.Margin.Left + column * cell_width;
	var cell_pos_y = cell_layout.Margin.Top + row * cell_height;

	// determine position of the grid
	var grid_width = cell_width * (grid_layout.Columns ?? 1);
	var grid_height = cell_height * (grid_layout.Rows ?? 1);

	var grid_position = GuiCalculateBoxElementPosition({Prototype = grid_layout, Width = grid_width, Height = grid_height});
	
	// merge everything into one
	return
	{
		Left =   Format("%s%s", grid_position.Left, Call(cell_layout.Dimension, cell_pos_x)),
		Top =    Format("%s%s", grid_position.Top, Call(cell_layout.Dimension, cell_pos_y)),
		Right =  Format("%s%s", grid_position.Left, Call(cell_layout.Dimension, cell_pos_x + cell_layout.Width)),
		Bottom = Format("%s%s", grid_position.Top, Call(cell_layout.Dimension, cell_pos_y + cell_layout.Height))
	};
}
