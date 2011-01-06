/*--
		Explode.c
		Authors: Newton
		
		Everything about the explosion.
		TODO: documentation.
--*/


/*-- Explosion --*/

global func Explode(int level)
{
	// Shake the viewport.
	ShakeViewPort(level, nil, GetX(), GetY());

	// Sound must be created before object removal, for it to be played at the right position.
	var grade = BoundBy(level / 10 - 1, 1, 3);
	Sound(Format("Blast%d", grade), false);

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
	exploding_id->DoExplosion(x, y, level, container, cause_plr, layer);
	return;
}

global func DoExplosion(int x, int y, int level, object inobj, int cause_plr, object layer)
{
	// Container to ContainBlast
	var container = inobj;
	while (container)
	{
		if (container->GetID()->GetDefContainBlast())
			break;
		else
			container = container->Contained();
	}

	// Explosion outside: Explosion effects.
	if (!container)
	{
		// Incinerate oil.
		if (!IncinerateLandscape(x, y))
			if (!IncinerateLandscape(x, y - 10))
				if (!IncinerateLandscape(x - 5, y - 5))
					IncinerateLandscape(x + 5, y - 5);
		// Graphic effects.
		ExplosionEffect(level, x, y);
	}
	// Damage in the objects, and outside.
	BlastObjects(x + GetX(), y + GetY(), level, inobj, cause_plr, layer);
	if (inobj != container)
		BlastObjects(x + GetX(), y + GetY(), level, container, cause_plr, layer);
	
	// Landschaft zerstören. Nach BlastObjects, damit neu freigesprengte Materialien nicht betroffen sind
	if (!container)
		BlastFree(x, y, level, cause_plr);

	return true;
}

global func ExplosionEffect(int level, int x, int y)
{
	// Blast particle.
	CreateParticle("Blast", x, y, 0, 0, level * 10, RGBa(255, 255, 255, 100));
	if(!GBackLiquid(x,y)) CastParticles("Spark", 10, 80 + level, x, y, 35, 40, RGB(255, 200, 0), RGB(255, 255, 150));
	if(GBackLiquid(x,y)) CastObjects(Fx_Bubble, level * 7 / 10, level, x, y);
	//CastParticles("FSpark", level/5+1, level, x,y, level*5+10,level*10+10, 0x00ef0000,0xffff1010));

	// Smoke trails.
	var i = 0, count = 3 + level / 8, angle = Random(360);
	while (count > 0 && ++i < count * 10)
	{
		angle += RandomX(40, 80);
		var smokex = Sin(angle, RandomX(level / 4, level / 2));
		var smokey = -Cos(angle, RandomX(level / 4, level / 2));
		if (GBackSolid(x + smokex, y + smokey))
			continue;
		var lvl = 16 * level / 10;
		CreateSmokeTrail(lvl, angle, x + smokex, y + smokey);
		count--;
	}
	return;
}

/*-- Blast objects & shockwaves --*/

// Damage and hurl objects away.
global func BlastObjects(int x, int y, int level, object container, int cause_plr, object layer)
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
		for (var obj in FindObjects(Find_AtRect(l_x - 5, l_y - 5, 10,10), Find_NoContainer(), Find_Layer(layer)))
			if (obj) obj->BlastObject(level, cause_plr);

		// TODO: -> Shockwave in own global func(?)

		// Hurl objects in explosion radius.
		var shockwave_objs = FindObjects(Find_Distance(level, l_x, l_y), Find_NoContainer(), Find_Layer(layer),
			Find_Or(Find_Category(C4D_Object|C4D_Living|C4D_Vehicle), Find_Func("CanBeHitByShockwaves")), Find_Func("BlastObjectsShockwaveCheck", x, y));
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
					//Log("%v v(%v %v)   d(%v %v)  m=%v  l=%v  s=%v", obj, vx,vy, dx,dy, mass_fact, level, shock_speed);
					obj->Fling(vx, vy, 100, true);
				}
		}
	}
	// Done.
	return true;
}

global func BlastObjectsShockwaveCheck(int x, int y)
{
	var def = GetID();
	// Some special cases, which won't go into FindObjects.
	if (def->GetDefHorizontalFix())
		return false;
	if (def->GetDefGrab() != 1)
	{
		if (GetCategory() & C4D_Vehicle)
			return false;
		if (GetProcedure() == "FLOAT")
			return false;
	}
	// Projectiles not when they fly downwards or are exactly in the explosion point.
	// This will catch the most cases in which multiple clonks throw flints at the same time.
	if (GetCategory() & C4D_Object)
	{
		if (GetX() == x && GetY() == y) return false;
		if (GetYDir() > 5) return false;
	}
	// No stuck objects.
	if (Stuck())
		return false;
	return true;
}


/*-- Shake view port --*/

global func ShakeViewPort(int level, int x_off, int y_off)
{
	if (level <= 0)
		return false;

	var eff = GetEffect("ShakeEffect", this);

	if (eff)
	{
		eff.var0 += level;
		return true;
	}

	eff = AddEffect("ShakeEffect", this, 200, 1);
	if (!eff)
		return false;

	eff.var0 = level;

	if (x_off || y_off)
	{
		eff.var1 = x_off;
		eff.var2 = y_off;
	}
	else
	{
		eff.var1 = GetX();
		eff.var2 = GetY();
	}
	return true;
}

// Variables:
// 0 - level
// 1 - x-pos
// 2 - y-pos

// Duration of the effect: as soon as strength==0
// Strength of the effect: strength=level/(1.5*fxtime+3)-fxtime^2/400

