/**
	AI Appearance
	Functionality that helps the AI to display messages, make sounds, etc.
	
	@author Maikel
*/


// AI Settings.
local AttackMessageWaitTime = 36 * 30; // Number of frames between displaying an attack message.
local AttackMessageRate = 80; // Likelihood of displaying an attack message in percent (0 - 100).


public func ExecuteAppearance(effect fx)
{
	// Do a random attack message.
	if (!Random(5) && Random(100) >= 100 - fx.control.AttackMessageRate)
		this->ExecuteAttackMessage(fx);
	return true;
}

// Shows an attack message if in a group of AI and is attacking.
public func ExecuteAttackMessage(effect fx)
{
	// Only if close to the target.
	if (fx.Target->ObjectDistance(fx.target) > 150)
		return true;
	// Find other AI enemies that attack together.
	var group_cnt = 0;
	for (var clonk in fx.Target->FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(40), Find_Owner(fx.Target->GetOwner())))
	{
		var ai = clonk->~GetAI();
		if (!ai)
			continue;
		if (ai.last_message != nil && FrameCounter() - ai.last_message < fx.control.AttackMessageWaitTime)
			continue;
		group_cnt++;
	}
	// Display a message if group is big enough.
	if (group_cnt >= RandomX(3, 6))
	{
		fx.Target->Sound("Clonk::Action::GroupAttack");
		fx.Target->Message(Translate(Format("MsgAttack%d", Random(5))));
		fx.last_message = FrameCounter();
	}
	return true;
}

// Shows a message when an intruder is spotted.
public func ExecuteIntruderMessage(effect fx)
{
	fx.Target->Message(Translate(Format("MsgIntruder%d", Random(3))));
	return true;
}
