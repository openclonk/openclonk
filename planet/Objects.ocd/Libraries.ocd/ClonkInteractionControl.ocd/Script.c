/**
	ClonkInteractionControl
	Handles the Clonk's interaction with other objects.

*/


/*
	used properties:
	this.control.is_interacting: whether interaction is in progress (user is holding [space])
	this.control.interaction_start_time: frame counter at the time of the selection process
	this.control.interaction_hud_controller: hud object that takes the callbacks. Updated when starting interaction.
*/


public func Construction()
{
	this.control.is_interacting = false;
	return _inherited(...);
}

public func OnShiftCursor(object new_cursor)
{
	if (this.control.is_interacting)
		AbortInteract();
	return _inherited(new_cursor, ...);
}

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	if (!this) 
		return inherited(plr, ctrl, x, y, strength, repeat, status, ...);
	
	// Begin interaction.
	if (ctrl == CON_Interact && status == CONS_Down)
	{			
		this->CancelUse();
		BeginInteract();
		return true;
	}
	
	// Switch object or finish interaction?
	if (this.control.is_interacting)
	{
		// Stop picking up.
		if (ctrl == CON_InteractNext_Stop)
		{
			AbortInteract();
			return true;
		}
		
		// Finish picking up (aka "collect").
		if (ctrl == CON_Interact && status == CONS_Up)
		{
			EndInteract();
			return true;
		}
		
		// Switch left/right through objects.
		var dir = nil;
		if (ctrl == CON_InteractNext_Left) dir = -1;
		else if (ctrl == CON_InteractNext_Right) dir = 1;
		else if (ctrl == CON_InteractNext_CycleObject) dir = 0;
		
		if (dir != nil)
		{
			var item = FindNextInteraction(this.control.interaction_hud_controller->GetCurrentInteraction(), dir);
			if (item)
				SetNextInteraction(item);
			return true;
		}
	}
	
	return inherited(plr, ctrl, x, y, strength, repeat, status, ...);
}

private func FxIntHighlightInteractionStart(object target, proplist fx, temp, proplist interaction, int nr_interactions)
{
	if (temp) return;
	fx.obj = interaction.interaction_object;
	fx.interaction = interaction;
	fx.interaction_help = target.control.interaction_hud_controller->GetInteractionHelp(interaction, target);
	
	fx.dummy = CreateObject(Dummy, fx.obj->GetX() - GetX(), fx.obj->GetY() - GetY(), GetOwner());
	fx.dummy.ActMap = 
	{
		Attach = 
		{
			Name = "Attach",
			Procedure = DFA_ATTACH,
			FacetBase = 1
		}
	};
	fx.dummy.Visibility = VIS_Owner;
	// The selector's plane should be just behind the Clonk for stuff that usually is behind the Clonk.
	// Otherwise, it looks rather odd when the catapult shines through the Clonk.
	if (fx.obj.Plane < this.Plane)
	{
		fx.dummy.Plane = this.Plane - 1;
	}
	else
	{
		fx.dummy.Plane = 1000;
	}
	var multiple_interactions_hint = "";
	if (fx.interaction.has_multiple_interactions)
		multiple_interactions_hint = Format("|<c 999999>[%s] $More$..</c>", GetPlayerControlAssignment(GetOwner(), CON_Up, true, false));
	var cycle_interactions_hint = "";
	if (nr_interactions > 1)
		cycle_interactions_hint = Format("|<c 999999>[%s/%s] $Cycle$..</c>", GetPlayerControlAssignment(GetOwner(), CON_Left, true, false), GetPlayerControlAssignment(GetOwner(), CON_Right, true, false));
	fx.dummy->Message("@<c eeffee>%s</c>%s%s|", fx.interaction_help.help_text, multiple_interactions_hint, cycle_interactions_hint);

	// Center dummy!
	fx.dummy->SetVertexXY(0, fx.obj->GetVertex(0, VTX_X), fx.obj->GetVertex(0, VTX_Y));
	fx.dummy->SetAction("Attach", fx.obj);
	
	fx.width  = fx.obj->GetDefWidth();
	fx.height = fx.obj->GetDefHeight();
	
	// Draw the item's graphics in front of it again to achieve a highlighting effect.
	fx.dummy->SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, GFX_BLIT_Additive, fx.obj);
	
	var custom_selector = nil;
	if (fx.obj) custom_selector = fx.obj->~DrawCustomInteractionSelector(fx.dummy, target, fx.interaction.interaction_index, fx.interaction.extra_data);
	
	if (!custom_selector)
	{
		fx.scheduled_selection_particle = (FrameCounter() - this.control.interaction_start_time) < 10;
		if (!fx.scheduled_selection_particle)
			EffectCall(nil, fx, "CreateSelectorParticle");
	}
	else
	{
		// Note that custom selectors are displayed immediately - particle because they might e.g. move the dummy.
		fx.scheduled_selection_particle = false;
	}
}

