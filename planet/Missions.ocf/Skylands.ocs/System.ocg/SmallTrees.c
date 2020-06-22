/* Small Trees */
// Trees stay small (50% growth) but still yield a lot of wood.

#appendto Library_Tree

// Initially placed trees are smaller
func Initialize(...)
{
	SetCon(GetCon() / 2);
	return _inherited(...);
}

// Stop growth at 50%
func FxIntGrowthTimer(object obj, effect, ...)
{
	if (obj->OnFire() || obj->GetCon() >= 50) return FX_Execute_Kill;
	return inherited(obj, effect, ...);
}

// Yield 1-2 extra wood
func Split2Components(...)
{
	CreateContents(Wood);
	if (GetCon() > 25) CreateContents(Wood);
	return inherited(...);
}

func GetComponent(comp_id)
{
	var result = inherited(comp_id, ...);
	if (GetType(this) == C4V_Def) return result;
	if (comp_id == Wood) result += 1 + (GetCon() > 25);
	return result;
}
