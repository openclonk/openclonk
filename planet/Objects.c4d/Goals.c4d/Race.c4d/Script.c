/*-- Wettlauf --*/

#include GOAL

local fWon;
local iLeader;
local iTeamCount;
local fTeamTie;
local player_deaths, team_deaths;

// Funktionen zum Überladen
// 1: links->rechts
// 2: rechts->links
// 3: unten->oben
// 4: oben->unten
public func GetDirection() { var i; if(i = GameCall("GetRACEDirection")) return i; return 1; }
public func GetStartOffset() { var i; if(i = GameCall("GetRACEStartOffset")) return i; return 50; }
public func GetEndOffset() { var i; if(i = GameCall("GetRACEEndOffset")) return i; return 50; }
public func CheckGoal(iPlr)
{
  var i;
  // Achtung! Die Szenarienfunktion CeckRACEGoal sollte -1 zurückgeben,
  // wenn das Zeil nicht erfüllt ist und 1 wenn es erfüllt ist
  if(i = GameCall("CheckRACEGoal", iPlr)) return(i);
  if (!GetCursor(iPlr)) return false;
  if(GetDirection()==1) return(GetCursor(iPlr)->GetX() > LandscapeWidth() - GetEndOffset());
  if(GetDirection()==2) return(GetCursor(iPlr)->GetX() < GetEndOffset());
  if(GetDirection()==3) return(GetCursor(iPlr)->GetY() < GetEndOffset());
  if(GetDirection()==4) return(GetCursor(iPlr)->GetY() > LandscapeHeight() - GetEndOffset());
}

func ScoreboardCol(id colid)
{
  if (colid == RACE) return 1;
  if (colid == DEAD) return 2;
  return 3;
}

public func CheckTeamGoal(iTeam)
{
  var iBest, iPlr, iWay;
  // Alle Spieler durchgehen
  for(var i = 0; i < GetPlayerCount(); i++)
  {
    iPlr = GetPlayerByIndex(i);
    // Nur welche vom gleichen Team suchen
    if(GetPlayerTeam(iPlr)!=iTeam) continue;
    // Hat es schon einer geschafft?
    if(CheckGoal(iPlr)==1) return true;
  }
  return false;
}

// Locals: Tode des Spielers
private func & PlayerDeaths(int iPlr)
{
  if (!player_deaths) player_deaths = [];
  return player_deaths[iPlr];
}

private func & TeamDeaths(int iTeam)
{
  if (!team_deaths) team_deaths = [];
  return team_deaths[iTeam];
}

static const SCOREBOARD_NAME = "Scoreboard";

protected func Initialize()
{
  // Alle Neustart-Objekte entfernen und eines neu erzeugen,
  // damit dieses als erstes in der Liste der Spielregeln auftaucht
  Schedule("RemoveAll(Find_ID(RSTR))", 1);
  Schedule("CreateObject(RSTR, 10, 10, -1)", 2);
  // Scoreboard-Initialisierungen
  SetScoreboardData(SBRD_Caption, SBRD_Caption, SCOREBOARD_NAME, SBRD_Caption);
  SetScoreboardData(SBRD_Caption, ScoreboardCol(RACE), "{{RACE}}", ScoreboardCol(RACE));
  SetScoreboardData(SBRD_Caption, ScoreboardCol(DEAD), "{{SKUL}}", ScoreboardCol(DEAD));
  Schedule("DoScoreboardShow(1)", 1);
  return inherited(...);
}

