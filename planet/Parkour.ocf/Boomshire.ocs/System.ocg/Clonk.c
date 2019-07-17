#appendto Clonk

// The clonk can only hold one item and only collect the windbag
protected func RejectCollect(id objid, object obj)
{
	if (objid != WindBag) 
		return true;
	return _inherited(objid, obj);
}

// Only one inventory slot.
local MaxContentsCount = 1;
local HandObjects = 1;

// Lose all items on death.
func StartDead()
{
	PlayAnimation("Dead", CLONK_ANIM_SLOT_Death, Anim_Linear(0, 0, GetAnimationLength("Dead"), 20, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	if (Contents(1))Contents(1)->RemoveObject();
	if (Contents(0))Contents(0)->RemoveObject();
	// Set proper turn type
	SetTurnType(1);
}