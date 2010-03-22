/*
		Shows an icon for the ringmenu.
		Author: Mimmo
		Overlays:
		0=Grey Circle
		1=Icon of the Object
		5=datakey
		10,11,12: Amount 
*/


local myid;
local amnt;
local data;
local hot;

protected func Construction()
{
	myid = nil;
	// visibility
	this["Visibility"] = VIS_Owner;
}

public func SetSymbol(id obj)
{

	this["Visibility"] = VIS_Owner;
	myid = obj;
		
	if(!myid) 
	{	
		SetGraphics(nil,nil,1);
	}
	else
	{
		SetGraphics(nil,myid,1,4);
		
		SetName(myid->GetName());
	}

}

public func GetAmount() 	{ return amnt; 	}
public func GetExtraData()	{ return data;	}
public func GetSymbol()		{ return myid;	}
public func GetHotkey()		{ return hot;	}
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
		SetObjDrawTransform(300,0,13000,0,300,-30000, 5);
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
	
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;

	if(hun > 0)
	{
		SetGraphics(Format("%d",hun),Icon_Number,10,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s,0,xoffs-spacing*2,0,s,yoffs, 10);
	}
	else
		SetGraphics(nil,nil,10);

	if(ten > 0 || hun > 0)
	{
		SetGraphics(Format("%d",ten),Icon_Number,11,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s,0,xoffs-spacing,0,s,yoffs, 11);
	}
	else
		SetGraphics(nil,nil,11);
		
	SetGraphics(Format("%d",one),Icon_Number,12,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(s,0,xoffs,0,s,yoffs, 12);
}


