/* Everything about the explosion */

// TODO: many comments are still in German

// TODO: docs

#strict 2

global func Explode(int iLevel) {
  // Viewport wackeln
  ShakeViewPort(iLevel, nil, GetX(), GetY());

  // Sound muss vor dem Löschen des Objektes erzeugt werden, damit die Position stimmt
  var grade = BoundBy((iLevel/10)-1,1,3);
  Sound(Format("Blast%d", grade), false);

  // Explosionsparameter
  var x=GetX(), y=GetY();
  var cause_plr = GetController();
  var container = Contained();
  var exploding_id = GetID();
  var layer = GetObjectLayer();

  // Explosionsparameter gesichert: Jetzt das Objekt entfernen, damit es von der Explosion selber nicht betroffen ist
  RemoveObject();

  // Und die Explosion im globalen Kontext ausführen
  // Leider gibt es keine Möglichkeit, auf den globalen Kontext zuzugreifen (außer GameCall, aber das löst die Funktion immer neu auf)
  // Also zumindest den Objektkontext entfernen
  exploding_id->DoExplosion(x, y, iLevel, container, cause_plr, layer);
}

global func DoExplosion(int x, int y, int level, object inobj, int cause_plr, object layer)
{
	// Container to ContainBlast
	var container = inobj;
	while(container)
	{
		if(container->GetID()->GetDefContainBlast()) break;
		else container = container->Contained();
	}

	// Explosion outside: Explosion effects
	if (!container)
	{
    // incinerate oil
    if (!IncinerateLandscape(x,y))
      if (!IncinerateLandscape(x,y-10))
        if (!IncinerateLandscape(x-5,y-5))
          IncinerateLandscape(x+5,y-5);

    // graphic effects:

	// blast particle
	CreateParticle("Blast", x,y, 0,0, level*10, RGBa(255,255,255,100));
	CastParticles("Spark",10,80+level,x,y,35,40,RGB(255,200,0),RGB(255,255,150));
	//CastParticles("FSpark", level/5+1, level, x,y, level*5+10,level*10+10, 0x00ef0000,0xffff1010));

	// smoke trails
	var i=0, count = 3+level/8, angle = Random(360);
	while(count > 0 && ++i < count*10) {
	  angle += RandomX(40,80);
	  var smokex = +Sin(angle,RandomX(level/4,level/2));
	  var smokey = -Cos(angle,RandomX(level/4,level/2));
	  if(GBackSolid(x+smokex,y+smokey))
	    continue;
	  var lvl = 16 * level / 10;
	  CreateSmokeTrail(lvl,angle,x+smokex,y+smokey);
	  count--;
	}

  }

  // Schaden in den Objekten bzw. draußen
  BlastObjects(x+GetX(),y+GetY(), level, inobj, cause_plr, layer);
  if (inobj != container) BlastObjects(x+GetX(),y+GetY(), level, container, cause_plr, layer);

  // Landschaft zerstören. Nach BlastObjects, damit neu freigesprengte Materialien nicht betroffen sind
  if (!container)
    {
    BlastFree(x,y, level, cause_plr);
    }

  return true;
  }


/* ----------------------- Blast objects & shockwaves  --------------------- */

