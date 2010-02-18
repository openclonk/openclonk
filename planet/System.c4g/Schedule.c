

// Einen Script zeitverzögert und ggf. wiederholt ausführen
global func Schedule(string strScript, int iInterval, int iRepeat, object pObj)
{
  
  // Default
  if(!iRepeat) iRepeat = 1;
  if(!pObj) pObj = this;
  // Effekt erzeugen
  var iEffect = AddEffect("IntSchedule", pObj, 1, iInterval, pObj);
  if(iEffect <= 0) return false;
  // Variablen setzen
  EffectVar(0, pObj, iEffect) = strScript;
  EffectVar(1, pObj, iEffect) = iRepeat;
  return true;
}

global func FxIntScheduleTimer(object pObj,  int iEffect)
{
  // Nur eine bestimmte Anzahl Ausführungen
  var fDone = (--EffectVar(1, pObj, iEffect) <= 0);
  // Ausführen
  eval(EffectVar(0, pObj, iEffect));
  return -fDone;
}

// Eine Funktion zeitverzögert und ggf. wiederholt aufrufen
global func ScheduleCall(object pObj, string strFunction, int iInterval, int iRepeat, par0, par1, par2, par3, par4)
{
  // Default
  if(!iRepeat) iRepeat = 1;
  if(!pObj) pObj = this;
  // Effekt erzeugen
  var iEffect = AddEffect("IntScheduleCall", pObj, 1, iInterval, pObj);
  if(iEffect <= 0) return false;
  // Variablen setzen
  EffectVar(0, pObj, iEffect) = strFunction;
  EffectVar(1, pObj, iEffect) = iRepeat;
  // EffectVar(2): Nur zur Abwärtskompatibilität reserviert
  EffectVar(2, pObj, iEffect) = pObj; 
  for(var i = 0; i < 5; i++)
    EffectVar(i + 3, pObj, iEffect) = Par(i + 4);
  return true;
}

global func FxIntScheduleCallTimer(object pObj, int iEffect)
{
  // Nur eine bestimmte Anzahl Ausführungen
  var fDone = (--EffectVar(1, pObj, iEffect) <= 0);
  // Ausführen
  Call(EffectVar(0, pObj, iEffect), EffectVar(3, pObj, iEffect), EffectVar(4, pObj, iEffect), EffectVar(5, pObj, iEffect), EffectVar(6, pObj, iEffect), EffectVar(7, pObj, iEffect));
  // Nur eine bestimmte Anzahl Ausführungen
  return (-fDone);
}

global func ClearScheduleCall(object pObj, string strFunction)
{
  var i, iEffect;
  // Von Effektzahl abwärts zählen, da Effekte entfernt werden
  i = GetEffectCount("IntScheduleCall", pObj);
  while (i--)
    // Alle ScheduleCall-Effekte prüfen
    if (iEffect = GetEffect("IntScheduleCall", pObj, i))
      // Gesuchte Zielfunktion
      if (EffectVar(0, pObj, iEffect) == strFunction)      
        // Effekt löschen
        RemoveEffect(0, pObj, iEffect);
}
