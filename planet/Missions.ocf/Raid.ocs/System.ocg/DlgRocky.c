#appendto Dialogue

/* Rocky dialogue */

func Dlg_Rocky_1(object clonk)
{
	if (clonk->FindContents(Axe))
	{
		MessageBox("$Rocky3$", clonk, clonk); // he's dnd
		StopDialogue();
		SetDialogueProgress(1);
	}
	else
	{
		MessageBox("$Rocky1$", clonk, clonk); // u got axe?
	}
	return true;
}

func Dlg_Rocky_2(object clonk)
{
	MessageBox("$Rocky2$", clonk, dlg_target); // check lorry
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}


// post attack dialogue
func Dlg_Rocky_100(object clonk)
{
	MessageBox("$Rocky100$", clonk, clonk); // u can stop
	return true;
}

func Dlg_Rocky_101(object clonk)
{
	MessageBox("$Rocky101$", clonk, dlg_target); // stfu. must work.
	StopDialogue();
	SetDialogueProgress(101);
	return true;
}

// Generic call on every dlg message of Rocky
func Dlg_Rocky(object clonk)
{
	// Only if Clonk is actually talking
	if (clonk->FindContents(Axe)) return false;
	// Yield animation
	if (this.anim) dlg_target->StopAnimation(this.anim);
	this.anim = 0;
	this.anim_continue_frame = FrameCounter() + 50;
	return false; // do call specific functions
}


/* NPC animations */

static const Rocky_Pickaxe_SwingTime = 60;

func Dlg_Rocky_Init(object clonk)
{
	// Big pickaxe!
	var pickaxe = clonk->FindContents(Pickaxe);
	if (!pickaxe) pickaxe = clonk->CreateContents(Pickaxe);
	pickaxe.GetCarryTransform = Dialogue.Inventory_GetCarryTransform; // defined in Newton's dialogue
	var h_scale = 2000;
	pickaxe.ExtraTransform = Trans_Scale(h_scale, h_scale, h_scale);
	clonk.pickaxe_particle = new Particles_Glimmer() { Size = PV_Linear(5, 0) };
	// Pickaxeing animation
	AddEffect("RockyPickaxeing", clonk, 1, Rocky_Pickaxe_SwingTime, this);
	return true;
}

func FxRockyPickaxeingTimer(object c, proplist fx, int time)
{
	if (FrameCounter() < this.anim_continue_frame) { fx.phase = false; return FX_OK; }
	c->SetDir(DIR_Right);
	var len = c->GetAnimationLength("StrikePickaxe");
	this.anim = c->PlayAnimation("StrikePickaxe", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, len, Rocky_Pickaxe_SwingTime, ANIM_Remove));
	c->Sound("Objects::Pickaxe::Clang?");
	var x = (c->GetDir()*2-1) * 9;
	var y = 9;
	c->CreateParticle("StarSpark", x, y, PV_Random(-20, 20), PV_Random(-20, 20), 20, c.pickaxe_particle, Random(10)+3);
	return FX_OK;
}
