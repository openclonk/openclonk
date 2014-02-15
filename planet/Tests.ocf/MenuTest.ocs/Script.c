static active_menu;

func Initialize()
{
	var starter_menu =
	{
		Style = GUI_Multiple | GUI_TextVCenter | GUI_TextHCenter,
		Decoration = GUI_MenuDeco,
		X = [1000, -100], Y = [0, 50],
		Wdt = [1000, -25], Hgt = [0, 75],
		Text = "OPEN MENU",
		BackgroundColor = {Std = 0, Hover = 0xffff0000},
		OnMouseIn = GuiAction_SetTag(nil, nil, "Hover"),
		OnMouseOut = GuiAction_SetTag(nil, nil, "Std"),
		OnClick = GuiAction_Call(Scenario, "StartMenu")
	};
	CustomGuiOpen(starter_menu);
}

func CloseCurrentMenu()
{
	CustomGuiClose(active_menu);
	active_menu = 0;
}	

/* -------------------------------- MAIN ----------------------------- */
func MainOnHover(parameter, int ID)
{
	Gui_UpdateText(parameter, active_menu, 9999);
}
func StartMenu(plr)
{
	if (active_menu)
		CustomGuiClose(active_menu);
	var main_menu = 
	{
		Decoration = GUI_MenuDeco,
		head = {Hgt = [0, 50], Text = "Please choose a test!", Style = GUI_TextHCenter | GUI_TextVCenter, IDs = 0},
		body = {Y = [0, 60], right = {ID = 9999, X = 500} },
	};
	Gui_AddCloseButton(main_menu, Scenario, "CloseCurrentMenu");
	var menu = CreateObject(MenuStyle_List);
	main_menu.body.left = menu;
	
	menu.Wdt = 500;
	menu->SetMouseOverCallback(Scenario, "MainOnHover");
	menu->AddItem(Chest, "Test Multiple Lists (Inventory)", nil, Scenario, "StartMultipleListTest", "Shows multiple list-style menus in one big menu.");
	menu->AddItem(Rule_TeamAccount, "Test Client/Host (Scenario Options)", nil, Scenario, "StartScenarioOptionsTest", "Shows how to display a dialogue that behaves differently for players.");
	menu->AddItem(Clonk, "Test Multiple Windows (Player List)", nil, Scenario, "StartPlayerListTest", "Shows how to display a permanent info dialogue.");
	menu->AddItem(Lorry, "Tests Two Grid Menus (Trade Menu)", nil, Scenario, "StartTransferTest", "Shows how to work with two grid menus.");
	menu->AddItem(Sproutberry, "Test HP Bars (HP Bars!)", nil, Scenario, "StartHPBarTest", "HP BARS!!!");
	
	active_menu = CustomGuiOpen(main_menu);
}

/* ------------------------ inventory test ----------------------------- */
static selected_inventory, inv_menus;
func StartMultipleListTest()
{
	CustomGuiClose(active_menu);
	selected_inventory = [];
	inv_menus = [];
	// layout: headline and four sections with items
	var menu = 
	{
		head = { ID = 999, Hgt = [0, 50], Text = "Inventory: <c ff0000>Empty</c>", Style = GUI_TextHCenter | GUI_TextVCenter, BackgroundColor = 0x55000000},
		contents = { Y = [0, 50], X = [0, 20], Wdt = [1000, -20] },
	};
	Gui_AddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
	var inventory = [[Sword, Axe, Club], [IronBomb, Dynamite, Boompack, Firestone], [Bow, Musket, Javelin], [Shield, Bread, Sproutberry, CookedMushroom]];
	var x = [0, [500, 20], 0, [500, 20]], y = [0, 0, [500, 20], [500, 20]], w = [[500, -20], 1000, [500, -20], 1000], h = [[500, -20], [500, -20], 1000, 1000];
	for (var i = 0; i < 4; ++i)
	{
		var inv = inventory[i];
		var ID = 9000 + i;
		var m = CreateObject(MenuStyle_List);
		m.Decoration = GUI_MenuDeco;
		m.X = x[i]; m.Y = y[i];
		m.Wdt = w[i]; m.Hgt = h[i];
		Gui_AddSubwindow(m, menu.contents);
		PushBack(inv_menus, m); // remember for later
		for (var obj in inv)
			m->AddItem(obj, obj.Description, nil, Scenario, "SelectInventory", [obj, ID]);
	}
	active_menu = CustomGuiOpen(menu);
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
		CustomGuiClose(active_menu);
	}
	else
	{
		var update = { Text = text };
		CustomGuiUpdate(update, active_menu, 999);
	}
}

