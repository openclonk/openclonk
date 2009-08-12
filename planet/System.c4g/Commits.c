#strict 2

/*-- flgr --*/

// Liefert das Offset zur gewünschten Landscape-X-Position zurück
global func AbsX(int x) { return x - GetX(); }

// Liefert das Offset zur gewünschten Landscape-Y-Position zurück
global func AbsY(int y) { return y - GetY(); }

// Unterstützt negative Werte und kann Zufallszahlen zwischen 2 Werten liefern

global func RandomX(int iStart, int iEnd) {
  var iSwap;
  // Werte vertauscht: trotzdem richtig rechnen
  if(iStart > iEnd) { iSwap=iStart; iStart=iEnd; iEnd=iSwap; }
  // Zufälligen Faktor bestimmen (Differenz plus die null) und iStart aufrechnen
  return Random(iEnd-iStart+1)+iStart;
}

// Setzt die Fertigstellung von this auf iNewCon
global func SetCon(int new_con) {
  return DoCon(new_con - GetCon());
}

// Setzt X- und Y-Dir eines Objektes
global func SetSpeed(int x_dir, int y_dir, int prec) {
  SetXDir(x_dir, prec);
  SetYDir(y_dir, prec);
}

// Setzt X- und Y-Koordinate eines Vertices zugleich
global func SetVertexXY(int index, int x, int y) {
  // Vertices setzen
  SetVertex(index, VTX_X, x);
  SetVertex(index, VTX_Y, y);
}

// Liefert die Anzahl aller feststeckenden Vertices von this zurück
global func VerticesStuck() {
  var vertices;
  var x_offset;
  var y_offset;

  // Vertices durchgehen
  for (var i = -1; i < GetVertexNum(); i++) {
    // solid?
    if (GBackSolid(x_offset + GetVertex(i, VTX_X),
                   y_offset + GetVertex(i, VTX_Y)))
      // hochzählen
      vertices++;
  }
  return vertices;
}

/*-- Joern --*/

//Fügt zum Konto des genannten Spielers iValue Gold hinzu
global func DoWealth(int iPlayer, int iValue)
{
  return SetWealth(iPlayer, iValue + GetWealth(iPlayer));
}

/*-- Tyron --*/


// Prüft ob die angegebene Definition vorhanden ist
global func FindDefinition(id idDef) {
 if(GetDefCoreVal("id","DefCore",idDef)) return true;
}

// Erzeugt amount viele Objekte des Typs id im angegebenen Zielrechteck (optional) im angegeben Material. Gibt die Anzahl der Iterationen zurück, oder -1 wenn die Erzeugung fehlschlägt
global func PlaceObjects(id id,int amount,string strmat,int x,int y,int wdt,int hgt,bool onsf,bool nostuck) {
  var i,j;
  var rndx,rndy,obj;
  var mtype, mat;
  var func, objhgt=GetDefCoreVal("Height","DefCore",id);
  
  mat=Material(strmat);
  // some failsavety
  if(mat==-1)
    if(strmat != "GBackSolid" && strmat != "GBackSemiSolid" && strmat != "GBackLiquid" && strmat != "GBackSky") 
      return -1;
  
  // optional parameters wdt and hgt
  if(!wdt) wdt=LandscapeWidth()-x-GetX();
  if(!hgt) hgt=LandscapeHeight()-y-GetY();

  // cycle-saving method
  if(mat!=-1) 
    while(i<amount) {
      // if there's isn't any or not enough of the given material, break before it gets an endless loop
      if(j++>20000) return -1;
      // destinated rectangle
      rndx=x+Random(wdt);
      rndy=y+Random(hgt);
      // positioning
      if(GetMaterial(rndx,rndy)==mat) {
        // on-surface Option
        if(onsf) while(GBackSemiSolid(rndx, rndy) && rndy>=y) rndy--;
        if(rndy<y) continue;
        // create and verify stuckness ;)
        obj=CreateObject(id,rndx,rndy+objhgt/2,-1);
        obj->SetR(Random(360));
        if(obj->Stuck() || nostuck) i++;
          else obj->RemoveObject();
      }
    }

  if(mat==-1) 
    while(i<amount) {
      // if there's isn't any or not enough of the given material, break before it gets an endless loop
      if(j++>20000) return -1;
      // destinated rectangle
      rndx=x+Random(wdt);
      rndy=y+Random(hgt);
      // positioning
      if(eval(Format("%s(%d,%d)",strmat,rndx,rndy))) {
        // on-surface Option
        if(onsf) while(GBackSemiSolid(rndx, rndy) && rndy>=y) rndy--;
        if(rndy<y)  continue;
        // create and verify stuckness ;)
        obj=CreateObject(id,rndx,rndy+objhgt/2,-1);
        obj->SetR(Random(360));
        if(obj->Stuck() || nostuck) i++;
          else obj->RemoveObject();
      }
    }

  return j;
}

global func CastObjects(iddef,am,lev,x,y,angs,angw) {
	if(!angw) angw = 180;
  for(var i=0; i<am; i++) {
		var obj = CreateObject(iddef,x,y,NO_OWNER);
		var ang = angs + RandomX(-angw/2,angw/2);
		var xdir = xdir=Cos(ang,lev) + RandomX(-3,3);
    obj->SetR(Random(360));
    obj->SetXDir(xdir);
    obj->SetYDir(Sin(ang,lev) + RandomX(-3,3));
    obj->SetRDir((10+Random(21))*xdir/Abs(xdir));
  }
}

global func CastPXS(string mat,int am,int lev,int x,int y,int angs,int angw) {
	if(!angw) angw = 180;
  for(var i=0;i<am;i++) {
		var ang = angs + RandomX(-angw/2,angw/2);
    InsertMaterial(Material(mat),x,y,Cos(ang,lev)+RandomX(-3,3),Sin(ang,lev)+RandomX(-3,3));
  }
}


global func Tan(int iAngle, int iRadius, int iPrec) {
    return  (iRadius * Sin(iAngle,iRadius*100, iPrec)) / Cos(iAngle,iRadius*100, iPrec) ;
}

/*-- Newton -- */

global func CheckVisibility(int iPlr)
{
  var iVisible = this["Visibility"];
  if (GetType(iVisible) == C4V_Array) iVisible = iVisible[0];

  // garnicht sichtbar
  if(iVisible == VIS_None) return false;
  // für jeden sichtbar
  if(iVisible == VIS_All) return true;

  // Objekt gehört dem anggb. Spieler
  if(GetOwner() == iPlr)
    { if(iVisible & VIS_Owner) return true; }
  // Objekt gehört einem Spieler, der iPlr feindlich gesonnen ist
  else if(Hostile(GetOwner(),iPlr))
    { if(iVisible & VIS_Enemies) return true; }
  // Objekt gehört einem Spieler, der iPlr freundlich gesonnen ist
  else
    { if(iVisible & VIS_Allies) return true; }

  if(iVisible & VIS_Select)
    if(this["Visibility"][1+iPlr/32] & 1<<iPlr)
      return true;

  return false;
}

/*-- timi --*/

global func GetPlayerByName(string strPlrName)
{
  // Alle Spieler durchgehen
  var i = GetPlayerCount();
  while (i--)
    // Passt der Spielername mit dem gesuchten überein?
    if (WildcardMatch(GetPlayerName(GetPlayerByIndex(i)), strPlrName))
      // Wenn ja, Spielernummer zurückgeben!
      return GetPlayerByIndex(i);
    
  // Es gibt keinen Spieler, der so heißt!
  return -1;
}
