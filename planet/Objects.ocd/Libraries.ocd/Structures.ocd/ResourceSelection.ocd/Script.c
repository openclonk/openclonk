/**
	Resource selection

	Adds resource selection for to the interaction menu, so that the resource can be switched on or off.
	What switching the resource on or off does is up to the user.
	
	May be expanded to several resource selection types in the future, but currently it supports only
	one type of resource.

	@author Maikel, Marky
*/

 // Proplist that avoids clashes in variables.
local lib_resource_selection = nil;

static const RESOURCE_SELECT_Menu_Action_Resource_Enable = "material_on";
static const RESOURCE_SELECT_Menu_Action_Resource_Disable = "material_off";

/* --- Engine Callbacks --- */

func Construction(object by)
{
	lib_resource_selection = lib_resource_selection ?? {};
	lib_resource_selection.selected_resources = [];
	this->~InitResourceSelection();
	return _inherited(by, ...);
}


/*-- Resource selection interface --*/

/**
	Sets the selected resources to a specific value.
 */
public func SetResourceSelection(array resources)
{
	lib_resource_selection.selected_resources = resources[:];
	return;
}

/**
	Removes a resource from the selection.
 */
func RemoveFromResourceSelection(id resource)
{
	// Remove all child resources as well
	var type, index;
	while (type = GetDefinition(index++))
	{
		if (this->IsResourceSelectionParent(type, resource))
		{
			RemoveFromResourceSelection(type);
		}
	}
	return RemoveArrayValue(lib_resource_selection.selected_resources, resource);
}

/**
	Adds a resource to the selection.
 */
func AddToResourceSelection(id resource)
{
	// Add all child resources as well
	var type, index;
	while (type = GetDefinition(index++))
	{
		if (this->IsResourceSelectionParent(type, resource))
		{
			AddToResourceSelection(type);
		}
	}
	if (!IsValueInArray(lib_resource_selection.selected_resources, resource))
	{
		return PushBack(lib_resource_selection.selected_resources, resource);
	}
}

/**
	Find out whether a resource is in the selection.
 */
func IsInResourceSelection(id resource)
{
	return IsValueInArray(lib_resource_selection.selected_resources, resource);
}

/**
	Gets the current resource selection.
 */
func GetResourceSelection()
{
	return lib_resource_selection.selected_resources;
}

/**
	Builds the menu entries for the object interaction menu.
	How you build the entry is up to you.
	"OnResourceSelectionClick" can be used as the default "callback"
	value of a menu entry.
 */
public func GetResourceSelectionMenuEntries(object clonk)
{
	var menu_entries = [];
	// Add resources to the selection.
	var index = 0, resource;
	while (resource = GetDefinition(index++))
	{
		if (this->ShowResourceSelectionMenuEntry(resource)
		 && this->IsResourceSelectionParent(resource, nil)) // Ignore those with parent resource, because they will be added/removed together with the parent
		{
			var enabled = IsInResourceSelection(resource);
			var action;
			if (enabled)
			{
				action = RESOURCE_SELECT_Menu_Action_Resource_Disable;
			}
			else
			{
				action = RESOURCE_SELECT_Menu_Action_Resource_Enable;
			}
			var icon = this->GetResourceSelectionIcon(resource, enabled), icon_type, icon_gfx;
			if (GetType(icon) == C4V_C4Object || GetType(icon) == C4V_Def)
			{
				icon_type = icon;
			}
			else if (icon)
			{
				icon_type = icon.Symbol;
				icon_gfx = icon.GraphicsName;
			}
			PushBack(menu_entries, 
				{symbol = resource, extra_data = action, 
					custom =
					{
						Right = "2em", Bottom = "2em",
						BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
						Priority = index,
						image =  {Priority = 1, Symbol = resource},
						icon =   {Priority = 2, Right = "1em", Top = "1em", Symbol = icon_type, GraphicsName = icon_gfx},
				}}
			);
		}
	}
	return menu_entries;
}


/**
	Possible callback for a menu entry:
	* action RESOURCE_SELECT_Menu_Action_Material_Enable adds the symbol_or_object
	* action RESOURCE_SELECT_Menu_Action_Material_Disable removes the symbol_or_object
	from the GetResourceSelectionMenuEntries(), which are also updated.
 */
public func OnResourceSelectionClick(symbol_or_object, string action, bool alt)
{
	if (action == RESOURCE_SELECT_Menu_Action_Resource_Enable)
	{
		AddToResourceSelection(symbol_or_object);
	}
	else if (action == RESOURCE_SELECT_Menu_Action_Resource_Disable)
	{
		RemoveFromResourceSelection(symbol_or_object);
	}
	UpdateInteractionMenus(this.GetResourceSelectionMenuEntries);	
}

/* --- Overloadable by the user --- */

/**
	Identifies a resource as being linked to a parent. Child resources are added/removed
	together with their parents.

	Example:
		return child_resource->~GetParentLiquidType() == parent_resource;
 */
func IsResourceSelectionParent(id child_resource, id parent_resource)
{
	return false; // Overload
}

/**
	Identifies a resource as shown in the resource selection.

	Example:
		return resource->~IsLiquid() && resource != Library_Liquid;
 */
func ShowResourceSelectionMenuEntry(id resource)
{
	return false; // Overload
}


/**
	Sets up the default material selection. Overload to your likings.
 */
func InitResourceSelection()
{
	return; // Might contain something as:
/*
	// Add all liquids to the list of ones allowed to pump.
	var index = 0, resource;
	while (resource = GetDefinition(index++))
		if (ShowResourceSelectionMenuEntry(resource))
			AddToResourceSelection(resource);
	return;	
*/
}

/**
	Defines how the icons for selection are displayed,
	in the lower left corner.

	By default, the enabled icon is a green check mark,
	the disabled icon is a red cross mark.
	
	Return values can be either:
	- ID: for an icon
	- Object: for an object
	- Proplist: {Symbol, GraphicsName}, as in usual script GUIs
 */
func GetResourceSelectionIcon(id resource, bool enabled)
{
	if (enabled)
	{
		return Icon_Ok;
	}
	else
	{
		return Icon_Cancel;
	}
}
