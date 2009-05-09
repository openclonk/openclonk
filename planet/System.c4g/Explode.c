/* Explode, BlastObjects und Sichtfenster-Kamera-Wackel-Effekt */
#strict 2

global func Explode(int iLevel, object pObj, id idEffect, string szEffect) {
  // Viewport wackeln
  ShakeViewPort(iLevel, 0, GetX(pObj), GetY(pObj));
  // Sound muss vor dem Löschen des Objektes erzeugt werden, damit die Position stimmt
  var grade = BoundBy((iLevel/10)-1,1,3);
  Sound(Format("Blast%d", grade), false, pObj);
  // Explosionsparameter
  if (!pObj) if (!(pObj = this)) return false;
  var x=GetX(pObj), y=GetY(pObj);
  var cause_plr = GetController(pObj);
  var container = Contained(pObj);
  var exploding_id = GetID(pObj);
  var layer = GetObjectLayer(pObj);
  // Explosionsparameter gesichert: Jetzt das Objekt entfernen, damit es von der Explosion selber nicht betroffen ist
  RemoveObject(pObj);
  // Und die Explosion im globalen Kontext ausführen
  // Leider gibt es keine Möglichkeit, auf den globalen Kontext zuzugreifen (außer GameCall, aber das löst die Funktion immer neu auf)
  // Also zumindest den Objektkontext entfernen
  exploding_id->DoExplosion(x, y, iLevel, container, cause_plr, idEffect, szEffect, layer);
}

global func DoExplosion(int x, int y, int level, object inobj, int cause_plr, id effect_def, string effect_particle, object layer)
  {
  // Container nach ContainBlast
  var container = inobj;
  while (container) if (GetDefContainBlast(GetID(container))) break; else container = Contained(container);
  // Explosion draußen: Explosionseffekte erzeugen
  if (!container)
    {
    // Öl anzünden
    if (!IncinerateLandscape(x,y))
      if (!IncinerateLandscape(x,y-10))
        if (!IncinerateLandscape(x-5,y-5))
          IncinerateLandscape(x+5,y-5);
    // Grafikeffekt
		var blast;
    if (effect_particle)
      effect_def = 0;
    else
      effect_particle = "Blast";
    if (effect_def)
      {
      if(blast = CreateConstruction(effect_def, x,y+level, cause_plr, level*5))
				blast->~Activate();
      }
    else
      {
      CreateParticle(effect_particle, x,y, 0,0, level*10);
      if (effect_particle == "Blast")
				if (!CastParticles("FSpark", level/5+1, level, x,y, level*5+10,level*10+10, 0x00ef0000,0xffff1010))
					if(blast = CreateConstruction(FXB1, x,y+level, cause_plr, level*5))
						blast->~Activate();
      }
    }
  // Schaden in den Objekten bzw. draußen
  BlastObjects(x+GetX(),y+GetY(), level, inobj, cause_plr+1, layer);
  if (inobj != container) BlastObjects(x+GetX(),y+GetY(), level, container, cause_plr+1, layer);
  // Landschaft zerstören. Nach BlastObjects, damit neu freigesprengte Materialien nicht betroffen sind
  if (!container)
    {
    BlastFree(x,y, level, cause_plr+1);
    }
  // Fertig!
  return true;
  }
  
