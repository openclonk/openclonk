

global func BlueExplode(int level)
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
	exploding_id->DoBlueExplosion(x, y, level, container, cause_plr, layer);
	return;
}

global func DoBlueExplosion(int x, int y, int level, object inobj, int cause_plr, object layer)
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
		BlueExplosionEffect(level, x, y);
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

global func BlueExplosionEffect(int level, int x, int y)
{
	// Blast particle.
	CreateParticle("Blast", x, y, 0, 0, level * 10, RGB(0,25,255));
	if(!GBackLiquid(x,y)) CastParticles("Spark", 3, 40 + level, x, y, 20, 30, RGB(0,0,255), RGB(0,30,255));
	if(GBackLiquid(x,y)) CastObjects(Fx_Bubble, level * 4 / 10, level, x, y);
	//CastParticles("FSpark", level/5+1, level, x,y, level*5+10,level*10+10, 0x00ef0000,0xffff1010));

	/*// Smoke trails.
	var i = 0, count = 1 + level / 16, angle = Random(360);
	while (count > 0 && ++i < count * 10)
	{
		angle += RandomX(40, 80);
		var smokex = Sin(angle, RandomX(level / 4, level / 2));
		var smokey = -Cos(angle, RandomX(level / 4, level / 2));
		if (GBackSolid(x + smokex, y + smokey))
			continue;
		//var lvl = 16 * level / 10;
		CreateSmokeTrail(level, angle, x + smokex, y + smokey);
		count--;*/
	}
	return;
}
