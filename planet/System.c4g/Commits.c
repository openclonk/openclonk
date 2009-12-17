#strict 2

/*-- flgr --*/

// Returns the offset to x.
global func AbsX(int x) { return x - GetX(); }

// Returns the offset to y.
global func AbsY(int y) { return y - GetY(); }

// Supports negative values, and can deliver random values between two bounds.
global func RandomX(int iStart, int iEnd) {
  var iSwap;
  // Values swapped: reswap them.
  if(iStart > iEnd) { iSwap=iStart; iStart=iEnd; iEnd=iSwap; }
  // Return random factor.
  return Random(iEnd-iStart+1)+iStart;
}

// Sets the completion of this to iNewCon. 
global func SetCon(int new_con) {
  return DoCon(new_con - GetCon());
}

// Does not set the speed of an object. But you can set two components of the velocity vector with this function.
global func SetSpeed(int x_dir, int y_dir, int prec) {
  SetXDir(x_dir, prec);
  SetYDir(y_dir, prec);
}

// Sets both the X and Y-coordinate of one vertex.
global func SetVertexXY(int index, int x, int y) {
  // Set vertices
  SetVertex(index, VTX_X, x);
  SetVertex(index, VTX_Y, y);
}

// Returns the number of stuck vertices. (of this) 
global func VerticesStuck() {
  var vertices;
  var x_offset;
  var y_offset;

  // Loop through vertices
  for (var i = -1; i < GetVertexNum(); i++) {
    // Solid?
    if (GBackSolid(x_offset + GetVertex(i, VTX_X),
                   y_offset + GetVertex(i, VTX_Y)))
      // Count vertices
      vertices++;
  }
  return vertices;
}

/*-- Joern --*/

// Adds iValue to the account of iPlayer.
global func DoWealth(int iPlayer, int iValue)
{
  return SetWealth(iPlayer, iValue + GetWealth(iPlayer));
}

/*-- Tyron --*/


// Checks whether the indicated definition is available.
global func FindDefinition(id idDef) {
 if(GetDefCoreVal("id","DefCore",idDef)) return true;
}

// Creates amount objects of type id inside the indicated rectangle(optional) in the indicated material. 
// Returns the number of iterations needed, or -1 when the placement failed.
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
	if(!angw) angw = 360;
  for(var i=0;i<am;i++) {
		var ang = angs-90 + RandomX(-angw/2,angw/2);
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

  // Not visible at all
  if(iVisible == VIS_None) return false;
  // Visible for all
  if(iVisible == VIS_All) return true;

  // Object is owned by the indicated player
  if(GetOwner() == iPlr)
    { if(iVisible & VIS_Owner) return true; }
  // Object belongs to a player, hostile to iPlr
  else if(Hostile(GetOwner(),iPlr))
    { if(iVisible & VIS_Enemies) return true; }
  // Object belongs to a player, friendly to iPlr
  else
    { if(iVisible & VIS_Allies) return true; }

  if(iVisible & VIS_Select)
    if(this["Visibility"][1+iPlr/32] & 1<<iPlr)
      return true;

  return false;
}

/*-- Ringwaul --*/

global func MaterialDepthCheck(int iX,int iY,string szMaterial,int iDepth)
{
	var iTravelled;
	var iXval = iX;
	var iYval = iY;

	//If iDepth is equal to zero, the function will always measure the depth of the material.
	//If iDepth is not equal to zero, the function will return true if the material is as deep or deeper than iDepth (in pixels).
	if(iDepth==nil) iDepth=LandscapeHeight();

	while(iTravelled!=iDepth) 
	{
		if(GetMaterial(iXval,iYval)==Material(szMaterial)) 
			{
				(iTravelled=++iTravelled);
				(iYval=++iYval); 
			}
		if(GetMaterial(iXval,iYval)!=Material(szMaterial)) return(iTravelled);//returns depth of material
	}
	if(iTravelled==iDepth) return(true);
}

global func SetVelocity(int iAngle, int iSpeed)
{
	// Sets an object's velocity
	var iX= Sin(180-iAngle,iSpeed);
	var iY= Cos(180-iAngle,iSpeed);

	SetXDir(iX);
	SetYDir(iY);
}

global func LaunchProjectile(int iAngle, int iDistance, iSpeed, int iX, int iY, bool iRelativeX)
{
	// iDistance: Distance object travels on angle. Offset from calling object.
	// iX: X offset from container's center
	// iY: Y offset from container's center
	// iRelativeX: if true, makes the X offset relative to container direction. (iX=+30 will become iX=-30 when Clonk turns left. This way offset always stays in front of a Clonk.)

	var iXOffset=Sin(180-iAngle, iDistance);
	var iYOffset=Cos(180-iAngle, iDistance);

	if(Contained()!=nil && iRelativeX==true) { if(Contained()->GetDir()==0) iX=-(iX); }
	if(Contained()!=nil) Exit(iXOffset+iX,iYOffset+iY, iAngle) && SetVelocity(iAngle, iSpeed);
	else SetPosition(GetX()+iXOffset+iX,GetY()+iYOffset+iY) && SetR(iAngle) && SetVelocity(iAngle, iSpeed);
}