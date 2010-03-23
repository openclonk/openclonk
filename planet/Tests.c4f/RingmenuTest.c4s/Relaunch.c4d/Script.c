
local menuitem;
local choses;

func ChooseMenu(object clonk)
{
	if(!menuitem)
	{
       	menuitem=clonk->CreateRingMenu(Clonk,0,0,this);
		menuitem->AddItem(Bow);
		menuitem->AddItem(Arrow);
		menuitem->AddItem(Musket);
		menuitem->AddItem(LeadShot);
		menuitem->AddItem(Shield);
		menuitem->AddItem(Sword);
		menuitem->AddItem(Club);
		menuitem->AddItem(Javelin);
		menuitem->AddItem(Boompack);
		menuitem->AddItem(DynamiteBox);
		menuitem->AddItem(Dynamite,3);
		menuitem->AddItem(JarOfWinds);
		menuitem->AddItem(Loam,3);
		menuitem->AddItem(Firestone,5);
		menuitem->AddItem(Fireglobe,3);
		menuitem->AddItem(Blackpowder,3); 
       	menuitem->Show();       		
    }
	return true;
}



public func Selected(object menu, object selector)
{
	if(!selector) return 0;
	
	for(var i=0; i<(selector->GetAmount()); i++)
	{
		var newobj=CreateObject(selector->GetSymbol());
		newobj->Enter(Contents());
	}
	selector->RemoveObject();
	menu->Show();
	if(choses==2)
	{
      	var clonk=Contents();
      	clonk->Exit();
      	clonk->SetPosition(RandomX(30,LandscapeWidth()-30),-20);
      	this->RemoveObject();
		return 1;
	}
	choses++;
	return 0;
}


func Definition(def) {
        SetProperty("Name", "relaunch", def);
}