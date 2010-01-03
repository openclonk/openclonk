/*-- Spielzielsteuerung --*/

// Bei allen C4D_Goal-Objekten zu importieren
// Überladbare Funktionen:
// int IsFullfilled(); - Ist das Ziel erfüllt?
// Statt Initialize InitGoal benutzen

local missionPassword; // Missionspasswort, das beim Erfüllen des Spielziels gesetzt wird

// Initialisierung
func Initialize()
{
  // GOAL selber sollte nicht erzeugt werden
  if (GetID()==GOAL)
  {
    Log("WARNING: Abstract GOAL object should not be created; object removed.");
    return RemoveObject();
  }
  // Timer erstellen, wenn er noch nicht existiert
  RecheckGoalTimer();
  // Fertig
  return _inherited(...);
}

func UpdateTransferZone()
{
  // Timer erstellen, wenn er noch nicht existiert
  RecheckGoalTimer();
  return _inherited(...);
}

func RecheckGoalTimer()
{
  // Timer erstellen, wenn er noch nicht existiert
  if (!GetEffect("IntGoalCheck", 0))
  {
    var timer_interval = 35;
    if(GetLeague()) timer_interval = 2; // Liga check das Ziel haeufiger
    AddEffect("IntGoalCheck", 0, 1, timer_interval, 0);
  }
}

global func FxIntGoalCheckTimer(object trg, int num, int time)
{
  var curr_goal = EffectVar(0, trg, num);
  // Momentanes Zielobjekt prüfen
  if (curr_goal && (curr_goal->GetCategory() & C4D_Goal))
    if (!curr_goal->~IsFulfilled()) 
      return true;
  // Jetzt in der Schleife suchen
  var goal_count = 0;
  for (curr_goal in FindObjects(Find_Category(C4D_Goal))) if (curr_goal)
  {
    ++goal_count;
    if (!curr_goal->~IsFulfilled())
    {
      EffectVar(0, trg, num) = curr_goal;
      return true;
    }
  }
  // Kein Zielobjekt? Wir sind überflüssig :(
  if (!goal_count) return FX_Execute_Kill;
  // Spiel zuende
  AllGoalsFulfilled();
  return FX_Execute_Kill;
}

global func AllGoalsFulfilled()
{
  // Spielziele erfüllt: Missionspasswort setzen
  for (var goal in FindObjects(Find_Category(C4D_Goal)))
    if (goal->LocalN("missionPassword"))
      GainMissionAccess(goal->LocalN("missionPassword"));
  // Ziel erfüllt: Vom Szenario abgefangen?
  if (GameCall("OnGoalsFulfilled")) return true;
  // Tja, jetzt ist das Spiel vorbei. Erstmal die Belohnung
  Sound("Fanfare", true);
  AddEffect("IntGoalDone", 0, 1, 30, 0);
}

global func FxIntGoalDoneStop()
{
  GameOver();
}





// Setzt das Missionspasswort, welches von diesem Spielziel bei Erfüllung beim Spieler eingetragen wird.

public func SetMissionAccess(string strPassword)
{
  missionPassword = strPassword;  
}

// Basis-Implementationen - diese sollten in abgeleiteten Spielziel-Objekten überladen werden

public func IsFulfilled() { return true; }


protected func Activate(iPlr)
{
  if (IsFulfilled()) return(MessageWindow("$MsgGoalFulfilled$",iPlr));
  return MessageWindow(GetDesc(),iPlr);
}
