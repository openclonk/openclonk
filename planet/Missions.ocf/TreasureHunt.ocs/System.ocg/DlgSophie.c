#appendto Dialogue

/* Sophie1 = Teeheehe. Look how it levitates!
Sophie2 = Are you a magician?
Sophie3 = Nope, it's tele gloves. I've found them in the chest beside me.
Sophie4 = There's more of them! Just try them out. Press and hold near a lose item to move it.
SophieSad = Why did you have to take away my playing items? Sophie sad :(
*/

/* Sophie dialogue */

func Dlg_Sophie_1(object clonk)
{
	if (!GetEffect("SophieTeleCheck", dlg_target) || !Sophie_FindBone())
	{
		MessageBox("$SophieSad$", clonk, dlg_target);  // sophie sad
		SetDialogueProgress(1);
		StopDialogue();
	}
	else
	{
		MessageBox("$Sophie1$", clonk, dlg_target); // look tele!
	}
	return true;
}

func Dlg_Sophie_2(object clonk)
{
	MessageBox("$Sophie2$", clonk, clonk); // magic?
	return true;
}

func Dlg_Sophie_3(object clonk)
{
	MessageBox("$Sophie3$", clonk, dlg_target); // nope tele
	return true;
}

func Dlg_Sophie_4(object clonk)
{
	MessageBox("$Sophie4$", clonk, dlg_target); // get 1 from the box!
	SetDialogueProgress(1);
	StopDialogue();
	return true;
}


// Generic call on every dlg message of Sophie
func Dlg_Sophie(object clonk)
{
	// Yield pickup of new items; stop item movement
	this.anim_continue_frame = FrameCounter() + 50;
	return false; // do call specific functions
}


/* NPC animations */

func Dlg_Sophie_Init(object clonk)
{
	// Check timer to see if we can use tele glove
	var fx = AddEffect("SophieTeleCheck", clonk, 1, 60, this);
	fx.glove = clonk->FindContents(TeleGlove);
	fx.wait_time = 100;
	return true;
}

func Sophie_FindBone()
{
	// find bone to fling around with tele glove
	return FindObject(Find_ID(Bone), Find_InRect(-80,-80, 160, 160), Find_OCF(OCF_InFree), Find_NoContainer(), Sort_Distance());
}

func FxSophieTeleCheckTimer(object c, proplist fx, int time)
{
	if (!FindObject(Find_ID(Clonk), Find_InRect(-150,-50, 240, 150), Find_Exclude(c))) return FX_OK; // only if people are watching
	var fc = FrameCounter();
	if (fc < this.anim_continue_frame) return FX_OK;
	if (GetEffect("SophieTeleUse", c)) return FX_OK;
	var bone = Sophie_FindBone();
	if (!bone) return FX_OK;
	if (!fx.glove) return FX_Execute_Kill;
	if (Random(3) >= fx.wait_time++) return FX_OK; // just after using it, take a short break
	AddEffect("SophieTeleUse", c, 1, 3, this, nil, fx.glove, bone);
	fx.wait_time = 0;
	return FX_OK;
}

func FxSophieTeleUseStart(object c, proplist fx, int temp, object glove, object bone)
{
	if (temp) return FX_OK;
	fx.glove = glove;
	fx.bone = bone;
	fx.x = fx.bone->GetX()-GetX();
	fx.y = fx.bone->GetY()-GetY() - 5;
	fx.tx = fx.x;
	fx.ty = fx.y;
	fx.glove->ControlUseStart(c, fx.x, fx.y);
	return FX_OK;
}

func FxSophieTeleUseTimer(object c, proplist fx, int time)
{
	if (!fx.glove || !fx.bone) return FX_Execute_Kill;
	if (Distance(GetX()+fx.x, GetY()+fx.y, fx.bone->GetX(), fx.bone->GetY()) > 30) return FX_Execute_Kill; // oops - bone dropped
	if (fx.x>0) c->SetDir(DIR_Right); else c->SetDir(DIR_Left);
	var fc = FrameCounter();
	if (fx.bored)
	{
		// Got bored of it? Wait for a while to stabilize. Then drop item.
		if (fc > fx.bored) return FX_Execute_Kill;
	}
	else if (time > 20 && fc >= this.anim_continue_frame) // Wait a bit in the beginning. Wait while talking.
	{
		if (!Random(30) && Abs(fx.x)>15) fx.bored = fc + 30;
		// Move to random locations
		if (fx.tx == fx.x && fx.ty == fx.y)
		{
			fx.tx = Random(91)-45;
			fx.ty = Random(50)-50;
		}
		fx.x += BoundBy(fx.tx-fx.x, -3, 3);
		fx.y += BoundBy(fx.ty-fx.y, -3, 3);
	}
	// Keep holding that bone
	fx.glove->ControlUseHolding(c, fx.x, fx.y);
	return FX_OK;
}

func FxSophieTeleUseStop(object c, proplist fx, int reason, bool temp)
{
	if (c && fx.glove) fx.glove->ControlUseStop(c, fx.x, fx.y);
	return FX_OK;
}