public func InitializePlayer(int iPlr, int x, int y, object pBase, int iTeam)
{
  // Ist das Spiel schon aus? Dann gleich eliminieren
  if(fWon) return(EliminatePlayer(iPlr));
  // Scoreboard aktualisieren:
  if (iTeam)
  {
    // Höherer Teamindex? Merken
    if(iTeamCount < iTeam) iTeamCount = iTeam;
    // den Teamnamen eintragen
    SetScoreboardData(iTeam,       SBRD_Caption, GetSmartTeamName(iTeam), iTeam);
    SetScoreboardData(iTeam,       ScoreboardCol(RACE), Format("%d%",GetTeamWayPercent(iTeam)), GetTeamWayPercent(iTeam));
    SetScoreboardData(iTeam,       ScoreboardCol(DEAD), Format("%d",TeamDeaths(iTeam)), TeamDeaths(iTeam));
  }
  else
  {
    // den Spielernamen eintragen
    var iPlrID = GetPlayerID(iPlr);
    SetScoreboardData(iPlrID,       SBRD_Caption, GetTaggedPlayerName(iPlr), iPlrID);
    SetScoreboardData(iPlrID,       ScoreboardCol(RACE),       Format("%d%",0), 0);
    SetScoreboardData(iPlrID,       ScoreboardCol(DEAD),       Format("%d",0), 0);
    PlayerDeaths(iPlr) = 0;
  }
  
  // Board anzeigen
  DoScoreboardShow(1, iPlr+1);

  // Mit keinem Spielern verfeindet sein
  for(var i = 0; i < GetPlayerCount(); i++)
  {
    SetHostility(iPlr, GetPlayerByIndex(i), 0, 1);
    SetHostility(GetPlayerByIndex(i), iPlr, 0, 1);
  }
  return(true);
}

public func RemovePlayer(int iPlr)
{
  // Beim Spielende Anzeigen stehenlassen
  if(fWon) return;
  // Teamanzeige nicht ausleeren
  if(GetPlayerTeam(iPlr)) return;
  // Anzeige leeren
  var iPlrID = GetPlayerID(iPlr);
  SetScoreboardData(iPlrID,       ScoreboardCol(RACE),       0 ,0);
  SetScoreboardData(iPlrID,       ScoreboardCol(DEAD),       0 ,0);
  PlayerDeaths(iPlr) = 0;
  return(true);
}

public func OnClonkDeath(object pClonk)
{
  // Nur wenn der Spieler noch existiert
  if (!pClonk) return;
  if(!GetPlayerName(pClonk->GetOwner())) return;
  // Nur Clonks der Crew zählen
  if(pClonk->GetName() == pClonk->GetID()->GetName()) return;
  var iDeathCount, iTeam = GetPlayerTeam(pClonk->GetOwner());
  if(GetPlayerTeam(pClonk->GetOwner()))
  {
    iDeathCount = ++TeamDeaths(iTeam);
    SetScoreboardData(iTeam,       ScoreboardCol(DEAD), Format("%d",iDeathCount), iDeathCount);
    return(true);
  }
  var iPlrID = GetPlayerID(pClonk->GetOwner()), iColID = ScoreboardCol(GetID());
  // Todzähler aktualisieren  
  iDeathCount = ++PlayerDeaths(pClonk->GetOwner());
  // Scoreboard aktualisieren
  SetScoreboardData(iPlrID,       ScoreboardCol(DEAD),       Format("%d",iDeathCount), iDeathCount);
  return(true);
}

public func GetTeamWayPercent(iTeam)
{
  var iBest, iPlr, iWay;
  // Alle Spieler durchgehen
  for(var i = 0; i < GetPlayerCount(); i++)
  {
    iPlr = GetPlayerByIndex(i);
    // Nur welche vom gleichen Team suchen
    if(GetPlayerTeam(iPlr)!=iTeam) continue;
    // Den Besten rausfinden
    iWay = GetWayPercent(iPlr);
    if(iWay > iBest) iBest = iWay;
  }
  return(iBest);
}

