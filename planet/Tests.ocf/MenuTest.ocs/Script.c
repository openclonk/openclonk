static active_menu;

func Initialize()
{
	var starter_menu =
	{
		Style = GUI_Multiple | GUI_TextVCenter | GUI_TextHCenter,
		Decoration = GUI_MenuDeco,
		Left = "100%-7em", Top = "+3em",
		Right = "100%-1em", Bottom = "+4em",
		Text = "OPEN MENU",
		BackgroundColor = {Std = 0, Hover = 0xffff0000},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(Scenario, "StartMenu")
	};
	GuiOpen(starter_menu);
	
	//Schedule(nil, "Scenario->TestDesync()", 2);
}

static desync_menu;
func TestDesync()
{
	var rock = CreateObject(Rock);
	Random(100);
	var menu =
	{
		Prototype = rock,
		BackgroundColor = RGB(255, 100, 0),
		OnClick = GuiAction_Call(Scenario, "Desync"),
		OnClose = GuiAction_Call(Scenario, "DesyncClose")
	};
	Log("DESYNC TEST - OPENING MENU*~");
	desync_menu = GuiOpen(menu);
	Log("DESYNC TEST - OPENING MENU DONE*~*");
}

func Desync()
{
	Random(100);
	GuiClose(desync_menu);
}

func DesyncClose()
{
	Random(100);
	Log("Desync Close");
}

func MarginTest()
{
	var menu = 
	{
		Style = GUI_GridLayout,
		BackgroundColor = RGB(255, 0, 0),
	};
	
	for (var i = 0; i < 10; ++i)
	{
		var sizes = ["8em", "8em", "16em", "16em"];
		ShuffleArray(sizes);
		var entry = 
		{
			BackgroundColor = RGB(0, 255, 0),
			Left = 0, Top = 0,
			Right = sizes[0], Bottom = sizes[1],
			Margin = ["1em"],//["1em", "1em", "1em", "1em"]
		};
		GuiAddSubwindow(entry, menu);
	}
	active_menu = GuiOpen(menu);
}

func CloseCurrentMenu()
{
	GuiClose(active_menu);
	active_menu = 0;
}	

/* -------------------------------- MAIN ----------------------------- */
func MainOnHover(parameter, int ID)
{
	GuiUpdateText(parameter, active_menu, 9999);
}
func StartMenu(plr)
{
	if (active_menu)
	{
		if (GuiClose(active_menu)) return;
	}

	var main_menu = 
	{
		Decoration = GUI_MenuDeco,
		head = {Bottom = "+3em", Text = "Please choose a test!", Style = GUI_TextHCenter | GUI_TextVCenter, IDs = 0},
		body = {Top = "+3em", right = {ID = 9999, Left = "50%"} },
	};
	GuiAddCloseButton(main_menu, Scenario, "CloseCurrentMenu");
	var menu = CreateObject(MenuStyle_List);
	main_menu.body.left = menu;
	
	menu.Right = "50%";
	menu->SetMouseOverCallback(Scenario, "MainOnHover");
	menu->AddItem(Chest, "Test Multiple Lists (Inventory)", nil, Scenario, "StartMultipleListTest", "Shows multiple list-style menus in one big menu.");
	menu->AddItem(Rule_TeamAccount, "Test Client/Host (Scenario Options)", nil, Scenario, "StartScenarioOptionsTest", "Shows how to display a dialogue that behaves differently for players.");
	menu->AddItem(Clonk, "Test Multiple Windows (Player List)", nil, Scenario, "StartPlayerListTest", "Shows how to display a permanent info dialogue.");
	menu->AddItem(Lorry, "Tests Two Grid Menus (Trade Menu)", nil, Scenario, "StartTransferTest", "Shows how to work with two grid menus.");
	menu->AddItem(Sproutberry, "Test HP Bars (HP Bars!)", nil, Scenario, "StartHPBarTest", "HP BARS!!!");
	menu->AddItem(Crate, "Test mixed grid layout.", nil, Scenario, "StartMixedGridTest", "Grid menu with differently-sized items.");
	
	active_menu = GuiOpen(main_menu);
}

