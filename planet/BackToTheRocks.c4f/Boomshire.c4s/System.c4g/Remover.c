/* larger dynamite explosion */

#appendto StoneDoor
local plants;

protected func Initialize()
{
	plants=[];
	while(!GBackSolid(0,19)) SetPosition(GetX(),GetY()+1);
	for(var i=0; i<8; i++)
	{
		plants[GetLength(plants)]=CreateObject(Grass,-6,-15+i*5,NO_OWNER);
		plants[GetLength(plants)-1]->SetR(-90);
		plants[GetLength(plants)-1]->SetCategory(C4D_StaticBack);
	}
	for(var i=0; i<8; i++)
	{
		plants[GetLength(plants)]=CreateObject(Grass,+6,-15+i*5,NO_OWNER);
		plants[GetLength(plants)-1]->SetR(+90);
		plants[GetLength(plants)-1]->SetCategory(C4D_StaticBack);
	}

	SetAction("Door");
	return;
}



public func OpenGateDoor()
{
	if(plants) for(var i=0; i<GetLength(plants); i++){ plants[i]->Incinerate(); }
	plants[0]=["nothing"];
	AddEffect("IntMoveGateUp", this, 100, 1, this);
	Sound("GateMove");
	return;
}

