/**
	Tree
	Basic functionality for everything choppable (trees).
	Must be included after Library_Plant!

	@author Clonkonaut
*/

/** Return true if this a 'burned tree'. It will not grow or seed then.
*/
public func IsBurnedTree()
{
	return false;
}

/* Placement */

// Overload Place to add the foreground parameter, foreground = true will roughly make every 3rd tree foreground (not the offspring though)
public func Place(int amount, proplist rectangle, proplist settings, bool foreground)
{
	// Default behaviour
	var trees = _inherited(amount, rectangle, settings);
	if (GetLength(trees) < 1) return trees;

	for (var tree in trees)
		if (!Random(3))
			tree.Plane = 510;
	return trees;
}

/* Creation */

/** Sets a random mesh rotation and Growth(5) if no growth set.
*/
private func Construction()
{
	_inherited(...);

	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	if (!GetGrowthValue() && !IsBurnedTree()) StartGrowth(5);
	if (IsBurnedTree()) RemoveTimer("Seed");
}

/* Chopping */

/** Determines whether this plant gives wood (we assume that are 'trees').
	@return \c true if the plant is choppable by the axe (default), \c false otherwise.
*/
public func IsTree()
{
	return true;
}

/** Determines whether the tree can still be chopped down (i.e. has not been chopped down).
	@return \c true if the tree is still a valid axe target.
*/
public func IsStanding()
{
	return GetCategory() & C4D_StaticBack;
}

/** Maximum damage the tree can take before it falls. Each blow from the axe deals 10 damage. Default is 50.
	@return \c the maximum amount of damage.
*/
private func MaxDamage()
{
	return 50;
}

/** Define a burned definition of the tree. If nil, the tree will burst into ashes.
*/
local lib_tree_burned;

private func Damage(int change, int cause)
{
	// do not grow for a few seconds
	var g = GetGrowthValue();
	if(g)
	{
		StopGrowth();
		ScheduleCall(this, "RestartGrowth", 36 * 10, 0, g);
	}
	// Max damage reached: burn or fall down
	if (GetDamage() > MaxDamage())
	{
		// Burn
		if (OnFire())
		{
			if (!lib_tree_burned) return BurstIntoAshes();
			var burned = CreateObject(lib_tree_burned, 0, 0, GetOwner());
			burned->SetCategory(GetCategory());
			burned.Touchable = this.Touchable;
			burned->SetCon(GetCon());
			if (burned)
			{
				burned->SetR(GetR());
				burned->Incinerate(OnFire());
				burned->SetPosition(GetX(), GetY());
				return RemoveObject();
			}
			// Something went wrong. Better Burst into ashes!
			return BurstIntoAshes();
		}
		// Fall down
		if (IsStanding())
			return this->ChopDown();
	}
	if (cause == FX_Call_DmgChop && IsStanding())
		ShakeTree();
}

// Restarts the growing of the tree (for example after taking damage)
private func RestartGrowth(int old_value)
{
	var g = GetGrowthValue(); // safety
	if(g) StopGrowth();
	g = Max(g, old_value);
	StartGrowth(g);
}

// Visual chopping effect
private func ShakeTree()
{
	var effect = AddEffect("IntShakeTree", this, 100, 1, this);
	effect.current_trans = this.MeshTransformation;
}

private func FxIntShakeTreeTimer(object target, proplist effect, int time)
{
	if (time > 24)
		return FX_Execute_Kill;
	var angle = Sin(time * 45, 2);
	var r = Trans_Rotate(angle, 0, 0, 1);
	target.MeshTransformation = Trans_Mul(r, effect.current_trans);
	return FX_OK;
}

public func BurstIntoAshes()
{
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	
	var r = GetR();
	var size = GetCon() * 110 / 100;
	
	for(var cnt = 0; cnt < 10; ++cnt)
	{
		var distance = Random(size/2);
		var x = Sin(r, distance);
		var y = -Cos(r, distance);
		
		for(var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("Ashes", 5, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
}

/** Called when the trees shall fall down (has taken max damage).
	Default behaviour is too remove the first vertex (ensure it's the bottom one), unstucking (5 pixel movement max) and removing C4D_StaticBack.
*/
public func ChopDown()
{
	// stop growing!
	ClearScheduleCall(this, "RestartGrowth");
	StopGrowth();
	// stop reproduction
	RemoveTimer("Seed");

	this.Touchable = 1;
	this.Plane = 300;
	SetCategory(GetCategory()&~C4D_StaticBack);
	// Use Special Vertex Mode 1 (see documentation) so the removed vertex won't come back when rotating the tree.
	SetVertex(0, VTX_Y, 0, 1);
	RemoveVertex(0);
	// Still stuck?
	if (Stuck())
	{
		var i = 5;
		while(Stuck() && i)
		{
			SetPosition(GetX(), GetY()-1);
			i--;
		}
	}
	Sound("TreeCrack");
	AddEffect("TreeFall", this, 1, 1, this);
}

// determine a random falling direction and passes it on to the FxTreeFallTimer.
private func FxTreeFallStart(object target, proplist effect)
{
	effect.direction = Random(2); 
	if (effect.direction == 0) effect.direction -= 1;
}

/* animates the falling of the tree: First 10 slow degress then speed up and play the landing sound at 80+ degrees. 
remember that degrees range from -180 to 180. */
private func FxTreeFallTimer(object target, proplist effect)
{
	//simple falling if tree is not fully grown
	if (target->GetCon() <= 50)
	{
		target->SetRDir(effect.direction * 10);
	} 
	//else rotate slowly first until about 10 degree. This will be the time needed for the crack sound and makes sense as a tree will start falling slowly.
	else
	{
		if (Abs(target->GetR()) < 10) 
		{
			target->SetRDir(effect.direction * 1);
			//Turn of gravity so the tree doesn't get stuck before its done falling.
			target->SetYDir(0);
		} 
		else 
		{
			//Then speed up and let gravity do the rest.
			target->SetRDir(effect.direction * 10);
		}	
	}
	//if the tree does not lend on a cliff or sth. (is rotated more then 80 degrees in the plus or minus direction) Play the landing sound of the tree.
	if (Abs(target->GetR()) > 80)
	{
		target->SetRDir(0);
		if (target->GetCon() > 50) target->Sound("TreeLanding", false);
		return -1;
	}
	//check every frame if the tree is stuck and stop rotation in that case this is necessary as a tree could get stuck before reaching 80 degrees
	if ((target->GetContact(-1, CNAT_Left) | target->GetContact(-1, CNAT_Right)) > 0)
	{
		target->SetRDir(0);
		return -1;
	}
}