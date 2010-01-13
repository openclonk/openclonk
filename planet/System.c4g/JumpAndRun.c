/*-- Hilfsfunktionen für JumpAndRun-Steuerung --*/

#strict 2

/* ComDir */

global func ComDirTransform(int comdir, int tocomdir)
{
  if (comdir == tocomdir) return (comdir);
  if (comdir == COMD_Stop) return (tocomdir);
  if (comdir == (tocomdir + 3) % 8 + 1) return (COMD_Stop);
  if (Inside(comdir, tocomdir + 1, tocomdir + 3)) return (comdir - 1);
  if (Inside(comdir, tocomdir - 1, tocomdir - 3)) return (comdir + 1);
  if (Inside(comdir, tocomdir - 7, tocomdir - 5)) return ((comdir + 6) % 8 + 1);
  return (comdir % 8 + 1);
}

global func ComDirLike(int comdir1, int comdir2)
{
  if (comdir1 == comdir2) return (true);
  if (comdir1 == COMD_Stop || comdir2 == COMD_Stop) return false;
  if (comdir1 % 8 + 1 == comdir2) return (true);
  if (comdir1 == comdir2 % 8 + 1) return (true);
  return (false);
}

/* Zielen von Armbrust/Katapult/Bogen/Winchester/etc. */

global func AimStdConf(int conf)
{
  var old_phase = GetPhase();
  SetPhase(BoundBy(GetPhase()+conf, 0, GetActMapVal("Length", GetAction(), GetID())-1));
  return GetPhase() != old_phase;
}

global func AimConf(string conffunc, int conf, object controller, id def)
{
  if(conffunc)
  {
    if(this)
      return ProtectedCall(this, conffunc, conf, controller);
    else if(def)
      return DefinitionCall(def, conffunc, conf, controller);
    else
      // Global call via eval?
      ;
  }

  // Standard über SetPhase
  return AimStdConf();
}

global func AimUpdate(object controller, int comdir, int interval, string conffunc, id def)
{
  // Globaler Effekt, wenn this 0 ist (DefinitionCall).
  if(GetPlrCoreJumpAndRunControl(controller->GetController()))
  {
    AimCancel(controller);

    if(ComDirLike(comdir, COMD_Up))
      AddEffect("IntJnRAim", this, 1, interval, 0, 0, conffunc, -1, controller, def);
    if(ComDirLike(comdir, COMD_Down))
      AddEffect("IntJnRAim", this, 1, interval, 0, 0, conffunc, 1, controller, def);
  }
}

global func AimUp(object controller, int interval, string conffunc, id def)
{
  if(!GetPlrCoreJumpAndRunControl(controller->GetController()))
    return AimConf(conffunc, -1, controller, def);
}

global func AimDown(object controller, int interval, string conffunc, id def)
{
  if(!GetPlrCoreJumpAndRunControl(controller->GetController()))
    return AimConf(conffunc, 1, controller, def);
}

global func AimCancel(object controller)
{
  var count = GetEffectCount("IntJnRAim", this);
  while(count --)
  {
    var number = GetEffect("IntJnRAim", this);
    if(EffectVar(2, this, number) != controller) continue;
    RemoveEffect(0, this, number); 
  }
}

global func FxIntJnRAimStart(object target, int number, int temp, string conffunc, int conf, object controller, id def)
{
  if(temp) return;
  EffectVar(0, target, number) = conffunc;
  EffectVar(1, target, number) = conf;
  EffectVar(2, target, number) = controller;
  EffectVar(3, target, number) = def;
//  EffectVar(4, target, number) = (controller != 0); // Speichert, ob ueberhaupt ein Controller gegeben war
}

global func FxIntJnRAimTimer(object target, int number, int time)
{
  // Controller weg?
  var controller = EffectVar(2, target, number);
  if(!controller) return -1;

  // Controller muss Fahrzeug anfassen
  if(target)
	  // target und controller das selbe Objekt? Kann ja garnicht anfassen.
    if(target->GetCategory() & C4D_Vehicle && target != controller)
      if(controller->GetProcedure() != "PUSH" || controller->GetActionTarget() != target)
        return -1;

  if(target)
    target->AimConf(EffectVar(0, target, number), EffectVar(1, target, number), EffectVar(2, target, number));
  else if(EffectVar(3, target, number))
    AimConf(EffectVar(0, target, number), EffectVar(1, target, number), EffectVar(2, target, number), EffectVar(3, target, number));
  else
    // Global call via eval()?
    ;
}
