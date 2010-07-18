/*-- 
	Ecke 
	Originally from Sven2
	Modified by Mimmo	
--*/


protected func Initialize() {
  Snap();
  AutoP();
  return true;
}

protected func AutoP()
{
	var dir=[];
	dir[0]=GBackSolid(10 ,0);
	dir[1]=GBackSolid(0,-10);
	dir[2]=GBackSolid(-10,0);
	dir[3]=GBackSolid(0 ,10);
	if(dir[0] && dir[1]) SetP(3);
	if(dir[1] && dir[2]) SetP(2);
	if(dir[0] && dir[3]) SetP(1);
	if(dir[2] && dir[3]) SetP(0);
}
  
  
public func Snap(int o,int p)
  {
  SetPosition((GetX()+5)/10*10-5+o, (GetY()+5)/10*10-5+p);
  }

public func SetP(int p)
  {
  SetObjDrawTransform(1000-((p%2)*2000),0,0,0,1000-((p/2)*2000));
  SetAction("Edge"); SetPhase(p);
  SetSolidMask(p*10,0,10,10);
  }
  