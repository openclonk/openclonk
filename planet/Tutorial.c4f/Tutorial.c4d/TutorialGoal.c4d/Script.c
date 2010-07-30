/*-- 
		Tutorial goal
--*/


#include Library_Goal

local finished; // Whether the goal has been reached by some player.
local cp_list; // List of checkpoints.
local cp_count; // Number of checkpoints.
local respawn_checkpoint; // last reached heckpoint that is a respawn position
local direction_help_enabled; // whether player sees an arrow attached to his Clonk

local goal_message, goal_desc; // Current goal description in short and long format

/*-- General --*/

func Initialize()
{
	finished = false;
	cp_list = [];
	cp_count = 0;
	respawn_checkpoint = 0;
	SetGoalMessage(GetName(), GetDesc()); // default messages
	EnableDirectionHelp();
	return _inherited(...);
}

/*-- Checkpoint creation --*/

func SetStartpoint(int x, int y)
{
	var cp = CreateObject(ParkourCheckpoint, x, y, NO_OWNER);
	cp->SetPosition(x, y);
	cp->SetCPMode(PARKOUR_CP_Start);
	cp->SetCPController(this);
	cp_list[0] = cp;
	// Init Respawn CP to start CP.
	if (!respawn_checkpoint) respawn_checkpoint = cp_list[0];
	return cp;
}

func SetFinishpoint(int x, int y)
{
	var cp = CreateObject(ParkourCheckpoint, x, y, NO_OWNER);
	cp->SetPosition(x, y);
	cp->SetCPMode(PARKOUR_CP_Finish);
	cp->SetCPController(this);
	cp_count++;
	cp_list[cp_count] = cp;
	return cp;
}

func AddCheckpoint(int x, int y, string callback_fn)
{
	var mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn;
	var cp = CreateObject(ParkourCheckpoint, x, y, NO_OWNER);
	cp->SetPosition(x, y);
	cp->SetCPMode(mode);
	cp->SetCPController(this);
	cp["TutorialCallback"] = callback_fn;
	cp_count++;
	cp_list[cp_count + 1] = cp_list[cp_count]; // Finish 1 place further.
	cp_list[cp_count] = cp;
	return cp;
}

/*-- Checkpoint interaction --*/

// Called from a finish CP to indicate that plr has reached it.
func PlrReachedFinishCP(int plr, object cp)
{
	if (finished)
		return;
	finished = true;
	return;
}

// Called from a respawn CP to indicate that plr has reached it.
func SetPlrRespawnCP(int plr, object cp)
{
	respawn_checkpoint = cp;
	return;
}

// Called from a check CP to indicate that plr has cleared it.
func AddPlrClearedCP(int plr, object cp)
{
	if (cp && cp["TutorialCallback"])
		GameCall(Format("Checkpoint_%s", cp["TutorialCallback"]), plr, cp);
	return;
}

/*-- Goal interface --*/

func SetGoalMessage(string msg_short, string msg_long)
{
  if (!msg_long) msg_long = msg_short;
  goal_message = msg_short;
  goal_desc = msg_long;
  return true;
}

func IsFulfilled()
{
	return finished;
}

func Activate(int plr)
{
	// Show goal message.
	if (goal_desc) MessageWindow(goal_desc, plr);
	return;
}

func GetShortDescription(int plr)
{
	return goal_message;
}



/*-- Player section --*/

func InitializePlayer(int plr, int x, int y, object base, int team)
{
	JoinPlayer(plr);
	// Scenario script callback.
	return _inherited(plr, x, y, base, team, ...);
}

func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	// Log message.
	Log(RndRespawnMsg(), GetPlayerName(plr));
	return _inherited(plr, ...);
}

private func RndRespawnMsg()
{
	return Translate(Format("MsgRespawn%d", Random(4)));
}

func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var pos = FindRespawnPos(plr);
	clonk->SetPosition(pos[0], pos[1]);
	if (direction_help_enabled) AddEffect("IntDirNextCP", clonk, 100, 1, this);
	// Scenario script callback.
	GameCall("PlrHasSpawned", plr, clonk, respawn_checkpoint);
	return;
}

private func FindRespawnPos(int plr)
{
	if (respawn_checkpoint)
	{
		return [respawn_checkpoint->GetX(), respawn_checkpoint->GetY()];
	}
	// shouldn't happen. Start somewhere...
	return [Random(LandscapeWidth()), 10];
}




/*-- Direction indication --*/

func EnableDirectionHelp()
{
  // add arrow pointer to all player controlled Clonks
  var crew;
  for (var iplr=GetPlayerCount(); iplr; --iplr)
  {
    var i=0;
    while (crew = GetCrew(GetPlayerByIndex(iplr, i++)))
      if (!GetEffect("IntDirNextCP", crew))
        AddEffect("IntDirNextCP", crew, 100, 1, this);
  }
  direction_help_enabled = true;
  return true;
}

func DisableDirectionHelp()
{
  // remove arrow pointer from all player controlled Clonks
  var crew, iplr = GetPlayerCount();
  while (iplr--)
  {
    var i=0;
    while (crew = GetCrew(GetPlayerByIndex(iplr), i++))
      RemoveEffect("IntDirNextCP", crew);
  }
  direction_help_enabled = false;
  return true;
}

// Effect for direction indication for the clonk.
protected func FxIntDirNextCPStart(object target, int fxnum)
{
	var arrow = CreateObject(GUI_GoalArrow, 0, 0, target->GetOwner());
	arrow->SetAction("Show", target);
	EffectVar(0, target, fxnum) = arrow;
	return FX_OK;
}

protected func FxIntDirNextCPTimer(object target, int fxnum)
{
	var plr = target->GetOwner();
	var team = GetPlayerTeam(plr);
	// Find nearest CP.
	var nextcp;
	for (var cp in FindObjects(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Check | PARKOUR_CP_Finish), Sort_Distance(target->GetX() - GetX(), target->GetY() - GetY())))
		if (!cp->ClearedByPlr(plr) && (cp->IsActiveForPlr(plr) || cp->IsActiveForTeam(GetPlayerTeam(plr))))
		{
			nextcp = cp;
			break;
		}	
	if (!nextcp)
		return EffectVar(0, target, fxnum)->SetClrModulation(RGBa(0, 0, 0, 0));
	// Calculate parameters.
	var angle = Angle(target->GetX(), target->GetY(), nextcp->GetX(), nextcp->GetY());
	var dist = Min(510 * ObjectDistance(GetCrew(plr), nextcp) / 400, 510); 
	var red = BoundBy(dist, 0, 255);
	var green = BoundBy(510 - dist, 0, 255);
	var color = RGBa(red, green, 0, 128);
	// Draw arrow.
	EffectVar(0, target, fxnum)->SetR(angle);
	EffectVar(0, target, fxnum)->SetClrModulation(color);
	return FX_OK;
}

protected func FxIntDirNextCPStop(object target, int fxnum)
{
	EffectVar(0, target, fxnum)->RemoveObject();
	return;
}


/*-- Proplist --*/

func Definition(def)
{
	SetProperty("Name", "$Name$", def);
}
