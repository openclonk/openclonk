/**
	AI helper functions: Clonk
	
	Contains functions that are related to the clonk.

	@author Sven2, Maikel
*/

// Selects an item the clonk is about to use.
public func SelectItem(effect fx, object item)
{
	if (!item)
		return;
	if (item->Contained() != fx.Target)
		return;
	fx.Target->SetHandItemPos(0, fx.Target->GetItemPos(item));
}

public func CancelAiming(effect fx)
{
	if (fx.aim_weapon)
	{
		fx.aim_weapon->~ControlUseCancel(fx.Target);
		fx.aim_weapon = nil;
	}
	else
	{
		// Also cancel aiming done outside AI control.
		fx.Target->~CancelAiming();
	}
	return true;
}


public func CheckHandsAction(effect fx)
{
	// Can use hands?
	if (fx.Target->~HasHandAction())
		return true;
	// Can't throw: Is it because e.g. we're scaling?
	if (!fx.Target->HasActionProcedure())
	{
		this->ExecuteStand(fx);
		return false;
	}
	// Probably hands busy. Just wait.
	return false;
}
