/*--
		Object restorer
		Author: Maikel

		Restores an object and transports it back to its original position or container,
		can be used to prevent players from messing up tutorials by wasting items.
--*/


/*-- Object Restoration --*/

protected func Initialize()
{
	return;
}

public func SetRestoreObject(object to_restore, object to_container, int to_x, int to_y, string ctrl_string)
{
	to_restore->Enter(this);
	var effect = AddEffect("Restore", this, 100, 1, this);
	effect.to_restore = to_restore;
	effect.to_container = to_container;
	effect.to_x = to_x;
	effect.to_y = to_y;
	effect.ctrl_string = ctrl_string;
	return;
}

protected func FxRestoreStart(object target, effect, int temporary)
{
	effect.init_x = target->GetX();
	effect.init_y = target->GetY();
	return 1;
}

protected func FxRestoreTimer(object target, effect, int time)
{
	// Remove effect if there is nothing to restore.
	if (!effect.to_restore)
		return -1;
	// Get coordinates.
	var init_x = effect.init_x;
	var init_y = effect.init_y;
	var to_container = effect.to_container;
	if (to_container)
	{
		var to_x = to_container->GetX();
		var to_y = to_container->GetY();
	}
	else
	{
		var to_x = effect.to_x;
		var to_y = effect.to_y;
	}
	// Are the coordinates specified now, if not remove effect.
	if (to_x == nil || to_y == nil)
	{
		effect.to_restore->RemoveObject();
		return -1;		
	}
	// Move to the object with a weighed sin-wave centered around the halfway point.
	var length = Distance(init_x, init_y, to_x, to_y);
	// Remove effect if animation is done.
	if (2 * time > length || length == 0)
		return -1;
	var angle = Angle(init_x, init_y, to_x, to_y);
	var std_dev = Min(length / 16, 40);
	var dev;
	if (time < length / 4)
		dev = 4 * std_dev * time / length;
	else
		dev = 2 * std_dev - 4 * std_dev * time / length;
	// Set container to current location to shift view.
	var x = init_x + Sin(angle, 2 * time);
	var y = init_y - Cos(angle, 2 * time);
	target->SetPosition(x, y);
	// Draw Particles.
	x = init_x + Sin(angle, 2 * time) + Cos(angle, Sin(20 * time, dev)) - target->GetX();
	y = init_y - Cos(angle, 2 * time) + Sin(angle, Sin(20 * time, dev)) - target->GetY();
	var color = RGB(128 + Cos(4 * time, 127), 128 + Cos(4 * time + 120, 127), 128 + Cos(4 * time + 240, 127));
	CreateParticle("PSpark", x, y, 0, 0, 32, color);
	x = init_x + Sin(angle, 2 * time) - Cos(angle, Sin(20 * time, dev)) - target->GetX();
	y = init_y - Cos(angle, 2 * time) - Sin(angle, Sin(20 * time, dev)) - target->GetY();
	CreateParticle("PSpark", x, y, 0, 0, 32, color);
	return 1;
}

protected func FxRestoreStop(object target, effect, int reason, bool temporary)
{
	var to_restore = effect.to_restore;
	var to_container = effect.to_container;
	var to_x = effect.to_x;
	var to_y = effect.to_y;
	var ctrl_string = effect.ctrl_string;
	// Only if there is something to restore.
	if (to_restore)
	{			
		to_restore->Exit();
		if (to_container)
			to_restore->Enter(to_container);
		else
			to_restore->SetPosition(to_x, to_y);
		// Restored object might have been removed on enter (Stackable).
		if (to_restore && !to_restore->~IsStackable())
		{
			// Add new restore mode, either standard one or effect supplied in EffectVar 4.
			if (ctrl_string)
			{
				var new_effect = AddEffect(ctrl_string, to_restore, 100, 10);
				new_effect.to_container = to_container;
				new_effect.to_x = to_x;
				new_effect.to_y = to_y;
			}
			else
				to_restore->AddRestoreMode(to_container, to_x, to_y);
		}
		// Add particle effect.
		for (var i = 0; i < 20; i++)
		{
			if (to_container)
			{
				to_x = to_container->GetX();
				to_y = to_container->GetY();			
			}
			var color = RGB(128 + Cos(18 * i, 127), 128 + Cos(18 * i + 120, 127), 128 + Cos(18 * i + 240, 127));
			CreateParticle("PSpark", to_x - GetX(), to_y - GetY(), RandomX(-10, 10), RandomX(-10, 10), 32, color);			
		}
		// Sound.
		//TODO new sound.
	}
	// Remove restoral object.
	if (target)
		target->RemoveObject();
	return 1;
}

/*-- Global restoration on destruction --*/

// Adds an effect to restore an item on destruction.
global func AddRestoreMode(object to_container, int to_x, int to_y)
{
	if (!this)
		return;
	var effect = AddEffect("RestoreMode", this, 100);
	effect.to_container = to_container;
	effect.to_x = to_x;
	effect.to_y = to_y;
	return;
}

// Destruction check, uses Fx*Stop to detect item removal.
// Effectvar 0: Container to which must be restored.
// Effectvar 1: x-coordinate to which must be restored.
// Effectvar 2: y-coordinate to which must be restored.
global func FxRestoreModeStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = effect.to_container;
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		var restored = CreateObject(target->GetID(), 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, to_x, to_y);
	}
	return 1;
}

local Name = "$Name$";
local Description = "$Description$";
