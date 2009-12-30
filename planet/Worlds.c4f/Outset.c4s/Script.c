#strict 2

static clonk;

func Initialize()
{
  //CreateObject(MONS, LandscapeWidth()/2, LandscapeHeight()/2);

  clonk = CreateObject(CLNK, LandscapeWidth()/2, LandscapeHeight()/4);
  clonk->SetAction("Idle");
  clonk->AnimationPlay("Walk", 1000);
  clonk->SetObjDrawTransform(500,0,500,0,1000,0);
  AddEffect("IntAnimPlay", clonk, 1, 1);
}

global func FxIntAnimPlayTimer(object target, int number, int time)
{
  var walk_pos = (time % 50) * 2400 / 50; // Walk animation ranges to 2400
  if(!EffectVar(0, target, number))
  {
    // Let the walk animation last 50 frames
    target->AnimationSetState("Walk", walk_pos);
    if(!Random(100))
    {
      // Transition to Jump animation
      EffectVar(0, target, number) = time;
      target->AnimationPlay("Jump", 0);
    }
  }
  else
  {
    // Jump animation lasts 20 frames
    if(time - EffectVar(0, target, number) <= 20)
    {
      var off = time - EffectVar(0, target, number);
      var walk_weight = 1000 - 50*off;
      var jump_pos = off * 1000 / 20; // Jump animation ranges to 1000
      target->AnimationSetState("Walk", walk_pos, walk_weight);
      target->AnimationSetState("Jump", jump_pos, 1000 - walk_weight);
    }
    else
    {
      // Hold until 50 frames
      if(time - EffectVar(0, target, number) > 50)
      {
        target->AnimationSetState("Walk", walk_pos, 1000);
        target->AnimationStop("Jump");
        EffectVar(0, target, number) = 0;
      }
    }
  }
}
