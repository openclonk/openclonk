/**
	Tree
	Basic functionality for everything choppable (trees)

	@author Clonkonaut
*/

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

/** Maximum damage the tree can take before it falls. Each blow from the axe deals 10 damage.
	@return \c the maximum amount of damage.
*/
private func MaxDamage()
{
	return 50;
}

protected func Damage()
{
	// do not grow for a few seconds
	var g = GetGrowthValue();
	if(g)
	{
		StopGrowth();
		ScheduleCall(this, "RestartGrowth", 36 * 10, 0, g);
	}
	
	// Max damage reached -> fall down
	if (GetDamage() > MaxDamage() && IsStanding()) this->ChopDown();
	_inherited(...);
}

// restarts the growing of the tree (for example after taking damage)
func RestartGrowth(int old_value)
{
	var g = GetGrowthValue(); // safety
	if(g) StopGrowth();
	g = Max(g, old_value);
	StartGrowth(g);
}

/** Called when the trees shall fall down (has taken max damage). Default behaviour is unstucking (5 pixel movement max) and removing C4D_StaticBack.
*/
public func ChopDown()
{
	// stop growing!
	ClearScheduleCall(this, "RestartGrowth");
	StopGrowth();
	this.Touchable = 1;
	this.Plane = 300;
	SetCategory(GetCategory()&~C4D_StaticBack);
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
func FxTreeFallStart(object target, proplist effect)
{
	effect.direction = Random(2); 
	if (effect.direction == 0) effect.direction -= 1;
}

/* animates the falling of the tree: First 10 slow degress then speed up and play the landing sound at 80+ degrees. 
remember that degrees range from -180 to 180. */
func FxTreeFallTimer(object target, proplist effect)
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
