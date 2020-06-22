/* Convert structures to ruins with some effects on damage */

global func MakeRuinsOnDamage()
{
	AddEffect("IntRuinOnDamage", g_chemical, 100, 0, g_chemical, nil, Ruin_ChemicalLab,[{x=-12, y = 18, r = 130, s = 80, t = 1300}]); // -8, 5 (big)   14, 5 (mid)   0,-14 (small)
	AddEffect("IntRuinOnDamage", g_cabin, 100, 0, g_cabin, nil, Ruin_WoodenCabin, [{x=-20, y=-10, r=-45, s = 50, t = 1500}]);
	AddEffect("IntRuinOnDamage", g_windmill, 100, 0, g_windmill, nil, Ruin_Windmill, [{x=-15, y = 42, r = 45, s = 50, t = 1200}]);
	AddEffect("IntRuinOnDamage", g_flagpole, 100, 0, g_flagpole, nil, Flagpole, [{x=-2, y=-8, r=-15, s = 50, t = 1250}]);
}

global func FxIntRuinOnDamageStart(object target, fx, int tmp, ruin_id, fire_positions)
{
	fx.ruin_id = ruin_id;
	fx.fire_positions = fire_positions;
	return FX_OK;
}

global func FxIntRuinOnDamageDamage(object target, fx, int dmg, int cause)
{
	if (dmg > 0 && !(cause & 32))
	{
		var ruin, i;
		if (fx.ruin_id == Flagpole)
		{
			// flagpole: just rotate and burn graphics
			ruin = this;
			SetClrModulation(0xff404040);
			var flag_r = 15;
			var flag_z = 990;
			var flag_yoff = 1000;
			SetObjDrawTransform(Cos(flag_r, flag_z), Sin(flag_r, flag_z), 0, -Sin(flag_r, flag_z), Cos(flag_r, flag_z), flag_yoff);
		}
		else
		{
			// other ruins have a separate ID
			ruin = CreateObjectAbove(fx.ruin_id, 0, GetDefBottom()-GetY(), GetOwner());
		}
		// lots of smoke!
		var particles = Particles_Smoke(true);
		particles.Size = PV_Linear(PV_Random(50, 100), PV_Random(100, 200));
		CreateParticle("Smoke", PV_Random(-30, 30), PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(-20, 0), PV_Random(300, 700), particles, 20);
		// additional flints will fall during the attack
		// make sure ruins don't just disappear
		ruin->MakeInvincible();
		// cast some fire at predefined positions on the ruin
		for (var fire in fx.fire_positions)
		{
			// smoke for some time
			ruin->AddScorch(fire.x, fire.y, fire.r, fire.s, fire.t);
			// cast burning wood
			for (i=-20; i<=20; i+= 20)
			{
				var item = CreateObjectAbove(Wood, fire.x, fire.y, GetOwner());
				if (item)
				{
					item->SetR(Random(360));
					item->SetRDir(Random(21)-10);
					item->Incinerate();
					item->SetXDir(Sin(fire.r + i, fire.s/2));
					item->SetYDir(-Cos(fire.r + i, fire.s/2));
				}
			}
			particles.Size = PV_Linear(fire.s, PV_Random(100, 200));
			CreateParticle("SmokeDirty", PV_Random(fire.x - 10, fire.x + 10), PV_Random(fire.y - 10, fire.y + 10), PV_Random(-20, 20), PV_Random(-20, 0), PV_Random(300, 400), particles, 3);
		}
		// remove non-ruin building (except flagpole)
		if (ruin != this)
		{
			RemoveObject();
		}
		else
		{
			RemoveEffect("IntRuinOnDamage", this);
		}
		return 0;
	}
	return dmg;
}

global func AddScorch(int x, int y, int r, int strength, int duration)
{
	var scorch = CreateObjectAbove(Wood, x, y, NO_OWNER);
	if (!scorch) return nil;
	scorch->SetObjectLayer(scorch);
	scorch->SetR(r);
	scorch->SetClrModulation(0xc0804000);
	scorch->SetCategory(C4D_StaticBack);
	scorch.Collectible = false; // SetObjectLayer is not enough...
	scorch.Plane = this.Plane + 1;
	var fx = AddEffect("FireScorching", scorch, 1, 2, scorch);
	fx.strength = strength;
	fx.duration = duration;
	return scorch;
}

global func FxFireScorchingTimer(object target, proplist effect, int time)
{
	if (time >= effect.duration) { RemoveObject(); return FX_Execute_Kill; }
	// particles
	var wind = BoundBy(GetWind(), -5, 5);
	CreateParticle("SmokeDirty", PV_Random(-5, 5), PV_Random(-5, 5), wind, -effect.strength/8, PV_Random(20, 40), Particles_SmokeTrail(), 2);
	return FX_OK;
}
