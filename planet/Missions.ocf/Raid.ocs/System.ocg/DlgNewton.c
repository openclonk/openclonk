#appendto Dialogue

/* Newton dialogue */

func Dlg_Newton_1(object clonk)
{
	MessageBox(Format("$Newton1$", clonk->GetName()), clonk, dlg_target); // %s, good that you're here!
	return true;
}

func Dlg_Newton_2(object clonk)
{
	MessageBox("$Newton2$", clonk, dlg_target); // u done chopping wood?
	return true;
}

func Dlg_Newton_3(object clonk)
{
	if (clonk->FindContents(Axe))
	{
		MessageBox("$Newton5$", clonk, clonk); // nope. where r trees?
		SetDialogueProgress(5);
	}
	else
	{
		MessageBox("$Newton3$", clonk, clonk); // nope. where is axe?
	}
	return true;
}

func Dlg_Newton_4(object clonk)
{
	MessageBox("$Newton4$", clonk, dlg_target); // look west @ mine
	StopDialogue();
	SetDialogueProgress(2);
	return true;
}

func Dlg_Newton_5(object clonk)
{
	MessageBox("$Newton6$", clonk, dlg_target); // look east @ forest
	StopDialogue();
	SetDialogueProgress(2);
	return true;
}


/* Dialogue after attack */

func Dlg_Newton_100(object clonk)
{
	SetBroadcast(true);
	MessageBox(Format("$Newton100$", clonk->GetName()), clonk, dlg_target); // %s, did you survive?
	return true;
}

func Dlg_Newton_101(object clonk)
{
	MessageBox("$Newton101$", clonk, clonk); // yes, was in forest
	return true;
}

func Dlg_Newton_102(object clonk)
{
	MessageBox("$Newton102$", clonk, dlg_target); // ...
	return true;
}