/* ------------------------ inventory test ----------------------------- */
static selected_inventory, inv_menus;
func StartMultipleListTest()
{
	GuiClose(active_menu);
	selected_inventory = [];
	inv_menus = [];
	// layout: headline and four sections with items
	var menu = 
	{
		head = { ID = 999, Bottom = "+3em", Text = "Inventory: <c ff0000>Empty</c>", Style = GUI_TextHCenter | GUI_TextVCenter, BackgroundColor = 0x55000000},
		contents = { Top = "+5em", Left = "+1em", Right = "100%-1em" },
	};
	GuiAddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
	var inventory = [[Sword, Axe, Club], [IronBomb, Dynamite, Boompack, Firestone], [Bow, Blunderbuss, Javelin], [Shield, Bread, Sproutberry, CookedMushroom]];
	var x = ["0%", "50%", "0%", "50%"], y = ["0%", "0%", "50%", "50%"], w = ["50%", "100%", "50%", "100%"], h = ["50%", "50%", "100%", "100%"];
	for (var i = 0; i < 4; ++i)
	{
		var inv = inventory[i];
		var ID = 9000 + i;
		var m = CreateObject(MenuStyle_List);
		m.Decoration = GUI_MenuDeco;
		m.Left = x[i]; m.Top = y[i];
		m.Right = w[i]; m.Bottom = h[i];
		m.Margin = "2em";
		GuiAddSubwindow(m, menu.contents);
		PushBack(inv_menus, m); // remember for later
		for (var obj in inv)
			m->AddItem(obj, obj.Description, nil, Scenario, "SelectInventory", [obj, ID]);
	}
	active_menu = GuiOpen(menu);
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
		GuiClose(active_menu);
	}
	else
	{
		var update = { Text = text };
		GuiUpdate(update, active_menu, 999);
	}
}

/* ------------------------ scenario options test ----------------------------- */
static scenoptions_dummies;
func StartScenarioOptionsTest(parameter, int ID, int player)
{
	GuiClose(active_menu);
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
			Right = "50%",
			Margin = ["2em", "0em"],
			Style = GUI_VerticalLayout,
		},
		right = {
			Left = "50%",
			Margin = ["2em", "0em"],
			Decoration = GUI_MenuDeco,
			hostdesc =
			{
				ID = 1,
				Target = scenoptions_dummies[0],
				// this is also a test for updating children by name
				icon = {Left="50%-4em", Right="50%+4em", Bottom="5em", Top="1em", Symbol = Clonk},
				textwindow =
				{
					Top = "6em",
					Text = "Please select the scenario options!"
				}
			},
			clientdesc =
			{
				
				ID = 1,
				Target = scenoptions_dummies[1],
				Text = Format("%s can set the options now! Please wait!", GetTaggedPlayerName(player))
			}
		}
	};
	GuiAddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
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
			Bottom = "+4em",
			Tooltip = rule.def->GetName(),
			icon = {Priority = 10, Symbol = rule.def, Right = "+4em", Bottom = "+4em"},
			text = {Priority = 10, Left = "+4em", Style = GUI_TextVCenter, Text = rule.def.Name},
			
			selector = // only visible for host
			{
				Target = scenoptions_dummies[0],
				Priority = 1,
				BackgroundColor = {Std = 0, Hover = 0x50ff0000, On = 0x2000ff00},
				OnMouseIn = {
					Std = [GuiAction_Call(Scenario, "ScenOptsUpdateDesc", [rule.def, rule.ID, false]), GuiAction_SetTag("Hover")],
					On = GuiAction_Call(Scenario, "ScenOptsUpdateDesc", [rule.def, rule.ID, true])
					},
				OnMouseOut = { Hover = GuiAction_SetTag("Std"), On = nil },
				OnClick = {
					Hover = [GuiAction_Call(Scenario, "ScenOptsActivate", [rule.def, rule.ID]), GuiAction_SetTag("On")],
					On = [GuiAction_Call(Scenario, "ScenOptsDeactivate", [rule.def, rule.ID]), GuiAction_SetTag("Hover")],
					},
			},
			tick = 
			{
				Left = "100%-3em", Top = "50%-1em",
				Right = "100%-1em", Bottom = "50%+1em",
				Symbol = {Std = 0, Unticked = 0, Ticked = Icon_Ok}
			}
		};
		GuiAddSubwindow(subm, menu.list);
	}
	
	active_menu = GuiOpen(menu);
}

func ScenOptsActivate(data, int player, int ID, int subwindowID, object target)
{
	if (!ObjectCount(Find_ID(data[0])))
		CreateObject(data[0]);
	GuiUpdateTag("Ticked", active_menu, data[1], nil);
}

func ScenOptsDeactivate(data, int player, int ID, int subwindowID, object target)
{
	RemoveAll(Find_ID(data[0]));
	GuiUpdateTag("Unticked", active_menu, data[1], nil);
}

func ScenOptsUpdateDesc(data, int player, int ID, int subwindowID, object target)
{
	var text = "<c ff0000>Do you really want to remove the rule???</c>";
	if (!data[2])
		text = data[0].Description;
	var update = 
	{
		icon = {Symbol = data[0]},
		textwindow = {Text = text}
	};
	GuiUpdate(update, active_menu, 1, scenoptions_dummies[0]);
}

