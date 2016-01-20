/* Automatically created objects file */

func InitializeObjects()
{
	CreateObject(Rule_NoPowerNeed);
	CreateObject(Basement, 111, 124);
	CreateObject(Basement, 112, 210);
	CreateObject(Basement, 111, 282);

	var WoodenBridge001 = CreateObjectAbove(WoodenBridge, 103, 388);
	WoodenBridge001->SetCategory(C4D_StaticBack);
	var WoodenBridge002 = CreateObjectAbove(WoodenBridge, 261, 535);
	WoodenBridge002->SetCategory(C4D_StaticBack);

	CreateObjectAbove(Elevator, 244, 119);
	CreateObjectAbove(Elevator, 496, 225);
	var Lorry001 = CreateObject(Lorry, 159, 111);
	Lorry001->SetR(-17);
	CreateObjectAbove(Lorry, 480, 222);
	var Lorry002 = CreateObject(Lorry, 305, 215);
	Lorry002->SetR(24);
	return true;
}
