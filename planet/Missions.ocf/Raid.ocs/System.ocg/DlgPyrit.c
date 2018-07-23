#appendto Dialogue

/* Pyrit dialogue */

func Dlg_Pyrit_1(object clonk)
{
	SetBroadcast(true);
	MessageBox("$Pyrit1$", clonk, clonk); // u pyrit?
	return true;
}

func Dlg_Pyrit_2(object clonk)
{
	MessageBox("$Pyrit2$", clonk, dlg_target); // ye. why u here?
	return true;
}

func Dlg_Pyrit_3(object clonk)
{
	MessageBox("$Pyrit3$", clonk, clonk); // my village got pwned
	return true;
}

func Dlg_Pyrit_4(object clonk)
{
	MessageBox("$Pyrit4$", clonk, dlg_target); // ye i saw. cuz u no pay tax
	return true;
}

func Dlg_Pyrit_5(object clonk)
{
	MessageBox("$Pyrit5$", clonk, clonk); // tax is communism
	return true;
}

func Dlg_Pyrit_6(object clonk)
{
	MessageBox("$Pyrit6$", clonk, dlg_target); // we pay but harx b greedy
	return true;
}

func Dlg_Pyrit_7(object clonk)
{
	MessageBox("$Pyrit7$", clonk, clonk); // so u help me?
	return true;
}

func Dlg_Pyrit_8(object clonk)
{
	MessageBox("$Pyrit8$", clonk, dlg_target); // how u get there?
	return true;
}

func Dlg_Pyrit_9(object clonk)
{
	MessageBox("$Pyrit9$", clonk, clonk); // can we build plane?
	return true;
}

func Dlg_Pyrit_10(object clonk)
{
	MessageBox("$Pyrit10$", clonk, dlg_target); // i can give plans, but not fuel
	return true;
}

func Dlg_Pyrit_11(object clonk)
{
	MessageBox("$Pyrit11$", clonk, clonk); // for gud reason
	return true;
}

func Dlg_Pyrit_12(object clonk)
{
	MessageBox("$Pyrit12$", clonk, dlg_target); // only way 2 get there
	return true;
}

func Dlg_Pyrit_13(object clonk)
{
	MessageBox("$Pyrit13$", clonk, dlg_target); // some ppl hide oil
	return true;
}

func Dlg_Pyrit_14(object clonk)
{
	MessageBox("$Pyrit14$", clonk, dlg_target); // ask around 4 oil
	// 2nd time in dialogue omit last message
	if (g_pyrit_spoken)
	{
		StopDialogue();
		SetDialogueProgress(16);
	}
	return true;
}

func Dlg_Pyrit_15(object clonk)
{
	MessageBox("$Pyrit15$", clonk, dlg_target); // take con plans for plane
	g_pyrit_spoken = true;
	g_goal->SetStagePlane();
	AddTimer(this.CheckOilAtPlane, 10);
	SetBroadcast(false);
	StopDialogue();
	SetDialogueProgress(16);
	SetPlrKnowledge(NO_OWNER, Airplane);
	// many NPCs get new texts now
	Dialogue->FindByTarget(npc_newton)->SetDialogueProgress(200, nil, true);
	Dialogue->FindByTarget(npc_mave)->SetDialogueProgress(100, nil, true);
	Dialogue->FindByTarget(npc_clonko)->SetDialogueProgress(100, nil, true);
	Dialogue->FindByTarget(npc_dora)->AddAttention(); // in case player spoke to Dora before speaking to Pyrit...
	return true;
}

// called every 10 frames after plane+oil task has been given
func CheckOilAtPlane()
{
	for (var plane in FindObjects(Find_ID(Airplane))) 
	{
		var barrel = plane->FindObject(plane->Find_AtRect(-30,-10,60,20), Find_ID(MetalBarrel));
		if (barrel)
		{
			RemoveTimer(Scenario.CheckOilAtPlane);
			ScheduleCall(nil, Global.GameCall, 1,1, "OnPlaneLoaded", plane, barrel);
		}
	}
	return true;
}

func Dlg_Pyrit_16(object clonk)
{
	// ask how to build plane unless it has been built
	if (g_plane_built) return Dlg_Pyrit_18(clonk);
	MessageBox("$Pyrit16$", clonk, clonk); // where i build plane?
	return true;
}

func Dlg_Pyrit_17(object clonk)
{
	MessageBox("$Pyrit17$", clonk, dlg_target); // in shipyard
	return true;
}

