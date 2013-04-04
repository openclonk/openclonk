static active_menu;

func Initialize()
{
	var starter_menu =
	{
		Style = MENU_Multiple,
		X = [1000], Y = [0, -100],
		Wdt = [1000, 200], Hgt = [0, 100],
		text = {Style = MENU_TextVCenter | MENU_TextHCenter, Text = "OPEN MENU"}
	};
}
/* -------------------------------- MAIN ----------------------------- */

func InitializePlayer(plr)
{
	Schedule(nil, "Scenario->StartMenu()", 5, 0);
}

func MainOnHover(parameter, int ID)
{
	Menu_UpdateText(parameter, active_menu, 9999);
}
func StartMenu(plr)
{
	var main_menu = 
	{
		Decoration = GUI_MenuDeco,
		head = {Hgt = [0, 50], Text = "Please choose a test!", Style = MENU_TextHCenter | MENU_TextVCenter, IDs = 0},
		body = {Y = [0, 60], right = {X = 500, BackgroundColor = 0x50ffffff } },
	};
	var menu = CreateCustomMenu(MenuStyle_List);
	main_menu.body.left = menu;
	
	menu.Wdt = 500;
	menu->SetMouseOverCallback(Scenario, "MainOnHover");
	menu->AddItem(Chest, "Test Multiple Lists (Inventory)", nil, Scenario, "StartMultipleListTest", "Shows multiple list-style menus in one big menu.");
	menu->AddItem(Rule_TeamAccount, "Test Client/Host (Scenario Options)", nil, Scenario, "StartScenarioOptionsTest", "Shows how to display a dialogue that behaves differently for players.");
	
	active_menu = CustomMenuOpen(main_menu);
}

/* ------------------------ inventory test ----------------------------- */
static selected_inventory, inv_menus;
func StartMultipleListTest()
{
	CustomMenuClose(active_menu);
	selected_inventory = [];
	inv_menus = [];
	// layout: headline and four sections with items
	var menu = 
	{
		head = { ID = 999, Hgt = [0, 50], Text = "Inventory: <c ff0000>Empty</c>", Style = MENU_TextHCenter | MENU_TextVCenter, BackgroundColor = 0x55000000},
		contents = { Y = [0, 50], X = [0, 20], Wdt = [1000, -20] },
	};
	
	var inventory = [[Sword, Axe, Club], [IronBomb, Dynamite, Boompack, Firestone], [Bow, Musket, Javelin], [Shield, Bread, Sproutberry, CookedMushroom]];
	var x = [0, [500, 20], 0, [500, 20]], y = [0, 0, [500, 20], [500, 20]], w = [[500, -20], 1000, [500, -20], 1000], h = [[500, -20], [500, -20], 1000, 1000];
	for (var i = 0; i < 4; ++i)
	{
		var inv = inventory[i];
		var ID = 9000 + i;
		var deco = { Decoration = GUI_MenuDeco, X = x[i], Y = y[i], Wdt = w[i], Hgt = h[i] };
		var m = CreateCustomMenu(MenuStyle_List);
		deco.menu = m;
		Menu_AddSubmenu(deco, menu.contents);
		PushBack(inv_menus, m); // remember for later
		Menu_AddMargin(m, 20, 20);
		for (var obj in inv)
			m->AddItem(obj, obj.Description, nil, Scenario, "SelectInventory", [obj, ID]);
	}
	active_menu = CustomMenuOpen(menu);
}

func SelectInventory(info)
{
	var obj = info[0];
	var ID = info[1];
	PushBack(selected_inventory, obj);
	var text = "Your inventory: ";
	for (var item in selected_inventory)
		text = Format("%s %s,", text, item.Name);
	if (GetLength(selected_inventory) == 4)
	{
		Log("HERO! YOU WILL SPAWN NOW! %s", text);
		for (var m in inv_menus)
			if (m) m->Close();
		CustomMenuClose(active_menu);
	}
	else
	{
		var update = { Text = text };
		CustomMenuUpdate(update, active_menu, 999);
	}
}

