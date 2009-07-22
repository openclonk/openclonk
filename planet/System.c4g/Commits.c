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

// Setzt die Fertigstellung von *pObj auf iNewCon
global func SetCon(int new_con, object obj) { // int iNewCon, [C4Object *pObj]
  // Kein Objekt vorhanden?
  if (!obj && !this) return 0;

  return DoCon(new_con - GetCon(obj), obj);
}

// Setzt X- und Y-Dir eines Objektes
global func SetSpeed(int x_dir, int y_dir, object obj, int prec) { // int iXDir, int iYDir, [C4Object *pObj]
  // Kein Objekt vorhanden?
  if (!obj && !this) return 0;

  if (!SetXDir(x_dir, obj, prec)) return 0;
  if (!SetYDir(y_dir, obj, prec)) return 0;
  return 1;
}

// Setzt X- und Y-Koordinate eines Vertices zugleich
global func SetVertexXY(int index, int x, int y, object obj) { // int iIndex, int iXPos, int iYPos, [C4Object *pObj]
  // Kein Objekt vorhanden?
  if (!obj && !this) return 0;

  // Vertices setzen
  if (!SetVertex(index, 0, x, obj)) return 0;
  if (!SetVertex(index, 1, y, obj)) return 0;
  return 1;
}

// Liefert die Anzahl aller feststeckenden Vertices von *pObj zurück
global func VerticesStuck(object obj) { // [C4Object *pObj]
  // Kein Objekt vorhanden?
  if (!obj && !this) return 0; 
 
  var vertices;
  var x_offset;
  var y_offset;

 // Offset zum Objekt anpassen
 if (obj) {
   x_offset = AbsX(GetX(obj));
   y_offset = AbsY(GetY(obj));
 }

  // Vertices durchgehen
  for (var i = -1; i < GetVertexNum(obj); i++) {
    // solid?
    if (GBackSolid(x_offset + GetVertex(i, 0, obj),
                   y_offset + GetVertex(i, 1, obj)))
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
        SetR(Random(360),obj);
        if(Stuck(obj) || nostuck) i++;
          else RemoveObject(obj);
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
        SetR(Random(360),obj);
        if(Stuck(obj) || nostuck) i++;
          else RemoveObject(obj);
      }
    }

  return j;
}

// Erzeugt eine Regenwolke
global func LaunchRain(int x, int mat, int wdt, int lev) {
  var obj=CreateObject(FXP1,x-GetX(),-GetY(),-1);
  if(!obj) return 0;
  obj->Activate(mat,wdt,lev);
  return obj;
}

// Erzeugt einen Vulkan
global func LaunchVolcano(int x, int y, int strength, string mat) {
  var obj=CreateObject(FXV1,x-GetX(),y-GetY());
  if(!obj) return 0;
  // old 'LaunchVolcano (int iX);' Function
  if(!y) y=LandscapeHeight()-1;
  if(!strength) strength=BoundBy(15*LandscapeHeight()/500+Random(10),10,60);
  if(!mat) mat="Lava";

  obj->Activate(x,y,strength,Material(mat));
  return 1;
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

/*-- Roscher --*/

// Wie FindConstructionSite, arbeitet jedoch mit Referenzen auf zwei benannte Variablen

global func FindConstructionSiteX(id idDef, &iVarX, &iVarY)
{
  Var(0) = iVarX; Var(1) = iVarY;
  if (!FindConstructionSite(idDef, 0, 1)) return 0;
  iVarX = Var(0); iVarY = Var(1);
  return 1;
}

/*-- Newton -- */

global func CheckVisibility(int iPlr, object pObj)
{
  var iVisible = GetVisibility(pObj);

  // garnicht sichtbar
  if(iVisible == VIS_None) return false;
  // für jeden sichtbar
  if(iVisible == VIS_All) return true;

  // Objekt gehört dem anggb. Spieler
  if(GetOwner(pObj) == iPlr)
    { if(iVisible & VIS_Owner) return true; }
  // Objekt gehört einem Spieler, der iPlr feindlich gesonnen ist
  else if(Hostile(GetOwner(pObj),iPlr))
    { if(iVisible & VIS_Enemies) return true; }
  // Objekt gehört einem Spieler, der iPlr freundlich gesonnen ist
  else
    { if(iVisible & VIS_Allies) return true; }

  if(iVisible & VIS_Local)
    if(Local(iPlr/32,pObj) & 1<<iPlr)
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
