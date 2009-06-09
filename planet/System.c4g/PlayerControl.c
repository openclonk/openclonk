#strict 2

global func PlayerControl(int plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat)
{
  Log("%d, %d, %i, %d, %d, %d, %v", plr, ctrl, spec_id, x,y,strength, repeat);
  return true;
}

global func PlayerControlRelease(int plr, int ctrl, id spec_id, int x, int y)
{
  Log("re %d, %d, %i, %d, %d", plr, ctrl, spec_id, x,y);
  return true;
}