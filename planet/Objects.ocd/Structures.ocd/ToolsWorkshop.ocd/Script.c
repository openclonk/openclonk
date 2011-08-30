/*-- Tools workshop --*/

#include Library_PowerConsumer
#include Library_Producer



public func Initialize()
{
	// SetProperty("MeshTransformation", Trans_Rotate(RandomX(-30,30),0,1,0));
	// the entrance is always open
	SetEntrance(1);
	return _inherited(...);
}


/*-- Production --*/

public func CanProduceItem(id item_id)
{
	return item_id->~IsToolProduct();
}

public func NeedsRawMaterial(id rawmat_id)
{
	return true;
}

public func IsProducing()
{
	if (GetEffect("Producing", this))
		return true;
	return false;
}

private func Produce(id item_id)
{
	// Check if material is available.
	var costs = ProductionCosts(item_id);
	for (var mat in costs)
		if (ContentsCount(mat[0]) < mat[1])
			return false;		
	// Check power need.
	if (!CheckPower(50, true))
		return false;
		
	// If already busy, wait a little.
	if (IsProducing())
		return false;
	// Start production.	
	AddEffect("Producing", this, 100, 1, this, nil, item_id);
	return true;
}

// Production effect: effect.ItemID is the item under production.
protected func FxProducingStart(object target, proplist effect, int temp, id item_id)
{
	if (temp)
		return 1;
	effect.ItemID = item_id;
	// Remove raw materials, then commence production.
	var costs = ProductionCosts(item_id);
	for (var mat in costs)
		for (var i = 0; i < mat[1]; i++)
			FindContents(mat[0])->RemoveObject();
	return 1;
}

protected func FxProducingTimer(object target, proplist effect, int time)
{
	// Production already done?
	if (time > 150) // Replace me with something sensible.
		return -1;
	// Substract a little power, if not available halt production.
	if (!CheckPower(2))
		return 1;
		
	Smoking();	

	return 1;
}

protected func FxProducingStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return 1;
	CreateObject(effect.ItemID, 0, 0);
	return 1;
}


/* Control */

protected func ContainedUp(object clonk)
{
	return MenuProduction(clonk);
}

private func MenuProduction(object clonk)
{
	// Create menu and fill it with the plans of the player
	clonk->CreateMenu(GetID(),this,1,"$NoPlrKnowledge$");
	var i = 0, know_id;
	while (know_id = GetPlrKnowledge(clonk->GetOwner(), nil, i, C4D_Object))
	{
		//Log("%v,%v", know_id,know_id->~IsToolProduct());
		if (know_id->~IsToolProduct())
			clonk->AddMenuItem("$Construction$: %s", "SelectProduction", know_id, 0, clonk);
		i++;
	}
	return 1;
}

protected func SelectProduction(id item_id)
{
	Message("Item {{%i}} selected", item_id);
	// Add production to queue.
	AddToQueue(item_id, 1);
	return;
}


private func Smoking()
{
	if (Random(6)) Smoke(+16,-14,16);
	if (Random(8)) Smoke(10,-14,15+Random(3));
	return 1;
}

public func SetSign(id def)
{
	var iSize = Max(def->GetDefCoreVal("Picture", "DefCore", 2), def->GetDefCoreVal("Picture", "DefCore", 3));
	SetGraphics("", def, 1, 4);
	SetObjDrawTransform(200, 0, 460*iSize, 0, 200, 90*iSize, 1);
}

local ActMap = {
	Build = {
		Prototype = Action,
		Name = "Build",
		Procedure = DFA_NONE,
		Length = 40,
		Delay = 1,
		FacetBase=1,
		NextAction = "Build",
		//Animation = "Turn",
		PhaseCall="Smoking",
	},
};
func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}

local Name = "$Name$";