// Objekte beschädigen und wegschleudern
global func BlastObjects(int x, int y, int level, object container, int cause_plr_plus_one, object layer)
  {
  var obj;
  
  // Koordinaten sind immer global angegeben. In lokale Koordinaten umrechnen
  var l_x = x - GetX(), l_y = y - GetY();
  
  // Im Container?
  if (container)
    {
    if (GetObjectLayer(container) == layer)
      {
      BlastObject(level, container, cause_plr_plus_one);
      if (!container) return true; // Container koennte inzwischen entfernt worden sein
      for (obj in FindObjects(Find_Container(container), Find_Layer(layer)))
        if (obj) BlastObject(level, obj, cause_plr_plus_one);
      }
    }
  else
    {
    // Objekt ist draußen
    // Objekte am Explosionspunkt beschädigen
    for (var obj in FindObjects(Find_AtRect(l_x-5, l_y-5, 10,10), Find_NoContainer(), Find_Layer(layer)))
      if (obj) BlastObject(level, obj, cause_plr_plus_one);
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
        var cat = GetCategory(obj);
        if (cat & C4D_Living)
          {
          DoEnergy(level/-2, obj, false, FX_Call_EngBlast, cause_plr_plus_one);
          DoDamage(level/2, obj, FX_Call_DmgBlast, cause_plr_plus_one);
          }
        // Killverfolgung bei Projektilen
        if (cat & C4D_Object) SetController(cause_plr_plus_one-1, obj);
        // Schockwelle
        var mass_fact = 20, mass_mul = 100; if (cat & C4D_Living) { mass_fact = 8; mass_mul = 80; }
        mass_fact = BoundBy(GetMass(obj)*mass_mul/1000, 4, mass_fact);
        var dx = 100*(GetX(obj)-x)+Random(51)-25;
        var dy = 100*(GetY(obj)-y)+Random(51)-25;
        var vx, vy;
        if (dx)
          {
          vx = Abs(dx)/dx * (100*level-Abs(dx)) * shock_speed / level / mass_fact;
          }
        vy = (Abs(dy) - 100*level) * shock_speed / level / mass_fact;
        if (cat & C4D_Object)
          {
          // Objekte nicht zu schnell werden lassen
          var ovx = GetXDir(obj, 100), ovy = GetYDir(obj, 100);
          if (ovx*vx > 0) vx = (Sqrt(vx*vx + ovx*ovx) - Abs(vx)) * Abs(vx)/vx;
          if (ovy*vy > 0) vy = (Sqrt(vy*vy + ovy*ovy) - Abs(vy)) * Abs(vy)/vy;
          }
        //Log("%v v(%v %v)   d(%v %v)  m=%v  l=%v  s=%v", obj, vx,vy, dx,dy, mass_fact, level, shock_speed);
        Fling(obj, vx,vy, 100, true);
        }
      }
    }
  // Fertig
  return true;
  }
  
global func BlastObjectsShockwaveCheck(int x, int y)
  {
  var def;
  // Einige Spezialfälle, die schwer in FindObjects passen
  if (GetDefHorizontalFix(def = GetID())) return false;
  if (GetDefGrab(def) != 1)
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

global func ShakeViewPort(int iLevel, object pObj, int iOffX, int iOffY) {
  if(iLevel <= 0) return false;
  var eff=GetEffect("ShakeEffect",pObj);
  if(eff) {
    EffectVar(0,pObj,eff)+=iLevel;
    return true;
  }
  eff=AddEffect("ShakeEffect",pObj,200,1);
  if (!eff) return false;
  EffectVar(0,pObj,eff)=iLevel;
  if(iOffX || iOffY) {
    EffectVar(1,pObj,eff)=iOffX;
    EffectVar(2,pObj,eff)=iOffY;
  } else {
    EffectVar(1,pObj,eff)=GetX(pObj);
    EffectVar(2,pObj,eff)=GetY(pObj);
  }
  return (true);
}

// Dauer des Effektes: sobald iStrength==0
// Stärke des Effektes: iStrength=iLevel/(1.5*iTime+3)-iTime^2/400
global func FxShakeEffectTimer(object pTarget, int iEffectNumber, int iTime) {
  var iPlr, iLevel;
  var iStrength;
  for(var i=0; i<GetPlayerCount(); i++) {
    iPlr=GetPlayerByIndex(i);
    // Schütteleffekt verringert sich je nach Entfernung
    iLevel=(300*EffectVar(0,pTarget,iEffectNumber))/Max(300,Distance(GetX(GetCursor(iPlr)),GetY(GetCursor(iPlr)),EffectVar(1,pTarget,iEffectNumber),EffectVar(2,pTarget,iEffectNumber)));
    if((iStrength=iLevel/((3*iTime)/2+3)-iTime**2/400)<=0) continue;
    // FIXME: Use GetViewOffset, make this relative, not absolute
    SetViewOffset(iPlr,Sin(iTime*100,iStrength),Cos(iTime*100,iStrength));
  }
  if(EffectVar(0,pTarget,iEffectNumber)/((3*iTime)/2+3)-iTime**2/400<=0) return -1;
}

global func FxShakeEffectStart(object pTarget, int iEffectNumber) {
  FxShakeEffectTimer(pTarget, iEffectNumber, GetEffect (0, pTarget, iEffectNumber, 6));
}

global func FxShakeEffectStop() {
  for(var i=0; i<GetPlayerCount(); i++) {
    // FIXME: Return the offset to the previous value, not zero
    SetViewOffset(GetPlayerByIndex(i),0,0);
  }
}
