/*--
	Scroll: Teleport
	Author: Mimmo

	Teleports you to a safe position.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	var r=Random(5);
	var x,y;
	if(r==0) { x=370; y=220; }
	if(r==1) { x=600; y=500; }
	if(r==2) { x=100; y=470; }
	if(r==3) { x=500; y=330; }
	if(r==4) { x=200; y=150; }
	if(r==5) { x=400; y=470; }
	
	if(!Random(20)) { x=65; y=650; }
	
	DrawParticleLine("Magic",0,0,-GetX()+x,-GetY()+y,2,64,RGB(0,128,255),RGB(0,200,255),-1);
	pClonk->SetPosition(x,y);
	pClonk->SetXDir(0);
	pClonk->SetYDir(-5);
	
	RemoveObject();
	return 1;
}


local Name = "$Name$";
local Description = "$Description$";

