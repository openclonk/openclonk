#appendto Dialogue

/* Tarzan dialogue */

func Dlg_Tarzan_1(object clonk)
{
	// Short dialogue as it should not interfere with the NPC activity
	MessageBox("$Tarzan1$", clonk, dlg_target); // i'm tarzan
	SetDialogueProgress(1);
	return StopDialogue();
}


/* NPC animations */

func Dlg_Tarzan_Init(object clonk)
{
	// Swinging timer
	var fx = AddEffect("TarzanSwinging", clonk, 1, 7, this);
	clonk->SetPosition(750, 850);
	fx.bows = FindObjects(Find_ID(GrappleBow), Find_Container(clonk));
	fx.force_shot = true;
	return true;
}

func FxTarzanSwingingTimer(object c, proplist fx, int time)
{
	// Wait for animations, etc?
	var fc = FrameCounter();
	SetComDir(COMD_Stop);
	if (fc < fx.wait) return FX_OK;
	// only if someone is looking
	if (!fx.force_shot) if (!FindObject(Find_ID(Clonk), Find_InRect(-180,-20, 210, 90), Find_Not(Find_Owner(NO_OWNER)))) return FX_OK;
	// Time to switch grappler?
	if (fc > fx.switch_time)
	{
		fx.switch_time = 0;
		fx.curr_bow = 1 - fx.curr_bow;
	}
	// check current grappler
	fx.bow = fx.bows[fx.curr_bow];
	if (!fx.bow) return FX_Execute_Kill; // oops. lost our bow.
	var hook = fx.bow->EnsureHook();
	if (!hook) return FX_Execute_Kill; // something went terribly wrong
	if (hook->Contained() == fx.bow)
	{
		fx.wait = fc + 40;
		// The hook is not shot yet - shoot it!
		// But stay in the middle of the area
		var shoot_off = BoundBy(670 + Random(130) - GetX(), -50, 50);
		fx.bow->ControlUseStart(c, shoot_off, -50);
		fx.bow->ControlUseStop(c, shoot_off, -50);
		fx.switch_time = fc + 70 + Random(40);
		fx.force_shot = false;
		return FX_OK;
	}
	// Unstick
	if (GetProcedure() == "SCALE")
	{
		SetAction("Jump");
		if (GetDir()) SetXDir(-5); else SetXDir(5);
		SetYDir(-10);
		return FX_OK;
	}
	// Turn
	if (GetXDir() > 0) SetDir(DIR_Right); else SetDir(DIR_Left);
	// Move along rope
	var grapple_fx = GetEffect("IntGrappleControl", c);
	if (!grapple_fx) return FX_OK;
	grapple_fx.mv_down = grapple_fx.mv_up = grapple_fx.mv_left = grapple_fx.mv_right = 0;
	var want_dy = 900 - GetY();
	if (want_dy < 0)
	{
		// We sunk too low - climb up
		grapple_fx.mv_up = 1;
		fx.switch_time = Max(fx.switch_time, fc + 20); // make sure we continue climbing
	}
	else
	{
		// No need to climb? Then swing.
		if (GetXDir() < 0) grapple_fx.mv_left = 1; else grapple_fx.mv_right = 1;
	}
	return FX_OK;
}
