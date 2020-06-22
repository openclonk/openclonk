#appendto Dialogue

/* Pyrit dialogue */

func Dlg_Pyrit_1(object clonk)
{
	var msg = "$Pyrit1$"; // almost done. oil found?
	if (!GetEffect("PyritHammering", dlg_target)) msg = "$Pyrit1B$"; // done. oil found?
	MessageBox(msg, clonk, dlg_target); 
	return true;
}

func Dlg_Pyrit_2(object clonk)
{
	if (g_got_oil)
	{
		MessageBox("$PyritQBarrel$", clonk, clonk); // ye. what now?
		this.pyrit_answer = "$PyritABarrel$"; // bring to plane
	}
	else if (g_got_gem_task)
	{
		MessageBox("$PyritQGem$", clonk, clonk); // where gem?
		this.pyrit_answer = "$PyritAGem$"; // look in caves
	}
	else
	{
		MessageBox("$PyritQSearch$", clonk, clonk); // where oil?
		this.pyrit_answer = "$PyritASearch$"; // ask around
	}
	return true;
}

func Dlg_Pyrit_3(object clonk)
{
	MessageBox(this.pyrit_answer, clonk, dlg_target);
	SetDialogueProgress(1);
	StopDialogue();
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

func Dlg_Pyrit_StartHammering(object clonk)
{
	// Hammers
	clonk->CreateContents(Hammer);
	clonk->CreateContents(Hammer);
	// Clonk moves slowly.
	clonk.ActMap = { Prototype = Clonk.ActMap, Walk = { Prototype = Clonk.ActMap.Walk } };
	clonk.ActMap.Walk.Speed /= 3;
	clonk->SetAction("Walk");
	// Hammering animation
	AddEffect("PyritHammering", clonk, 1, Pyrit_Hammer_SwingTime + 5, this);
	return true;
}

func FxPyritHammeringTimer(object c, proplist fx, int time)
{
	var fc = FrameCounter();
	if (fc < this.anim_continue_frame || c.has_sequence) return FX_OK;
	this.anim = 0;
	if (!fx.plane) if (!(fx.plane = FindObject(Find_ID(Airplane), Sort_Distance()))) return FX_OK;
	// After a while, the plane is finished. Prefer to finish while no players are nearby.
	if ((fc > 11500 && !ObjectCount(Find_ID(Clonk), Find_InRect(-300,-200, 600, 400), Find_Not(Find_Owner(NO_OWNER)))) || fc > 24000)
	{
		fx.plane->SetMeshMaterial(Airplane->GetMeshMaterial());
		fx.plane->SetR(90);
		fx.plane.MeshTransformation = Airplane.MeshTransformation;
		return FX_Execute_Kill;
	}
	if ((!Random(20)) || this.was_walk_interrupted)
	{
		// Move between two places (only if players are joined so Pyrit stays in place for object saving)
		var new_pos = [fx.plane->GetX()-24, fx.plane->GetX()+26][c->GetX() < fx.plane->GetX()];
		c->SetCommand("MoveTo", nil, new_pos, fx.plane->GetY());
		this.anim_continue_frame = FrameCounter() + 50;
		this.was_walk_interrupted = false;
	}
	else
	{
		// Ensure proper direction
		if ((c->GetDir()==DIR_Right) != (c->GetX() < fx.plane->GetX())) { c->SetDir(!c->GetDir()); return FX_OK; }
		// No movement: Swing hammer
		var anim_idx = Random(4);
		var anim_name = ["SwordSlash1.L", "SwordSlash1.R", "SwordSlash2.L", "SwordSlash2.R"][anim_idx];
		var anim_len = c->GetAnimationLength(anim_name);
		this.anim = c->PlayAnimation(anim_name, CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, anim_len, Pyrit_Hammer_SwingTime, ANIM_Remove));
		// Schedule effect when hammer hits object
		var hit_delay = [50, 50, 30, 30][anim_idx] * Pyrit_Hammer_SwingTime / 100;
		ScheduleCall(c, Dialogue.Pyrit_HitFx, hit_delay, 1);
	}
	return FX_OK;
}

func Pyrit_HitFx()
{
	var x = (GetDir()*2-1) * 14;
	var y = 4;
	CreateParticle("StarSpark", x*9/10, y*9/10, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 10);
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