/* ------------------------ scenario options test ----------------------------- */
static scenoptions_dummies;
func StartScenarioOptionsTest(parameter, int ID, int player)
{
	CustomGuiClose(active_menu);
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
			Style = GUI_VerticalLayout,
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
	Gui_AddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
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
			text = {Priority = 10, X = [0, 64], Style = GUI_TextVCenter, Text = rule.def.Name},
			
			selector = // only visible for host
			{
				Target = scenoptions_dummies[0],
				Priority = 1,
				BackgroundColor = {Std = 0, Hover = 0x50ff0000, On = 0x2000ff00},
				OnMouseIn = {
					Std = [GuiAction_Call(Scenario, "ScenOptsUpdateDesc", [rule.def, rule.ID, false]), GuiAction_SetTag(nil, nil, "Hover")],
					On = GuiAction_Call(Scenario, "ScenOptsUpdateDesc", [rule.def, rule.ID, true])
					},
				OnMouseOut = { Hover = GuiAction_SetTag(nil, nil, "Std"), On = nil },
				OnClick = {
					Hover = [GuiAction_Call(Scenario, "ScenOptsActivate", [rule.def, rule.ID]), GuiAction_SetTag(nil, nil, "On")],
					On = [GuiAction_Call(Scenario, "ScenOptsDeactivate", [rule.def, rule.ID]), GuiAction_SetTag(nil, nil, "Hover")],
					},
			},
			tick = 
			{
				X = [1000, -60], Y = [500, -15],
				Wdt = [1000, -30], Hgt =[500, 15],
				Symbol = {Std = 0, Unticked = 0, Ticked = Icon_Ok}
			}
		};
		Gui_AddSubwindow(subm, menu.list);
	}
	
	active_menu = CustomGuiOpen(menu);
}

func ScenOptsActivate(data, int player, int ID, int subwindowID, object target)
{
	if (!ObjectCount(Find_ID(data[0])))
		CreateObject(data[0]);
	CustomGuiSetTag("Ticked", active_menu, data[1], nil);
}

func ScenOptsDeactivate(data, int player, int ID, int subwindowID, object target)
{
	RemoveAll(Find_ID(data[0]));
	CustomGuiSetTag("Unticked", active_menu, data[1], nil);
}

func ScenOptsUpdateDesc(data, int player, int ID, int subwindowID, object target)
{
	var text = "<c ff0000>Do you really want to remove the rule???</c>";
	if (!data[2])
		text = data[0].Description;
	Gui_UpdateText(text, active_menu, 1, scenoptions_dummies[0]);
}

/* ------------------------ player list test ----------------------------- */
static player_list_menu;
func StartPlayerListTest(parameter, int ID, int player)
{
	if (player_list_menu)
	{
		CustomGuiClose(player_list_menu);
		player_list_menu = nil;
		return -1;
	}
	
	var menu =
	{
		X = [1000, -150], Y = [0, 100],
		Wdt = [1000, -5], Hgt = [0, 200],
		Style = GUI_Multiple | GUI_VerticalLayout | GUI_FitChildren,
		BackgroundColor = 0x30000000,
	};
	
	var player_names = [];
	for (var i = 0; i < 15; ++i)
	{
		var p = GetPlayerByIndex(i);
		var name;
		if (p == NO_OWNER) name = Format("Player %d", i + 1);
		else name = GetTaggedPlayerName(p);
		var subm =
		{
			Priority = i,
			Hgt = [0, 25],
			Text = name,
			Style = GUI_TextRight | GUI_TextVCenter,
			icon = 
			{
				Symbol = Clonk,
				Wdt = [0, 25]
			}
		};
		Gui_AddSubwindow(subm, menu);
	}
	
	player_list_menu = CustomGuiOpen(menu);
	
	return -1; // keep open
}

