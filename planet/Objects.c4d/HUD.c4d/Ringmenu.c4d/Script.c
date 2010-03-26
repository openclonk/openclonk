/*
		The Ringmenu
		Made by Mimmo
*/



local command_object; //at which selectes will be sent
local menu_icons;	//array for the icons
local menu_object;	//the clonk which the menu is for
local shown;	//am i visible?


func Construction()
{
        menu_icons=[];
        command_object=nil;
        shown=false;
}



// rinmenu certain position for the calling object
global func CreateRingMenu(id symbol, int x, int y, object commander)
{
	if(!(this->GetOCF() & OCF_CrewMember)) return nil;
	if(!(this->~HasMenuControl())) return nil;
	var menu=CreateObject(GUI_RingMenu,x,y+16,this->GetOwner());
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
	}
}

//adds an item, icon, amount, extra
public func AddItem(id new_item, int amount, extra) 
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

//selects by angle alt=alternative selection
public func Select(int angle, bool alt)
{
	var item_count=GetLength(menu_icons); 
	if(!item_count) item_count = 1;
	var segment=360/item_count;
	var dvar=0;
    
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
		var stat=false;
		if(item_count<=10) stat=true;
        for(var i=0; i<(GetLength(menu_icons)); i++) 
        {	
        		if(menu_icons[i])
        		{
        			var angle=(i*segment)+(segment/2);
        			if(GetLength(menu_icons)==1) angle=90;
        			menu_icons[i]->SetPosition(x+Sin(angle,100),y-Cos(angle,100));
        			menu_icons[i]["Visibility"] = VIS_Owner;
        			if(!stat) 	menu_icons[i]->SetSize(((620000)/item_count)/32);
        			else 		menu_icons[i]->SetSize(1950);
        		}
        }
 		this["Visibility"] = VIS_Owner;

 		shown=true;
}

public func UpdateCursor(int angle)
{	
	var item_count=GetLength(menu_icons); 
	if(!item_count) item_count = 1;
	var segment=360/item_count;
	var dvar=0;
    var stat=false;
	if(item_count<=10) stat=true;
	for(var i=0; i<=item_count ; i++)
	{ 
		if(menu_icons[i])
        		{
        			if(!stat) 	menu_icons[i]->SetSize(((600000)/item_count)/32);
        			else 		menu_icons[i]->SetSize(1950);
        		}
		if(i==item_count) var miss=360-(segment*item_count);
		if(angle>=(segment*i) && angle<=((segment*(i+1)+miss))) dvar=i+1;
	}    
	if(dvar==item_count+1) dvar=item_count;
	if(menu_icons[dvar-1])
	{
	if(!stat)	menu_icons[dvar-1]->SetSize(((((600000)/item_count)/32)*16) /10);
    else 		menu_icons[dvar-1]->SetSize(1950*14 /10);
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

	RemoveObject(); 
}



func Definition(def) {
        SetProperty("Name", "$Name$", def);
}