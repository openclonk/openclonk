

global func BlueExplode(int level)
{
	// Shake the viewport.
	ShakeViewport(level, GetX(), GetY());

	// Sound must be created before object removal, for it to be played at the right position.
	var grade = BoundBy(level / 10 - 1, 1, 3);
	Sound(Format("Fire::Blast%d", grade), false);

	// Explosion parameters.
	var x = GetX(), y = GetY();
	var cause_plr = GetController();
	var container = Contained();
	var exploding_id = GetID();
	var layer = GetObjectLayer();

	// Explosion parameters saved: Remove object now, since it should not be involved in the explosion.
	RemoveObject();

	// Execute the explosion in global context.
	// There is no possibility to interact with the global context, apart from GameCall.
	// So at least remove the object context.
	exploding_id->DoBlueExplosion(x, y, level, container, cause_plr, layer);
	return;
}

global func DoBlueExplosion(int x, int y, int level, object inobj, int cause_plr, object layer)
{
	// Container to ContainBlast
	var container = inobj;
	while (container)
	{
		if (container.ContainBlast)
			break;
		else
			container = container->Contained();
	}

	// Explosion outside: Explosion effects.
	if (!container)
	{
		// Incinerate oil.
		if (!IncinerateLandscape(x, y, cause_plr))
			if (!IncinerateLandscape(x, y - 10, cause_plr))
				if (!IncinerateLandscape(x - 5, y - 5, cause_plr))
					IncinerateLandscape(x + 5, y - 5, cause_plr);
		// Graphic effects.
		BlueExplosionEffect(level, x, y);
	}
	// Damage in the objects, and outside.
	BlastObjectsBlue(x + GetX(), y + GetY(), level, inobj, cause_plr, layer);
	if (inobj != container)
		BlastObjectsBlue(x + GetX(), y + GetY(), level, container, cause_plr, layer);
	
	// Landschaft zerstören. Nach BlastObjects, damit neu freigesprengte Materialien nicht betroffen sind
	if (!container)
		BlastFree(x, y, level, cause_plr);

	return true;
}

global func BlueExplosionEffect(int level, int x, int y)
{
	var glimmer =
	{
		Prototype = Particles_Glimmer(),
		R = PV_Random(100, 150),
		G = PV_Random(100, 150),
		B = PV_Random(200, 255)
	};
	var fire =
	{
		Prototype = Particles_Fire(),
		R = PV_Random(100, 150),
		G = PV_Random(100, 150),
		B = PV_Random(200, 255),
		Size = PV_Random(level/2, level/3)
	};
	var smoke =
	{
		Prototype = Particles_SmokeTrail(),
		R = PV_Linear(PV_Random(100, 150), 0),
		G = PV_Linear(PV_Random(100, 150), 0),
		B = PV_Linear(PV_Random(200, 255), 0),
		Size = PV_Random(level - 5, level + 5)
	};
	// Blast particle.
	CreateParticle("SmokeDirty", x, y, PV_Random(-2, 2), PV_Random(-2, 2), PV_Random(20, 40), smoke, 10);
	CreateParticle("MagicFire", x, y, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(5, 10), fire, 20);
	if (!GBackLiquid(x, y)) CreateParticle("SphereSpark", x, y, PV_Random(-100, 100), PV_Random(-100, 100), PV_Random(5, 36 * 3), glimmer, level);

	if (GBackLiquid(x, y)) CastObjects(Fx_Bubble, level * 4 / 10, level, x, y);
	return;
}

global func BlastObjectsBlue(int x, int y, int level, object container, int cause_plr, object layer)
{
	var obj;
	
	// Coordinates are always supplied globally, convert to local coordinates.
	var l_x = x - GetX(), l_y = y - GetY();
	
	// In a container?
	if (container)
	{
		if (container->GetObjectLayer() == layer)
		{
			container->BlastObject(level, cause_plr);
			if (!container)
				return true; // Container could be removed in the meanwhile.
			for (obj in FindObjects(Find_Container(container), Find_Layer(layer)))
				if (obj)
					obj->BlastObject(level, cause_plr);
		}
	}
	else
	{
		// Object is outside.
		// Damage objects at point of explosion.
		for (var obj in FindObjects(Find_AtRect(l_x - 5, l_y - 5, 10, 10), Find_NoContainer(), Find_Layer(layer), Find_Not(Find_ID(StoneDoor))))
			if (obj) obj->BlastObject(level, cause_plr);

		// TODO: -> Shockwave in own global func(?)

		// Hurl objects in explosion radius.
		var shockwave_objs = FindObjects(Find_Distance(level, l_x, l_y), Find_NoContainer(), Find_Layer(layer),
			Find_Or(Find_Category(C4D_Object | C4D_Living | C4D_Vehicle), Find_Func("CanBeHitByShockwaves")), Find_Func("BlastObjectsShockwaveCheck", x, y));
		var cnt = GetLength(shockwave_objs);
		if (cnt)
		{
			// The hurl energy is distributed over the objects.
			//Log("Shockwave objs %v (%d)", shockwave_objs, cnt);
			var shock_speed = Sqrt(2 * level * level / BoundBy(cnt, 2, 12));
			for (var obj in shockwave_objs)
				if (obj) // Test obj, cause OnShockwaveHit could have removed objects.
				{
					// Object has special reaction on shockwave?
					if (obj->~OnShockwaveHit(level, x, y))
						continue;
					// Living beings are hurt more.
					var cat = obj->GetCategory();
					if (cat & C4D_Living)
					{
						obj->DoEnergy(level / -2, false, FX_Call_EngBlast, cause_plr);
						obj->DoDamage(level / 2, FX_Call_DmgBlast, cause_plr);
					}
					// Killtracing for projectiles.
					if (cat & C4D_Object)
						obj->SetController(cause_plr);
					// Shockwave.
					var mass_fact = 20, mass_mul = 100;
					if (cat & C4D_Living)
					{
						mass_fact = 8;
						mass_mul = 80;
					}
					mass_fact = BoundBy(obj->GetMass() * mass_mul / 1000, 4, mass_fact);
					var dx = 100 * (obj->GetX() - x) + Random(51) - 25;
					var dy = 100 * (obj->GetY() - y) + Random(51) - 25;
					var vx, vy;
					if (dx)
						vx = Abs(dx) / dx * (100 * level - Abs(dx)) * shock_speed / level / mass_fact;
					vy = (Abs(dy) - 100 * level) * shock_speed / level / mass_fact;
					if (cat & C4D_Object)
					{
						// Objects shouldn't move too fast.
						var ovx = obj->GetXDir(100), ovy = obj->GetYDir(100);
						if (ovx * vx > 0)
							vx = (Sqrt(vx * vx + ovx * ovx) - Abs(vx)) * Abs(vx) / vx;
						if (ovy * vy > 0)
							vy = (Sqrt(vy * vy + ovy * ovy) - Abs(vy)) * Abs(vy) / vy;
					}
					//Log("%v v(%v %v)   d(%v %v)  m=%v  l=%v  s=%v", obj, vx, vy, dx, dy, mass_fact, level, shock_speed);
					obj->Fling(vx, vy, 100, true);
				}
		}
	}
	// Done.
	return true;
}