/* ------------------------ transfer test ----------------------------- */
static transfer_left, transfer_right, transfer_menus, transfer_id_count;
func StartTransferTest()
{
	CustomGuiClose(active_menu);
	if (transfer_left == nil)
	{
		transfer_left = [Rock, Loam, Wood, Metal, Nugget, Coal, Shovel, Sword, Bow, Arrow, Boompack];
		transfer_right = [Clonk];
		transfer_menus = [];
		transfer_id_count = 1;
	}
	
	// layout: headline and two submenus
	var menu = 
	{
		head = { Hgt = [0, 50], Text = "Welcome to the trade menu!", Style = GUI_TextHCenter | GUI_TextVCenter},
		contents = { Y = [0, 25], X = [0, 20], Wdt = [1000, -20] },
	};
	Gui_AddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
	for (var i = 0; i < 2; ++i)
	{
		var m = CreateObject(MenuStyle_Grid);
		m.Decoration = GUI_MenuDeco;
		m.Text = "FROM";
		m.Style = GUI_TextHCenter | GUI_GridLayout;
		if (i == 1)
		{
			m.X = [500, 15];
			m.Text = "TO";
		} else m.Wdt = [500, -15];
		Gui_AddSubwindow(m, menu.contents);
		var a = transfer_left;
		if (i == 1) a = transfer_right;
		
		for (var c = 0; c < GetLength(a); ++c)
		{
			var obj = a[c];
			m->AddItem(obj, obj.Name, ++transfer_id_count, Scenario, "SelectTransferGood", [obj, i]);
		}
		transfer_menus[i] = m;
	}
	active_menu = CustomGuiOpen(menu);
}

func SelectTransferGood(data, int user_id, int player)
{
	var obj = data[0];
	var fromLeft = 0 == data[1];
	var menu = transfer_menus[data[1]];
	
	// first, move item from array to array
	var from = transfer_left, to = transfer_right;
	if (!fromLeft)
	{
		from = transfer_right;
		to = transfer_left;
	}
	var found = false;
	for (var i = 0; i < GetLength(from); ++i)
	{
		if (from[i] != obj) continue;
		found = true;
		PushBack(to, obj);
		RemoveArrayIndex(from, i);
		break;
	}
	if (!found) return -1;
	if (!menu->RemoveItem(user_id, active_menu)) Log("remove fail!");
	transfer_menus[1 - data[1]]->AddItem(obj, obj.Name, user_id, Scenario, "SelectTransferGood", [obj, 1 - data[1]], nil, active_menu);
	return -1;
}


/* ------------------------ HP bar test ----------------------------- */
static HP_bar_menu;
func StartHPBarTest(parameter, int ID, int player)
{	
	if (HP_bar_menu)
	{
		CustomGuiClose(HP_bar_menu);
		return -1; // keep open
	}
	
	var menu =
	{
		X = [0, 10], Y = [0, 50],
		Wdt = [0, 15], Hgt = [1000, -50],
		Style = GUI_Multiple | GUI_IgnoreMouse,
		BackgroundColor = RGB(255, 0, 0),
		blackOverlay = {ID = 1, Hgt = 0, BackgroundColor = RGB(10, 10, 10)},
		OnClose = GuiAction_Call(Scenario, "OnHPBarClose")
	};
	if (!GetEffect("FoolAroundWithHPBars"))
		AddEffect("FoolAroundWithHPBar", nil, 1, 2);
	HP_bar_menu = CustomGuiOpen(menu);
	
	return -1; // keep open
}

global func FxFoolAroundWithHPBarTimer(target, effect, time)
{
	var state = Abs(Cos(time, 1000));
	var update = {Hgt = state};
	CustomGuiUpdate(update, HP_bar_menu, 1);
}

func OnHPBarClose()
{
	RemoveEffect("FoolAroundWithHPBar");
	HP_bar_menu = nil;
	Log("HP bar off!");
}