private func FxIntHighlightInteractionCreateSelectorParticle(object target, effect fx)
{
	// Failsafe.
	if (!fx.dummy) return;
	
	// Draw a nice selector particle on item change.
	var selector =
	{
		Size = PV_Step(5, 2, 1, Max(fx.width, fx.height)),
		Attach = ATTACH_Front,
		Rotation = PV_Step(1, PV_Random(0, 360), 1),
		Alpha = 200
	};

	fx.dummy->CreateParticle("Selector", 0, 0, 0, 0, 0, Particles_Colored(selector, GetPlayerColor(GetOwner())), 1);
}

private func FxIntHighlightInteractionTimer(object target, proplist fx, int time)
{
	if (!fx.dummy) return -1;
	if (!fx.obj) return -1;
	
	if (fx.scheduled_selection_particle && time > 10)
	{
		EffectCall(nil, fx, "CreateSelectorParticle");
		fx.scheduled_selection_particle = false;
	}
}

private func FxIntHighlightInteractionStop(object target, proplist fx, int reason, temp)
{
	if (temp) return;
	if (fx.dummy) fx.dummy->RemoveObject();
	if (!this) return;
} 

private func FxIntHighlightInteractionOnExecute(object target, proplist fx)
{
	if (!fx.obj || !fx.dummy) return;
	var message = fx.dummy->CreateObject(FloatingMessage, 0, 0, GetOwner());
	message.Visibility = VIS_Owner;
	message->SetMessage(Format("%s||", fx.interaction_help.help_text));
	message->SetYDir(-10);
	message->FadeOut(1, 20);
}

private func SetNextInteraction(proplist to)
{
	// Clear all old markers.
	var e = nil;
	while (e = GetEffect("IntHighlightInteraction", this))
		RemoveEffect(nil, this, e);
	// And set & mark new one.
	this.control.interaction_hud_controller->SetCurrentInteraction(to);
	var interaction_cnt = GetInteractableObjectsCount();
	if (to)
		AddEffect("IntHighlightInteraction", this, 1, 2, this, nil, to, interaction_cnt);
}

private func FindNextInteraction(proplist start_from, int x_dir)
{
	var starting_object = this;
	if (start_from && start_from.interaction_object)
		starting_object = start_from.interaction_object;
	var sort = Sort_Func("Library_ClonkInventoryControl_Sort_Priority", starting_object->GetX());
	var interactions = GetInteractableObjects(sort);
	var len = GetLength(interactions);
	if (!len) return nil;
	// Find object next to the current one.
	// (note that index==-1 accesses the last element)
	var index = -1;
	// GetIndexOf does not use DeepEqual, so work around that here.
	for (var i = 0; i < len; ++i)
	{
		if (!DeepEqual(start_from, interactions[i])) continue;
		index = i;
		break;
	}
	
	if (index != -1) // Previous item was found in the list.
	{
		var previous_interaction = interactions[index];
		// Cycle interactions of same object (dir == 0).
		// Or cycle through objects to the right (x_dir==1) or left (x_dir==-1).
		var cycle_dir = x_dir;
		var do_cycle_object = x_dir == 0;
		if (do_cycle_object) cycle_dir = 1;
		
		var found = false;
		for (var i = 1; i < len; ++i)
		{
			index = (index + cycle_dir) % len;
			if (index < 0) index += len;
			var is_same_object = interactions[index].interaction_object == previous_interaction.interaction_object;
			if (do_cycle_object == is_same_object)
			{
				found = true;
				
				// When cycling to the left, make sure to arrive at the first interaction for that object (and not the last).
				// Otherwise it's pretty unintuitive, why you sometimes grab and sometimes enter the catapult as the first interaction.
				if (x_dir == -1)
				{
					// Fast forward to first interaction.
					var target_object = interactions[index].interaction_object;
					// It's guaranteed that the interactions are not split over the array borders. So we can just search until the index is 0.
					for (var current_index = index - 1; current_index >= 0; --current_index)
					{
						if (interactions[current_index].interaction_object == target_object)
						{
							index = current_index;
						}
						else
						{
							break;
						}
					}
				}
				break;
			}
		}
		
		if (!found) index = -1;
	}
	else
	{
		// Find highest priority item.
		var high_prio = nil;
		for (var i = 0; i < len; ++i)
		{
			var interaction = interactions[i];
			if (high_prio != nil && interaction.priority <= high_prio.priority) continue;
			high_prio = interaction;
			index = i;
		}
	}
	
	if (index == -1) return nil;
	var next = interactions[index];
	if (DeepEqual(next, start_from)) return nil;
	return next;
}