public func GetWayPercent(iPlr)
{
  // Wenn das Ziel erfüllt ist 100%
  if(CheckGoal(iPlr)==1) return 100;
  if (!GetCursor(iPlr)) return 0;
  // Wenn das Szenarioscript eine eigene Berechnung hat
  var i;
  if(i = GameCall("GetRACEWayPercent", iPlr)) return(BoundBy(i, 0, 99));
  // Ansonsten je nach Wegstrecke 0%-99%
  if(GetDirection()==1)
    return(BoundBy((GetCursor(iPlr)->GetX()-GetStartOffset())*100/(LandscapeWidth()-GetEndOffset()-GetStartOffset()), 0, 99));
  if(GetDirection()==2)
    return(BoundBy((LandscapeWidth()-GetCursor(iPlr)->GetX()-GetStartOffset())*100/(LandscapeWidth()-GetEndOffset()-GetStartOffset()), 0, 99));
  if(GetDirection()==3)
    return(BoundBy((LandscapeHeight()-GetCursor(iPlr)->GetY()-GetStartOffset())*100/(LandscapeHeight()-GetEndOffset()-GetStartOffset()), 0, 99));
  if(GetDirection()==4)
    return(BoundBy((GetCursor(iPlr)->GetY()-GetStartOffset())*100/(LandscapeHeight()-GetEndOffset()-GetStartOffset()), 0, 99));
}

public func UpdateScoreboard()
{
  if(fWon) return;
  var iPlr, iWay, iOldWay;
  // Bei Teamplay nur die Teams eintragen
  if(iTeamCount)
  {
    for(var iTeam = 1; iTeam <= iTeamCount; iTeam++)
    {
      iWay = GetTeamWayPercent(iTeam);
      // Weitestes Team finden
      if(!iOldWay || iOldWay<iWay)
      {
        iOldWay = iWay;
        iLeader = iTeam;
      }
      // Daten eintragen
      SetScoreboardData(iTeam,       ScoreboardCol(RACE),       Format("%3d%", iWay), iWay);
      SetScoreboardData(iTeam,       ScoreboardCol(DEAD),       Format("%d", TeamDeaths(iTeam)), TeamDeaths(iTeam));
      // Hat schon jemand gewonnen?
      if(CheckTeamGoal(iTeam)==1)
      {
        // Mit Wettstreit gewinnt nur der im Ziel
        if(ObjectCount(Find_ID(RVLR))) EliminatePlayers();
        else fWon = GetPlayerCount();
      }
    }  
    return;
  }
  // alle Spieler durchgehen
  for(var i = 0; i < GetPlayerCount(); i++)
  {
    iPlr = GetPlayerByIndex(i);
    // Keinen Cursor? Dann ist er bestimmt noch in der Teamauswahl
    if(!GetCursor(iPlr)) continue;
    iWay = GetWayPercent(iPlr);
    // Weitesten Spieler finden
    if(!iOldWay || iOldWay<iWay)
    {
      iOldWay = iWay;
      iLeader = iPlr;
    }
    // Daten eintragen
    SetScoreboardData(GetPlayerID(iPlr),       ScoreboardCol(RACE),       Format("%3d%", iWay), iWay);
    SetScoreboardData(GetPlayerID(iPlr),       ScoreboardCol(DEAD),       Format("%d", PlayerDeaths(iPlr)), PlayerDeaths(iPlr));
    // Hat schon jemand gewonnen?
    if(CheckGoal(iPlr)==1)
    {
      // Mit Wettstreit gewinnt nur der im Ziel
      if(ObjectCount(Find_ID(RVLR))) EliminatePlayers();
      else fWon = GetPlayerCount();
    }
  }
  // Sortieren nach Frags
  SortScoreboard(ScoreboardCol(DEAD), false);
  SortScoreboard(ScoreboardCol(RACE), true);
  return(true);
}

public func EliminatePlayers()
{
  var iPlr, szWinners, fHasWon;
  // alle Spieler durchgehen
  for(var i = 0; i < GetPlayerCount(); i++)
  {
    iPlr = GetPlayerByIndex(i);
    if(iTeamCount) fHasWon = CheckTeamGoal(GetPlayerTeam(iPlr));
    else fHasWon = CheckGoal(iPlr);
    if(fHasWon != 1)
      EliminatePlayer(iPlr);
    else 
    {
      fWon++;
      if(iTeamCount) if(GetPlayerTeam(iPlr)!=iLeader) fTeamTie = 1;
      if(!szWinners) szWinners = Format("%s", GetPlayerName(iPlr));
      else szWinners = Format("%s, %s", szWinners, GetPlayerName(iPlr));
    }
  }
  if(iTeamCount)
  {
    if(fTeamTie) Message ("$MsgTie$", 0);
    else Message ("$MsgTeamWinner$", 0, GetSmartTeamName(iLeader), szWinners);
  }
  else Message ("$MsgWinner$", 0, szWinners);
}