// Objekte beschädigen und wegschleudern
global func BlastObjects(int x, int y, int level, object container, int cause_plr, object layer)
  {
  var obj;
  
  // Koordinaten sind immer global angegeben. In lokale Koordinaten umrechnen
  var l_x = x - GetX(), l_y = y - GetY();
  
  // Im Container?
  if (container)
    {
    if (container->GetObjectLayer() == layer)
      {
      container->BlastObject(level, cause_plr);
      if (!container) return true; // Container koennte inzwischen entfernt worden sein
      for (obj in FindObjects(Find_Container(container), Find_Layer(layer)))
        if (obj) obj->BlastObject(level, cause_plr);
      }
    }
  else
    {
    // Objekt ist draußen
    // Objekte am Explosionspunkt beschädigen
    for (var obj in FindObjects(Find_AtRect(l_x-5, l_y-5, 10,10), Find_NoContainer(), Find_Layer(layer)))
      if (obj) obj->BlastObject(level, cause_plr);

		// TODO: -> Shockwave in own global func(?)
 
		// Objekte im Explosionsradius schleudern
    var shockwave_objs = FindObjects(Find_Distance(level, l_x,l_y), Find_NoContainer(), Find_Layer(layer),
        Find_Or(Find_Category(C4D_Object|C4D_Living|C4D_Vehicle), Find_Func("CanBeHitByShockwaves")), Find_Func("BlastObjectsShockwaveCheck",x,y));
    var cnt = GetLength(shockwave_objs);
    if (cnt)
      {
      // Die Schleuderenergie teilt sich bei vielen Objekten auf
      //Log("Shockwave objs %v (%d)", shockwave_objs, cnt);
      var shock_speed = Sqrt(2 * level * level / BoundBy(cnt, 2, 12));
      for (var obj in shockwave_objs) if (obj) // obj noch prüfen, weil OnShockwaveHit Objekte aus dem Array löschen könnte
        {
        // Objekt hat benutzerdefinierte Reaktion auf die Schockwelle?
        if (obj->~OnShockwaveHit(level, x,y)) continue;
        // Lebewesen leiden besonders
        var cat = obj->GetCategory();
        if (cat & C4D_Living)
          {
          obj->DoEnergy(level/-2, false, FX_Call_EngBlast, cause_plr);
          obj->DoDamage(level/2, FX_Call_DmgBlast, cause_plr);
          }
        // Killverfolgung bei Projektilen
        if (cat & C4D_Object) obj->SetController(cause_plr);
        // Schockwelle
        var mass_fact = 20, mass_mul = 100; if (cat & C4D_Living) { mass_fact = 8; mass_mul = 80; }
        mass_fact = BoundBy(obj->GetMass()*mass_mul/1000, 4, mass_fact);
        var dx = 100*(obj->GetX()-x)+Random(51)-25;
        var dy = 100*(obj->GetY()-y)+Random(51)-25;
        var vx, vy;
        if (dx)
          {
          vx = Abs(dx)/dx * (100*level-Abs(dx)) * shock_speed / level / mass_fact;
          }
        vy = (Abs(dy) - 100*level) * shock_speed / level / mass_fact;
        if (cat & C4D_Object)
          {
          // Objekte nicht zu schnell werden lassen
          var ovx = obj->GetXDir(100), ovy = obj->GetYDir(100);
          if (ovx*vx > 0) vx = (Sqrt(vx*vx + ovx*ovx) - Abs(vx)) * Abs(vx)/vx;
          if (ovy*vy > 0) vy = (Sqrt(vy*vy + ovy*ovy) - Abs(vy)) * Abs(vy)/vy;
          }
        //Log("%v v(%v %v)   d(%v %v)  m=%v  l=%v  s=%v", obj, vx,vy, dx,dy, mass_fact, level, shock_speed);
        obj->Fling(vx,vy, 100, true);
        }
      }
    }
  // Fertig
  return true;
  }
  
global func BlastObjectsShockwaveCheck(int x, int y)
  {
  var def = GetID();
  // Einige Spezialfälle, die schwer in FindObjects passen
  if (def->GetDefHorizontalFix()) return false;
  if (def->GetDefGrab() != 1)
    {
    if (GetCategory() & C4D_Vehicle) return false;
    if (GetProcedure() == "FLOAT") return false;
    }
  // Projektile nicht wenn sie nach unten fliegen oder exakt am Explosionsort liegen
  // Dies fängt die meisten Fälle ab, in denen mehrere Clonks gleichzeitig Flints werfen
  if (GetCategory() & C4D_Object)
    {
    if (GetX() == x && GetY() == y) return false;
    if (GetYDir() > 5) return false;
    }
  // Und keine feststeckenden Objekte
  if (Stuck()) return false;
  return true;
  }


/* ---------------------------- Shake view port  --------------------------- */

global func ShakeViewPort(int iLevel, int iOffX, int iOffY) {
  if(iLevel <= 0) return false;

  var eff=GetEffect("ShakeEffect",this);

  if(eff)
	{
    EffectVar(0,this,eff)+=iLevel;
    return true;
  }

  eff=AddEffect("ShakeEffect",this,200,1);
  if (!eff) return false;

  EffectVar(0,this,eff)=iLevel;

  if(iOffX || iOffY)
	{
    EffectVar(1,this,eff)=iOffX;
    EffectVar(2,this,eff)=iOffY;
  }
	else
	{
    EffectVar(1,this,eff)=GetX();
    EffectVar(2,this,eff)=GetY();
  }
  return true;
}

// Variables:
// 0 - level
// 1 - x-pos
// 2 - y-pos

// Duration of the effect: as soon as iStrength==0
// Strength of the effect: iStrength=iLevel/(1.5*iTime+3)-iTime^2/400