private func BeginInteract()
{
	this.control.interaction_hud_controller = this->GetHUDController();
	this.control.is_interacting = true;
	this.control.interaction_start_time = FrameCounter();
	
	// Force update the HUD controller, which is responsible for pre-selecting the "best" object.
	this.control.interaction_hud_controller->UpdateInteractionObject();
	// Then, iff the HUD shows an object, pre-select one.
	var interaction = this.control.interaction_hud_controller->GetCurrentInteraction();
	if (interaction)
		SetNextInteraction(interaction);
	this.control.interaction_hud_controller->EnableInteractionUpdating(false);
}

// Stops interaction selection without executing the current selection.
private func AbortInteract()
{
	this.control.interaction_hud_controller->SetCurrentInteraction(nil);
	EndInteract();
}

private func EndInteract()
{
	this.control.is_interacting = false;

	var executed = false;
	if (this.control.interaction_hud_controller->GetCurrentInteraction())
	{
		ExecuteInteraction(this.control.interaction_hud_controller->GetCurrentInteraction());
		executed = true;
	}
	
	var e = nil;
	while (e = GetEffect("IntHighlightInteraction", this))
	{
		if (executed)
			EffectCall(this, e, "OnExecute");
		RemoveEffect(nil, this, e);
	}
	
	this.control.interaction_hud_controller->SetCurrentInteraction(nil);
	this.control.interaction_hud_controller->EnableInteractionUpdating(true);
}

/*
	Wraps "PushBack", but also sets a flag when two interactions of the same object exist.
*/
private func PushBackInteraction(array to, proplist interaction)
{
	PushBack(to, interaction);
	var count = 0;
	for (var other in to)
	{
		if (other.interaction_object && (other.interaction_object == interaction.interaction_object))
		{
			count += 1;
			if (count > 1 || other != interaction)
				other.has_multiple_interactions = true;
		}
	}
}

/*
	returns an array containing proplists with informations about the interactable actions.
	The proplist properties are:
		interaction_object
		priority: used for sorting the objects in the action bar. Note that the returned objects are not yet sorted
		interaction_index: when an object has multiple defined interactions, this is the index
		extra_data: custom extra_data for an interaction specified by the object
		actiontype: the kind of interaction. One of the ACTIONTYPE_* constants
*/
func GetInteractableObjects(array sort)
{
	var possible_interactions = [];
	// find vehicles & structures & script interactables
	// Get custom interactions from the clonk
	// extra interactions are an array of proplists. proplists have to contain at least a function pointer "f", a description "desc" and an "icon" definition/object. Optional "front"-boolean for sorting in before/after other interactions.
	var extra_interactions = this->~GetExtraInteractions() ?? []; // if not present, just use []. Less error prone than having multiple if(!foo).
		
	// all except structures only if outside
	var can_only_use_container = !!Contained();

	// add extra-interactions
	if (!can_only_use_container)
	for(var interaction in extra_interactions)
	{
		PushBackInteraction(possible_interactions,
			{
				interaction_object = interaction.Object,
				priority = interaction.Priority,
				interaction_index = nil,
				extra_data = interaction,
				actiontype = ACTIONTYPE_EXTRA
			});
	}

	// Make sure that the Clonk's action target is always shown.
	// You can push a lorry out of your bounding box and would, otherwise, then be unable to release it.
	var main_criterion = Find_AtRect(-5, -10, 10, 20);
	var action_target = nil;
	if (action_target = GetActionTarget())
	{
		main_criterion = Find_Or(main_criterion, Find_InArray([action_target]));
	}

	// add interactables (script interface)
	var interactables = FindObjects(
		main_criterion,
		Find_Or(Find_OCF(OCF_Grab), Find_Func("IsInteractable", this), Find_OCF(OCF_Entrance)),
		Find_NoContainer(), Find_Layer(GetObjectLayer()),
		sort);
	for (var interactable in interactables)
	{
		var interaction_count = interactable->~GetInteractionCount() ?? 1;
		var uses_container = interactable == Contained();
		var can_be_grabbed = interactable->GetOCF() & OCF_Grab;
		var has_script_interaction = interactable->~IsInteractable(this);

		// handle the script interactions first
		// one object could have a scripted interaction AND be a vehicle
		if (has_script_interaction && (!can_only_use_container || uses_container))
		for(var index = 0; index < interaction_count; index++)
		{
			PushBackInteraction(possible_interactions,
				{
					interaction_object = interactable,
					priority = 9,
					interaction_index = index,
					extra_data = nil,
					actiontype = ACTIONTYPE_SCRIPT
				});
		}
		
		// check whether further interactions are possible
		// can be grabbed? (vehicles/chests..)
		if (can_be_grabbed && !can_only_use_container)
		{
			var priority = 19;
			// not if swimming because the grab command cannot really fix that (unlike e.g. scale/hangle)
			if (GetProcedure() == "SWIM")
				if (!this->~CanGrabUnderwater(interactable)) // unless it's a special clonk that can grab underwater. It needs to define a callback then.
					continue;
			// high priority if already grabbed
			if (GetActionTarget() == interactable) priority = 0;
			
			PushBackInteraction(possible_interactions,
				{
					interaction_object = interactable,
					priority = priority,
					interaction_index = nil,
					extra_data = nil,
					actiontype = ACTIONTYPE_VEHICLE
				});
		}
		
		// Can be entered or exited?
		var can_be_exited = interactable == Contained();
		var can_be_entered = interactable->GetOCF() & OCF_Entrance;
		// Check if object shape overlaps with entrance area.
		if (can_be_entered)
		{
			var entrance = interactable->GetEntranceRectangle();
			var shape = GetShape();
			entrance = Rectangle(interactable->GetX() + entrance[0], interactable->GetY() + entrance[1], entrance[2], entrance[3]);
			shape = Rectangle(GetX() + shape[0], GetY() + shape[1], shape[2], shape[3]);
			var overlap = Shape->Intersect(entrance, shape);
			// Interactable can be entered if the area of overlap is bigger than zero.
			can_be_entered = overlap->GetArea() > 0;
			// Interactable can be entered if you are pushing a vehcile and the object is a container, see issue #1969
			if (GetProcedure() == "PUSH")
			{
				can_be_entered &= (interactable->~IsContainer() || interactable->~AllowsVehicleEntrance());
			}
		}
		if (can_be_entered && (!can_only_use_container || can_be_exited))
		{
			var priority = 29;
			if (can_be_exited)
				priority = 0;
			PushBackInteraction(possible_interactions,
				{
					interaction_object = interactable,
					priority = priority,
					interaction_index = nil,
					extra_data = nil,
					actiontype = ACTIONTYPE_STRUCTURE
				});
		}
	}
	
	return possible_interactions;
}