func Dlg_Pyrit_18(object clonk)
{
	// ask for oil unless we got the oil location from Mave already
	if (g_mave_oil_spoken) return Dlg_Pyrit_19(clonk);
	MessageBox("$Pyrit18$", clonk, clonk); // where oil?
	SetDialogueProgress(13);
	return true;
}

func Dlg_Pyrit_19(object clonk)
{
	// after talking to Mave, we need a gold bar. ask where to find.
	MessageBox("$Pyrit19$", clonk, clonk); // where gold?
	SetDialogueProgress(20);
	return true;
}

func Dlg_Pyrit_20(object clonk)
{
	MessageBox("$Pyrit20$", clonk, dlg_target); // gold underground
	StopDialogue();
	SetDialogueProgress(16);
	return true;
}




// Generic call on every dlg message of Pyrit
func Dlg_Pyrit(object clonk)
{
	// Stop walking.
	if (dlg_target->GetCommand())
	{
		
		this.was_walk_interrupted = true;
		dlg_target->SetCommand("None");
		dlg_target->SetComDir(COMD_Stop);
		dlg_target->SetXDir();
	}
	// Yield animation
	if (this.anim) dlg_target->StopAnimation(this.anim);
	this.anim = 0;
	this.anim_continue_frame = FrameCounter() + 50;
	return false; // do call specific functions
}


/* NPC animations */

static const Pyrit_Hammer_SwingTime = 40;

func Dlg_Pyrit_Init(object clonk)
{
	// Clonk moves slowly.
	clonk.ActMap = { Prototype = Clonk.ActMap, Walk = { Prototype = Clonk.ActMap.Walk } };
	clonk.ActMap.Walk.Speed /= 3;
	clonk->SetAction("Walk");
	// Hammering animation
	AddEffect("PyritHammering", clonk, 1, Pyrit_Hammer_SwingTime+5, this);
	return true;
}

func FxPyritHammeringTimer(object c, proplist fx, int time)
{
	if (!fx.hat)
	{
		// Pyit has a red hat!
		fx.hat = c->AttachMesh(Hat, "skeleton_head", "main", Trans_Translate(5500, 0, 0));
	}
	if (FrameCounter() < this.anim_continue_frame || c.has_sequence) return FX_OK;
	this.anim = 0;
	if (!fx.catapult) if (!(fx.catapult = c->FindObject(Find_ID(Catapult), Sort_Distance()))) return FX_OK;
	if ((!Random(20) && GetPlayerCount()) || this.was_walk_interrupted)
	{
		// Move between two places (only if players are joined so Pyrit stays in place for object saving)
		var new_pos = [fx.catapult->GetX()-24, fx.catapult->GetX()+26][c->GetX() < fx.catapult->GetX()];
		c->SetCommand("MoveTo", nil, new_pos, fx.catapult->GetY());
		this.anim_continue_frame = FrameCounter() + 50;
		this.was_walk_interrupted = false;
	}
	else
	{
		// Ensure proper direction
		if ((c->GetDir()==DIR_Right) != (c->GetX() < fx.catapult->GetX())) { c->SetDir(!c->GetDir()); return FX_OK; }
		// No movement: Swing hammer
		var anim_idx = Random(2);
		var anim_name = ["SwordSlash1.R", "SwordSlash2.R"][anim_idx];
		var anim_len = c->GetAnimationLength(anim_name);
		this.anim = c->PlayAnimation(anim_name, CLONK_ANIM_SLOT_Arms, Anim_Linear(0,0,anim_len, Pyrit_Hammer_SwingTime, ANIM_Remove));
		// Schedule effect when hammer hits object
		var hit_delay = [50,50,30,30][anim_idx] * Pyrit_Hammer_SwingTime / 100;
		ScheduleCall(c, Dialogue.Pyrit_HitFx, hit_delay, 1);
	}
	return FX_OK;
}

func Pyrit_HitFx()
{
	var x = (GetDir()*2-1) * 14;
	var y = 4;
	CreateParticle("StarSpark", x*9/10,y*9/10, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 10);
	Sound("Objects::Pickaxe::Clang?");
	return true;
}

func FxPyritHammeringStop(object c, proplist fx, int reason, bool temp)
{
	if (!temp && this.anim)
	{
		c->StopAnimation(this.anim);
		this.anim = 0;
	}
	return FX_OK;
}
