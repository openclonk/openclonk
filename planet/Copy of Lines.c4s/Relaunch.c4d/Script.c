/*
	
*/


/*
Spellchooser via Mouse
Author: MimmoO

*/

local menuitem;
local choses;

func ContainedUse(object clonk, int x, int y)
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
        	return false;
      }
      else
      {
      	menuitem->Select(Angle(menuitem->GetX(),menuitem->GetY(),GetX()+x,GetY()+y));
      	if(choses==2)
      	{
      		var clnk=Contents();
      		clonk->Exit();
      		clonk->SetPosition(RandomX(30,LandscapeWidth()-30),-20);
      		this->RemoveObject();
      	} 
      	choses++;
      }
}



public func Selected(object menu, object selector)
{
	for(var i=0; i<(selector->GetAmount()); i++)
	{
	var newobj=CreateObject(selector->GetSymbol());
	newobj->Enter(Contents());
	}
	selector->RemoveObject();
	menu->Show();
	if(choses==2) return 1;
	return 0;
}


func Definition(def) {
        SetProperty("Name", "$Name$", def);
}