// Returns the number of interactable objects, which is different from the total number of available interactions.
private func GetInteractableObjectsCount()
{
	var interactions = GetInteractableObjects();
	var interaction_objects = [];
	for (var interaction in interactions)
		PushBack(interaction_objects, interaction.interaction_object);
	RemoveDuplicates(interaction_objects);
	return GetLength(interaction_objects);
}

// executes interaction with an object. /action_info/ is a proplist as returned by GetInteractableObjects
func ExecuteInteraction(proplist action_info)
{
	if (!action_info.interaction_object)
		return;
		
	// object is a pushable vehicle
	if(action_info.actiontype == ACTIONTYPE_VEHICLE)
	{
		var proc = GetProcedure();
		// object is inside building -> activate
		if(Contained() && action_info.interaction_object->Contained() == Contained())
		{
			SetCommand("Activate", action_info.interaction_object);
			return true;
		}
		// crew is currently pushing vehicle
		else if(proc == "PUSH")
		{
			// which is mine -> let go
			if(GetActionTarget() == action_info.interaction_object)
				ObjectCommand("UnGrab");
			else
				ObjectCommand("Grab", action_info.interaction_object);
				
			return true;
		}
		// grab
		else if(proc == "WALK")
		{
			ObjectCommand("Grab", action_info.interaction_object);
			return true;
		}
	}
	// object is a building
	else if (action_info.actiontype == ACTIONTYPE_STRUCTURE)
	{
		// inside? -> exit
		if (Contained() == action_info.interaction_object)
		{
			ObjectCommand("Exit");
			return true;
		}
		// outside? -> enter
		else if (this->CanEnter())
		{
			// First attempt to enter the pushed vehicle.
			ObjectCommand("Enter", action_info.interaction_object, nil, nil, nil, C4CMD_Enter_PushTarget);
			return true;
		}
	}
	else if (action_info.actiontype == ACTIONTYPE_SCRIPT)
	{
		if(action_info.interaction_object->~IsInteractable(this))
		{
			action_info.interaction_object->Interact(this, action_info.interaction_index);
			return true;
		}
	}
	else if (action_info.actiontype == ACTIONTYPE_EXTRA)
	{
		if(action_info.extra_data)
			action_info.extra_data.Object->Call(action_info.extra_data.Fn, this);
	}
}