global func FxShakeEffectTimer(object target, effect, int fxtime)
{
	var strength;

	var str = effect.var0;
	var xpos = effect.var1;
	var ypos = effect.var2;


	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		var cursor = GetCursor(plr);
		if (!cursor)
			continue;
		var distance = Distance(cursor->GetX(), cursor->GetY(), xpos, ypos);

		// Shake effect lowers as a function of the distance.
		var level = (300 * str) / Max(300, distance);

		if ((strength = level / ((3 * fxtime) / 2 + 3) - fxtime**2 / 400) <= 0)
			continue;

		// FixME: Use GetViewOffset, make this relative, not absolute
		SetViewOffset(plr, Sin(fxtime * 100, strength), Cos(fxtime * 100, strength));
	}

	if (str / ((3 * fxtime) / 2 + 3) - fxtime**2 / 400 <= 0)
		return -1;
}

global func FxShakeEffectStart(object target, effect)
{
	FxShakeEffectTimer(target, effect, effect.Time);
}

global func FxShakeEffectStop()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		// FxME: Return the offset to the previous value, not zero
		SetViewOffset(GetPlayerByIndex(i), 0, 0);
	}
}

/*-- Smoke trails --*/

global func CreateSmokeTrail(int strength, int angle, int x, int y, int color, bool noblast) {
	x += GetX();
	y += GetY();
	var num = AddEffect("SmokeTrail", nil, 300, 1, nil, nil, strength, angle, x, y);
	if (!color)
		color = RGBa(130, 130, 130, 70);
	num.var6 = color;
	num.var7 = noblast;
	return;
}

// Variables:
// 0 - Strength
// 1 - Current strength
// 2 - X-Position
// 3 - Y-Position
// 4 - Starting-X-Speed
// 5 - Starting-Y-Speed
// 6 - color [opt]
// 7 - no blast
global func FxSmokeTrailStart(object target, effect, int temp, strength, angle, x, y)
{
	if (temp)
		return;
	
	if (angle % 90 == 1)
		angle += 1;
	strength = Max(strength, 5);

	effect.var0 = strength;
	effect.var1 = strength;
	effect.var2 = x;
	effect.var3 = y;
	effect.var4 = Sin(angle, strength * 40);
	effect.var5 = -Cos(angle, strength * 40);
}

global func FxSmokeTrailTimer(object target, effect, int fxtime)
{
	var strength = effect.var0;
	var str = effect.var1;
	var x = effect.var2;
	var y = effect.var3;
	var x_dir = effect.var4;
	var y_dir = effect.var5;
	var color = effect.var6;

	str = Max(1, str - str / 5);
	str--;
	y_dir += GetGravity() * 2 / 3;

	var x_dir = x_dir * str / strength;
	var y_dir = y_dir * str / strength;

	// new: random
	x += RandomX(-3,3);
	y += RandomX(-3,3);
	
	// draw
	CreateParticle("ExploSmoke", x, y, RandomX(-2, 2), RandomX(-2, 4), 150 + str * 12, color);
	if (!effect.var7)
		CreateParticle("Blast", x, y, 0, 0, 10 + str * 8, RGBa(255, 100, 50, 150));

	// then calc next position
	x += x_dir / 100;
	y += y_dir / 100;
	
	if (GBackSemiSolid(x, y))
		return -1;
	if (str <= 3)
		return -1;
	
	effect.var1 = str;
	effect.var2 = x;
	effect.var3 = y;
	effect.var5 = y_dir;
}

/*-- Fireworks --*/

global func Fireworks(int color, int x, int y)
{
	if (!color)
		color = HSL(Random(8) * 32, 255, 127);
	
	var speed = 12;
	for (var i = 0; i < 36; ++i)
	{
		var oangle = Random(70);
		var num = AddEffect("Firework", nil, 300, 1, nil, nil, Cos(oangle,speed), i * 10 + Random(5), x + GetX(), y + GetY());
		num.var4 = color;
	}
	
	for (var i = 0; i < 16; ++i)
	{
		CreateParticle("ExploSmoke", RandomX(-80, 80), RandomX(-80, 80), 0, 0, RandomX(500, 700), RGBa(255, 255, 255, 90));
	}
	CastParticles("Spark", 60, 190, 0, 0, 40, 70, color, color);
	
	CreateParticle("Flash", 0, 0, 0, 0, 3500, color | (200 & 255) << 24);
	return;
}

global func FxFireworkStart(object target, effect, int tmp, speed, angle, x, y, color)
{
	if (tmp)
		return;

	effect.var0 = speed * 100;
	effect.var1 = angle;
	effect.var2 = x * 100;
	effect.var3 = y * 100;
}

global func FxFireworkTimer(object target, effect, int time)
{
	var speed = effect.var0;
	var angle = effect.var1;
	var x = effect.var2;
	var y = effect.var3;
	
	if (time > 65) return -1;
	
	if (GBackSemiSolid(x / 100, y / 100))
		return -1;
	
	// loose speed
	speed = 25 * speed / 26;
	
	var x_dir = Sin(angle, speed);
	var y_dir = -Cos(angle, speed);
	
	CreateParticle("Flash", x / 100, y / 100, x_dir / 100, y_dir / 100, 50, effect.var4 | (200 & 255) << 24);
	
	// gravity
	y_dir += GetGravity() * 18 / 100;
	
	effect.var0 = speed;
	effect.var1 = Angle(0, 0, x_dir, y_dir);
	effect.var2 = x + x_dir;
	effect.var3 = y + y_dir;
}
