/*-- Tools workshop --*/

#include Library_PowerConsumer

/* Product properties (can be overloaded) */
func ProductType() { return C4D_Vehicle | C4D_Object; }
func ProductCondition() { return "IsToolProduct"; }

public func Construction()
{
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(-30,30),0,1,0));
}

public func Initialize()
{
	// the entrance is always open
	SetEntrance(1);
}

public func NeedsEnergy()
{
	if(CheckPower(100, true)) return false;
	return true;
}

/* Production */

public func IsProducerOf(caller, def)
{
	if (!(def->GetCategory () & ProductType())) return 0;
	if (!IsBuilt ()) return 0;
	if (!GetPlrKnowledge (caller->GetOwner(), def)) return 0;
	if (ProductCondition ())
		if (!DefinitionCall (def, ProductCondition ()))
			return 0;
	// Look for better
	if (NeedsEnergy ())
		if (FindSuppliedObjectCloseTo (caller)) return 0;
	return 1;
}

func FindSuppliedObjectCloseTo (obj, def)
{
	var obj2;
	if (!def) def = GetID ();
	for(var obj2 in FindObjects(Find_ID(def), Find_InRect((obj->GetX () - 1000) - GetX (), (obj->GetY () - 500) - GetY (), 2000, 1000),
		Find_OCF(OCF_Fullcon)))
			if (FindObject (Find_ID(PowerLine),Find_Action("Connect",obj2)))
				return obj2;
	return 0;
}

public func HowToProduce (clonk, def)
{
	if(NeedsEnergy())
	{
		clonk->AddCommand ("Call", this, def, 0, 0, 0, "HowToProduce");
		clonk->AddCommand ("Energy", this);
		return 1;
	}
	clonk->AddCommand ("Wait", 0, 0, 0, 0, 10);
	clonk->AddCommand ("Call", this, def, 0, 0, 0, "StartProduction");
	clonk->AddCommand ("Enter", this);
	return 1;
}

private func MenuProduction(pCaller)
{
	// Create menu and fill it with the plans of the player
	pCaller->CreateMenu(GetID(),this,1,"$NoPlrKnowledge$");
	for(var i=0,idKnowledge; idKnowledge=GetPlrKnowledge(pCaller->GetOwner(),0,i,ProductType ()); ++i)
	{
		if(ProductCondition())
			if(!DefinitionCall(idKnowledge, ProductCondition()))
				continue;
		pCaller->AddMenuItem("$Construction$: %s", "SelectProduction", idKnowledge, 0, pCaller);
	}
	return 1;
}

public func SelectProduction(idType,pWorker,bSpecial2)
{
	// Start working
	pWorker->AddCommand("Call",this,idType,bSpecial2,0,0,"StartProduction", 0, 1);
	// but enter the workshop before
	pWorker->AddCommand("Enter",this);
	return 1;
}
	
public func StartProduction(pWorker,idType,bSpecial2)
{
	var pToBuild;
	// Look for half object to finish
	pToBuild=FindIncompleteContents(idType);
	// If not create new object
	if(!pToBuild)
		if(!(pToBuild=CreateConstruction(idType,0,0,GetOwner(),1))) return 0;
	// Put new object into the workshop
	if (pToBuild->Contained()!=this)
		pToBuild->Enter(this);
	// Inform product
	pToBuild->~OnStartProduction(this);
	// Order to build
	pWorker->AddCommand("Build",pToBuild, 0,0,0,0,0,0, 3);
	
	if(bSpecial2)
		pWorker->AppendCommand("Call",this,idType,bSpecial2,0,0,"ProductionComplete", 0, 1);
	return 1;
}

public func ProductionCompleteFailed(pWorker,idType,bSpecial2)
{
	// Action "Build" can't be executed (resources are missing)
	// To recieve the Message "x needs y" execute one more the command "Build"
	// so that it fails and puts out the message.
	pWorker->AddCommand("Build",FindIncompleteContents(idType));
	return 1;
}

public func ProductionComplete(pWorker,idType,bSpecial2)
{
	// No continuous production? Stop
	if(!bSpecial2) return 0;
	// and than start again ... (and wait a bit)
	pWorker->AppendCommand("Wait",0,0,0,0,35);
	pWorker->AppendCommand("Call",this,idType,bSpecial2,0,0,"StartProduction");
	return 1;
}

/* Context */

public func ContextConstruction(pCaller)
{
	[$Production$|Image=CXCN|Condition=IsBuilt]
	return MenuProduction(pCaller);
}

protected func IsBuilt()
{
	return GetCon() >= 100;
}

/* Control */

protected func ContainedUp(pCaller)
{
	[$Production$|Image=CXCN]
	return MenuProduction(pCaller);
}

/* Activity */

private func CheckBuild()
{
	// TimerCall: The workshop starts its own Build-Action, when
	// someone works in the workshop. The work in a building only proceeds
	// when the building supports this with an own build-action.
	var bWorkingClonk=IsWorking();
	var bBuildingAction=(GetAction()=="Build");
	if(bWorkingClonk	&& !bBuildingAction) if (ActIdle()) SetAction("Build");
	if(!bWorkingClonk && bBuildingAction )
	{
		SetAction("Idle");
		NeedsEnergy();
	}
	return 1;
}

private func IsWorking()
{
	// Does someone work in the workshop?
	if (!Contents()) return 0;
	return FindObject(Find_Container(this), Find_Func("IsClonk"));//Find_Action("Build"));
}

private func Smoking()
{
	if (GetPhase()%3) return 1;
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


/* Helper functions */

private func FindIncompleteContents(idSearched)
{
	for(var i=0,pContent; pContent=Contents(i); ++i)
		if(pContent->GetID()==idSearched)
			if(pContent->GetCon()<100)
				return pContent;
	return 0;
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
