
static g_intro_initialized;

global func IntroStart()
{
	if (!g_intro_initialized && !GetEffect("IntIntro"))
	{
		AddEffect("IntIntro", nil, 1, 2, nil, nil);
		g_intro_initialized = true;
	}
}

global func IntroAddPlayer(int plr)
{
	var effect = GetEffect("IntIntro");
	if (!effect) return false;
	if (effect.Time > 30) return false;

	var crew;
	for (var index = 0; crew = GetCrew(plr, index); ++index)
	{
	        var skin = crew->GetCrewExtraData("Skin");
        	if (skin == nil) skin = GetPlrClonkSkin(plr);

		var container = effect.Cabin->CreateContents(Clonk);
		container->SetOwner(plr);
		if (skin != nil) container->SetSkin(skin);
		container->SetName(crew->GetName());
		container->SetAction("Walk");
		crew->Enter(container);

		container.ActMap = { Prototype = Clonk.ActMap };
		container.ActMap.Walk = { Prototype = Clonk.ActMap.Walk, Decel = 100 };
		container.ActMap.Jump = { Prototype = Clonk.ActMap.Jump, Speed = 0, Accel = 0 };
		container.JumpSpeed = 0;

		SetPlrView(plr, crew);
		SetPlayerViewLock(plr, true);
		SetPlayerZoomByViewRange(plr, 320, 240);

		container->SetCommand("None", container);
		crew->SetCommand("None", crew);

		// Give everyone but the first player a shovel -- the first
		// player's shovel can be found in the valley
		if (GetLength(effect.Players) != 0)
			crew->CreateContents(Shovel);

		effect.Players[GetLength(effect.Players)] = crew;
	}

	return true;
}

global func IntroCreateBoompack(int x, int y, int fuel)
{
	var boompack = CreateObjectAbove(Boompack, x, y, NO_OWNER);
	boompack->SetFuel(fuel);
	boompack->SetDirectionDeviation(8); // make sure direction of boompack is roughly kept
	boompack->SetControllable(false);
	return boompack;
}

global func FxIntIntroStart(object target, proplist effect)
{
	effect.Cabin = FindObject(Find_ID(WoodenCabin));
	if (!effect.Cabin) return -1;

	effect.Sister = CreateObjectAbove(Clonk, 174, 532, NO_OWNER);
	effect.Sister->MakeInvincible();
	effect.Sister->SetSkin(1);
	effect.Sister->SetName("$NameSister$");
	effect.Sister->SetColor(RGB(213, 68, 172));
	effect.Sister->SetObjectLayer(effect.Sister);
	effect.Sister->SetDir(DIR_Right);
	effect.Sister.ActMap = { Prototype = Clonk.ActMap };
	effect.Sister.ActMap.Walk = { Prototype = Clonk.ActMap.Walk, Decel = 100 };
	effect.Sister.ActMap.Jump = { Prototype = Clonk.ActMap.Jump, Speed = 0, Accel = 0 };
	effect.Sister.JumpSpeed = 0;

	effect.Dialog = effect.Sister->SetDialogue("Sister");
	effect.Rock = effect.Sister->CreateContents(Rock);
	effect.Rock->SetObjectLayer(nil);
	effect.Players = [];
}

global func FxIntIntroTimer(object target, proplist effect, int time)
{
	if (effect.Time == 40)
	{
		effect.Sister->SetCommand("MoveTo", effect.Sister, effect.Cabin->GetX() + 65 - effect.Sister->GetX(), effect.Cabin->GetY() + 10 - effect.Sister->GetY());
	}

	if (effect.Time == 110)
		effect.Dialog->MessageBoxAll("$MsgIntro1$", effect.Sister);

	if (effect.Time == 150)
	{
		for (var crew in effect.Players)
		{
			crew = crew->Contained();

			crew->SetCommand("Exit", crew);
			crew->AppendCommand("MoveTo", crew, effect.Cabin->GetX() + RandomX(10, 40) - crew->GetX(), effect.Cabin->GetY() - crew->GetY());
		}
	}

	if (effect.Time == 200)
		effect.Dialog->MessageBoxAll("$MsgIntro2$", GetCrew(GetPlayerByIndex(Random(GetPlayerCount())), 0));

	if (effect.Time == 270)
	{
		effect.Dialog->MessageBoxAll("$MsgIntro3$", effect.Sister);
	}

	if (effect.Time == 350)
	{
		effect.Sister->SetCommand("MoveTo", effect.Sister, 214 - effect.Sister->GetX(), 540 - effect.Sister->GetY());
	}

	if (effect.Time == 370)
	{
		for (var crew in effect.Players)
		{
			crew = crew->Contained();
			crew->SetCommand("MoveTo", crew, 245 - crew->GetX(), 555 - crew->GetY());
		}
	}

	if (effect.Time == 500)
	{
		effect.Sister->SetCommand("MoveTo", effect.Sister, 214 - effect.Sister->GetX(), 540 - effect.Sister->GetY());
		for (var crew in effect.Players)
			crew->Contained()->SetDir(DIR_Left);
		effect.Dialog->MessageBoxAll("$MsgIntro4$", GetCrew(GetPlayerByIndex(Random(GetPlayerCount())), 0));
	}

	if (effect.Time == 520)
		effect.Sister->SetDir(DIR_Right);

	if (effect.Time == 550)
	{
		effect.Sister->ObjectCommand("Throw", effect.Rock, 500, 100);
		for (var i = 0; i < GetPlayerCount(); ++i)
			SetPlrView(GetPlayerByIndex(i), effect.Sister);
	}
	
	if (effect.Time == 556)
	{
		for (var crew in effect.Players)
		{
			crew->Contained()->Fling(2,-2);
			crew->Contained()->PlaySoundScream();
		}
	}

	if (effect.Time == 570)
	{
		effect.Dialog->MessageBoxAll("$MsgIntro5$", effect.Sister);
	}

	if (effect.Time == 620)
	{
		effect.Dialog->MessageBoxAll("$MsgIntro6$", GetCrew(GetPlayerByIndex(Random(GetPlayerCount())), 0));
	}

	if (effect.Time == 700)
	{
		effect.Dialog->MessageBoxAll("$MsgIntro7$", effect.Sister);
	}

	if (effect.Time == 800)
	{
		effect.Dialog->MessageBoxAll("$MsgIntro8$", effect.Sister);
	}

	if (effect.Time == 860)
	{
		effect.Sister->SetCommand("Enter", effect.Cabin);
	}

	if (effect.Time == 920)
	{
		for (var i = 0; i < GetPlayerCount(); ++i)
			GetCursor(GetPlayerByIndex(i))->CloseMenu();
	}

	if (effect.Time == 950)
	{
		for (var crew in effect.Players)
		{
			SetPlrView(crew->GetOwner(), crew);
			SetPlayerViewLock(crew->GetOwner(), true);
			var container = crew->Contained();
			crew->Exit(0, 10);
			container->RemoveObject();
		}

		for (var i = 0; i < GetPlayerCount(); ++i)
			GetCursor(GetPlayerByIndex(i))->CloseMenu();
	}

	if (effect.Time >= 1000)
	{
		// just to be sure...
		effect.Sister->Enter(effect.Cabin);
		return -1;
	}
}
