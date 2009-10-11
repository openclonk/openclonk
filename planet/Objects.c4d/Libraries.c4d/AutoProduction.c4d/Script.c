/*--- Auto production ---*/

#strict 2

public func ControlCommandAcquire(target, x, y, target2, def)
{
  // If the object is lying around just pick it up
  var obj = GetAvailableObject (def, target2);
  if (obj) {
    AddEffect("IntNotAvailable", obj, 1, 5, this);
    AddCommand (this, "Get", obj, 0, 0, 0, 40);
    return 1;
  }
  // Search for a building to produce the object  
  if (obj = GetProducerOf (def)) {
    AddCommand (this (), "Call", this, 0, 0, 0, 0, "AutoProduction", 0, 1);
    obj -> HowToProduce (this, def);
    return 1;
  }
  AddCommand (this, "Buy", 0, 0, 0, 0, 100, def, 0, C4CMD_Sub);
  return 1;
}

public func AutoProduction() { return 1; }

public func AutoProductionFailed() 
{
  var def = GetCommand (this (), 5, 1);
  if (!FindContents(def)) {
    var obj = GetAvailableObject (def, GetCommand (this (), 4, 1));
    if (obj) {
      AddEffect("IntNotAvailable", obj, 1, 5, this);
      AddCommand (this, "Get", obj,0,0,0,40);
      return 1;
    }
    AddCommand (this, "Buy", 0, 0, 0, 0, 100, GetCommand(this, 5, 1), 0, C4CMD_Sub);
  }
  return 1;
}

public func FxIntNotAvailableStart(target, number)
{
  EffectVar(0, target, number) = this;
}

public func FxIntNotAvailableTimer(target, number)
{
  var clonk = EffectVar(0, target, number);
  // Check wether the clonk still wants to get the object
  for (var i = 0; GetCommand(clonk,0,i); ++i)  {
    if (GetCommand(clonk, 0, i) == "Get" && GetCommand(clonk, 1, i) == target)
      return;
  }
  return FX_Execute_Kill;
}

public func GetProducerOf(def)
{
  return FindObject2(Find_InRect(-500,-250,1000,500), Find_Func("IsProducerOf", this, def), Sort_Distance());
}
