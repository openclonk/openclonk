#appendto Dialogue

/* Lara Dialogue */

func Dlg_Lara_1(object clonk)
{
	MessageBox("$Lara1$", clonk, dlg_target); // we need more wood
	return true;
}

func Dlg_Lara_2(object clonk)
{
	MessageBox("$Lara2$", clonk, clonk); // why so hasty?
	return true;
}

func Dlg_Lara_3(object clonk)
{
	MessageBox("$Lara3$", clonk, dlg_target); // cuz of them
	return true;
}

func Dlg_Lara_4(object clonk)
{
	MessageBox("$Lara4$", clonk, clonk); // cuz of who?
	return true;
}

func Dlg_Lara_5(object clonk)
{
	MessageBox("$Lara5$", clonk, dlg_target); // later. get wood plz.
	StopDialogue();
	SetDialogueProgress(6);
	return true;
}

func Dlg_Lara_6(object clonk)
{
	MessageBox("$Lara6$", clonk, dlg_target); // get wood plz.
	StopDialogue();
	SetDialogueProgress(6);
	return true;
}

// post attack dialogue
func Dlg_Lara_100(object clonk)
{
	MessageBox("$Lara100$", clonk, dlg_target); // they will never leave us in peace
	StopDialogue();
	SetDialogueProgress(100);
	return true;
}


// Generic call on every dlg message of Lara
func Dlg_Lara(clonk)
{
	// Stop walking.
	if (dlg_target->GetCommand())
	{
		
		this.was_walk_interrupted = true;
		dlg_target->SetCommand("None");
		dlg_target->SetComDir(COMD_Stop);
		dlg_target->SetXDir();
	}
	// Yield activity
	this.anim_continue_frame = FrameCounter() + 50;
	return false; // do call specific functions
}


/* NPC activity */

func Dlg_Lara_Init(object clonk)
{
	// Clonk movement speed adjustment.
	clonk.ActMap = { Prototype = Clonk.ActMap, Walk = { Prototype = Clonk.ActMap.Walk } };
	clonk.ActMap.Walk.Speed /= 2;
	clonk->SetAction("Walk");
	// Carrying stuff around
	AddEffect("LaraWalking", clonk, 1, 12, this);
  return true;
}

func FxLaraWalkingTimer(object c, proplist fx, int time)
{
	var speed_factor = 50;
	// Interrupted by dialogue?
	if (FrameCounter() < this.anim_continue_frame) return FX_OK;
	// No player joined? Stay in place for proper scenario saving.
	if (!GetPlayerCount()) return FX_OK;
	// Busy lifting/dropping?
	if (GetEffect("IntLiftHeavy", c)) return FX_OK;
	if (GetEffect("IntDropHeavy", c)) return FX_OK;
	// Do we have something to carry?
	if (!fx.carry_obj)
	{
		// Relax for a random time...
		if (!Random(10))
			// ...and search new work after
			fx.carry_obj = fx.last_barrel = Lara_FindCarryObj(c, fx);
	}
	if (fx.carry_obj)
	{
		// need to pick it up?
		if (fx.carry_obj->Contained() != c)
		{
			// need to walk there?
			var dist = Abs(fx.carry_obj->GetX() - c->GetX());
			if (dist < 10)
			{
				// pick up!
				c->SetCommand("None");
				c->SetComDir(COMD_Stop);
				// short break - then pick up.
				if (c->GetXDir()) return FX_OK;
				c->SetXDir();
				fx.carry_obj->Enter(c);
				// find a target
				fx.target_pos = [[220 + Random(11), 311], [495 + Random(51),358]][fx.carry_obj->GetX() < 350];
			}
			else 
			{
				// we're close. slow down.
				if (dist < 40) speed_factor = 30;
				// make sure we have a command
				if (!c->GetCommand()) c->SetCommand("MoveTo", fx.carry_obj);
			}
		}
		else
		{
			// object is picked up. deliver to destination
			// need to walk there?
			var dist = Abs(fx.target_pos[0] - c->GetX());
			if (dist < 10)
			{
				// drop.
				c->SetCommand("None");
				c->SetComDir(COMD_Stop);
				// short break - then drop.
				if (c->GetXDir()) return FX_OK;
				c->SetXDir();
				fx.carry_obj->Exit();
				fx.carry_obj = fx.target_pos = nil;
			}
			else 
			{
				// we're close. slow down.
				if (dist < 40) speed_factor = 30;
				// make sure we have a command
				if (!c->GetCommand()) c->SetCommand("MoveTo", nil, fx.target_pos[0], fx.target_pos[1]);
			}
		}
	}
	// Update walk speed
	c.ActMap.Walk.Speed = Clonk.ActMap.Walk.Speed * speed_factor / 100;
}

func Lara_FindCarryObj(object c, proplist fx)
{
	var last_exclude;
	if (fx.last_barrel) last_exclude = Find_Exclude(fx.last_barrel);
	var objs = Global->FindObjects(Global->Find_InRect(200, 200, 650, 180), Find_Func("IsBarrel"), last_exclude);
	return objs[Random(GetLength(objs))];
}