global func FxShakeEffectTimer(object pTarget, int iEffectNumber, int iTime) {

  var iStrength;

	var str = EffectVar(0,pTarget,iEffectNumber);
	var xpos = EffectVar(1,pTarget,iEffectNumber);
	var ypos = EffectVar(2,pTarget,iEffectNumber);


  for(var i=0; i<GetPlayerCount(); i++) {
    var iPlr=GetPlayerByIndex(i);
		var distance = Distance(GetCursor(iPlr)->GetX(),GetCursor(iPlr)->GetY(),xpos,ypos);

    // Schütteleffekt verringert sich je nach Entfernung
    var iLevel= (300*str) / Max(300,distance);

    if((iStrength=iLevel/((3*iTime)/2 + 3) - iTime**2/400) <= 0) continue;

    // FIXME: Use GetViewOffset, make this relative, not absolute
    SetViewOffset(iPlr,Sin(iTime*100,iStrength),Cos(iTime*100,iStrength));
  }

  if(EffectVar(0,pTarget,iEffectNumber)/((3*iTime)/2 + 3) - iTime**2/400 <= 0) return -1;
}

global func FxShakeEffectStart(object pTarget, int iEffectNumber) {
  FxShakeEffectTimer(pTarget, iEffectNumber, GetEffect (nil, pTarget, iEffectNumber, 6));
}

global func FxShakeEffectStop() {
  for(var i=0; i<GetPlayerCount(); i++) {
    // FIXME: Return the offset to the previous value, not zero
    SetViewOffset(GetPlayerByIndex(i),0,0);
  }
}

/* ----------------------------- Smoke trails ------------------------------ */

global func CreateSmokeTrail(int iStrength, int iAngle, int iX, int iY) {
    iX += GetX();
    iY += GetY();
  AddEffect("SmokeTrail", nil, 300, 1, nil, nil, iStrength, iAngle, iX, iY);
}

// Variables:
// 0 - Strength
// 1 - Current strength
// 2 - X-Position
// 3 - Y-Position
// 4 - Starting-X-Speed
// 5 - Starting-Y-Speed
global func FxSmokeTrailStart(object pTarget, int iEffectNumber, int iTemp, iStrength, iAngle, iX, iY) {

  if(iTemp)
    return;
  
  if(iAngle%90 == 1) iAngle += 1;
  iStrength = Max(iStrength,5);

  EffectVar(0, pTarget, iEffectNumber) = iStrength;
  EffectVar(1, pTarget, iEffectNumber) = iStrength;
  EffectVar(2, pTarget, iEffectNumber) = iX;
  EffectVar(3, pTarget, iEffectNumber) = iY;
  EffectVar(4, pTarget, iEffectNumber) = +Sin(iAngle,iStrength*40);
  EffectVar(5, pTarget, iEffectNumber) = -Cos(iAngle,iStrength*40);
}

global func FxSmokeTrailTimer(object pTarget, int iEffectNumber, int iEffectTime) {
  var iStrength = EffectVar(0, pTarget, iEffectNumber);
  var iAStr = EffectVar(1, pTarget, iEffectNumber);
  var iX = EffectVar(2, pTarget, iEffectNumber);
  var iY = EffectVar(3, pTarget, iEffectNumber);
  var iXDir = EffectVar(4, pTarget, iEffectNumber);
  var iYDir = EffectVar(5, pTarget, iEffectNumber);

  iAStr = Max(1,iAStr-iAStr/5);
  iAStr--;
  iYDir += GetGravity()*2/3;

  var xdir = iXDir*iAStr/iStrength;
  var ydir = iYDir*iAStr/iStrength;

  // new: random
  iX += RandomX(-3,3);
  iY += RandomX(-3,3);
  
  // draw
  CreateParticle("ExploSmoke",iX,iY,RandomX(-2,2),RandomX(-2,4),150+iAStr*12,RGBa(130,130,130,90));
  CreateParticle("Blast",iX,iY,0,0,10+iAStr*8,RGBa(255,100,50,150));

  // then calc next position
  iX += xdir/100;
  iY += ydir/100;
  
  if(GBackSemiSolid(iX,iY))
    return -1;
  if(iAStr <= 3)
    return -1;
    
  EffectVar(1, pTarget, iEffectNumber) = iAStr;
  EffectVar(2, pTarget, iEffectNumber) = iX;
  EffectVar(3, pTarget, iEffectNumber) = iY;
  EffectVar(5, pTarget, iEffectNumber) = iYDir;
}
