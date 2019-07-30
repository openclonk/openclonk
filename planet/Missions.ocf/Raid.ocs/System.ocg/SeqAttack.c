/* Attack sequence */

#appendto Sequence

func Attack_Start(chopping_clonk)
{
	this.hero = chopping_clonk;
	g_attack_started = true;
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(chopping_clonk);
	MessageBox_last_pos = true; // force first message right side of screen
	MessageBoxAll("$Attack1$", this.hero, true); // tree down
	return ScheduleNext(30);
}

func Attack_1()
{
	var n_planes = 3;
	var plane_x = [1700, 1750, 1800];
	var plane_y = [90, 110, 80];
	this.planes = CreateArray(n_planes);
	for (var i = 0; i<n_planes; ++i)
	{
		var plane = CreateObjectAbove(Airplane, plane_x[i], plane_y[i]);
		var pilot = CreateObjectAbove(Clonk, plane_x[i], plane_y[i]);
		pilot->SetSkin(2);
		pilot->Enter(plane);
		pilot->SetAction("Walk"); // prevents falling out
		pilot->SetColor(0xff101010);
		pilot->SetDir(DIR_Left);
		//plane->FaceLeft();
		plane->PlaneMount(pilot);
		plane->StartInstantFlight(260, 15);
		plane->SetXDir(-15);
		plane->MakeInvincible();
		this.planes[i] = plane;
	}
	//SetPlayerZoomByViewRange(NO_OWNER, 600, 500, PLRZOOM_Set | PLRZOOM_LimitMax);
	//SetViewTarget(this.planes[0]);
	return ScheduleNext(35);
}

func Attack_2()
{
	MessageBoxAll("$Attack2$", this.hero, true); // what's that sound?
	return ScheduleNext(5);
}

func Attack_3()
{
	this.hero->SetDir(DIR_Left);
	return ScheduleNext(15);
}

func Attack_4()
{
	SetViewTarget(this.planes[0]);
	SetPlayerZoomByViewRange(NO_OWNER, 600, 500, PLRZOOM_Set | PLRZOOM_LimitMax);
	return ScheduleNext(50);
}

func Attack_5()
{
	if (this.planes[0]->GetX() > 880) return ScheduleSame(5);
	MessageBoxAll("$Attack3$", npc_lara, true); // oh god!
	for (var i = 0; i<3; ++i)
	{
		this.planes[i]->StartInstantFlight(270, 15);
		this.planes[i]->SetXDir(-15);
		this.planes[i]->SetYDir(0);
	}
	SetViewTarget(g_flagpole);
	// NPCs go nuts
	RemoveEffect("NewtonHammering", npc_newton);
	RemoveEffect("LaraWalking", npc_lara);
	RemoveEffect("WoodyWalking", npc_woody);
	AddEffect("Attack_Panic", npc_newton, 1, 30, this);
	AddEffect("Attack_Panic", npc_lara, 1, 35, this);
	AddEffect("Attack_Panic", npc_woody, 1, 40, this);
	AddEffect("Attack_Panic", npc_lisa, 1, 45, this);
	// Update dialogues; remove attention markers
	for (var npc in [npc_newton, npc_lara, npc_woody, npc_lisa, npc_rocky])
	{
		var dlg = Dialogue->FindByTarget(npc);
		if (dlg)
		{
			dlg->RemoveAttention();
			dlg->SetDialogueProgress(100);
		}
	}
	// Start dropping bombs immediately
	return CallNext();
}

func FxAttack_PanicStart(object c, proplist fx, int temp)
{
	if (temp) return;
	// high walk speed
	c.ActMap = { Prototype = Clonk.ActMap, Walk = { Prototype = Clonk.ActMap.Walk } };
	c.ActMap.Walk.Speed = c.ActMap.Walk.Speed * 3 / 2;
	c->SetAction("Walk");
	// drop heavy stuff
	var item = c->Contents();
	if (item && item->~IsBarrel()) item->Exit();
	// start running
	FxAttack_PanicTimer(c, fx, 0);
	return FX_OK;
}

func FxAttack_PanicTimer(object c, proplist fx, int time)
{
	// Run around in random directions
	var x = c->GetX();
	if (x < 400)
		x += 200;
	else if (x > 600)
		x -= 200;
	else
		x += Random(2)*400 - 200;
	c->SetCommand("MoveTo", nil, x, 300);
	return FX_OK;
}


func Attack_6()
{
	Attack_DropBomb(0, 0);
	Attack_DropBomb(1, 15);
	Attack_DropBomb(2, 30);
	if (this.planes[0]->GetX() < 300)
		return ScheduleNext(5);
	else
		return ScheduleSame(50);
}

func Attack_DropBomb(int plane_idx, int delay)
{
	if (delay) return ScheduleCall(this, this.Attack_DropBomb, delay, 1, plane_idx);
	var plane = this.planes[plane_idx];
	if (!plane) return;
	for (var i = 0; i<3; ++i)
	{
		var bomb = plane->CreateObjectAbove(IronBomb, 0, 12);
		if (!bomb) return;
		bomb->SetXDir(plane->GetXDir() + (i-2) * 10);
		bomb->Fuse(true); // fuse and explode on hit
	}
	plane->Sound("Goal_Raid::BombDrop");
	return true;
}

func Attack_7()
{
	for (var i = 0; i<3; ++i)
	{
		this.planes[i]->StartInstantFlight(60, 15);
	}
	return ScheduleNext(10);
}

func Attack_8()
{
	// NPCs return to center of village at regular speed
	for (var npc in [npc_newton, npc_lara, npc_lisa, npc_woody])
	{
		RemoveEffect("Attack_Panic", npc);
		npc.ActMap = Clonk.ActMap;
		npc->SetAction("Walk");
	}
	npc_newton->SetCommand("MoveTo", nil, 422, 342);
	npc_lara->SetCommand("MoveTo", nil, 455, 358);
	npc_lisa->SetCommand("MoveTo", nil, 474, 358);
	npc_woody->SetCommand("MoveTo", nil, 493, 358);
	return ScheduleNext(150);
}

func Attack_9()
{
	Attack_ClonkDirs();
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(npc_newton);
	return ScheduleNext(40);
}

func Attack_10()
{
	Attack_ClonkDirs();
	MessageBoxAll("$Attack4$", npc_newton, true);
	return ScheduleNext(150);
}

func Attack_11()
{
	Attack_ClonkDirs();
	MessageBoxAll("$Attack5$", npc_woody, true);
	return ScheduleNext(200);
}

func Attack_12()
{
	Attack_ClonkDirs();
	MessageBoxAll("$Attack6$", npc_newton, true);
	return ScheduleNext(250);
}

func Attack_13()
{
	Attack_ClonkDirs();
	MessageBoxAll("$Attack7$", this.hero, true);
	return Stop();
}

func Attack_ClonkDirs()
{
	npc_newton->SetDir(DIR_Right);
	npc_lara->SetDir(DIR_Left);
	npc_lisa->SetDir(DIR_Left);
	npc_woody->SetDir(DIR_Left);
	return true;
}

func Attack_Stop()
{
	g_attack_done = true;
	// Remove AI planes
	for (var i = 0; i<3; ++i)
	{
		if (this.planes[i]) this.planes[i]->RemoveObject();
	}
	// Newton gives the next quest
	Dialogue->FindByTarget(npc_newton)->AddAttention();
	g_goal->SetStageNewton();
	// revert to regular zoom
	SetPlayerZoomByViewRange(NO_OWNER, 400, 300, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}
