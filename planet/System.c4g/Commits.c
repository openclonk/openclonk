#strict 2

/*-- flgr --*/

// Liefert das Offset zur gewünschten Landscape-X-Position zurück
global func AbsX(int x) {
  return x - GetX();
}

// Liefert das Offset zur gewünschten Landscape-Y-Position zurück
global func AbsY(int y) {
  return y - GetY();
}

// Liefert 1 zurück wenn keine der angegeben Konditionen wahr ist
// sollte nicht mehr benutzt werden!
global func Nor(bool con1, bool con2, bool con3, bool con4, bool con5) {
  return !(con1 || con2 || con3 || con4 || con5);
}

// Erzeugt ein Objekt mit der angegeben ID in iMaterial. iRetries ist die Anzahl der Versuche.
global func PlaceInMaterial(id def, int material, int _retries) { // C4ID id, int iMaterial, [int iRetries]
  // Gültiger Materialindex? (-1 ist Sky)
  if (!Inside(material, 0, 127)) return 0;

  // Standardwert von 50000
  var retries = _retries;
  if (!retries) retries = 50000;

  for (var retry_num = 0; retry_num < retries; retry_num++) {
    var x = Random(LandscapeWidth());
    var y = Random(LandscapeHeight());
    if (GetMaterial(x, y) == material)
      return CreateObject(def, x, y, -1);
  }
  return 0;
}

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
global func SetSpeed(int x_dir, int y_dir, object obj) { // int iXDir, int iYDir, [C4Object *pObj]
  // Kein Objekt vorhanden?
  if (!obj && !this) return 0;

  if (!SetXDir(x_dir, obj)) return 0;
  if (!SetYDir(y_dir, obj)) return 0;
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

// Verbindet zwei oder mehrere Strings miteinander und liefert den resultierenden String zurück
global func Concat() {
  var string;

  // Parameter durchgehen
  var par_num = 0;
  var new_string;
  while (new_string = Par(par_num++))
    string = Format("%s%s", string, new_string);

  return string;
}

/*-- Joern --*/

//Objekt hüpft mit der angegebenen Kraft.
global func Bounce(int iPower)
{
  if(!GBackSolid(0,-2)) SetYDir(-Random(iPower));
  if(GBackSolid(0,-2))  SetYDir(Random(10));
  if(GBackSolid(-2,0))  SetXDir(-GetXDir());
  if(GBackSolid(2,0))   SetXDir(-GetXDir());
  return 1;
}
  
//Fügt zum Konto des genannten Spielers iValue Gold hinzu
global func DoWealth(int iPlayer, int iValue)
{
  return SetWealth(iPlayer, iValue + GetWealth(iPlayer));
}
  
//Erstellt für iPlayer ein Crewmember mit der genannten ID zufällig an einer begehbaren Stelle (an Land, nicht im Wasser).
global func RndMakeCrewMember(int iPlayer, id ID)
{
 var anim=PlaceAnimal(WIPF),crw;
 MakeCrewMember(crw=CreateObject( ID,GetX(anim),GetY(anim),iPlayer ), iPlayer);
 RemoveObject(anim);
 return crw;
}

//Objekt Explodiert mehrmals.
global func SemiExplode(int iLevel, int incidence)
{
  for(var i = 0; i < incidence; i++)
    Explode(iLevel,CreateObject(ROCK, 0, 0, GetOwner())); 
  RemoveObject();
  return 1;
}
    
/*-- Tyron --*/

// Wandelt einen DWORD in RGB werte um
// select 0: a, select 1: R, select 2: G, select 3: B
global func GetRGBaValue(val,sel) {
  return (val>>((3-sel)*8)&255);
}

// Abwärtskompatibilität
global func GetRGBValue(val,sel) {
  return (GetRGBaValue(val,sel));
}

// Hiermit lässt sich eine Farbe eines RGB Wertes setzen
// select 0: a, select 1: R, select 2: G, select 3: B
global func SetRGBaValue(int val, int newval, int sel) {
    // Alte Farbe 'löschen'
    val = val&~(255<<((3-sel)*8));
    // Neue hinzufügen
    return (val|newval<<((3-sel)*8));
}

// Diese Funktion verändert die angegeben Farbe eines RGB Wertes
// select 0: a, select 1: R, select 2: G, select 3: B
global func DoRGBaValue(int val, int chng, int sel) {
    return (val + (chng<<((3-sel)*8)));
}

global func SplitRGBaValue(rgb, &red, &green, &blue, &alpha) {
    red=GetRGBaValue(rgb,1);
    green=GetRGBaValue(rgb,2);
    blue=GetRGBaValue(rgb,3);
    alpha=GetRGBaValue(rgb,0);
}

global func HSL2RGB(hsl) {
    var hue=GetRGBaValue(hsl,1), sat=GetRGBaValue(hsl,2),lightness=GetRGBaValue(hsl,3);
    var red, green, blue;
    var var1, var2;
    
    //Log("hue: %d sat: %d lightness: %d",hue,sat, lightness);
    
    if(sat==0) {
        red = green = blue = lightness;
    } else {
        if(lightness<128) var2 = (lightness*(255 + sat))/255;
            else var2 = lightness+sat-lightness*sat/255;
                
        var1 = 2*lightness-var2;
            
        red   = Hue_2_RGB( var1, var2, hue+85);
        green= Hue_2_RGB( var1, var2, hue );
        blue  = Hue_2_RGB( var1, var2, hue-85);
    }
    
    //Log("red: %d green: %d blue: %d",red, green, blue);
    
    return RGB(red, green, blue);
}

global func Hue_2_RGB(var1, var2, hue) {
     if(hue<0) hue+=255;
   if(hue>255) hue-=255;
   if(6*hue<255) return ( var1 + ((var2 - var1) * 6 * hue)/255);
   if(2*hue<255) return ( var2 );
   if(3*hue<510) return ( var1 + ((var2 - var1)*( 510 / 3 - hue )*6)/255);
   return (var1);
}

global func RGB2HSL(rgb) {
    var red=GetRGBaValue(rgb,1), green=GetRGBaValue(rgb,2),blue=GetRGBaValue(rgb,3);
    var min_val = Min(red, Min(green, blue)), max_val = Max(red, Max(green, blue));
    var diff_val = max_val - min_val;
    var lightness = (max_val + min_val)/2;
    var hue, sat, diff_red, diff_green, diff_blue;

    //Log("red: %d green: %d blue: %d",red, green, blue);
    //Log("max_val: %d, min_val: %d",max_val, min_val);
    
    if (diff_val==0) {
   hue=0;                             
   sat=0;
    } else {
        //Log("%d/%d",255*diff_val,510-(max_val+min_val));
        if(lightness<128) sat=(255*diff_val)/(max_val+min_val);
            else sat=(255*diff_val)/(510-(max_val+min_val));

        diff_red  = ((255*(max_val-red  ))/6 + (255*diff_val)/2)/diff_val;
        diff_green= ((255*(max_val-green))/6 + (255*diff_val)/2)/diff_val;
        diff_blue = ((255*(max_val-blue ))/6 + (255*diff_val)/2)/diff_val;
            
        if      (red  ==max_val) hue=diff_blue-diff_green;
        else if (green==max_val) hue=255/3+diff_red-diff_blue;
        else if (blue ==max_val) hue=510/3+diff_green-diff_red;
        
        if (hue<0)   hue+=255;
        if (hue>255) hue-=255;
    }
    
    //Log("hue: %d",hue);
    //Log("sat: %d",sat);
    //Log("lightness: %d",lightness);
    
    return (RGB(hue,sat,lightness));
}

// Prüft ob die angegebene Definition vorhanden ist
global func FindDefinition(id idDef) {
 if(GetDefCoreVal("id","DefCore",idDef)) return 1;
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

// Gleich wie CastObjects nur dass man hier Schleuderwinkel verändern kann
global func CastObjectsX(iddef,am,lev,x,y,angs,angw,callback) {
  var ang, obj, xdir;
  for(var i=0;i<am;i++) {
    ang=angs+Random(angw) - angw/2;
    SetR(Random(360),obj=CreateObject(iddef,x,y,-1));
    SetXDir(xdir=Cos(ang,lev)+RandomX(-3,3),obj);
    SetYDir(Sin(ang,lev)+RandomX(-3,3),obj);
    SetRDir((10+Random(21))*xdir/Abs(xdir),obj);
        if(callback)
            if(callback==-1) GameCall("ObjectCast",obj);
                else Call(callback,obj);
  }
}
  
// Gleich wie CastPXS nur dass man hier Schleuderwinkel verändern kann
global func CastPXSX(string mat,int am,int lev,int x,int y,int angs,int angw) {
  var ang;
  for(var i=0;i<am;i++) {
    ang=angs+Random(angw) - angw/2;
    InsertMaterial(Material(mat),x,y,Cos(ang,lev)+RandomX(-3,3),Sin(ang,lev)+RandomX(-3,3));
  }
}

// selbsterklärend...
global func Tan(int iAngle, int iRadius) {
    return  (iRadius * Sin(iAngle,iRadius*100)) / Cos(iAngle,iRadius*100) ;
}

global func GetMaterialColorX(mat, num) {
  return RGB(GetMaterialColor(mat, num,0),GetMaterialColor(mat, num,1),GetMaterialColor(mat, num,2));
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

global func Visible(int iPlr, object pObj)
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

global func HSL(int h, int s, int l)  { return HSL2RGB(RGB(h,s,l)); }
global func HSLa(int h, int s, int l, int a) { return  HSL2RGB(RGB(h,s,l)) | (a & 255)<<24; }

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