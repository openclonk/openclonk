#appendto LargeCaveMushroom

func AddPoisonEffect(int off_x, int off_y)
{
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
	var victim = FindObject(Find_AtRect(-15+fx.off_x,-15+fx.off_y,30,45), Find_OCF(OCF_Alive), Find_Layer(GetObjectLayer()));
	if (!victim) return FX_OK;
	// Target found. Spray!
	var tx=victim->GetX()-GetX()-fx.off_x, ty=victim->GetY()-GetY()-fx.off_y+30;
	var d=Max(Distance(tx,ty),1);
	var vx=tx*30/d, vy=ty*30/d;
	for (var x=-10; x<=10; x+=3)
	{
		var y=-30+Random(10);
		LargeCaveMushroomPoison->LaunchPoison(XY2TX(x+fx.off_x,y+fx.off_y)+GetX(),XY2TY(x+fx.off_x,y+fx.off_y)+GetY(),vx+Random(11)-5,vy+Random(11)-5, GetOwner());
	}
	Sound("PoisonLaunch");
	// Don't spray again for a few seconds
	fx.cooldown=10;
	return FX_OK;
}

// apply rotation and scale to x/y coordinates
func XY2TX(int x, int y) { var r=GetR(); return (Cos(r,x)-Sin(r,y))*GetCon()/100; }
func XY2TY(int x, int y) { var r=GetR(); return (Sin(r,x)+Cos(r,y))*GetCon()/100; }
