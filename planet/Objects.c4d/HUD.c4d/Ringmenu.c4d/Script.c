/*
		The Ringmenu
		Authors: Mimmo, Newton
*/



local command_object; //at which selectes will be sent
local menu_icons;     //array for the icons
local menu_object;    //the clonk which the menu is for
local shown;          //am i visible?

static const GUI_Ringmenu_Radius = 100;

static const GUI_Ringmenu_Ring = 70;

func Construction()
{
	menu_icons=[];
	command_object=nil;
	shown=false;
	// parallaxity
	this["Parallaxity"] = [0,0];
}

// rinmenu certain position for the calling object
global func CreateRingMenu(id symbol, object commander)
{
	if(!(this->GetOCF() & OCF_CrewMember)) return nil;
	if(!(this->~HasMenuControl())) return nil;
	var menu = CreateObject(GUI_RingMenu,0,0,this->GetOwner());
	
	// minimum padding:
	var paddingy = GUI_Ringmenu_Radius + 175;
	var paddingx = GUI_Ringmenu_Radius + 50;
	
	// use x/y coordinates from last known cursor pos
	var plr_cursor_pos = GetPlayerCursorPos(GetOwner());
	var x, y;
	if (plr_cursor_pos)
	{
		x = plr_cursor_pos[0];
		y = plr_cursor_pos[1];
	}
	else
	{
		// Cursor pos unknown? This can't really happen
		x = y = 300;
	}
	
	menu->SetPosition(Max(paddingx,x),Max(paddingy,y));
	menu->SetMenu(this,commander);
	menu->SetMenuIcon(symbol);
	menu->Hide();
	return menu;	
}


public func GetMenuObject()
{
	return menu_object;
}

//re-set clonk and commandobject
public func SetMenu(object menuobject, object commandobject)
{
	if(menuobject->GetOCF() & OCF_CrewMember)
		menu_object=menuobject;	
	command_object=commandobject;
	menuobject->~SetMenu(this);

	
}	

//re-set icon
func SetMenuIcon(id symbol)
{
	this["Visibility"] = VIS_Owner;		
	if(!symbol) 
	{	
		SetGraphics(nil,nil,0);
		SetGraphics(nil,nil,1);
	}
	else
	{
		SetGraphics(nil,symbol,1,4);
		SetObjDrawTransform(2000,0,0,0,2000,0,1);
		SetObjDrawTransform(2000,0,0,0,2000,0,0);
	}
}

//adds an item, icon, amount, extra (the item can be an object too)
public func AddItem(new_item, int amount, extra) 
{ 
	var index = GetLength(menu_icons);
	menu_icons[index] = CreateObject(GUI_RingMenu_Icon,0,0,menu_object->GetOwner());
	menu_icons[index]->SetSymbol(new_item);
	menu_icons[index]->SetExtraData(extra);	
	if(amount == nil)
	{
		menu_icons[index]->SetAmount(1);
	}
	else 
	{
		menu_icons[index]->SetAmount(amount);
	}
	menu_icons[index]["Visibility"] = VIS_None;
	return index;
}

//selects by dx,dy and alt=alternative selection
public func Select(int dx, int dy, bool alt)
{
	var item_count=GetLength(menu_icons); 
	if(!item_count)
		if(command_object->Selected(this,nil,alt))
			Close();
	
	var distance = Sqrt(dx*dx+dy*dy);
	
	var segment=360/item_count;
	if(!segment) segment = 1;
	var angle = Angle(0,0,dx,dy);
	var item = BoundBy(angle/segment,0,item_count-1);
	
	// center selected: close
	if(Abs(distance - GUI_Ringmenu_Radius) >= GUI_Ringmenu_Ring) Close();
	// otherwise, select something
	else if(command_object->Selected(this,menu_icons[item],alt))
		Close();
}

//am i visible?
func IsVisible() { return shown; }

//makes me visible/updates me
func Show() 
{
	var item_count = GetLength(menu_icons); 
	if(!item_count) return;
	
	var segment=360/item_count;
	
	var x = GetX();
	var y = GetY();
	for(var i=0; i<item_count; i++) 
	{	
		if(menu_icons[i])
		{
			var angle=(i*segment)+(segment/2);
			if(item_count == 1)
				menu_icons[i]->SetPosition(x,y);
			else
				menu_icons[i]->SetPosition(x+Sin(angle,GUI_Ringmenu_Radius),y-Cos(angle,GUI_Ringmenu_Radius));
			menu_icons[i]["Visibility"] = VIS_Owner;
			//420000~=u*1000*2/3;  u=r*pi*2; r=100px;
			// Size will never get bigger as if there were 5 items shown
			menu_icons[i]->SetSize(420000/Max(item_count,5));
		}
	}
	this["Visibility"] = VIS_Owner;

	shown = true;
}

public func UpdateCursor(int dx, int dy)
{	
	if(shown)
	{
		var angle = Angle(0,0,dx,dy);
		var item_count = GetLength(menu_icons); 
		if(!item_count) return;
		
		var distance = Sqrt(dx*dx+dy*dy);
		var outside = (Abs(distance - GUI_Ringmenu_Radius) >= GUI_Ringmenu_Ring);
		if(outside)
		{
			CustomMessage("",this,menu_object->GetOwner(),0,64,RGB(255,0,0));
			SetGraphics("Close",GetID(),2,GFXOV_MODE_Base);
			SetObjDrawTransform(1000,0,16000,0,1000,16000,2);
		}
		else
			SetGraphics(nil,nil,2);
		
		var segment = 360 / item_count;
		if(!segment) segment = 1;
		var item = BoundBy(angle/segment,0,item_count-1);
		if(outside) item = -1;
		
		for(var i=0; i<= item_count; i++)
		{ 
			if(menu_icons[i])
			{
					// calculate distance to angle
					var dist = Normalize(angle - (segment*i + segment/2),-180);
					dist = BoundBy(dist*240/segment,-180,180);
					dist = (dist**3)/(180**2);
					var siz = (Cos(dist,1000)+1000)/2; // 0..1000
					if(outside) siz = Max(0,siz-Abs(distance - GUI_Ringmenu_Radius)*10);
					menu_icons[i]->SetSize((siz+2000)*420000/Max(item_count,5)/2000); //see Show()
					if(i == item)
						CustomMessage(Format("%s",menu_icons[i]->GetName()),this,menu_object->GetOwner(),0,64,RGB(255,0,0));
			}
		}
	}
}

public func Hide() {
	for(var i=0; i<GetLength(menu_icons); i++)
		if(menu_icons[i])
			menu_icons[i]["Visibility"] = VIS_None;
	this["Visibility"] = VIS_None;
	CustomMessage("",this,menu_object->GetOwner());
	shown=false;
}

//closes (removes) the menu
func Close()
{
	for(var i=0; i<GetLength(menu_icons); i++)
		if(menu_icons[i])
			menu_icons[i]->RemoveObject();
	
	if(menu_object)
		menu_object->SetMenu(nil);
	
	if(command_object)
		command_object->~MenuClosed(this);

	RemoveObject(); 
}



func Definition(def) {
	SetProperty("Name", "$Name$", def);
}