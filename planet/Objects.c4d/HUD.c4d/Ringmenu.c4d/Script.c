/*
		The Ringmenu
		Made by Mimmo
*/



local command_object; //at which selectes will be sent
local menu_icons;     //array for the icons
local menu_object;    //the clonk which the menu is for
local shown;          //am i visible?


func Construction()
{
	menu_icons=[];
	command_object=nil;
	shown=false;
	// parallaxity
	this["Parallaxity"] = [0,0];
}



// rinmenu certain position for the calling object
global func CreateRingMenu(id symbol, int x, int y, object commander)
{
	if(!(this->GetOCF() & OCF_CrewMember)) return nil;
	if(!(this->~HasMenuControl())) return nil;
	var menu = CreateObject(GUI_RingMenu,0,0,this->GetOwner());
	menu->SetPosition(x,y);
	menu->SetMenu(this,commander);
	menu->SetMenuIcon(symbol);
	menu->Hide();
	return menu;	
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
		SetObjDrawTransform(750,0,0,0,750,0,1);
		SetObjDrawTransform(750,0,0,0,750,0,0);
	}
}

//adds an item, icon, amount, extra
public func AddItem(new_item, int amount, extra) 
{ 
	var index=GetLength(menu_icons);
	menu_icons[index]=CreateObject(GUI_RingMenu_Icon,0,0,menu_object->GetOwner());
	menu_icons[index]->SetSymbol(new_item);
	menu_icons[index]->SetExtraData(extra);	
	menu_icons[index]->SetHotkey(index+1);
	if(amount==nil)
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
	if(!item_count) item_count = 1;
	var segment=360/item_count;
	var dvar=0;
	var angle = Angle(0,0,dx,dy);
	
	for(var i=0; i<=item_count ; i++)
	{ 
		if(i==item_count) var miss=360-(segment*item_count);
		if(angle>=(segment*i) && angle<=((segment*(i+1)+miss))) dvar=i+1;
	}
	if(dvar==item_count+1) dvar=item_count;

	if(command_object->Selected(this,menu_icons[dvar-1],alt)) Close();
	return 1;
}


//...
public func SelectHotkey(int key, bool alt)
{	
	if(command_object->Selected(this,menu_icons[key],alt)) Close();
	return 1;
	
}


//am i visible?
func IsVisible() { return shown; }

//makes me visible/updates me
func Show() 
{
	var item_count=GetLength(menu_icons); 
	if(!item_count) item_count = 1;
	var segment=360/item_count;
	
	var x=GetX();
	var y=GetY();
	for(var i=0; i<(GetLength(menu_icons)); i++) 
	{	
		if(menu_icons[i])
		{
			var angle=(i*segment)+(segment/2);
			if(GetLength(menu_icons)==1) angle=90;
			menu_icons[i]->SetPosition(x+Sin(angle,100),y-Cos(angle,100));
			menu_icons[i]["Visibility"] = VIS_Owner;
			menu_icons[i]->	SetSize(((620000)/Max(item_count,12))/64);
							//620000~=u*1000;  u=r*pi*2; r=100px;
							//620000 is almost the circumference of the ringmenu with 200px radius
							//this divided by the itemcount (or 12, if too less items,
							//it guarantees no oversized icons)
							//and then divided by the diameter of the icons, 64 pixels
							//gives the value, with that the icons can be stretched/compressed (1000 standard)
		}
	}
	this["Visibility"] = VIS_Owner;

	shown=true;
}

public func UpdateCursor(int dx, int dy)
{	
	var angle = Angle(0,0,dx,dy);
	var item_count=GetLength(menu_icons); 
	if(!item_count) item_count = 1;
	var segment=360/item_count;
	var dvar=0;
	for(var i=0; i<=item_count ; i++)
	{ 
		if(menu_icons[i])
		{
				menu_icons[i]->	SetSize(((620000)/Max(item_count,12))/64); //see Show()
		}
		if(i==item_count) var miss=360-(segment*item_count);
		if(angle>=(segment*i) && angle<=((segment*(i+1)+miss))) dvar=i+1;
	}
	if(dvar==item_count+1) dvar=item_count;
	if(menu_icons[dvar-1])
	{
			menu_icons[dvar-1]->	SetSize(((620000)/Max(item_count,12))/64 *3/2); //see Show()
	}

}

public func Hide() {
	for(var i=0; i<GetLength(menu_icons); i++)if(menu_icons[i]) menu_icons[i]["Visibility"] = VIS_None;
	this["Visibility"] = VIS_None;
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