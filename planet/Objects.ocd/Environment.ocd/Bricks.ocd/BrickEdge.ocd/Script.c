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
	if(p==0) DrawMaterialQuad("Brick-brick1",GetX()-4,GetY()-4,GetX(),GetY(),GetX()+4,GetY()+4,GetX()-4,GetY()+4);
	if(p==1) DrawMaterialQuad("Brick-brick1",GetX(),GetY(),GetX()+4,GetY()-4,GetX()+4,GetY()+4,GetX()-4,GetY()+4);
	if(p==3) DrawMaterialQuad("Brick-brick1",GetX()-4,GetY()-4,GetX()+4,GetY()-4,GetX()+4,GetY()+4,GetX(),GetY());
	if(p==2) DrawMaterialQuad("Brick-brick1",GetX()-4,GetY()-4,GetX()+4,GetY()-4,GetX(),GetY(),GetX()-4,GetY()+4);
	this->RemoveObject();
}


protected func AutoP()
{
	var dir=[];
	dir[0]=GBackSolid(5 ,0);
	dir[1]=GBackSolid(0,-5);
	dir[2]=GBackSolid(-5,0);
	dir[3]=GBackSolid(0 ,5);
	if(dir[0] && dir[1]) SetP(3);
	if(dir[1] && dir[2]) SetP(2);
	if(dir[0] && dir[3]) SetP(1);
	if(dir[2] && dir[3]) SetP(0);
}


public func SetP(int p)
{
	SetObjDrawTransform(1000-((p%2)*2000),0,0,0,1000-((p/2)*2000));
	SetAction("Edge"); SetPhase(p);
	SetSolidMask(p*8,0,8,8);
	dir=p;
}

// Edges in saved scenarios
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (dir) props->AddCall("P", this, "SetP", dir);
	return true;
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

  
