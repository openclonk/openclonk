/*--
		Object restorer
		Author: Maikel

		Restores an object and transports it back to its original position or container,
		can be used to prevent players from messing up tutorials by wasting items.
--*/


/*-- Object Restoration --*/

local particles;

protected func Initialize()
{
	particles =
	{
		Size = PV_Linear(10, 0),
		Stretch = PV_Speed(1000, 1000),
		Rotation = PV_Direction(),
		Alpha = PV_KeyFrames(0, 0, 0, 100, 64, 1000, 64),
		R = PV_Random(180, 200),
		G = PV_Random(180, 200),
		B = PV_Random(220, 255),
		DampingX = 900, DampingY = 900
	};
	return;
}

public func SetRestoreObject(object to_restore, object to_container, int to_x, int to_y, int to_r, string ctrl_string)
{
	to_restore->Enter(this);
	var effect = AddEffect("Restore", this, 100, 1, this);
	effect.to_restore = to_restore;
	effect.to_container = to_container;
	effect.to_x = to_x;
	effect.to_y = to_y;
	effect.to_r = to_r;
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
	var to_x, to_y;
	if (to_container)
	{
		to_x = to_container->GetX();
		to_y = to_container->GetY();
	}
	else
	{
		to_x = effect.to_x;
		to_y = effect.to_y;
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
	
	CreateParticle("SphereSpark", 0, 0, 0, 0, 36, particles, 1);
	return 1;
}

protected func FxRestoreStop(object target, effect, int reason, bool temporary)
{
	var to_restore = effect.to_restore;
	var to_container = effect.to_container;
	var to_x = effect.to_x;
	var to_y = effect.to_y;
	var to_r = effect.to_r;
	var ctrl_string = effect.ctrl_string;
	// Only if there is something to restore.
	if (to_restore)
	{			
		to_restore->Exit();
		if (to_container)
			to_restore->Enter(to_container);
		else
		{
			to_restore->SetPosition(to_x, to_y);
			to_restore->SetR(to_r);
		}
		// Restored object might have been removed on enter (Stackable).
		if (to_restore)
		{
			// Add new restore mode, either standard one or effect supplied in EffectVar 4.
			if (ctrl_string)
			{
				var new_effect = AddEffect(ctrl_string, to_restore, 100, 10);
				new_effect.to_container = to_container;
				new_effect.to_x = to_x;
				new_effect.to_y = to_y;
				new_effect.to_r = to_r;
			}
			else
				to_restore->AddRestoreMode(to_container, to_x, to_y, to_r);
		}
		// Add particle effect.
		for (var i = 0; i < 20; i++)
		{
			if (to_container)
			{
				to_x = to_container->GetX();
				to_y = to_container->GetY();			
			}
			CreateParticle("SphereSpark", to_x - GetX(), to_y - GetY(), PV_Random(-50, 50), PV_Random(-50, 50), PV_Random(2, 10), particles, 10);		
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
global func AddRestoreMode(object to_container, int to_x, int to_y, int to_r)
{
	if (!this)
		return;
	var effect = AddEffect("RestoreMode", this, 100);
	if (to_container == nil) to_container = Contained();
	if (to_x == nil) to_x = GetX();
	if (to_y == nil) to_y = GetY();
	if (to_r == nil) to_r = GetR();
	effect.to_container = to_container;
	effect.to_x = to_x;
	effect.to_y = to_y;
	effect.to_r = to_r;
	return;
}

global func RemoveRestoreMode()
{
	if (!this)
		return;
	return RemoveEffect("RestoreMode", this);
}

// Destruction check, uses Fx*Stop to detect item removal.
// Effectvar 0: Container to which must be restored.
// Effectvar 1: x-coordinate to which must be restored.
// Effectvar 2: y-coordinate to which must be restored.
global func FxRestoreModeStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = effect.to_container;
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		var to_r = effect.to_r;
		var restored = CreateObjectAbove(target->GetID(), 0, 0, target->GetOwner());
		if (restored)
		{
			restorer->SetRestoreObject(restored, to_container, to_x, to_y, to_r);
		}
	}
	return 1;
}

/* Scenario saving: Objects with restoration effect are stored. Restorations "on the way" are not. */

func SaveScenarioObject() { return false; }

global func FxRestoreModeSaveScen(obj, effect, props)
{
	props->AddCall("Restore", obj, "AddRestoreMode", effect.to_container, effect.to_x, effect.to_y);
	return true;
}


local Name = "$Name$";
local Description = "$Description$";
