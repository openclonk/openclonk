/* Poison trigger on cave mushroom */
// Adds a timer that checks for passing Clonks and sprays poison at them

#appendto LargeCaveMushroom

static const ShroomPoison_SprayOffY = -20,
             ShroomPoison_SearchWdt = 30,
             ShroomPoison_SearchHgt = 50,
             ShroomPoison_SearchOffY = 8;

func AddPoisonEffect(int off_x, int off_y)
{
	// off_x and off_y is the offset of the cap of the unrotated mushroom from its regular position at GetX()/GetY()-20
	// set this to match offset caused by animation
	var fx = AddEffect("SearchPoisonTarget", this, 1, 12, this);
	if (fx)
	{
		fx.off_x = off_x;
		fx.off_y = off_y;
	}
	return fx;
}

func FxSearchPoisonTargetTimer(object target, proplist fx)
{
	// Currently on cooldown?
	if (fx.cooldown) if (--fx.cooldown) return FX_OK;
	// Find target to spray poison at
	var victim = FindObject(Find_AtRect(XY2TX(fx.off_x,fx.off_y+ShroomPoison_SearchOffY)-ShroomPoison_SearchWdt/2,XY2TY(fx.off_x,fx.off_y+ShroomPoison_SearchOffY)-ShroomPoison_SearchHgt/2,ShroomPoison_SearchWdt,ShroomPoison_SearchHgt), Find_OCF(OCF_Alive), Find_Layer(GetObjectLayer()));
	if (!victim) return FX_OK;
	// Target found. Spray!
	var src_x = GetX()+XY2TX(fx.off_x,fx.off_y);
	var src_y = GetY()+XY2TY(fx.off_x,fx.off_y+ShroomPoison_SprayOffY);
	var tx=victim->GetX()-src_x, ty=victim->GetY()-src_y;
	var d=Max(Distance(tx,ty),1);
	var vx=tx*30/d, vy=ty*30/d-15;
	for (var i=0; i<5; ++i)
		LargeCaveMushroomPoison->LaunchPoison(src_x+Random(11)-5,src_y+Random(11)-5,vx+Random(11)-5,vy+Random(11)-5, GetOwner());
	Sound("PoisonLaunch");
	// Don't spray again for a few seconds
	fx.cooldown=10;
	return FX_OK;
}

// apply rotation and scale to x/y coordinates
func XY2TX(int x, int y) { var r=GetR(); return (Cos(r,x)-Sin(r,y))*GetCon()/100; }
func XY2TY(int x, int y) { var r=GetR(); return (Sin(r,x)+Cos(r,y))*GetCon()/100; }


/* Debug display */

func DbgShowSprayRange(fx)
{
	if (fx.dbg_rect) DbgHideSprayRange(fx);
	// Debug function to show search and spray range
	var x1=ShroomPoison_SearchWdt/-2+XY2TX(fx.off_x,fx.off_y+ShroomPoison_SearchOffY)+GetX(), y1=ShroomPoison_SearchHgt/-2+XY2TY(fx.off_x,fx.off_y+ShroomPoison_SearchOffY)+GetY();
	var x2=x1+ShroomPoison_SearchWdt, y2=y1+ShroomPoison_SearchHgt;
	fx.dbg_rect = DrawRect(x1,y1,x2,y2,0xff00ff00);
	fx.dbg_pos = CreateObject(Rock);
	if (fx.dbg_pos)
	{
		fx.dbg_pos->SetPosition(GetX()+XY2TX(fx.off_x,fx.off_y),GetY()+XY2TY(fx.off_x,fx.off_y+ShroomPoison_SprayOffY));
		fx.dbg_pos->SetCategory(1);
	}
	return true;
}

func DrawRect(int x1,int y1,int x2,int y2,int clr)
{
	var r = {};
	r.t=DebugLine->Create(x1,y1,x2,y1,clr);
	r.r=DebugLine->Create(x2,y1,x2,y2,clr);
	r.b=DebugLine->Create(x2,y2,x1,y2,clr);
	r.l=DebugLine->Create(x1,y2,x1,y1,clr);
	return r;
}

func ClearRect(r)
{
	if (r.t) r.t->RemoveObject();
	if (r.r) r.r->RemoveObject();
	if (r.b) r.b->RemoveObject();
	if (r.l) r.l->RemoveObject();
	return true;
}

func DbgHideSprayRange(fx)
{
	if (fx.dbg_rect) { ClearRect(fx.dbg_rect); fx.dbg_rect = nil; }
	if (fx.dbg_pos) { fx.dbg_pos->RemoveObject(); fx.dbg_pos = nil; }
	return true;
}

func EditCursorSelection()
{
	var fx = GetEffect("SearchPoisonTarget", this);
	if (fx) DbgShowSprayRange(fx);
	return _inherited(...);
}

func EditCursorDeselection()
{
	var fx = GetEffect("SearchPoisonTarget", this);
	if (fx) DbgHideSprayRange(fx);
	return _inherited(...);
}