/* ------------------------ scenario options test ----------------------------- */
static scenoptions_dummies;
func StartScenarioOptionsTest(parameter, int ID, int player)
{
	CustomMenuClose(active_menu);
	scenoptions_dummies = [];
	scenoptions_dummies[0] = CreateObject(Dummy, nil, nil, player);
	scenoptions_dummies[1] = CreateObject(Dummy, nil, nil, player);
	
	for (var i = 0; i <= 1; ++i)
	{
		if (i == 0)
		{
			scenoptions_dummies[i]->SetOwner(player);
			scenoptions_dummies[i].Visibility = VIS_Owner;
		}
		else
		{
			var vis = [VIS_Select];
			for (var p = 0; p <= GetPlayerCount(); ++p)
			{
				var plr = GetPlayerByIndex(p);
				if (plr == player) continue;
				vis[plr + 1] = 1;
			}
			scenoptions_dummies[i].Visibility = vis;
		}
	}
	var menu = 
	{
		list = 
		{
			Wdt = 500,
			Style = MENU_VerticalLayout,
		},
		right = {
			X = 500,
			Decoration = GUI_MenuDeco,
			hostdesc =
			{
				ID = 1,
				Target = scenoptions_dummies[0],
				Text = "Please select the scenario options!"
			},
			clientdesc =
			{
				
				ID = 1,
				Target = scenoptions_dummies[1],
				Text = Format("%s can set the options now! Please wait!", GetTaggedPlayerName(player))
			}
		}
	};
	Menu_AddMargin(menu.right.hostdesc, 25, 25);
	Menu_AddMargin(menu.right.clientdesc, 25, 25);
	
	var def, rules =[], i = 0;
	while (def = GetDefinition(i++))
	{
		if (!(def->GetCategory() & C4D_Rule)) continue;
		PushBack(rules, {def = def, ID = i});
	}
	for (var rule in rules)
	{	
		var subm =
		{
			ID = rule.ID,
			Hgt = [0, 64],
			icon = {Priority = 10, Symbol = rule.def, Wdt = [0, 64], Hgt = [0, 64]},
			text = {Priority = 10, X = [0, 64], Style = MENU_TextVCenter, Text = rule.def.Name},
			
			selector = // only visible for host
			{
				Target = scenoptions_dummies[0],
				Priority = 1,
				BackgroundColor = {Std = 0, Hover = 0x50ff0000, On = 0x5000ff00},
				OnMouseIn = {
					Std = [MenuAction_Call(Scenario, "ScenOptsUpdateDesc", [rule.def, rule.ID, false]), MenuAction_SetTag(nil, nil, "Hover")],
					On = MenuAction_Call(Scenario, "ScenOptsUpdateDesc", [rule.def, rule.ID, true])
					},
				OnMouseOut = { Hover = MenuAction_SetTag(nil, nil, "Std"), On = nil },
				OnClick = {
					Hover = [MenuAction_Call(Scenario, "ScenOptsActivate", [rule.def, rule.ID]), MenuAction_SetTag(nil, nil, "On")],
					On = [MenuAction_Call(Scenario, "ScenOptsDeactivate", [rule.def, rule.ID]), MenuAction_SetTag(nil, nil, "Hover")],
					},
			}
		};
		Menu_AddSubmenu(subm, menu.list);
	}
	
	active_menu = CustomMenuOpen(menu);
}

func ScenOptsActivate(int player, int ID, int subwindowID, object target, data)
{
	if (!ObjectCount(Find_ID(data[0])))
		CreateObject(data[0]);
}

func ScenOptsDeactivate(int player, int ID, int subwindowID, object target, data)
{
	RemoveAll(Find_ID(data[0]));
}

func ScenOptsUpdateDesc(int player, int ID, int subwindowID, object target, data)
{
	var text = "<c ff0000>Do you really want to remove the rule???</c>";
	if (!data[2])
		text = data[0].Description;
	Menu_UpdateText(text, active_menu, 1, scenoptions_dummies[0]);
}