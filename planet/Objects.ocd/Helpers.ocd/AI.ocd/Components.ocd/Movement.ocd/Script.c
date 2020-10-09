/**
	AI Movement
	Functionality that helps the AI move.
	
	@author Sven2, Maikel
*/


/*-- Public interface --*/

// Set attack path
public func SetAttackPath(object clonk, array new_attack_path)
{
	AssertDefinitionContext(Format("SetAttackPath(%v, %v)", clonk, new_attack_path));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	fx_ai.attack_path = new_attack_path;
	return true;
}


/*-- Callbacks --*/

// Callback from the effect SaveScen()-call
public func OnSaveScenarioAI(proplist fx_ai, proplist props)
{
	_inherited(fx_ai, props);

	if (fx_ai.attack_path)
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetAttackPath", fx_ai.Target, fx_ai.attack_path);

}


/*-- Editor Properties --*/

// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);
	
	def->GetControlEffect().SetAttackPath = this.EditorDelegate_SetAttackPath;
	
	// Set the additional editor properties
	var additional_props =
	{
		attack_path = { Name = "$AttackPath$", EditorHelp = "$AttackPathHelp$", Type = "enum", Set = "SetAttackPath", Options = [
			{ Name="$None$" },
			{ Name="$AttackPath$", Type = C4V_Array, Value = [{X = 0, Y = 0}], Delegate =
				{ Name="$AttackPath$", EditorHelp="$AttackPathHelp$", Type="polyline", StartFromObject = true, DrawArrows = true, Color = 0xdf0000, Relative = false }
			}
		] },
	};
	
	AddProperties(def->GetControlEffect().EditorProps, additional_props);
}


func EditorDelegate_SetAttackPath(array attack_path)
{
	// Called by editor delegate when attack mode is changed.
	// For now, attack mode parameter delegates are not supported. Just set by name.
	return this->GetControl()->SetAttackPath(this.Target, attack_path);
}


/*-- Internals --*/

// Tries to make sure the clonk stands: i.e. scales down or let's go when hangling.
public func ExecuteStand(effect fx)
{
	fx.Target->SetCommand("None");
	if (fx.Target->GetProcedure() == "SCALE")
	{
		var tx;
		if (fx.target)
			tx = fx.target->GetX() - fx.Target->GetX();
		// Scale: Either scale up if target is beyond this obstacle or let go if it's not.
		if (fx.Target->GetDir() == DIR_Left)
		{
			if (tx < -20)
				fx.Target->SetComDir(COMD_Left);
			else
				fx.Target->ObjectControlMovement(fx.Target->GetOwner(), CON_Right, 100); // let go
		}
		else
		{
			if (tx > -20)
				fx.Target->SetComDir(COMD_Right);
			else
				fx.Target->ObjectControlMovement(fx.Target->GetOwner(), CON_Left, 100); // let go
		}
	}
	else if (fx.Target->GetAction() == "Climb")
	{
		var climb_fx = GetEffect("IntClimbControl", fx.Target);
		if (climb_fx)
		{
			// For now just climb down the ladder.
			var ctrl = CON_Down;		
			EffectCall(fx.Target, climb_fx, "Control", ctrl, 0, 0, 100, false, CONS_Down);
		}	
	}
	else if (fx.Target->GetProcedure() == "HANGLE")
	{
		fx.Target->ObjectControlMovement(fx.Target->GetOwner(), CON_Down, 100);
	}
	else if (fx.Target->GetProcedure() == "FLIGHT" || fx.Target->GetAction() == "Roll")
	{
		// Don't do anything for these procedures and actions as they will end automatically.
	}
	else
	{		
		this->~LogAI_Warning(fx, Format("ExecuteStand has no idea what to do for action %v and procedure %v.", fx.Target->GetAction(), fx.Target->GetProcedure()));
		// Hm. What could it be? Let's just hope it resolves itself somehow...
		fx.Target->SetComDir(COMD_Stop);
	}
	return true;
}