func Dlg_Newton_103(object clonk)
{
	MessageBox("$Newton103$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_104(object clonk)
{
	MessageBox("$Newton104$", clonk, dlg_target); // 
	return true;
}

func Dlg_Newton_105(object clonk)
{
	MessageBox("$Newton105$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_106(object clonk)
{
	MessageBox("$Newton106$", clonk, dlg_target); // 
	return true;
}

func Dlg_Newton_107(object clonk)
{
	MessageBox("$Newton107$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_108(object clonk)
{
	MessageBox("$Newton108$", clonk, dlg_target); // 
	return true;
}

func Dlg_Newton_109(object clonk)
{
	MessageBox("$Newton109$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_110(object clonk)
{
	MessageBox("$Newton110$", clonk, dlg_target); // 
	return true;
}

func Dlg_Newton_111(object clonk)
{
	MessageBox("$Newton111$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_112(object clonk)
{
	MessageBox("$Newton112$", clonk, dlg_target); // 
	return true;
}

func Dlg_Newton_113(object clonk)
{
	MessageBox("$Newton113$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_114(object clonk)
{
	MessageBox("$Newton114$", clonk, clonk); // 
	return true;
}

func Dlg_Newton_115(object clonk)
{
	MessageBox("$Newton115$", clonk, dlg_target); // 
	return true;
}

func Dlg_Newton_116(object clonk)
{
	// option to speaker. rest just gets the message
	MessageBox("$Newton116$", clonk, dlg_target, nil, false, ["$Newton117$", "$Newton117$"]); // "will you help us" - yes/yes
	return true;
}

func Dlg_Newton_117(object clonk)
{
	MessageBox("$Newton117$", clonk, clonk); // yes
	g_challenge_accepted = true;
	// challenge accepted - update some dialogues
	Dialogue->FindByTarget(npc_lisa)->SetDialogueProgress(200);
	Dialogue->FindByTarget(npc_mave)->AddAttention();
	return true;
}

func Dlg_Newton_118(object clonk)
{
	MessageBox("$Newton118$ $Newton119$", clonk, dlg_target); // excellent! go east, take shovel
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew = GetCrew(plr);
		if (crew) crew->CreateContents(Shovel);
	}
	SetDialogueProgress(120);
	return true;
}

func Dlg_Newton_119(object clonk)
{
	MessageBox("$Newton119$", clonk, dlg_target); // go east, take shovel
	return true;
}

func Dlg_Newton_120(object clonk)
{
	MessageBox("$Newton120$", clonk, dlg_target); // next village, ask for Pyrit
	return true;
}

func Dlg_Newton_121(object clonk)
{
	MessageBox("$Newton121$", clonk, dlg_target); // gl
	g_goal->SetStagePyrit();
	SetBroadcast(false);
	StopDialogue();
	SetDialogueProgress(119);
	return true;
}


/* Dialogue after speaking to Pyrit */

func Dlg_Newton_200(object clonk)
{
	if (g_plane_built)
		MessageBox("$Newton200$", clonk, clonk); // got plane, need fuel
	else
		MessageBox("$Newton200a$", clonk, clonk); // got plane plans, need fuel
	return true;
}

func Dlg_Newton_201(object clonk)
{
	MessageBox(Format("$Newton201$", clonk->GetName()), clonk, dlg_target); // no oil
	return true;
}

func Dlg_Newton_202(object clonk)
{
	MessageBox("$Newton202$", clonk, dlg_target); // all sold
	return true;
}

func Dlg_Newton_203(object clonk)
{
	MessageBox("$Newton203$", clonk, dlg_target); // ask mave
	StopDialogue();
	SetDialogueProgress(200);
	return true;
}

// Generic call on every dlg message of Newton
func Dlg_Newton(object clonk)
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

static const Newton_Hammer_SwingTime = 30;

func Dlg_Newton_Init(object clonk)
{
	// Big hammer!
	var hammer = clonk->FindContents(Hammer);
	if (!hammer) hammer = clonk->CreateContents(Hammer);
	hammer.GetCarryTransform = Dialogue.Inventory_GetCarryTransform;
	var h_scale = 2000;
	hammer.ExtraTransform = Trans_Scale(h_scale, h_scale, h_scale);
	// Hammer is heavy. Clonk moves slowly.
	clonk.ActMap = { Prototype = Clonk.ActMap, Walk = { Prototype = Clonk.ActMap.Walk } };
	clonk.ActMap.Walk.Speed /= 3;
	clonk->SetAction("Walk");
	// Hammering animation
	AddEffect("NewtonHammering", clonk, 1, Newton_Hammer_SwingTime, this);
	return true;
}

func FxNewtonHammeringTimer(object c, proplist fx, int time)
{
	if (FrameCounter() < this.anim_continue_frame || c.has_sequence) { fx.phase = false; return FX_OK; }
	var len = c->GetAnimationLength("StrikePickaxe");
	var a = len*70/100;
	var b = len*94/100;
	fx.phase = !fx.phase;
	if (fx.phase)
	{
		if ((!Random(5) && GetPlayerCount()) || this.was_walk_interrupted)
		{
			// Move between two places (only if players are joined so Newton stays in place for object saving)
			var new_pos = [[226, 312], [258, 314]][GetX() < 242];
			c->SetCommand("MoveTo", nil, new_pos[0], new_pos[1]);
			this.anim_continue_frame = FrameCounter() + 50;
			this.was_walk_interrupted = false;
			this.anim = 0;
		}
		else
		{
			// No movement: Swing hammer
			this.anim = c->PlayAnimation("StrikePickaxe", CLONK_ANIM_SLOT_Arms, Anim_Linear(a, a, b, Newton_Hammer_SwingTime, ANIM_Remove));
		}
	}
	else
	{
		// Hammer backswing
		this.anim = c->PlayAnimation("StrikePickaxe", CLONK_ANIM_SLOT_Arms, Anim_Linear(b, b, a, Newton_Hammer_SwingTime, ANIM_Remove));
		c->Sound("Objects::Pickaxe::Clang?");
		var x = (c->GetDir()*2-1) * 9;
		var y = -16;
		c->CreateParticle("Dust", x, y, PV_Random(-10, 10), PV_Random(-10, 20), PV_Random(10, 20), new Particles_Dust() { R = 120, G = 100, B = 80 }, 10);
		if (Random(3)) c->CreateParticle("StarSpark", x, y, PV_Random(-5, 5), PV_Random(-5, 5), PV_Random(10, 20), Particles_Glimmer(), Random(10)+3);
	}
	return FX_OK;
}

func FxNewtonHammeringStop(object c, proplist fx, int reason, bool temp)
{
	if (!temp && this.anim)
	{
		c->StopAnimation(this.anim);
		this.anim = 0;
	}
	return FX_OK;
}

// Helper function appended to enlarged objects
func Inventory_GetCarryTransform()
{
	if (GetID().GetCarryTransform)
		return Trans_Mul(Call(GetID().GetCarryTransform, ...), this.ExtraTransform);
	else
		return this.ExtraTransform;
}

