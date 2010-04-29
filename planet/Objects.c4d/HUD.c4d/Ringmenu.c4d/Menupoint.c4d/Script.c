/*
		Shows an icon for the ringmenu.
		Author: Mimmo
		Overlays:
		0=Grey Circle
		1=Icon of the Object
		5=hotkey
		10,11,12: Amount 
*/


local myid;
local amnt;
local data;
local hot;
local size;


protected func Construction()
{
	myid = nil;
	// visibility
	this["Visibility"] = VIS_Owner;
	// parallaxity
	this["Parallaxity"] = [0,0];
	size=1000;
}

public func SetSize(int s) // in px *1000
{
	size=s/96;
	SetObjDrawTransform(size,0,0,0,size,0,0);
	SetObjDrawTransform(size,0,0,0,size,0,1);
	SetAmount(GetAmount());
}

public func ResetSize() { SetSize(64000); }


public func SetSymbol(obj)
{

	this["Visibility"] = VIS_Owner;
		
	if(!obj) 
	{	
		SetGraphics(nil,nil,1);
	}
	else
	{
		if (GetType(obj) == C4V_C4Object)
			SetGraphics(nil,nil,1,GFXOV_MODE_ObjectPicture, 0, 0, obj);
		else
			SetGraphics(nil,obj,1,GFXOV_MODE_IngamePicture);
		
		SetName(obj->GetName());
	}
	myid = obj;
}

public func GetAmount()    { return amnt; }
public func GetExtraData() { return data; }
public func GetSymbol()    { return myid; }
public func GetHotkey()    { return hot;  }
public func GetSize()      { return size; }

public func SetHotkey(int hotkey)
{
	if(hotkey > 10 || hotkey <= 0)
	{
		SetGraphics(nil,nil,5);
	}
	else
	{
		hot=hotkey;
		var num = hotkey;
		if(hotkey == 10) num = 0;
		var name = Format("%d",num);
		SetGraphics(name,Icon_Number,5,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(200,0,6500,0,200,-15000, 5);
		SetClrModulation(RGB(160,0,0),5);
	}
}

public func SetExtraData(extradata)
{
	data=extradata;
}
public func SetAmount(Amount)
{

	amnt=Amount;
	if(Amount==1) return ;
	var one = Amount%10;
	var ten = (Amount/10)%10;
	var hun = (Amount/100)%10;
	
	var s = (200*size)/1000;
	var yoffs = (10000*size)/1000;
	var xoffs = (13000*size)/1000;
	var spacing = (5000*size)/1000;
	SetGraphics(Format("10"),Icon_SlimNumber,9,GFXOV_MODE_IngamePicture); //10 == "x"

	SetGraphics(Format("%d",one),Icon_SlimNumber,12,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(s,0,xoffs-spacing-500,0,s,yoffs+300, 9);
	SetObjDrawTransform(s,0,xoffs,0,s,yoffs, 12);

	
		if(ten > 0 || hun > 0)
	{
		SetGraphics(Format("%d",ten),Icon_SlimNumber,11,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s,0,xoffs-spacing*2-500,0,s,yoffs+300, 9);
		SetObjDrawTransform(s,0,xoffs-spacing,0,s,yoffs, 11);

	}
	else
		SetGraphics(nil,nil,11);
		
	if(hun > 0)
	{
		SetGraphics(Format("%d",hun),Icon_SlimNumber,10,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s,0,xoffs-spacing*3-500,0,s,yoffs+300, 9);	
		SetObjDrawTransform(s,0,xoffs-spacing*2,0,s,yoffs, 10);

	}
	else
		SetGraphics(nil,nil,10);
}