// Evade a threat from the given coordinates.
public func ExecuteEvade(effect fx, int threat_dx, int threat_dy)
{
	// Don't try to evade if the AI has a commander, if an AI is being commanded
	// it has more important tasks, like staying on an airship.
	if (fx.commander)
		return false;
	// Evade from threat at position delta threat_dx, threat_dy.
	if (threat_dx < 0)
		fx.Target->SetComDir(COMD_Left);
	else
		fx.Target->SetComDir(COMD_Right);
	if (threat_dy >= -5 && !Random(2))
		if (this->ExecuteJump(fx))
			return true;
	// Shield? Todo.
	return true;
}

public func ExecuteJump(effect fx)
{
	// Jump if standing on floor.
	if (fx.Target->GetProcedure() == "WALK")
	{
		if (fx.Target->~ControlJump())
			return true; // For clonks.
		return fx.Target->Jump(); // For others.
	}
	return false;
}

// Follow attack path or return to the AI's home if not yet there.
public func ExecuteIdle(effect fx)
{
	// Persist commands because constant command resets may hinder execution
	if (fx.Target->GetCommand() && Random(4)) return true;
	// Follow attack path
	if (this->ExecuteAttackPath(fx)) return true;
	// Movement done (for now)
	fx.Target->SetCommand("None");
	fx.Target->SetComDir(COMD_Stop);
	fx.Target->SetDir(fx.home_dir);
	if (fx.vehicle) fx.vehicle->SetCommand();
	// Nothing to do.
	return false;
}

public func ExecuteAttackPath(effect fx)
{
	// Attack path is to follow the commander.
	if (fx.commander) return true;
	if (fx.attack_path)
	{
		// Follow attack path
		var next_pt = fx.attack_path[0];
		// Check for structure to kill on path. Only if the structure is alive or of the clonk can attack structures with the current weapon.
		var alive_check;
		if (!fx.can_attack_structures && !fx.can_attack_structures_after_weapon_respawn)
		{
			alive_check = Find_OCF(OCF_Alive);
		}
		fx.target = FindObject(Find_AtPoint(next_pt.X, next_pt.Y), Find_Func("IsStructure"), alive_check);
		if (fx.target)
		{
			// Do not advance on path unless target(s) destroyed.
			return true;
		}
		// Follow path
		fx.home_x = next_pt.X;
		fx.home_y = next_pt.Y;
		fx.home_dir = Random(2);
	}
	// Check if we need to move/push to a target
	if (fx.vehicle)
	{
		if (!Inside(fx.vehicle->GetX() - fx.home_x, -15, 15) || !Inside(fx.vehicle->GetY() - fx.home_y, -20, 20))
		{
			if (fx.vehicle->~IsAirship())
			{
				if (!fx.vehicle->GetCommand())
				{
					fx.vehicle->SetCommand("MoveTo", nil, fx.home_x, fx.home_y);
				}
			}
			else
			{
				// Default vehicle movement
				return fx.Target->SetCommand("PushTo", fx.vehicle, fx.home_x, fx.home_y);
			}
			return true;
		}
	}
	else
	{
		if (!Inside(fx.Target->GetX() - fx.home_x, -5, 5) || !Inside(fx.Target->GetY() - fx.home_y, -15, 15))
		{
			return fx.Target->SetCommand("MoveTo", nil, fx.home_x, fx.home_y);
		}
	}
	// Next section on path or done?
	this->AdvanceAttackPath(fx);
	return false;
}

public func AdvanceAttackPath(effect fx)
{
	// Pick next element in attack path if an attack path is set. Return whether a path remains.
	if (fx.attack_path)
	{
		if (GetLength(fx.attack_path) > 1)
		{
			fx.attack_path = fx.attack_path[1:];
			return true;
		}
		else
		{
			fx.attack_path = nil;
		}
	}
	return false;
}

// Turns around the AI such that it looks at its target.
public func ExecuteLookAtTarget(effect fx)
{
	// Set direction to look at target, we can assume this is instantaneous.
	if (fx.target->GetX() > fx.Target->GetX())
		fx.Target->SetDir(DIR_Right);
	else
		fx.Target->SetDir(DIR_Left);
	return true;
}
