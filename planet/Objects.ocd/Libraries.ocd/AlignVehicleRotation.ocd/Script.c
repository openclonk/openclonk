//Makes objects follow the solid mask of a vehicle if they are touching it (within a specified rectangle)
//Works for both livings and vehicles.

//To enable in a vehicle, you need two things:
//1. A parent object which will rotate 'children' to match it's rotation on the Z axis (screen-vertical)
//	which either AlignObjectsToRotation or AlignToRotation will be called from, and
//2. A 'turnTarget' which will be queried for the Z-angle, in the form of 'turnTarget->GetTurnAngle()'.
//	This function should return a value from 0-180, where 0 is turned completely left, and 180 right.

//	The turnTarget can be the same object as the parent object. The reason they are separated is due
//	to the separation of the 3D graphic and solidmask object of any solidmask enabled 3D vehicle.
//	In this way, the 3D graphic module of a vehicle can query its own animation position.

///Function must be called from vehicle context
///param x,y,w,h = target rectangle to look for objects in to follow the vehicle's rotation
func AlignObjectsToRotation(object turnTarget, int x, int y, int w, int h)
{
	if(!this) FatalError("Function AlignToRotation must be called from object (vehicle) context");
	if(!turnTarget) FatalError("Function requires a valid turnTarget to get rotation value from");
	
	if(x == nil) x = this->GetDefCoreVal("Offset", "DefCore", 0);
	if(y == nil) y = this->GetDefCoreVal("Offset", "DefCore", 1);
	if(w == nil) w = this->GetDefCoreVal("Width", "DefCore");
	if(h == nil) h = this->GetDefCoreVal("Height", "DefCore");
	
	//make objects follow ship rotation
	for(var targetObj in FindObjects(Find_Not(Find_ID(this)), Find_NoContainer(), Find_Or(Find_Category(C4D_Living), Find_Category(C4D_Object), Find_Func("IsVehicle")), Find_InRect(x,y,w,h))){
		AlignToRotation(targetObj, turnTarget);
	}
}

//Used to parent a single object to the vehicle's rotation
/// param 'target' is the child object. Function is called from the vehicle context.
/// turnTarget is the object which must have a function returning the angle from
/// 0-180 (left to right), from the function GetTurnAngle()
func AlignToRotation(object target, object turnTarget)
{
	if(target){
	//only objects with lower indexes can ride
	if(target.Plane <= this.Plane) return false;
	
		//Must have a turnTarget to check the rotation
		if(turnTarget == nil){
			FatalError("No defined turnTarget object to get rotation from");
		}
		
		//turnTarget must have a function returning what angle the vehicle is at
		else {
			if(turnTarget->~GetTurnAngle() == nil){
				FatalError(Format("turnTarget %s has no GetTurnAngleFunction", turnTarget->GetName()));
			}
		}
		
//		if(target->GetContact(-1) > 0){
		if(target->GetY() > -1 || target->GetY() < 1){
			var oldNewX = nil;
			var oldNewR = nil;
			
			var oldEffect = GetEffect("AlignRotation", target);
			//Get originalX from previous effect if it existed
			if(oldEffect){
				oldNewX = oldEffect.originalX * -1;
				if(oldEffect.originalR != nil) oldNewR = oldEffect.originalR * -1;
				RemoveEffect(nil,target,oldEffect);
			}
			
			//Create the turn effect
			var effect = AddEffect("AlignRotation", target, 1, 1, this);
			//set effect vars
			effect.turnTarget = turnTarget;
			
			//If the turn was already in progress, reuse the last one
			if(oldNewX) effect.originalX = oldNewX;
			if(oldNewR) effect.originalR = oldNewR;
		}
	}
}
	
private func FxAlignRotationStart(object target, proplist effect)
{
	/// param 'target' is the object which follows 'this'
	/// 'this' refers to the vehicle the effect is created in the object context of.
	
	DebugLog(Format("Added AlignRotation effect to %s, child of %s", target->GetName(), GetName()));
	
	effect.originalX = target->GetX(100) - this->GetX(100);
	effect.originalY = target->GetY(100) - this->GetY(100);
	effect.originalR = nil;
	if(target->GetDefCoreVal("Rotate", "DefCore") >= 1){
		effect.originalR = Normalize(target->GetR(),-180);
		if(target->GetDefCoreVal("Rotate", "DefCore") == 1){
			effect.noRSmooth = true;
		}
	}
	
	//Temporary no collection effect
	effect.oldCollect = target.Collectible;
	if(effect.oldCollect == 1) target.Collectible = 0;
	
	effect.floorYOff = (effect.originalY  + ((target->GetDefCoreVal("Height", "DefCore") / 2 + 2) * 100));
	
	//Create floor helper. Makes certain clonks don't fall while boat is turning
	//(due to the solid mask flipping and such); so they keep a solid footing.
	if(target->GetCategory() & C4D_Living){
//	if(target->GetY() == 0){
		effect.floorHelper = this->CreateObject(Vehicle_FloorHelper, effect.originalX / 100, (effect.originalY / 100) + (effect.floorYOff / 100) + 1);
		
		//adjust floor helper size
		var width = BoundBy(target->GetDefCoreVal("Width", "DefCore"),6,64);
		effect.floorHelper->SetSolidMask(0,0,width,2, (width * -1) / 2 + 1,0);
	}
	
	return 1;
}

//Basically what this does is move the target object in a sinal path to emulate
//rotating with the boat.
private func FxAlignRotationTimer(object target, proplist effect, int timer)
{
	//Z-Rotation value of the vehicle
	var ang = effect.turnTarget->GetTurnAngle() - 90;
	
	//sinal movement and rotation
	var newX = Sin(ang, Abs(effect.originalX));
	var newR = Sin(ang, Abs(effect.originalR)) * -1;
	
	//Is the target on the left side of the vehicle, and the vehicle is turning left?
	//Is the target on the right side of the vehicle, and the vehicle is turning right?
	if((effect.originalX < 0 && this->GetDir() == 0) || (effect.originalX > 0 && this->GetDir() == 1)){
		newX = Sin(ang, Abs(effect.originalX)) * -1;
		newR = Sin(ang, Abs(effect.originalR));
	}
	
	//Debug
	target->Message(Format("%d,%d; %d", effect.originalX, newX, effect.floorYOff));
	
	if(newX != effect.originalX * -1){
		target->SetPosition(newX + this->GetX(100), target->GetY(100), true, 100);
		
		//change rotation
		if(effect.originalR != nil){
			//debug
//			target->Message(Format("R=%d", newR));

//			target->SetR(newR);
			if(target->GetCategory() & C4D_Object) target->SetR();
		}
		
		//Make sure vehicles aren't affected by gravity when rotating
		if((target->GetCategory() & C4D_Living) == 0){
			if(target->GetProcedure() == "FLOAT") target->SetYDir(0);
			else target->SetYDir(-2);
		}
			
		//update floor-helper position for clonks/livings
		if(effect.floorHelper != nil){
			effect.floorHelper->SetPosition(this->GetX(100) + newX, this->GetY(100) + effect.floorYOff, true, 100);
		}
	}
	else{
		return -1;
	}
}

private func FxAlignRotationStop(object target, proplist effect)
{
	//re-enable collection
	target.Collectible = effect.oldCollect;

	var newX = effect.originalX * -1;
	//push vehicle up out of solidmask if it got stuck in it
	while(target->Stuck()) target->SetPosition(newX + this->GetX(100), target->GetY(100) - 50, true, 100);
	
	//Get rid of the floor helper object
	if(effect.floorHelper) effect.floorHelper->RemoveObject();
}