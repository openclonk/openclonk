/*
		Shows an icon for the ringmenu.
		Author: Mimmo, Clonkonaut
*/


local myid;
local amnt;
local data;
local size;

local bg;

static const MI_BG_LAYER = 0;				// background
static const MI_ICON_LAYER = 1;				// icon
static const MI_EXTRASLOT_BG_LAYER = 2;		// background for extraslot
static const MI_EXTRASLOT_LAYER = 3;		// icon for extraslot

											// amount, e.g. x192
static const MI_AMOUNTX_LAYER = 9;			// x
static const MI_AMOUNT100_LAYER = 10;		// 1
static const MI_AMOUNT10_LAYER = 11;		// 9
static const MI_AMOUNT1_LAYER = 12;			// 2

protected func Construction()
{
	bg = true;
	myid = nil;
	// visibility
	this.Visibility = VIS_Owner;
	// parallaxity
	this.Parallaxity = [0,0];
	size=1000;
}

public func SetBG(bool newbg) {
	bg = newbg;
	SetSymbol(GetSymbol());
}

public func SetNothing() // No item, no image, no whatever, just a plain button
{
	SetAmount(nil);
	SetSymbol();
}

public func SetSize(int s) // in px *1000
{
	size=s/96;
	SetObjDrawTransform(size,0,0,0,size,0,MI_BG_LAYER);
	SetSymbol(GetSymbol());
	SetAmount(GetAmount());
}

public func ResetSize() { SetSize(64000); }


public func SetSymbol(obj)
{

	if(bg) {
		SetGraphics(nil,nil,MI_BG_LAYER);
	} else {
		SetGraphics("NoBG",nil,MI_BG_LAYER);
	}

	this.Visibility = VIS_Owner;
		
	if(!obj)
	{
		SetGraphics(nil, nil, MI_ICON_LAYER);
		SetGraphics(nil, nil, MI_EXTRASLOT_BG_LAYER);
		SetGraphics(nil, nil, MI_EXTRASLOT_LAYER);
	}
	else
	{
		if (GetType(obj) == C4V_C4Object)
		{
			SetGraphics(nil, nil, MI_ICON_LAYER, GFXOV_MODE_ObjectPicture, nil, 0, obj);
			SetObjDrawTransform(size, 0, 0, 0, size, 0, MI_ICON_LAYER);
			if (obj->~HasExtraSlot())
			{
				if(bg) {
					SetGraphics(nil, GUI_ExtraSlot, MI_EXTRASLOT_BG_LAYER, GFXOV_MODE_Base);
					SetObjDrawTransform(size, 0, 16*size, 0, size, 16*size, MI_EXTRASLOT_BG_LAYER);
				}
				var content = obj->Contents(0);
				if (content)
				{
					SetGraphics(nil, nil, MI_EXTRASLOT_LAYER, GFXOV_MODE_ObjectPicture, nil, 0, content);
					SetObjDrawTransform(size/3, 0, 16*size, 0, size/3, 16*size, MI_EXTRASLOT_LAYER);
				}
				else
					SetGraphics(nil, nil, MI_EXTRASLOT_LAYER);
			}
			else
			{
				SetGraphics(nil, nil, MI_EXTRASLOT_BG_LAYER);
				SetGraphics(nil, nil, MI_EXTRASLOT_LAYER);
			}
		}
		else
			SetGraphics(nil,obj,MI_ICON_LAYER,GFXOV_MODE_IngamePicture);
		
		SetName(obj->GetName());
	}
	myid = obj;
}

public func GetAmount()    { return amnt; }
public func GetExtraData() { return data; }
public func GetSymbol()    { return myid; }
public func GetSize()      { return size; }

public func SetExtraData(extradata)
{
	data=extradata;
}
public func SetAmount(Amount)
{
	amnt = Amount;
	if (Amount == nil)
	{
		SetGraphics(nil,nil,MI_AMOUNTX_LAYER);
		SetGraphics(nil,nil,MI_AMOUNT1_LAYER);
		SetGraphics(nil,nil,MI_AMOUNT10_LAYER);
		SetGraphics(nil,nil,MI_AMOUNT100_LAYER);	
		return;
	}	
	var one = Amount%10;
	var ten = (Amount/10)%10;
	var hun = (Amount/100)%10;
	
	var s = (200*size)/1000;
	var yoffs = (10000*size)/1000;
	var xoffs = (13000*size)/1000;
	var spacing = (5000*size)/1000;
	SetGraphics(Format("10"),Icon_SlimNumber,MI_AMOUNTX_LAYER,GFXOV_MODE_IngamePicture); //10 == "x"

	SetGraphics(Format("%d",one),Icon_SlimNumber,MI_AMOUNT1_LAYER,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(s,0,xoffs-spacing-500,0,s,yoffs+300, MI_AMOUNTX_LAYER);
	SetObjDrawTransform(s,0,xoffs,0,s,yoffs, MI_AMOUNT1_LAYER);

	
	if(ten > 0 || hun > 0)
	{
		SetGraphics(Format("%d",ten),Icon_SlimNumber,MI_AMOUNT10_LAYER,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s,0,xoffs-spacing*2-500,0,s,yoffs+300, MI_AMOUNTX_LAYER);
		SetObjDrawTransform(s,0,xoffs-spacing,0,s,yoffs, MI_AMOUNT10_LAYER);

	}
	else
		SetGraphics(nil,nil,MI_AMOUNT10_LAYER);
		
	if(hun > 0)
	{
		SetGraphics(Format("%d",hun),Icon_SlimNumber,MI_AMOUNT100_LAYER,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s,0,xoffs-spacing*3-500,0,s,yoffs+300, MI_AMOUNTX_LAYER);
		SetObjDrawTransform(s,0,xoffs-spacing*2,0,s,yoffs, MI_AMOUNT100_LAYER);

	}
	else
		SetGraphics(nil,nil,MI_AMOUNT100_LAYER);
}