/* ------------------------ player list test ----------------------------- */
static player_list_menu;
func StartPlayerListTest(parameter, int ID, int player)
{
	if (player_list_menu)
	{
		GuiClose(player_list_menu);
		player_list_menu = nil;
		return -1;
	}
	
	var menu =
	{
		Left = "100%-9em", Top = "+6em",
		Right = "100%0em", Bottom = "+12em",
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
			Bottom = "+1em",
			Text = name,
			Style = GUI_TextRight | GUI_TextVCenter,
			icon = 
			{
				Symbol = Clonk,
				Right = "+1em"
			}
		};
		GuiAddSubwindow(subm, menu);
	}
	
	player_list_menu = GuiOpen(menu);
	
	return -1; // keep open
}

/* ------------------------ transfer test ----------------------------- */
static transfer_left, transfer_right, transfer_menus, transfer_id_count;
func StartTransferTest()
{
	GuiClose(active_menu);
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
		head = { Bottom = "+3em", Text = "Welcome to the trade menu!", Style = GUI_TextHCenter | GUI_TextVCenter},
		contents = { Margin = "1em" },
	};
	GuiAddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
	for (var i = 0; i < 2; ++i)
	{
		var m = CreateObject(MenuStyle_Grid);
		m.Decoration = GUI_MenuDeco;
		m.Text = "FROM";
		m.Style = GUI_TextHCenter | GUI_GridLayout;
		if (i == 1)
		{
			m.Left = "50% + 2em";
			m.Text = "TO";
		} else m.Right = "50% - 2em";
		GuiAddSubwindow(m, menu.contents);
		var a = transfer_left;
		if (i == 1) a = transfer_right;
		
		for (var c = 0; c < GetLength(a); ++c)
		{
			var obj = a[c];
			m->AddItem(obj, obj.Name, ++transfer_id_count, Scenario, "SelectTransferGood", [obj, i]);
		}
		transfer_menus[i] = m;
	}
	active_menu = GuiOpen(menu);
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
		GuiClose(HP_bar_menu);
		return -1; // keep open
	}
	
	var menu =
	{
		Left = "+0.5em", Top = "+3em",
		Right = "+1em", Bottom = "100%-3em",
		Style = GUI_Multiple | GUI_IgnoreMouse,
		BackgroundColor = RGB(255, 0, 0),
		blackOverlay = {ID = 1, Bottom = "0%", BackgroundColor = RGB(10, 10, 10)},
		OnClose = GuiAction_Call(Scenario, "OnHPBarClose")
	};
	if (!GetEffect("FoolAroundWithHPBars"))
		AddEffect("FoolAroundWithHPBar", nil, 1, 2);
	HP_bar_menu = GuiOpen(menu);
	
	return -1; // keep open
}

global func FxFoolAroundWithHPBarTimer(target, effect, time)
{
	var state = Abs(Cos(time, 100));
	var update = {Bottom = Format("%d%%", state)};
	GuiUpdate(update, HP_bar_menu, 1);
}

func OnHPBarClose()
{
	RemoveEffect("FoolAroundWithHPBar");
	HP_bar_menu = nil;
	Log("HP bar off!");
}

/* ------------------- mixed grid menu test ---------------*/
func StartMixedGridTest()
{
	GuiClose(active_menu);
	
	var menu = 
	{
		toptext = {Bottom = "2em", Text="GUI_TightGridLayout", Style = GUI_TextHCenter},
		top = {Top = "2em", Bottom = "50%", Margin = "0.5em", Style = GUI_TightGridLayout, Decoration = GUI_MenuDeco},
		bottomtext = {Top = "50%", Bottom = "50% + 2em", Text="GUI_GridLayout", Style = GUI_TextHCenter}, 
		bottom = {Top = "50% + 2em", Margin = "0.5em", Style = GUI_GridLayout, Decoration = GUI_MenuDeco},
		BackgroundColor = RGBa(0, 0, 0, 128),
	};
	GuiAddCloseButton(menu, Scenario, "CloseCurrentMenu");
	
	var seed = 1;
	for (var i = 0; i < 50; ++i)
	{
		seed = ((seed * 17) + 7) % 1357;
		var w = (seed % 4) + 1;
		seed = ((seed * 23) + 13)% 1357;
		var h = (seed % 4) + 1;
		var item = 
		{
			Right = Format("%dem", w),
			Bottom = Format("%dem", h),
			Priority = i + 1,
			BackgroundColor = HSL(seed % 255, 200, 200)
		};
		GuiAddSubwindow(item, menu.top);
		GuiAddSubwindow(item, menu.bottom);
	}
	active_menu = GuiOpen(menu);
}