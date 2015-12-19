// A pilot which tells about the sky islands.

#appendto Dialogue

public func Dlg_Pilot_Init(object clonk)
{
	AddEffect("IntPilot", clonk, 100, 5, this);
	return true;
}

public func Dlg_Pilot_1(object clonk)
{
	MessageBox(Format("$DlgPilotHello$", dlg_target->GetName()), clonk, dlg_target);
	return true;
}

public func Dlg_Pilot_2(object clonk)
{
	MessageBox("$DlgPilotReply$", clonk, clonk);
	return true;
}

public func Dlg_Pilot_3(object clonk)
{
	MessageBox("$DlgPilotBats$", clonk, dlg_target);
	return true;
}

public func Dlg_Pilot_4(object clonk)
{
	MessageBox("$DlgPilotAgainstBats$", clonk, clonk);
	return true;
}

public func Dlg_Pilot_5(object clonk)
{
	MessageBox("$DlgPilotBowArrow$", clonk, dlg_target);
	return true;
}

public func Dlg_Pilot_6(object clonk)
{
	MessageBox("$DlgPilotHaveBow$", clonk, clonk);
	return true;
}

public func Dlg_Pilot_7(object clonk)
{
	MessageBox("$DlgPilotArmory$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Pilot_Closed(object clonk)
{
	GameCall("OnHasTalkedToFireman", clonk);
	return true;
}

public func FxIntPilotStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	// Give the pilot a teleglove.
	effect.glove = target->CreateContents(TeleGlove);
	effect.glove->SetObjectLayer();
	return FX_OK;
}

public func FxIntPilotTimer(object target, proplist effect, int time)
{
	var lorry = target->FindObject(Find_ID(Lorry),  Find_AnyLayer(), Find_Distance(100));
	// Check if a gem comes near and then catch it with the teleglove.
	for (var gem in target->FindObjects(Find_ID(Ruby), Find_Distance(effect.glove->GetTeleGloveReach() - 15), Find_NoContainer(), Find_AnyLayer(), Find_Not(Find_Property("has_been_telegloved")), Sort_Distance()))
	{
		var to_x = lorry->GetX();
		var to_y = lorry->GetY();
		// Only if the gem is not stuck and the path to the lorry is free.
		if (!PathFree(gem->GetX(), gem->GetY(), to_x, to_y) || gem->Stuck())
			continue;
		// Only if the gem is not controlled by another glove.
		if (GetEffect("TeleGloveWeight", gem))		
			continue;
		if (!GetEffect("IntControlGem", target))
			AddEffect("IntControlGem", target, 100, 1, this, nil, gem, effect.glove);
		break;
	}
	return FX_OK;
}

public func FxIntControlGemStart(object target, proplist effect, int temp, object gem, object glove)
{
	if (temp)
		return FX_OK;
	effect.gem = gem;
	effect.glove = glove;
	effect.startx = gem->GetX();
	effect.starty = gem->GetY();
	// Determine time it takes to move the gem.
	var lorry = target->FindObject(Find_ID(Lorry), Find_AnyLayer(), Find_Distance(100));
	effect.move_time = 2 * Distance(effect.startx, effect.starty, lorry->GetX(), lorry->GetY()) / 3;
	// Start using the teleglove.
	effect.glove->ControlUseStart(target, effect.gem->GetX() - target->GetX(), effect.gem->GetY() - target->GetY());
	// Play a confirm sound and show a short message.
	target->PlaySoundConfirm();
	target->Message(Translate(Format("DlgPilotGem%d", Random(3))));
	return FX_OK;
}

public func FxIntControlGemTimer(object target, proplist effect, int time)
{
	var lorry = target->FindObject(Find_ID(Lorry), Find_AnyLayer(), Find_Distance(100));
	if (!lorry || time > effect.move_time)
		return FX_Execute_Kill;
	var ox = effect.startx;
	var oy = effect.starty;
	var tx = lorry->GetX();
	var ty = lorry->GetY() - 12;
	var new_x = ox + time * (tx - ox) / (effect.move_time);
	var new_y = oy + time * (ty - oy) / (effect.move_time);
	effect.glove->ControlUseHolding(target, new_x - target->GetX(), new_y - target->GetY());
	return FX_OK;
}

public func FxIntControlGemStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	var lorry = target->FindObject(Find_ID(Lorry), Find_AnyLayer(), Find_Distance(100));
	if (lorry && ObjectDistance(lorry, effect.gem) < 24)
		effect.gem.has_been_telegloved = true;
	effect.glove->ControlUseStop(target);
	return FX_OK;
}
