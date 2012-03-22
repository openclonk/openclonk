/*--
	Ecke
	Originally from Sven2
	Modified by Mimmo
--*/

local dir;
protected func Initialize() {
	AutoP();
	return true;
}

func PermaEdge()
{
	var p=dir;
	if(p==0) DrawMaterialQuad("Brick-brick1",GetX()-5,GetY()-5,GetX(),GetY(),GetX()+5,GetY()+5,GetX()-5,GetY()+5);
	if(p==1) DrawMaterialQuad("Brick-brick1",GetX(),GetY(),GetX()+5,GetY()-5,GetX()+5,GetY()+5,GetX()-5,GetY()+5);
	if(p==3) DrawMaterialQuad("Brick-brick1",GetX()-5,GetY()-5,GetX()+5,GetY()-5,GetX()+4,GetY()+5,GetX(),GetY());
	if(p==2) DrawMaterialQuad("Brick-brick1",GetX()-5,GetY()-5,GetX()+5,GetY()-5,GetX(),GetY(),GetX()-5,GetY()+5);
	this->RemoveObject();
}


protected func AutoP()
{
	var dir=[];
	dir[0]=GBackSolid(6 ,0);
	dir[1]=GBackSolid(0,-6);
	dir[2]=GBackSolid(-6,0);
	dir[3]=GBackSolid(0 ,6);
	if(dir[0] && dir[1]) SetP(3);
	if(dir[1] && dir[2]) SetP(2);
	if(dir[0] && dir[3]) SetP(1);
	if(dir[2] && dir[3]) SetP(0);
}


public func SetP(int p)
{
	SetObjDrawTransform(1000-((p%2)*2000),0,0,0,1000-((p/2)*2000));
	SetAction("Edge"); SetPhase(p);
	SetSolidMask(p*10,0,10,10);
	dir=p;
}

public func GetD() { return dir; }

global func MakeEdgeFunction(bool fExact)
{
	var x=[];
	var y=[];
	if(fExact)var d=[];
	for(var e in FindObjects(Find_ID(BrickEdge)))
	{
		x[GetLength(x)]=e->GetX();
		y[GetLength(y)]=e->GetY();
		if(fExact) d[GetLength(d)]=e->GetD();
	}
	Log("private func PlaceEdges()");
	Log("{");
	Log("	var x=%v;",x);
	Log("	var y=%v;",y);
	if(fExact) Log("	var d=%v;",d);
	Log("	for (var i = 0; i < GetLength(x); i++)");
	Log("	{");
	Log("		var edge=CreateObject(BrickEdge, x[i], y[i], NO_OWNER);");
	Log("		edge->Initialize();"); //additional initialize for anti self blocking
	if(fExact)
	{
		Log("		edge->SetP(d[i]);");
		Log("		edge->SetPosition(x[i],y[i]);");
	}
	Log("		edge->PermaEdge();");
	Log("	}");
	Log("	return;");
	Log("}");
}

  