func IsFulfilled () {
  if(!fWon) return;
  // Das Ziel ist erfüllt, wenn alle Spieler die verloren haben eliminiert sind
  // denn sonst könnte es passieren, das das GameOver vor der Eliminierung kommt
  return(GetPlayerCount()<=fWon);
}

func Activate (iPlr) {
  // Nachricht nach Erfüllung ausgeben
  if(iTeamCount && fWon)
  {
    if(fTeamTie) return(MessageWindow ("$MsgTie$", iPlr));
    else if (iLeader == GetPlayerTeam(iPlr)) return(MessageWindow ("$MsgYourTeamWon$", iPlr));
    else return(MessageWindow (Format("$MsgOtherTeamWon$", GetSmartTeamName(iLeader)), iPlr));
  }
  else if(fWon==1)
  {
    if (iLeader == iPlr) return(MessageWindow ("$MsgYouWon$", iPlr));
    else return(MessageWindow (Format("$MsgOtherWon$", iLeader), iPlr));
  }
  else if(fWon>1) return(MessageWindow ("$MsgTie$", iPlr));
  return (MessageWindow (GetFulfillText (iPlr), iPlr));
}

func GetFulfillText (int iPlayer) {
  if(iTeamCount)
  {
    // Kein Team?!
    if(!iLeader) return("");
    // Fortschritt zeigen
    if (iLeader == GetPlayerTeam(iPlayer)) return(Format("$MsgYourTeamAhead$", GetTeamWayPercent(iLeader)));
    return(Format("$MsgOtherTeamAhead$",GetSmartTeamName(iLeader),GetTeamWayPercent(iLeader),GetTeamWayPercent(GetPlayerTeam(iPlayer))));
  }
  // kein Clonk im Spiel??!
  if (!GetPlayerName(iLeader) || !GetCursor(iLeader)) return("");
  // Fortschritt zeigen
  if (iLeader == iPlayer) return(Format("$MsgYouAhead$", GetWayPercent(iLeader)));
  return(Format("$MsgOtherAhead$",GetCursor(iLeader)->GetName(),GetPlayerName(iLeader),GetWayPercent(iLeader),GetWayPercent(iPlayer)));
}

// Gibt für unbenannte Teams den Spieleranamen aus
private func GetSmartTeamName(int iTeam)
  {
  var sTeamName;
  if (!GetTeamConfig(TEAM_AutoGenerateTeams))
    {
    // Vom Szenario vorgegebene Teams: Original-Teamnamen benutzen
    sTeamName = GetTeamName(iTeam);
    }
  else
    {
    // Automatische Teams: Diese haben keinen ordentlichen Namen. Aus erstem Mitgliedsnamen zusammenbauen
    for (var i, cnt=GetPlayerCount(); i<cnt; ++i)
      {
      var iPlr = GetPlayerByIndex(i);
      if (GetPlayerTeam(iPlr) == iTeam)
        {
        // Teamname als Spielername; gefärbt wenn nicht ohnehin nach Team gefärbt wird
        var sTeamPlrName;
        if (GetTeamConfig(TEAM_TeamColors))
          sTeamPlrName = GetPlayerName(iPlr);
        else
          sTeamPlrName = GetTaggedPlayerName(iPlr);
        if (!sTeamName) sTeamName = sTeamPlrName; else sTeamName = Format("%s, %s", sTeamName, sTeamPlrName);
        }
      }
    }
  // Bei Teamfarben auch diese einbeziehen
  if (GetTeamConfig(TEAM_TeamColors))
    return Format("<c %x>%s</c>", GetTeamColor(iTeam), sTeamName);
  else
    return sTeamName;
  }



func Definition(def) {
  SetProperty("Name", "$NameRace$", def);
}
